ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

TARGET := ipl
BUILD := build_ipl
OUTPUT := output
SOURCEDIR := ipl

OBJS = $(addprefix $(BUILD)/, \
	start.o \
	main.o \
	config.o \
	btn.o \
	blz.o \
	clock.o \
	cluster.o \
	fuse.o \
	gpio.o \
	heap.o \
	hos.o \
	i2c.o \
	kfuse.o \
	lz.o \
	bq24193.o \
	max7762x.o \
	max17050.o \
	mc.o \
	nx_emmc.o \
	sdmmc.o \
	sdmmc_driver.o \
	sdram.o \
	sdram_lp0.o \
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
	ini.o \
)

OBJS += $(addprefix $(BUILD)/, diskio.o ff.o ffunicode.o ffsystem.o)
OBJS += $(addprefix $(BUILD)/elfloader/, elfload.o elfreloc_arm.o)

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb -mthumb-interwork
CUSTOMDEFINES := -DMENU_LOGO_ENABLE #-DDEBUG
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

$(TARGET).bin: $(BUILD)/$(TARGET).elf $(MODULEDIRS)
	$(OBJCOPY) -S -O binary $< $(OUTPUT)/$@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -T ipl/link.ld $^ -o $@

$(BUILD)/%.o: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SOURCEDIR)/%.S
	@mkdir -p "$(BUILD)"
	@mkdir -p "$(BUILD)/elfloader"
	@mkdir -p "$(OUTPUT)"
	$(CC) $(CFLAGS) -c $< -o $@
