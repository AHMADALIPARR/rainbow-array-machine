import Mathlib

/-! Generated algebraic obligation for SHA256 f473f8ed9b7f403236b43bd6a6378a9b9c8e7f3b823523e70fe5a54d6102d40d.
    This is an integer identity, NOT a proof that the WASM module implements it.
    WASM i64 uses modular arithmetic; no overflow-free claim is made. -/
example (x : ℤ) : (2 : ℤ) * x + 3 * x + 7 = (5 : ℤ) * x + 7 := by
  ring
