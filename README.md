# 🌈 Rainbow Array Machine (RAM)

**An executable UTF-8 emoji instruction language for array computation.**

This is an experimental, deterministic **C11 interpreter**, not yet a full compiler, neural-network runtime, or implementation of BQN/APL/K language semantics. Namespaces distinguish intended language origins; v0.1 supports only a common explicitly defined subset for BQN, APL and K.

## Run

```sh
make
make test
./ram --dictionary
./ram examples/demo.ram
```

No Python, external packages, or runtime dependencies beyond a C11 compiler and C standard library are required.

## Grammar v0.1

One instruction per line, whitespace-separated UTF-8 emoji tokens. `#` begins a comment.

- Binary elementwise: `LANG TYPE OP [dimensions] A-values B-values`
- Unary: `LANG TYPE OP [dimensions] A-values`
- Matrix multiplication: `LANG 🟦 🧲 M K N A-values B-values`

Dimensions: scalar = none, vector = `N`, matrix = `ROWS COLS`. Values are row-major, finite decimal numbers. Max 4096 elements per array. Each instruction is independent; results are printed, not bound to variables. No broadcasting, rank polymorphism, arbitrary tensors, or automatic differentiation yet.

### Working operations

| Emoji | Name | Semantics |
|---|---|---|
| ➕ | add | elementwise sum of same-shaped operands |
| ➖ | subtract | elementwise subtraction |
| ✖️ | multiply | elementwise multiplication |
| 🧲 | matmul | matrix multiplication `(M×K)·(K×N)` |
| 🔄 | transpose | transpose rank-2 matrix |
| 📦 | flatten | row-major flatten to vector |
| 📐 | shape | vector of array dimensions (scalar gives `[1]` in v0.1) |
| 🧮 | sum | full reduction to scalar |

### Registered language namespaces

`🌀 BQN`, `🔺 APL`, `⚡ K`, `🧩 J`, `🪐 Uiua`, `🏛️ Pascal`, `⚙️ C`, `🦀 Rust`, `🧱 Forth`, `🔌 SystemVerilog`, `🐦 Unlambda`, `λ Haskell`, `🕸️ Prolog`.

🌀/🔺/⚡ execute the original array subset. 🐺 Wolfram executes a **restricted, independently implemented symbolic polynomial subset**. Other namespaces fail closed.

### Registered types

`🔹 scalar`, `📏 vector`, `🟦 matrix` execute. `🧊 tensor` and `🔱 ternary` are reserved, and fail closed until their semantics and storage are implemented.

## Sample

```text
🌀 📏 ➕ 3 1 2 3 4 5 6
🔺 🟦 🔄 2 3 1 2 3 4 5 6
🌀 🟦 🧲 2 3 2 1 2 3 4 5 6 7 8 9 10 11 12
```

## Design constraints

1. Emoji tokens are UTF-8 strings; never index them by bytes as if they were single characters. The lexer matches whitespace-delimited complete emoji sequences. Variation selectors matter.
2. The namespace/type/operator triplet is part of the instruction's identity.
3. No equivalence is assumed between distinct source languages' native semantics.
4. Invalid programs and unsupported instructions return nonzero status, never simulated results.
5. Shape and buffer size are checked before allocation.
6. Future Church numeral and Unlambda combinator semantics must be normalized into a typed IR before optimized lowering; they do not imply an allocator or runtime performance advantage by themselves.

## Roadmap

- v0.2: variable bindings, explicit rank/shape metadata, typed AST, canonical opcodes, and source positions.
- v0.3: true source-language adapters for BQN/APL/K with reference comparisons and explicit disagreement cases.
- v0.4: arena allocation, views, ownership, tensor and ternary storage, contraction and fusion.
- v0.5: Church-3 and Unlambda combinator reduction with bounded evaluation and property tests.
- v0.6: flat decoder reference, reproducible numeric tests, and native C backend.

The cassette artwork is a proposed project identity, not embedded in this source distribution.

## v0.2 Wolfram-inspired symbolic adapter and FLAT lowering

`🐺` is the Wolfram namespace. This build does **not** bundle, call, or claim to implement Wolfram Engine, Mathematica, WSTP, or LibraryLink. No license or proprietary dependency is required. It implements a deliberately small compatible mathematical pattern: `a*x + b*x + c` with bounded signed integer coefficients.

```text
🐺 🔹 ✨ 2 3 7    # collect coefficients: (5*x)+7
🐺 🔹 📜 2 3 7    # emit typed FLAT v1 SSA-like textual IR
🐺 🔹 🛡️ 2 3 7    # verify coefficient identity algebraically
```

`✨` is symbolic simplification by collecting like terms, `📜` emits a flat scalar IR, and `🛡️` emits a narrowly scoped algebraic certificate. The certificate establishes the polynomial identity over mathematical integers; it is **not** a formal proof of overflow-free i64 execution. No arbitrary Wolfram expressions, tensors, integration, or actual Wolfram library integration are supported.

A future optional WSTP adapter could use a separately installed, appropriately licensed Wolfram Engine and compare outputs with this independent reference implementation. That adapter is not part of this release.

## v0.3: UTF-8 emoji to WAT backend

Run `make wat-test` to emit and inspect `examples/wat/vector.wat` and
`examples/wat/symbolic.wat`. This is **real WAT source**, not yet a `.wasm`
binary. Compile the WAT with an external WebAssembly text assembler such as
`wat2wasm` (WABT). That tool is not bundled.

Supported v0.3 lowering: exactly one statement per module; BQN/APL/K vector
addition, subtraction, or elementwise multiplication with embedded f64 inputs,
exported `run()` and exported linear `memory` containing the resulting vector;
Wolfram-compatible `🐺 🔹 📜 a b c` symbolic affine expression exports
`run(x:i64)->i64` for `(a+b)*x+c`. The latter uses **wrapping i64 WebAssembly
arithmetic**, so its runtime behavior differs from unbounded mathematical
integers on overflow. It does not invoke Wolfram Engine. Other instructions
are rejected by the WAT compiler. Source is Unicode UTF-8, with emoji tokens
separated by ASCII whitespace; no reliance on grapheme-length or emoji bytes.

For the vector example, output values are stored as little-endian f64 at
byte offsets 0, 8, and 16. There is no dynamic allocation in the generated
module. The original interpreter remains available as `./ram`.

## v0.4: Geometry FLAT IR and KCL-inspired bridge

The experimental geometry compiler is `src/ram_geometry.py`. It consumes UTF-8
`.ram` plaintext and emits `.wat` and `.flat.json` sidecar metadata. This is
RAM-specific syntax, **not** native BQN or KCL source. `⚡ @@` is a proposed
bridge notation; it does not assert that `@@` is a native KCL operator.

```text
🌀 📐 GRID lattice 2 2 1 1
>>>>>>>€ SHAPE 2 2 1
⚡ @@ lattice ROTATE_Z 90
>>>>>>>€ POINT 1 0 1 0
↶ ⚡ @@ lattice ROTATE_Z 90
>>>>>>>€ POINT 1 1 0 0
```

The supported subset is integer grid generation, 90-degree Z-axis rotations,
integer translations, their exact inverses, shape and indexed-point assertions.
The generated WAT exports linear memory initialized with packed little-endian
`i32` XYZ coordinates, plus `run` returning the byte length. Coordinates are
precomputed by the compiler; **this is not yet a runtime transform kernel**.
The sidecar records shape, byte offsets, point count and dtype. Assertions are
checked at compile time against the finite sample, not universally proven.

Run `make geometry-test` or
`python3 src/ram_geometry.py examples/geometry/grid.ram -o examples/geometry/grid.wat`.
The WAT is not assembled into `.wasm` without an external WAT assembler.

## v0.5 Runtime WASM + 💀👑 binary

`src/ram_runtime.py` compiles the geometry subset into **actual WebAssembly binary**, a readable `.wat` listing, a `.flat.json` manifest, and a reversible `.skull` UTF-8 representation. Unlike v0.4, transforms execute when the exported WASM `run()` function is called, and point assertions are evaluated by the WASM function. The compiler also checks the same assertions during source validation.

```sh
make runtime-test
python3 src/ram_runtime.py examples/geometry/grid.ram -o examples/geometry/grid-runtime.wasm
node tests/runtime.js examples/geometry/grid-runtime.wasm
python3 src/ram_runtime.py --decode examples/geometry/grid-runtime.skull -o /tmp/roundtrip.wasm
cmp examples/geometry/grid-runtime.wasm /tmp/roundtrip.wasm
```

`💀` means bit 0, `👑` means bit 1, most-significant bit first. Whitespace is ignored on decoding; every eight emojis represent one byte. This is lossless encoding, not compression or encryption. The `.wat` is a textual rendering of the generated instruction sequence, not an independently assembled build input. `@@` is RAM-specific syntax inspired by KCL, not a claim of native KCL support. Current limitations: one grid, integer coordinates, exact quarter-turn Z rotations, integer translation, point/shape assertions; no arbitrary geometry, floating point, or Wolfram engine integration.

## v0.6: Executable array and affine WASM subsets

`make array-runtime-test` compiles BQN/APL/K-qualified f64 vector add/subtract/multiply and the standalone Wolfram-compatible affine i64 subset into **real WebAssembly binaries**. Node.js instantiates the modules and verifies runtime outputs. The vector compiler places input operands in linear memory and performs arithmetic inside a runtime WASM loop. It also writes matching WAT, a FLAT-layout JSON manifest, and a reversible 💀👑 `.skull` serialization.

This is **not** a complete lowering of all array, geometry, or Wolfram operations: supported vector operations are fixed-length f64, the symbolic subset is affine i64 with wraparound arithmetic, and the two runtime compilers are not yet unified into a general typed FLAT lowering pipeline. No licensed Wolfram Engine is invoked. Unsupported operations are rejected.
