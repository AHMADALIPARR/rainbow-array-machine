# 🌈💀👑 Rainbow Array Machine — v1

![License: Sovereign Leviathan](https://img.shields.io/badge/license-Sovereign%20Leviathan-blueviolet)
![Architecture: WebAssembly](https://img.shields.io/badge/architecture-WebAssembly-654ff0)
![Toolchain: C11 + Clang wasm32](https://img.shields.io/badge/toolchain-C11%20%2B%20Clang%20wasm32-blue)
![Python: none](https://img.shields.io/badge/python-none-success)
![Theorems: lake-checked](https://img.shields.io/badge/theorems-lake%20build%20%E2%9C%93-brightgreen)
![Encoding: Skull/Crown](https://img.shields.io/badge/encoding-%F0%9F%92%80%F0%9F%91%91%20.skull-black)

An array machine whose source language is UTF-8 emoji and whose
execution target is **WebAssembly**. Every program compiles to
its **own standalone WASM module** — parser, typed FLAT stages,
and code generator are C11; Clang's wasm32 target produces the
module; the operations and the assertions execute inside it. 🕸️

🌀 **BQN** · 🔺 **APL** · ⚡ **K** · 🐺 **Wolfram subset** · 📐 **geometry**
(namespace tags for a bounded, explicitly defined subset — not
full implementations of those languages)

---

## ▶️ A program, executing

One program, five instructions — vectors, matmul, geometry, an
inverse, and the affine — compiled into a single module:

```
🌀 📏 ➕ 3 1 2 3 4 5 6 >>>>>>>€ 5 7 9
🌀 🟦 🧲 2 3 2 1 2 3 4 5 6 7 8 9 10 11 12 >>>>>>>€ 58 64 139 154
🌀 📐 @@ ROTATE_Z 2 1 1 0 0 0 1 0 >>>>>>>€ 0 1 0 -1 0 0
↶ 🌀 📐 @@ ROTATE_Z 1 1 0 1 0 >>>>>>>€ 1 0 0
🐺 🔹 📜 2 3 7 4 >>>>>>>€ 27
```

```sh
make -C core modules        # ramc → flat_check → flat_lower
                            # → ram_codegen → clang wasm32
```

```
$ run showcase.wasm
PASS standalone WASM: 17 outputs
[5,7,9, 58,64,139,154, 0,1,0,-1,0,0, 1,0,0, 27]
```

`run()` returns 0 only when every operation **and every
assertion inside the module** passes. A false assertion returns
a nonzero code from the WASM itself — the host computes nothing. 🛡️

---

## 🏗️ Pipeline

```
🌈 emoji source (.ram)
  → ramc        C11 strict parser            → RAMIR1
  → flat_check  C11 shape/op validation      → FLAT1 metadata
  → flat_lower  C11 metadata + payload       → FLAT2
  → ram_codegen C11 standalone lowering      → C source
  → clang       wasm32                       → 📦 program.wasm
  → run()       operations + assertions execute in-module
💀👑 .skull      reversible serialization of the .wasm bytes
```

---

## 📊 Benchmarks

Measured by execution (2026-11-05, AMD EPYC 9D25 VM, Clang 23,
Node 24; medians of 5 runs). Reproduce with `make test` and the
timing method in STATUS.

**Compile pipeline — the showcase program, source → module:**

| Stage | Median |
|---|---|
| ramc (parse → RAMIR1) | 1.08 ms |
| flat_check (→ FLAT1) | 1.43 ms |
| flat_lower (→ FLAT2) | 1.43 ms |
| ram_codegen (FLAT2 → C) | 2.87 ms |
| clang (C → wasm32) | 234.26 ms |
| **Total** | **≈ 241 ms** |

**Modules and execution:**

| Module | Size | Instantiate | `run()` median | Throughput |
|---|---|---|---|---|
| array.wasm (5 instr) | 3,093 B | 0.94 ms | 0.094 µs | ≈ 10.6 M runs/s |
| geometry.wasm (5 instr) | 2,984 B | — | — | — |
| symbolic.wasm (1 instr) | 2,298 B | 0.38 ms | 0.027 µs | ≈ 37.0 M runs/s |
| showcase.wasm (5 instr) | 2,975 B | 2.29 ms | 0.170 µs | ≈ 5.9 M runs/s |

**Serialization:** a 3,093-byte module encodes to a 98,976-byte
`.skull` — exactly 32× (8 symbols per byte × 4 UTF-8 bytes per
symbol), byte-for-byte reversible. 💀👑

---

## 🔒 Invariants

These hold by construction and are exercised by the test suites:

1. **Per-program modules.** Each program compiles to its own
   standalone `.wasm`. There is no instruction interpreter and
   no shared dispatcher in the execution path.
2. **Assertions execute in WASM.** `>>>>>>>€` expectations are
   compiled into the module and checked by the kernel's
   `assert_equal` (tolerance 1e-10) inside `run()`; failure
   returns a nonzero code identifying the instruction.
3. **The host computes nothing.** A harness instantiates the
   module, calls `run()`, and reads exported linear memory.
4. **Division by zero is refused** — the elementwise kernel
   returns an error code, never an infinity.
5. **Kernel bounds are enforced in-module:** elementwise
   n ≤ 4096, matmul dimensions ≤ 64, geometry points ≤ 1024.
   Violations return an error code; nothing is clamped silently.
6. **FLAT2 integrity.** Every executable payload line travels
   paired with its validated FLAT1 metadata line. A missing
   payload or a count mismatch is refused before code generation.
7. **The frontend fails closed.** Unknown namespaces or
   opcodes, missing operands, non-finite numbers, and trailing
   input are rejected with a nonzero exit — no partial program
   is ever lowered.
8. **↶ is exact for geometry.** Inverses are the parse-time
   negation of quarter turns and translation deltas; a transform
   followed by its inverse returns the declared points exactly.
9. **💀👑 is serialization, never execution.** A `.skull` is
   decoded to bytes before any runtime loads the module.
10. **No scripting languages in the build path.** Parser, FLAT
    stages, and code generator are C11; the compiler target is
    Clang wasm32.

---

## 📜 Theorems

`proofs/Arithmetic.lean` — mathematical specifications of the
kernels, **kernel-checked with `lake build`** (Lean v4.9.0,
Mathlib v4.9.0; build exit 0, no `sorry`):

```lean
def affineSpec (a b c x : ℤ) : ℤ := (a + b) * x + c

theorem affine_expand (a b c x : ℤ) :
    affineSpec a b c x = a * x + b * x + c

theorem inverse_translation (x delta : ℤ) :
    (x + delta) - delta = x

theorem transpose_index_roundtrip (m n i j : ℕ)
    (hi : i < m) (hj : j < n) :
    (j * m + i) / m = j ∧ (j * m + i) % m = i
```

Scope, stated plainly: these theorems specify the **mathematics**
the kernels implement — the affine law, translation inversion,
and the row-major transpose index decode (both axes recovered).
They do not claim instruction-level equivalence between the
specification and the emitted WASM; that requires a formal WASM
semantics and a simulation proof, and it is not claimed here. 🧾

---

## 🧩 Components

| Piece | Role |
|---|---|
| `core/src/ramc.c` | 🌐 Strict UTF-8 emoji parser → RAMIR1 (↶ geometry inverses at parse) |
| `core/src/flat_check.c` | 🏷️ RAMIR1 validation → FLAT1 typed metadata (rank, shape, elements, assert flag) |
| `core/src/flat_lower.c` | 📐 FLAT1 + payload → FLAT2, count-checked pairing |
| `core/src/ram_codegen.c` | ⚙️ FLAT2 → standalone C module source |
| `core/src/kernel.c` | 🕸️ The WASM kernel — elementwise (➕➖✖️➗), matmul, transpose, reduce_sum, affine, rotate, translate, assert_equal — compiled by Clang to wasm32 |
| `src/skull.c` | 💀👑 Skull/Crown encode/decode/invert — the repo codec; it frames the same MSB-first symbol stream with a newline per byte |
| `core/proofs/Arithmetic.lean` | 📜 The theorems above |

---

## 🔐 V3 — the ASTRA-VAULT layer (`crypto/`)

Hand-rolled cryptography and the mathematics it seals with —
owner-authored, carried verbatim, verified by execution;
findings are recorded in STATUS.md, never silently patched.

| Artifact | What it is | Verification status |
|---|---|---|
| `crypto/astra-vault-unlambda.unl` | 🌌 Madhava's π/sine series + Qin Jiushao's Dayan (CRT) closure in pure Unlambda combinators | Numerals execute with exact iteration counts (variant encoding); 21 of 28 definitions are open fragments completed textually at use sites; `mod` is referenced but undefined; `odd` is textually the numeral 3 — conversion to standard Unlambda is the open step |
| `crypto/astra_math_vault.m` | 🧮 Recursive mathematical vault — layered bases, entropy statistics, deviation analysis, invariants, compression over a synthetic trillion-parameter field | Runs end to end (Octave record in STATUS); construction-consistent figures verified (RMS ≈ 1, mean \|z\| ≈ √(2/π)); two entropy-semantics findings recorded (hash-pooled bins; discrete-vs-differential delta) |
| `crypto/seal_recursion.hc` | 🔏 The seal — 2070 descending by 10s to 0 as a 12-bit 💀👑 grid, with inversion-carry detection | Executes via the C11 port: 208 rows, carry at row 1 col 7 (bit 4), 2070 → 2060 |
| `crypto/seal_recursion.c` | The C11 port of the seal — the form that enters the CompCert pipeline | Output identical to the verified reference run |

---

## ⚙️ CompCert lowering (`compcert/`)

The host-side C — the seal, the four core pipeline tools, and
the v0.1-line tools — lowers to assembly through **CompCert**,
the verified C compiler whose Coq-checked theorem guarantees
the assembly preserves the C semantics. `compcert/lower.sh`
performs the lowering and the differential re-verification of
the CompCert-built binaries against the reference suites.

Scope: CompCert proves C → assembly preservation; it does not
prove the C itself, and it has no wasm32 target — the
per-program WASM modules stay clang-built, and CompCert lowers
the host toolchain that produces them. Toolchain status and
lowering results are recorded in STATUS.md.

---

## 🛣️ Open frontier

- FLAT2 carries validated metadata with the payload; it is not
  yet an SSA IR with buffer allocation or a type-soundness proof
- Full BQN/APL/KCL/Wolfram semantics are not implemented; the
  namespaces tag a bounded subset
- Runtime-input ABI (programs that take external data at run
  time) is future work; v1 programs carry their constants
- WASM↔specification equivalence proofs are not claimed

---

## 🏷️ Topics

```
rainbow-array-machine, webassembly, wasm, emoji-programming,
array-programming, bqn, apl, k, wolfram, compiler, typed-ir,
flat-ir, skull-crown, cold-boot, provenance, reversible-computing,
lean, mathlib, c11, clang
```

---

## ⚖️ License

**SNAPKITTY OPAQUE SOURCE LICENSE v1.0** —
`LicenseRef-Sovereign-Leviathan-SL-AGPL3-001` © 2026 SnapKitty West.
Owners: **Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty**.
Publication is not commercial authorization; commercial use
requires separate written authorization from the owners. 📜
