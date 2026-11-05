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

.PHONY: wat-test
wat-test:
	python3 src/ram_wat.py examples/wat/vector.ram -o examples/wat/vector.wat
	python3 src/ram_wat.py examples/wat/symbolic.ram -o examples/wat/symbolic.wat
	python3 tests/test_wat.py

.PHONY: geometry-test
geometry-test:
	python3 -m unittest discover -s tests -p 'test_geometry.py' -v
	python3 src/ram_geometry.py examples/geometry/grid.ram -o examples/geometry/grid.wat
