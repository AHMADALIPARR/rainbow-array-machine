/* CompCert build shim (build tooling, not stack source).
   The system <math.h> declares long double functions, which
   CompCert's parser rejects as an unsupported feature. The
   stack's C sources use only isfinite, round and exp, all in
   double precision. This header declares exactly those so the
   unmodified sources compile under ccomp; the implementations
   remain libm's, the same library the gcc build links. */
#ifndef RAM_COMPCERT_MATH_SHIM_H
#define RAM_COMPCERT_MATH_SHIM_H
double round(double x);
double exp(double x);
static int ram_isfinite(double x){union{double d;unsigned long long u;}v;v.d=x;return ((v.u>>52)&0x7ffULL)!=0x7ffULL;}
#define isfinite(x) ram_isfinite(x)
#endif
