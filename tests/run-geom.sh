#!/bin/sh
# Geometry suite: ramc runs the owner's v0.4 grid program and must
# pass exactly the 6 assertions his Python compiler reports.
# Final lattice points are cross-checked against the payload tuple
# his test suite pins: (0,0,0) (1,0,0) (0,1,0) (1,1,0).
set -eu
T=/tmp/ramgeom-test-$$
trap 'rm -f $T.*' EXIT

./ramc examples/geometry/grid.ram > $T.out
ok=$(grep -c 'assertion ok' $T.out)
[ "$ok" = "6" ] || { echo "FAIL: expected 6 assertions ok, got $ok"; exit 1; }

cat > $T.final.ram <<'EOF'
🌀 📐 GRID g 2 2 1 1
>>>>>>>€ POINT 0 0 0 0
>>>>>>>€ POINT 1 1 0 0
>>>>>>>€ POINT 2 0 1 0
>>>>>>>€ POINT 3 1 1 0
EOF
./ramc $T.final.ram >/dev/null || { echo 'FAIL: final lattice points disagree with the v0.4 payload'; exit 1; }

cat > $T.ops.ram <<'EOF'
🌀 🟦 ⥊ [2 3] 1 2 3 4 5 6
>>>>>>>€ [[1 2 3] [4 5 6]]
🌀 🟦 ⍉ [[1 2 3] [4 5 6]]
>>>>>>>€ [[1 4] [2 5] [3 6]]
🌀 📏 ↕ [1 2 3 4]
>>>>>>>€ [4 3 2 1]
🌀 🟦 ⊑ 1 [[1 2] [3 4]]
>>>>>>>€ [3 4]
🌀 🟦 ⊏ [0 2] [[1 2] [3 4] [5 6]]
>>>>>>>€ [[1 2] [5 6]]
EOF
./ramc $T.ops.ram >/dev/null || { echo 'FAIL: BQN axis operations'; exit 1; }

bad() { printf '%s\n' "$1" > $T.bad.ram; if ./ramc $T.bad.ram >/dev/null 2>&1; then echo "FAIL: accepted: $1"; exit 1; fi; }
bad '🌀 📐 GRID g 2 2 1 1
⚡ @@ g ROTATE_Z 45'
bad '🌀 📐 GRID g 2 2 1 1
⚡ @@ missing ROTATE_Z 90'
bad '🌀 📐 GRID g 2 2 1 1
>>>>>>>€ POINT 9 0 0 0'
bad '🌀 📐 GRID g 2 2 1 1
>>>>>>>€ SHAPE 3 3 3'
bad '🌀 🟦 ⥊ [3 3] 1 2 3 4'
bad '↶ 🌀 🟦 ⊑ 0 [[1 2] [3 4]]'
echo 'PASS: v0.4 grid program (6 assertions), payload cross-check, BQN axis ops, 6 negative cases'
