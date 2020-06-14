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

#if USE_HEKATE_SYMBOL_120 != 0	/*Can be enabled in lv_conf.h*/

/***********************************************************************************
 * hekate-symbols-huge.ttf 120 px Font in U+f002 () .. U+f007 ()  range with all bpp
 * Sparse font with only these characters: 
***********************************************************************************/

/*Store the glyph descriptions*/
static const lv_font_glyph_dsc_t hekate_symbol_120_glyph_dsc[] =
{
#if USE_HEKATE_SYMBOL_120 == 8
  {.w_px = 103,	.glyph_index = 0},	/*Unicode: U+f002 ()*/
  {.w_px = 103,	.glyph_index = 12360},	/*Unicode: U+f003 ()*/
  {.w_px = 103,	.glyph_index = 24720},	/*Unicode: U+f005 ()*/
  {.w_px = 103,	.glyph_index = 37080},	/*Unicode: U+f007 ()*/

#endif
};

lv_font_t hekate_symbol_120 =
{
    .unicode_first = LV_SYMBOL_GLYPH_FIRST,	/*First Unicode letter in this font*/
    .unicode_last = LV_SYMBOL_GLYPH_LAST,	/*Last Unicode letter in this font*/
    .h_px = 120,				/*Font height in pixels*/
    .glyph_bitmap = (const uint8_t *)(NYX_RES_ADDR + 0x36E00),	/*Bitmap of glyphs*/
    .glyph_dsc = hekate_symbol_120_glyph_dsc,		/*Description of glyphs*/
    .glyph_cnt = 4,			/*Number of glyphs in the font*/
    .unicode_list = NULL,	/*List of unicode characters*/
    .get_bitmap = lv_font_get_bitmap_continuous,	/*Function pointer to get glyph's bitmap*/
    .get_width = lv_font_get_width_continuous,	/*Function pointer to get glyph's width*/
#if USE_HEKATE_SYMBOL_120 == 8
    .bpp = 8,				/*Bit per pixel*/
#endif
    .monospace = 0,				/*Fix width (0: if not used)*/
    .next_page = NULL,		/*Pointer to a font extension*/
};

#endif /*USE_HEKATE_SYMBOL_100*/
