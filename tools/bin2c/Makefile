NATIVE_CC ?= gcc

ifeq (, $(shell which $(NATIVE_CC) 2>/dev/null))
$(error "Native GCC is missing. Please install it first. If it's path is custom, set it with export NATIVE_CC=<path to native gcc toolchain>")
endif

.PHONY: all clean

all: bin2c
	@echo > /dev/null

clean:
	@rm -f bin2c

bin2c: bin2c.c
	@$(NATIVE_CC) -o $@ bin2c.c
