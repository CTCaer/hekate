/*
 * Copyright (c) 2019 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../lv_misc/lv_font.h"

#include <memory_map.h>

#if USE_HEKATE_SYMBOL_30 != 0	/*Can be enabled in lv_conf.h*/

/***********************************************************************************
 * hekate-symbols.ttf 30 px Font in U+f000 () .. U+f2ee ()  range with all bpp
 * Sparse font with only these characters: 
***********************************************************************************/

/*Store the glyph descriptions*/
static const lv_font_glyph_dsc_t hekate_symbol_30_glyph_dsc[] =
{
#if USE_HEKATE_SYMBOL_30 == 4
  {.w_px = 7,	.glyph_index = 0},	/*Unicode: U+f001 ()*/
  {.w_px = 25,	.glyph_index = 120},	/*Unicode: U+f008 ()*/
  {.w_px = 27,	.glyph_index = 510},	/*Unicode: U+f00b ()*/
  {.w_px = 31,	.glyph_index = 930},	/*Unicode: U+f00c ()*/
  {.w_px = 22,	.glyph_index = 1410},	/*Unicode: U+f00d ()*/
  {.w_px = 25,	.glyph_index = 1740},	/*Unicode: U+f011 ()*/
  {.w_px = 25,	.glyph_index = 2130},	/*Unicode: U+f013 ()*/
  {.w_px = 23,	.glyph_index = 2520},	/*Unicode: U+f014 ()*/
  {.w_px = 34,	.glyph_index = 2880},	/*Unicode: U+f015 ()*/
  {.w_px = 25,	.glyph_index = 3390},	/*Unicode: U+f019 ()*/
  {.w_px = 32,	.glyph_index = 3780},	/*Unicode: U+f01c ()*/
  {.w_px = 25,	.glyph_index = 4260},	/*Unicode: U+f021 ()*/
  {.w_px = 25,	.glyph_index = 4650},	/*Unicode: U+f026 ()*/
  {.w_px = 25,	.glyph_index = 5040},	/*Unicode: U+f027 ()*/
  {.w_px = 20,	.glyph_index = 5430},	/*Unicode: U+f028 ()*/
  {.w_px = 20,	.glyph_index = 5730},	/*Unicode: U+f03e ()*/
  {.w_px = 25,	.glyph_index = 6030},	/*Unicode: U+f040 ()*/
  {.w_px = 20,	.glyph_index = 6420},	/*Unicode: U+f048 ()*/
  {.w_px = 20,	.glyph_index = 6720},	/*Unicode: U+f04b ()*/
  {.w_px = 20,	.glyph_index = 7020},	/*Unicode: U+f04c ()*/
  {.w_px = 13,	.glyph_index = 7320},	/*Unicode: U+f04d ()*/
  {.w_px = 32,	.glyph_index = 7530},	/*Unicode: U+f051 ()*/
  {.w_px = 30,	.glyph_index = 8010},	/*Unicode: U+f052 ()*/
  {.w_px = 16,	.glyph_index = 8460},	/*Unicode: U+f053 ()*/
  {.w_px = 16,	.glyph_index = 8700},	/*Unicode: U+f054 ()*/
  {.w_px = 25,	.glyph_index = 8940},	/*Unicode: U+f067 ()*/
  {.w_px = 25,	.glyph_index = 9330},	/*Unicode: U+f068 ()*/
  {.w_px = 27,	.glyph_index = 9720},	/*Unicode: U+f071 ()*/
  {.w_px = 29,	.glyph_index = 10140},	/*Unicode: U+f074 ()*/
  {.w_px = 26,	.glyph_index = 10590},	/*Unicode: U+f077 ()*/
  {.w_px = 26,	.glyph_index = 10980},	/*Unicode: U+f078 ()*/
  {.w_px = 25,	.glyph_index = 11370},	/*Unicode: U+f079 ()*/
  {.w_px = 29,	.glyph_index = 11760},	/*Unicode: U+f07b ()*/
  {.w_px = 25,	.glyph_index = 12210},	/*Unicode: U+f093 ()*/
  {.w_px = 37,	.glyph_index = 12600},	/*Unicode: U+f095 ()*/
  {.w_px = 25,	.glyph_index = 13170},	/*Unicode: U+f0c4 ()*/
  {.w_px = 23,	.glyph_index = 13560},	/*Unicode: U+f0c5 ()*/
  {.w_px = 24,	.glyph_index = 13920},	/*Unicode: U+f0c7 ()*/
  {.w_px = 13,	.glyph_index = 14280},	/*Unicode: U+f0e7 ()*/
  {.w_px = 18,	.glyph_index = 14490},	/*Unicode: U+f0f3 ()*/
  {.w_px = 33,	.glyph_index = 14760},	/*Unicode: U+f11c ()*/
  {.w_px = 25,	.glyph_index = 15270},	/*Unicode: U+f124 ()*/
  {.w_px = 20,	.glyph_index = 15660},	/*Unicode: U+f15b ()*/
  {.w_px = 29,	.glyph_index = 15960},	/*Unicode: U+f1eb ()*/
  {.w_px = 38,	.glyph_index = 16410},	/*Unicode: U+f240 ()*/
  {.w_px = 38,	.glyph_index = 16980},	/*Unicode: U+f241 ()*/
  {.w_px = 38,	.glyph_index = 17550},	/*Unicode: U+f242 ()*/
  {.w_px = 38,	.glyph_index = 18120},	/*Unicode: U+f243 ()*/
  {.w_px = 38,	.glyph_index = 18690},	/*Unicode: U+f244 ()*/
  {.w_px = 29,	.glyph_index = 19260},	/*Unicode: U+f293 ()*/

#elif USE_HEKATE_SYMBOL_30 == 8
  {.w_px = 7,	.glyph_index = 0},	/*Unicode: U+f001 ()*/
  {.w_px = 25,	.glyph_index = 210},	/*Unicode: U+f008 ()*/
  {.w_px = 27,	.glyph_index = 960},	/*Unicode: U+f00b ()*/
  {.w_px = 31,	.glyph_index = 1770},	/*Unicode: U+f00c ()*/
  {.w_px = 22,	.glyph_index = 2700},	/*Unicode: U+f00d ()*/
  {.w_px = 25,	.glyph_index = 3360},	/*Unicode: U+f011 ()*/
  {.w_px = 25,	.glyph_index = 4110},	/*Unicode: U+f013 ()*/
  {.w_px = 23,	.glyph_index = 4860},	/*Unicode: U+f014 ()*/
  {.w_px = 34,	.glyph_index = 5550},	/*Unicode: U+f015 ()*/
  {.w_px = 25,	.glyph_index = 6570},	/*Unicode: U+f019 ()*/
  {.w_px = 32,	.glyph_index = 7320},	/*Unicode: U+f01c ()*/
  {.w_px = 25,	.glyph_index = 8280},	/*Unicode: U+f021 ()*/
  {.w_px = 25,	.glyph_index = 9030},	/*Unicode: U+f026 ()*/
  {.w_px = 25,	.glyph_index = 9780},	/*Unicode: U+f027 ()*/
  {.w_px = 20,	.glyph_index = 10530},	/*Unicode: U+f028 ()*/
  {.w_px = 20,	.glyph_index = 11130},	/*Unicode: U+f03e ()*/
  {.w_px = 25,	.glyph_index = 11730},	/*Unicode: U+f040 ()*/
  {.w_px = 20,	.glyph_index = 12480},	/*Unicode: U+f048 ()*/
  {.w_px = 20,	.glyph_index = 13080},	/*Unicode: U+f04b ()*/
  {.w_px = 20,	.glyph_index = 13680},	/*Unicode: U+f04c ()*/
  {.w_px = 13,	.glyph_index = 14280},	/*Unicode: U+f04d ()*/
  {.w_px = 32,	.glyph_index = 14670},	/*Unicode: U+f051 ()*/
  {.w_px = 30,	.glyph_index = 15630},	/*Unicode: U+f052 ()*/
  {.w_px = 16,	.glyph_index = 16530},	/*Unicode: U+f053 ()*/
  {.w_px = 16,	.glyph_index = 17010},	/*Unicode: U+f054 ()*/
  {.w_px = 25,	.glyph_index = 17490},	/*Unicode: U+f067 ()*/
  {.w_px = 25,	.glyph_index = 18240},	/*Unicode: U+f068 ()*/
  {.w_px = 27,	.glyph_index = 18990},	/*Unicode: U+f071 ()*/
  {.w_px = 29,	.glyph_index = 19800},	/*Unicode: U+f074 ()*/
  {.w_px = 26,	.glyph_index = 20670},	/*Unicode: U+f077 ()*/
  {.w_px = 26,	.glyph_index = 21450},	/*Unicode: U+f078 ()*/
  {.w_px = 25,	.glyph_index = 22230},	/*Unicode: U+f079 ()*/
  {.w_px = 29,	.glyph_index = 22980},	/*Unicode: U+f07b ()*/
  {.w_px = 25,	.glyph_index = 23850},	/*Unicode: U+f093 ()*/
  {.w_px = 37,	.glyph_index = 24600},	/*Unicode: U+f095 ()*/
  {.w_px = 25,	.glyph_index = 25710},	/*Unicode: U+f0c4 ()*/
  {.w_px = 23,	.glyph_index = 26460},	/*Unicode: U+f0c5 ()*/
  {.w_px = 24,	.glyph_index = 27150},	/*Unicode: U+f0c7 ()*/
  {.w_px = 13,	.glyph_index = 27870},	/*Unicode: U+f0e7 ()*/
  {.w_px = 18,	.glyph_index = 28260},	/*Unicode: U+f0f3 ()*/
  {.w_px = 33,	.glyph_index = 28800},	/*Unicode: U+f11c ()*/
  {.w_px = 25,	.glyph_index = 29790},	/*Unicode: U+f124 ()*/
  {.w_px = 20,	.glyph_index = 30540},	/*Unicode: U+f15b ()*/
  {.w_px = 29,	.glyph_index = 31140},	/*Unicode: U+f1eb ()*/
  {.w_px = 38,	.glyph_index = 32010},	/*Unicode: U+f240 ()*/
  {.w_px = 38,	.glyph_index = 33150},	/*Unicode: U+f241 ()*/
  {.w_px = 38,	.glyph_index = 34290},	/*Unicode: U+f242 ()*/
  {.w_px = 38,	.glyph_index = 35430},	/*Unicode: U+f243 ()*/
  {.w_px = 38,	.glyph_index = 36570},	/*Unicode: U+f244 ()*/
  {.w_px = 29,	.glyph_index = 37710},	/*Unicode: U+f293 ()*/

#endif
};

lv_font_t hekate_symbol_30 =
{
    .unicode_first = LV_SYMBOL_GLYPH_FIRST,	/*First Unicode letter in this font*/
    .unicode_last = LV_SYMBOL_GLYPH_LAST,	/*Last Unicode letter in this font*/
    .h_px = 30,				/*Font height in pixels*/
    //.glyph_bitmap = hekate_symbol_30_glyph_bitmap,	/*Bitmap of glyphs*/
    .glyph_bitmap = (const uint8_t *)(NYX_RES_ADDR + 0x14200),
    .glyph_dsc = hekate_symbol_30_glyph_dsc,		/*Description of glyphs*/
    .glyph_cnt = 50,			/*Number of glyphs in the font*/
    .unicode_list = NULL,	/*List of unicode characters*/
    .get_bitmap = lv_font_get_bitmap_continuous,	/*Function pointer to get glyph's bitmap*/
    .get_width = lv_font_get_width_continuous,	/*Function pointer to get glyph's width*/
#if USE_HEKATE_SYMBOL_30 == 4
    .bpp = 4,				/*Bit per pixel*/
 #elif USE_HEKATE_SYMBOL_30 == 8
    .bpp = 8,				/*Bit per pixel*/
#endif
    .monospace = 0,				/*Fix width (0: if not used)*/
    .next_page = NULL,		/*Pointer to a font extension*/
};

#endif /*USE_HEKATE_SYMBOL_30*/
