ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

################################################################################

IPL_LOAD_ADDR := 0x40008000
IPL_MAGIC := 0x43544349 #"ICTC"
include ./Versions.inc

################################################################################

TARGET := hekate
BUILDDIR := build
BUILDTDIR := build/$(TARGET)
OUTPUTDIR := output
SOURCEDIR = bootloader
BDKDIR := bdk
BDKINC := -I./$(BDKDIR)
VPATH = $(dir ./$(SOURCEDIR)/) $(dir $(wildcard ./$(SOURCEDIR)/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/))
VPATH += $(dir $(wildcard ./$(BDKDIR)/)) $(dir $(wildcard ./$(BDKDIR)/*/)) $(dir $(wildcard ./$(BDKDIR)/*/*/))

# Track compiler flags
TRACK_CFLAGS = $(BUILDTDIR)/.cflags
TRACK_LDFLAGS = $(BUILDTDIR)/.ldflags

# Main and graphics.
OBJS =  start exception_handlers main heap gfx logos tui fe_info fe_tools

# Hardware.
OBJS += bpmp ccplex clock di i2c irq timer \
		mc sdram minerva smmu \
		gpio pinmux pmc se tsec uart \
		fuse kfuse \
		sdmmc sdmmc_driver emmc sd emummc \
		bq24193 max17050 max7762x max77620-rtc \
		hw_init

# Utilities.
OBJS += btn dirlist ianos ini util config

# OS loaders.
OBJS += l4t hos hos_config pkg1 pkg2 pkg3 pkg2_ini_kippatch secmon_exo

# Libraries.
OBJS += lz lz4 blz diskio ff ffunicode ffsystem elfload elfreloc_arm

OBJS := $(addsuffix .o, $(OBJS))
OBJS := $(addprefix $(BUILDTDIR)/, $(OBJS))

GFX_INC   := '"../$(SOURCEDIR)/gfx/gfx.h"'
FFCFG_INC := '"../$(SOURCEDIR)/libs/fatfs/ffconf.h"'

################################################################################

CUSTOMDEFINES := -DIPL_LOAD_ADDR=$(IPL_LOAD_ADDR) -DBL_MAGIC=$(IPL_MAGIC)
CUSTOMDEFINES += -DBL_VER_MJ=$(BLVERSION_MAJOR) -DBL_VER_MN=$(BLVERSION_MINOR) -DBL_VER_HF=$(BLVERSION_HOTFX) -DBL_VER_RL=$(BLVERSION_REL)
CUSTOMDEFINES += -DNYX_VER_MJ=$(NYXVERSION_MAJOR) -DNYX_VER_MN=$(NYXVERSION_MINOR) -DNYX_VER_HF=$(NYXVERSION_HOTFX) -DNYX_VER_RL=$(NYXVERSION_REL)

# BDK defines.
CUSTOMDEFINES += -DBDK_MALLOC_NO_DEFRAG -DBDK_MC_ENABLE_AHB_REDIRECT -DBDK_EMUMMC_ENABLE
CUSTOMDEFINES += -DBDK_WATCHDOG_FIQ_ENABLE -DBDK_RESTART_BL_ON_WDT
CUSTOMDEFINES += -DGFX_INC=$(GFX_INC) -DFFCFG_INC=$(FFCFG_INC)

#CUSTOMDEFINES += -DDEBUG

# UART Logging: Max baudrate 12.5M.
# DEBUG_UART_PORT - 0: UART_A, 1: UART_B, 2: UART_C.
#CUSTOMDEFINES += -DDEBUG_UART_BAUDRATE=115200 -DDEBUG_UART_INVERT=0 -DDEBUG_UART_PORT=1

#TODO: Considering reinstating some of these when pointer warnings have been fixed.
WARNINGS := -Wall -Wsign-compare -Wtype-limits -Wno-array-bounds -Wno-stringop-overread -Wno-stringop-overflow
#-fno-delete-null-pointer-checks
#-Wstack-usage=byte-size -fstack-usage

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb -mthumb-interwork $(WARNINGS)
CFLAGS = $(ARCH) -O2 -g -gdwarf-4 -nostdlib -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-inline -std=gnu11 $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections -Xlinker --defsym=IPL_LOAD_ADDR=$(IPL_LOAD_ADDR)

MODULEDIRS := $(wildcard modules/*)
NYXDIR := $(wildcard nyx)
LDRDIR := $(wildcard loader)
TOOLSLZ := $(wildcard tools/lz)
TOOLSB2C := $(wildcard tools/bin2c)
TOOLS := $(TOOLSLZ) $(TOOLSB2C)

ifndef IPLECHO
T := $(shell $(MAKE) $(BUILDTDIR)/$(TARGET).elf --no-print-directory -nrRf $(firstword $(MAKEFILE_LIST)) IPLECHO="IPLOBJ" | grep -c "IPLOBJ")

N := x
C = $(words $N)$(eval N := x $N)
IPLECHO = echo -ne "\r`expr "  [\`expr $C '*' 100 / $T\`" : '.*\(....\)$$'`%]\033[K"
endif

################################################################################

.PHONY: all clean $(LDRDIR) $(TOOLS) $(NYXDIR) $(MODULEDIRS)

all: $(TARGET).bin $(LDRDIR)
	@printf ICTC49 >> $(OUTPUTDIR)/$(TARGET).bin
	@echo "--------------------------------------"
	@echo "$(TARGET) size:"
	@echo -n "Uncompr:  "
	$(eval BIN_SIZE = $(shell wc -c < $(OUTPUTDIR)/$(TARGET)_unc.bin))
	@echo $(BIN_SIZE)" Bytes"
	@if [ ${BIN_SIZE} -gt 140288 ]; then echo "\e[1;33mUncompr size exceeds limit!\e[0m"; fi
	@echo -n "Payload:  "
	$(eval BIN_SIZE = $(shell wc -c < $(OUTPUTDIR)/$(TARGET).bin))
	@echo $(BIN_SIZE)" Bytes"
	@if [ ${BIN_SIZE} -gt 126296 ]; then echo "\e[1;33mPayload size exceeds limit!\e[0m"; fi
	@echo "--------------------------------------"

clean: $(TOOLS)
	@rm -rf $(BUILDDIR)
	@rm -rf $(OUTPUTDIR)
	@$(MAKE) --no-print-directory -C $(LDRDIR) $(MAKECMDGOALS) -$(MAKEFLAGS)

$(MODULEDIRS): $(BUILDTDIR)/$(TARGET).elf
	@$(MAKE) --no-print-directory -C $@ $(MAKECMDGOALS) -$(MAKEFLAGS)

$(NYXDIR): $(BUILDTDIR)/$(TARGET).elf $(MODULEDIRS)
	@echo --------------------------------------
	@$(MAKE) --no-print-directory -C $@ $(MAKECMDGOALS) -$(MAKEFLAGS)

$(LDRDIR): $(TARGET).bin $(TOOLS) $(NYXDIR) $(MODULEDIRS)
	@$(TOOLSLZ)/lz77 $(OUTPUTDIR)/$(TARGET).bin
	@mv $(OUTPUTDIR)/$(TARGET).bin $(OUTPUTDIR)/$(TARGET)_unc.bin
	@mv $(OUTPUTDIR)/$(TARGET).bin.00.lz payload_00
	@mv $(OUTPUTDIR)/$(TARGET).bin.01.lz payload_01
	@$(TOOLSB2C)/bin2c payload_00 > $(LDRDIR)/payload_00.h
	@$(TOOLSB2C)/bin2c payload_01 > $(LDRDIR)/payload_01.h
	@rm payload_00
	@rm payload_01
	@$(MAKE) --no-print-directory -C $@ $(MAKECMDGOALS) -$(MAKEFLAGS) PAYLOAD_NAME=$(TARGET)

$(TOOLS):
	@$(MAKE) --no-print-directory -C $@ $(MAKECMDGOALS) -$(MAKEFLAGS)

$(TARGET).bin: $(BUILDTDIR)/$(TARGET).elf
	@$(OBJCOPY) -S -O binary $< $(OUTPUTDIR)/$@
	@echo --------------------------------------

$(BUILDTDIR)/$(TARGET).elf: $(OBJS) $(TRACK_LDFLAGS)
	@echo -ne "\r[100%] Linking $(TARGET).elf\033[K"
	@$(CC) $(LDFLAGS) -T $(SOURCEDIR)/link.ld $(OBJS) -o $@
	@printf "\n$(TARGET) was built with the following flags:\nCFLAGS:  $(CFLAGS)\nLDFLAGS: $(LDFLAGS)\n"

$(BUILDTDIR)/%.o: %.c $(TRACK_CFLAGS) | $(BUILDTDIR)
	@$(IPLECHO) Building $@
	@$(CC) $(CFLAGS) $(BDKINC) -MMD -MP -c $< -o $@

$(BUILDTDIR)/%.o: %.S $(TRACK_CFLAGS) | $(BUILDTDIR)
	@$(IPLECHO) Building $@
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDTDIR):
	@mkdir -p "$(BUILDDIR)"
	@mkdir -p "$(BUILDTDIR)"
	@mkdir -p "$(OUTPUTDIR)"

# Non objects change detectors.
$(TRACK_CFLAGS): $(BUILDTDIR)
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
$(TRACK_LDFLAGS): $(BUILDTDIR)
	@echo '$(LDFLAGS)' | cmp -s - $@ || echo '$(LDFLAGS)' > $@
-include $(OBJS:.o=.d)
