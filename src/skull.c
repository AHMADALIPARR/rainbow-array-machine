// skull — the Skull & Crown binary encoding (owner's design).
//
// 💀 (U+1F480) is bit 0, 👑 (U+1F451) is bit 1, most-significant
// bit first, byte-aligned. Every byte becomes eight emojis; the
// encoding is lossless and reversible — an alphabet, not
// compression or encryption. In UTF-8 each symbol is four bytes,
// so a .skull file is roughly 32x the binary it encodes.
//
//   skull encode in.bin  out.skull
//   skull decode in.skull out.bin
//
// Canonical form: one byte per line (eight symbols, then '\n') —
// the layout of the owner's worked example: the WASM magic
// 00 61 73 6D encodes as four lines of eight symbols.
// The decoder skips ASCII whitespace and refuses everything else:
// any other codepoint, a truncated UTF-8 sequence, or a symbol
// count that is not a multiple of eight (byte alignment enforced).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char SKULL[4]={(char)0xF0,(char)0x9F,(char)0x92,(char)0x80};  // U+1F480
static const char CROWN[4]={(char)0xF0,(char)0x9F,(char)0x91,(char)0x91};  // U+1F451
#define MAX_BYTES (64u<<20)

static unsigned char *read_all(FILE *f,size_t *n_out){
    unsigned char *buf=malloc(MAX_BYTES);
    if(!buf) return NULL;
    size_t n=fread(buf,1,MAX_BYTES,f);
    if(n==MAX_BYTES&&!feof(f)){ free(buf); return NULL; }
    *n_out=n; return buf;
}
static int do_encode(FILE *in,FILE *out){
    size_t n; unsigned char *buf=read_all(in,&n);
    if(!buf){ fprintf(stderr,"skull: input too large\n"); return 1; }
    for(size_t i=0;i<n;i++){
        for(int bit=7;bit>=0;bit--){
            const char *sym=((buf[i]>>bit)&1)?CROWN:SKULL;
            if(fwrite(sym,1,4,out)!=4){ free(buf); fprintf(stderr,"skull: write failed\n"); return 1; }
        }
        fputc('\n',out);
    }
    free(buf); return 0;
}
// Bitwise NOT on the encoded form: swap 💀 and 👑 symbol for
// symbol. Involutive — inverting twice restores the input. This
// operates on the representation: it is not a bit reversal and
// not a decode. Whitespace is preserved; anything else refuses.
static int do_invert(FILE *in,FILE *out){
    size_t n; unsigned char *buf=read_all(in,&n);
    if(!buf){ fprintf(stderr,"skull: input too large\n"); return 1; }
    size_t pos=0;
    while(pos<n){
        unsigned char c=buf[pos];
        if(c==' '||c=='\t'||c=='\r'||c=='\n'){ fputc(c,out); pos++; continue; }
        if(pos+4<=n&&!memcmp(buf+pos,SKULL,4)){ if(fwrite(CROWN,1,4,out)!=4){free(buf);return 1;} }
        else if(pos+4<=n&&!memcmp(buf+pos,CROWN,4)){ if(fwrite(SKULL,1,4,out)!=4){free(buf);return 1;} }
        else { fprintf(stderr,"skull: invalid symbol at byte offset %zu\n",pos); free(buf); return 1; }
        pos+=4;
    }
    free(buf); return 0;
}
static int do_decode(FILE *in,FILE *out){
    size_t n; unsigned char *buf=read_all(in,&n);
    if(!buf){ fprintf(stderr,"skull: input too large\n"); return 1; }
    unsigned cur=0; int bits=0; size_t pos=0;
    while(pos<n){
        unsigned char c=buf[pos];
        if(c==' '||c=='\t'||c=='\r'||c=='\n'){ pos++; continue; }
        int bit=-1;
        if(pos+4<=n&&!memcmp(buf+pos,SKULL,4)) bit=0;
        else if(pos+4<=n&&!memcmp(buf+pos,CROWN,4)) bit=1;
        if(bit<0){ fprintf(stderr,"skull: invalid symbol at byte offset %zu\n",pos); free(buf); return 1; }
        pos+=4;
        cur=(cur<<1)|(unsigned)bit; bits++;
        if(bits==8){ fputc((int)cur,out); cur=0; bits=0; }
    }
    free(buf);
    if(bits!=0){ fprintf(stderr,"skull: symbol count is not a multiple of 8 (byte alignment)\n"); return 1; }
    return 0;
}
int main(int argc,char **argv){
    if(argc!=4||(strcmp(argv[1],"encode")&&strcmp(argv[1],"decode")&&strcmp(argv[1],"invert"))){
        fprintf(stderr,"usage: skull (encode|decode|invert) <input> <output>\n"); return 2;
    }
    FILE *in=fopen(argv[2],"rb"); if(!in){perror(argv[2]);return 2;}
    FILE *out=fopen(argv[3],"wb"); if(!out){perror(argv[3]);fclose(in);return 2;}
    int rc= !strcmp(argv[1],"encode")?do_encode(in,out)
          : !strcmp(argv[1],"decode")?do_decode(in,out):do_invert(in,out);
    fclose(in);
    if(fclose(out)!=0&&rc==0) rc=1;
    if(rc!=0) remove(argv[3]);
    return rc;
}
