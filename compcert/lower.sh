#!/bin/sh
# CompCert lowering pass for the Rainbow Array Machine.
#
# Every host-side C program in the stack is compiled with CompCert
# (ccomp), the verified C compiler: its Coq-checked correctness
# theorem guarantees the generated assembly preserves the C
# program's semantics. The assembly is emitted to compcert/asm/
# and the CompCert-built binaries are then put through the same
# suites the gcc/clang builds pass (the differential check).
#
# Scope, stated plainly: CompCert proves C -> assembly semantic
# preservation for these programs. It does not prove the C itself
# correct; that evidence is the test suites and the Lean theorems.
# The WASM modules remain clang-built (CompCert has no wasm32
# target); what CompCert lowers here is the host toolchain and
# the seal.
#
# Build notes (verified 2026-11-05, CompCert 3.12):
# - CCOMP must point at an installed ccomp, e.g.
#   CCOMP=~/workspace/.tools/compcert-3.12-install/bin/ccomp
# - The system <math.h> declares long double functions, which
#   CompCert's parser rejects. compcert/include/math.h is a build
#   shim declaring only the double-precision functions the stack
#   uses (isfinite, round, exp); sources are compiled unmodified
#   and the implementations remain libm's.
# - src/ramc.c returns a struct from one function; ccomp handles
#   it with -fstruct-passing.
set -eu
cd "$(dirname "$0")/.."
CCOMP=${CCOMP:-ccomp}
SHIM="-I compcert/include"
OUT=compcert/build
ASM=compcert/asm
mkdir -p "$OUT" "$ASM"

echo "== CompCert version =="
$CCOMP --version | head -n 1

echo "== lowering: seal =="
$CCOMP -S -o "$ASM/seal_recursion.s" crypto/seal_recursion.c
$CCOMP -o "$OUT/seal" crypto/seal_recursion.c

echo "== lowering: core host tools =="
for t in ramc flat_check flat_lower ram_codegen; do
    $CCOMP $SHIM -S -o "$ASM/core-$t.s" core/src/$t.c
    $CCOMP $SHIM -o "$OUT/core-$t" core/src/$t.c -lm
done

echo "== lowering: v0.1-line host tools =="
$CCOMP $SHIM -S -o "$ASM/ram.s" src/ram.c
$CCOMP $SHIM -o "$OUT/ram" src/ram.c -lm
$CCOMP $SHIM -fstruct-passing -S -o "$ASM/ramc-typed.s" src/ramc.c
$CCOMP $SHIM -fstruct-passing -o "$OUT/ramc-typed" src/ramc.c -lm
$CCOMP $SHIM -S -o "$ASM/flat.s" src/flat.c
$CCOMP $SHIM -o "$OUT/flat" src/flat.c
$CCOMP $SHIM -S -o "$ASM/skull.s" src/skull.c
$CCOMP $SHIM -o "$OUT/skull" src/skull.c
$CCOMP $SHIM -S -o "$ASM/ram_array_runtime.s" src/ram_array_runtime.c
$CCOMP $SHIM -o "$OUT/ram-array-runtime" src/ram_array_runtime.c -lm

echo "== differential: seal output vs the gcc-built reference (byte diff) =="
gcc -O2 -std=c11 -o "$OUT/seal-gcc-ref" crypto/seal_recursion.c
"$OUT/seal-gcc-ref" > "$OUT/seal-gcc.out"
"$OUT/seal" > "$OUT/seal.out"
diff "$OUT/seal-gcc.out" "$OUT/seal.out"
echo "seal output byte-identical ($(wc -l < "$OUT/seal.out") lines)"

echo "== differential: CompCert-built core tools vs gcc-built core tools =="
# The CompCert-built pipeline must produce byte-identical IR,
# FLAT2 and generated C to the gcc-built pipeline (core/build).
T=$(mktemp -d)
for prog in polynomial showcase; do
    "$OUT/core-ramc" core/examples/$prog.ram "$T/$prog.ir"
    "$OUT/core-flat_check" "$T/$prog.ir" "$T/$prog.flat"
    "$OUT/core-flat_lower" "$T/$prog.ir" "$T/$prog.flat" "$T/$prog.flat2"
    "$OUT/core-ram_codegen" "$T/$prog.flat2" "$T/$prog.c" > /dev/null
    core/build/ramc core/examples/$prog.ram "$T/${prog}_g.ir"
    core/build/flat_check "$T/${prog}_g.ir" "$T/${prog}_g.flat"
    core/build/flat_lower "$T/${prog}_g.ir" "$T/${prog}_g.flat" "$T/${prog}_g.flat2"
    core/build/ram_codegen "$T/${prog}_g.flat2" "$T/${prog}_g.c" > /dev/null
    diff "$T/$prog.ir" "$T/${prog}_g.ir"
    diff "$T/$prog.flat2" "$T/${prog}_g.flat2"
    diff "$T/$prog.c" "$T/${prog}_g.c"
    echo "$prog: IR + FLAT2 + generated C byte-identical"
done
rm -rf "$T"

echo "LOWERING COMPLETE — assembly in $ASM"
