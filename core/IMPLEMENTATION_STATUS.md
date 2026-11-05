# Verified implementation status

This repository implements a **restricted emoji array language**, not full BQN, APL, KCL, or Wolfram Language. Those languages have substantial semantics not represented here. Namespaces are tags, not complete frontends.

## Executable typed lowering

The production build now passes through `RAMIR1 -> FLAT1 validation -> FLAT2 typed payload -> C source -> clang wasm32 -> .wasm`. FLAT2 retains validated operation metadata alongside the exact executable operand payload; it is not yet an SSA IR, does not implement buffer allocation, and does not independently prove type soundness. Every generated module is executed by Node's WebAssembly runtime during `make test`.

## Formal verification boundary

`proofs/Arithmetic.lean` uses only `import Mathlib` and specifies mathematical identities. No Lean executable or Mathlib installation is present in this environment, so no Lean proof-checking result is claimed. The identities do not establish instruction-level WASM equivalence. A complete WASM equivalence proof requires a formal semantics of the emitted WASM subset, a relation between FLAT values and WASM memory, and a machine-checked simulation theorem for each kernel and code-generation step.

## Missing

Complete BQN/APL/KCL/Wolfram semantics, an integrated Wolfram Engine, KCL geometry parsing, full typed FLAT with memory planning, and Mathlib-verified WASM equivalence remain unimplemented. Do not report them as complete.
