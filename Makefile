CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: all test test-ir test-flat test-skull test-inv test-geom test-array-c core-test clean
all: ram ramc flat skull ram-array-runtime
ram: src/ram.c
	$(CC) $(CFLAGS) -o $@ $< -lm
ramc: src/ramc.c
	$(CC) $(CFLAGS) -o $@ $< -lm
ram-array-runtime: src/ram_array_runtime.c
	$(CC) $(CFLAGS) -o $@ $< -lm
flat: src/flat.c
	$(CC) $(CFLAGS) -o $@ $<
skull: src/skull.c
	$(CC) $(CFLAGS) -o $@ $<
test: ram
	sh tests/run.sh
test-ir: ram ramc
	sh tests/run-ir.sh
test-flat: ram ramc flat
	sh tests/run-flat.sh
test-skull: skull
	sh tests/run-skull.sh
test-inv: ramc
	sh tests/run-inv.sh
test-geom: ramc
	sh tests/run-geom.sh
test-array-c: ram-array-runtime
	./ram-array-runtime examples/runtime-array/vector.ram -o examples/runtime-array/vector.wasm
	./ram-array-runtime examples/runtime-array/apl.ram -o examples/runtime-array/apl.wasm
	./ram-array-runtime examples/runtime-array/k.ram -o examples/runtime-array/k.wasm
	./ram-array-runtime examples/runtime-array/symbolic.ram -o examples/runtime-array/symbolic.wasm
core-test:
	$(MAKE) -C core test
clean:
	rm -f ram ramc flat skull ram-array-runtime

.PHONY: report
report:
	@echo "RAINBOW ARRAY MACHINE"
	@echo "Architecture: WebAssembly (standalone per-program modules)"
	@echo "Source:       UTF-8 Unicode Emoji"
	@echo "Frontend:     BQN / APL / K / Wolfram subsets (bounded)"
	@echo "Pipeline:     C11 -> RAMIR1 -> FLAT1 -> FLAT2 -> C -> clang wasm32"
	@echo "Core:         core/ (parser, FLAT stages, codegen, kernel)"
	@echo "Execution:    WebAssembly Runtime (assertions execute in-module)"
	@echo "Memory:       WebAssembly Linear Memory"
	@echo "Binary:       .wasm"
	@echo "Encoding:     💀👑 .skull (serialization only)"
	@echo "Assertions:   >>>>>>>€"
	@echo "Inverse:      ↶"

.PHONY: cold-boot
cold-boot:
	./cold-boot.sh
