ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

TARGET := module_sample
BUILD := ../../build_ipl
OUTPUT := ../../output

OBJS = $(addprefix $(BUILD)/,\
	module_sample.o \
)

LD = $(DEVKITPRO)/devkitARM/bin/arm-none-eabi-ld
ARCH := -march=armv4t -mtune=arm7tdmi -mthumb -mthumb-interwork
CUSTOMDEFINES := -D__arm__ -DARM
CFLAGS = $(ARCH) -O2 -nostdlib  -std=gnu11 -fpie -Wall $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc

.PHONY: clean all 

all: $(TARGET).so
$(BUILD)/%.o: ./%.c
	$(CC) $(CFLAGS) -c $< -o $@
	
$(TARGET).so: $(BUILD)/$(TARGET).o
	$(LD) $(LDLAGS) -pie -entry _pluginInit $(BUILD)/$(TARGET).o -o $(OUTPUT)/$(TARGET).so

clean:
	@rm -rf $(OUTPUT)/$(TARGET).so
