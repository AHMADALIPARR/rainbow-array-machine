#!/usr/bin/env python3
"""Fail-closed artifact/source consistency check and Mathlib-only proof generator.

This is a bounded traceback, NOT a general decompiler or a WASM correctness proof.
"""
import argparse
import hashlib
import json
import pathlib
import sys
from ram_array_runtime import compile_source


def generate(source: bytes, wasm: bytes):
    expected, wat, manifest = compile_source(source.decode('utf-8'))
    if wasm != expected:
        raise ValueError('WASM does not match deterministic lowering of supplied source')
    digest = hashlib.sha256(wasm).hexdigest()
    if manifest['kind'] == 'affine-i64':
        a, b, c = map(int, source.decode('utf-8').strip().split()[3:])
        k = a + b
        lean = f'''import Mathlib

/-! Generated algebraic obligation for SHA256 {digest}.
    This is an integer identity, NOT a proof that the WASM module implements it.
    WASM i64 uses modular arithmetic; no overflow-free claim is made. -/
example (x : ℤ) : ({a} : ℤ) * x + {b} * x + {c} = ({k} : ℤ) * x + {c} := by
  ring
'''
        obligations = ['affine coefficient collection over ℤ']
    elif manifest['kind'] == 'vector-f64':
        # Algebraic proofs apply over ℤ, not floating-point rounding or NaNs.
        lean = f'''import Mathlib

/-! Generated abstract array-index obligation for SHA256 {digest}.
    This proves pointwise mapping over integers, NOT IEEE-754 or WASM semantics. -/
example (n : ℕ) (a b : Fin n → ℤ) (i : Fin n) :
    (fun j => a j { {'f64.add': '+', 'f64.sub': '-', 'f64.mul': '*'}[manifest['operation']] } b j) i =
      a i { {'f64.add': '+', 'f64.sub': '-', 'f64.mul': '*'}[manifest['operation']] } b i := by
  rfl
'''
        obligations = ['pointwise array map over ℤ']
    else:
        raise ValueError('unsupported proof kind')
    record = {'wasm_sha256': digest, 'source_sha256': hashlib.sha256(source).hexdigest(),
              'kind': manifest['kind'], 'manifest': manifest, 'obligations': obligations,
              'artifact_match': True, 'lean_status': 'not_checked',
              'scope': 'source-to-artifact byte equality and abstract algebraic obligation only'}
    return lean, record


def main():
    p = argparse.ArgumentParser()
    p.add_argument('source', type=pathlib.Path)
    p.add_argument('wasm', type=pathlib.Path)
    p.add_argument('-o', '--output', type=pathlib.Path, required=True)
    a = p.parse_args()
    lean, record = generate(a.source.read_bytes(), a.wasm.read_bytes())
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(lean, encoding='utf-8')
    a.output.with_suffix('.trace.json').write_text(json.dumps(record, indent=2)+'\n', encoding='utf-8')
    print('artifact/source match: PASS; Mathlib proof: GENERATED, NOT CHECKED')

if __name__ == '__main__':
    try:
        main()
    except (ValueError, UnicodeError) as e:
        print(f'traceback rejected: {e}', file=sys.stderr)
        sys.exit(1)
