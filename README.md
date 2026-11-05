# 🌈💀👑 Rainbow Array Machine

![License: Sovereign Leviathan](https://img.shields.io/badge/license-Sovereign%20Leviathan-blueviolet)
![Architecture: WebAssembly](https://img.shields.io/badge/architecture-WebAssembly-654ff0)
![Source: UTF-8 Emoji](https://img.shields.io/badge/source-UTF--8%20emoji-orange)
![Cold Boot: 18 gates PASS](https://img.shields.io/badge/cold%20boot-18%20gates%20PASS-brightgreen)
![Suites: 11 passing](https://img.shields.io/badge/suites-11%20passing-success)
![Encoding: Skull/Crown](https://img.shields.io/badge/encoding-%F0%9F%92%80%F0%9F%91%91%20.skull-black)

An array machine whose source language is UTF-8 emoji, whose
intermediate representation is typed FLAT, and whose execution
target is **WebAssembly**. 🌈

🌀 **BQN** · 🔺 **APL** · ⚡ **K** · 🐺 **Wolfram subset** · 📐 **geometry**

The C11 compiler is a build-time tool. WASM is the machine.

---

## 🧊 Cold Boot — start here

This repository carries a **Cold Boot** front door:
[COLD-BOOT.md](COLD-BOOT.md) declares the owners, the license,
the verified baseline commit, the architecture, and the entry
points, sealed with a SHA-256 over its field block. 🔐

```sh
./cold-boot.sh        # or: make cold-boot
```

The pass re-verifies the tree by execution and fails closed:

```
GATE seal: PASS            GATE license_sha: PASS
GATE license_spdx: PASS    GATE baseline_ancestor: PASS
GATE identity: PASS        GATE build: PASS
GATE suite_* (×11): PASS   GATE artifact_traceback: PASS
GATE artifact_skull: PASS
COLD BOOT: PASS — every gate above was executed in this run.
```

An arriving agent reads the front door, runs one command, and
works only with what the pass proves. ✅

---

## 👀 The program

Emoji source in, typed results out:

```
# examples/demo.ram
🌀 📏 ➕ 3 1 2 3 4 5 6
🔺 🟦 🔄 2 3 1 2 3 4 5 6
⚡ 📏 🧮 4 1 2 3 4
🌀 🟦 🧲 2 3 2 1 2 3 4 5 6 7 8 9 10 11 12
```

```
BQN vector add       => rank=1 shape=[3]   values=[5,7,9]
APL matrix transpose => rank=2 shape=[3,2] values=[1,4,2,5,3,6]
K vector sum         => rank=0 shape=[]    values=[10]
BQN matrix matmul    => rank=2 shape=[2,2] values=[58,64,139,154]
```

### >>>>>>>€ — assertions in the source 🔍

```
🔺 📏 ➕ [1 2 3] [4 5 6]
>>>>>>>€ [5 7 9]
🔺 🔹 ⚖️ 3 3
>>>>>>>€ 👑
```

```
APL vector add     => values=[5,7,9]   assertion ok
APL scalar compare => values=[👑]      assertion ok
```

### 📐 Geometry is a first-class type

```
🌀 📐 GRID lattice 2 2 1 1
>>>>>>>€ SHAPE 2 2 1
⚡ @@ lattice ROTATE_Z 90
>>>>>>>€ POINT 1 0 1 0
↶ ⚡ @@ lattice ROTATE_Z 90
>>>>>>>€ POINT 1 1 0 0
```

The grid executes inside a WASM binary: transforms and POINT
assertions run in `run()` at runtime, and a tampered point makes
the module return −1. 🛡️

### 🕸️ The same source, executing in WebAssembly

```
🌀 📏 ➕ 3 1 2 3 4 5 6     →  .wasm loop over linear memory  →  [5, 7, 9]
🐺 🔹 📜 2 3 7             →  exported run(x:i64)->i64      →  5x + 7
```

---

## 🏗️ Architecture

```
🌈 Unicode Emoji Source
   BQN · APL · KCL · Wolfram · Array primitives
📐 Typed FLAT IR
   Geometry · Tensors · Inverses · Assertions
⚙️ Compiler / Lowering
   C11 frontend · WAT code generation · hand-assembled modules
🕸️ WebAssembly Stack Machine
   i32 · i64 · f32 · f64 · Linear memory
📦 WASM Binary (.wasm)
💀👑 Skull/Crown Encoding (.skull) — serialization, decoded before load
🚀 WASM Runtime — Node.js · Wasmtime-compatible targets
```

```
RAINBOW ARRAY MACHINE        (make report)
Architecture: WebAssembly Stack Machine
Source:       UTF-8 Unicode Emoji
Frontend:     BQN / APL / KCL / Wolfram subsets
IR:           Typed FLAT
Compiler:     C11 → WAT → WASM
Execution:    WASM Runtime
Memory:       WebAssembly Linear Memory
Binary:       .wasm
Encoding:     💀👑 .skull
Assertions:   >>>>>>>€
Inverse:      ↶
```

---

## 🧩 Components

| Piece | Role |
|---|---|
| `src/ram.c` | 🌐 The emoji interpreter — BQN/APL/K namespaces, the 🐺 Wolfram subset (✨ simplify · 📜 FLAT lowering · 🛡️ certificate) |
| `src/ramc.c` | 🏷️ Typed compiler IR — canonical SSA, bindings, shape/kind checks, assertions, inverses, geometry |
| `src/flat.c` | ▶️ FLAT v1 virtual machine — scalar + elementwise tensor execution, checked i64 (overflow refused) |
| `src/ram_wat.py` | 📝 WAT backend — vector ops and the affine function as exported modules |
| `src/ram_geometry.py` | 📐 Geometry compiler — GRID, the @@ bridge, Geometric FLAT |
| `src/ram_runtime.py` | 🕸️ Hand-assembled WASM — geometry transforms + assertions execute in `run()` |
| `src/ram_array_runtime.py` | 🕸️ Hand-assembled WASM — vector ops loop in-module; affine `run(x)` exported |
| `src/ram_array_runtime.c` | 🛠️ The C11 twin of the array runtime — byte-identical artifacts, no Python (`make test-array-c`) |
| `src/ram_traceback.py` | 🔄 Generative traceback — artifact/source byte equality + generated Lean proof obligations |
| `src/skull.c` | 💀👑 Skull & Crown — `encode` · `decode` · `invert` |
| `proofs/` | 📜 Generated Lean obligations + hash evidence records |
| `cold-boot.sh` | 🧊 The Cold Boot scoping pass |

---

## 💀👑 Skull & Crown

A lossless Unicode encoding of binary data — the machine's
canonical alphabet for binaries, weights, and fixtures:

- 💀 = bit `0` · 👑 = bit `1` · MSB first · byte-aligned
- The WASM magic `00 61 73 6D` encodes as four 8-symbol lines
- Two independent implementations (C and Python) decode each
  other's output byte-for-byte 🔁
- `.skull` is **serialization**: it is decoded before a standard
  runtime loads the module — never an execution layer

---

## 🔁 Reversible algebra

- **↶** registered inverses — add, multiply, transpose, reverse
  are exact; flatten/reshape inverses require saved shape
  metadata and compilation fails without it
- **⟲** one-line round trips, verified in place
- **🛡️ ROUNDTRIP** certificates —
  `status=VERIFIED_ON_TESTED_INPUTS`, scoped to the tested inputs
- **⚖️** compare yields a verdict: 👑 or 💀

---

## 🔄 Traceback — metaprogramming inversion

Given a source and a `.wasm`, the traceback recompiles the
source deterministically and demands **byte equality** with the
artifact — altered binaries and wrong sources are rejected.
It then *generates* a Lean proof obligation for the pair (the
affine identity over ℤ; the pointwise array map over ℤ) with a
SHA-256 evidence record. The obligations state the algebra;
artifact consistency is what the byte check proves. 📜

---

## ✅ Verification

Eleven suites, all green, all by execution:

```sh
make test  test-ir  test-flat  test-skull  test-inv  test-geom
make wat-test  geometry-test  runtime-test  array-runtime-test
make traceback-test
make cold-boot   # the whole tree, sealed, in one pass 🧊
```

Per-version scope and verification records live in
[STATUS.md](STATUS.md) — proven and claimed are kept separate
there, and here. 🧾

---

## 🛣️ Open frontier

Stated plainly, in the present tense:

- The WASM vector subset is fixed-length **f64**; the symbolic
  subset is affine **i64** with declared modulo-2⁶⁴ arithmetic
- The geometry and array runtime compilers are separate
  lowerings, not yet one unified typed-FLAT → WASM pipeline
- Persistent tensor buffers 🧊, ternary arithmetic 🔱,
  Church-3 reduction, Unlambda 🐦, and the flat decoder are
  designed and listed — not yet implemented
- The Wolfram layer is a compatible subset; no Wolfram Engine
  is involved

---

## 🏷️ Topics

```
rainbow-array-machine, webassembly, wasm, emoji-programming,
array-programming, bqn, apl, k, wolfram, compiler, typed-ir,
flat-ir, skull-crown, cold-boot, provenance, reversible-computing,
lean, mathlib, c11
```

---

## ⚖️ License

**SNAPKITTY OPAQUE SOURCE LICENSE v1.0** —
`LicenseRef-Sovereign-Leviathan-SL-AGPL3-001` © 2026 SnapKitty West.
Owners: **Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty**.

The full text is [LICENSE](LICENSE) at the root — the only
license in this tree. Publication, access, download, visibility,
or possession is **not** commercial authorization; commercial
use requires separate written authorization from the owners. 📜
