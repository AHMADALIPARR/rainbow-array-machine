from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
vec = (root/'examples/wat/vector.wat').read_text()
sym = (root/'examples/wat/symbolic.wat').read_text()
assert vec.count('    f64.add') == 3
assert vec.count('f64.store') == 3
assert '(memory (export "memory") 1)' in vec
assert 'i64.const 5' in sym and 'i64.const 7' in sym
with tempfile.TemporaryDirectory() as d:
    bad = Path(d)/'bad.ram'
    bad.write_text('🌀 🧊 🧲 1 2 3', encoding='utf8')
    out = Path(d)/'bad.wat'
    p = subprocess.run(['python3', str(root/'src/ram_wat.py'), str(bad), '-o', str(out)], capture_output=True)
    assert p.returncode != 0 and not out.exists()
print('WAT generation and rejection tests passed')
