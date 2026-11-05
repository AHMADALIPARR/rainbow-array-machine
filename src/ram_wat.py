#!/usr/bin/env python3
"""UTF-8 emoji source to WebAssembly Text, deterministic supported subset."""
import argparse
import pathlib
import re
import sys

LANG = {'🌀': 'BQN', '🔺': 'APL', '⚡': 'K'}
OPS = {'➕': 'f64.add', '➖': 'f64.sub', '✖️': 'f64.mul'}

def compile_line(line, number):
    tokens = line.split('#', 1)[0].split()
    if not tokens:
        return None
    if len(tokens) >= 3 and tokens[:2] == ['🐺', '🔹'] and tokens[2] == '📜':
        if len(tokens) != 6:
            raise ValueError(f'line {number}: expected 🐺 🔹 📜 a b c')
        try:
            a, b, c = (int(v) for v in tokens[3:])
        except ValueError:
            raise ValueError(f'line {number}: expected integer coefficients') from None
        if any(abs(v) > 1000000 for v in (a,b,c)):
            raise ValueError(f'line {number}: coefficient out of supported range')
        return f'''  ;; 🐺 🔹 📜 : symbolic coefficient collection, integer mathematical semantics
  (func (export "run") (param $x i64) (result i64)
    local.get $x
    i64.const {a+b}
    i64.mul
    i64.const {c}
    i64.add)'''
    if len(tokens) < 5 or tokens[0] not in LANG or tokens[1] != '📏' or tokens[2] not in OPS:
        raise ValueError(f'line {number}: unsupported lowering (expected LANGUAGE 📏 ADD|SUB|MUL n A... B...)')
    try:
        n = int(tokens[3])
    except ValueError:
        raise ValueError(f'line {number}: invalid vector length') from None
    if not 1 <= n <= 2048 or len(tokens) != 4 + 2*n:
        raise ValueError(f'line {number}: invalid vector length or operand count')
    try:
        vals = [float(v) for v in tokens[4:]]
    except ValueError:
        raise ValueError(f'line {number}: invalid numeric operand') from None
    import math
    if not all(math.isfinite(v) for v in vals):
        raise ValueError(f'line {number}: nonfinite operand')
    # Operands are embedded as constants. The exported function writes n f64 values
    # into linear memory at byte offset zero. The caller reads memory[0:n*8].
    op = OPS[tokens[2]]
    instructions = []
    for i in range(n):
        instructions += [f'    i32.const {8*i}', f'    f64.const {repr(vals[i])}',
                         f'    f64.const {repr(vals[n+i])}', f'    {op}', '    f64.store']
    return f'''  ;; {LANG[tokens[0]]} vector {op}; result length {n}, f64 little-endian at memory offset 0
  (memory (export "memory") 1)
  (func (export "run")
{chr(10).join(instructions)})'''

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source', type=pathlib.Path)
    ap.add_argument('-o', '--output', type=pathlib.Path, required=True)
    args = ap.parse_args()
    entries = [(i, compile_line(line, i)) for i,line in enumerate(args.source.read_text(encoding='utf-8').splitlines(), 1)]
    entries = [entry for _,entry in entries if entry]
    if len(entries) != 1:
        raise ValueError('WAT v0.3 requires exactly one supported executable statement')
    wat = '(module\n' + entries[0] + '\n)\n'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(wat, encoding='utf-8')
    print(f'WAT emitted: {args.output}')

if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as exc:
        print(f'error: {exc}', file=sys.stderr)
        sys.exit(1)
