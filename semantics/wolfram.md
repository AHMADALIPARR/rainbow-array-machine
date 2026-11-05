# 🐺 Wolfram — the symbolic core (Stage 1)

The 🐺 namespace's core semantics, generalized from the v0.1
subset (`src/ram.c`: `🐺 🔹 OP a b c` over a·x + b·x + c) into
the WASM pipeline (`core/`). Three tokens, three jobs:

| Token | Job | Where it happens |
|---|---|---|
| ✨ | Simplify: normalize a polynomial at compile time | `core/src/ramc.c` |
| 📜 | Lower: emit the executable form | AFFINE (existing) and POLY via FLAT2 → `poly_eval` Horner kernel |
| 🛡️ | Certify: a certificate records the normalization | `<program>.ir.cert` sidecar written by ramc |

## The ✨ statement

`🐺 🔹 ✨ x (coeff power)* [>>>>>>>€ expected]`

Terms are (integer coefficient, power) pairs — coefficients in
[−1000000, 1000000], powers in [0, 32], at most 64 terms.
Normalization rules, applied at compile time in this order:

1. **collect_like_terms** — coefficients of equal powers sum.
2. **drop_zero_terms** — zero coefficients vanish.
3. **canonical_ascending** — the canonical form is the dense
   coefficient list c0..cD, D the highest nonzero power.

## Worked example (executed, in the test suite)

`(2,1)+(3,1)+(5,0)+(0,3)+(−1,2)` at x=2 canonicalizes to
deg 2, coeffs `5 5 −1` (the polynomial −x² + 5x + 5), evaluates
to **11**, and the certificate records the full derivation:

```
CERT v1: line 2; rules=collect_like_terms,drop_zero_terms,canonical_ascending; terms=(2,1)+(3,1)+(5,0)+(0,3)+(-1,2); canonical deg=2 coeffs=5 5 -1; status=NORMALIZED
```

## Proof obligations (`core/proofs/Polynomial.lean`, lake-verified)

- `collect_like_terms`: a·x^p + b·x^p = (a+b)·x^p over ℤ
- `drop_zero_term`: 0·x^p = 0
- `horner_step`: a·x·x + b·x + c = (a·x + b)·x + c — the
  evaluation order the kernel's `poly_eval` implements

Scope: the theorems state the mathematics of normalization and
evaluation over ℤ; they do not prove WASM equivalence. The
literature-facing Wolfram layer (entities, interpretation,
emoji DSL) is the companion package
`ScientificLiteratureSemantics.wl` in this directory.
