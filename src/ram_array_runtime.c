/* ram_array_runtime.c — C11 port of the owner's ram_array_runtime.py.
 *
 * Lowers the registered vector and affine Wolfram subsets to real
 * WebAssembly binaries, byte-identical to the Python generator:
 * BQN/APL/K fixed-length f64 vector add/subtract/multiply execute
 * in a runtime loop over linear memory; the Wolfram affine subset
 * becomes an exported run(x:i64)->i64 in modulo-2^64 arithmetic.
 * Emits .wasm, .wat, .flat.json, and the .skull serialization.
 * The Python original stays in the tree; this is its C11 twin,
 * proven by byte equality of every emitted artifact. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* UTF-8 tokens (byte-exact). */
#define T_WOLF   "\xF0\x9F\x90\xBA"
#define T_SCALAR "\xF0\x9F\x94\xB9"
#define T_LOWER  "\xF0\x9F\x93\x9C"
#define T_VECTOR "\xF0\x9F\x93\x8F"
#define T_BQN    "\xF0\x9F\x8C\x80"
#define T_APL    "\xF0\x9F\x94\xBA"
#define T_K      "\xE2\x9A\xA1"
#define T_ADD    "\xE2\x9E\x95"
#define T_SUB    "\xE2\x9E\x96"
#define T_MUL    "\xE2\x9C\x96\xEF\xB8\x8F"
#define SKULL    "\xF0\x9F\x92\x80"
#define CROWN    "\xF0\x9F\x91\x91"

typedef struct { uint8_t *d; size_t n, cap; } Buf;
static void bput(Buf *b, const void *p, size_t n) {
    if (b->n + n > b->cap) { b->cap = (b->n + n) * 2 + 64; b->d = realloc(b->d, b->cap); if (!b->d) exit(1); }
    memcpy(b->d + b->n, p, n); b->n += n;
}
static void b1(Buf *b, uint8_t v) { bput(b, &v, 1); }
static void bstr(Buf *b, const char *s) { bput(b, s, strlen(s)); }
static void uleb(Buf *b, uint64_t v) { do { uint8_t x = v & 0x7F; v >>= 7; if (v) x |= 0x80; b1(b, x); } while (v); }
static void sleb(Buf *b, int64_t v) {
    for (;;) { uint8_t x = v & 0x7F; v >>= 7;
        if ((v == 0 && !(x & 0x40)) || (v == -1 && (x & 0x40))) { b1(b, x); break; }
        b1(b, x | 0x80); }
}
static void section(Buf *out, uint8_t id, const Buf *payload) {
    b1(out, id); uleb(out, payload->n); bput(out, payload->d, payload->n);
}
static void vec_wrap(Buf *out, const Buf *items) { /* count 1 vector */
    uleb(out, 1); bput(out, items->d, items->n);
}

static void die(const char *m) { fprintf(stderr, "ram_array_runtime: %s\n", m); exit(1); }

/* tokenize: strip #-comments per line, split on ASCII whitespace */
static char **tokenize(char *text, int *count) {
    int cap = 64, n = 0; char **tok = malloc(cap * sizeof(char *));
    for (char *p = text; *p; p++) if (*p == '#') { while (*p && *p != '\n') *p++ = ' '; }
    char *save = NULL;
    for (char *t = strtok_r(text, " \t\r\n", &save); t; t = strtok_r(NULL, " \t\r\n", &save)) {
        if (n == cap) { cap *= 2; tok = realloc(tok, cap * sizeof(char *)); }
        tok[n++] = t;
    }
    *count = n; return tok;
}

static void write_file(const char *path, const void *d, size_t n) {
    FILE *f = fopen(path, "wb"); if (!f) die("cannot write output");
    fwrite(d, 1, n, f); fclose(f);
}
static void out_path(char *dst, const char *out, const char *suffix) {
    strcpy(dst, out);
    char *dot = strrchr(dst, '.'); if (dot) *dot = 0;
    strcat(dst, suffix);
}

int main(int argc, char **argv) {
    const char *src_path = NULL, *out = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) out = argv[++i];
        else if (argv[i][0] != '-') src_path = argv[i];
    }
    if (!src_path || !out) die("usage: ram_array_runtime SOURCE -o OUTPUT");
    FILE *f = fopen(src_path, "rb"); if (!f) die("cannot read source");
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *text = malloc(sz + 1); if (fread(text, 1, sz, f) != (size_t)sz) die("read failed");
    text[sz] = 0; fclose(f);
    int ntok = 0; char **t = tokenize(text, &ntok);

    Buf wasm = {0}, wat = {0}, json = {0};
    { const uint8_t magic[8] = {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00}; bput(&wasm, magic, 8); }

    int is_affine = ntok >= 3 && strcmp(t[0], T_WOLF) == 0 && strcmp(t[1], T_SCALAR) == 0 && strcmp(t[2], T_LOWER) == 0;
    if (is_affine) {
        if (ntok != 6) die("expected three integer coefficients");
        long long a = strtoll(t[3], NULL, 10), b = strtoll(t[4], NULL, 10), c = strtoll(t[5], NULL, 10);
        if (llabs(a) > 1000000 || llabs(b) > 1000000 || llabs(c) > 1000000) die("coefficient out of range");
        long long k = a + b;
        Buf types = {0}, funcs = {0}, exps = {0}, codes = {0}, body = {0}, one = {0};
        bput(&one, "\x60\x01\x7E\x01\x7E", 5); vec_wrap(&types, &one);
        Buf zero = {0}; b1(&zero, 0); vec_wrap(&funcs, &zero);
        Buf e = {0}; uleb(&e, 3); bstr(&e, "run"); b1(&e, 0); uleb(&e, 0); vec_wrap(&exps, &e);
        b1(&body, 0x00); b1(&body, 0x20); b1(&body, 0x00); b1(&body, 0x42); sleb(&body, k);
        b1(&body, 0x7E); b1(&body, 0x42); sleb(&body, c); b1(&body, 0x7C); b1(&body, 0x0B);
        Buf entry = {0}; uleb(&entry, body.n); bput(&entry, body.d, body.n); vec_wrap(&codes, &entry);
        section(&wasm, 1, &types); section(&wasm, 3, &funcs); section(&wasm, 7, &exps); section(&wasm, 10, &codes);
        char buf[512];
        snprintf(buf, sizeof buf,
            "(module\n  (func (export \"run\") (param i64) (result i64)\n"
            "    local.get 0\n    i64.const %lld\n    i64.mul\n    i64.const %lld\n    i64.add)\n)\n", k, c);
        bstr(&wat, buf);
        snprintf(buf, sizeof buf,
            "{\n  \"kind\": \"affine-i64\",\n  \"language\": \"Wolfram-compatible subset\",\n"
            "  \"coefficients\": [\n    %lld,\n    %lld\n  ],\n  \"arithmetic\": \"modulo 2^64\"\n}\n", k, c);
        bstr(&json, buf);
    } else {
        const char *lang = NULL; uint8_t op = 0; const char *opname = NULL;
        if (ntok >= 3) {
            if (strcmp(t[0], T_BQN) == 0) lang = "BQN";
            else if (strcmp(t[0], T_APL) == 0) lang = "APL";
            else if (strcmp(t[0], T_K) == 0) lang = "K";
            if (strcmp(t[2], T_ADD) == 0) { op = 0xA0; opname = "f64.add"; }
            else if (strcmp(t[2], T_SUB) == 0) { op = 0xA1; opname = "f64.sub"; }
            else if (strcmp(t[2], T_MUL) == 0) { op = 0xA2; opname = "f64.mul"; }
        }
        if (!lang || !opname || strcmp(t[1], T_VECTOR) != 0) die("unsupported array lowering");
        long n = strtol(t[3], NULL, 10);
        if (n < 1 || n > 2048 || ntok != 4 + 2 * n) die("invalid vector length or operand count");
        Buf mem = {0};
        for (long i = 0; i < 2 * n; i++) {
            double v = strtod(t[4 + i], NULL);
            if (!isfinite(v)) die("nonfinite input");
            bput(&mem, &v, 8); /* host is little-endian, like struct.pack('<d') */
        }
        Buf code = {0};
        #define INS(...) do { const uint8_t x[] = {__VA_ARGS__}; bput(&code, x, sizeof x); } while (0)
        #define I32(v) do { b1(&code, 0x41); sleb(&code, (v)); } while (0)
        #define GET() INS(0x20, 0x00)
        #define ADDR(base) do { GET(); I32(8); b1(&code, 0x6C); I32(base); b1(&code, 0x6A); } while (0)
        I32(0); INS(0x21, 0x00);
        INS(0x02, 0x40, 0x03, 0x40);
        GET(); I32(n); INS(0x4F, 0x0D, 0x01);
        ADDR(2 * n * 8);
        ADDR(0); INS(0x2B, 0x03, 0x00);
        ADDR(n * 8); INS(0x2B, 0x03, 0x00);
        b1(&code, op); INS(0x39, 0x03, 0x00);
        GET(); I32(1); INS(0x6A, 0x21, 0x00, 0x0C, 0x00, 0x0B, 0x0B, 0x0B);
        Buf body = {0}; bput(&body, "\x01\x01\x7F", 3); bput(&body, code.d, code.n);
        Buf types = {0}, funcs = {0}, mems = {0}, exps = {0}, codes = {0}, data = {0};
        Buf sig = {0}; bput(&sig, "\x60\x00\x00", 3); vec_wrap(&types, &sig);
        Buf zero = {0}; b1(&zero, 0); vec_wrap(&funcs, &zero);
        Buf m = {0}; b1(&m, 0); uleb(&m, 1); vec_wrap(&mems, &m);
        Buf e1 = {0}; uleb(&e1, 6); bstr(&e1, "memory"); b1(&e1, 2); uleb(&e1, 0);
        Buf e2 = {0}; uleb(&e2, 3); bstr(&e2, "run"); b1(&e2, 0); uleb(&e2, 0);
        uleb(&exps, 2); bput(&exps, e1.d, e1.n); bput(&exps, e2.d, e2.n);
        Buf entry = {0}; uleb(&entry, body.n); bput(&entry, body.d, body.n); vec_wrap(&codes, &entry);
        Buf d = {0}; bput(&d, "\x00\x41\x00\x0B", 4); uleb(&d, mem.n); bput(&d, mem.d, mem.n); vec_wrap(&data, &d);
        section(&wasm, 1, &types); section(&wasm, 3, &funcs); section(&wasm, 5, &mems);
        section(&wasm, 7, &exps); section(&wasm, 10, &codes); section(&wasm, 11, &data);
        char buf[1024];
        snprintf(buf, sizeof buf,
            "{\n  \"kind\": \"vector-f64\",\n  \"language\": \"%s\",\n  \"operation\": \"%s\",\n"
            "  \"length\": %ld,\n  \"input_a_offset\": 0,\n  \"input_b_offset\": %ld,\n"
            "  \"output_offset\": %ld,\n  \"result_bytes\": %ld\n}\n",
            lang, opname, n, n * 8, 2 * n * 8, n * 8);
        bstr(&json, buf);
        /* WAT rendering, byte-identical to the Python template. */
        bstr(&wat, "(module\n  (memory (export \"memory\") 1)\n  (func (export \"run\") (local $i i32)\n"
                   "    i32.const 0\n    local.set $i\n    block\n      loop\n"
                   "        local.get $i\n");
        snprintf(buf, sizeof buf, "        i32.const %ld\n        i32.ge_u\n        br_if 1\n", n); bstr(&wat, buf);
        bstr(&wat, "        local.get $i\n        i32.const 8\n        i32.mul\n");
        snprintf(buf, sizeof buf, "        i32.const %ld\n        i32.add\n", 2 * n * 8); bstr(&wat, buf);
        bstr(&wat, "        local.get $i\n        i32.const 8\n        i32.mul\n        f64.load\n"
                   "        local.get $i\n        i32.const 8\n        i32.mul\n");
        snprintf(buf, sizeof buf, "        i32.const %ld\n        i32.add\n        f64.load\n        %s\n        f64.store\n", n * 8, opname); bstr(&wat, buf);
        bstr(&wat, "        local.get $i\n        i32.const 1\n        i32.add\n        local.set $i\n"
                   "        br 0\n      end\n    end)\n  (data (i32.const 0) \"");
        for (size_t i = 0; i < mem.n; i++) { snprintf(buf, sizeof buf, "\\%02x", mem.d[i]); bstr(&wat, buf); }
        bstr(&wat, "\")\n)\n");
    }

    /* .skull: one line, MSB first, 💀=0 👑=1 (his encode()). */
    Buf skull = {0};
    for (size_t i = 0; i < wasm.n; i++)
        for (int bit = 7; bit >= 0; bit--)
            bstr(&skull, (wasm.d[i] >> bit) & 1 ? CROWN : SKULL);
    b1(&skull, '\n');

    char path[1024];
    write_file(out, wasm.d, wasm.n);
    out_path(path, out, ".wat"); write_file(path, wat.d, wat.n);
    out_path(path, out, ".skull"); write_file(path, skull.d, skull.n);
    out_path(path, out, ".flat.json"); write_file(path, json.d, json.n);
    printf("array runtime WASM: %zu bytes (%s)\n", wasm.n, is_affine ? "affine-i64" : "vector-f64");
    return 0;
}
