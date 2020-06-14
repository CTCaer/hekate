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

#if USE_INTERUI_20 != 0	/*Can be enabled in lv_conf.h*/

/***********************************************************************************
 * Inter-UI-Regular-stretched.ttf 20 px Font in U+0020 ( ) .. U+007e (~)  range with all bpp
***********************************************************************************/

/*Store the glyph descriptions*/
static const lv_font_glyph_dsc_t interui_20_glyph_dsc[] =
{
#if USE_INTERUI_20 == 4
  {.w_px = 6,	.glyph_index = 0},	/*Unicode: U+0020 ( )*/
  {.w_px = 3,	.glyph_index = 60},	/*Unicode: U+0021 (!)*/
  {.w_px = 5,	.glyph_index = 100},	/*Unicode: U+0022 (")*/
  {.w_px = 8,	.glyph_index = 160},	/*Unicode: U+0023 (#)*/
  {.w_px = 9,	.glyph_index = 240},	/*Unicode: U+0024 ($)*/
  {.w_px = 14,	.glyph_index = 340},	/*Unicode: U+0025 (%)*/
  {.w_px = 12,	.glyph_index = 480},	/*Unicode: U+0026 (&)*/
  {.w_px = 3,	.glyph_index = 600},	/*Unicode: U+0027 (')*/
  {.w_px = 5,	.glyph_index = 640},	/*Unicode: U+0028 (()*/
  {.w_px = 5,	.glyph_index = 700},	/*Unicode: U+0029 ())*/
  {.w_px = 7,	.glyph_index = 760},	/*Unicode: U+002a (*)*/
  {.w_px = 10,	.glyph_index = 840},	/*Unicode: U+002b (+)*/
  {.w_px = 3,	.glyph_index = 940},	/*Unicode: U+002c (,)*/
  {.w_px = 5,	.glyph_index = 980},	/*Unicode: U+002d (-)*/
  {.w_px = 3,	.glyph_index = 1040},	/*Unicode: U+002e (.)*/
  {.w_px = 8,	.glyph_index = 1080},	/*Unicode: U+002f (/)*/
  {.w_px = 10,	.glyph_index = 1160},	/*Unicode: U+0030 (0)*/
  {.w_px = 5,	.glyph_index = 1260},	/*Unicode: U+0031 (1)*/
  {.w_px = 9,	.glyph_index = 1320},	/*Unicode: U+0032 (2)*/
  {.w_px = 10,	.glyph_index = 1420},	/*Unicode: U+0033 (3)*/
  {.w_px = 9,	.glyph_index = 1520},	/*Unicode: U+0034 (4)*/
  {.w_px = 9,	.glyph_index = 1620},	/*Unicode: U+0035 (5)*/
  {.w_px = 10,	.glyph_index = 1720},	/*Unicode: U+0036 (6)*/
  {.w_px = 8,	.glyph_index = 1820},	/*Unicode: U+0037 (7)*/
  {.w_px = 10,	.glyph_index = 1900},	/*Unicode: U+0038 (8)*/
  {.w_px = 10,	.glyph_index = 2000},	/*Unicode: U+0039 (9)*/
  {.w_px = 3,	.glyph_index = 2100},	/*Unicode: U+003a (:)*/
  {.w_px = 3,	.glyph_index = 2140},	/*Unicode: U+003b (;)*/
  {.w_px = 12,	.glyph_index = 2180},	/*Unicode: U+003c (<)*/
  {.w_px = 10,	.glyph_index = 2300},	/*Unicode: U+003d (=)*/
  {.w_px = 12,	.glyph_index = 2400},	/*Unicode: U+003e (>)*/
  {.w_px = 8,	.glyph_index = 2520},	/*Unicode: U+003f (?)*/
  {.w_px = 14,	.glyph_index = 2600},	/*Unicode: U+0040 (@)*/
  {.w_px = 12,	.glyph_index = 2740},	/*Unicode: U+0041 (A)*/
  {.w_px = 10,	.glyph_index = 2860},	/*Unicode: U+0042 (B)*/
  {.w_px = 11,	.glyph_index = 2960},	/*Unicode: U+0043 (C)*/
  {.w_px = 12,	.glyph_index = 3080},	/*Unicode: U+0044 (D)*/
  {.w_px = 9,	.glyph_index = 3200},	/*Unicode: U+0045 (E)*/
  {.w_px = 8,	.glyph_index = 3300},	/*Unicode: U+0046 (F)*/
  {.w_px = 12,	.glyph_index = 3380},	/*Unicode: U+0047 (G)*/
  {.w_px = 10,	.glyph_index = 3500},	/*Unicode: U+0048 (H)*/
  {.w_px = 3,	.glyph_index = 3600},	/*Unicode: U+0049 (I)*/
  {.w_px = 7,	.glyph_index = 3640},	/*Unicode: U+004a (J)*/
  {.w_px = 10,	.glyph_index = 3720},	/*Unicode: U+004b (K)*/
  {.w_px = 7,	.glyph_index = 3820},	/*Unicode: U+004c (L)*/
  {.w_px = 15,	.glyph_index = 3900},	/*Unicode: U+004d (M)*/
  {.w_px = 12,	.glyph_index = 4060},	/*Unicode: U+004e (N)*/
  {.w_px = 14,	.glyph_index = 4180},	/*Unicode: U+004f (O)*/
  {.w_px = 9,	.glyph_index = 4320},	/*Unicode: U+0050 (P)*/
  {.w_px = 14,	.glyph_index = 4420},	/*Unicode: U+0051 (Q)*/
  {.w_px = 9,	.glyph_index = 4560},	/*Unicode: U+0052 (R)*/
  {.w_px = 9,	.glyph_index = 4660},	/*Unicode: U+0053 (S)*/
  {.w_px = 9,	.glyph_index = 4760},	/*Unicode: U+0054 (T)*/
  {.w_px = 12,	.glyph_index = 4860},	/*Unicode: U+0055 (U)*/
  {.w_px = 12,	.glyph_index = 4980},	/*Unicode: U+0056 (V)*/
  {.w_px = 18,	.glyph_index = 5100},	/*Unicode: U+0057 (W)*/
  {.w_px = 10,	.glyph_index = 5280},	/*Unicode: U+0058 (X)*/
  {.w_px = 10,	.glyph_index = 5380},	/*Unicode: U+0059 (Y)*/
  {.w_px = 10,	.glyph_index = 5480},	/*Unicode: U+005a (Z)*/
  {.w_px = 5,	.glyph_index = 5580},	/*Unicode: U+005b ([)*/
  {.w_px = 8,	.glyph_index = 5640},	/*Unicode: U+005c (\)*/
  {.w_px = 5,	.glyph_index = 5720},	/*Unicode: U+005d (])*/
  {.w_px = 9,	.glyph_index = 5780},	/*Unicode: U+005e (^)*/
  {.w_px = 8,	.glyph_index = 5880},	/*Unicode: U+005f (_)*/
  {.w_px = 4,	.glyph_index = 5960},	/*Unicode: U+0060 (`)*/
  {.w_px = 9,	.glyph_index = 6000},	/*Unicode: U+0061 (a)*/
  {.w_px = 10,	.glyph_index = 6100},	/*Unicode: U+0062 (b)*/
  {.w_px = 8,	.glyph_index = 6200},	/*Unicode: U+0063 (c)*/
  {.w_px = 10,	.glyph_index = 6280},	/*Unicode: U+0064 (d)*/
  {.w_px = 9,	.glyph_index = 6380},	/*Unicode: U+0065 (e)*/
  {.w_px = 4,	.glyph_index = 6480},	/*Unicode: U+0066 (f)*/
  {.w_px = 9,	.glyph_index = 6520},	/*Unicode: U+0067 (g)*/
  {.w_px = 9,	.glyph_index = 6620},	/*Unicode: U+0068 (h)*/
  {.w_px = 3,	.glyph_index = 6720},	/*Unicode: U+0069 (i)*/
  {.w_px = 5,	.glyph_index = 6760},	/*Unicode: U+006a (j)*/
  {.w_px = 8,	.glyph_index = 6820},	/*Unicode: U+006b (k)*/
  {.w_px = 4,	.glyph_index = 6900},	/*Unicode: U+006c (l)*/
  {.w_px = 13,	.glyph_index = 6940},	/*Unicode: U+006d (m)*/
  {.w_px = 9,	.glyph_index = 7080},	/*Unicode: U+006e (n)*/
  {.w_px = 10,	.glyph_index = 7180},	/*Unicode: U+006f (o)*/
  {.w_px = 10,	.glyph_index = 7280},	/*Unicode: U+0070 (p)*/
  {.w_px = 10,	.glyph_index = 7380},	/*Unicode: U+0071 (q)*/
  {.w_px = 5,	.glyph_index = 7480},	/*Unicode: U+0072 (r)*/
  {.w_px = 8,	.glyph_index = 7540},	/*Unicode: U+0073 (s)*/
  {.w_px = 5,	.glyph_index = 7620},	/*Unicode: U+0074 (t)*/
  {.w_px = 9,	.glyph_index = 7680},	/*Unicode: U+0075 (u)*/
  {.w_px = 8,	.glyph_index = 7780},	/*Unicode: U+0076 (v)*/
  {.w_px = 14,	.glyph_index = 7860},	/*Unicode: U+0077 (w)*/
  {.w_px = 8,	.glyph_index = 8000},	/*Unicode: U+0078 (x)*/
  {.w_px = 8,	.glyph_index = 8080},	/*Unicode: U+0079 (y)*/
  {.w_px = 7,	.glyph_index = 8160},	/*Unicode: U+007a (z)*/
  {.w_px = 4,	.glyph_index = 8240},	/*Unicode: U+007b ({)*/
  {.w_px = 3,	.glyph_index = 8280},	/*Unicode: U+007c (|)*/
  {.w_px = 4,	.glyph_index = 8320},	/*Unicode: U+007d (})*/
  {.w_px = 6,	.glyph_index = 8360},	/*Unicode: U+007e (~)*/

#elif USE_INTERUI_20 == 8
  {.w_px = 6,	.glyph_index = 0},	/*Unicode: U+0020 ( )*/
  {.w_px = 3,	.glyph_index = 120},	/*Unicode: U+0021 (!)*/
  {.w_px = 5,	.glyph_index = 180},	/*Unicode: U+0022 (")*/
  {.w_px = 8,	.glyph_index = 280},	/*Unicode: U+0023 (#)*/
  {.w_px = 9,	.glyph_index = 440},	/*Unicode: U+0024 ($)*/
  {.w_px = 14,	.glyph_index = 620},	/*Unicode: U+0025 (%)*/
  {.w_px = 12,	.glyph_index = 900},	/*Unicode: U+0026 (&)*/
  {.w_px = 3,	.glyph_index = 1140},	/*Unicode: U+0027 (')*/
  {.w_px = 5,	.glyph_index = 1200},	/*Unicode: U+0028 (()*/
  {.w_px = 5,	.glyph_index = 1300},	/*Unicode: U+0029 ())*/
  {.w_px = 7,	.glyph_index = 1400},	/*Unicode: U+002a (*)*/
  {.w_px = 10,	.glyph_index = 1540},	/*Unicode: U+002b (+)*/
  {.w_px = 3,	.glyph_index = 1740},	/*Unicode: U+002c (,)*/
  {.w_px = 5,	.glyph_index = 1800},	/*Unicode: U+002d (-)*/
  {.w_px = 3,	.glyph_index = 1900},	/*Unicode: U+002e (.)*/
  {.w_px = 8,	.glyph_index = 1960},	/*Unicode: U+002f (/)*/
  {.w_px = 10,	.glyph_index = 2120},	/*Unicode: U+0030 (0)*/
  {.w_px = 5,	.glyph_index = 2320},	/*Unicode: U+0031 (1)*/
  {.w_px = 9,	.glyph_index = 2420},	/*Unicode: U+0032 (2)*/
  {.w_px = 10,	.glyph_index = 2600},	/*Unicode: U+0033 (3)*/
  {.w_px = 9,	.glyph_index = 2800},	/*Unicode: U+0034 (4)*/
  {.w_px = 9,	.glyph_index = 2980},	/*Unicode: U+0035 (5)*/
  {.w_px = 10,	.glyph_index = 3160},	/*Unicode: U+0036 (6)*/
  {.w_px = 8,	.glyph_index = 3360},	/*Unicode: U+0037 (7)*/
  {.w_px = 10,	.glyph_index = 3520},	/*Unicode: U+0038 (8)*/
  {.w_px = 10,	.glyph_index = 3720},	/*Unicode: U+0039 (9)*/
  {.w_px = 3,	.glyph_index = 3920},	/*Unicode: U+003a (:)*/
  {.w_px = 3,	.glyph_index = 3980},	/*Unicode: U+003b (;)*/
  {.w_px = 12,	.glyph_index = 4040},	/*Unicode: U+003c (<)*/
  {.w_px = 10,	.glyph_index = 4280},	/*Unicode: U+003d (=)*/
  {.w_px = 12,	.glyph_index = 4480},	/*Unicode: U+003e (>)*/
  {.w_px = 8,	.glyph_index = 4720},	/*Unicode: U+003f (?)*/
  {.w_px = 14,	.glyph_index = 4880},	/*Unicode: U+0040 (@)*/
  {.w_px = 12,	.glyph_index = 5160},	/*Unicode: U+0041 (A)*/
  {.w_px = 10,	.glyph_index = 5400},	/*Unicode: U+0042 (B)*/
  {.w_px = 11,	.glyph_index = 5600},	/*Unicode: U+0043 (C)*/
  {.w_px = 12,	.glyph_index = 5820},	/*Unicode: U+0044 (D)*/
  {.w_px = 9,	.glyph_index = 6060},	/*Unicode: U+0045 (E)*/
  {.w_px = 8,	.glyph_index = 6240},	/*Unicode: U+0046 (F)*/
  {.w_px = 12,	.glyph_index = 6400},	/*Unicode: U+0047 (G)*/
  {.w_px = 10,	.glyph_index = 6640},	/*Unicode: U+0048 (H)*/
  {.w_px = 3,	.glyph_index = 6840},	/*Unicode: U+0049 (I)*/
  {.w_px = 7,	.glyph_index = 6900},	/*Unicode: U+004a (J)*/
  {.w_px = 10,	.glyph_index = 7040},	/*Unicode: U+004b (K)*/
  {.w_px = 7,	.glyph_index = 7240},	/*Unicode: U+004c (L)*/
  {.w_px = 15,	.glyph_index = 7380},	/*Unicode: U+004d (M)*/
  {.w_px = 12,	.glyph_index = 7680},	/*Unicode: U+004e (N)*/
  {.w_px = 14,	.glyph_index = 7920},	/*Unicode: U+004f (O)*/
  {.w_px = 9,	.glyph_index = 8200},	/*Unicode: U+0050 (P)*/
  {.w_px = 14,	.glyph_index = 8380},	/*Unicode: U+0051 (Q)*/
  {.w_px = 9,	.glyph_index = 8660},	/*Unicode: U+0052 (R)*/
  {.w_px = 9,	.glyph_index = 8840},	/*Unicode: U+0053 (S)*/
  {.w_px = 9,	.glyph_index = 9020},	/*Unicode: U+0054 (T)*/
  {.w_px = 12,	.glyph_index = 9200},	/*Unicode: U+0055 (U)*/
  {.w_px = 12,	.glyph_index = 9440},	/*Unicode: U+0056 (V)*/
  {.w_px = 18,	.glyph_index = 9680},	/*Unicode: U+0057 (W)*/
  {.w_px = 10,	.glyph_index = 10040},	/*Unicode: U+0058 (X)*/
  {.w_px = 10,	.glyph_index = 10240},	/*Unicode: U+0059 (Y)*/
  {.w_px = 10,	.glyph_index = 10440},	/*Unicode: U+005a (Z)*/
  {.w_px = 5,	.glyph_index = 10640},	/*Unicode: U+005b ([)*/
  {.w_px = 8,	.glyph_index = 10740},	/*Unicode: U+005c (\)*/
  {.w_px = 5,	.glyph_index = 10900},	/*Unicode: U+005d (])*/
  {.w_px = 9,	.glyph_index = 11000},	/*Unicode: U+005e (^)*/
  {.w_px = 8,	.glyph_index = 11180},	/*Unicode: U+005f (_)*/
  {.w_px = 4,	.glyph_index = 11340},	/*Unicode: U+0060 (`)*/
  {.w_px = 9,	.glyph_index = 11420},	/*Unicode: U+0061 (a)*/
  {.w_px = 10,	.glyph_index = 11600},	/*Unicode: U+0062 (b)*/
  {.w_px = 8,	.glyph_index = 11800},	/*Unicode: U+0063 (c)*/
  {.w_px = 10,	.glyph_index = 11960},	/*Unicode: U+0064 (d)*/
  {.w_px = 9,	.glyph_index = 12160},	/*Unicode: U+0065 (e)*/
  {.w_px = 4,	.glyph_index = 12340},	/*Unicode: U+0066 (f)*/
  {.w_px = 9,	.glyph_index = 12420},	/*Unicode: U+0067 (g)*/
  {.w_px = 9,	.glyph_index = 12600},	/*Unicode: U+0068 (h)*/
  {.w_px = 3,	.glyph_index = 12780},	/*Unicode: U+0069 (i)*/
  {.w_px = 5,	.glyph_index = 12840},	/*Unicode: U+006a (j)*/
  {.w_px = 8,	.glyph_index = 12940},	/*Unicode: U+006b (k)*/
  {.w_px = 4,	.glyph_index = 13100},	/*Unicode: U+006c (l)*/
  {.w_px = 13,	.glyph_index = 13180},	/*Unicode: U+006d (m)*/
  {.w_px = 9,	.glyph_index = 13440},	/*Unicode: U+006e (n)*/
  {.w_px = 10,	.glyph_index = 13620},	/*Unicode: U+006f (o)*/
  {.w_px = 10,	.glyph_index = 13820},	/*Unicode: U+0070 (p)*/
  {.w_px = 10,	.glyph_index = 14020},	/*Unicode: U+0071 (q)*/
  {.w_px = 5,	.glyph_index = 14220},	/*Unicode: U+0072 (r)*/
  {.w_px = 8,	.glyph_index = 14320},	/*Unicode: U+0073 (s)*/
  {.w_px = 5,	.glyph_index = 14480},	/*Unicode: U+0074 (t)*/
  {.w_px = 9,	.glyph_index = 14580},	/*Unicode: U+0075 (u)*/
  {.w_px = 8,	.glyph_index = 14760},	/*Unicode: U+0076 (v)*/
  {.w_px = 14,	.glyph_index = 14920},	/*Unicode: U+0077 (w)*/
  {.w_px = 8,	.glyph_index = 15200},	/*Unicode: U+0078 (x)*/
  {.w_px = 8,	.glyph_index = 15360},	/*Unicode: U+0079 (y)*/
  {.w_px = 7,	.glyph_index = 15520},	/*Unicode: U+007a (z)*/
  {.w_px = 4,	.glyph_index = 15660},	/*Unicode: U+007b ({)*/
  {.w_px = 3,	.glyph_index = 15740},	/*Unicode: U+007c (|)*/
  {.w_px = 4,	.glyph_index = 15800},	/*Unicode: U+007d (})*/
  {.w_px = 6,	.glyph_index = 15880},	/*Unicode: U+007e (~)*/

#endif
};

lv_font_t interui_20 =
{
    .unicode_first = 32,	/*First Unicode letter in this font*/
    .unicode_last = 126,	/*Last Unicode letter in this font*/
    .h_px = 20,				/*Font height in pixels*/
    //.glyph_bitmap = interui_20_glyph_bitmap,	/*Bitmap of glyphs*/
    .glyph_bitmap = (const uint8_t *)(NYX_RES_ADDR + 0x3A00),
    .glyph_dsc = interui_20_glyph_dsc,		/*Description of glyphs*/
    .glyph_cnt = 95,			/*Number of glyphs in the font*/
    .unicode_list = NULL,	/*Every character in the font from 'unicode_first' to 'unicode_last'*/
    .get_bitmap = lv_font_get_bitmap_continuous,	/*Function pointer to get glyph's bitmap*/
    .get_width = lv_font_get_width_continuous,	/*Function pointer to get glyph's width*/
#if USE_INTERUI_20 == 4
    .bpp = 4,				/*Bit per pixel*/
#elif USE_INTERUI_20 == 8
    .bpp = 8,				/*Bit per pixel*/
#endif
    .monospace = 0,				/*Fix width (0: if not used)*/
    .next_page = NULL,		/*Pointer to a font extension*/
};

#endif /*USE_INTERUI_20*/
