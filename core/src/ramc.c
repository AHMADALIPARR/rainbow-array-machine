#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// Strict Unicode source parser. Emits normalized typed operations, not precomputed results.
static void die(int line,const char *why){fprintf(stderr,"line %d: %s\n",line,why);exit(2);}
static char*next(char**p){while(**p&&isspace((unsigned char)**p))++*p;if(!**p)return NULL;char*s=*p;while(**p&&!isspace((unsigned char)**p))++*p;if(**p)*(*p)++=0;return s;}
static int integer(char*s,int line){if(!s)die(line,"missing integer");char*e;long x=strtol(s,&e,10);if(*e||x<1||x>64)die(line,"invalid dimension (1..64)");return (int)x;}
static void values(char**p,int n,int line,FILE*out){for(int i=0;i<n;i++){char*s=next(p);if(!s)die(line,"missing operand");char*e;double x=strtod(s,&e);if(e==s||*e)die(line,"invalid numeric operand");fprintf(out," %.17g",x);}}
int main(int argc,char**argv){if(argc!=3){fprintf(stderr,"usage: ramc source.ram program.ir\n");return 2;}FILE*in=fopen(argv[1],"rb");if(!in){perror(argv[1]);return 2;}FILE*out=fopen(argv[2],"wb");if(!out){perror(argv[2]);return 2;}char line[32768];int ln=0;fputs("RAMIR1\n",out);
while(fgets(line,sizeof(line),in)){ln++;if(!strchr(line,'\n')&&!feof(in))die(ln,"line too long");char*p=line,*ns=next(&p);if(!ns||ns[0]=='#')continue;
int inverse=0;if(!strcmp(ns,"↶")){inverse=1;ns=next(&p);if(!ns)die(ln,"missing inverse namespace");}
int language=0;if(!strcmp(ns,"🌀"))language=1;else if(!strcmp(ns,"🔺"))language=2;else if(!strcmp(ns,"⚡"))language=3;else if(!strcmp(ns,"🐺"))language=4;else die(ln,"unknown namespace");
char*type=next(&p),*op=next(&p);if(!type||!op)die(ln,"missing type or opcode");
if(inverse&&strcmp(type,"📐"))die(ln,"inverse only implemented for geometry");
if(language==4){if(strcmp(type,"🔹")||strcmp(op,"📜"))die(ln,"unsupported symbolic expression");fprintf(out,"AFFINE");values(&p,4,ln,out);}
else if(!strcmp(type,"📏")){int code=!strcmp(op,"➕")?0:!strcmp(op,"➖")?1:!strcmp(op,"✖️")?2:!strcmp(op,"➗")?3:-1;if(code<0)die(ln,"unsupported vector opcode");int n=integer(next(&p),ln);fprintf(out,"VECTOR %d %d",code,n);values(&p,2*n,ln,out);}
else if(!strcmp(type,"🟦")){if(!strcmp(op,"🧲")){int m=integer(next(&p),ln),k=integer(next(&p),ln),n=integer(next(&p),ln);fprintf(out,"MATMUL %d %d %d",m,k,n);values(&p,m*k+k*n,ln,out);}else if(!strcmp(op,"🔄")){int m=integer(next(&p),ln),n=integer(next(&p),ln);fprintf(out,"TRANSPOSE %d %d",m,n);values(&p,m*n,ln,out);}else die(ln,"unsupported matrix opcode");}
else if(!strcmp(type,"📐")){if(strcmp(op,"@@"))die(ln,"geometry requires @@");char*kind=next(&p);if(!kind)die(ln,"missing geometry transform");int rot=!strcmp(kind,"ROTATE_Z"),trans=!strcmp(kind,"TRANSLATE");if(!rot&&!trans)die(ln,"unsupported geometry transform");int n=integer(next(&p),ln);fprintf(out,"%s %d",rot?"ROTATE":"TRANSLATE",n);if(rot){char*s=next(&p);if(!s)die(ln,"missing quarter turns");char*e;long q=strtol(s,&e,10);if(*e||q< -1000||q>1000)die(ln,"invalid quarter turns");fprintf(out," %ld",inverse?-q:q);}else {if(inverse){double d[3];for(int i=0;i<3;i++){char*s=next(&p);if(!s)die(ln,"missing translation");char*e;d[i]=strtod(s,&e);if(e==s||*e)die(ln,"invalid translation");fprintf(out," %.17g",-d[i]);}}else values(&p,3,ln,out);}values(&p,3*n,ln,out);}
else die(ln,"unsupported array type");
char*assertion=next(&p);if(assertion){if(strcmp(assertion,">>>>>>>€"))die(ln,"unexpected trailing input");fputs(" ASSERT",out);int count=0;if(!strcmp(type,"📏")) { // count must be read from emitted command; parse from original line instead
 char tmp[32768];strcpy(tmp,line);(void)tmp;
 } // assertion expected count resolved by executor from operation shape
 // consume remaining expected numeric values without evaluating
 while((assertion=next(&p))){char*e;strtod(assertion,&e);if(e==assertion||*e)die(ln,"invalid assertion number");fprintf(out," %s",assertion);count++;}
 if(!count)die(ln,"empty assertion");}
fputc('\n',out);
}if(ferror(in))die(ln,"read error");fclose(in);if(fclose(out))return 2;return 0;}
