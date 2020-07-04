NATIVE_CC ?= gcc

.PHONY: all clean

all: lz77
	@echo > /dev/null

clean:
	rm -f lz77

lz77: lz.c lz77.c
	@$(NATIVE_CC) -o $@ lz.c lz77.c
