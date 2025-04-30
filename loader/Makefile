ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

################################################################################

LDR_LOAD_ADDR := 0x40007000
IPL_MAGIC := 0x43544349 #"ICTC"
include ../Versions.inc

################################################################################

TARGET := loader
BUILDDIR := ../build
OUTPUTDIR := ../output
BDKDIR := bdk
BDKINC := -I../$(BDKDIR)
VPATH += $(dir $(wildcard ../$(BDKDIR)/*/)) $(dir $(wildcard ../$(BDKDIR)/*/*/))

# Main and graphics.
OBJS = $(addprefix $(BUILDDIR)/$(TARGET)/, \
	start.o loader.o lz.o \
)

################################################################################

CUSTOMDEFINES := -DBL_MAGIC=$(IPL_MAGIC)
CUSTOMDEFINES += -DBL_VER_MJ=$(BLVERSION_MAJOR) -DBL_VER_MN=$(BLVERSION_MINOR) -DBL_VER_HF=$(BLVERSION_HOTFX) -DBL_VER_RL=$(BLVERSION_REL)

#TODO: Considering reinstating some of these when pointer warnings have been fixed.
WARNINGS := -Wall -Wsign-compare -Wno-array-bounds -Wno-stringop-overflow

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb-interwork
CFLAGS = $(ARCH) -O2 -g -nostdlib -ffunction-sections -fdata-sections -fomit-frame-pointer -std=gnu11 $(WARNINGS) $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections -Xlinker --defsym=LDR_LOAD_ADDR=$(LDR_LOAD_ADDR)

################################################################################

.PHONY: all clean

all: $(TARGET).bin $(TOOLSLZ) $(TOOLSB2C)

clean:
	@rm -rf $(OBJS)

$(TARGET).bin: $(BUILDDIR)/$(TARGET)/$(TARGET).elf
	@$(OBJCOPY) -S -O binary $< $(OUTPUTDIR)/$(PAYLOAD_NAME).bin

$(BUILDDIR)/$(TARGET)/$(TARGET).elf: $(OBJS)
	@$(CC) $(LDFLAGS) -T link.ld $^ -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.c
	@$(CC) $(CFLAGS) $(BDKINC) -c $< -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.S
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJS): $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET):
	@mkdir -p "$(BUILDDIR)"
	@mkdir -p "$(BUILDDIR)/$(TARGET)"
	@mkdir -p "$(OUTPUTDIR)"
