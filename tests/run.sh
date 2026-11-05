#!/bin/sh
set -eu
./ram examples/demo.ram > /tmp/ram-test-$$.out
trap 'rm -f /tmp/ram-test-$$.out /tmp/ram-bad-$$.ram' EXIT
for expected in 'values=[5,7,9]' 'values=[1,4,2,5,3,6]' 'values=[10]' 'values=[58,64,139,154]' 'values=[2,3]' 'values=[1,2,3,4]'; do
  grep -F "$expected" /tmp/ram-test-$$.out >/dev/null || { echo "FAIL: $expected"; exit 1; }
done
printf '🌀 📏 ➕ 2 1 2 3\n' > /tmp/ram-bad-$$.ram
if ./ram /tmp/ram-bad-$$.ram >/dev/null 2>&1; then echo 'FAIL: invalid operand count accepted';exit 1;fi
printf '🦀 📏 ➕ 2 1 2 3 4\n' > /tmp/ram-bad-$$.ram
if ./ram /tmp/ram-bad-$$.ram >/dev/null 2>&1; then echo 'FAIL: unsupported namespace accepted';exit 1;fi
./ram examples/wolfram.ram > /tmp/ram-test-$$.out
for expected in 'simplify => (5*x)+(7)' 'FLAT v1: input x:i64' 'status=VERIFIED_BY_INTEGER_COEFFICIENT_IDENTITY'; do
 grep -F "$expected" /tmp/ram-test-$$.out >/dev/null || { echo "FAIL: $expected"; exit 1; }
done
printf '🐺 🔹 ✨ 2.5 3 7\n' > /tmp/ram-bad-$$.ram
if ./ram /tmp/ram-bad-$$.ram >/dev/null 2>&1; then echo 'FAIL: noninteger coefficient accepted';exit 1;fi
printf '🐺 🔹 ✨ 2 3\n' > /tmp/ram-bad-$$.ram
if ./ram /tmp/ram-bad-$$.ram >/dev/null 2>&1; then echo 'FAIL: missing coefficient accepted';exit 1;fi
echo 'PASS: 9 positive cases, 4 negative cases' 
