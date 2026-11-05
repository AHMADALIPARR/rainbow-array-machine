#!/bin/sh
# IR suite: (1) ramc executing demo.ram must be byte-identical to
# v0.1's ram; (2) --emit-ir shows canonical typed nodes with source
# positions; (3) bindings and @references compute correctly;
# (4) the fail-closed and type errors are refused.
set -eu
T=/tmp/ramc-test-$$
trap 'rm -f $T.*' EXIT

./ram examples/demo.ram > $T.v1.out
./ramc examples/demo.ram > $T.ir.out
diff $T.v1.out $T.ir.out >/dev/null || { echo 'FAIL: ramc output differs from ram on demo.ram'; exit 1; }

./ramc --emit-ir examples/demo.ram > $T.emit.out
grep -F '= bqn.add' $T.emit.out >/dev/null || { echo 'FAIL: emit-ir missing canonical bqn.add'; exit 1; }
grep -F ': vector[3]' $T.emit.out >/dev/null || { echo 'FAIL: emit-ir missing result type vector[3]'; exit 1; }
grep -F '@L2' $T.emit.out >/dev/null || { echo 'FAIL: emit-ir missing source position'; exit 1; }

cat > $T.bind.ram <<'EOF'
v ← 🌀 📏 ➕ 3 1 2 3 4 5 6
🌀 📏 ✖️ @v @v
s ← ⚡ 📏 🧮 @v
a ← 🌀 🟦 🧲 2 2 2 1 2 3 4 5 6 7 8
🌀 🟦 🔄 @a
EOF
./ramc $T.bind.ram > $T.bind.out
for expected in 'values=[25,49,81]' 'values=[21]' 'values=[19,43,22,50]'; do
  grep -F "$expected" $T.bind.out >/dev/null || { echo "FAIL: bindings case $expected"; exit 1; }
done
./ramc --emit-ir $T.bind.ram > $T.bind.emit
grep -F 'bind "v"' $T.bind.emit >/dev/null || { echo 'FAIL: emit-ir missing bind node for v'; exit 1; }

bad() { printf '%s\n' "$1" > $T.bad.ram; if ./ramc $T.bad.ram >/dev/null 2>&1; then echo "FAIL: accepted: $1"; exit 1; fi; }
bad '🦀 📏 ➕ 2 1 2 3 4'
bad '🌀 🔱 ➕ 2 1 2 3 4'
bad '🌀 📏 ➕ 2 @nope 3 4'
bad 'v ← 🌀 📏 ➕ 2 1 2 3 4
v ← 🌀 📏 ➕ 2 1 2 3 4'
bad 'v ← 🌀 📏 ➕ 2 1 2 3 4
🌀 🟦 🔄 @v'
bad 'v ← 🌀 📏 ➕ 3 1 2 3 4 5 6
🌀 📏 ➕ 2 @v 7 8'
echo 'PASS: equivalence, emit-ir, bindings, 6 negative cases'
