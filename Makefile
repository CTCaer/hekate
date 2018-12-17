ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

TARGET := hekate
BLVERSION_MAJOR := 4
BLVERSION_MINOR := 6
BUILD := build
OUTPUT := output
SOURCEDIR = bootloader
VPATH = $(dir $(wildcard ./$(SOURCEDIR)/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/))

OBJS = $(addprefix $(BUILD)/$(TARGET)/, \
	start.o \
	main.o \
	fe_emmc_tools.o \
	fe_info.o \
	fe_tools.o \
	config.o \
	btn.o \
	clock.o \
	cluster.o \
	fuse.o \
	gpio.o \
	heap.o \
	hos.o \
	hos_config.o \
	secmon_exo.o \
	i2c.o \
	kfuse.o \
	bq24193.o \
	max7762x.o \
	max17050.o \
	mc.o \
	nx_emmc.o \
	sdmmc.o \
	sdmmc_driver.o \
	sdram.o \
	tui.o \
	util.o \
	di.o \
	gfx.o \
	pinmux.o \
	pkg1.o \
	pkg2.o \
	se.o \
	tsec.o \
	uart.o \
	hw_init.o \
	dirlist.o \
	ini.o \
	ianos.o \
	smmu.o \
)

OBJS += $(addprefix $(BUILD)/$(TARGET)/, \
	lz.o blz.o \
	diskio.o ff.o ffunicode.o ffsystem.o \
	elfload.o elfreloc_arm.o \
)

CUSTOMDEFINES := -DBLVERSIONMJ=$(BLVERSION_MAJOR) -DBLVERSIONMN=$(BLVERSION_MINOR) -DMENU_LOGO_ENABLE
#CUSTOMDEFINES += -DDEBUG
# 0: UART_A, 1: UART_B.
#CUSTOMDEFINES += -DDEBUG_UART_PORT=0

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb -mthumb-interwork
CFLAGS = $(ARCH) -O2 -nostdlib -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-inline -std=gnu11 -Wall $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections

MODULEDIRS := $(wildcard modules/*)

.PHONY: all clean $(MODULEDIRS)

all: $(TARGET).bin
	@echo -n "Payload size is "
	@wc -c < $(OUTPUT)/$(TARGET).bin
	@echo "Max size is 126296 Bytes."

clean:
	@rm -rf $(OBJS)
	@rm -rf $(BUILD)
	@rm -rf $(OUTPUT)

$(MODULEDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(TARGET).bin: $(BUILD)/$(TARGET)/$(TARGET).elf $(MODULEDIRS)
	$(OBJCOPY) -S -O binary $< $(OUTPUT)/$@
	@printf ICTC$(BLVERSION_MAJOR)$(BLVERSION_MINOR) >> $(OUTPUT)/$@

$(BUILD)/$(TARGET)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -T $(SOURCEDIR)/link.ld $^ -o $@

$(BUILD)/$(TARGET)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET)/%.o: %.S
	@mkdir -p "$(BUILD)"
	@mkdir -p "$(BUILD)/$(TARGET)"
	@mkdir -p "$(OUTPUT)"
	$(CC) $(CFLAGS) -c $< -o $@
