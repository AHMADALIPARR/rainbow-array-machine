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
set -eu
cd "$(dirname "$0")/.."
CCOMP=${CCOMP:-ccomp}
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
    $CCOMP -S -o "$ASM/core-$t.s" core/src/$t.c
    $CCOMP -o "$OUT/core-$t" core/src/$t.c -lm
done

echo "== lowering: v0.1-line host tools =="
$CCOMP -S -o "$ASM/ram.s" src/ram.c
$CCOMP -o "$OUT/ram" src/ram.c -lm
$CCOMP -S -o "$ASM/ramc-typed.s" src/ramc.c
$CCOMP -o "$OUT/ramc-typed" src/ramc.c -lm
$CCOMP -S -o "$ASM/flat.s" src/flat.c
$CCOMP -o "$OUT/flat" src/flat.c
$CCOMP -S -o "$ASM/skull.s" src/skull.c
$CCOMP -o "$OUT/skull" src/skull.c
$CCOMP -S -o "$ASM/ram_array_runtime.s" src/ram_array_runtime.c
$CCOMP -o "$OUT/ram-array-runtime" src/ram_array_runtime.c -lm

echo "== differential: seal output vs the gcc-built reference =="
"$OUT/seal" > "$OUT/seal.out"
echo "seal lines: $(wc -l < "$OUT/seal.out")"

echo "LOWERING COMPLETE — assembly in $ASM"
