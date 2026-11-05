#include <stdint.h>
extern int elementwise(const double*,const double*,double*,int,int);
extern int matmul(const double*,const double*,double*,int,int,int);
extern int transpose(const double*,double*,int,int);
extern double affine(double,double,double,double);
extern int rotate_z_quarter(const double*,double*,int,int);
extern int translate(const double*,double*,int,double,double,double);
extern int assert_equal(const double*,const double*,int,double);
static double output[65536];
__attribute__((export_name("result_ptr"))) int result_ptr(void){return (int)(uintptr_t)output;}
static const double a0[3]={1,2,3};
static const double b0[3]={4,5,6};
static const double expected0[3]={5,7,9};
static const double a1[6]={1,2,3,4,5,6};
static const double b1[6]={7,8,9,10,11,12};
static const double expected1[4]={58,64,139,154};
static const double a2[6]={1,0,0,0,1,0};
static const double expected2[6]={0,1,0,-1,0,0};
static const double a3[3]={0,1,0};
static const double expected3[3]={1,0,0};
