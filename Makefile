CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: all test clean
all: ram
ram: src/ram.c
	$(CC) $(CFLAGS) -o $@ $< -lm
test: ram
	sh tests/run.sh
clean:
	rm -f ram
