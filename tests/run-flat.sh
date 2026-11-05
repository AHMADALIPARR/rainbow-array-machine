#!/bin/sh
# FLAT suite: the symbolic layer's emitted IR, executed.
# (1) Round trip: ram's 📜 emission feeds flat directly; scalar
#     results agree with shell arithmetic (an independent reference).
# (2) Tensor mode: the same FLAT program applied elementwise agrees
#     with ramc computing the same affine map by array operations.
# (3) Overflow and malformed programs are refused, never wrapped.
set -eu
T=/tmp/ramf-test-$$
trap 'rm -f $T.*' EXIT

./ram examples/wolfram.ram | grep '^FLAT v1:' > $T.prog.flat
for x in 0 1 10 -4 100000; do
  got=$(./flat $T.prog.flat $x)
  want=$((5*x+7))
  [ "$got" = "$want" ] || { echo "FAIL: scalar x=$x: got $got want $want"; exit 1; }
done

printf 'vector 4\n1 2 3 4\n' > $T.tensor
./flat $T.prog.flat $T.tensor > $T.tensor.out
grep -F 'rank=1 shape=[4] values=[12,17,22,27]' $T.tensor.out >/dev/null || { echo 'FAIL: tensor mode values'; exit 1; }

cat > $T.affine.ram <<'EOF'
🌀 📏 ✖️ 4 1 2 3 4 5 5 5 5
🌀 📏 ➕ 4 5 10 15 20 7 7 7 7
EOF
# ramc computes (5*v + 7) for v = [1,2,3,4] by literal elementwise ops;
# its second line must equal flat's tensor output values.
./ramc $T.affine.ram | tail -1 > $T.ramc.out
grep -F 'values=[12,17,22,27]' $T.ramc.out >/dev/null || { echo 'FAIL: ramc cross-check disagrees'; exit 1; }

bad() { printf '%s\n' "$1" > $T.bad.flat; if ./flat $T.bad.flat 3 >/dev/null 2>&1; then echo "FAIL: accepted: $1"; exit 1; fi; }
bad 'FLAT v1: input x:i64; t0=div_i64(x,2); return t0'
bad 'FLAT v1: input x:i64; t0=mul_i64(t9,2); return t0'
bad 'FLAT v1: input x:i64; t0=mul_i64(x,2); t0=add_i64(t0,1); return t0'
bad 'FLAT v2: input x:i64; return x'
printf 'FLAT v1: input x:i64; t0=mul_i64(x,-1); return t0\n' > $T.ovf.flat
./flat $T.ovf.flat -9223372036854775808 >/dev/null 2>&1 && { echo 'FAIL: LLONG_MIN * -1 wrapped'; exit 1; } || true
echo 'PASS: round trip (5 scalar cases), tensor mode, ramc cross-check, 5 negative cases'
