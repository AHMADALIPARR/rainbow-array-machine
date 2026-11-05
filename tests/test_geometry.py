import importlib.util
import pathlib
import struct
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('ram_geometry', ROOT / 'src/ram_geometry.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class GeometryTests(unittest.TestCase):
    def test_roundtrip(self):
        source = (ROOT / 'examples/geometry/grid.ram').read_text()
        payload, meta = mod.compile_geometry(source)
        self.assertEqual(meta['assertions_passed'], 6)
        self.assertEqual(len(payload), 48)
        self.assertEqual(struct.unpack('<12i', payload), (0,0,0, 1,0,0, 0,1,0, 1,1,0))
        self.assertIn('(data (i32.const 0)', mod.wat_for(payload))
    def test_failed_assertion(self):
        with self.assertRaisesRegex(ValueError, 'assertion failed'):
            mod.compile_geometry('🌀 📐 GRID g 1 1 1 1\n>>>>>>>€ POINT 0 1 0 0')
    def test_unsupported_angle(self):
        with self.assertRaisesRegex(ValueError, '90-degree'):
            mod.compile_geometry('🌀 📐 GRID g 1 1 1 1\n⚡ @@ g ROTATE_Z 45')
    def test_unknown(self):
        with self.assertRaisesRegex(ValueError, 'unknown geometry'):
            mod.compile_geometry('🌀 📐 GRID g 1 1 1 1\n⚡ @@ missing ROTATE_Z 90')
    def test_inverse(self):
        source = '🌀 📐 GRID g 2 1 1 2\n⚡ @@ g ROTATE_Z -90\n↶ ⚡ @@ g ROTATE_Z -90\n>>>>>>>€ POINT 1 2 0 0'
        self.assertEqual(mod.compile_geometry(source)[1]['assertions_passed'], 1)

if __name__ == '__main__':
    unittest.main()
