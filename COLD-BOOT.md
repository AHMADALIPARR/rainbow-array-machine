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
baseline_commit: 6733ae28752d791d5505d9608da9010fed662659
architecture: WebAssembly Stack Machine
execution_target: WebAssembly (.wasm)
serialization: skull/crown (.skull) — decoded before load; not an execution layer
entry_points: README.md, STATUS.md, Makefile, src/ram.c, src/ramc.c, src/ram_array_runtime.py, src/ram_runtime.py, src/ram_geometry.py, src/ram_traceback.py
verify_command: ./cold-boot.sh
suites: test,test-ir,test-flat,test-skull,test-inv,test-geom,wat-test,geometry-test,runtime-test,array-runtime-test,traceback-test
<!-- COLD-BOOT:FIELDS:END -->

seal: 0e1f0c23a24bf73b19df5c10f4d9d0dcdd6de1b45e61b65db57bceb3d968ccae

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
7. **Artifacts** — the generative traceback re-derives the
   symbolic module from its source and demands byte equality;
   the C skull decoder reproduces a runtime `.wasm` from its
   shipped `.skull`.

The pass prints PROVEN (executed in this run, with counts),
DECLARED (the sealed fields), and the open list below, and exits
nonzero on any failure. A README claim is not a verification;
only the pass output is.

## Proven at the baseline

- Eleven suites green (see STATUS.md for the per-version record).
- Geometry and the array/symbolic subsets execute in real WASM
  binaries, with runtime assertions and tamper evidence.
- The 💀👑 encoding round-trips byte-for-byte across two
  independent implementations (C and Python).
- The generated affine Lean obligation checks in a real Mathlib
  environment (recorded in STATUS.md with the environment named).

## Claimed / open — not proven, do not merge the lists

- The two runtime compilers are not yet unified into one general
  typed-FLAT → WASM lowering pipeline.
- Persistent tensor buffers, ternary arithmetic, Church-3
  reduction, Unlambda execution, and the flat decoder remain on
  the owner's "what comes next" list in STATUS.md.
- `traceback-proof` inside this repo fails closed: no Lean
  project is configured here.

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
