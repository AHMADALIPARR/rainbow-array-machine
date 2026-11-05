CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: all test test-ir test-flat test-skull test-inv test-geom clean
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
	node tests/array_runtime.js
clean:
	rm -f ram ramc flat skull ram-array-runtime

.PHONY: report
report:
	@echo "RAINBOW ARRAY MACHINE"
	@echo "Architecture: WebAssembly Stack Machine"
	@echo "Source:       UTF-8 Unicode Emoji"
	@echo "Frontend:     BQN / APL / KCL / Wolfram subsets"
	@echo "IR:           Typed FLAT"
	@echo "Compiler:     C11 → WAT → WASM"
	@echo "Execution:    WASM Runtime"
	@echo "Memory:       WebAssembly Linear Memory"
	@echo "Binary:       .wasm"
	@echo "Encoding:     💀👑 .skull"
	@echo "Assertions:   >>>>>>>€"
	@echo "Inverse:      ↶"

.PHONY: wat-test
wat-test:
	python3 src/ram_wat.py examples/wat/vector.ram -o examples/wat/vector.wat
	python3 src/ram_wat.py examples/wat/symbolic.ram -o examples/wat/symbolic.wat
	python3 tests/test_wat.py

.PHONY: geometry-test
geometry-test:
	python3 -m unittest discover -s tests -p 'test_geometry.py' -v
	python3 src/ram_geometry.py examples/geometry/grid.ram -o examples/geometry/grid.wat

.PHONY: runtime-test
runtime-test:
	python3 src/ram_runtime.py examples/geometry/grid.ram -o examples/geometry/grid-runtime.wasm
	node tests/runtime.js examples/geometry/grid-runtime.wasm
	python3 src/ram_runtime.py --decode examples/geometry/grid-runtime.skull -o examples/geometry/grid-roundtrip.wasm
	cmp examples/geometry/grid-runtime.wasm examples/geometry/grid-roundtrip.wasm

.PHONY: array-runtime-test
array-runtime-test:
	python3 src/ram_array_runtime.py examples/runtime-array/vector.ram -o examples/runtime-array/vector.wasm
	python3 src/ram_array_runtime.py examples/runtime-array/apl.ram -o examples/runtime-array/apl.wasm
	python3 src/ram_array_runtime.py examples/runtime-array/k.ram -o examples/runtime-array/k.wasm
	python3 src/ram_array_runtime.py examples/runtime-array/symbolic.ram -o examples/runtime-array/symbolic.wasm
	node tests/array_runtime.js

.PHONY: traceback-test traceback-proof
traceback-test:
	python3 -m unittest discover -s tests -p 'test_traceback.py' -v
	python3 src/ram_traceback.py examples/runtime-array/symbolic.ram examples/runtime-array/symbolic.wasm -o proofs/Symbolic.lean

# Requires an installed Lean/mathlib project; never report success without checking.
traceback-proof: traceback-test
	@command -v lake >/dev/null || { echo 'FAIL: lake/mathlib unavailable'; exit 1; }
	lake env lean proofs/Symbolic.lean

.PHONY: cold-boot
cold-boot:
	./cold-boot.sh
