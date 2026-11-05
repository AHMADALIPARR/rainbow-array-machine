/* RAMIR1 -> standalone WebAssembly C source lowering. No runtime IR interpreter. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#define MAXTOK 9000
static void fail(int line,const char *reason){fprintf(stderr,"IR line %d: %s\n",line,reason);exit(2);}
static double number(const char *s,int ln){if(!s)fail(ln,"missing numeric operand");char *end;errno=0;double v=strtod(s,&end);if(errno||end==s||*end||!isfinite(v))fail(ln,"invalid finite numeric operand");return v;}
static int dim(char **t,int *p,int limit,int ln){int x=(int)number(t[(*p)++],ln);if(x<1||x>limit)fail(ln,"invalid dimension");return x;}
static void literal(FILE *o,char **t,int *p,int n,int ln){fputc('{',o);for(int i=0;i<n;i++){if(i)fputc(',',o);fprintf(o,"%.17g",number(t[(*p)++],ln));}fputc('}',o);}
int main(int argc,char **argv){if(argc!=3){fputs("usage: ram_codegen program.ir program.c\n",stderr);return 2;}FILE *in=fopen(argv[1],"r"),*out=fopen(argv[2],"w");if(!in||!out){perror("open");return 2;}char line[131072];if(!fgets(line,sizeof line,in)||(strcmp(line,"RAMIR1\n") && strcmp(line,"FLAT2\n"))){fputs("invalid IR header\n",stderr);return 2;}
fputs("#include <stdint.h>\nextern int elementwise(const double*,const double*,double*,int,int);\nextern int matmul(const double*,const double*,double*,int,int,int);\nextern int transpose(const double*,double*,int,int);\nextern double affine(double,double,double,double);\nextern int rotate_z_quarter(const double*,double*,int,int);\nextern int translate(const double*,double*,int,double,double,double);\nextern int assert_equal(const double*,const double*,int,double);\nstatic double output[65536];\n__attribute__((export_name(\"result_ptr\"))) int result_ptr(void){return (int)(uintptr_t)output;}\n",out);
int flat2=0; /* header was consumed; use file position to inspect it */
rewind(in);if(!fgets(line,sizeof line,in))return 2;flat2=!strcmp(line,"FLAT2\n");
int ln=1,ops=0,total=0;char statements[2621440];size_t used=0;statements[0]=0;
while(fgets(line,sizeof line,in)){ln++;if(flat2){if(strncmp(line,"OP ",3))fail(ln,"missing typed FLAT operation");if(!fgets(line,sizeof line,in))fail(ln,"missing executable FLAT payload");ln++;}if(!strchr(line,'\n')&&!feof(in))fail(ln,"IR line too long");char *t[MAXTOK];int nt=0;for(char *s=strtok(line," \t\r\n");s;s=strtok(NULL," \t\r\n")){if(nt>=MAXTOK)fail(ln,"too many operands");t[nt++]=s;}if(!nt)continue;int p=1,n=0,m=0,k=0,mode=0;const char *op=t[0];char *buf=statements+used;size_t room=sizeof statements-used;FILE *tmp=tmpfile();if(!tmp){perror("tmpfile");return 2;}
if(!strcmp(op,"VECTOR")){mode=(int)number(t[p++],ln);if(mode<0||mode>3)fail(ln,"invalid vector opcode");n=dim(t,&p,64,ln);fprintf(out,"static const double a%d[%d]=",ops,n);literal(out,t,&p,n,ln);fputs(";\n",out);fprintf(out,"static const double b%d[%d]=",ops,n);literal(out,t,&p,n,ln);fputs(";\n",out);fprintf(tmp,"if(elementwise(a%d,b%d,output+%d,%d,%d))return %d;\n",ops,ops,total,n,mode,100+ops);}
else if(!strcmp(op,"MATMUL")){m=dim(t,&p,64,ln);k=dim(t,&p,64,ln);n=dim(t,&p,64,ln);fprintf(out,"static const double a%d[%d]=",ops,m*k);literal(out,t,&p,m*k,ln);fputs(";\n",out);fprintf(out,"static const double b%d[%d]=",ops,k*n);literal(out,t,&p,k*n,ln);fputs(";\n",out);fprintf(tmp,"if(matmul(a%d,b%d,output+%d,%d,%d,%d))return %d;\n",ops,ops,total,m,k,n,100+ops);n=m*n;}
else if(!strcmp(op,"TRANSPOSE")){m=dim(t,&p,64,ln);n=dim(t,&p,64,ln);fprintf(out,"static const double a%d[%d]=",ops,m*n);literal(out,t,&p,m*n,ln);fputs(";\n",out);fprintf(tmp,"if(transpose(a%d,output+%d,%d,%d))return %d;\n",ops,total,m,n,100+ops);n=m*n;}
else if(!strcmp(op,"AFFINE")){double a=number(t[p++],ln),b=number(t[p++],ln),c=number(t[p++],ln),x=number(t[p++],ln);n=1;fprintf(tmp,"output[%d]=affine(%.17g,%.17g,%.17g,%.17g);\n",total,a,b,c,x);}
else if(!strcmp(op,"ROTATE")||!strcmp(op,"TRANSLATE")){n=dim(t,&p,64,ln);double x=0,y=0,z=0;int q=0;if(!strcmp(op,"ROTATE"))q=(int)number(t[p++],ln);else {x=number(t[p++],ln);y=number(t[p++],ln);z=number(t[p++],ln);}fprintf(out,"static const double a%d[%d]=",ops,3*n);literal(out,t,&p,3*n,ln);fputs(";\n",out);if(!strcmp(op,"ROTATE"))fprintf(tmp,"if(rotate_z_quarter(a%d,output+%d,%d,%d))return %d;\n",ops,total,n,q,100+ops);else fprintf(tmp,"if(translate(a%d,output+%d,%d,%.17g,%.17g,%.17g))return %d;\n",ops,total,n,x,y,z,100+ops);n=3*n;}
else fail(ln,"unknown IR instruction");
if(total+n>65536)fail(ln,"output capacity exceeded");
if(p<nt){if(strcmp(t[p++],"ASSERT"))fail(ln,"unexpected trailing token");if(nt-p!=n)fail(ln,"assertion shape mismatch");fprintf(out,"static const double expected%d[%d]=",ops,n);literal(out,t,&p,n,ln);fputs(";\n",out);fprintf(tmp,"if(assert_equal(output+%d,expected%d,%d,1e-10))return %d;\n",total,ops,n,200+ops);}if(p!=nt)fail(ln,"extra tokens");
rewind(tmp);int c;size_t x=0;while((c=fgetc(tmp))!=EOF){if(x+1>=room)fail(ln,"program too long");buf[x++]=(char)c;}fclose(tmp);buf[x]=0;used+=x;total+=n;ops++;}
if(!ops)fail(ln,"no instructions");
fprintf(out,"__attribute__((export_name(\"result_len\"))) int result_len(void){return %d;}\n",total);fputs("__attribute__((export_name(\"run\"))) int run(void){\n",out);fputs(statements,out);fputs("return 0;}\n",out);fclose(in);if(fclose(out))return 2;printf("lowered %d instructions, %d numeric outputs\n",ops,total);return 0;}
