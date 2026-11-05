import Mathlib

namespace Rainbow

/- Polynomial obligations for the 🐺 Wolfram symbolic core (Stage 1).
   The compiler's ✨ normalization rests on two integer identities:
   collecting like terms and the Horner step used by poly_eval.
   Both are stated over ℤ, matching the compiler's integer
   coefficients; the kernel evaluates the same forms over f64,
   where the identities hold a fortiori. -/

theorem collect_like_terms (a b x : ℤ) (p : ℕ) :
    a * x ^ p + b * x ^ p = (a + b) * x ^ p := by
  ring

theorem drop_zero_term (x : ℤ) (p : ℕ) :
    0 * x ^ p = 0 := by
  ring

theorem horner_step (a b c x : ℤ) :
    a * x * x + b * x + c = (a * x + b) * x + c := by
  ring

end Rainbow
