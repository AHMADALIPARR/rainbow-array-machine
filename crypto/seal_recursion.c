/* SealRecursion — C11 port of the owner's HolyC original
   (crypto/seal_recursion.hc). Print -> printf; %012b rendered
   manually; the logic is unchanged. This is the form that
   enters the CompCert lowering pipeline. */
#include <stdio.h>
typedef long I64;
static void printb12(I64 v){ for(int i=11;i>=0;i--) putchar(((v>>i)&1)?'1':'0'); }
int main(void) {
  I64 start = 2070, step = 10, cols = 12;
  I64 n, r, c, bit, maxRows;
  I64 transitionRow = -1, transitionCol = -1;
  printf("/SEAL:2026.06.07{ITERATE:INVERTED,ABJAD:2070,STEP:-10,UNTIL:0}\n\n");
  printf("   "); for (c=0; c<cols; c++) printf("%lld", (long long)(11-c)); printf("\n");
  maxRows = (start / step) + 1;
  for (r = 0; r < maxRows; r++) {
    n = start - (r * step);
    printf("%3lld: ", (long long)r);
    for (c = 0; c < cols; c++) {
      bit = (n >> (cols-1-c)) & 1;
      if (bit == 0 && transitionCol == -1 && r > 0) {
        I64 prevBit = ((start - ((r-1)*step)) >> (cols-1-c)) & 1;
        if (prevBit == 1) { transitionCol = c; transitionRow = r; }
      }
      if (bit) printf("👑"); else printf("💀");
    }
    printf("\n");
  }
  printf("\n");
  if (transitionRow >= 0) {
    printf("INVERSION CARRY @ row %lld, col %lld (bit %lld)\n", (long long)transitionRow, (long long)transitionCol, (long long)(cols-1-transitionCol));
    I64 before = start - (transitionRow-1)*step;
    I64 after  = start - transitionRow*step;
    printf("  before: %lld  (", (long long)before); printb12(before); printf(")\n");
    printf("  after:  %lld  (", (long long)after); printb12(after); printf(")\n");
    printf("  The borrow propagated through %lld bit positions.\n", (long long)transitionCol);
    printf("  This is the recursion trap: subtraction inverts\n");
    printf("  the binary representation until the carry resolves.\n");
  } else {
    printf("No inversion carry detected — the recursion is linear.\n");
  }
  printf("\nTERMINUS: %lld iterations, final value 0\n", (long long)(maxRows-1));
  printf("ABJAD SUM - %lld×%lld = 0\n", (long long)(maxRows-1), (long long)step);
  return 0;
}
