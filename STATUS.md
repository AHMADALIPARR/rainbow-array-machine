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

## v0.2 — as received (owner's zip, 2026-11-04)

Committed verbatim (`6d441b1`): the 🐺 Wolfram namespace joins
`src/ram.c` — a restricted, independently implemented symbolic
polynomial subset (a·x + b·x + c, coefficients bounded to ±10⁶)
with ✨ simplify (collect like terms), 📜 FLAT v1 lowering (typed
SSA-like textual IR), and 🛡️ an algebraic certificate whose scope
is stated exactly (the coefficient identity over mathematical
integers; not an overflow proof). Suite as shipped: 9 positive,
4 negative — passes.

## Milestone — FLAT is executable; symbolic layer meets tensors

The owner's named milestone, built on the v0.2 arrival: `src/flat.c`
is a FLAT v1 virtual machine. It parses exactly the grammar 📜
emits (single-assignment i64 chains over one input; mul_i64 /
add_i64) and executes it in two modes:

- **Scalar** — `flat prog.flat <integer>` prints f(x).
- **Tensor** — `flat prog.flat <tensor-file>` applies f elementwise
  to a scalar/vector/matrix and prints in ram's array format: the
  symbolic expression layer now drives actual tensor operations.

FLAT discipline: redefinition of a temp, an undefined reference, an
unknown instruction, or a version other than v1 are refused, and
i64 overflow is checked at every step and refused — never wrapped
(the 🛡️ certificate does not cover overflow; the executor does).

Verified (`make test-flat`): the 📜 emission round-trips — five
scalar cases agree with independent shell arithmetic; tensor mode
maps [1,2,3,4] to [12,17,22,27], agreeing element-for-element with
ramc computing the same affine map by array operations; five
negative cases refused, including LLONG_MIN × −1. All three suites
green: `make test` (9+4), `make test-ir`, `make test-flat`.

Standing seam, stated plainly: two IRs now exist — ramc's typed IR
for the array core and FLAT v1 for the symbolic subset — and they
are tied numerically by the cross-check above, not yet unified.
Unifying them is a future decision for the owner.

## v0.3 — as received (owner's zip, 2026-11-04)

Committed verbatim (`2590d94`): a WAT backend, `src/ram_wat.py`,
lowering exactly one statement per module — BQN/APL/K vector
add/subtract/multiply with f64 constants embedded, results stored
to exported linear memory; and the 🐺 📜 symbolic affine as an
exported `run(x:i64)->i64`. `examples/wat/` holds both source/.wat
pairs; `make wat-test` regenerates and tests them.

Verified scope, stated as the owner stated it: **the compiler
emits WAT; the resulting .wasm has not been assembled or executed
in this drop. The Wolfram layer is the standalone compatible
subset, not an integration with Wolfram Engine.** Additionally
verified here: the generation and rejection tests pass, and
regeneration is byte-deterministic (md5-identical .wat outputs).

Two facts for the follow-on step: Node is present on this VM and
can execute .wasm; `wat2wasm` (WABT) is absent — assembling is the
single missing piece. Auditor note, no action taken: the WAT
symbolic function uses wrapping i64 WebAssembly arithmetic (so
stated in the owner's README), while `flat.c` refuses overflow —
two backends currently carry two overflow semantics; whether they
should agree is the owner's call.

## Skull & Crown binary alphabet — built to the owner's spec (2026-11-04)

`src/skull.c`: the owner's binary representation system — 💀
(U+1F480) is 0, 👑 (U+1F451) is 1, MSB first, byte-aligned,
exact symbol matching. `skull encode` / `skull decode` convert
between raw bytes and `.skull` text losslessly; canonical form is
one byte per line, the layout of the owner's worked example. The
decoder skips ASCII whitespace only and refuses any other
codepoint and any symbol count not divisible by eight.

This is the same alphabet the owner used for machine-bit
inspection on the Jetson (Bonsai work); per his direction it is
now the machine's canonical binary alphabet for binaries, tensor
weights, instruction streams, and verification fixtures — the
first fixture is in the tree: `tests/fixtures/wasm-magic.skull`.

Verified (`make test-skull`): the WASM magic bytes 00 61 73 6D
encode to exactly the owner's four lines and decode back; an
independent reference encoder agrees with the C implementation
on all 256 byte values in both directions; a real artifact
(symbolic.wat) round-trips byte-identically; the size law checks
arithmetically (221 bytes → 7,293: 32x of symbols plus one
newline per byte in canonical form); misalignment, a foreign
emoji, and an ASCII digit are refused.

Pipeline state, honestly: source → typed IR → WAT all exist and
execute; the .wasm stage is still unassembled (see v0.3 note),
so `.skull` has been exercised on every other artifact class and
takes a `.wasm` unchanged the day the assembler lands — the
encoder neither knows nor cares which bytes it is given.

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
