// ramc — the Rainbow Array Machine compiler front end (v0.2 core).
//
// Pipeline: source lines -> typed IR -> execution of the IR.
// v0.1's ram executes lines directly; ramc first lowers every line
// into canonical typed IR nodes (SSA ids, explicit result types with
// rank and shape, source line positions), type-checks operand and
// reference shapes at IR-build time, and only then executes. Results
// print in exactly the v0.1 format, so the two binaries can be
// diffed line for line (tests/run-ir.sh does).
//
// Bindings: a line may open with `NAME <-` (the two tokens NAME and
// the assignment arrow) to bind its result. A later operand may be
// written `@NAME` in place of a literal operand; the operand's kind
// and shape come from the bound node's type and are checked against
// the line's declared type. This is the binding form the persistent
// buffers of v0.4 build on.
//
// Namespaces and types that are registered but unimplemented
// (including the reserved tensor and ternary types) fail closed at
// the IR stage, exactly as v0.1 fails them at execution.
//
// Usage: ramc [--emit-ir] file.ram
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#define MAX_ITEMS 4096
#define MAX_LINE 16384
#define MAX_TOKENS 8192

typedef struct { const char *emoji, *name; } Symbol;
static const Symbol languages[]={{"🌀","BQN"},{"🔺","APL"},{"⚡","K"},{"🧩","J"},{"🪐","Uiua"},{"🏛️","Pascal"},{"⚙️","C"},{"🦀","Rust"},{"🧱","Forth"},{"🔌","SystemVerilog"},{"🐦","Unlambda"},{"λ","Haskell"},{"🕸️","Prolog"}};
static const Symbol types[]={{"🔹","scalar"},{"📏","vector"},{"🟦","matrix"},{"🧊","tensor"},{"🔱","ternary"}};
static const Symbol operations[]={{"➕","add"},{"➖","subtract"},{"✖️","multiply"},{"🧲","matmul"},{"🔄","transpose"},{"📦","flatten"},{"📐","shape"},{"🧮","sum"}};
static int lookup(const Symbol *s,size_t n,const char *t){for(size_t i=0;i<n;i++)if(strcmp(s[i].emoji,t)==0)return (int)i;return -1;}

typedef enum { K_SCALAR=0, K_VECTOR=1, K_MATRIX=2 } Kind;
typedef struct { Kind kind; size_t d0, d1; } Type;   // vector: d0=n; matrix: d0=rows, d1=cols
typedef enum { IR_CONST, IR_ADD, IR_SUB, IR_MUL, IR_MATMUL,
               IR_TRANSPOSE, IR_FLATTEN, IR_SHAPE, IR_SUM, IR_BIND } IrOp;
static const char *ir_names[]={"const","add","subtract","multiply","matmul",
                               "transpose","flatten","shape","sum","bind"};

typedef struct {
    IrOp op; Type type; int lang; int line; int decl;
    int lhs, rhs;              // operand node ids (-1 = none)
    double *data; size_t n;    // IR_CONST payload
    char *name;                // IR_BIND name
} Node;

static Node *nodes=NULL; static size_t nnodes=0, nodecap=0;
static int add_node(Node nd){
    if(nnodes==nodecap){ nodecap=nodecap?nodecap*2:64; nodes=realloc(nodes,nodecap*sizeof(Node)); if(!nodes){perror("realloc");exit(2);} }
    nodes[nnodes]=nd; return (int)nnodes++;
}
typedef struct { char *name; int id; } Binding;
static Binding *binds=NULL; static size_t nbinds=0, bindcap=0;
static int find_bind(const char *name){ for(size_t i=0;i<nbinds;i++) if(!strcmp(binds[i].name,name)) return binds[i].id; return -1; }
static char *dupstr(const char *s){ size_t n=strlen(s)+1; char *p=malloc(n); if(!p){perror("malloc");exit(2);} memcpy(p,s,n); return p; }

static int cur_line=0;
static int ir_fail(const char *why){ fprintf(stderr,"line %d: %s\n",cur_line,why); return -1; }

static int parse_size(const char *s,size_t *out){char *end;errno=0;unsigned long long x=strtoull(s,&end,10);if(errno||!s[0]||*end||!x||x>MAX_ITEMS)return 0;*out=(size_t)x;return 1;}
static int parse_num(const char *s,double *out){char *end;errno=0;double x=strtod(s,&end);if(errno||!s[0]||*end||!isfinite(x))return 0;*out=x;return 1;}
static int is_name(const char *s){
    if(!s[0])return 0;
    for(size_t i=0;s[i];i++){ char c=s[i];
        if(!(c=='_'||(c>='a'&&c<='z')||(c>='A'&&c<='Z')||(i&&c>='0'&&c<='9')))return 0; }
    return 1;
}
static size_t type_count(Type t){ return t.kind==K_SCALAR?1:(t.kind==K_VECTOR?t.d0:t.d0*t.d1); }
static int type_equal(Type a,Type b){ return a.kind==b.kind&&a.d0==b.d0&&a.d1==b.d1; }
static void fmt_type(Type t,char *buf,size_t cap){
    if(t.kind==K_SCALAR)snprintf(buf,cap,"scalar");
    else if(t.kind==K_VECTOR)snprintf(buf,cap,"vector[%zu]",t.d0);
    else snprintf(buf,cap,"matrix[%zu,%zu]",t.d0,t.d1);
}

// Parse one operand starting at token *pos. Declared kind/dims are
// the line's, used for literals. Returns the operand's node id and
// advances *pos, or -1 on error.
static int parse_operand(char **t,int count,int *pos,Kind declared,Type header,int lang){
    if(*pos>=count) { ir_fail("missing operand"); return -1; }
    const char *tok=t[*pos];
    if(tok[0]=='@'){
        if(!is_name(tok+1)){ ir_fail("malformed reference"); return -1; }
        int id=find_bind(tok+1);
        if(id<0){ ir_fail("reference to unbound name"); return -1; }
        (*pos)++;
        return id;
    }
    Type ty; ty.kind=declared; ty.d0=header.d0; ty.d1=header.d1;
    size_t need=type_count(ty);
    if((size_t)(count-*pos)<need){ ir_fail("operand has too few values"); return -1; }
    Node nd; memset(&nd,0,sizeof nd); nd.op=IR_CONST; nd.type=ty; nd.lang=lang; nd.line=cur_line;
    nd.lhs=nd.rhs=-1; nd.n=need; nd.data=malloc(need*sizeof(double));
    if(!nd.data){perror("malloc");exit(2);}
    for(size_t i=0;i<need;i++)
        if(!parse_num(t[*pos+(int)i],&nd.data[i])){ free(nd.data); ir_fail("invalid numeric value"); return -1; }
    *pos+=(int)need;
    return add_node(nd);
}

// Lower one source line. Returns 0 on success, 1 on IR error.
static int lower_line(char **t,int count){
    size_t checkpoint=nnodes;
    int base=0; const char *bindname=NULL;
    if(count>=2 && is_name(t[0]) && !strcmp(t[1],"←")){ bindname=t[0]; base=2; }
    if(count-base<3){ ir_fail("expected: [NAME <-] LANGUAGE TYPE OP operands..."); goto bad; }
    int lang=lookup(languages,sizeof(languages)/sizeof(*languages),t[base]);
    int type=lookup(types,sizeof(types)/sizeof(*types),t[base+1]);
    int op=lookup(operations,sizeof(operations)/sizeof(*operations),t[base+2]);
    if(lang<0){ ir_fail("unknown language namespace"); goto bad; }
    if(type<0){ ir_fail("unknown array type"); goto bad; }
    if(op<0){ ir_fail("unknown operation"); goto bad; }
    if(lang>2){ ir_fail("namespace registered, execution semantics not implemented"); goto bad; }
    if(type>2){ ir_fail("array type registered, execution semantics not implemented"); goto bad; }
    Kind declared=(Kind)type;
    int pos=base+3; Node nd; memset(&nd,0,sizeof nd); nd.lang=lang; nd.line=cur_line; nd.lhs=nd.rhs=-1; nd.decl=type;
    int result=-1;
    if(op<=2){                                    // elementwise binary
        // The dimension header sizes literal operands. When the first
        // operand is a reference it carries its own shape, and any
        // literal after it is sized by the reference's type instead.
        Type header; header.kind=declared; header.d0=1; header.d1=1;
        int first_is_ref = pos<count && t[pos][0]=='@';
        if(!first_is_ref){
            if(declared==K_VECTOR){ if(pos>=count||!parse_size(t[pos],&header.d0)){ir_fail("missing vector length");goto bad;} pos++; }
            if(declared==K_MATRIX){ if(pos+1>=count||!parse_size(t[pos],&header.d0)||!parse_size(t[pos+1],&header.d1)){ir_fail("missing matrix dimensions");goto bad;} pos+=2; }
        }
        int a=parse_operand(t,count,&pos,declared,header,lang); if(a<0)goto bad;
        Type hb = first_is_ref? nodes[a].type : header;
        int b=parse_operand(t,count,&pos,declared,hb,lang); if(b<0)goto bad;
        if(!type_equal(nodes[a].type,nodes[b].type)){ ir_fail("elementwise operands must have identical type and shape"); goto bad; }
        if(nodes[a].type.kind!=declared){ ir_fail("operand kind does not match the declared type"); goto bad; }
        nd.op= op==0?IR_ADD: op==1?IR_SUB: IR_MUL; nd.type=nodes[a].type; nd.lhs=a; nd.rhs=b;
        result=add_node(nd);
    } else if(op==3){                             // matmul: header M K N
        if(declared!=K_MATRIX){ ir_fail("matmul requires the matrix type"); goto bad; }
        size_t m,k,nn;
        if(pos+2>=count||!parse_size(t[pos],&m)||!parse_size(t[pos+1],&k)||!parse_size(t[pos+2],&nn)){ ir_fail("matmul expects M K N"); goto bad; }
        pos+=3;
        Type ha={K_MATRIX,m,k}, hb={K_MATRIX,k,nn};
        int a=parse_operand(t,count,&pos,declared,ha,lang); if(a<0)goto bad;
        int b=parse_operand(t,count,&pos,declared,hb,lang); if(b<0)goto bad;
        if(!type_equal(nodes[a].type,ha)||!type_equal(nodes[b].type,hb)){ ir_fail("matmul operand shape does not match M K N"); goto bad; }
        nd.op=IR_MATMUL; nd.type=(Type){K_MATRIX,m,nn}; nd.lhs=a; nd.rhs=b;
        result=add_node(nd);
    } else {                                      // unary ops 4..7
        Type header; header.kind=declared; header.d0=1; header.d1=1;
        int only_is_ref = pos<count && t[pos][0]=='@';
        if(!only_is_ref){
            if(declared==K_VECTOR){ if(pos>=count||!parse_size(t[pos],&header.d0)){ir_fail("missing vector length");goto bad;} pos++; }
            if(declared==K_MATRIX){ if(pos+1>=count||!parse_size(t[pos],&header.d0)||!parse_size(t[pos+1],&header.d1)){ir_fail("missing matrix dimensions");goto bad;} pos+=2; }
        }
        int a=parse_operand(t,count,&pos,declared,header,lang); if(a<0)goto bad;
        if(nodes[a].type.kind!=declared){ ir_fail("operand kind does not match the declared type"); goto bad; }
        Type in=nodes[a].type;
        nd.lhs=a;
        if(op==4){ if(in.kind!=K_MATRIX){ir_fail("transpose requires a matrix");goto bad;}
                   nd.op=IR_TRANSPOSE; nd.type=(Type){K_MATRIX,in.d1,in.d0}; }
        else if(op==5){ nd.op=IR_FLATTEN; nd.type=(Type){K_VECTOR,type_count(in),1}; }
        else if(op==6){ nd.op=IR_SHAPE; nd.type=(Type){K_VECTOR,in.kind==K_MATRIX?2:1,1}; }
        else { nd.op=IR_SUM; nd.type=(Type){K_SCALAR,1,1}; }
        result=add_node(nd);
    }
    if(pos!=count){ ir_fail("trailing tokens after instruction"); goto bad; }
    if(bindname){
        if(find_bind(bindname)>=0){ ir_fail("name is already bound"); goto bad; }
        Node bd; memset(&bd,0,sizeof bd); bd.op=IR_BIND; bd.type=nodes[result].type;
        bd.lang=lang; bd.line=cur_line; bd.lhs=result; bd.rhs=-1; bd.name=dupstr(bindname);
        int bid=add_node(bd);
        if(nbinds==bindcap){ bindcap=bindcap?bindcap*2:32; binds=realloc(binds,bindcap*sizeof(Binding)); if(!binds){perror("realloc");exit(2);} }
        binds[nbinds].name=bd.name; binds[nbinds].id=bid; nbinds++;
    }
    return 0;
 bad:
    for(size_t i=checkpoint;i<nnodes;i++){ free(nodes[i].data); free(nodes[i].name); }
    nnodes=checkpoint;
    return 1;
}

static void emit_ir(void){
    for(size_t i=0;i<nnodes;i++){
        Node *nd=&nodes[i]; char tb[64]; fmt_type(nd->type,tb,sizeof tb);
        printf("%%%zu = ",i);
        if(nd->op==IR_CONST){
            printf("const %s [",tb);
            for(size_t j=0;j<nd->n;j++)printf("%s%.12g",j?", ":"",nd->data[j]);
            printf("] @L%d\n",nd->line);
        } else if(nd->op==IR_BIND){
            printf("bind \"%s\" %%%d @L%d\n",nd->name,nd->lhs,nd->line);
        } else {
            for(const char *p=languages[nd->lang].name;*p;p++)putchar(*p>='A'&&*p<='Z'?*p-'A'+'a':*p);
            putchar('.');
            fputs(ir_names[nd->op],stdout);
            printf(" %%%d",nd->lhs);
            if(nd->rhs>=0)printf(", %%%d",nd->rhs);
            printf(" : %s @L%d\n",tb,nd->line);
        }
    }
}

// ---- IR execution (mirrors the v0.1 numeric semantics) ----
typedef struct { double *data; size_t n, rows, cols; int rank; } Array;
static Array *vals=NULL;
static int res(int id){ while(nodes[id].op==IR_BIND) id=nodes[id].lhs; return id; }
static void run_ir(void){
    vals=calloc(nnodes?nnodes:1,sizeof(Array));
    if(!vals){perror("calloc");exit(2);}
    for(size_t i=0;i<nnodes;i++){
        Node *nd=&nodes[i]; Array *out=&vals[i];
        if(nd->op==IR_CONST){
            out->n=nd->n; out->data=malloc(nd->n*sizeof(double));
            if(!out->data){perror("malloc");exit(2);}
            memcpy(out->data,nd->data,nd->n*sizeof(double));
            out->rank= nd->type.kind==K_SCALAR?0: nd->type.kind==K_VECTOR?1:2;
            out->rows= nd->type.kind==K_MATRIX?nd->type.d0:1;
            out->cols= nd->type.kind==K_MATRIX?nd->type.d1:nd->type.d0;
            continue;
        }
        if(nd->op==IR_BIND) continue;   // alias only; res() resolves through it
        Array *a=&vals[res(nd->lhs)];
        size_t n=type_count(nd->type);
        out->n=n; out->data=calloc(n,sizeof(double)); if(!out->data){perror("calloc");exit(2);}
        out->rank= nd->type.kind==K_SCALAR?0: nd->type.kind==K_VECTOR?1:2;
        out->rows= nd->type.kind==K_MATRIX?nd->type.d0:1;
        out->cols= nd->type.kind==K_MATRIX?nd->type.d1:nd->type.d0;
        switch(nd->op){
            case IR_ADD: {
                Array *b=&vals[res(nd->rhs)];
                for(size_t j=0;j<n;j++) out->data[j]=a->data[j]+b->data[j];
                break; }
            case IR_SUB: {
                Array *b=&vals[res(nd->rhs)];
                for(size_t j=0;j<n;j++) out->data[j]=a->data[j]-b->data[j];
                break; }
            case IR_MUL: {
                Array *b=&vals[res(nd->rhs)];
                for(size_t j=0;j<n;j++) out->data[j]=a->data[j]*b->data[j];
                break; }
            case IR_MATMUL: {
                Array *b=&vals[res(nd->rhs)];
                size_t m=nd->type.d0, k=nodes[res(nd->lhs)].type.d1, nn=nd->type.d1;
                for(size_t r=0;r<m;r++)
                    for(size_t c=0;c<nn;c++){
                        double s=0;
                        for(size_t z=0;z<k;z++) s+=a->data[r*k+z]*b->data[z*nn+c];
                        out->data[r*nn+c]=s;
                    }
                break; }
            case IR_TRANSPOSE: {
                size_t r0=nodes[nd->lhs].type.d0, c0=nodes[nd->lhs].type.d1;
                for(size_t r=0;r<r0;r++)
                    for(size_t c=0;c<c0;c++) out->data[c*r0+r]=a->data[r*c0+c];
                break; }
            case IR_FLATTEN:
                memcpy(out->data,a->data,a->n*sizeof(double));
                break;
            case IR_SHAPE: {
                Type it=nodes[nd->lhs].type;
                out->data[0]=(double)(it.kind==K_MATRIX?it.d0: it.kind==K_VECTOR?it.d0:1);
                if(it.kind==K_MATRIX) out->data[1]=(double)it.d1;
                break; }
            case IR_SUM: {
                double s=0;
                for(size_t j=0;j<a->n;j++) s+=a->data[j];
                out->data[0]=s;
                break; }
            default: break;
        }
        const char *kname= nd->decl==K_SCALAR?"scalar": nd->decl==K_VECTOR?"vector":"matrix";
        const char *oname= nd->op==IR_ADD?"add": nd->op==IR_SUB?"subtract": nd->op==IR_MUL?"multiply":
                           nd->op==IR_MATMUL?"matmul": nd->op==IR_TRANSPOSE?"transpose":
                           nd->op==IR_FLATTEN?"flatten": nd->op==IR_SHAPE?"shape":"sum";
        printf("%s %s %s => rank=%d shape=",languages[nd->lang].name,kname,oname,out->rank);
        if(out->rank==0)printf("[]"); else if(out->rank==1)printf("[%zu]",out->n); else printf("[%zu,%zu]",out->rows,out->cols);
        printf(" values=[");
        for(size_t j=0;j<out->n;j++)printf("%s%.12g",j?",":"",out->data[j]);
        puts("]");
    }
}

int main(int argc,char **argv){
    int emit=0; const char *file=NULL;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--emit-ir"))emit=1;
        else if(!file)file=argv[i];
        else { fprintf(stderr,"usage: ramc [--emit-ir] file.ram\n"); return 2; }
    }
    if(!file){ fprintf(stderr,"usage: ramc [--emit-ir] file.ram\n"); return 2; }
    FILE *f=fopen(file,"r"); if(!f){perror(file);return 2;}
    char line[MAX_LINE]; int errors=0;
    while(fgets(line,sizeof(line),f)){
        cur_line++;
        if(!strchr(line,'\n')&&!feof(f)){ fprintf(stderr,"line %d: line too long\n",cur_line); errors++; int c; while((c=fgetc(f))!='\n'&&c!=EOF); continue; }
        char *tok[MAX_TOKENS]; int n=0; char *p=strtok(line," \t\r\n");
        while(p&&n<MAX_TOKENS){ if(p[0]=='#'){p=NULL;break;} tok[n++]=p; p=strtok(NULL," \t\r\n"); }
        if(p){ fprintf(stderr,"line %d: too many tokens\n",cur_line); errors++; continue; }
        if(n) errors+=lower_line(tok,n);
    }
    if(ferror(f)){perror("read");errors++;} fclose(f);
    if(emit) emit_ir(); else if(!errors) run_ir();
    return errors?1:0;
}
