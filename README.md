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

Only 🌀/🔺/⚡ execute in v0.1. Other namespaces are registered but fail closed.

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
