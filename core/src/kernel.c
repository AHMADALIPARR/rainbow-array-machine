#include <stdint.h>
// Runtime operations on f64 arrays in WebAssembly linear memory.
__attribute__((export_name("elementwise")))
int elementwise(const double*a,const double*b,double*out,int n,int op){
 if(n<0||n>4096||op<0||op>3)return -1;
 for(int i=0;i<n;i++){double x=a[i],y=b[i];if(op==3&&y==0)return -2;out[i]=op==0?x+y:op==1?x-y:op==2?x*y:x/y;}return 0;
}
__attribute__((export_name("matmul")))
int matmul(const double*a,const double*b,double*out,int m,int k,int n){
 if(m<1||k<1||n<1||m>64||k>64||n>64)return -1;
 for(int i=0;i<m;i++)for(int j=0;j<n;j++){double s=0;for(int t=0;t<k;t++)s+=a[i*k+t]*b[t*n+j];out[i*n+j]=s;}return 0;
}
__attribute__((export_name("transpose")))
int transpose(const double*a,double*out,int m,int n){if(m<1||n<1||m>64||n>64)return -1;for(int i=0;i<m;i++)for(int j=0;j<n;j++)out[j*m+i]=a[i*n+j];return 0;}
__attribute__((export_name("reduce_sum")))
double reduce_sum(const double*a,int n){double s=0;for(int i=0;i<n;i++)s+=a[i];return s;}
__attribute__((export_name("affine")))
double affine(double a,double b,double c,double x){return (a+b)*x+c;}
__attribute__((export_name("rotate_z_quarter")))
int rotate_z_quarter(const double*in,double*out,int n,int quarter){if(n<0||n>1024)return -1;int q=((quarter%4)+4)%4;for(int i=0;i<n;i++){double x=in[i*3],y=in[i*3+1];out[i*3]=q==0?x:q==1?-y:q==2?-x:y;out[i*3+1]=q==0?y:q==1?x:q==2?-y:-x;out[i*3+2]=in[i*3+2];}return 0;}
__attribute__((export_name("translate")))
int translate(const double*in,double*out,int n,double dx,double dy,double dz){if(n<0||n>1024)return -1;for(int i=0;i<n;i++){out[i*3]=in[i*3]+dx;out[i*3+1]=in[i*3+1]+dy;out[i*3+2]=in[i*3+2]+dz;}return 0;}
__attribute__((export_name("assert_equal")))
int assert_equal(const double*a,const double*b,int n,double tolerance){if(n<0||n>4096||tolerance<0)return -1;for(int i=0;i<n;i++){double d=a[i]-b[i];if(d<0)d=-d;if(!(d<=tolerance))return i+1;}return 0;}
