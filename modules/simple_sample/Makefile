ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

TARGET := module_sample
BUILD := ../../build/$(TARGET)
OUTPUT := ../../output
VPATH = $(dir $(wildcard ./*/)) $(dir $(wildcard ./*/*/))

OBJS = $(addprefix $(BUILD)/,\
	module_sample.o \
)

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb -mthumb-interwork
CFLAGS = $(ARCH) -O2 -nostdlib -fpie -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-inline -std=gnu11 -Wall $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections

.PHONY: clean all

all: $(TARGET).bso
$(BUILD)/%.o: ./%.c
	@mkdir -p "$(BUILD)"
	$(CC) $(CFLAGS) -c $< -o $@
	
$(TARGET).bso: $(OBJS)
	$(CC) $(LDLAGS) -pie -e _modInit $^ -o $(OUTPUT)/$(TARGET).bso

clean:
	@rm -rf $(OUTPUT)/$(TARGET).bso
