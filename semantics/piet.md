# 🎨 Piet — the painting layer (Turing-complete)

Piet is the stack's esolang layer: a program is a painting.
Execution is fully Turing-complete — Piet has a stack, I/O,
arithmetic, and control flow driven by color geometry.

## The machine (Piet semantics, as specified by its author)

- A program is a grid of **color blocks** (connected regions of
  one color). The Direction Pointer (DP) moves between blocks;
  the Codel Chooser (CC) breaks ties at irregular edges.
- There are **18 colors**: 6 hues (red, yellow, green, cyan,
  blue, magenta) × 3 lightnesses (light, normal, dark), plus
  **white** (free movement, no command) and **black** (walls /
  rotation triggers).
- Entering a block **pushes its area** (number of codels) onto
  the stack — block size is how literals are written.
- The command executed on a block transition is determined by
  the **change in hue** (0–5 steps) and **change in lightness**
  (0–2 steps) between the exited and entered colors:

| Δhue \ Δlight | 0 | 1 | 2 |
|---|---|---|---|
| 0 | push | pop | — |
| 1 | add | subtract | multiply |
| 2 | divide | mod | not |
| 3 | greater | pointer | switch |
| 4 | duplicate | roll | in(number) |
| 5 | in(char) | out(number) | out(char) |

- Blocked movement rotates the CC, then the DP (up to 8
  attempts, then the program ends).

## The Genmoji mapping (this stack)

**Each painting block is a Genmoji.** A program is a painting
whose blocks are generated emoji glyphs: the glyph grid is the
artwork, the 18-color Piet palette is its color discipline, and
the executor reads color transitions — not glyph identities —
so the same program can be re-skinned with different emoji
while its meaning (the transition sequence) is preserved.
Glyph identity is free surface; color geometry is the code.

Status: semantics fixed here; an executor is future work — the
natural home is the canvas layer (the saver VM's grid is
already a painting surface), with programs carried as data,
never as page code.
