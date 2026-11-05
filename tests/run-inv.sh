#!/bin/sh
# Reversible-algebra suite: assertions, registered inverses,
# metadata-gated inverses, round-trip certificates.
set -eu
T=/tmp/raminv-test-$$
trap 'rm -f $T.*' EXIT

./ramc examples/inverse.ram > $T.out
for expected in 'values=[5,7,9]' 'values=[👑]' 'values=[💀]' 'values=[1,2]' \
  'CERT v1: roundtrip; op=transpose; status=VERIFIED_ON_TESTED_INPUTS' \
  'values=[3,5]' 'values=[1,3,2,4]'; do
  grep -F "$expected" $T.out >/dev/null || { echo "FAIL: missing: $expected"; exit 1; }
done
ok=$(grep -c 'assertion ok' $T.out)
[ "$ok" = "11" ] || { echo "FAIL: expected 11 assertions ok, got $ok"; exit 1; }

bad() { printf '%s\n' "$1" > $T.bad.ram; if ./ramc $T.bad.ram >/dev/null 2>&1; then echo "FAIL: accepted: $1"; exit 1; fi; }
# assertion failures
bad '🔺 📏 ➕ [1 2] [3 4]
>>>>>>>€ [9 9]'
bad '🔺 📏 ➕ [1 2] [3 4]
>>>>>>>€ [1 2 3]'
# no registered inverse for a reduction
bad '↶ 🌀 📏 🧮 [1 2 3]'
# flatten inverse without saved metadata (bare literal)
bad '↶ 🌀 📏 📦 4 1 3 2 4'
# inverse of flatten applied to a non-flatten binding
bad 'v ← 🌀 📏 ➕ [1 2] [3 4]
↶ 🌀 📏 📦 @v'
# inverse multiply by zero
bad '↶ 🌀 📏 ✖️ [4 6] [0 4]'
# certificate without an inverse pair
bad '🔺 🟦 🔄 [[1 2] [3 4]]
>>>>>>>€ [[1 3] [2 4]]
🛡️ ROUNDTRIP'
echo 'PASS: algebra demo (11 assertions, certificate, verdicts), 7 negative cases'
