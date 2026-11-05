#!/bin/sh
# Skull & Crown suite: the owner's worked example is the acceptance
# vector; an independent Python reference encoder (test tooling)
# cross-checks the C implementation over all 256 byte values and a
# real file, in both directions.
set -eu
T=/tmp/ramskull-test-$$
trap 'rm -f $T.*' EXIT

printf '\000\141\163\155' > $T.magic
./skull encode $T.magic $T.magic.skull
cmp $T.magic.skull tests/fixtures/wasm-magic.skull || { echo 'FAIL: WASM magic vector mismatch'; exit 1; }
./skull decode $T.magic.skull $T.magic2
cmp $T.magic $T.magic2 || { echo 'FAIL: magic round trip'; exit 1; }

python3 - "$T" <<'EOF'
import subprocess, sys
T = sys.argv[1]
data = bytes(range(256)) * 4
open(T + '.all', 'wb').write(data)
def ref_encode(bs):
    return "".join(("👑" if (b >> bit) & 1 else "💀") for b in bs for bit in range(7, -1, -1))
ref = "".join(ref_encode([b]) + "\n" for b in data)
subprocess.run(['./skull', 'encode', T + '.all', T + '.all.skull'], check=True)
got = open(T + '.all.skull', encoding='utf-8').read()
assert got == ref, 'C encoder disagrees with reference over all byte values'
open(T + '.ref.skull', 'w', encoding='utf-8').write(ref)
subprocess.run(['./skull', 'decode', T + '.ref.skull', T + '.all2'], check=True)
assert open(T + '.all2', 'rb').read() == data, 'C decoder disagrees with reference encoding'
print('reference cross-check: all 256 byte values, both directions, agree')
EOF

./skull encode examples/runtime-array/symbolic.wasm $T.wat.skull
./skull decode $T.wat.skull $T.wat2
cmp examples/runtime-array/symbolic.wasm $T.wat2 || { echo 'FAIL: real-file round trip'; exit 1; }
python3 -c "
import sys; n = len(open('$T.wat.skull','rb').read()); src = len(open('examples/runtime-array/symbolic.wasm','rb').read())
assert n == src*33, (n, src)  # 8 symbols x 4 bytes + 1 newline per source byte
print(f'size law: {src} bytes -> {n} bytes (33x in canonical line form, 32x symbols alone)')"

# invert: the owner's worked NOT vector (0x65), involution, and
# decode(invert(x)) == bitwise NOT of decode(x) over all byte values.
printf '\145' > $T.b65
./skull encode $T.b65 $T.b65.skull
python3 -c "open('$T.want','w',encoding='utf-8').write('💀👑👑💀💀👑💀👑\n')"
cmp $T.b65.skull $T.want || { echo 'FAIL: 0x65 encodes to the owner vector'; exit 1; }
./skull invert $T.b65.skull $T.b65.inv
python3 -c "open('$T.want2','w',encoding='utf-8').write('👑💀💀👑👑💀👑💀\n')"
cmp $T.b65.inv $T.want2 || { echo 'FAIL: invert of the owner vector'; exit 1; }
./skull invert $T.all.skull $T.all.inv
./skull invert $T.all.inv $T.all.inv2
cmp $T.all.skull $T.all.inv2 || { echo 'FAIL: invert is not involutive'; exit 1; }
./skull decode $T.all.inv $T.all.not
python3 -c "
d = open('$T.all','rb').read(); n = open('$T.all.not','rb').read()
assert n == bytes((~b) & 0xFF for b in d), 'decode(invert(x)) != NOT decode(x)'
print('invert: owner vector, involution, NOT semantics over all byte values')"

bad() { printf '%b' "$1" > $T.bad.skull; if ./skull decode $T.bad.skull $T.bad.out >/dev/null 2>&1; then echo "FAIL: accepted: $1"; exit 1; fi; }
bad '\360\237\222\200\360\237\222\200\360\237\222\200\360\237\222\200\360\237\222\200\360\237\222\200\360\237\222\200\n'
bad '\360\237\224\245\360\237\222\200\360\237\222\200\360\237\222\200\360\237\222\200\360\237\222\200\360\237\222\200\360\237\222\200\n'
printf '0' > $T.bad.skull; if ./skull decode $T.bad.skull $T.bad.out >/dev/null 2>&1; then echo 'FAIL: ASCII digit accepted'; exit 1; fi
echo 'PASS: magic vector, 256-value cross-check, real-file round trip, size law, 3 negative cases'
