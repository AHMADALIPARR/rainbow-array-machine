// ramc — the Rainbow Array Machine compiler front end:
// source -> typed IR -> execution, with the reversible Unicode
// instruction algebra and the geometry layer (owner's designs).
//
//   >>>>>>>€ <expected>   assertion delimiter: the preceding
//                         statement's value must equal the expected
//                         literal in shape, kind, and values.
//                         Geometry statements take field assertions
//                         (SHAPE, POINT) in the owner's v0.4 forms.
//   ↶ <statement>         apply the registered inverse of the
//                         statement's operation. Exact inverses
//                         (add, multiply, transpose, reverse, and
//                         the geometry bridge transforms) need
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
// Geometry layer (semantics = the owner's v0.4 artifact,
// src/ram_geometry.py): geometry is a first-class type. A grid is
// defined on one line — 🌀 📐 GRID name nx ny nz spacing — as an
// integer point lattice (x-fastest, origin at zero). The KCL
// bridge ⚡ @@ name OP applies ROTATE_Z (quarter turns, exact) or
// TRANSLATE to the named grid's points; ↶ inverts it. @@ is RAM
// syntax, not a native KCL operator. Shape is the definition
// shape; the points carry the geometry. BQN axis operations
// ⥊ reshape, ⍉ transpose, ↕ reverse, ⊑ pick, ⊏ select are the
// executable geometry-flavored array operations.
//
// Operands: dimension-headed literal lists (v0.1 grammar),
// @references to bound results, and bracket literals — [1 2 3],
// [[1 2] [3 4]] — which carry their own shape. ram.c is
// untouched; ramc's computed output on v0.1 programs is
// byte-identical to ram's.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define MAXN 512
#define MAXV 4096
enum { K_SCALAR=0, K_VECTOR=1, K_MATRIX=2, K_VERDICT=3, K_GEOM=4 };
typedef struct { int kind, rank, dims[3]; } Type;
typedef struct {
    int shape[3], spacing, npoints;
    int32_t *points;   // npoints * 3, x-fastest; owned by the node
} Geom;
typedef struct { int is_ref, ref, self_shaped; Type type; double vals[MAXV]; int n; } Operand;
typedef struct { int has, field; Type type; double vals[MAXV]; int n; } Expected;
// field: 0=value 1=shape 2=point (vals = idx, x, y, z)
typedef struct {
    int lang; char lang_name[16]; Type type; char op[28]; char decl[16];
    Operand a, b; int has_a, has_b, line, inv, kind;  // kind: 0 normal, 1 roundtrip-cert, 2 roundtrip-line
    Type pre_type;                                    // flatten/reshape: the input's type
    Expected asserts[8]; int nasserts;
    Geom geom; int geom_src;                          // geometry value / bridge source node
    char tf_op[16]; long tf_args[3];                  // bridge transform
    char gname[64];                                   // geometry name (grid / bridge target)
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
    for(int i=0;i<x->rank;i++){ if(x->dims[i]!=y->dims[i]) return 0; }
    return 1;
}
static int find_bind(const char *name){ for(int i=0;i<nb;i++) if(!strcmp(binds[i].name,name)) return binds[i].node; return -1; }
static int find_geom(const char *name){ for(int i=0;i<ng;i++) if(!strcmp(geoms_tab[i].name,name)) return geoms_tab[i].node; return -1; }
static void set_geom(const char *name,int node){
    for(int i=0;i<ng;i++) if(!strcmp(geoms_tab[i].name,name)){ geoms_tab[i].node=node; return; }
    if(ng>=MAXN) return;
    strcpy(geoms_tab[ng].name,name); geoms_tab[ng].node=node; ng++;
}
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
        Type child={0,0,{0,0,0}}; int count=0;
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
static long parse_gint(const char *s,int line){   // geometry integers: the v0.4 bound
    char *end; long v=strtol(s,&end,10);
    if(*end||!s[0]||v<-100000||v>100000) die(line,"geometry integer out of range [-100000,100000]");
    return v;
}
static int valid_name(const char *s){
    if(!( (s[0]>='A'&&s[0]<='Z')||(s[0]>='a'&&s[0]<='z')||s[0]=='_')) return 0;
    for(int i=1;s[i];i++){ char c=s[i]; if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_')) return 0; }
    return 1;
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
    // one value assertion and one SHAPE per statement; POINT
    // assertions may repeat (each names a different lattice index)
    if(e->field!=2)
        for(int k=0;k<nd->nasserts;k++)
            if(nd->asserts[k].field==e->field) die(line,"duplicate assertion (same field) for one statement");
    if(nd->nasserts>=8) die(line,"too many assertions for one statement");
    nd->asserts[nd->nasserts++]=*e;
}
static void parse_expected(char **tok,int ntok,int i,Expected *e,int line){
    if(i>=ntok) die(line,"assertion without an expected value");
    memset(e,0,sizeof *e); e->has=1; e->field=0;
    if(!strcmp(tok[i],"SHAPE")){
        e->field=1;
        if(i+1<ntok&&tok[i+1][0]=='['){
            Operand o; int used=parse_bracket(tok,ntok,i+1,&o,line);
            if(i+1+used!=ntok) die(line,"trailing tokens after SHAPE assertion");
            e->n=o.n; memcpy(e->vals,o.vals,sizeof(double)*(size_t)o.n);
        } else {
            if(ntok-(i+1)<1||ntok-(i+1)>3) die(line,"SHAPE assertion: 1..3 integers");
            e->n=ntok-(i+1);
            for(int k=0;k<e->n;k++) e->vals[k]=(double)parse_gint(tok[i+1+k],line);
        }
        return;
    }
    if(!strcmp(tok[i],"POINT")){
        e->field=2;
        if(ntok!=i+5) die(line,"POINT assertion: >>>>>>>€ POINT idx x y z");
        for(int k=0;k<4;k++) e->vals[k]=(double)parse_gint(tok[i+1+k],line);
        e->n=4;
        return;
    }
    if(!strcmp(tok[i],"👑")||!strcmp(tok[i],"💀")){
        if(ntok!=i+1) die(line,"trailing tokens after verdict expectation");
        e->type.kind=K_VERDICT; e->type.rank=0; e->n=1;
        e->vals[0]= !strcmp(tok[i],"👑")?1.0:0.0;
        return;
    }
    if(tok[i][0]!='[') die(line,"expected value must be a bracket literal, a verdict, SHAPE, or POINT");
    Operand o; int used=parse_bracket(tok,ntok,i,&o,line);
    if(i+used!=ntok) die(line,"trailing tokens after expected value");
    e->type=o.type; e->n=o.n; memcpy(e->vals,o.vals,sizeof(double)*(size_t)o.n);
}
static int chase(int idx){ while(!strcmp(nodes[idx].op,"bind")) idx=nodes[idx].a.ref; return idx; }
static int inverse_registered(const char *oname){
    return !strcmp(oname,"add")||!strcmp(oname,"multiply")||!strcmp(oname,"transpose")
        ||!strcmp(oname,"flatten")||!strcmp(oname,"reshape")||!strcmp(oname,"reverse");
}

static Node *new_node(int line){
    if(nn>=MAXN) die(line,"too many statements");
    Node *nd=&nodes[nn]; memset(nd,0,sizeof *nd); nd->line=line; nd->geom_src=-1;
    return nd;
}
// Geometry bridge statement: [↶] ⚡ @@ name OP args...
static void build_bridge(char **tok,int ntok,int inv,int line){
    int base= inv?1:0;
    if(ntok<base+4) die(line,"bridge syntax: ⚡ @@ name ROTATE_Z deg | TRANSLATE dx dy dz");
    const char *name=tok[base+2];
    int src=find_geom(name);
    if(src<0) die(line,"unknown geometry in @@");
    Node *nd=new_node(line);
    nd->lang=2; strcpy(nd->lang_name,"kcl");
    strcpy(nd->op,"bridge"); nd->inv=inv; nd->geom_src=src;
    snprintf(nd->gname,sizeof nd->gname,"%s",name);
    nd->type=(Type){K_GEOM,0,{0,0,0}}; strcpy(nd->decl,"geometry");
    const char *op=tok[base+3];
    if(!strcmp(op,"ROTATE_Z")){
        if(ntok!=base+5) die(line,"ROTATE_Z takes one angle");
        long deg=parse_gint(tok[base+4],line);
        if(deg%90) die(line,"only exact 90-degree rotations supported");
        strcpy(nd->tf_op,"ROTATE_Z"); nd->tf_args[0]=deg;
    } else if(!strcmp(op,"TRANSLATE")){
        if(ntok!=base+7) die(line,"TRANSLATE takes dx dy dz");
        strcpy(nd->tf_op,"TRANSLATE");
        for(int k=0;k<3;k++) nd->tf_args[k]=parse_gint(tok[base+4+k],line);
    } else die(line,"unsupported @@ operation");
    nn++;
    set_geom(name,nn-1);
}
static void build_grid(char **tok,int ntok,int line){
    if(ntok!=8||strcmp(tok[2],"GRID")) die(line,"grid syntax: 🌀 📐 GRID name nx ny nz spacing");
    const char *name=tok[3];
    if(!valid_name(name)) die(line,"invalid grid name");
    if(find_geom(name)>=0) die(line,"duplicate grid name");
    long nx=parse_gint(tok[4],line), ny=parse_gint(tok[5],line), nz=parse_gint(tok[6],line), sp=parse_gint(tok[7],line);
    if(nx<1||ny<1||nz<1||sp<1) die(line,"grid dimensions and spacing must be positive");
    if(nx*ny*nz>4096) die(line,"grid exceeds 4096 points");
    Node *nd=new_node(line);
    nd->lang=0; strcpy(nd->op,"grid");
    snprintf(nd->gname,sizeof nd->gname,"%s",name);
    nd->type=(Type){K_GEOM,0,{0,0,0}}; strcpy(nd->decl,"geometry");
    Geom *g=&nd->geom;
    g->shape[0]=(int)nx; g->shape[1]=(int)ny; g->shape[2]=(int)nz;
    g->spacing=(int)sp; g->npoints=(int)(nx*ny*nz);
    g->points=malloc(sizeof(int32_t)*3*(size_t)g->npoints);
    if(!g->points) die(line,"out of memory");
    int idx=0;
    for(long z=0;z<nz;z++) for(long y=0;y<ny;y++) for(long x=0;x<nx;x++){
        g->points[idx++]=(int32_t)(x*sp); g->points[idx++]=(int32_t)(y*sp); g->points[idx++]=(int32_t)(z*sp);
    }
    nn++;
    set_geom(name,nn-1);
}
static void build_ir(FILE *f){
    char line_buf[65536]; int lineno=0;
    while(fgets(line_buf,sizeof line_buf,f)){
        lineno++;
        char *hash=strchr(line_buf,'#'); if(hash)*hash=0;
        char *tok[4096]; int ntok=0;
        for(char *t=strtok(line_buf," \t\r\n");t&&ntok<4096;t=strtok(NULL," \t\r\n")) tok[ntok++]=t;
        if(!ntok) continue;
        if(!strcmp(tok[0],">>>>>>>€")){
            if(!nn) die(lineno,"assertion without a preceding statement");
            Node *prev=&nodes[nn-1];
            if(prev->kind==1) die(lineno,"assertion after a certificate line");
            Expected e; parse_expected(tok,ntok,1,&e,lineno);
            add_assert(prev,&e,lineno);
            continue;
        }
        if(ntok==2&&!strcmp(tok[0],"🛡️")&&!strcmp(tok[1],"ROUNDTRIP")){
            Node *nd=new_node(lineno);
            nd->kind=1; strcpy(nd->op,"roundtrip_cert");
            nn++; continue;
        }
        if(ntok>=3&&!strcmp(tok[0],"🌀")&&!strcmp(tok[1],"📐")){ build_grid(tok,ntok,lineno); continue; }
        if(ntok>=2&&!strcmp(tok[0],"⚡")&&!strcmp(tok[1],"@@")){ build_bridge(tok,ntok,0,lineno); continue; }
        if(ntok>=3&&!strcmp(tok[0],"↶")&&!strcmp(tok[1],"⚡")&&ntok>=3&&!strcmp(tok[2],"@@")){ build_bridge(tok,ntok,1,lineno); continue; }
        int inv=0, rt=0, ti=0;
        if(!strcmp(tok[0],"↶")){ inv=1; ti=1; }
        else if(!strcmp(tok[0],"⟲")){ rt=1; ti=1; }
        char bindname[64]; int has_bind=0;
        if(ntok-ti>=2&&!strcmp(tok[ti+1],"←")){
            if(strlen(tok[ti])>=sizeof bindname) die(lineno,"binding name too long");
            strcpy(bindname,tok[ti]); has_bind=1; ti+=2;
        }
        if(ntok-ti<3) die(lineno,"expected LANG TYPE OP ...");
        int li=lookup(tok[ti],0); if(li<0) die(lineno,"unknown language namespace");
        if(li==13) die(lineno,"wolfram namespace belongs to the symbolic adapter in ram");
        if(!(li==0||li==1||li==2)) die(lineno,"namespace registered but unimplemented; failing closed");
        int tk; const char *tn=tok[ti+1];
        if(!strcmp(tn,"🔹")){tk=K_SCALAR;} else if(!strcmp(tn,"📏")){tk=K_VECTOR;}
        else if(!strcmp(tn,"🟦")){tk=K_MATRIX;}
        else if(!strcmp(tn,"🧊")) die(lineno,"tensor type reserved; failing closed");
        else if(!strcmp(tn,"🔱")) die(lineno,"ternary type reserved; failing closed");
        else die(lineno,"unknown type");
        int oi=lookup(tok[ti+2],1); if(oi<0) die(lineno,"unknown operation");
        const char *oname=ops[oi].name;
        if(inv&&!inverse_registered(oname)) die(lineno,"no registered inverse for this operation; compilation fails");
        Node *nd=new_node(lineno);
        nd->lang=li; nd->inv=inv; nd->kind= rt?2:0;
        strcpy(nd->decl, type_names[tk]);
        int hdr= tk==K_SCALAR?0:tk==K_VECTOR?1:2;
        int pos=ti+3;
        if(!strcmp(oname,"matmul")){
            if(tk!=K_MATRIX) die(lineno,"matmul requires the matrix type");
            if(inv) die(lineno,"matmul has no registered inverse");
            if(ntok-pos<3) die(lineno,"matmul needs M K N");
            long M=parse_dim(tok[pos],lineno),K=parse_dim(tok[pos+1],lineno),N=parse_dim(tok[pos+2],lineno);
            pos+=3;
            if(ntok-pos!=(int)(M*K+K*N)) die(lineno,"matmul operand count mismatch");
            nd->a.type=(Type){K_MATRIX,2,{(int)M,(int)K,0}}; nd->a.n=(int)(M*K);
            for(long k=0;k<M*K;k++) nd->a.vals[k]=parse_val(tok[pos+k],lineno);
            pos+=(int)(M*K);
            nd->b.type=(Type){K_MATRIX,2,{(int)K,(int)N,0}}; nd->b.n=(int)(K*N);
            for(long k=0;k<K*N;k++) nd->b.vals[k]=parse_val(tok[pos+k],lineno);
            nd->has_a=nd->has_b=1;
            nd->type=(Type){K_MATRIX,2,{(int)M,(int)N,0}};
            strcpy(nd->op,"matmul");
        } else if(!strcmp(oname,"reconstruct")){
            if(inv) die(lineno,"reconstruct is itself an inverse; do not prefix it");
            if(tk!=K_MATRIX) die(lineno,"reconstruct requires the matrix type");
            if(ntok-pos<2) die(lineno,"reconstruct needs ROWS COLS (the recovery metadata)");
            long R=parse_dim(tok[pos],lineno),C=parse_dim(tok[pos+1],lineno); pos+=2;
            if(R*C>MAXV||ntok-pos!=(int)(R*C)) die(lineno,"reconstruct value count must equal ROWS*COLS");
            nd->a.type=(Type){K_VECTOR,1,{(int)(R*C),0,0}}; nd->a.n=(int)(R*C);
            for(long k=0;k<R*C;k++) nd->a.vals[k]=parse_val(tok[pos+k],lineno);
            nd->has_a=1;
            nd->type=(Type){K_MATRIX,2,{(int)R,(int)C,0}};
            strcpy(nd->op,"reconstruct");
        } else if(!strcmp(oname,"reshape")){
            // forms: ⥊ [shape] data... | ⥊ data [shape]
            Operand shape_o, data_o; int shape_first=0;
            if(pos<ntok&&tok[pos][0]=='['){
                shape_first=1;
                int u=parse_bracket(tok,ntok,pos,&shape_o,lineno); pos+=u;
                if(pos<ntok&&(tok[pos][0]=='@'||tok[pos][0]=='[')){
                    int u2=parse_operand(tok,ntok,pos,-1,&data_o,lineno); pos+=u2;
                } else {
                    int cnt=1; for(int k=0;k<shape_o.n;k++) cnt*=(int)shape_o.vals[k];
                    if(shape_o.type.kind!=K_VECTOR||shape_o.n>2) die(lineno,"reshape shape must be [n] or [r c]");
                    if(cnt>MAXV||pos+cnt!=ntok) die(lineno,"reshape value count must equal the shape product");
                    data_o.type=(Type){K_VECTOR,1,{cnt,0,0}}; data_o.n=cnt;
                    for(int k=0;k<cnt;k++) data_o.vals[k]=parse_val(tok[pos+k],lineno);
                    pos+=cnt;
                }
            } else {
                int u=parse_operand(tok,ntok,pos,-1,&data_o,lineno); pos+=u;
                if(pos>=ntok||tok[pos][0]!='[') die(lineno,"reshape needs a shape bracket");
                int u2=parse_bracket(tok,ntok,pos,&shape_o,lineno); pos+=u2;
            }
            (void)shape_first;
            if(pos!=ntok) die(lineno,"trailing tokens after reshape");
            if(shape_o.type.kind!=K_VECTOR||shape_o.n<1||shape_o.n>2) die(lineno,"reshape shape must be [n] or [r c]");
            int prod=1; for(int k=0;k<shape_o.n;k++){ if(shape_o.vals[k]<1||shape_o.vals[k]!=(int)shape_o.vals[k]) die(lineno,"reshape dimensions must be positive integers"); prod*=(int)shape_o.vals[k]; }
            if(prod!=data_o.n) die(lineno,"reshape: element count must be preserved");
            nd->a=data_o; nd->has_a=1; nd->pre_type=data_o.type;
            if(shape_o.n==1) nd->type=(Type){K_VECTOR,1,{(int)shape_o.vals[0],0,0}};
            else nd->type=(Type){K_MATRIX,2,{(int)shape_o.vals[0],(int)shape_o.vals[1],0}};
            strcpy(nd->op,"reshape");
            if(inv){
                if(!data_o.is_ref) die(lineno,"inverse of reshape requires saved shape metadata: reference a bound reshape result");
                int src=chase(data_o.ref);
                if(strcmp(nodes[src].op,"reshape")) die(lineno,"inverse of reshape applies to a reshape result");
                nd->type=nodes[src].pre_type;
                strcpy(nd->op,"inv_reshape");
            }
        } else if(!strcmp(oname,"pick")){
            if(pos>=ntok) die(lineno,"pick needs an index");
            char *end; long ix=strtol(tok[pos],&end,10);
            if(*end||ix<0) die(lineno,"pick index must be a non-negative integer");
            pos++;
            int used=parse_operand(tok,ntok,pos,hdr,&nd->a,lineno); pos+=used; nd->has_a=1;
            if(pos!=ntok) die(lineno,"trailing tokens after pick");
            nd->b.type=(Type){K_SCALAR,0,{0,0,0}}; nd->b.n=1; nd->b.vals[0]=(double)ix; nd->has_b=1;
            if(nd->a.type.kind==K_VECTOR) nd->type=(Type){K_SCALAR,0,{0,0,0}};
            else if(nd->a.type.kind==K_MATRIX) nd->type=(Type){K_VECTOR,1,{nd->a.type.dims[1],0,0}};
            else die(lineno,"pick requires a vector or matrix");
            strcpy(nd->op,"pick");
        } else if(!strcmp(oname,"select")){
            if(pos>=ntok||tok[pos][0]!='[') die(lineno,"select needs an index bracket");
            int u=parse_bracket(tok,ntok,pos,&nd->b,lineno); pos+=u; nd->has_b=1;
            int used=parse_operand(tok,ntok,pos,hdr,&nd->a,lineno); pos+=used; nd->has_a=1;
            if(pos!=ntok) die(lineno,"trailing tokens after select");
            for(int k=0;k<nd->b.n;k++) if(nd->b.vals[k]<0||nd->b.vals[k]!=(int)nd->b.vals[k]) die(lineno,"select indices must be non-negative integers");
            if(nd->a.type.kind==K_VECTOR) nd->type=(Type){K_VECTOR,1,{nd->b.n,0,0}};
            else if(nd->a.type.kind==K_MATRIX) nd->type=(Type){K_MATRIX,2,{nd->b.n,nd->a.type.dims[1],0}};
            else die(lineno,"select requires a vector or matrix");
            strcpy(nd->op,"select");
        } else if(ops[oi].arity==2){
            int used=parse_operand(tok,ntok,pos,hdr,&nd->a,lineno); pos+=used; nd->has_a=1;
            if(!nd->a.self_shaped&&pos<ntok&&tok[pos][0]!='@'&&tok[pos][0]!='['){
                // v0.1 grammar: one header sizes both literal operands;
                // the second literal is values only, shaped like the first.
                int count=nd->a.n;
                if(pos+count>ntok) die(lineno,"not enough values for second operand");
                nd->b.type=nd->a.type; nd->b.n=count;
                for(int k=0;k<count;k++) nd->b.vals[k]=parse_val(tok[pos+k],lineno);
                pos+=count;
            } else if(pos<ntok&&(tok[pos][0]=='@'||tok[pos][0]=='[')){
                int u2=parse_operand(tok,ntok,pos,-1,&nd->b,lineno); pos+=u2;
            } else if(nd->a.self_shaped){
                int count=type_count(&nd->a.type);
                if(pos+count>ntok) die(lineno,"not enough values for second operand");
                nd->b.type=nd->a.type; nd->b.n=count;
                for(int k=0;k<count;k++) nd->b.vals[k]=parse_val(tok[pos+k],lineno);
                pos+=count;
            } else {
                int u2=parse_operand(tok,ntok,pos,hdr,&nd->b,lineno); pos+=u2;
            }
            nd->has_b=1;
            if(pos!=ntok) die(lineno,"trailing tokens after operands");
            if(!strcmp(oname,"compare")){
                if(nd->a.type.kind!=K_SCALAR||nd->b.type.kind!=K_SCALAR) die(lineno,"compare is scalar-only");
                nd->type=(Type){K_VERDICT,0,{0,0,0}};
                strcpy(nd->op,"compare");
            } else {
                if(!same_type(&nd->a.type,&nd->b.type)) die(lineno,"operand shapes must match exactly");
                nd->type=nd->a.type;
                snprintf(nd->op,sizeof nd->op,"%s%s", inv?"inv_":"", oname);
            }
        } else { // unary
            int used=parse_operand(tok,ntok,pos,hdr,&nd->a,lineno); pos+=used; nd->has_a=1;
            if(pos!=ntok) die(lineno,"trailing tokens after operand");
            if(!strcmp(oname,"transpose")){
                if(nd->a.type.kind!=K_MATRIX) die(lineno,"transpose requires a matrix");
                nd->type=(Type){K_MATRIX,2,{nd->a.type.dims[1],nd->a.type.dims[0],0}};
                strcpy(nd->op, inv?"inv_transpose":"transpose");
            } else if(!strcmp(oname,"reverse")){
                nd->type=nd->a.type;
                strcpy(nd->op, inv?"inv_reverse":"reverse");
            } else if(!strcmp(oname,"flatten")){
                nd->pre_type=nd->a.type;
                int n=type_count(&nd->a.type);
                nd->type=(Type){K_VECTOR,1,{n,0,0}};
                strcpy(nd->op,"flatten");
                if(inv){
                    if(!nd->a.is_ref) die(lineno,"inverse of flatten requires saved shape metadata: reference a bound flatten result, or supply the shape via 🗳️");
                    int src=chase(nd->a.ref);
                    if(strcmp(nodes[src].op,"flatten")) die(lineno,"inverse of flatten applies to a flatten result");
                    nd->type=nodes[src].pre_type;
                    strcpy(nd->op,"inv_flatten");
                }
            } else if(!strcmp(oname,"shape")){
                if(inv) die(lineno,"shape has no registered inverse");
                nd->type=(Type){K_VECTOR,1,{nd->a.type.rank?nd->a.type.rank:1,0,0}};
                strcpy(nd->op,"shape");
            } else if(!strcmp(oname,"sum")){
                nd->type=(Type){K_SCALAR,0,{0,0,0}};
                strcpy(nd->op,"sum");
            } else die(lineno,"unhandled unary operation");
        }
        if(rt&&!inverse_registered(oname)) die(lineno,"round trip requires an operation with a registered inverse");
        if(rt&&!strcmp(oname,"flatten")) die(lineno,"round trip of flatten needs a program-level pair (bind, invert, assert)");
        if(rt&&!strcmp(oname,"reshape")) die(lineno,"round trip of reshape needs a program-level pair (bind, invert, assert)");
        nn++;
        if(has_bind){
            if(find_bind(bindname)>=0) die(lineno,"duplicate binding");
            Node *bd=new_node(lineno);
            strcpy(bd->op,"bind"); bd->lang=li; bd->type=nodes[nn-1].type;
            strcpy(bd->decl, type_names[bd->type.kind]);
            bd->a.is_ref=1; bd->a.ref=nn-1; bd->a.type=bd->type; bd->a.n=type_count(&bd->type); bd->has_a=1;
            strcpy(binds[nb].name,bindname); binds[nb].node=nn; nb++;
            nn++;
        }
    }
}

static void shape_str(const Type *t,char *buf,size_t cap){
    if(t->kind==K_VERDICT||t->kind==K_GEOM||t->rank==0){ snprintf(buf,cap,"[]"); return; }
    if(t->rank==1){ snprintf(buf,cap,"[%d]",t->dims[0]); return; }
    if(t->rank==2){ snprintf(buf,cap,"[%d,%d]",t->dims[0],t->dims[1]); return; }
    snprintf(buf,cap,"[%d,%d,%d]",t->dims[0],t->dims[1],t->dims[2]);
}
static void emit_asserts(Node *nd,int i){
    for(int k=0;k<nd->nasserts;k++){
        Expected *e=&nd->asserts[k];
        printf("  assert %%%d ",i);
        if(e->field==1){ printf("SHAPE ["); for(int j=0;j<e->n;j++)printf("%s%g",j?" ":"",e->vals[j]); printf("]"); }
        else if(e->field==2){ printf("POINT %g %g %g %g",(double)e->vals[0],e->vals[1],e->vals[2],e->vals[3]); }
        else if(e->type.kind==K_VERDICT) printf("== %s", e->vals[0]?"👑":"💀");
        else { printf("== ["); for(int j=0;j<e->n;j++)printf("%s%g",j?", ":"",e->vals[j]); printf("]"); }
        printf(" @L%d\n",nd->line);
    }
}
static void emit_ir(void){
    for(int i=0;i<nn;i++){
        Node *nd=&nodes[i]; char sb[64]; shape_str(&nd->type,sb,sizeof sb);
        if(nd->kind==1){ printf("%%cert%d = roundtrip_cert @L%d\n",i,nd->line); continue; }
        if(!strcmp(nd->op,"grid")){
            printf("%%%d = bqn.grid \"%s\" shape=[%d,%d,%d] spacing=%d points=%d @L%d\n",
                   i,nd->gname,nd->geom.shape[0],nd->geom.shape[1],nd->geom.shape[2],nd->geom.spacing,nd->geom.npoints,nd->line);
        } else if(!strcmp(nd->op,"bridge")){
            printf("%%%d = kcl.%sbridge %%%d %s(%ld",i, nd->inv?"inv_":"", nd->geom_src, nd->tf_op, nd->tf_args[0]);
            if(!strcmp(nd->tf_op,"TRANSLATE")) printf(",%ld,%ld",nd->tf_args[1],nd->tf_args[2]);
            printf(") : geometry @L%d\n",nd->line);
        } else if(!strcmp(nd->op,"bind")){
            printf("%%%d = bind \"%s\" %%%d @L%d\n",i,bind_name_of(i),nd->a.ref,nd->line);
        } else if(nd->has_b&&!strcmp(nd->op,"pick")){
            printf("%%%d = %s.pick %%%s, index %g: %s%s @L%d\n",i,lang_disp(nd), nd->a.is_ref?"":"const", (double)nd->b.vals[0],type_names[nd->type.kind],sb,nd->line);
        } else if(nd->has_b){
            printf("%%%d = %s.%s ",i,lang_disp(nd),nd->op);
            if(nd->a.is_ref)printf("%%%d",nd->a.ref); else printf("const %s",type_names[nd->a.type.kind]);
            printf(", ");
            if(nd->b.is_ref)printf("%%%d",nd->b.ref); else printf("const %s",type_names[nd->b.type.kind]);
            printf(": %s%s @L%d\n",type_names[nd->type.kind],sb,nd->line);
        } else {
            printf("%%%d = %s.%s ",i,lang_disp(nd),nd->op);
            if(nd->a.is_ref)printf("%%%d",nd->a.ref); else printf("const %s",type_names[nd->a.type.kind]);
            printf(": %s%s @L%d\n",type_names[nd->type.kind],sb,nd->line);
        }
        if(nd->kind==2) printf("  roundtrip: inverse must restore the first operand @L%d\n",nd->line);
        emit_asserts(nd,i);
    }
}

typedef struct { Type type; double data[MAXV]; int n; } Value;
static Value vals[MAXN];
static Value get_operand(Operand *o){
    if(o->is_ref) return vals[o->ref];
    Value v; v.type=o->type; v.n=o->n; memcpy(v.data,o->vals,sizeof(double)*(size_t)o->n); return v;
}
static int values_equal(const Value *v,const Expected *e){
    if(v->type.kind!=e->type.kind) return 0;
    if(!same_type(&v->type,&e->type)) return 0;
    if(v->n!=e->n) return 0;
    for(int k=0;k<v->n;k++) if(v->data[k]!=e->vals[k]) return 0;
    return 1;
}
static void check_asserts(Node *nd,Value *v){
    for(int k=0;k<nd->nasserts;k++){
        Expected *e=&nd->asserts[k];
        if(nd->type.kind==K_GEOM){
            Geom *g=&nd->geom;
            if(e->field==1){
                if(e->n!=3||(int)e->vals[0]!=g->shape[0]||(int)e->vals[1]!=g->shape[1]||(int)e->vals[2]!=g->shape[2]){
                    fprintf(stderr,"ramc: line %d: SHAPE assertion failed\n",nd->line); exit(1);
                }
            } else if(e->field==2){
                long idx=(long)e->vals[0];
                if(idx<0||idx>=g->npoints){ fprintf(stderr,"ramc: line %d: POINT index out of bounds\n",nd->line); exit(1); }
                if(g->points[idx*3]!=(int32_t)e->vals[1]||g->points[idx*3+1]!=(int32_t)e->vals[2]||g->points[idx*3+2]!=(int32_t)e->vals[3]){
                    fprintf(stderr,"ramc: line %d: POINT assertion failed\n",nd->line); exit(1);
                }
            } else { fprintf(stderr,"ramc: line %d: geometry statements take SHAPE or POINT assertions\n",nd->line); exit(1); }
        } else {
            if(e->field!=0){ fprintf(stderr,"ramc: line %d: field assertion on a non-geometry statement\n",nd->line); exit(1); }
            if(!values_equal(v,e)){ fprintf(stderr,"ramc: line %d: assertion failed\n",nd->line); exit(1); }
        }
        printf("assertion ok @L%d\n",nd->line);
    }
}
static Value apply_op(Node *nd,Value *a,Value *b,int inverse){
    Value r; r.type=nd->type;
    const char *op=nd->op;
    if(inverse&&!strncmp(op,"inv_",4)) op+=4;
    if(!strcmp(op,"add")){ r.n=a->n; for(int k=0;k<a->n;k++) r.data[k]= inverse? a->data[k]-b->data[k] : a->data[k]+b->data[k];
    } else if(!strcmp(op,"subtract")){ r.n=a->n; for(int k=0;k<a->n;k++) r.data[k]=a->data[k]-b->data[k];
    } else if(!strcmp(op,"multiply")){
        r.n=a->n;
        for(int k=0;k<a->n;k++){
            if(inverse){ if(b->data[k]==0) die(nd->line,"inverse multiply: division by zero"); r.data[k]=a->data[k]/b->data[k]; }
            else r.data[k]=a->data[k]*b->data[k];
        }
    } else if(!strcmp(op,"matmul")){
        int M=a->type.dims[0],K=a->type.dims[1],N=b->type.dims[1]; r.n=M*N;
        for(int i=0;i<M;i++)for(int j=0;j<N;j++){ double s=0; for(int k=0;k<K;k++) s+=a->data[i*K+k]*b->data[k*N+j]; r.data[i*N+j]=s; }
    } else if(!strcmp(op,"transpose")||!strcmp(op,"inv_transpose")){
        int R=a->type.dims[0],C=a->type.dims[1]; r.n=a->n;
        for(int i=0;i<R;i++)for(int j=0;j<C;j++) r.data[j*R+i]=a->data[i*C+j];
    } else if(!strcmp(op,"reverse")||!strcmp(op,"inv_reverse")){
        r.n=a->n;
        if(a->type.kind==K_VECTOR){ for(int i=0;i<a->n;i++) r.data[i]=a->data[a->n-1-i]; }
        else { int R=a->type.dims[0],C=a->type.dims[1]; for(int i=0;i<R;i++)for(int j=0;j<C;j++) r.data[i*C+j]=a->data[(R-1-i)*C+j]; }
    } else if(!strcmp(op,"flatten")||!strcmp(op,"inv_flatten")||!strcmp(op,"reconstruct")||!strcmp(op,"reshape")||!strcmp(op,"inv_reshape")){
        r.n=a->n; memcpy(r.data,a->data,sizeof(double)*(size_t)a->n);
    } else if(!strcmp(op,"pick")){
        long ix=(long)b->data[0];
        if(a->type.kind==K_VECTOR){
            if(ix>=a->n) die(nd->line,"pick index out of range");
            r.n=1; r.data[0]=a->data[ix];
        } else {
            int R=a->type.dims[0],C=a->type.dims[1];
            if(ix>=R) die(nd->line,"pick index out of range");
            r.n=C; for(int j=0;j<C;j++) r.data[j]=a->data[ix*C+j];
        }
    } else if(!strcmp(op,"select")){
        r.n=0;
        if(a->type.kind==K_VECTOR){
            for(int k=0;k<b->n;k++){ long ix=(long)b->data[k]; if(ix>=a->n) die(nd->line,"select index out of range"); r.data[r.n++]=a->data[ix]; }
        } else {
            int R=a->type.dims[0],C=a->type.dims[1];
            for(int k=0;k<b->n;k++){ long ix=(long)b->data[k]; if(ix>=R) die(nd->line,"select index out of range"); for(int j=0;j<C;j++) r.data[r.n++]=a->data[ix*C+j]; }
        }
    } else if(!strcmp(op,"shape")){
        if(a->type.kind==K_SCALAR||a->type.rank==0){ r.n=1; r.data[0]=1; }
        else { r.n=a->type.rank; for(int k=0;k<a->type.rank;k++) r.data[k]=a->type.dims[k]; }
    } else if(!strcmp(op,"sum")){ double s=0; for(int k=0;k<a->n;k++) s+=a->data[k]; r.n=1; r.data[0]=s;
    } else if(!strcmp(op,"compare")){ r.type=(Type){K_VERDICT,0,{0,0,0}}; r.n=1; r.data[0]= a->data[0]==b->data[0]?1.0:0.0;
    } else if(!strcmp(op,"bind")){ return *a;
    } else die(nd->line,"internal: unhandled op in evaluator");
    return r;
}
static void print_value(Node *nd,Value *v){
    // Computed results print in ram's v0.1 format exactly, so the
    // two executors stay byte-identical on v0.1 programs.
    char sb[64]; shape_str(&v->type,sb,sizeof sb);
    const char *ln= nd->lang==0?"BQN":nd->lang==1?"APL":nd->lang==2?"K":lang_disp(nd);
    if(v->type.kind==K_VERDICT){ printf("%s %s %s => rank=0 shape=[] values=[%s]\n",ln,nd->decl,nd->op, v->data[0]?"👑":"💀"); return; }
    printf("%s %s %s => rank=%d shape=%s values=[",ln,nd->decl,nd->op,v->type.rank,sb);
    for(int k=0;k<v->n;k++)printf("%s%g",k?",":"",v->data[k]);
    puts("]");
}
static void print_geom(Node *nd){
    Geom *g=&nd->geom;
    if(!strcmp(nd->op,"grid")){
        printf("bqn.grid \"%s\" => type=geometry shape=[%d,%d,%d] spacing=%d points=%d\n",
               nd->gname,g->shape[0],g->shape[1],g->shape[2],g->spacing,g->npoints);
    } else {
        printf("kcl.bridge \"%s\" => type=geometry shape=[%d,%d,%d] points=%d %s%s",
               nd->gname,g->shape[0],g->shape[1],g->shape[2],g->npoints, nd->inv?"inv ":"", nd->tf_op);
        if(!strcmp(nd->tf_op,"ROTATE_Z")) printf(" %ld\n",nd->tf_args[0]);
        else printf(" %ld %ld %ld\n",nd->tf_args[0],nd->tf_args[1],nd->tf_args[2]);
    }
}
static void eval_bridge(Node *nd){
    Geom *src=&nodes[nd->geom_src].geom;
    Geom *g=&nd->geom;
    g->shape[0]=src->shape[0]; g->shape[1]=src->shape[1]; g->shape[2]=src->shape[2];
    g->spacing=src->spacing; g->npoints=src->npoints;
    g->points=malloc(sizeof(int32_t)*3*(size_t)g->npoints);
    if(!g->points) die(nd->line,"out of memory");
    memcpy(g->points,src->points,sizeof(int32_t)*3*(size_t)g->npoints);
    long eff[3]; for(int k=0;k<3;k++) eff[k]= nd->inv? -nd->tf_args[k] : nd->tf_args[k];
    if(!strcmp(nd->tf_op,"ROTATE_Z")){
        long q=((eff[0]/90)%4+4)%4;
        for(long s=0;s<q;s++)
            for(int i=0;i<g->npoints;i++){
                int32_t x=g->points[i*3], y=g->points[i*3+1];
                g->points[i*3]=-y; g->points[i*3+1]=x;
            }
    } else {
        for(int i=0;i<g->npoints;i++)
            for(int k=0;k<3;k++){
                long nv=(long)g->points[i*3+k]+eff[k];
                if(nv<INT32_MIN||nv>INT32_MAX) die(nd->line,"coordinate exceeds i32");
                g->points[i*3+k]=(int32_t)nv;
            }
    }
}
typedef struct { int stmt; char base_op[28]; int is_inv; Value first_operand; int asserted; Expected exp; } StmtInfo;
static StmtInfo hist[MAXN];   // static: a stack array of these would overflow
static void execute(void){
    int nh=0;
    for(int i=0;i<nn;i++){
        Node *nd=&nodes[i];
        if(nd->kind==1){
            if(nh<2) die(nd->line,"ROUNDTRIP certificate without a preceding forward/inverse pair");
            StmtInfo *iv=&hist[nh-1], *fw=&hist[nh-2];
            if(!iv->is_inv||strcmp(iv->base_op,fw->base_op)) die(nd->line,"ROUNDTRIP: preceding pair is not a forward/inverse pair for one operation");
            if(!fw->asserted||!iv->asserted) die(nd->line,"ROUNDTRIP: both statements must carry value assertions");
            Expected fo; memset(&fo,0,sizeof fo); fo.has=1; fo.type=fw->first_operand.type; fo.n=fw->first_operand.n;
            memcpy(fo.vals,fw->first_operand.data,sizeof(double)*(size_t)fo.n);
            Value ivv; ivv.type=iv->exp.type; ivv.n=iv->exp.n; memcpy(ivv.data,iv->exp.vals,sizeof(double)*(size_t)ivv.n);
            if(!values_equal(&ivv,&fo)) die(nd->line,"ROUNDTRIP: inverse assertion does not restore the forward first operand");
            printf("CERT v1: roundtrip; op=%s; status=VERIFIED_ON_TESTED_INPUTS\n",fw->base_op);
            continue;
        }
        if(nd->type.kind==K_GEOM){
            if(!strcmp(nd->op,"bridge")) eval_bridge(nd);
            print_geom(nd);
            Value dummy; dummy.type=nd->type; dummy.n=0;
            check_asserts(nd,&dummy);
            StmtInfo *si=&hist[nh++]; si->stmt=i; si->is_inv=nd->inv;
            snprintf(si->base_op,sizeof si->base_op,"%s",nd->op);
            si->asserted=0; memset(&si->exp,0,sizeof si->exp);
            continue;
        }
        Value a=get_operand(&nd->a);
        Value b= nd->has_b?get_operand(&nd->b):a;
        Value r;
        if(nd->kind==2){
            Value fwd=apply_op(nd,&a,&b,0);
            Value rec= nd->has_b? apply_op(nd,&fwd,&b,1) : apply_op(nd,&fwd,NULL,1);
            if(rec.type.kind!=a.type.kind||!same_type(&rec.type,&a.type)||rec.n!=a.n) die(nd->line,"round trip changed the shape");
            for(int k=0;k<a.n;k++) if(rec.data[k]!=a.data[k]) die(nd->line,"round trip did not restore the first operand");
            r=rec;
        } else if(nd->inv){
            if(nd->has_b) r=apply_op(nd,&a,&b,1);
            else r=apply_op(nd,&a,NULL,0);
        } else {
            r=apply_op(nd,&a, nd->has_b?&b:NULL, 0);
        }
        vals[i]=r; print_value(nd,&r); check_asserts(nd,&r);
        StmtInfo *si=&hist[nh++]; si->stmt=i; si->is_inv=nd->inv;
        const char *base=nd->op; if(nd->inv&&!strncmp(base,"inv_",4)) base+=4;
        snprintf(si->base_op,sizeof si->base_op,"%s",base);
        si->first_operand=a;
        si->asserted=0; memset(&si->exp,0,sizeof si->exp);
        for(int k=0;k<nd->nasserts;k++) if(nd->asserts[k].field==0){ si->asserted=1; si->exp=nd->asserts[k]; }
    }
}
int main(int argc,char **argv){
    if(argc<2||argc>3){ fprintf(stderr,"usage: ramc [--emit-ir] program.ram\n"); return 2; }
    int emit= argc==3&&!strcmp(argv[1],"--emit-ir");
    const char *path= emit?argv[2]:argv[1];
    FILE *f=fopen(path,"r"); if(!f){ perror(path); return 2; }
    build_ir(f); fclose(f);
    if(emit) emit_ir(); else execute();
    return 0;
}
