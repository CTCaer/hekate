NATIVE_CC ?= gcc

.PHONY: all clean

all: bin2c
	@echo > /dev/null

clean:
	rm -f bin2c

bin2c: bin2c.c
	@$(NATIVE_CC) -o $@ bin2c.c
