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

#if USE_HEKATE_SYMBOL_20 != 0	/*Can be enabled in lv_conf.h*/

/***********************************************************************************
 * hekate-symbols.ttf 20 px Font in U+f000 () .. U+f2ee ()  range with all bpp
 * Sparse font with only these characters: 
***********************************************************************************/

/*Store the glyph descriptions*/
static const lv_font_glyph_dsc_t hekate_symbol_20_glyph_dsc[] =
{
#if USE_HEKATE_SYMBOL_20 == 4
  {.w_px = 5,	.glyph_index = 0},	/*Unicode: U+f001 ()*/
  {.w_px = 16,	.glyph_index = 60},	/*Unicode: U+f008 ()*/
  {.w_px = 20,	.glyph_index = 220},	/*Unicode: U+f00b ()*/
  {.w_px = 22,	.glyph_index = 420},	/*Unicode: U+f00c ()*/
  {.w_px = 16,	.glyph_index = 640},	/*Unicode: U+f00d ()*/
  {.w_px = 18,	.glyph_index = 800},	/*Unicode: U+f011 ()*/
  {.w_px = 18,	.glyph_index = 980},	/*Unicode: U+f013 ()*/
  {.w_px = 16,	.glyph_index = 1160},	/*Unicode: U+f014 ()*/
  {.w_px = 23,	.glyph_index = 1320},	/*Unicode: U+f015 ()*/
  {.w_px = 18,	.glyph_index = 1560},	/*Unicode: U+f019 ()*/
  {.w_px = 23,	.glyph_index = 1740},	/*Unicode: U+f01c ()*/
  {.w_px = 18,	.glyph_index = 1980},	/*Unicode: U+f021 ()*/
  {.w_px = 18,	.glyph_index = 2160},	/*Unicode: U+f026 ()*/
  {.w_px = 18,	.glyph_index = 2340},	/*Unicode: U+f027 ()*/
  {.w_px = 13,	.glyph_index = 2520},	/*Unicode: U+f028 ()*/
  {.w_px = 13,	.glyph_index = 2660},	/*Unicode: U+f03e ()*/
  {.w_px = 16,	.glyph_index = 2800},	/*Unicode: U+f040 ()*/
  {.w_px = 13,	.glyph_index = 2960},	/*Unicode: U+f048 ()*/
  {.w_px = 13,	.glyph_index = 3100},	/*Unicode: U+f04b ()*/
  {.w_px = 13,	.glyph_index = 3240},	/*Unicode: U+f04c ()*/
  {.w_px = 9,	.glyph_index = 3380},	/*Unicode: U+f04d ()*/
  {.w_px = 23,	.glyph_index = 3480},	/*Unicode: U+f051 ()*/
  {.w_px = 21,	.glyph_index = 3720},	/*Unicode: U+f052 ()*/
  {.w_px = 11,	.glyph_index = 3940},	/*Unicode: U+f053 ()*/
  {.w_px = 11,	.glyph_index = 4060},	/*Unicode: U+f054 ()*/
  {.w_px = 18,	.glyph_index = 4180},	/*Unicode: U+f067 ()*/
  {.w_px = 18,	.glyph_index = 4360},	/*Unicode: U+f068 ()*/
  {.w_px = 20,	.glyph_index = 4540},	/*Unicode: U+f071 ()*/
  {.w_px = 20,	.glyph_index = 4740},	/*Unicode: U+f074 ()*/
  {.w_px = 18,	.glyph_index = 4940},	/*Unicode: U+f077 ()*/
  {.w_px = 18,	.glyph_index = 5120},	/*Unicode: U+f078 ()*/
  {.w_px = 18,	.glyph_index = 5300},	/*Unicode: U+f079 ()*/
  {.w_px = 20,	.glyph_index = 5480},	/*Unicode: U+f07b ()*/
  {.w_px = 18,	.glyph_index = 5680},	/*Unicode: U+f093 ()*/
  {.w_px = 25,	.glyph_index = 5860},	/*Unicode: U+f095 ()*/
  {.w_px = 18,	.glyph_index = 6120},	/*Unicode: U+f0c4 ()*/
  {.w_px = 16,	.glyph_index = 6300},	/*Unicode: U+f0c5 ()*/
  {.w_px = 17,	.glyph_index = 6460},	/*Unicode: U+f0c7 ()*/
  {.w_px = 8,	.glyph_index = 6640},	/*Unicode: U+f0e7 ()*/
  {.w_px = 12,	.glyph_index = 6720},	/*Unicode: U+f0f3 ()*/
  {.w_px = 23,	.glyph_index = 6840},	/*Unicode: U+f11c ()*/
  {.w_px = 18,	.glyph_index = 7080},	/*Unicode: U+f124 ()*/
  {.w_px = 13,	.glyph_index = 7260},	/*Unicode: U+f15b ()*/
  {.w_px = 20,	.glyph_index = 7400},	/*Unicode: U+f1eb ()*/
  {.w_px = 26,	.glyph_index = 7600},	/*Unicode: U+f240 ()*/
  {.w_px = 26,	.glyph_index = 7860},	/*Unicode: U+f241 ()*/
  {.w_px = 26,	.glyph_index = 8120},	/*Unicode: U+f242 ()*/
  {.w_px = 26,	.glyph_index = 8380},	/*Unicode: U+f243 ()*/
  {.w_px = 26,	.glyph_index = 8640},	/*Unicode: U+f244 ()*/
  {.w_px = 20,	.glyph_index = 8900},	/*Unicode: U+f293 ()*/

#elif USE_HEKATE_SYMBOL_20 == 8
  {.w_px = 5,	.glyph_index = 0},	/*Unicode: U+f001 ()*/
  {.w_px = 16,	.glyph_index = 100},	/*Unicode: U+f008 ()*/
  {.w_px = 20,	.glyph_index = 420},	/*Unicode: U+f00b ()*/
  {.w_px = 22,	.glyph_index = 820},	/*Unicode: U+f00c ()*/
  {.w_px = 16,	.glyph_index = 1260},	/*Unicode: U+f00d ()*/
  {.w_px = 18,	.glyph_index = 1580},	/*Unicode: U+f011 ()*/
  {.w_px = 18,	.glyph_index = 1940},	/*Unicode: U+f013 ()*/
  {.w_px = 16,	.glyph_index = 2300},	/*Unicode: U+f014 ()*/
  {.w_px = 23,	.glyph_index = 2620},	/*Unicode: U+f015 ()*/
  {.w_px = 18,	.glyph_index = 3080},	/*Unicode: U+f019 ()*/
  {.w_px = 23,	.glyph_index = 3440},	/*Unicode: U+f01c ()*/
  {.w_px = 18,	.glyph_index = 3900},	/*Unicode: U+f021 ()*/
  {.w_px = 18,	.glyph_index = 4260},	/*Unicode: U+f026 ()*/
  {.w_px = 18,	.glyph_index = 4620},	/*Unicode: U+f027 ()*/
  {.w_px = 13,	.glyph_index = 4980},	/*Unicode: U+f028 ()*/
  {.w_px = 13,	.glyph_index = 5240},	/*Unicode: U+f03e ()*/
  {.w_px = 16,	.glyph_index = 5500},	/*Unicode: U+f040 ()*/
  {.w_px = 13,	.glyph_index = 5820},	/*Unicode: U+f048 ()*/
  {.w_px = 13,	.glyph_index = 6080},	/*Unicode: U+f04b ()*/
  {.w_px = 13,	.glyph_index = 6340},	/*Unicode: U+f04c ()*/
  {.w_px = 9,	.glyph_index = 6600},	/*Unicode: U+f04d ()*/
  {.w_px = 23,	.glyph_index = 6780},	/*Unicode: U+f051 ()*/
  {.w_px = 21,	.glyph_index = 7240},	/*Unicode: U+f052 ()*/
  {.w_px = 11,	.glyph_index = 7660},	/*Unicode: U+f053 ()*/
  {.w_px = 11,	.glyph_index = 7880},	/*Unicode: U+f054 ()*/
  {.w_px = 18,	.glyph_index = 8100},	/*Unicode: U+f067 ()*/
  {.w_px = 18,	.glyph_index = 8460},	/*Unicode: U+f068 ()*/
  {.w_px = 20,	.glyph_index = 8820},	/*Unicode: U+f071 ()*/
  {.w_px = 20,	.glyph_index = 9220},	/*Unicode: U+f074 ()*/
  {.w_px = 18,	.glyph_index = 9620},	/*Unicode: U+f077 ()*/
  {.w_px = 18,	.glyph_index = 9980},	/*Unicode: U+f078 ()*/
  {.w_px = 18,	.glyph_index = 10340},	/*Unicode: U+f079 ()*/
  {.w_px = 20,	.glyph_index = 10700},	/*Unicode: U+f07b ()*/
  {.w_px = 18,	.glyph_index = 11100},	/*Unicode: U+f093 ()*/
  {.w_px = 25,	.glyph_index = 11460},	/*Unicode: U+f095 ()*/
  {.w_px = 18,	.glyph_index = 11960},	/*Unicode: U+f0c4 ()*/
  {.w_px = 16,	.glyph_index = 12320},	/*Unicode: U+f0c5 ()*/
  {.w_px = 17,	.glyph_index = 12640},	/*Unicode: U+f0c7 ()*/
  {.w_px = 8,	.glyph_index = 12980},	/*Unicode: U+f0e7 ()*/
  {.w_px = 12,	.glyph_index = 13140},	/*Unicode: U+f0f3 ()*/
  {.w_px = 23,	.glyph_index = 13380},	/*Unicode: U+f11c ()*/
  {.w_px = 18,	.glyph_index = 13840},	/*Unicode: U+f124 ()*/
  {.w_px = 13,	.glyph_index = 14200},	/*Unicode: U+f15b ()*/
  {.w_px = 20,	.glyph_index = 14460},	/*Unicode: U+f1eb ()*/
  {.w_px = 26,	.glyph_index = 14860},	/*Unicode: U+f240 ()*/
  {.w_px = 26,	.glyph_index = 15380},	/*Unicode: U+f241 ()*/
  {.w_px = 26,	.glyph_index = 15900},	/*Unicode: U+f242 ()*/
  {.w_px = 26,	.glyph_index = 16420},	/*Unicode: U+f243 ()*/
  {.w_px = 26,	.glyph_index = 16940},	/*Unicode: U+f244 ()*/
  {.w_px = 20,	.glyph_index = 17460},	/*Unicode: U+f293 ()*/

#endif
};

lv_font_t hekate_symbol_20 =
{
    .unicode_first = LV_SYMBOL_GLYPH_FIRST,	/*First Unicode letter in this font*/
    .unicode_last = LV_SYMBOL_GLYPH_LAST,	/*Last Unicode letter in this font*/
    .h_px = 20,				/*Font height in pixels*/
    //.glyph_bitmap = hekate_symbol_20_glyph_bitmap,	/*Bitmap of glyphs*/
    .glyph_bitmap = (const uint8_t *)(NYX_RES_ADDR + 0xFC00),
    .glyph_dsc = hekate_symbol_20_glyph_dsc,		/*Description of glyphs*/
    .glyph_cnt = 50,			/*Number of glyphs in the font*/
    .unicode_list = NULL,	/*List of unicode characters*/
    .get_bitmap = lv_font_get_bitmap_continuous,	/*Function pointer to get glyph's bitmap*/
    .get_width = lv_font_get_width_continuous,	/*Function pointer to get glyph's width*/
#if USE_HEKATE_SYMBOL_20 == 4
    .bpp = 4,				/*Bit per pixel*/
#elif USE_HEKATE_SYMBOL_20 == 8
    .bpp = 8,				/*Bit per pixel*/
#endif
    .monospace = 0,				/*Fix width (0: if not used)*/
    .next_page = NULL,		/*Pointer to a font extension*/
};

#endif /*USE_HEKATE_SYMBOL_20*/
