CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: all test test-ir test-flat clean wat-test
all: ram ramc flat
ram: src/ram.c
	$(CC) $(CFLAGS) -o $@ $< -lm
ramc: src/ramc.c
	$(CC) $(CFLAGS) -o $@ $< -lm
flat: src/flat.c
	$(CC) $(CFLAGS) -o $@ $<
test: ram
	sh tests/run.sh
test-ir: ram ramc
	sh tests/run-ir.sh
test-flat: ram ramc flat
	sh tests/run-flat.sh
clean:
	rm -f ram ramc flat

.PHONY: wat-test
wat-test:
	python3 src/ram_wat.py examples/wat/vector.ram -o examples/wat/vector.wat
	python3 src/ram_wat.py examples/wat/symbolic.ram -o examples/wat/symbolic.wat
	python3 tests/test_wat.py
