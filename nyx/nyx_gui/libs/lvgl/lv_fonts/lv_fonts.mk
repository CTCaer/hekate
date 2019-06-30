CSRCS += lv_font_builtin.c
CSRCS += hekate_symbol_10.c
CSRCS += hekate_symbol_20.c
CSRCS += hekate_symbol_30.c
CSRCS += hekate_symbol_40.c
CSRCS += interui_12.c
CSRCS += interui_20.c
CSRCS += interui_30.c
CSRCS += interui_40.c

DEPPATH += --dep-path $(LVGL_DIR)/lvgl/lv_fonts
VPATH += :$(LVGL_DIR)/lvgl/lv_fonts

CFLAGS += "-I$(LVGL_DIR)/lvgl/lv_fonts"
