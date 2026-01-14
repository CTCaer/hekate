ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

TARGET := module_sample
BUILDDIR := ../../build/$(TARGET)
OUTPUTDIR := ../../output
SOURCEDIR = simple_sample
BDKDIR := bdk
BDKINC := -I../../$(BDKDIR)
VPATH = $(dir ./) $(dir $(wildcard ./*/))

# Track compiler flags
TRACK_CFLAGS = $(BUILDDIR)/.cflags
TRACK_LDFLAGS = $(BUILDDIR)/.ldflags

OBJS = $(addprefix $(BUILDDIR)/,\
	module_sample.o \
	gfx.o \
)

GFX_INC   := '"../modules/$(SOURCEDIR)/gfx/gfx.h"'

CUSTOMDEFINES := -DGFX_INC=$(GFX_INC)

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb-interwork
CFLAGS = $(ARCH) -O2 -nostdlib -fpie -ffunction-sections -fdata-sections -fomit-frame-pointer -std=gnu11 -Wall -Wsign-compare $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -fpie -pie -nostartfiles -lgcc

.PHONY: clean all

all: $(TARGET).bso
$(BUILDDIR)/%.o: ./%.c $(TRACK_CFLAGS)
	@$(CC) $(CFLAGS) $(BDKINC) -MMD -MP -c $< -o $@

$(TARGET).bso: $(OBJS) $(TRACK_LDFLAGS)
	@$(CC) $(LDFLAGS) -e mod_entry $(OBJS) -o $(OUTPUTDIR)/$(TARGET).bso
	@$(STRIP) -g $(OUTPUTDIR)/$(TARGET).bso
	@echo "Built module: "$(TARGET)".bso"

clean:
	@rm -rf $(BUILDDIR)/$(TARGET)
	@rm -rf $(OUTPUTDIR)/$(TARGET).bso

$(BUILDDIR):
	@mkdir -p "$(BUILDDIR)"

# Non objects change detectors.
$(TRACK_CFLAGS): $(BUILDDIR)
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
$(TRACK_LDFLAGS): $(BUILDDIR)
	@echo '$(LDFLAGS)' | cmp -s - $@ || echo '$(LDFLAGS)' > $@
-include $(OBJS:.o=.d)
