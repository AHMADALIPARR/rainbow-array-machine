// Rainbow Array Machine: deterministic UTF-8 emoji array interpreter (C11).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>

#define MAX_ITEMS 4096
#define MAX_LINE 16384
#define MAX_TOKENS 8192

typedef struct { double *data; size_t n; size_t rows, cols; int rank; } Array;
typedef struct { const char *emoji, *name; } Symbol;
static const Symbol languages[]={{"🌀","BQN"},{"🔺","APL"},{"⚡","K"},{"🧩","J"},{"🪐","Uiua"},{"🏛️","Pascal"},{"⚙️","C"},{"🦀","Rust"},{"🧱","Forth"},{"🔌","SystemVerilog"},{"🐦","Unlambda"},{"λ","Haskell"},{"🕸️","Prolog"}};
static const Symbol types[]={{"🔹","scalar"},{"📏","vector"},{"🟦","matrix"},{"🧊","tensor"},{"🔱","ternary"}};
static const Symbol operations[]={{"➕","add"},{"➖","subtract"},{"✖️","multiply"},{"🧲","matmul"},{"🔄","transpose"},{"📦","flatten"},{"📐","shape"},{"🧮","sum"}};
static int lookup(const Symbol *s,size_t n,const char *t){for(size_t i=0;i<n;i++)if(strcmp(s[i].emoji,t)==0)return (int)i;return -1;}
static int fail(int line,const char *why){fprintf(stderr,"line %d: %s\n",line,why);return 1;}
static void release(Array *a){free(a->data);memset(a,0,sizeof(*a));}
static int allocate(Array *a,size_t rows,size_t cols,int rank){
 if(!rows||!cols||rows>MAX_ITEMS/cols)return 0;
 a->data=calloc(rows*cols,sizeof(double));if(!a->data)return 0;
 a->n=rows*cols;a->rows=rows;a->cols=cols;a->rank=rank;return 1;
}
static int parse_size(const char *s,size_t *out){char *end;errno=0;unsigned long long x=strtoull(s,&end,10);if(errno||!s[0]||*end||!x||x>MAX_ITEMS)return 0;*out=(size_t)x;return 1;}
static int parse_num(const char *s,double *out){char *end;errno=0;double x=strtod(s,&end);if(errno||!s[0]||*end||!isfinite(x))return 0;*out=x;return 1;}
static void print_array(const Array *a){printf("rank=%d shape=",a->rank);if(a->rank==0)printf("[]");else if(a->rank==1)printf("[%zu]",a->n);else printf("[%zu,%zu]",a->rows,a->cols);printf(" values=[");for(size_t i=0;i<a->n;i++)printf("%s%.12g",i?",":"",a->data[i]);puts("]");}
static int compatible(const Array *a,const Array *b){return a->rank==b->rank&&a->rows==b->rows&&a->cols==b->cols;}
static int execute(char **t,int count,int line){
 if(count<4)return fail(line,"expected: LANGUAGE TYPE OP operands...");
 int lang=lookup(languages,sizeof(languages)/sizeof(*languages),t[0]);
 int type=lookup(types,sizeof(types)/sizeof(*types),t[1]);
 int op=lookup(operations,sizeof(operations)/sizeof(*operations),t[2]);
 if(lang<0)return fail(line,"unknown language namespace");
 if(type<0)return fail(line,"unknown array type");
 if(op<0)return fail(line,"unknown operation");
 // The executable v0.1 subset uses explicitly shared mathematical semantics.
 // Language-specific semantics are NOT silently inferred or claimed.
 if(lang>2)return fail(line,"namespace registered, execution semantics not implemented");
 if(type>2)return fail(line,"array type registered, execution semantics not implemented");
 Array a={0},b={0},result={0};int status=1;size_t rows=1,cols=1,offset=3;
 if(type==0){if(count<4||!allocate(&a,1,1,0))goto bad;a.data[0]=0;}
 else if(type==1){if(count<5||!parse_size(t[3],&cols)||!allocate(&a,1,cols,1))goto bad;offset=4;}
 else {if(count<6||!parse_size(t[3],&rows)||!parse_size(t[4],&cols)||!allocate(&a,rows,cols,2))goto bad;offset=5;}
 size_t required=a.n;
 int binary=(op<=3);
 if(binary){
  if(count!=(int)(offset+required*2))goto bad;
  if(!allocate(&b,rows,cols,type))goto bad;
  for(size_t i=0;i<required;i++)if(!parse_num(t[offset+i],&a.data[i])||!parse_num(t[offset+required+i],&b.data[i]))goto bad;
 }else{
  if(count!=(int)(offset+required))goto bad;
  for(size_t i=0;i<required;i++)if(!parse_num(t[offset+i],&a.data[i]))goto bad;
 }
 if(binary&&op<=2){
  if(!compatible(&a,&b)||!allocate(&result,rows,cols,type))goto bad;
  for(size_t i=0;i<a.n;i++)result.data[i]=op==0?a.data[i]+b.data[i]:op==1?a.data[i]-b.data[i]:a.data[i]*b.data[i];
 }else if(op==3){
  // Matrix multiply is intentionally separate: use a dedicated syntax below.
  goto bad;
 }else if(op==4){
  if(type!=2||!allocate(&result,cols,rows,2))goto bad;
  for(size_t r=0;r<rows;r++)for(size_t c=0;c<cols;c++)result.data[c*rows+r]=a.data[r*cols+c];
 }else if(op==5){
  if(!allocate(&result,1,a.n,1))goto bad;
  memcpy(result.data,a.data,a.n*sizeof(double));
 }else if(op==6){
  if(!allocate(&result,1,type==2?2:1,1))goto bad;
  result.data[0]=(double)(type==2?rows:a.n);if(type==2)result.data[1]=(double)cols;
 }else if(op==7){
  if(!allocate(&result,1,1,0))goto bad;
  for(size_t i=0;i<a.n;i++)result.data[0]+=a.data[i];
 }
 printf("%s %s %s => ",languages[lang].name,types[type].name,operations[op].name);print_array(&result);status=0;goto done;
 bad:fail(line,"invalid shape, operand count, value, or unsupported operation");
 done:release(&a);release(&b);release(&result);return status;
}
static int matmul(char **t,int n,int line){
 // 🌀 🟦 🧲 M K N A[M*K] B[K*N]
 if(n<7)return fail(line,"matmul expects M K N and matrix values");
 int lang=lookup(languages,sizeof(languages)/sizeof(*languages),t[0]);
 if(lang<0||lang>2||strcmp(t[1],"🟦")||strcmp(t[2],"🧲"))return fail(line,"matmul supports BQN/APL/K matrix namespaces");
 size_t m,k,nn;if(!parse_size(t[3],&m)||!parse_size(t[4],&k)||!parse_size(t[5],&nn)||m>MAX_ITEMS/k||k>MAX_ITEMS/nn||m>MAX_ITEMS/nn)return fail(line,"invalid matmul dimensions");
 if((size_t)n!=6+m*k+k*nn)return fail(line,"incorrect matmul operand count");
 Array out={0};if(!allocate(&out,m,nn,2))return fail(line,"allocation failed");
 double left[MAX_ITEMS],right[MAX_ITEMS];for(size_t i=0;i<m*k;i++)if(!parse_num(t[6+i],&left[i])){release(&out);return fail(line,"invalid left operand");}
 for(size_t i=0;i<k*nn;i++)if(!parse_num(t[6+m*k+i],&right[i])){release(&out);return fail(line,"invalid right operand");}
 for(size_t i=0;i<m;i++)for(size_t j=0;j<nn;j++)for(size_t z=0;z<k;z++)out.data[i*nn+j]+=left[i*k+z]*right[z*nn+j];
 printf("%s matrix matmul => ",languages[lang].name);print_array(&out);release(&out);return 0;
}
int main(int argc,char **argv){
 if(argc==2&&!strcmp(argv[1],"--dictionary")){
  for(size_t i=0;i<sizeof(languages)/sizeof(*languages);i++)printf("LANG %s %s\n",languages[i].emoji,languages[i].name);
  for(size_t i=0;i<sizeof(types)/sizeof(*types);i++)printf("TYPE %s %s\n",types[i].emoji,types[i].name);
  for(size_t i=0;i<sizeof(operations)/sizeof(*operations);i++)printf("OP %s %s\n",operations[i].emoji,operations[i].name);
  return 0;
 }
 if(argc!=2){fprintf(stderr,"usage: ram file.ram | --dictionary\n");return 2;}
 FILE *f=fopen(argv[1],"r");if(!f){perror(argv[1]);return 2;}
 char line[MAX_LINE];int lineno=0,errors=0;
 while(fgets(line,sizeof(line),f)){
  lineno++;if(!strchr(line,'\n')&&!feof(f)){errors+=fail(lineno,"line too long");int c;while((c=fgetc(f))!='\n'&&c!=EOF);continue;}
  char *tok[MAX_TOKENS];int n=0;char *p=strtok(line," \t\r\n");
  while(p&&n<MAX_TOKENS){if(p[0]=='#'){p=NULL;break;}tok[n++]=p;p=strtok(NULL," \t\r\n");}
  if(p){errors+=fail(lineno,"too many tokens");continue;}
  if(n){if(n>=3&&!strcmp(tok[2],"🧲"))errors+=matmul(tok,n,lineno);else errors+=execute(tok,n,lineno);}
 }
 if(ferror(f)){perror("read");errors++;}fclose(f);return errors?1:0;
}
