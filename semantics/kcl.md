# ⚡ KCL — the K semantic layer

KCL is the stack's K dialect. Its semantics are not invented
here: they are lifted from the binary layer up. The ground of
KCL meaning is the binary contract — a value is what its bytes
are — and the normative source is the KSB+ engine's Layer 0,
carried verbatim in this directory as `kcl-l00.k` (SHA-256
a90111a1d55db8df86fc831b13cbcbc5811667a4bb554bd3d3fcd9f996b76cc2,
byte-identical to `ksb-plus/src/l00.k` at the verified baseline
149a788; the full KSB+ suite passes 16/16 layers under Kona,
and t00 passes against this copy).

## The binary ground (Layer 0, normative)

| Function | Semantic rule it fixes |
|---|---|
| `bValid` | A byte string is a list of ints, each in [0, 255]; type is list or int atom |
| `bMod` | Modulo for non-negative a, positive m (Kona `%` is integer division) |
| `bEncU8/16/32`, `bDecU8/16/32` | Fixed-width unsigned integers, big-endian, exact-length decode |
| `bEncI8/16/32`, `bDecI8/16/32` | Signed integers are two's complement over the unsigned width |
| `bEncU64`, `bDecU64` | u64 is a (hi; lo) pair of u32 — there is no 64-bit atom |
| `bEncStr`, `bStrLen`, `bDecStr` | Byte strings are u32be length-prefixed |
| `bCksum` | Byte strings carry a checksum |
| `bFrame`, `bFrameLen`, `bUnframe` | Framing: a frame has a computable length and an exact inverse |
| `bCont` | Continuation marker for multi-frame values |
| `butf8ok` | A byte string is (or is not) valid UTF-8 — checked, never assumed |

## The semantic rules lifted from the binary layer

1. **Bytes first.** Every KCL value denotes a byte string under
   the Layer 0 encodings. Two values are equal when their
   encodings are equal.
2. **Width is meaning.** An integer has no meaning without its
   width: u8/u16/u32/u64-pair and i8/i16/i32 are distinct types,
   and decoding demands exactly the encoded length.
3. **Signedness is representation.** Negative numbers exist only
   as two's complement over a declared width; there is no
   width-less negative integer at the binary layer.
4. **Order is big-endian.** Multi-byte encodings read most
   significant byte first, everywhere, without exception.
5. **Strings are counted, not terminated.** Length prefixes
   (u32be) delimit strings and frames; no sentinel bytes.
6. **Frames invert or fail.** `bUnframe` is the exact inverse of
   `bFrame` on valid frames; invalid bytes are rejected by
   `bValid`/`butf8ok`, never silently coerced.
7. **Evaluation is K's.** Above the binary ground, KCL inherits
   K's evaluation: right-to-left, no operator precedence, verbs
   with fixed valence, adverbs modifying verbs — the same
   dialect the upper KSB+ layers (l01–l15: pages, indexes,
   transactions, WAL, SQL) are written in, and the dialect the
   ⚡ namespace denotes in `ram.c` / `ramc`.

## Where KCL sits in this stack

- ⚡ is namespace 2 in the v0.1 registry (`src/ram.c`) and a
  first-class namespace in the typed IR (`src/ramc.c`).
- The layers above the binary ground already exist in the
  KSB+ engine (storage, pages, columnar format, indexes,
  transactions, WAL, matrix ops, SQL — `ksb-plus/docs/`); KCL
  programs that need them compose those layers rather than
  re-deriving them.
- Open: the core WASM pipeline (`core/`) does not yet lower ⚡
  statements; when it does, the lowering target for KCL data is
  the Layer 0 byte representation above.
