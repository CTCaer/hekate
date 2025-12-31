ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

################################################################################

NYX_LOAD_ADDR := 0x81000000
NYX_MAGIC := 0x43544347 #"GCTC"
include ./../Versions.inc

################################################################################

TARGET := nyx
BUILDDIR := ./../build/$(TARGET)
OUTPUTDIR := ./../output
SOURCEDIR = nyx_gui
BDKDIR := bdk
BDKINC := -I../$(BDKDIR)
VPATH = $(dir $(wildcard ./$(SOURCEDIR)/)) $(dir $(wildcard ./$(SOURCEDIR)/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/))
VPATH += $(dir $(wildcard ./$(SOURCEDIR)/*/*/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/*/*/))
VPATH += $(dir $(wildcard ../$(BDKDIR)/)) $(dir $(wildcard ../$(BDKDIR)/*/)) $(dir $(wildcard ../$(BDKDIR)/*/*/)) $(dir $(wildcard ../$(BDKDIR)/*/*/*/))

# Track compiler flags
TRACK_CFLAGS = $(BUILDDIR)/.cflags
TRACK_LDFLAGS = $(BUILDDIR)/.ldflags

# Main and graphics.
OBJS =  start exception_handlers nyx heap gfx \
		gui gui_info gui_tools gui_options gui_emmc_tools gui_emummc_tools gui_tools_partition_manager \
		fe_emummc_tools fe_emmc_tools

# Hardware.
OBJS += actmon bpmp ccplex clock di vic i2c irq timer \
		gpio  pinmux pmc se smmu tsec uart \
		fuse kfuse \
		mc sdram minerva ramdisk \
		sdmmc sdmmc_driver emmc sd nx_emmc_bis \
		bm92t36 bq24193 max17050 max7762x max77620-rtc regulator_5v \
		touch joycon tmp451 fan \
		usbd xusbd usb_descriptors usb_gadget_ums usb_gadget_hid \
		hw_init

# Utilities.
OBJS += btn dirlist ianos ini util config sprintf

# Horizon.
OBJS += hos pkg1 pkg2

# Libraries.
OBJS += diskio ff ffunicode ffsystem \
		elfload elfreloc_arm blz \
		lv_group lv_indev lv_obj lv_refr lv_style lv_vdb \
		lv_draw lv_draw_rbasic lv_draw_vbasic lv_draw_arc lv_draw_img \
		lv_draw_label lv_draw_line lv_draw_rect lv_draw_triangle \
		lv_hal_disp lv_hal_indev lv_hal_tick \
		interui_20 interui_30 ubuntu_mono hekate_symbol_20 hekate_symbol_30 hekate_symbol_120 lv_font_builtin \
		lv_anim lv_area lv_circ lv_color lv_font lv_ll lv_math lv_mem lv_task lv_txt lv_gc \
		lv_bar lv_btn lv_btnm lv_cb lv_cont lv_ddlist lv_img lv_label lv_line lv_list lv_lmeter lv_mbox \
		lv_page lv_roller lv_slider lv_sw lv_tabview lv_ta lv_win lv_log lv_imgbtn \
		lv_theme lv_theme_hekate

OBJS := $(addsuffix .o, $(OBJS))
OBJS := $(addprefix $(BUILDDIR)/, $(OBJS))

GFX_INC   := '"../nyx/$(SOURCEDIR)/gfx/gfx.h"'
FFCFG_INC := '"../nyx/$(SOURCEDIR)/libs/fatfs/ffconf.h"'

################################################################################

CUSTOMDEFINES := -DNYX_LOAD_ADDR=$(NYX_LOAD_ADDR) -DNYX_MAGIC=$(NYX_MAGIC)
CUSTOMDEFINES += -DNYX_VER_MJ=$(NYXVERSION_MAJOR) -DNYX_VER_MN=$(NYXVERSION_MINOR) -DNYX_VER_HF=$(NYXVERSION_HOTFX) -DNYX_VER_RL=$(NYXVERSION_REL)

# BDK defines.
CUSTOMDEFINES += -DBDK_MC_ENABLE_AHB_REDIRECT -DBDK_MINERVA_CFG_FROM_RAM -DBDK_HW_EXTRA_DEINIT -DBDK_SDMMC_EXTRA_PRINT
CUSTOMDEFINES += -DGFX_INC=$(GFX_INC) -DFFCFG_INC=$(FFCFG_INC)

#CUSTOMDEFINES += -DDEBUG

# UART Logging: Max baudrate 12.5M. Disables Joycon on Nyx if UARTB or UARTC.
# DEBUG_UART_PORT - 0: UART_A, 1: UART_B, 2: UART_C.
#CUSTOMDEFINES += -DDEBUG_UART_BAUDRATE=115200 -DDEBUG_UART_INVERT=0 -DDEBUG_UART_PORT=0

# LvGL UART LOG.
#CUSTOMDEFINES += -DDEBUG_UART_LV_LOG

#TODO: Considering reinstating some of these when pointer warnings have been fixed.
WARNINGS := -Wall -Wsign-compare -Wtype-limits -Wno-array-bounds -Wno-stringop-overread -Wno-stringop-overflow
#-fno-delete-null-pointer-checks
#-Wstack-usage=byte-size -fstack-usage

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb-interwork $(WARNINGS)
CFLAGS = $(ARCH) -O2 -g -gdwarf-4 -nostdlib -ffunction-sections -fdata-sections -fomit-frame-pointer -std=gnu11 $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections -Xlinker --defsym=NYX_LOAD_ADDR=$(NYX_LOAD_ADDR)

ifndef NYXECHO
T := $(shell $(MAKE) $(BUILDDIR)/$(TARGET).elf --no-print-directory -nrRf $(firstword $(MAKEFILE_LIST)) NYXECHO="NYXOBJ" | grep -c "NYXOBJ")

N := x
C = $(words $N)$(eval N := x $N)
NYXECHO = echo -ne "\r`expr "  [\`expr $C '*' 100 / $T\`" : '.*\(....\)$$'`%]\033[K"
endif

################################################################################

.PHONY: all clean

all: $(TARGET).bin
	@echo "--------------------------------------"
	@echo -n "Nyx size: "
	$(eval BIN_SIZE = $(shell wc -c < $(OUTPUTDIR)/$(TARGET).bin))
	@echo $(BIN_SIZE)" Bytes"
	@echo "--------------------------------------"

clean:
	@rm -rf $(OBJS)
	@rm -rf $(BUILDDIR)
	@rm -rf $(OUTPUTDIR)

$(TARGET).bin: $(BUILDDIR)/$(TARGET).elf
	@$(OBJCOPY) -S -O binary $< $(OUTPUTDIR)/$@

$(BUILDDIR)/$(TARGET).elf: $(OBJS) $(TRACK_LDFLAGS)
	@echo -ne "\r[100%] Linking $(TARGET).elf\033[K"
	@$(CC) $(LDFLAGS) -T $(SOURCEDIR)/link.ld $(OBJS) -o $@
	@printf "\n$(TARGET) was built with the following flags:\nCFLAGS:  $(CFLAGS)\nLDFLAGS: $(LDFLAGS)\n"

$(BUILDDIR)/%.o: %.c $(TRACK_CFLAGS) | $(BUILDDIR)
	@$(NYXECHO) Building $@
	@$(CC) $(CFLAGS) $(BDKINC) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: %.S $(TRACK_CFLAGS) | $(BUILDDIR)
	@$(NYXECHO) Building $@
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR):
	@mkdir -p "$(BUILDDIR)"
	@mkdir -p "$(OUTPUTDIR)"

# Non objects change detectors.
$(TRACK_CFLAGS): $(BUILDDIR)
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
$(TRACK_LDFLAGS): $(BUILDDIR)
	@echo '$(LDFLAGS)' | cmp -s - $@ || echo '$(LDFLAGS)' > $@
-include $(OBJS:.o=.d)
