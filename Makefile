ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

################################################################################

IPL_LOAD_ADDR := 0x40008000
IPL_MAGIC := 0x43544349 #"ICTC"
BLVERSION_MAJOR := 4
BLVERSION_MINOR := 10
BLVERSION_HOTFX := 1

BL_RESERVED := 0

################################################################################

TARGET := hekate
BUILDDIR := build
OUTPUTDIR := output
SOURCEDIR = bootloader
VPATH = $(dir $(wildcard ./$(SOURCEDIR)/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/))

# Main and graphics.
OBJS = $(addprefix $(BUILDDIR)/$(TARGET)/, \
	start.o \
	main.o heap.o \
	gfx.o tui.o \
	fe_emmc_tools.o fe_info.o fe_tools.o \
)

# Hardware.
OBJS += $(addprefix $(BUILDDIR)/$(TARGET)/, \
	clock.o cluster.o di.o gpio.o i2c.o mc.o sdram.o pinmux.o se.o smmu.o tsec.o uart.o \
	fuse.o kfuse.o \
	sdmmc.o sdmmc_driver.o \
	bq24193.o max17050.o max7762x.o max77620-rtc.o \
	hw_init.o \
)

# Utilities.
OBJS += $(addprefix $(BUILDDIR)/$(TARGET)/, \
	btn.o dirlist.o ianos.o util.o \
	config.o ini.o \
)

# Horizon.
OBJS += $(addprefix $(BUILDDIR)/$(TARGET)/, \
	nx_emmc.o \
	hos.o hos_config.o pkg1.o pkg2.o fss.o secmon_exo.o sept.o \
)

# Libraries.
OBJS += $(addprefix $(BUILDDIR)/$(TARGET)/, \
	lz.o blz.o \
	diskio.o ff.o ffunicode.o ffsystem.o \
	elfload.o elfreloc_arm.o \
)

################################################################################

CUSTOMDEFINES := -DIPL_LOAD_ADDR=$(IPL_LOAD_ADDR) -DBL_MAGIC=$(IPL_MAGIC)
CUSTOMDEFINES += -DBL_VER_MJ=$(BLVERSION_MAJOR) -DBL_VER_MN=$(BLVERSION_MINOR) -DBL_VER_HF=$(BLVERSION_HOTFX) -DBL_RESERVED=$(BL_RESERVED)
CUSTOMDEFINES += -DMENU_LOGO_ENABLE

# 0: UART_A, 1: UART_B.
#CUSTOMDEFINES += -DDEBUG_UART_PORT=0

#CUSTOMDEFINES += -DDEBUG

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb -mthumb-interwork
CFLAGS = $(ARCH) -O2 -nostdlib -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-inline -std=gnu11 -Wall $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections -Xlinker --defsym=IPL_LOAD_ADDR=$(IPL_LOAD_ADDR)

MODULEDIRS := $(wildcard modules/*)

################################################################################

.PHONY: all clean $(MODULEDIRS)

all: $(TARGET).bin
	@echo -n "Payload size is "
	@wc -c < $(OUTPUTDIR)/$(TARGET).bin
	@echo "Max size is 126296 Bytes."

clean:
	@rm -rf $(OBJS)
	@rm -rf $(BUILDDIR)
	@rm -rf $(OUTPUTDIR)

$(MODULEDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(TARGET).bin: $(BUILDDIR)/$(TARGET)/$(TARGET).elf $(MODULEDIRS)
	$(OBJCOPY) -S -O binary $< $(OUTPUTDIR)/$@
	@printf ICTC49 >> $(OUTPUTDIR)/$@

$(BUILDDIR)/$(TARGET)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -T $(SOURCEDIR)/link.ld $^ -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.S
	@mkdir -p "$(BUILDDIR)"
	@mkdir -p "$(BUILDDIR)/$(TARGET)"
	@mkdir -p "$(OUTPUTDIR)"
	$(CC) $(CFLAGS) -c $< -o $@
