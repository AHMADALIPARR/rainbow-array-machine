/* Validated RAMIR1 -> executable typed FLAT2. The payload is canonical IR,
   never a precomputed result. Validation is performed by flat_check first. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 if(argc!=4){fputs("usage: flat_lower input.ir metadata.flat output.flat2\n",stderr);return 2;}
 FILE *ir=fopen(argv[1],"rb"),*meta=fopen(argv[2],"rb"),*out=fopen(argv[3],"wb");
 if(!ir||!meta||!out){perror("open");return 2;}
 char il[131072],ml[131072];
 if(!fgets(il,sizeof il,ir)||strcmp(il,"RAMIR1\n")||!fgets(ml,sizeof ml,meta)||strcmp(ml,"FLAT1\n")){fputs("bad headers\n",stderr);return 2;}
 fputs("FLAT2\n",out);int count=0;
 while(fgets(il,sizeof il,ir)){
  if(!fgets(ml,sizeof ml,meta)||strncmp(ml,"OP ",3)){fputs("metadata missing\n",stderr);return 2;}
  if(!strchr(il,'\n')||!strchr(ml,'\n')){fputs("line too long\n",stderr);return 2;}
  fputs(ml,out);fputs(il,out);count++;
 }
 if(fgets(ml,sizeof ml,meta)||!count){fputs("metadata mismatch or empty\n",stderr);return 2;}
 return fclose(out)!=0;
}
