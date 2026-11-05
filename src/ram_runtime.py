#!/usr/bin/env python3
"""Runtime WASM geometry: single grid, executable transforms, skull/crown encoding."""
import argparse
import json
import pathlib
import re
import struct

ZERO, ONE = '💀', '👑'

def uleb(n):
    if n < 0: raise ValueError('negative uleb')
    out = bytearray()
    while True:
        b = n & 127; n >>= 7
        out.append(b | (128 if n else 0))
        if not n: return bytes(out)

def sleb(n):
    out = bytearray()
    while True:
        b = n & 127; n >>= 7
        stop = (n == 0 and not b & 64) or (n == -1 and bool(b & 64))
        out.append(b if stop else b | 128)
        if stop: return bytes(out)

def section(i, data): return bytes([i]) + uleb(len(data)) + data

def vector(items): return uleb(len(items)) + b''.join(items)

def encode(data): return ''.join(ONE if b & (1 << shift) else ZERO for b in data for shift in range(7,-1,-1))

def decode(s):
    chars = [c for c in s if not c.isspace()]
    if len(chars) % 8 or any(c not in (ZERO, ONE) for c in chars): raise ValueError('invalid skull/crown byte stream')
    return bytes(sum((c == ONE) << (7-i) for i,c in enumerate(chars[j:j+8])) for j in range(0,len(chars),8))

def parse(source):
    shape = None; points = []; ops = []; name = None
    for lineno, raw in enumerate(source.splitlines(),1):
        t = raw.split('#',1)[0].split()
        if not t: continue
        if t[:3] == ['🌀','📐','GRID']:
            if shape is not None or len(t)!=8 or not re.fullmatch('[A-Za-z_][A-Za-z_0-9]*',t[3]): raise ValueError(f'line {lineno}: invalid grid')
            name=t[3]
            try: nx,ny,nz,spacing=map(int,t[4:])
            except ValueError: raise ValueError(f'line {lineno}: invalid integers') from None
            if min(nx,ny,nz,spacing)<=0 or nx*ny*nz>4096 or max(nx,ny,nz,spacing)>10000: raise ValueError('invalid dimensions')
            shape=[nx,ny,nz]
            points=[(x*spacing,y*spacing,z*spacing) for z in range(nz) for y in range(ny) for x in range(nx)]
        elif t[0]=='>>>>>>>€':
            # Assertions checked against simulated runtime steps; also tested against actual WASM.
            if shape is None: raise ValueError('assertion before grid')
            if t[1:2]==['SHAPE'] and len(t)==5:
                if list(map(int,t[2:]))!=shape: raise ValueError(f'line {lineno}: shape assertion failed')
            elif t[1:2]==['POINT'] and len(t)==6:
                idx=int(t[2]); expected=tuple(map(int,t[3:]))
                if idx<0 or idx>=len(points) or points[idx]!=expected: raise ValueError(f'line {lineno}: point assertion failed')
                ops.append(('assert',idx,expected))
            else: raise ValueError(f'line {lineno}: invalid assertion')
        else:
            inverse=t[0]=='↶'; k=t[1:] if inverse else t
            if shape is None or len(k)<4 or k[:2]!=['⚡','@@'] or k[2]!=name: raise ValueError(f'line {lineno}: invalid transform')
            op=k[3]
            try: args=list(map(int,k[4:]))
            except ValueError: raise ValueError(f'line {lineno}: invalid transform integer') from None
            if op=='ROTATE_Z' and len(args)==1 and args[0]%90==0:
                turns=(((-1 if inverse else 1)*args[0])//90)%4
                for _ in range(turns): points=[(-y,x,z) for x,y,z in points]
                ops.append(('rotate',turns))
            elif op=='TRANSLATE' and len(args)==3:
                delta=tuple((-v if inverse else v) for v in args)
                points=[tuple(a+b for a,b in zip(p,delta)) for p in points]
                ops.append(('translate',*delta))
            else: raise ValueError(f'line {lineno}: unsupported transform')
            if any(abs(v)>2**30 for p in points for v in p): raise ValueError('coordinate overflow')
    if shape is None: raise ValueError('no grid')
    # Original grid before transforms, WASM performs every operation at runtime.
    nx,ny,nz=shape
    spacing=next((int(raw.split()[7]) for raw in source.splitlines() if raw.strip().startswith('🌀 📐 GRID')),1)
    initial=[(x*spacing,y*spacing,z*spacing) for z in range(nz) for y in range(ny) for x in range(nx)]
    return shape, initial, ops, points

def compile_program(source):
    shape, initial, ops, expected = parse(source)
    count=len(initial); memory=bytearray(b''.join(struct.pack('<iii',*p) for p in initial))
    # WASM run(): apply operations in source order; assert-point checks return -1 on failure.
    # Locals: i, x, y, z. i32 indices 0..3.
    instructions=[]
    def emit(wat, data): instructions.append((wat,data))
    def lc(n): emit(f'local.get {n}',b'\x20'+uleb(n))
    def ls(n): emit(f'local.set {n}',b'\x21'+uleb(n))
    def cn(n): emit(f'i32.const {n}',b'\x41'+sleb(n))
    def simple(name,code): emit(name,bytes([code]))
    def load(offset=0): emit(f'i32.load offset={offset}',b'\x28'+uleb(2)+uleb(offset))
    def store(offset=0): emit(f'i32.store offset={offset}',b'\x36'+uleb(2)+uleb(offset))
    def addr(offset): lc(0); cn(12); simple('i32.mul',0x6c); cn(offset); simple('i32.add',0x6a)
    for op in ops:
        if op[0]=='assert':
            _,idx,xyz=op
            for axis,value in enumerate(xyz):
                cn(idx*12+axis*4); load(); cn(value); simple('i32.ne',0x47)
                emit('if',b'\x04\x40'); cn(-1); simple('return',0x0f); simple('end',0x0b)
            continue
        if op[0]=='rotate' and op[1]==0: continue
        cn(0); ls(0)
        emit('block',b'\x02\x40'); emit('loop',b'\x03\x40')
        lc(0); cn(count); simple('i32.ge_u',0x4f); emit('br_if 1',b'\x0d\x01')
        for axis in range(3): addr(axis*4); load(); ls(axis+1)
        if op[0]=='rotate':
            for _ in range(op[1]):
                lc(2); cn(-1); simple('i32.mul',0x6c); ls(3) # z temporarily holds -y
                lc(1); ls(2); lc(3); ls(1)
            # z must be preserved, rotations only use x/y
            # recover z from original memory below
            addr(8); load(); ls(3)
        else:
            for axis,delta in enumerate(op[1:]):
                lc(axis+1); cn(delta); simple('i32.add',0x6a); ls(axis+1)
        for axis in range(3): addr(axis*4); lc(axis+1); store()
        lc(0); cn(1); simple('i32.add',0x6a); ls(0)
        emit('br 0',b'\x0c\x00'); simple('end',0x0b); simple('end',0x0b)
    cn(count); simple('end',0x0b)
    code=b''.join(data for _,data in instructions)
    body=uleb(1)+uleb(4)+b'\x7f'+code
    types=section(1,vector([b'\x60\x00\x01\x7f']))
    funcs=section(3,vector([uleb(0)]))
    mem=section(5,vector([b'\x00'+uleb(max(1,(len(memory)+65535)//65536))]))
    exports=section(7,vector([uleb(6)+b'memory'+b'\x02'+uleb(0),uleb(3)+b'run'+b'\x00'+uleb(0)]))
    code_section=section(10,vector([uleb(len(body))+body]))
    data_section=section(11,vector([b'\x00\x41\x00\x0b'+uleb(len(memory))+memory]))
    wasm=b'\x00asm\x01\x00\x00\x00'+types+funcs+mem+exports+code_section+data_section
    escaped=''.join(f'\\{b:02x}' for b in memory)
    wat='(module\n  (memory (export "memory") '+str(max(1,(len(memory)+65535)//65536))+')\n  (data (i32.const 0) "'+escaped+'")\n  (func (export "run") (result i32)\n    (local i32 i32 i32 i32)\n'+''.join('    '+w+'\n' for w,_ in instructions)+'  )\n)\n'
    manifest={'format':'ram-runtime-v1','shape':shape,'point_count':count,'initial_points':initial,'expected_points':expected,'operations':ops,'bytes':len(memory)}
    return wasm,wat,manifest

def main():
    p=argparse.ArgumentParser(); p.add_argument('source',type=pathlib.Path,nargs='?'); p.add_argument('-o','--output',type=pathlib.Path); p.add_argument('--encode',type=pathlib.Path); p.add_argument('--decode',type=pathlib.Path)
    a=p.parse_args()
    if a.encode:
        if not a.output: p.error('output required')
        a.output.write_text(encode(a.encode.read_bytes())+'\n',encoding='utf8'); return
    if a.decode:
        if not a.output: p.error('output required')
        a.output.write_bytes(decode(a.decode.read_text(encoding='utf8'))); return
    if not a.source or not a.output: p.error('source and output required')
    wasm,wat,manifest=compile_program(a.source.read_text(encoding='utf8'))
    a.output.write_bytes(wasm)
    a.output.with_suffix('.wat').write_text(wat,encoding='utf8')
    a.output.with_suffix('.skull').write_text(encode(wasm)+'\n',encoding='utf8')
    a.output.with_suffix('.flat.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
    print(f'{len(wasm)} wasm bytes, {manifest["point_count"]} runtime points')
if __name__=='__main__': main()
