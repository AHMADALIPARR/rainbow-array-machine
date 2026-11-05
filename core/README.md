# Rainbow WASM — standalone Unicode-to-WebAssembly compiler (bounded subset)

This repository replaces the earlier Python pseudo-compiler prototypes. It compiles each supported UTF-8 emoji program into **its own WebAssembly module**; the compiled module runs its operations and assertions in WASM, not in a JavaScript instruction dispatcher.

## Build and execute

Requirements: POSIX `make`, C11 compiler, Clang with `wasm32` support, Node.js.

```sh
make clean && make test
node src/run_module.js build/array.wasm
```

Pipeline:

```
examples/*.ram (UTF-8 emoji)
  → build/ramc (C11 source validator)
  → RAMIR1 normalized operations
  → build/ram_codegen (C11 standalone module generator)
  → Clang wasm32 (kernel.c + generated module source)
  → build/*.wasm (each independent executable module)
  → WebAssembly.instantiate + exported run() executes & checks assertions
  → skull.js encode/decode provides reversible 💀 0 / 👑 1 serialization
```

### Exact implemented subset

- `🌀`, `🔺`, `⚡`: permitted source prefixes; **not full BQN/APL/K parsers**.
- `📏 ➕ ➖ ✖️ ➗`: f64 elementwise vector operations.
- `🟦 🧲`: dense f64 matrix multiplication; `🟦 🔄`: transpose.
- `📐 @@ ROTATE_Z`: quarter-turn about Z; `📐 @@ TRANSLATE`: 3D translation.
- `↶` preceding a geometry source command: exact inverse of a quarter-turn/translation for the declared inputs.
- `🐺 🔹 📜`: numeric `(a+b)*x+c`, **not Wolfram Engine or symbolic algebra**.
- `>>>>>>>€`: fixed-tolerance numeric assertion **executed by exported WASM `run()`**; mismatches return nonzero.
- `.skull`: non-executable, byte-exact reversible UTF-8 binary representation of `.wasm`.

This version accepts constants in the program and compiles them into a module; it has no external runtime input ABI. `run()` is called by a Node.js harness to check returned status and read results from exported linear memory. The JavaScript harness does not interpret instructions.

### Tests actually run

`make test` compiles three independent WASM modules, instantiates and runs them, verifies numeric assertions, checks exact WASM encoding/decoding, rejects invalid source/inverse/Unicode binaries, and **checks that a deliberately false assertion fails from inside the WASM execution**.

### Not implemented

General BQN/APL/KCL/Wolfram parsing or semantics, unified typed FLAT, dynamic shape inference, runtime operator graph, WAT emission, arbitrary symbolic inversion, WebAssembly decompilation, formal WASM equivalence, and Mathlib-checked theorems. No proof claims are made.

## New experimental blocks: FLAT1 and Mathlib

`make flat-test` validates RAMIR1 operations and emits `build/*.flat` with explicit f64 result ranks, shapes, element counts and assertion flags. **FLAT1 is currently a validated metadata projection, not the single input to code generation.** The existing WASM generator still consumes RAMIR1; unification remains unimplemented.

`proofs/Arithmetic.lean` imports only `Mathlib` and states integer arithmetic and index lemmas. Lean is unavailable in this environment, so these files have **not** been kernel-checked here. These lemmas do **not** establish equivalence between generated WASM instructions and the mathematical specification. That requires an independent WASM semantics model, instruction decoder and a verified refinement proof.

The 🌀/🔺/⚡/🐺 namespace tokens still do **not** implement full BQN/APL/KCL/Wolfram semantics. Those language frontends are not present and must not be described as implemented.
