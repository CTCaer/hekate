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

#if USE_INTERUI_30 != 0	/*Can be enabled in lv_conf.h*/

/***********************************************************************************
 * Inter-UI-Regular-stretched.ttf 30 px Font in U+0020 ( ) .. U+007e (~)  range with all bpp
***********************************************************************************/

/*Store the glyph descriptions*/
static const lv_font_glyph_dsc_t interui_30_glyph_dsc[] =
{
#if USE_INTERUI_30 == 4
  {.w_px = 8,	.glyph_index = 0},	/*Unicode: U+0020 ( )*/
  {.w_px = 3,	.glyph_index = 120},	/*Unicode: U+0021 (!)*/
  {.w_px = 6,	.glyph_index = 180},	/*Unicode: U+0022 (")*/
  {.w_px = 13,	.glyph_index = 270},	/*Unicode: U+0023 (#)*/
  {.w_px = 12,	.glyph_index = 480},	/*Unicode: U+0024 ($)*/
  {.w_px = 18,	.glyph_index = 660},	/*Unicode: U+0025 (%)*/
  {.w_px = 16,	.glyph_index = 930},	/*Unicode: U+0026 (&)*/
  {.w_px = 3,	.glyph_index = 1170},	/*Unicode: U+0027 (')*/
  {.w_px = 6,	.glyph_index = 1230},	/*Unicode: U+0028 (()*/
  {.w_px = 7,	.glyph_index = 1320},	/*Unicode: U+0029 ())*/
  {.w_px = 9,	.glyph_index = 1440},	/*Unicode: U+002a (*)*/
  {.w_px = 16,	.glyph_index = 1590},	/*Unicode: U+002b (+)*/
  {.w_px = 3,	.glyph_index = 1830},	/*Unicode: U+002c (,)*/
  {.w_px = 8,	.glyph_index = 1890},	/*Unicode: U+002d (-)*/
  {.w_px = 3,	.glyph_index = 2010},	/*Unicode: U+002e (.)*/
  {.w_px = 11,	.glyph_index = 2070},	/*Unicode: U+002f (/)*/
  {.w_px = 13,	.glyph_index = 2250},	/*Unicode: U+0030 (0)*/
  {.w_px = 7,	.glyph_index = 2460},	/*Unicode: U+0031 (1)*/
  {.w_px = 13,	.glyph_index = 2580},	/*Unicode: U+0032 (2)*/
  {.w_px = 14,	.glyph_index = 2790},	/*Unicode: U+0033 (3)*/
  {.w_px = 13,	.glyph_index = 3000},	/*Unicode: U+0034 (4)*/
  {.w_px = 14,	.glyph_index = 3210},	/*Unicode: U+0035 (5)*/
  {.w_px = 13,	.glyph_index = 3420},	/*Unicode: U+0036 (6)*/
  {.w_px = 13,	.glyph_index = 3630},	/*Unicode: U+0037 (7)*/
  {.w_px = 13,	.glyph_index = 3840},	/*Unicode: U+0038 (8)*/
  {.w_px = 13,	.glyph_index = 4050},	/*Unicode: U+0039 (9)*/
  {.w_px = 3,	.glyph_index = 4260},	/*Unicode: U+003a (:)*/
  {.w_px = 3,	.glyph_index = 4320},	/*Unicode: U+003b (;)*/
  {.w_px = 18,	.glyph_index = 4380},	/*Unicode: U+003c (<)*/
  {.w_px = 16,	.glyph_index = 4650},	/*Unicode: U+003d (=)*/
  {.w_px = 18,	.glyph_index = 4890},	/*Unicode: U+003e (>)*/
  {.w_px = 12,	.glyph_index = 5160},	/*Unicode: U+003f (?)*/
  {.w_px = 18,	.glyph_index = 5340},	/*Unicode: U+0040 (@)*/
  {.w_px = 17,	.glyph_index = 5610},	/*Unicode: U+0041 (A)*/
  {.w_px = 13,	.glyph_index = 5880},	/*Unicode: U+0042 (B)*/
  {.w_px = 15,	.glyph_index = 6090},	/*Unicode: U+0043 (C)*/
  {.w_px = 17,	.glyph_index = 6330},	/*Unicode: U+0044 (D)*/
  {.w_px = 12,	.glyph_index = 6600},	/*Unicode: U+0045 (E)*/
  {.w_px = 11,	.glyph_index = 6780},	/*Unicode: U+0046 (F)*/
  {.w_px = 17,	.glyph_index = 6960},	/*Unicode: U+0047 (G)*/
  {.w_px = 14,	.glyph_index = 7230},	/*Unicode: U+0048 (H)*/
  {.w_px = 3,	.glyph_index = 7440},	/*Unicode: U+0049 (I)*/
  {.w_px = 11,	.glyph_index = 7500},	/*Unicode: U+004a (J)*/
  {.w_px = 14,	.glyph_index = 7680},	/*Unicode: U+004b (K)*/
  {.w_px = 9,	.glyph_index = 7890},	/*Unicode: U+004c (L)*/
  {.w_px = 23,	.glyph_index = 8040},	/*Unicode: U+004d (M)*/
  {.w_px = 16,	.glyph_index = 8400},	/*Unicode: U+004e (N)*/
  {.w_px = 19,	.glyph_index = 8640},	/*Unicode: U+004f (O)*/
  {.w_px = 11,	.glyph_index = 8940},	/*Unicode: U+0050 (P)*/
  {.w_px = 19,	.glyph_index = 9120},	/*Unicode: U+0051 (Q)*/
  {.w_px = 13,	.glyph_index = 9420},	/*Unicode: U+0052 (R)*/
  {.w_px = 12,	.glyph_index = 9630},	/*Unicode: U+0053 (S)*/
  {.w_px = 14,	.glyph_index = 9810},	/*Unicode: U+0054 (T)*/
  {.w_px = 16,	.glyph_index = 10020},	/*Unicode: U+0055 (U)*/
  {.w_px = 18,	.glyph_index = 10260},	/*Unicode: U+0056 (V)*/
  {.w_px = 27,	.glyph_index = 10530},	/*Unicode: U+0057 (W)*/
  {.w_px = 15,	.glyph_index = 10950},	/*Unicode: U+0058 (X)*/
  {.w_px = 15,	.glyph_index = 11190},	/*Unicode: U+0059 (Y)*/
  {.w_px = 15,	.glyph_index = 11430},	/*Unicode: U+005a (Z)*/
  {.w_px = 6,	.glyph_index = 11670},	/*Unicode: U+005b ([)*/
  {.w_px = 11,	.glyph_index = 11760},	/*Unicode: U+005c (\)*/
  {.w_px = 7,	.glyph_index = 11940},	/*Unicode: U+005d (])*/
  {.w_px = 13,	.glyph_index = 12060},	/*Unicode: U+005e (^)*/
  {.w_px = 12,	.glyph_index = 12270},	/*Unicode: U+005f (_)*/
  {.w_px = 6,	.glyph_index = 12450},	/*Unicode: U+0060 (`)*/
  {.w_px = 12,	.glyph_index = 12540},	/*Unicode: U+0061 (a)*/
  {.w_px = 13,	.glyph_index = 12720},	/*Unicode: U+0062 (b)*/
  {.w_px = 11,	.glyph_index = 12930},	/*Unicode: U+0063 (c)*/
  {.w_px = 13,	.glyph_index = 13110},	/*Unicode: U+0064 (d)*/
  {.w_px = 12,	.glyph_index = 13320},	/*Unicode: U+0065 (e)*/
  {.w_px = 6,	.glyph_index = 13500},	/*Unicode: U+0066 (f)*/
  {.w_px = 12,	.glyph_index = 13590},	/*Unicode: U+0067 (g)*/
  {.w_px = 11,	.glyph_index = 13770},	/*Unicode: U+0068 (h)*/
  {.w_px = 3,	.glyph_index = 13950},	/*Unicode: U+0069 (i)*/
  {.w_px = 7,	.glyph_index = 14010},	/*Unicode: U+006a (j)*/
  {.w_px = 12,	.glyph_index = 14130},	/*Unicode: U+006b (k)*/
  {.w_px = 4,	.glyph_index = 14310},	/*Unicode: U+006c (l)*/
  {.w_px = 19,	.glyph_index = 14370},	/*Unicode: U+006d (m)*/
  {.w_px = 11,	.glyph_index = 14670},	/*Unicode: U+006e (n)*/
  {.w_px = 14,	.glyph_index = 14850},	/*Unicode: U+006f (o)*/
  {.w_px = 13,	.glyph_index = 15060},	/*Unicode: U+0070 (p)*/
  {.w_px = 13,	.glyph_index = 15270},	/*Unicode: U+0071 (q)*/
  {.w_px = 7,	.glyph_index = 15480},	/*Unicode: U+0072 (r)*/
  {.w_px = 11,	.glyph_index = 15600},	/*Unicode: U+0073 (s)*/
  {.w_px = 8,	.glyph_index = 15780},	/*Unicode: U+0074 (t)*/
  {.w_px = 11,	.glyph_index = 15900},	/*Unicode: U+0075 (u)*/
  {.w_px = 12,	.glyph_index = 16080},	/*Unicode: U+0076 (v)*/
  {.w_px = 21,	.glyph_index = 16260},	/*Unicode: U+0077 (w)*/
  {.w_px = 13,	.glyph_index = 16590},	/*Unicode: U+0078 (x)*/
  {.w_px = 13,	.glyph_index = 16800},	/*Unicode: U+0079 (y)*/
  {.w_px = 10,	.glyph_index = 17010},	/*Unicode: U+007a (z)*/
  {.w_px = 6,	.glyph_index = 17160},	/*Unicode: U+007b ({)*/
  {.w_px = 3,	.glyph_index = 17250},	/*Unicode: U+007c (|)*/
  {.w_px = 5,	.glyph_index = 17310},	/*Unicode: U+007d (})*/
  {.w_px = 8,	.glyph_index = 17400},	/*Unicode: U+007e (~)*/

#elif USE_INTERUI_30 == 8
  {.w_px = 8,	.glyph_index = 0},	/*Unicode: U+0020 ( )*/
  {.w_px = 3,	.glyph_index = 240},	/*Unicode: U+0021 (!)*/
  {.w_px = 6,	.glyph_index = 330},	/*Unicode: U+0022 (")*/
  {.w_px = 13,	.glyph_index = 510},	/*Unicode: U+0023 (#)*/
  {.w_px = 12,	.glyph_index = 900},	/*Unicode: U+0024 ($)*/
  {.w_px = 18,	.glyph_index = 1260},	/*Unicode: U+0025 (%)*/
  {.w_px = 16,	.glyph_index = 1800},	/*Unicode: U+0026 (&)*/
  {.w_px = 3,	.glyph_index = 2280},	/*Unicode: U+0027 (')*/
  {.w_px = 6,	.glyph_index = 2370},	/*Unicode: U+0028 (()*/
  {.w_px = 7,	.glyph_index = 2550},	/*Unicode: U+0029 ())*/
  {.w_px = 9,	.glyph_index = 2760},	/*Unicode: U+002a (*)*/
  {.w_px = 16,	.glyph_index = 3030},	/*Unicode: U+002b (+)*/
  {.w_px = 3,	.glyph_index = 3510},	/*Unicode: U+002c (,)*/
  {.w_px = 8,	.glyph_index = 3600},	/*Unicode: U+002d (-)*/
  {.w_px = 3,	.glyph_index = 3840},	/*Unicode: U+002e (.)*/
  {.w_px = 11,	.glyph_index = 3930},	/*Unicode: U+002f (/)*/
  {.w_px = 13,	.glyph_index = 4260},	/*Unicode: U+0030 (0)*/
  {.w_px = 7,	.glyph_index = 4650},	/*Unicode: U+0031 (1)*/
  {.w_px = 13,	.glyph_index = 4860},	/*Unicode: U+0032 (2)*/
  {.w_px = 14,	.glyph_index = 5250},	/*Unicode: U+0033 (3)*/
  {.w_px = 13,	.glyph_index = 5670},	/*Unicode: U+0034 (4)*/
  {.w_px = 14,	.glyph_index = 6060},	/*Unicode: U+0035 (5)*/
  {.w_px = 13,	.glyph_index = 6480},	/*Unicode: U+0036 (6)*/
  {.w_px = 13,	.glyph_index = 6870},	/*Unicode: U+0037 (7)*/
  {.w_px = 13,	.glyph_index = 7260},	/*Unicode: U+0038 (8)*/
  {.w_px = 13,	.glyph_index = 7650},	/*Unicode: U+0039 (9)*/
  {.w_px = 3,	.glyph_index = 8040},	/*Unicode: U+003a (:)*/
  {.w_px = 3,	.glyph_index = 8130},	/*Unicode: U+003b (;)*/
  {.w_px = 18,	.glyph_index = 8220},	/*Unicode: U+003c (<)*/
  {.w_px = 16,	.glyph_index = 8760},	/*Unicode: U+003d (=)*/
  {.w_px = 18,	.glyph_index = 9240},	/*Unicode: U+003e (>)*/
  {.w_px = 12,	.glyph_index = 9780},	/*Unicode: U+003f (?)*/
  {.w_px = 18,	.glyph_index = 10140},	/*Unicode: U+0040 (@)*/
  {.w_px = 17,	.glyph_index = 10680},	/*Unicode: U+0041 (A)*/
  {.w_px = 13,	.glyph_index = 11190},	/*Unicode: U+0042 (B)*/
  {.w_px = 15,	.glyph_index = 11580},	/*Unicode: U+0043 (C)*/
  {.w_px = 17,	.glyph_index = 12030},	/*Unicode: U+0044 (D)*/
  {.w_px = 12,	.glyph_index = 12540},	/*Unicode: U+0045 (E)*/
  {.w_px = 11,	.glyph_index = 12900},	/*Unicode: U+0046 (F)*/
  {.w_px = 17,	.glyph_index = 13230},	/*Unicode: U+0047 (G)*/
  {.w_px = 14,	.glyph_index = 13740},	/*Unicode: U+0048 (H)*/
  {.w_px = 3,	.glyph_index = 14160},	/*Unicode: U+0049 (I)*/
  {.w_px = 11,	.glyph_index = 14250},	/*Unicode: U+004a (J)*/
  {.w_px = 14,	.glyph_index = 14580},	/*Unicode: U+004b (K)*/
  {.w_px = 9,	.glyph_index = 15000},	/*Unicode: U+004c (L)*/
  {.w_px = 23,	.glyph_index = 15270},	/*Unicode: U+004d (M)*/
  {.w_px = 16,	.glyph_index = 15960},	/*Unicode: U+004e (N)*/
  {.w_px = 19,	.glyph_index = 16440},	/*Unicode: U+004f (O)*/
  {.w_px = 11,	.glyph_index = 17010},	/*Unicode: U+0050 (P)*/
  {.w_px = 19,	.glyph_index = 17340},	/*Unicode: U+0051 (Q)*/
  {.w_px = 13,	.glyph_index = 17910},	/*Unicode: U+0052 (R)*/
  {.w_px = 12,	.glyph_index = 18300},	/*Unicode: U+0053 (S)*/
  {.w_px = 14,	.glyph_index = 18660},	/*Unicode: U+0054 (T)*/
  {.w_px = 16,	.glyph_index = 19080},	/*Unicode: U+0055 (U)*/
  {.w_px = 18,	.glyph_index = 19560},	/*Unicode: U+0056 (V)*/
  {.w_px = 27,	.glyph_index = 20100},	/*Unicode: U+0057 (W)*/
  {.w_px = 15,	.glyph_index = 20910},	/*Unicode: U+0058 (X)*/
  {.w_px = 15,	.glyph_index = 21360},	/*Unicode: U+0059 (Y)*/
  {.w_px = 15,	.glyph_index = 21810},	/*Unicode: U+005a (Z)*/
  {.w_px = 6,	.glyph_index = 22260},	/*Unicode: U+005b ([)*/
  {.w_px = 11,	.glyph_index = 22440},	/*Unicode: U+005c (\)*/
  {.w_px = 7,	.glyph_index = 22770},	/*Unicode: U+005d (])*/
  {.w_px = 13,	.glyph_index = 22980},	/*Unicode: U+005e (^)*/
  {.w_px = 12,	.glyph_index = 23370},	/*Unicode: U+005f (_)*/
  {.w_px = 6,	.glyph_index = 23730},	/*Unicode: U+0060 (`)*/
  {.w_px = 12,	.glyph_index = 23910},	/*Unicode: U+0061 (a)*/
  {.w_px = 13,	.glyph_index = 24270},	/*Unicode: U+0062 (b)*/
  {.w_px = 11,	.glyph_index = 24660},	/*Unicode: U+0063 (c)*/
  {.w_px = 13,	.glyph_index = 24990},	/*Unicode: U+0064 (d)*/
  {.w_px = 12,	.glyph_index = 25380},	/*Unicode: U+0065 (e)*/
  {.w_px = 6,	.glyph_index = 25740},	/*Unicode: U+0066 (f)*/
  {.w_px = 12,	.glyph_index = 25920},	/*Unicode: U+0067 (g)*/
  {.w_px = 11,	.glyph_index = 26280},	/*Unicode: U+0068 (h)*/
  {.w_px = 3,	.glyph_index = 26610},	/*Unicode: U+0069 (i)*/
  {.w_px = 7,	.glyph_index = 26700},	/*Unicode: U+006a (j)*/
  {.w_px = 12,	.glyph_index = 26910},	/*Unicode: U+006b (k)*/
  {.w_px = 4,	.glyph_index = 27270},	/*Unicode: U+006c (l)*/
  {.w_px = 19,	.glyph_index = 27390},	/*Unicode: U+006d (m)*/
  {.w_px = 11,	.glyph_index = 27960},	/*Unicode: U+006e (n)*/
  {.w_px = 14,	.glyph_index = 28290},	/*Unicode: U+006f (o)*/
  {.w_px = 13,	.glyph_index = 28710},	/*Unicode: U+0070 (p)*/
  {.w_px = 13,	.glyph_index = 29100},	/*Unicode: U+0071 (q)*/
  {.w_px = 7,	.glyph_index = 29490},	/*Unicode: U+0072 (r)*/
  {.w_px = 11,	.glyph_index = 29700},	/*Unicode: U+0073 (s)*/
  {.w_px = 8,	.glyph_index = 30030},	/*Unicode: U+0074 (t)*/
  {.w_px = 11,	.glyph_index = 30270},	/*Unicode: U+0075 (u)*/
  {.w_px = 12,	.glyph_index = 30600},	/*Unicode: U+0076 (v)*/
  {.w_px = 21,	.glyph_index = 30960},	/*Unicode: U+0077 (w)*/
  {.w_px = 13,	.glyph_index = 31590},	/*Unicode: U+0078 (x)*/
  {.w_px = 13,	.glyph_index = 31980},	/*Unicode: U+0079 (y)*/
  {.w_px = 10,	.glyph_index = 32370},	/*Unicode: U+007a (z)*/
  {.w_px = 6,	.glyph_index = 32670},	/*Unicode: U+007b ({)*/
  {.w_px = 3,	.glyph_index = 32850},	/*Unicode: U+007c (|)*/
  {.w_px = 5,	.glyph_index = 32940},	/*Unicode: U+007d (})*/
  {.w_px = 8,	.glyph_index = 33090},	/*Unicode: U+007e (~)*/

#endif
};

lv_font_t interui_30 =
{
    .unicode_first = 32,	/*First Unicode letter in this font*/
    .unicode_last = 126,	/*Last Unicode letter in this font*/
    .h_px = 30,				/*Font height in pixels*/
    //.glyph_bitmap = interui_30_glyph_bitmap,	/*Bitmap of glyphs*/
    .glyph_bitmap = (const uint8_t *)(NYX_RES_ADDR + 0x7900),
    .glyph_dsc = interui_30_glyph_dsc,		/*Description of glyphs*/
    .glyph_cnt = 95,			/*Number of glyphs in the font*/
    .unicode_list = NULL,	/*Every character in the font from 'unicode_first' to 'unicode_last'*/
    .get_bitmap = lv_font_get_bitmap_continuous,	/*Function pointer to get glyph's bitmap*/
    .get_width = lv_font_get_width_continuous,	/*Function pointer to get glyph's width*/
#if USE_INTERUI_30 == 4
    .bpp = 4,				/*Bit per pixel*/
#elif USE_INTERUI_30 == 8
    .bpp = 8,				/*Bit per pixel*/
#endif
    .monospace = 0,				/*Fix width (0: if not used)*/
    .next_page = NULL,		/*Pointer to a font extension*/
};

#endif /*USE_INTERUI_30*/
