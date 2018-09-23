include ../../rules/dkarm_compat

TARGET := module_sample
BUILD := ../../build/$(TARGET)
OUTPUT := ../../output
VPATH = $(dir $(wildcard ./*/)) $(dir $(wildcard ./*/*/))

OBJS = $(addprefix $(BUILD)/,\
	module_sample.o \
	gfx.o \
)

.PHONY: clean all

all: $(TARGET).bso
$(BUILD)/%.o: ./%.c
	@mkdir -p "$(BUILD)"
	$(CC) $(SO_CFLAGS) -c $< -o $@
	
$(TARGET).bso: $(OBJS)
	$(CC) $(SO_LDFLAGS) -e _modInit $^ -o $(OUTPUT)/$(TARGET).bso
	$(STRIP) -g $(OUTPUT)/$(TARGET).bso

clean:
	@rm -rf $(OUTPUT)/$(TARGET).bso
