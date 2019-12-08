#ifndef _LOGOS_GUI_H_
#define _LOGOS_GUI_H_

#include "../../../common/memory_map.h"

#include "../libs/lv_conf.h"
#include "../libs/lvgl/lv_draw/lv_draw_img.h"
#include "../utils/types.h"

#define HEKATE_LOGO

#ifdef HEKATE_LOGO

lv_img_dsc_t hekate_logo = {
	.header.always_zero = 0,
	.header.w = 193,
	.header.h = 76,
	.data_size = 14668 * LV_IMG_PX_SIZE_ALPHA_BYTE,
	.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
	.data = (const uint8_t *)(NYX_RES_ADDR + 0x1D900),
};

lv_img_dsc_t ctcaer_logo = {
	.header.always_zero = 0,
	.header.w = 147,
	.header.h = 76,
	.data_size = 11172 * LV_IMG_PX_SIZE_ALPHA_BYTE,
	.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
	.data = (const uint8_t *)(NYX_RES_ADDR + 0x2BF00),
};

#endif

#endif