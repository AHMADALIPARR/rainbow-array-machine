import pathlib
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'src'))
from ram_array_runtime import compile_source
from ram_traceback import generate

class TracebackTests(unittest.TestCase):
    def test_affine(self):
        source = '🐺 🔹 📜 2 3 7'.encode()
        wasm, _, _ = compile_source(source.decode())
        lean, record = generate(source, wasm)
        self.assertIn('import Mathlib\n', lean)
        self.assertIn('by\n  ring', lean)
        self.assertEqual(record['lean_status'], 'not_checked')
    def test_vector(self):
        source = '🔺 📏 ✖️ 3 1 2 3 4 5 6'.encode()
        wasm, _, _ = compile_source(source.decode())
        lean, _ = generate(source, wasm)
        self.assertIn('rfl', lean)
    def test_modified_binary_rejected(self):
        source = '🐺 🔹 📜 2 3 7'.encode()
        wasm, _, _ = compile_source(source.decode())
        with self.assertRaisesRegex(ValueError, 'does not match'):
            generate(source, wasm[:-1]+bytes([wasm[-1]^1]))
    def test_wrong_source_rejected(self):
        source = '🐺 🔹 📜 2 3 7'.encode()
        wasm, _, _ = compile_source(source.decode())
        with self.assertRaises(ValueError):
            generate('🐺 🔹 📜 2 3 8'.encode(), wasm)

if __name__ == '__main__': unittest.main()
