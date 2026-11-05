import Mathlib

namespace Rainbow

-- Mathematical specification of the affine kernel, not a WASM machine proof.
def affineSpec (a b c x : ℤ) : ℤ := (a + b) * x + c

theorem affine_expand (a b c x : ℤ) :
    affineSpec a b c x = a * x + b * x + c := by
  simp [affineSpec, add_mul, add_assoc]

theorem inverse_translation (x delta : ℤ) :
    (x + delta) - delta = x := by
  omega

-- Row-major transpose: (i,j) in m×n maps to (j,i) in n×m.
-- The output linear offset is j*m+i; decoding must recover both axes.
theorem transpose_index_roundtrip (m n i j : ℕ)
    (hi : i < m) (hj : j < n) :
    (j * m + i) / m = j ∧ (j * m + i) % m = i := by
  have hm : 0 < m := Nat.zero_lt_of_lt hi
  constructor
  · calc
      (j * m + i) / m = (i + j * m) / m := by rw [Nat.add_comm]
      _ = i / m + j := by rw [Nat.add_mul_div_right i j hm]
      _ = j := by simp [Nat.div_eq_of_lt hi]
  · calc
      (j * m + i) % m = (i + j * m) % m := by rw [Nat.add_comm]
      _ = i % m := by rw [Nat.add_mul_mod_self_right]
      _ = i := Nat.mod_eq_of_lt hi

end Rainbow
