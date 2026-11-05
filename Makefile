CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: all test test-ir test-flat clean
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
