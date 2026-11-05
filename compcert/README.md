# CompCert lowering

The host-side C of this stack — the core pipeline tools
(`ramc`, `flat_check`, `flat_lower`, `ram_codegen`), the v0.1-line
tools (`ram`, the typed-IR `ramc`, `flat`, `skull`,
`ram_array_runtime`), and the seal (`crypto/seal_recursion.c`) —
lowers to assembly through **CompCert**, the verified C compiler
(Xavier Leroy, Inria; this tree builds CompCert 3.12 against
Coq 8.12.2).

CompCert's correctness theorem is machine-checked in Coq: for
every program it compiles, the generated assembly behaves
exactly as the C source semantics prescribes (for the behaviors
the C program defines). That makes the C → assembly step the one
link in this stack's chain that carries a proof instead of a
test.

Run it:

```sh
sh compcert/lower.sh        # needs ccomp on PATH
```

Outputs: binaries in `compcert/build/`, assembly listings in
`compcert/asm/`.

Scope, stated plainly:

- CompCert proves **C → assembly semantic preservation**. It
  does not prove the C programs correct; that evidence remains
  the test suites, the execution records in STATUS.md, and the
  Lean theorems in `core/proofs/`.
- CompCert has no wasm32 target. The per-program WASM modules
  stay clang-built; CompCert lowers the **host toolchain** that
  produces them and the seal that signs the descent.
- The seal enters this pipeline as `crypto/seal_recursion.c`,
  the C11 port of the owner's HolyC `crypto/seal_recursion.hc`;
  the port's output is byte-identical to the verified reference
  run recorded in STATUS.md.
