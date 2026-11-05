CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: all test test-ir clean
all: ram ramc
ram: src/ram.c
	$(CC) $(CFLAGS) -o $@ $< -lm
ramc: src/ramc.c
	$(CC) $(CFLAGS) -o $@ $< -lm
test: ram
	sh tests/run.sh
test-ir: ram ramc
	sh tests/run-ir.sh
clean:
	rm -f ram ramc
