include rules/dkarm_compat

TARGET := hekate
BLVERSION_MAJOR := 4
BLVERSION_MINOR := 1
BUILD := build
OUTPUT := output
SOURCEDIR = bootloader
VPATH = $(dir $(wildcard ./$(SOURCEDIR)/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/))

OBJS = $(addprefix $(BUILD)/$(TARGET)/, \
	start.o \
	main.o \
	config.o \
	btn.o \
	clock.o \
	cluster.o \
	fuse.o \
	gpio.o \
	heap.o \
	hos.o \
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
	dirlist.o \
	ini.o \
	ianos.o \
)

OBJS += $(addprefix $(BUILD)/$(TARGET)/, \
	lz.o blz.o \
	diskio.o ff.o ffunicode.o ffsystem.o \
	elfload.o elfreloc_arm.o \
)

CFLAGS += -mthumb
LDFLAGS += -mthumb
CFLAGS += -DBLVERSIONMJ=$(BLVERSION_MAJOR) -DBLVERSIONMN=$(BLVERSION_MINOR)
CFLAGS += -DMENU_LOGO_ENABLE
#CFLAGS += -DDEBUG

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
