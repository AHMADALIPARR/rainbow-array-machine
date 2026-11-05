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

## v0.4 — as received (owner's zip, 2026-11-04)

Committed verbatim (`121bef5`): the geometry compiler,
`src/ram_geometry.py` — one-line GRID definition (name, nx ny nz,
scalar spacing; integer lattice points, x-fastest), the @@ bridge
with ROTATE_Z quarter turns and TRANSLATE plus their ↶ inverses,
compile-time SHAPE and POINT assertions, and a Geometric FLAT IR
sidecar (JSON manifest: shape, offsets, point count, i32 dtype)
with a WAT memory image. His suite passes (5 unittest cases);
regeneration reports "48 bytes of geometry; 6 assertions passed."
Scope as he states it: RAM-specific syntax, not native BQN/KCL;
@@ is a proposed bridge notation, not a native KCL operator;
coordinates are precomputed by the compiler, not a runtime
transform kernel; assertions are checked against the finite
sample, not universally proven; WAT still unassembled.

## Milestone — reversible algebra + geometry in the typed IR

The owner's two design messages (assertion/inverse algebra;
BQN geometry layer with the @@ bridge) implemented in `ramc`,
with geometry semantics aligned to his v0.4 artifact where the
earlier prose sketch differed (one-line GRID, scalar spacing,
POINT index form, definition shape invariant under transforms —
the points carry the geometry):

- `>>>>>>>€` assertions on any statement: value literals in
  bracket form, verdicts (👑/💀), geometry SHAPE and POINT fields;
  several POINT assertions may attach to one statement.
- `↶` registered inverses: add, multiply, transpose, reverse are
  exact; flatten and reshape inverses require saved shape
  metadata — IR provenance via a bound reference, or explicit
  🗳️ reconstruct — and compilation fails without it; sum, pick,
  select and shape have no inverse and compilation fails.
- `⟲` one-line round trips and the `🛡️ ROUNDTRIP` certificate
  (`CERT v1: roundtrip; op=...; status=VERIFIED_ON_TESTED_INPUTS`)
  over a forward/inverse assertion pair.
- ⚖️ scalar compare yielding a verdict value.
- Bracket operands throughout the array grammar.
- Geometry as a first-class IR type: GRID definition, @@ bridge
  (ROTATE_Z / TRANSLATE, plus ↶ forms), BQN axis operations
  ⥊ reshape, ⍉ transpose, ↕ reverse, ⊑ pick, ⊏ select.

Cross-implementation verification (`make test-geom`): ramc runs
the owner's `examples/geometry/grid.ram` and passes exactly the
6 assertions his compiler reports; the final lattice points match
the payload tuple his test suite pins. Also verified: the algebra
demo (`examples/inverse.ram`, 11 assertions + certificate + 7
negative cases) and full regression — `make test`,
`make test-ir` (demo output still byte-identical to ram),
`make test-flat`, `make test-skull`, `make wat-test`,
`make geometry-test` all pass. `skull invert` (bitwise NOT on
the encoded form) also landed with this milestone: the owner's
0x65 vector, involution, and NOT semantics cross-checked over
all byte values.

## v0.5 — as received and checked (owner's zip, 2026-11-04)

Committed verbatim (`771f814`): `src/ram_runtime.py` closes the
pipeline the owner drew — it hand-assembles a real WebAssembly
binary (his own ULEB/SLEB/section encoders, no external
assembler), in which the geometry transforms and the POINT
assertions execute at runtime inside `run()`; SHAPE assertions
are compile-time. Emits `.wasm`, a `.wat` listing of the same
instruction sequence, the `ram-runtime-v1` manifest, and the
`.skull` encoding of the binary.

Checked by execution here (the owner's request):
- `make runtime-test` passes: Node instantiates the binary,
  `run()` applies 9 operations to 4 points, final memory matches
  the manifest's expected points.
- Tamper evidence: flipping one initial-point byte in a copy of
  the binary makes `run()` return −1 (the in-WASM assertion
  fires) and the harness rejects it — the execution and the
  assertions are real, not ceremonial.
- Regeneration of all four artifacts is byte-deterministic.
- 💀👑 interoperability, both directions: this repo's C decoder
  (`skull decode`) reproduces the `.wasm` from the owner's
  `.skull` byte-for-byte, and the owner's Python decoder
  reproduces the `.wasm` from the C encoder's canonical
  `.skull`. The two implementations of the alphabet agree; only
  line wrapping differs, and both decoders ignore whitespace.
- The `.skull` opens with the WASM magic in skull/crown form,
  matching the acceptance fixture from the alphabet milestone.
Scope, as his README states it: one grid, integer coordinates,
quarter-turn Z rotations, integer translations, point/shape
assertions; no floating point, no arbitrary geometry, no Wolfram
Engine integration.

## Architectural identity (owner, 2026-11-04)

Rainbow Array Machine is a **WebAssembly stack**, WASM-first:
the C11 compiler is a build-time tool and WebAssembly is the
execution target. BQN/APL/KCL/Wolfram are source semantics; the
typed FLAT IR carries geometry, tensors, inverses, and
assertions; skull/crown is a **serialization format** for the
WASM binary — it is decoded before a standard runtime loads the
module, and is not itself an execution layer. `make report`
prints the stack's identity block in the owner's form. One
distinction he states and this file preserves: the current WASM
runtime executes the implemented geometry subset, not yet the
entire array and symbolic instruction set — that lowering is the
open frontier, and nothing here claims otherwise.

## v0.6 — as received and checked (owner's zip, 2026-11-04)

Committed verbatim (`0ea3217`): `src/ram_array_runtime.py`
lowers the registered array and symbolic subsets to runtime
WASM, closing the gap named at v0.5 for those subsets. Vector
add/subtract/multiply (BQN/APL/K, fixed-length f64) execute in a
runtime loop over linear memory — operands at fixed offsets,
result computed by the module, nothing precomputed. The Wolfram
affine subset becomes `run(x:i64)->i64` in the module, with
modulo-2^64 arithmetic declared in its manifest. Each module
ships with WAT, a FLAT-layout manifest, and its `.skull`.

Checked by execution here:
- His Node harness passes all four modules (vector [5,7,9],
  APL multiply [4,10,18], K subtract [−3,−3,−3], affine
  run(x) = 5x+7 on the tested points) plus per-module skull
  round trips.
- All 16 artifacts regenerate byte-deterministically.
- This repo's C skull decoder reproduces all four `.wasm`
  binaries from his `.skull` files byte-for-byte.
- The declared wrapping model verified at the boundary:
  run(2⁶²) equals the modulo-2⁶⁴ value exactly.
- Tamper evidence: editing operand B in a copy of the vector
  module's memory image changes the runtime output to the
  correctly recomputed [5,7,10] — the loop computes.
Scope, as his README states it: this is not yet a complete
lowering of all array, geometry, or Wolfram operations — the
vector subset is fixed-length f64, the symbolic subset is affine
i64, and the two runtime compilers are not yet unified into a
general typed-FLAT lowering pipeline. Unsupported operations
are rejected. All ten test targets pass.

## v0.7 — as received and checked (owner's zip, 2026-11-04)

Committed verbatim (`1c61087`): the symbolic metaprogramming
inversion layer — `src/ram_traceback.py`, a generative
traceback. Given a source and a WASM artifact, it recompiles the
source deterministically and requires byte equality with the
artifact (fail closed: altered binaries and wrong sources are
rejected, both covered by his tests), then *generates* a Lean
proof obligation for the pair — the affine coefficient identity
over ℤ, by `ring`; the pointwise array map over ℤ, by `rfl` —
with a `.trace.json` evidence record (SHA-256 of source and
artifact, manifest, obligations, scope). His scoping is exact
and is preserved here: this is artifact/source consistency and
an abstract algebraic obligation — NOT a proof that the WASM
module implements the identity, not IEEE-754 or compiler
correctness, and not independent reverse engineering.

Checked by execution here:
- His traceback suite passes (4 cases); regeneration of
  `proofs/Symbolic.lean` and the trace record is
  byte-deterministic; both recorded hashes match the artifacts.
- **The generated proofs were actually run through Lean.** In
  the workspace's Mathlib environment (scm-mathlib-test lake
  project), `lake env lean` checks both obligation kinds: the
  affine identity and the generated vector pointwise obligation.
  The shipped trace record keeps its generation-time status
  (`lean_status: not_checked`) — that field describes the state
  at generation on the author's side; the check recorded here is
  a separate, later verification in a named environment. The
  repo's own `traceback-proof` target still fails closed, as
  designed: this repo has no configured Lean project.
Eleven test targets pass.

## Cold Boot — the process, first implementation (2026-11-04)

The owner's Cold Boot idea (drafted 2026-11-03, never approved
until his go-ahead tonight) is now a working process in this
repo, new files only: `COLD-BOOT.md` at the root and the
executable pass `cold-boot.sh` (also `make cold-boot`).
Decisions taken at his "create it" and recorded here: the front
door is a standalone `COLD-BOOT.md`; the first repo is this one;
the seal is the minimal v0.1 form — SHA-256 over the canonical
field block in the file (owners, license file + SPDX + SHA-256,
baseline commit, architecture, entry points, suite list).

Verified by execution:
- Full pass on the sealed tree: **18 gates PASS** — seal,
  license hash, license SPDX, baseline ancestry, stack identity,
  build, all eleven suites, traceback byte-equality, and the
  skull decode round trip.
- Negative test in a scratch clone: a one-character change to a
  sealed field makes the seal gate FAIL and the pass exit
  nonzero. The seal bites.
The pass prints PROVEN (executed in the run) separately from
DECLARED (the sealed fields) and grants nothing: no commercial
authorization, no push, no publish, no license change.

## README — production form (owner's order, 2026-11-04)

The README is rewritten in production form: badges (license,
architecture, source, cold boot, suites, encoding), the program
shown with its actual executed outputs (demo, assertions,
geometry, the WASM modules), the stack report, a components
table, the topic list for the repo's GitHub page, and a license
section. License audit at the same pass: exactly one LICENSE
file in the tree (the sovereign text, hash matching the
Cold Boot seal), the only copyright line in the tree is
SnapKitty West's, and no MIT license text exists anywhere in
the tree. Cold Boot still passes end-to-end (18 gates) with the
new README in place; the tree is clean.

## C11 port, stage 1 — the array runtime (owner's call, 2026-11-04)

The owner flagged that the generator layer is Python while the
stack is WASM. The Python generators are his own shipped code
(zips v0.3–v0.7, integrated verbatim); the WASM binaries were
never Python — but the toolchain now moves to C11, stage by
stage, with byte identity as the acceptance test:

- `src/ram_array_runtime.c` is a full C11 port of
  `ram_array_runtime.py`. All 16 artifacts it emits (4 modules ×
  .wasm/.wat/.flat.json/.skull) are **byte-identical** to the
  Python generator's, and the C-built modules pass the Node
  harness (`make test-array-c` regenerates in place and the
  tracked tree does not change). The Python original stays in
  the tree untouched; the C twin is the path forward.
- Still Python, staged next in the same pattern: the geometry
  compiler (`ram_geometry.py`), the geometry runtime
  (`ram_runtime.py`), the WAT backend (`ram_wat.py`), and the
  traceback (`ram_traceback.py`).
Cold Boot still passes (18 gates); the sealed suite list is
unchanged this stage.

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

## v1 production launch (owner's order, 2026-11-05)

- The execution core is the owner's rebuild line, integrated at
  core/ exactly as shipped (rainbow-wasm flat2 tree): ramc
  (RAMIR1), flat_check (FLAT1), flat_lower (FLAT2), ram_codegen,
  kernel.c, the three examples, his README and IMPLEMENTATION_STATUS
  carried verbatim. Two disclosed integration deltas:
  (1) ram_codegen.c carries a one-line fix — the FLAT2 header
  re-read now checks fgets' return value; the shipped line fails
  -Werror=unused-result under GCC 13 with glibc fortify;
  (2) proofs/Arithmetic.lean is the owner's transpose-proof patch
  (strengthened conjunction), lake-verified in the Mathlib v4.9.0
  environment: lake build exit 0, all three theorems kernel-check.
- examples/showcase.ram (assistant-composed from the shipped
  operators) is the README program: one module, five instructions,
  17 outputs [5,7,9,58,64,139,154,0,1,0,-1,0,0,1,0,0,27], executed
  by core's suite in this launch verification.
- Python and JavaScript strip-out: all 8 .py and 2 .js files are
  removed from the tree, with the generated artifacts of the
  retired Python backends (examples/wat/*.wat, the v0.4/v0.5
  geometry runtime outputs). The v0.1–v0.7 .ram sources and the
  C line (interpreter, typed IR, FLAT, skull, C array-runtime
  port) stand. proofs/Symbolic.lean and its trace remain as
  standing artifacts; their generator is retired. One disclosed
  residue: tests/run-skull.sh (assistant test tooling) still uses
  an embedded Python reference encoder as a cross-check oracle —
  test tooling only, no Python ships in the stack; a C port of
  the oracle is open work.
- Skull formats: the core's shipped JS serializer and the repo C
  codec are the same MSB-first encoding; the C codec frames it
  with a newline per byte (33x) where the JS wrote the unbroken
  stream (32x). The repo codec is canonical in-tree; the formats
  are not silently unified.
- Makefile: Python targets (wat-test, geometry-test, runtime-test,
  array-runtime-test, traceback-test, traceback-proof) removed;
  test-array-c regenerates the C port's artifacts (byte-identical,
  verified by a clean tree after regeneration); core-test runs the
  core suite. README is the v1 production README (executing
  program, measured benchmarks, invariants, theorems).
- Benchmark method (README figures): pipeline stages timed with
  Python's perf_counter around subprocess runs (median of 5);
  execution timed in Node over 20,000–50,000 run() calls per round
  (median of 5 rounds), AMD EPYC 9D25 VM, Clang 23, Node 24.
- Cold Boot resealed on the v1 baseline (this launch): field block
  updated (baseline = the launch integration commit, suites = the
  eight surviving suites, entry points name the core), seal
  cd1b29dd1d6f19063b52d9174b3f0674f6364e1436467e3147f1404740a1ed70;
  the retired traceback artifact gate is replaced by a core
  theorem-file integrity gate. Full pass at reseal: 16/16 gates.
