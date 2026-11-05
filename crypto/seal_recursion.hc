U0 SealRecursion() {
  I64 start = 2070;     // ABJAD_GHAYB sum of أحمد
  I64 step  = 10;       // decrement per iteration
  I64 cols  = 12;       // 12-bit binary representation (2070 fits in 12 bits)
  I64 n, r, c, bit, maxRows;
  I64 transitionRow = -1;
  I64 transitionCol = -1;

  Print("/SEAL:2026.06.07{ITERATE:INVERTED,ABJAD:2070,STEP:-10,UNTIL:0}\n\n");
  Print("   "); for (c=0; c<cols; c++) Print("%d", 11-c); Print("\n"); // bit position labels


  // Compute rows: start / step = 207 iterations + 1 for 0
  maxRows = (start / step) + 1;

  // Print the FULL recursion grid — each row is one iteration
  for (r = 0; r < maxRows; r++) {
    n = start - (r * step);

    // Print row number (right-aligned)
    Print("%3d: ", r);

    // Print each bit as 👑 (1) or 💀 (0)
    for (c = 0; c < cols; c++) {
      bit = (n >> (cols-1-c)) & 1;

      // Detect transition: first time we see a 💀 after 👑 in a column
      if (bit == 0 && transitionCol == -1 && r > 0) {
        // Check if previous row had a 1 in this column
        I64 prevBit = ((start - ((r-1)*step)) >> (cols-1-c)) & 1;
        if (prevBit == 1) {
          transitionCol = c;
          transitionRow = r;
        }
      }

      if (bit) Print("👑");
      else      Print("💀");
    }
    Print("\n");
  }

  Print("\n");

  // Print the transition analysis
  if (transitionRow >= 0) {
    Print("INVERSION CARRY @ row %d, col %d (bit %d)\n", transitionRow, transitionCol, cols-1-transitionCol);
    I64 before = start - (transitionRow-1)*step;
    I64 after  = start - transitionRow*step;
    Print("  before: %d  (%012b)\n", before, before);
    Print("  after:  %d  (%012b)\n", after, after);
    Print("  The borrow propagated through %d bit positions.\n", transitionCol);
    Print("  This is the recursion trap: subtraction inverts\n");
    Print("  the binary representation until the carry resolves.\n");
  } else {
    Print("No inversion carry detected — the recursion is linear.\n");
  }

  Print("\nTERMINUS: %d iterations, final value 0\n", maxRows-1);
  Print("ABJAD SUM - %d×%d = 0\n", maxRows-1, step);
}

SealRecursion;
