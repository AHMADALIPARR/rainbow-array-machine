# COLD-BOOT — Rainbow Array Machine

The front door of this repository. An agent arriving cold reads
this file first and learns, in one pass, who owns the work, what
the license allows, where the verified baseline is, what is
proven, what is only claimed, and the exact command that
re-verifies all of it. This file scopes and verifies; it grants
nothing — no commercial authorization, no push, no publish, no
license change. Those remain the owners' explicit go-ahead.

## Sealed fields

The block between the markers is the canonical field set. The
seal is the SHA-256 of those exact lines, in order, one per line,
excluding the markers themselves. `cold-boot.sh` recomputes it
and refuses to proceed past a mismatch.

<!-- COLD-BOOT:FIELDS:BEGIN -->
protocol: cold-boot/1
repository: rainbow-array-machine
owners: Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty
license_file: LICENSE
license_spdx: LicenseRef-Sovereign-Leviathan-SL-AGPL3-001
license_sha256: 6f26b3c4a6017597ef0a302b31125dbb9848b77129d598b503ad33818acbf8a6
baseline_commit: 38577c3df150b87890e7021079cb558e4d1cdbbe
architecture: WebAssembly
execution_target: WebAssembly (.wasm), standalone per-program modules
serialization: skull/crown (.skull) — decoded before load; not an execution layer
entry_points: README.md, STATUS.md, Makefile, core/Makefile, core/src/ramc.c, core/src/kernel.c, src/ram.c, src/ramc.c
verify_command: ./cold-boot.sh
suites: test,test-ir,test-flat,test-skull,test-inv,test-geom,test-array-c,core-test
<!-- COLD-BOOT:FIELDS:END -->

seal: d53a2305bdb4e3248ab246b6eb4a317f41faa33e5c295123b4734acb239661ff

## The scoping pass

Run `./cold-boot.sh` (or `make cold-boot`) from the repo root.
In order, failing closed at the first broken gate:

1. **Seal** — recompute the field-block hash; compare to `seal:`.
2. **License** — `LICENSE` exists, its SHA-256 matches the sealed
   field, and its SPDX identifier matches. An MIT license bearing
   the owners' names is by definition not theirs; if one appears
   here, stop.
3. **Baseline** — the sealed baseline commit exists and is an
   ancestor of HEAD. Everything claimed as proven was verified at
   or before that commit.
4. **Identity** — `make report` states the sealed architecture.
5. **Build** — the C11 front end builds clean (`-Werror`).
6. **Suites** — every suite in the sealed list runs and passes.
7. **Artifacts** — the core theorem file carries the
   strengthened transpose statement and no `sorry`; the C skull
   decoder reproduces a runtime `.wasm` from its shipped `.skull`.

The pass prints PROVEN (executed in this run, with counts),
DECLARED (the sealed fields), and the open list below, and exits
nonzero on any failure. A README claim is not a verification;
only the pass output is.

## Proven at the baseline

- Eight suites green, including the core suite: the FLAT2
  pipeline compiles the examples and the showcase to standalone
  WASM modules whose operations and assertions execute in-module
  (see STATUS.md for the launch record).
- The 💀👑 encoding round-trips byte-for-byte (repo C codec).
- The three theorems in core/proofs/Arithmetic.lean are
  kernel-checked with lake build in a Mathlib v4.9.0 environment
  (exit 0; method and scope recorded in STATUS.md).
- CompCert 3.12 lowers all 10 host C programs to assembly
  (compcert/asm/), differentials byte-exact against the gcc builds
  (seal output sha 3b2ae65b...; core tools' IR/FLAT2/generated C
  identical on polynomial + showcase).

## Claimed / open — not proven, do not merge the lists

- FLAT2 is a validated metadata+payload format, not yet an SSA
  IR with buffer allocation or a type-soundness proof; WASM↔spec
  equivalence is not claimed (see core/IMPLEMENTATION_STATUS.md).
- Runtime-input ABI, persistent tensor buffers, ternary
  arithmetic, Church-3 reduction, Unlambda execution, and the
  flat decoder remain on the owner's "what comes next" list in
  STATUS.md.
- No Lean project is configured in this repo; theorem checking
  runs in the external Mathlib environment named in STATUS.md.

## Invariants an arriving agent does not rewrite

- WASM-first: C11 is a build-time compiler; WebAssembly is the
  execution target; 💀👑 is serialization, never execution.
- Overflow semantics differ by backend on purpose until the
  owner rules otherwise: `flat.c` refuses, WASM wraps (declared
  in the manifests).
- User-supplied code in this tree is verbatim; fixes are offered,
  never silently installed.
- Born public stays public; publication is visibility and
  provenance, not a transfer of rights.
