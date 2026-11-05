// ramc — the Rainbow Array Machine compiler front end:
// source -> typed IR -> execution, with the reversible Unicode
// instruction algebra and the geometry layer (owner's design):
//
//   >>>>>>>€ <expected>   assertion delimiter: the preceding
//                         statement's value must equal the expected
//                         literal in shape, kind, and values.
//                         Geometry statements take field assertions
//                         instead: SHAPE / ORIGIN / SPACING / DIM /
//                         COORD [i j k] [x y z]; several distinct
//                         fields may be asserted per statement.
//   ↶ <statement>         apply the registered inverse of the
//                         statement's operation. Exact inverses
//                         (add, multiply, transpose, reverse) need
//                         nothing more; the flatten and reshape
//                         inverses need saved shape metadata — from
//                         the typed IR's provenance (reference a
//                         bound result) or supplied explicitly via
//                         🗳️ — and compilation fails without it.
//                         Reductions and picks discard information
//                         and have no inverse: compilation fails.
//   ⟲ <statement>         one-line round trip: run forward, apply
//                         the inverse, require the original first
//                         operand back.
//   🛡️ ROUNDTRIP          certificate line: the immediately
//                         preceding forward/inverse assertion pair
//                         must form a verified round trip. Scope,
//                         in the owner's words: a passing test
//                         demonstrates the property for the tested
//                         inputs; a universal proof would require
//                         a separate formal verification step.
//   ⚖️                     scalar compare, yields a verdict whose
//                         printed form is 👑 (equal) or 💀 (not).
//
// Geometry layer: BQN (🌀) is the geometry namespace. A block
//   🌀 📐 NAME / DIM / SHAPE / ORIGIN / SPACING
// defines a first-class geometry value (coordinates, spacing,
// topology), distinct from a bare array shape. The KCL bridge
//   ⚡ @@ NAME / TRANSFORM ROTATE_Z 90
// applies geometric transformations through our own @@ operator
// (KCL's @ is annotation syntax; @@ is defined here, not assumed
// to be native KCL). Rotations are quarter turns — multiples of
// 90 degrees — so every coordinate stays exact. BQN axis
// operations ⥊ reshape, ⍉ transpose, ↕ reverse, ⊑ pick, ⊏ select
// are the executable geometry-flavored array operations.
//
// Operands: dimension-headed literal lists (v0.1 grammar),
// @references to bound results, and bracket literals — [1 2 3],
// [[1 2] [3 4]] — which carry their own shape. ram.c is
// untouched; ramc's computed output on v0.1 programs is
// byte-identical to ram's.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXN 512
#define MAXV 4096
enum { K_SCALAR=0, K_VECTOR=1, K_MATRIX=2, K_VERDICT=3, K_GEOM=4 };
typedef struct { int kind, rank, dims[3]; } Type;
typedef struct {
    int dim; double shape[3], origin[3], spacing[3];
    double spacing_def[3], rot[9]; int ntf;
} Geom;
typedef struct { int is_ref, ref, self_shaped; Type type; double vals[MAXV]; int n; } Operand;
typedef struct { int has, field; Type type; double vals[MAXV]; int n; double idx[3]; } Expected;
// field: 0=value 1=shape 2=origin 3=spacing 4=coord 5=dim
typedef struct {
    int lang; char lang_name[16]; Type type; char op[28]; char decl[16];
    Operand a, b; int has_a, has_b, line, inv, kind;  // kind: 0 normal, 1 roundtrip-cert, 2 roundtrip-line
    Type pre_type;                                    // flatten/reshape: the input's type
    Expected asserts[8]; int nasserts;
    Geom geom; int geom_ref;                          // grid definition / bridge source
    char tf_kind[8][16]; double tf_deg[8];            // bridge transform list
} Node;
static Node nodes[MAXN]; static int nn=0;
typedef struct { char name[64]; int node; } Binding;
static Binding binds[MAXN]; static int nb=0;
static Binding geoms_tab[MAXN]; static int ng=0;
static const struct { const char *emoji, *name; } langs[]={
    {"🌀","bqn"},{"🔺","apl"},{"⚡","k"},{"🧩","j"},{"🪐","uiua"},{"🏛️","pascal"},
    {"⚙️","c"},{"🦀","rust"},{"🧱","forth"},{"🔌","systemverilog"},{"🐦","unlambda"},
    {"λ","haskell"},{"🕸️","prolog"},{"🐺","wolfram"}};
static const struct { const char *emoji, *name; int arity; } ops[]={
    {"➕","add",2},{"➖","subtract",2},{"✖️","multiply",2},{"🧲","matmul",3},
    {"🔄","transpose",1},{"📦","flatten",1},{"📐","shape",1},{"🧮","sum",1},
    {"⚖️","compare",2},{"🗳️","reconstruct",3},
    {"⥊","reshape",3},{"⍉","transpose",1},{"↕","reverse",1},{"⊑","pick",3},{"⊏","select",3}};
static const char *type_names[]={"scalar","vector","matrix","verdict","geometry"};
static void die(int line,const char *msg){ fprintf(stderr,"ramc: line %d: %s\n",line,msg); exit(1); }
static int lookup(const char *tok,int which){
    if(which==0){ for(size_t i=0;i<sizeof(langs)/sizeof(*langs);i++) if(!strcmp(tok,langs[i].emoji)) return (int)i; }
    else { for(size_t i=0;i<sizeof(ops)/sizeof(*ops);i++) if(!strcmp(tok,ops[i].emoji)) return (int)i; }
    return -1;
}
static int type_count(const Type *t){ int n=1; if(t->kind==K_VERDICT||t->kind==K_GEOM) return 1; for(int i=0;i<t->rank;i++) n*=t->dims[i]; return t->rank?n:1; }
static int same_type(const Type *x,const Type *y){
    if(x->kind!=y->kind||x->rank!=y->rank) return 0;
    for(int i=0;i<x->rank;i++) if(x->dims[i]!=y->dims[i]) return 0; return 1;
}
static int find_bind(const char *name){ for(int i=0;i<nb;i++) if(!strcmp(binds[i].name,name)) return binds[i].node; return -1; }
static int find_geom(const char *name){ for(int i=0;i<ng;i++) if(!strcmp(geoms_tab[i].name,name)) return geoms_tab[i].node; return -1; }
static const char *bind_name_of(int node){ for(int i=0;i<nb;i++) if(binds[i].node==node) return binds[i].name; return "?"; }
static const char *lang_disp(Node *nd){ return nd->lang_name[0]?nd->lang_name:langs[nd->lang].name; }

// ---- bracket literals: recursive descent over a joined string ----
typedef struct { const char *s; double vals[MAXV]; int n; } BParse;
static void bp_ws(BParse *b){ while(*b->s==' '||*b->s=='\t') b->s++; }
static int bp_level(BParse *b,Type *t){
    bp_ws(b);
    if(*b->s!='[') return 0;
    b->s++; bp_ws(b);
    if(*b->s=='['){
        Type child; int count=0;
        while(*b->s=='['){
            Type ct; if(!bp_level(b,&ct)) return 0;
            if(count&&!same_type(&ct,&child)) return 0;
            child=ct; count++; bp_ws(b);
        }
        if(*b->s!=']') return 0;
        b->s++;
        if(child.rank>=2) return 0;
        t->rank=child.rank+1; t->dims[0]=count;
        for(int i=0;i<child.rank;i++) t->dims[i+1]=child.dims[i];
        t->kind= t->rank==1?K_VECTOR:K_MATRIX;
        return 1;
    }
    int count=0;
    for(;;){
        bp_ws(b);
        if(*b->s==']'){ b->s++; break; }
        char *end; double v=strtod(b->s,&end);
        if(end==b->s||!isfinite(v)) return 0;
        if(b->n>=MAXV) return 0;
        b->vals[b->n++]=v; count++; b->s=end;
    }
    if(!count) return 0;
    t->rank=1; t->dims[0]=count; t->kind=K_VECTOR;
    return 1;
}
static int parse_bracket(char **tok,int ntok,int i,Operand *o,int line){
    static char buf[16384]; buf[0]=0;
    int depth=0, used=0;
    for(int j=i;j<ntok;j++){
        for(const char *c=tok[j];*c;c++){ if(*c=='[')depth++; else if(*c==']')depth--; }
        strcat(buf,tok[j]); strcat(buf," "); used++;
        if(depth==0) break;
    }
    if(depth!=0||used==0) die(line,"unterminated bracket literal");
    BParse b={buf,{0},0};
    if(!bp_level(&b,&o->type)) die(line,"malformed bracket literal (rank at most 2, rectangular, finite)");
    bp_ws(&b); if(*b.s) die(line,"trailing characters after bracket literal");
    o->n=b.n; memcpy(o->vals,b.vals,sizeof(double)*(size_t)b.n);
    o->is_ref=0; o->ref=-1; o->self_shaped=1;
    if(o->n!=type_count(&o->type)) die(line,"bracket literal element count does not match its shape");
    return used;
}
static long parse_dim(const char *s,int line){
    char *end; long v=strtol(s,&end,10);
    if(*end||v<1||v>MAXV) die(line,"bad dimension");
    return v;
}
static double parse_val(const char *s,int line){
    char *end; double v=strtod(s,&end);
    if(*end||!isfinite(v)) die(line,"bad numeric value");
    return v;
}
// Parse one operand beginning at token i. hdr_dims: -1 = shape from
// the operand itself (reference or bracket); >=0 = literal header
// with that many dimension tokens. Returns tokens consumed.
static int parse_operand(char **tok,int ntok,int i,int hdr_dims,Operand *o,int line){
    if(i>=ntok) die(line,"missing operand");
    memset(o,0,sizeof *o);
    if(tok[i][0]=='@'){
        int src=find_bind(tok[i]+1);
        if(src<0) die(line,"unbound reference");
        o->is_ref=1; o->ref=src; o->self_shaped=1; o->type=nodes[src].type; o->n=type_count(&o->type);
        return 1;
    }
    if(tok[i][0]=='[') return parse_bracket(tok,ntok,i,o,line);
    if(hdr_dims<0) die(line,"operand needs a shape (reference, bracket, or header)");
    long d[2]={1,1};
    for(int k=0;k<hdr_dims;k++){
        if(i+k>=ntok) die(line,"missing dimension");
        d[k]=parse_dim(tok[i+k],line);
    }
    int count= hdr_dims==0?1:(int)(hdr_dims==1?d[0]:d[0]*d[1]);
    if(count>MAXV) die(line,"operand exceeds 4096 elements");
    if(i+hdr_dims+count>ntok) die(line,"not enough values for operand");
    for(int k=0;k<count;k++) o->vals[k]=parse_val(tok[i+hdr_dims+k],line);
    o->n=count;
    o->type.kind= hdr_dims==0?K_SCALAR:hdr_dims==1?K_VECTOR:K_MATRIX;
    o->type.rank=hdr_dims;
    o->type.dims[0]=(int)d[0]; o->type.dims[1]=(int)d[1];
    return hdr_dims+count;
}
static void add_assert(Node *nd,Expected *e,int line){
    for(int k=0;k<nd->nasserts;k++)
        if(nd->asserts[k].field==e->field) die(line,"duplicate assertion (same field) for one statement");
    if(nd->nasserts>=8) die(line,"too many assertions for one statement");
    nd->asserts[nd->nasserts++]=*e;
}
static void parse_expected(char **tok,int ntok,int i,Expected *e,int line){
    if(i>=ntok) die(line,"assertion without an expected value");
    memset(e,0,sizeof *e); e->has=1; e->field=0;
    static const struct { const char *word; int field; } fields[]={
        {"SHAPE",1},{"ORIGIN",2},{"SPACING",3},{"COORD",4},{"DIM",5}};
    for(size_t k=0;k<sizeof(fields)/sizeof(*fields);k++){
        if(!strcmp(tok[i],fields[k].word)){
            e->field=fields[k].field;
            if(e->field==5){
                if(ntok!=i+2) die(line,"DIM assertion: >>>>>>>€ DIM n");
                char *end; long v=strtol(tok[i+1],&end,10);
                if(*end||v<1||v>3) die(line,"bad DIM value");
                e->n=1; e->vals[0]=(double)v; return;
            }
            if(e->field==4){
                Operand oi,ov;
                int u1=parse_bracket(tok,ntok,i+1,&oi,line);
                int u2=parse_bracket(tok,ntok,i+1+u1,&ov,line);
                if(i+1+u1+u2!=ntok) die(line,"trailing tokens after COORD assertion");
                if(oi.n<1||oi.n>3||ov.n!=oi.n) die(line,"COORD assertion: index and point must share a length 1..3");
                for(int j=0;j<oi.n;j++) e->idx[j]=oi.vals[j];
                e->n=ov.n; memcpy(e->vals,ov.vals,sizeof(double)*(size_t)ov.n);
                return;
            }
            Operand o; int used=parse_bracket(tok,ntok,i+1,&o,line);
            if(i+1+used!=ntok) die(line,"trailing tokens after field assertion");
            e->n=o.n; memcpy(e->vals,o.vals,sizeof(double)*(size_t)o.n);
            return;
        }
    }
    if(!strcmp(tok[i],"👑")||!strcmp(tok[i],"💀")){
        if(ntok!=i+1) die(line,"trailing tokens after verdict expectation");
        e->type.kind=K_VERDICT; e->type.rank=0; e->n=1;
        e->vals[0]= !strcmp(tok[i],"👑")?1.0:0.0;
        return;
    }
    if(tok[i][0]!='[') die(line,"expected value must be a bracket literal, a verdict, or a geometry field");
    Operand o; int used=parse_bracket(tok,ntok,i,&o,line);
    if(i+used!=ntok) die(line,"trailing tokens after expected value");
    e->type=o.type; e->n=o.n; memcpy(e->vals,o.vals,sizeof(double)*(size_t)o.n);
}
static int chase(int idx){ while(!strcmp(nodes[idx].op,"bind")) idx=nodes[idx].a.ref; return idx; }
static int inverse_registered(const char *oname){
    return !strcmp(oname,"add")||!strcmp(oname,"multiply")||!strcmp(oname,"transpose")
        ||!strcmp(oname,"flatten")||!strcmp(oname,"reshape")||!strcmp(oname,"reverse");
}
static void validate_grid(Node *nd,int line){
    Geom *g=&nd->geom;
    if(g->dim<1||g->dim>3) die(line,"GRID block needs DIM 1..3");
    int sc= nd->has_b? nd->b.n : 0;   // shape bracket parked in operand b during the block
    if(!sc) die(line,"GRID block needs a SHAPE");
    if(sc!=g->dim) die(line,"SHAPE length must equal DIM");
    for(int k=0;k<g->dim;k++) g->shape[k]=nd->b.vals[k];
    if(nd->has_a){ if(nd->a.n!=g->dim) die(line,"ORIGIN/SPACING length must equal DIM"); }
    g->rot[0]=g->rot[4]=g->rot[8]=1.0;
    for(int k=0;k<g->dim;k++) g->spacing_def[k]=g->spacing[k];
    nd->has_a=nd->has_b=0;
}
