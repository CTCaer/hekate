CSRCS += lv_theme.c
CSRCS += lv_theme_default.c
CSRCS += lv_theme_templ.c
CSRCS += lv_theme_material.c

DEPPATH += --dep-path $(LVGL_DIR)/lvgl/lv_themes
VPATH += :$(LVGL_DIR)/lvgl/lv_themes

CFLAGS += "-I$(LVGL_DIR)/lvgl/lv_themes"
