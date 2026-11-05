# Rainbow Array Machine — standalone WASM execution core.
# Integration Makefile for the shipped C11 pipeline (ramc -> flat_check
# -> flat_lower -> ram_codegen -> clang wasm32). The test target runs
# the FLAT validation, executes every example module in a WebAssembly
# runtime (Node, used as a runtime only — no JavaScript ships in this
# tree), the compiler negative cases, and a skull round trip through
# the repo's C codec (src/skull.c).
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
CLANG ?= clang
MODULES = array geometry symbolic showcase polynomial

.PHONY: all test flat-test run-check negatives skull-test modules clean
all: build/ramc build/flat_check build/flat_lower build/ram_codegen

build:
	mkdir -p build

build/ramc: src/ramc.c | build
	$(CC) $(CFLAGS) $< -o $@
build/flat_check: src/flat_check.c | build
	$(CC) $(CFLAGS) $< -lm -o $@
build/flat_lower: src/flat_lower.c | build
	$(CC) $(CFLAGS) $< -o $@
build/ram_codegen: src/ram_codegen.c | build
	$(CC) $(CFLAGS) $< -o $@

build/%.ir: examples/%.ram build/ramc | build
	build/ramc $< $@
build/%.flat: build/%.ir build/flat_check | build
	build/flat_check $< $@
build/%.flat2: build/%.ir build/%.flat build/flat_lower | build
	build/flat_lower build/$*.ir build/$*.flat $@
build/%.c: build/%.flat2 build/ram_codegen | build
	build/ram_codegen $< $@
build/%.wasm: build/%.c src/kernel.c | build
	$(CLANG) --target=wasm32 -O2 -nostdlib -Wl,--no-entry -Wl,--export-memory -Wl,--initial-memory=1048576 build/$*.c src/kernel.c -o $@

modules: $(MODULES:%=build/%.wasm)

flat-test: $(MODULES:%=build/%.flat)
	grep -q 'OP 0 AFFINE' build/symbolic.flat
	grep -q 'OP 0 POLY' build/polynomial.flat
	grep -q 'canonical deg=2 coeffs=5 5 -1' build/polynomial.ir.cert
	grep -q 'OP 4 TRANSPOSE' build/array.flat
	@echo 'PASS typed FLAT validation'

run-check: modules
	sh tests/run-modules.sh

negatives: build/ramc build/ram_codegen build/showcase.flat2
	@printf '🚀 📏 ➕ 1 2\n' > build/bad1.ram; \
	if build/ramc build/bad1.ram build/bad1.ir 2>/dev/null; then echo 'FAIL: accepted unknown namespace'; exit 1; fi
	@printf '🌀 📏 ➕ 3 1 2\n' > build/bad2.ram; \
	if build/ramc build/bad2.ram build/bad2.ir 2>/dev/null; then echo 'FAIL: accepted missing operand'; exit 1; fi
	@printf '🐺 🔹 📜 2 3 x 4\n' > build/bad3.ram; \
	if build/ramc build/bad3.ram build/bad3.ir 2>/dev/null; then echo 'FAIL: accepted invalid numeric'; exit 1; fi
	@grep -v '^AFFINE' build/showcase.flat2 > build/bad4.flat2; \
	if build/ram_codegen build/bad4.flat2 build/bad4.c 2>/dev/null; then echo 'FAIL: accepted FLAT2 with missing payload'; exit 1; fi
	@echo 'PASS compiler negatives (unknown namespace, missing operand, invalid numeric, FLAT2 payload)'

skull-test: build/array.wasm
	$(CC) $(CFLAGS) ../src/skull.c -o build/skull
	build/skull encode build/array.wasm build/array.skull
	build/skull decode build/array.skull build/array.roundtrip.wasm
	cmp build/array.wasm build/array.roundtrip.wasm
	@echo 'PASS skull/crown round trip (repo C codec)'

test: flat-test run-check negatives skull-test

clean:
	rm -rf build
