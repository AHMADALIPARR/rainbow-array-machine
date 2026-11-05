#!/usr/bin/env python3
"""Deterministic UTF-8 geometry subset: BQN-qualified geometry and KCL-inspired @@ bridge.

@@ is RAM syntax, NOT claimed to be a native KCL operator.
"""
import argparse
import json
import pathlib
import re
import struct

INT = re.compile(r'-?(?:0|[1-9][0-9]*)\Z')

def ints(words, count, line):
    if len(words) != count or any(not INT.fullmatch(w) for w in words):
        raise ValueError(f'line {line}: expected {count} integer operands')
    values = tuple(map(int, words))
    if any(abs(v) > 100000 for v in values):
        raise ValueError(f'line {line}: coordinate out of range')
    return values

def compile_geometry(source):
    geometry = {}
    active = None
    assertions = 0
    for line_no, raw in enumerate(source.splitlines(), 1):
        s = raw.split('#', 1)[0].strip()
        if not s:
            continue
        t = s.split()
        if t[:2] == ['🌀', '📐']:
            if len(t) != 8 or t[2] != 'GRID':
                raise ValueError(f'line {line_no}: expected 🌀 📐 GRID name nx ny nz spacing')
            name = t[3]
            if not name.isascii() or not name.isidentifier() or name in geometry:
                raise ValueError(f'line {line_no}: invalid or duplicate grid name')
            nx, ny, nz, spacing = ints(t[4:], 4, line_no)
            if min(nx, ny, nz, spacing) <= 0 or nx*ny*nz > 4096:
                raise ValueError(f'line {line_no}: invalid grid dimensions or spacing')
            pts = [(x*spacing, y*spacing, z*spacing) for z in range(nz) for y in range(ny) for x in range(nx)]
            geometry[name] = {'shape': (nx, ny, nz), 'spacing': spacing, 'points': pts}
            active = name
        elif t[:2] == ['⚡', '@@']:
            if len(t) < 4 or t[2] not in geometry:
                raise ValueError(f'line {line_no}: unknown geometry in @@')
            name = t[2]
            op = t[3]
            if op == 'ROTATE_Z':
                (degrees,) = ints(t[4:], 1, line_no)
                if degrees % 90:
                    raise ValueError(f'line {line_no}: only exact 90-degree rotations supported')
                for _ in range((degrees // 90) % 4):
                    geometry[name]['points'] = [(-y, x, z) for x, y, z in geometry[name]['points']]
            elif op == 'TRANSLATE':
                dx, dy, dz = ints(t[4:], 3, line_no)
                geometry[name]['points'] = [(x+dx, y+dy, z+dz) for x, y, z in geometry[name]['points']]
            else:
                raise ValueError(f'line {line_no}: unsupported @@ operation')
            active = name
        elif t[0] == '↶':
            if len(t) < 4 or t[1:3] != ['⚡', '@@'] or t[3] not in geometry:
                raise ValueError(f'line {line_no}: invalid inverse')
            name = t[3]
            if len(t) < 5:
                raise ValueError(f'line {line_no}: missing inverse operation')
            op = t[4]
            if op == 'ROTATE_Z':
                (angle,) = ints(t[5:], 1, line_no)
                if angle % 90:
                    raise ValueError(f'line {line_no}: inverse requires 90-degree rotation')
                for _ in range((-angle // 90) % 4):
                    geometry[name]['points'] = [(-y, x, z) for x, y, z in geometry[name]['points']]
            elif op == 'TRANSLATE':
                dx, dy, dz = ints(t[5:], 3, line_no)
                geometry[name]['points'] = [(x-dx, y-dy, z-dz) for x, y, z in geometry[name]['points']]
            else:
                raise ValueError(f'line {line_no}: unsupported inverse')
            active = name
        elif t[0] == '>>>>>>>€':
            if active is None or len(t) < 2:
                raise ValueError(f'line {line_no}: assertion without geometry')
            g = geometry[active]
            if t[1] == 'SHAPE':
                expected = ints(t[2:], 3, line_no)
                actual = g['shape']
            elif t[1] == 'POINT':
                if len(t) != 6:
                    raise ValueError(f'line {line_no}: POINT requires index and xyz')
                idx, *coords = ints(t[2:], 4, line_no)
                if idx < 0 or idx >= len(g['points']):
                    raise ValueError(f'line {line_no}: point index out of bounds')
                actual, expected = g['points'][idx], tuple(coords)
            else:
                raise ValueError(f'line {line_no}: unsupported assertion')
            if actual != expected:
                raise ValueError(f'line {line_no}: assertion failed: {actual} != {expected}')
            assertions += 1
        else:
            raise ValueError(f'line {line_no}: unknown instruction')
    if not geometry:
        raise ValueError('no geometry declared')
    # Fixed deterministic layout: points in x,y,z order, little-endian i32.
    payload = bytearray()
    manifest = {'format': 'ram-geometry-flat-v1', 'assertions_passed': assertions, 'grids': {}}
    for name, g in geometry.items():
        offset = len(payload)
        for point in g['points']:
            for v in point:
                if not -(2**31) <= v < 2**31:
                    raise ValueError('coordinate exceeds i32')
                payload.extend(struct.pack('<i', v))
        manifest['grids'][name] = {'shape': g['shape'], 'offset_bytes': offset, 'point_count': len(g['points']), 'dtype': 'i32', 'components': 3}
    if len(payload) > 65536:
        raise ValueError('memory exceeds one WASM page')
    return bytes(payload), manifest

def wat_for(payload):
    # WebAssembly memory is initialized using one data segment, exported for inspection.
    # Escape every byte as a WAT hexadecimal escape, including zeros and quotes.
    escaped = ''.join(f'\\{b:02x}' for b in payload)
    return '(module\n  (memory (export "memory") 1)\n  (data (i32.const 0) "' + escaped + '")\n  (func (export "run") (result i32) i32.const ' + str(len(payload)) + ')\n)\n'

def main():
    p = argparse.ArgumentParser()
    p.add_argument('source', type=pathlib.Path)
    p.add_argument('-o', '--output', type=pathlib.Path, required=True)
    args = p.parse_args()
    data, manifest = compile_geometry(args.source.read_text(encoding='utf-8'))
    args.output.write_text(wat_for(data), encoding='utf-8')
    args.output.with_suffix('.flat.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'{len(data)} bytes of geometry; {manifest["assertions_passed"]} assertions passed')

if __name__ == '__main__':
    main()
