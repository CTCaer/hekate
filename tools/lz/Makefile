NATIVE_CC ?= gcc

ifeq (, $(shell which $(NATIVE_CC) 2>/dev/null))
$(error "Native GCC is missing. Please install it first. If it's path is custom, set it with export NATIVE_CC=<path to native gcc toolchain>")
endif

.PHONY: all clean

all: lz77
	@echo > /dev/null

clean:
	@rm -f lz77

lz77: lz.c lz77.c
	@$(NATIVE_CC) -o $@ lz.c lz77.c
