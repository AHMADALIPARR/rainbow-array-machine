#!/usr/bin/env python3
"""Runtime WASM lowering for the registered vector and affine Wolfram subsets."""
import argparse
import json
import math
import pathlib
import struct
from ram_runtime import uleb, sleb, section, vector, encode

OPS = {'➕': (0xa0, 'f64.add'), '➖': (0xa1, 'f64.sub'), '✖️': (0xa2, 'f64.mul')}
LANG = {'🌀': 'BQN', '🔺': 'APL', '⚡': 'K'}

def compile_source(source):
    lines = [(i, s.split('#', 1)[0].split()) for i, s in enumerate(source.splitlines(), 1)]
    lines = [(i, t) for i, t in lines if t]
    if len(lines) != 1:
        raise ValueError('exactly one executable statement required')
    line, t = lines[0]
    memory = b''
    if t[:3] == ['🐺', '🔹', '📜']:
        if len(t) != 6:
            raise ValueError(f'line {line}: expected three integer coefficients')
        a, b, c = map(int, t[3:])
        if any(abs(v) > 1000000 for v in (a, b, c)):
            raise ValueError('coefficient out of range')
        k = a+b
        # run(x:i64) -> i64, i64 modular arithmetic; not unbounded symbolic integers.
        body = b'\x00\x20\x00\x42' + sleb(k) + b'\x7e\x42' + sleb(c) + b'\x7c\x0b'
        signature = b'\x60\x01\x7e\x01\x7e'
        manifest = {'kind': 'affine-i64', 'language': 'Wolfram-compatible subset', 'coefficients': [k,c], 'arithmetic': 'modulo 2^64'}
        wat_body = f'local.get 0\n    i64.const {k}\n    i64.mul\n    i64.const {c}\n    i64.add'
        wat = f'(module\n  (func (export "run") (param i64) (result i64)\n    {wat_body})\n)\n'
    else:
        if len(t) < 5 or t[0] not in LANG or t[1] != '📏' or t[2] not in OPS:
            raise ValueError(f'line {line}: unsupported array lowering')
        n = int(t[3])
        if n < 1 or n > 2048 or len(t) != 4+2*n:
            raise ValueError('invalid vector length or operand count')
        vals = [float(x) for x in t[4:]]
        if not all(math.isfinite(x) for x in vals):
            raise ValueError('nonfinite input')
        memory = struct.pack('<'+'d'*(2*n), *vals)
        # Input A at 0, B at n*8, result at 2*n*8. Runtime loop, no precomputed results.
        # One local i32 index. Loop from 0 to n-1, loading both operands.
        op, opname = OPS[t[2]]
        code = bytearray()
        def ins(x): code.extend(x)
        def i32(v): ins(b'\x41'+sleb(v))
        def get(): ins(b'\x20\x00')
        def address(base):
            get(); i32(8); ins(b'\x6c'); i32(base); ins(b'\x6a')
        i32(0); ins(b'\x21\x00')
        ins(b'\x02\x40\x03\x40')
        get(); i32(n); ins(b'\x4f\x0d\x01')
        address(2*n*8)
        address(0); ins(b'\x2b\x03\x00')
        address(n*8); ins(b'\x2b\x03\x00')
        ins(bytes([op])); ins(b'\x39\x03\x00')
        get(); i32(1); ins(b'\x6a\x21\x00\x0c\x00\x0b\x0b\x0b')
        body = b'\x01\x01\x7f'+bytes(code)
        signature = b'\x60\x00\x00'
        manifest = {'kind':'vector-f64','language':LANG[t[0]],'operation':opname,'length':n,'input_a_offset':0,'input_b_offset':n*8,'output_offset':2*n*8,'result_bytes':n*8}
        escaped = ''.join('\\%02x' % b for b in memory)
        wat = f'''(module
  (memory (export "memory") 1)
  (func (export "run") (local $i i32)
    i32.const 0
    local.set $i
    block
      loop
        local.get $i
        i32.const {n}
        i32.ge_u
        br_if 1
        local.get $i
        i32.const 8
        i32.mul
        i32.const {2*n*8}
        i32.add
        local.get $i
        i32.const 8
        i32.mul
        f64.load
        local.get $i
        i32.const 8
        i32.mul
        i32.const {n*8}
        i32.add
        f64.load
        {opname}
        f64.store
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        br 0
      end
    end)
  (data (i32.const 0) "{escaped}")
)
'''
    types = section(1, vector([signature]))
    funcs = section(3, vector([uleb(0)]))
    mem = section(5, vector([b'\x00'+uleb(1)])) if memory else b''
    exports = [uleb(3)+b'run'+b'\x00'+uleb(0)]
    if memory: exports.insert(0, uleb(6)+b'memory'+b'\x02'+uleb(0))
    exp = section(7, vector(exports))
    codes = section(10, vector([uleb(len(body))+body]))
    data = section(11, vector([b'\x00\x41\x00\x0b'+uleb(len(memory))+memory])) if memory else b''
    wasm = b'\x00asm\x01\x00\x00\x00'+types+funcs+mem+exp+codes+data
    return wasm, wat, manifest

def main():
    p=argparse.ArgumentParser()
    p.add_argument('source',type=pathlib.Path)
    p.add_argument('-o','--output',required=True,type=pathlib.Path)
    a=p.parse_args()
    wasm,wat,manifest=compile_source(a.source.read_text(encoding='utf8'))
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_bytes(wasm)
    a.output.with_suffix('.wat').write_text(wat,encoding='utf8')
    a.output.with_suffix('.skull').write_text(encode(wasm)+'\n',encoding='utf8')
    a.output.with_suffix('.flat.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
    print(f'array runtime WASM: {len(wasm)} bytes ({manifest["kind"]})')
if __name__=='__main__': main()
