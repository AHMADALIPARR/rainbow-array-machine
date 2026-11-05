# STATUS

## v0.1 — as received (2026-11-04)

Extracted from the owner's zip and committed untouched: the C11
interpreter (`src/ram.c`), demo, tests, Makefile, README. Its suite
passes as received: 6 positive cases, 2 negative cases. The zip
shipped no LICENSE; the sovereign license was applied as the repo
license per the owner's standing rule.

## v0.2 core — the typed compiler IR (2026-11-04)

The first of the owner's "what comes next" list, built as new code
(`src/ramc.c`); `src/ram.c` is unchanged.

- Pipeline: source lines → typed IR → execution of the IR. Every
  instruction lowers to canonical SSA nodes with explicit result
  types (kind, rank, shape), canonical opcodes independent of the
  emoji surface, and source line positions. `ramc --emit-ir` prints
  the IR (e.g. `%2 = bqn.add %0, %1 : vector[3] @L2`).
- Bindings: a line may open `NAME ←` to bind its result; `@NAME`
  references it as an operand. Kinds and shapes are checked at
  IR-build time: unbound references, duplicate bindings, kind
  mismatches, and shape mismatches are refused before execution.
  Dimension headers size literal operands; a reference carries its
  own shape and sizes any literal that follows it.
- Registered-but-unimplemented namespaces and the reserved tensor /
  ternary types fail closed at the IR stage.
- Verified: `make test-ir` — ramc's output on `examples/demo.ram`
  is byte-identical to v0.1's `ram`; bindings cases compute
  ([25,49,81], sum 21, a bound matmul product transposed to
  [19,43,22,50]); 6 negative cases refused. `make test` (the v0.1
  suite) still passes unchanged.

## The remaining "what comes next" list (owner's order)

1. ~~A genuine typed compiler IR~~ — done above (v0.2 core).
2. Persistent tensor buffers — named buffers that outlive a line,
   on the binding mechanism, with the 🧊 tensor type's storage.
3. Ternary arithmetic — the reserved 🔱 type gets semantics and
   storage (the owner's ternary kernels are the reference shape).
4. Church-3 reduction — combinator reduction into the typed IR
   (README design constraint 6), bounded evaluation.
5. Unlambda execution — the 🐦 namespace executes, through the IR.
6. The flat decoder — the v0.6 reference: the array machine runs
   the decoder workload end to end.
