// flat — the FLAT v1 executor: makes the symbolic layer's emitted
// IR an executable representation instead of a textual one, and
// connects it to tensor operations.
//
// FLAT v1 grammar (exactly what the 🐺 📜 lowering in ram.c emits):
//   FLAT v1: input x:i64; t0=mul_i64(x,5); t1=add_i64(t0,7); return t1
// A program is a single-assignment chain of i64 steps over one
// input x. Operands are x, an earlier temp, or an integer literal.
// Instructions: mul_i64, add_i64. Anything outside this grammar is
// refused (fail closed), as is redefinition of a temp, reference to
// an undefined temp, and any i64 overflow during execution — the
// 🛡️ certificate covers the coefficient identity, not overflow,
// so the executor checks overflow itself and never wraps.
//
// Usage:
//   flat prog.flat <integer>       scalar mode: print f(x)
//   flat prog.flat <tensor-file>   tensor mode: apply f elementwise
// Tensor file: first line `scalar` | `vector N` | `matrix R C`,
// second line the values. Tensor output uses ram's array format,
// so results compose with the rest of the machine and can be
// diffed against ramc's elementwise computation of the same map.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define MAX_TEMPS 1024
#define MAX_ITEMS 4096
#define MAX_PROG 65536

typedef struct { int is_temp; long long value; int temp; } Operand;
typedef struct { int op; Operand a, b; int dest; } Step;   // op: 0=mul, 1=add
static Step steps[MAX_TEMPS]; static int nsteps=0;
static int ret_is_temp=0, ret_temp=0;

static const char *p;   // parse cursor
static void skip_ws(void){ while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n') p++; }
static int expect(const char *s){
    skip_ws();
    size_t n=strlen(s);
    if(strncmp(p,s,n)) return 0;
    p+=n; return 1;
}
static int parse_operand(Operand *o){
    skip_ws();
    if(*p=='x' && (p[1]==' '||p[1]==','||p[1]==')'||p[1]==';'||p[1]=='\t'||p[1]=='\0')){ o->is_temp=0; o->value=0; o->temp=-1; p++; return 1; }
    if(*p=='t'){
        char *end; errno=0; long v=strtol(p+1,&end,10);
        if(end==p+1||errno||v<0||v>=MAX_TEMPS) return 0;
        o->is_temp=1; o->temp=(int)v; o->value=0; p=end; return 1;
    }
    char *end; errno=0; long long v=strtoll(p,&end,10);
    if(end==p||errno) return 0;
    o->is_temp=0; o->value=v; o->temp=-2; p=end; return 1;
}
static int defined[MAX_TEMPS];
static int parse_program(const char *text){
    p=text;
    if(!expect("FLAT v1:")) return 0;
    if(!expect("input x:i64")) return 0;
    if(!expect(";")) return 0;
    for(;;){
        skip_ws();
        if(!strncmp(p,"return",6)){
            p+=6;
            Operand o; if(!parse_operand(&o)) return 0;
            if(o.temp==-2) return 0;                    // return must name x or a temp
            ret_is_temp=o.is_temp; ret_temp=o.temp;
            skip_ws();
            if(*p) return 0;                            // trailing junk refused
            return 1;
        }
        if(*p!='t') return 0;
        char *end; errno=0; long dest=strtol(p+1,&end,10);
        if(end==p+1||errno||dest<0||dest>=MAX_TEMPS) return 0;
        if(defined[dest]) return 0;                     // SSA: single assignment
        p=end;
        if(!expect("=")) return 0;
        skip_ws();
        int op;
        if(!strncmp(p,"mul_i64",7)){ op=0; p+=7; }
        else if(!strncmp(p,"add_i64",7)){ op=1; p+=7; }
        else return 0;
        if(!expect("(")) return 0;
        Operand a,b;
        if(!parse_operand(&a)) return 0;
        if(!expect(",")) return 0;
        if(!parse_operand(&b)) return 0;
        if(!expect(")")) return 0;
        if(!expect(";")) return 0;
        if(a.is_temp&&!defined[a.temp]) return 0;
        if(b.is_temp&&!defined[b.temp]) return 0;
        if(nsteps>=MAX_TEMPS) return 0;
        steps[nsteps].op=op; steps[nsteps].a=a; steps[nsteps].b=b; steps[nsteps].dest=(int)dest;
        defined[dest]=1; nsteps++;
    }
}
static int add_ovf(long long a,long long b,long long *r){
    if((b>0&&a>LLONG_MAX-b)||(b<0&&a<LLONG_MIN-b)) return 1;
    *r=a+b; return 0;
}
static int mul_ovf(long long a,long long b,long long *r){
    if(a==0||b==0){ *r=0; return 0; }
    if(a>0){ if(b>0){ if(a>LLONG_MAX/b) return 1; } else { if(b<LLONG_MIN/a) return 1; } }
    else { if(b>0){ if(a<LLONG_MIN/b) return 1; } else { if(a<LLONG_MAX/b) return 1; } }
    *r=a*b; return 0;
}
static int run_one(long long x,long long *out){
    static long long temps[MAX_TEMPS];
    for(int i=0;i<nsteps;i++){
        Step *s=&steps[i];
        long long av= s->a.is_temp?temps[s->a.temp] : s->a.temp==-1?x : s->a.value;
        long long bv= s->b.is_temp?temps[s->b.temp] : s->b.temp==-1?x : s->b.value;
        long long r;
        if(s->op==0){ if(mul_ovf(av,bv,&r)) return 0; }
        else { if(add_ovf(av,bv,&r)) return 0; }
        temps[s->dest]=r;
    }
    *out= ret_is_temp?temps[ret_temp]:x;
    return 1;
}
static char *read_all(FILE *f){
    static char buf[MAX_PROG+1];
    size_t n=fread(buf,1,MAX_PROG,f);
    if(n==MAX_PROG&&!feof(f)) return NULL;
    buf[n]=0; return buf;
}
static int parse_ll(const char *s,long long *out){
    char *end; errno=0; long long v=strtoll(s,&end,10);
    if(errno||!s[0]||*end) return 0;
    *out=v; return 1;
}
int main(int argc,char **argv){
    if(argc!=3){ fprintf(stderr,"usage: flat prog.flat (<integer> | <tensor-file>)\n"); return 2; }
    FILE *f=fopen(argv[1],"r"); if(!f){perror(argv[1]);return 2;}
    char *text=read_all(f); fclose(f);
    if(!text){ fprintf(stderr,"flat: program too large\n"); return 1; }
    if(!parse_program(text)){ fprintf(stderr,"flat: invalid FLAT v1 program\n"); return 1; }
    long long x;
    if(parse_ll(argv[2],&x)){
        long long out;
        if(!run_one(x,&out)){ fprintf(stderr,"flat: i64 overflow\n"); return 1; }
        printf("%lld\n",out);
        return 0;
    }
    FILE *tf=fopen(argv[2],"r"); if(!tf){perror(argv[2]);return 2;}
    char kind[32]; long long d0=1,d1=1;
    if(fscanf(tf,"%31s",kind)!=1){ fclose(tf); fprintf(stderr,"flat: bad tensor file\n"); return 1; }
    int rank;
    if(!strcmp(kind,"scalar")){ rank=0; }
    else if(!strcmp(kind,"vector")){ rank=1; if(fscanf(tf,"%lld",&d0)!=1||d0<1||d0>MAX_ITEMS){fclose(tf);fprintf(stderr,"flat: bad vector size\n");return 1;} }
    else if(!strcmp(kind,"matrix")){ rank=2; if(fscanf(tf,"%lld %lld",&d0,&d1)!=2||d0<1||d1<1||d0>MAX_ITEMS/d1){fclose(tf);fprintf(stderr,"flat: bad matrix size\n");return 1;} }
    else { fclose(tf); fprintf(stderr,"flat: tensor kind must be scalar, vector, or matrix\n"); return 1; }
    size_t n= rank==0?1:(size_t)(d0*(rank==2?d1:1));
    static long long vals[MAX_ITEMS];
    for(size_t i=0;i<n;i++){
        if(fscanf(tf,"%lld",&vals[i])!=1){ fclose(tf); fprintf(stderr,"flat: tensor has too few values\n"); return 1; }
    }
    fclose(tf);
    for(size_t i=0;i<n;i++){
        long long out;
        if(!run_one(vals[i],&out)){ fprintf(stderr,"flat: i64 overflow at element %zu\n",i); return 1; }
        vals[i]=out;
    }
    printf("rank=%d shape=",rank);
    if(rank==0)printf("[]"); else if(rank==1)printf("[%lld]",d0); else printf("[%lld,%lld]",d0,d1);
    printf(" values=[");
    for(size_t i=0;i<n;i++)printf("%s%lld",i?",":"",vals[i]);
    puts("]");
    return 0;
}
