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

#if USE_UBUNTU_MONO != 0	/*Can be enabled in lv_conf.h*/

/***********************************************************************************
 * UbuntuMono-B.ttf 20 px Font in U+0020 ( ) .. U+007e (~)  range with all bpp
***********************************************************************************/

/*Store the glyph descriptions*/
static const lv_font_glyph_dsc_t ubuntu_mono_glyph_dsc[] =
{
#if USE_UBUNTU_MONO == 4
  {.w_px = 6,	.glyph_index = 0},	/*Unicode: U+0020 ( )*/
  {.w_px = 2,	.glyph_index = 60},	/*Unicode: U+0021 (!)*/
  {.w_px = 5,	.glyph_index = 80},	/*Unicode: U+0022 (")*/
  {.w_px = 10,	.glyph_index = 140},	/*Unicode: U+0023 (#)*/
  {.w_px = 8,	.glyph_index = 240},	/*Unicode: U+0024 ($)*/
  {.w_px = 10,	.glyph_index = 320},	/*Unicode: U+0025 (%)*/
  {.w_px = 10,	.glyph_index = 420},	/*Unicode: U+0026 (&)*/
  {.w_px = 2,	.glyph_index = 520},	/*Unicode: U+0027 (')*/
  {.w_px = 6,	.glyph_index = 540},	/*Unicode: U+0028 (()*/
  {.w_px = 6,	.glyph_index = 600},	/*Unicode: U+0029 ())*/
  {.w_px = 9,	.glyph_index = 660},	/*Unicode: U+002a (*)*/
  {.w_px = 8,	.glyph_index = 760},	/*Unicode: U+002b (+)*/
  {.w_px = 4,	.glyph_index = 840},	/*Unicode: U+002c (,)*/
  {.w_px = 5,	.glyph_index = 880},	/*Unicode: U+002d (-)*/
  {.w_px = 4,	.glyph_index = 940},	/*Unicode: U+002e (.)*/
  {.w_px = 8,	.glyph_index = 980},	/*Unicode: U+002f (/)*/
  {.w_px = 8,	.glyph_index = 1060},	/*Unicode: U+0030 (0)*/
  {.w_px = 7,	.glyph_index = 1140},	/*Unicode: U+0031 (1)*/
  {.w_px = 8,	.glyph_index = 1220},	/*Unicode: U+0032 (2)*/
  {.w_px = 8,	.glyph_index = 1300},	/*Unicode: U+0033 (3)*/
  {.w_px = 8,	.glyph_index = 1380},	/*Unicode: U+0034 (4)*/
  {.w_px = 8,	.glyph_index = 1460},	/*Unicode: U+0035 (5)*/
  {.w_px = 8,	.glyph_index = 1540},	/*Unicode: U+0036 (6)*/
  {.w_px = 7,	.glyph_index = 1620},	/*Unicode: U+0037 (7)*/
  {.w_px = 8,	.glyph_index = 1700},	/*Unicode: U+0038 (8)*/
  {.w_px = 8,	.glyph_index = 1780},	/*Unicode: U+0039 (9)*/
  {.w_px = 4,	.glyph_index = 1860},	/*Unicode: U+003a (:)*/
  {.w_px = 4,	.glyph_index = 1900},	/*Unicode: U+003b (;)*/
  {.w_px = 9,	.glyph_index = 1940},	/*Unicode: U+003c (<)*/
  {.w_px = 8,	.glyph_index = 2040},	/*Unicode: U+003d (=)*/
  {.w_px = 8,	.glyph_index = 2120},	/*Unicode: U+003e (>)*/
  {.w_px = 8,	.glyph_index = 2200},	/*Unicode: U+003f (?)*/
  {.w_px = 9,	.glyph_index = 2280},	/*Unicode: U+0040 (@)*/
  {.w_px = 10,	.glyph_index = 2380},	/*Unicode: U+0041 (A)*/
  {.w_px = 8,	.glyph_index = 2480},	/*Unicode: U+0042 (B)*/
  {.w_px = 8,	.glyph_index = 2560},	/*Unicode: U+0043 (C)*/
  {.w_px = 8,	.glyph_index = 2640},	/*Unicode: U+0044 (D)*/
  {.w_px = 8,	.glyph_index = 2720},	/*Unicode: U+0045 (E)*/
  {.w_px = 8,	.glyph_index = 2800},	/*Unicode: U+0046 (F)*/
  {.w_px = 8,	.glyph_index = 2880},	/*Unicode: U+0047 (G)*/
  {.w_px = 8,	.glyph_index = 2960},	/*Unicode: U+0048 (H)*/
  {.w_px = 8,	.glyph_index = 3040},	/*Unicode: U+0049 (I)*/
  {.w_px = 8,	.glyph_index = 3120},	/*Unicode: U+004a (J)*/
  {.w_px = 9,	.glyph_index = 3200},	/*Unicode: U+004b (K)*/
  {.w_px = 8,	.glyph_index = 3300},	/*Unicode: U+004c (L)*/
  {.w_px = 9,	.glyph_index = 3380},	/*Unicode: U+004d (M)*/
  {.w_px = 8,	.glyph_index = 3480},	/*Unicode: U+004e (N)*/
  {.w_px = 8,	.glyph_index = 3560},	/*Unicode: U+004f (O)*/
  {.w_px = 8,	.glyph_index = 3640},	/*Unicode: U+0050 (P)*/
  {.w_px = 8,	.glyph_index = 3720},	/*Unicode: U+0051 (Q)*/
  {.w_px = 8,	.glyph_index = 3800},	/*Unicode: U+0052 (R)*/
  {.w_px = 8,	.glyph_index = 3880},	/*Unicode: U+0053 (S)*/
  {.w_px = 8,	.glyph_index = 3960},	/*Unicode: U+0054 (T)*/
  {.w_px = 8,	.glyph_index = 4040},	/*Unicode: U+0055 (U)*/
  {.w_px = 9,	.glyph_index = 4120},	/*Unicode: U+0056 (V)*/
  {.w_px = 9,	.glyph_index = 4220},	/*Unicode: U+0057 (W)*/
  {.w_px = 10,	.glyph_index = 4320},	/*Unicode: U+0058 (X)*/
  {.w_px = 10,	.glyph_index = 4420},	/*Unicode: U+0059 (Y)*/
  {.w_px = 8,	.glyph_index = 4520},	/*Unicode: U+005a (Z)*/
  {.w_px = 6,	.glyph_index = 4600},	/*Unicode: U+005b ([)*/
  {.w_px = 8,	.glyph_index = 4660},	/*Unicode: U+005c (\)*/
  {.w_px = 6,	.glyph_index = 4740},	/*Unicode: U+005d (])*/
  {.w_px = 10,	.glyph_index = 4800},	/*Unicode: U+005e (^)*/
  {.w_px = 10,	.glyph_index = 4900},	/*Unicode: U+005f (_)*/
  {.w_px = 4,	.glyph_index = 5000},	/*Unicode: U+0060 (`)*/
  {.w_px = 8,	.glyph_index = 5040},	/*Unicode: U+0061 (a)*/
  {.w_px = 8,	.glyph_index = 5120},	/*Unicode: U+0062 (b)*/
  {.w_px = 8,	.glyph_index = 5200},	/*Unicode: U+0063 (c)*/
  {.w_px = 8,	.glyph_index = 5280},	/*Unicode: U+0064 (d)*/
  {.w_px = 9,	.glyph_index = 5360},	/*Unicode: U+0065 (e)*/
  {.w_px = 9,	.glyph_index = 5460},	/*Unicode: U+0066 (f)*/
  {.w_px = 8,	.glyph_index = 5560},	/*Unicode: U+0067 (g)*/
  {.w_px = 8,	.glyph_index = 5640},	/*Unicode: U+0068 (h)*/
  {.w_px = 9,	.glyph_index = 5720},	/*Unicode: U+0069 (i)*/
  {.w_px = 7,	.glyph_index = 5820},	/*Unicode: U+006a (j)*/
  {.w_px = 9,	.glyph_index = 5900},	/*Unicode: U+006b (k)*/
  {.w_px = 8,	.glyph_index = 6000},	/*Unicode: U+006c (l)*/
  {.w_px = 8,	.glyph_index = 6080},	/*Unicode: U+006d (m)*/
  {.w_px = 8,	.glyph_index = 6160},	/*Unicode: U+006e (n)*/
  {.w_px = 8,	.glyph_index = 6240},	/*Unicode: U+006f (o)*/
  {.w_px = 8,	.glyph_index = 6320},	/*Unicode: U+0070 (p)*/
  {.w_px = 8,	.glyph_index = 6400},	/*Unicode: U+0071 (q)*/
  {.w_px = 7,	.glyph_index = 6480},	/*Unicode: U+0072 (r)*/
  {.w_px = 8,	.glyph_index = 6560},	/*Unicode: U+0073 (s)*/
  {.w_px = 8,	.glyph_index = 6640},	/*Unicode: U+0074 (t)*/
  {.w_px = 8,	.glyph_index = 6720},	/*Unicode: U+0075 (u)*/
  {.w_px = 9,	.glyph_index = 6800},	/*Unicode: U+0076 (v)*/
  {.w_px = 10,	.glyph_index = 6900},	/*Unicode: U+0077 (w)*/
  {.w_px = 10,	.glyph_index = 7000},	/*Unicode: U+0078 (x)*/
  {.w_px = 9,	.glyph_index = 7100},	/*Unicode: U+0079 (y)*/
  {.w_px = 8,	.glyph_index = 7200},	/*Unicode: U+007a (z)*/
  {.w_px = 7,	.glyph_index = 7280},	/*Unicode: U+007b ({)*/
  {.w_px = 2,	.glyph_index = 7360},	/*Unicode: U+007c (|)*/
  {.w_px = 8,	.glyph_index = 7380},	/*Unicode: U+007d (})*/
  {.w_px = 9,	.glyph_index = 7460},	/*Unicode: U+007e (~)*/

#elif USE_UBUNTU_MONO == 8
  {.w_px = 6,	.glyph_index = 0},	/*Unicode: U+0020 ( )*/
  {.w_px = 2,	.glyph_index = 120},	/*Unicode: U+0021 (!)*/
  {.w_px = 5,	.glyph_index = 160},	/*Unicode: U+0022 (")*/
  {.w_px = 10,	.glyph_index = 260},	/*Unicode: U+0023 (#)*/
  {.w_px = 8,	.glyph_index = 460},	/*Unicode: U+0024 ($)*/
  {.w_px = 10,	.glyph_index = 620},	/*Unicode: U+0025 (%)*/
  {.w_px = 10,	.glyph_index = 820},	/*Unicode: U+0026 (&)*/
  {.w_px = 2,	.glyph_index = 1020},	/*Unicode: U+0027 (')*/
  {.w_px = 6,	.glyph_index = 1060},	/*Unicode: U+0028 (()*/
  {.w_px = 6,	.glyph_index = 1180},	/*Unicode: U+0029 ())*/
  {.w_px = 9,	.glyph_index = 1300},	/*Unicode: U+002a (*)*/
  {.w_px = 8,	.glyph_index = 1480},	/*Unicode: U+002b (+)*/
  {.w_px = 4,	.glyph_index = 1640},	/*Unicode: U+002c (,)*/
  {.w_px = 5,	.glyph_index = 1720},	/*Unicode: U+002d (-)*/
  {.w_px = 4,	.glyph_index = 1820},	/*Unicode: U+002e (.)*/
  {.w_px = 8,	.glyph_index = 1900},	/*Unicode: U+002f (/)*/
  {.w_px = 8,	.glyph_index = 2060},	/*Unicode: U+0030 (0)*/
  {.w_px = 7,	.glyph_index = 2220},	/*Unicode: U+0031 (1)*/
  {.w_px = 8,	.glyph_index = 2360},	/*Unicode: U+0032 (2)*/
  {.w_px = 8,	.glyph_index = 2520},	/*Unicode: U+0033 (3)*/
  {.w_px = 8,	.glyph_index = 2680},	/*Unicode: U+0034 (4)*/
  {.w_px = 8,	.glyph_index = 2840},	/*Unicode: U+0035 (5)*/
  {.w_px = 8,	.glyph_index = 3000},	/*Unicode: U+0036 (6)*/
  {.w_px = 7,	.glyph_index = 3160},	/*Unicode: U+0037 (7)*/
  {.w_px = 8,	.glyph_index = 3300},	/*Unicode: U+0038 (8)*/
  {.w_px = 8,	.glyph_index = 3460},	/*Unicode: U+0039 (9)*/
  {.w_px = 4,	.glyph_index = 3620},	/*Unicode: U+003a (:)*/
  {.w_px = 4,	.glyph_index = 3700},	/*Unicode: U+003b (;)*/
  {.w_px = 9,	.glyph_index = 3780},	/*Unicode: U+003c (<)*/
  {.w_px = 8,	.glyph_index = 3960},	/*Unicode: U+003d (=)*/
  {.w_px = 8,	.glyph_index = 4120},	/*Unicode: U+003e (>)*/
  {.w_px = 8,	.glyph_index = 4280},	/*Unicode: U+003f (?)*/
  {.w_px = 9,	.glyph_index = 4440},	/*Unicode: U+0040 (@)*/
  {.w_px = 10,	.glyph_index = 4620},	/*Unicode: U+0041 (A)*/
  {.w_px = 8,	.glyph_index = 4820},	/*Unicode: U+0042 (B)*/
  {.w_px = 8,	.glyph_index = 4980},	/*Unicode: U+0043 (C)*/
  {.w_px = 8,	.glyph_index = 5140},	/*Unicode: U+0044 (D)*/
  {.w_px = 8,	.glyph_index = 5300},	/*Unicode: U+0045 (E)*/
  {.w_px = 8,	.glyph_index = 5460},	/*Unicode: U+0046 (F)*/
  {.w_px = 8,	.glyph_index = 5620},	/*Unicode: U+0047 (G)*/
  {.w_px = 8,	.glyph_index = 5780},	/*Unicode: U+0048 (H)*/
  {.w_px = 8,	.glyph_index = 5940},	/*Unicode: U+0049 (I)*/
  {.w_px = 8,	.glyph_index = 6100},	/*Unicode: U+004a (J)*/
  {.w_px = 9,	.glyph_index = 6260},	/*Unicode: U+004b (K)*/
  {.w_px = 8,	.glyph_index = 6440},	/*Unicode: U+004c (L)*/
  {.w_px = 9,	.glyph_index = 6600},	/*Unicode: U+004d (M)*/
  {.w_px = 8,	.glyph_index = 6780},	/*Unicode: U+004e (N)*/
  {.w_px = 8,	.glyph_index = 6940},	/*Unicode: U+004f (O)*/
  {.w_px = 8,	.glyph_index = 7100},	/*Unicode: U+0050 (P)*/
  {.w_px = 8,	.glyph_index = 7260},	/*Unicode: U+0051 (Q)*/
  {.w_px = 8,	.glyph_index = 7420},	/*Unicode: U+0052 (R)*/
  {.w_px = 8,	.glyph_index = 7580},	/*Unicode: U+0053 (S)*/
  {.w_px = 8,	.glyph_index = 7740},	/*Unicode: U+0054 (T)*/
  {.w_px = 8,	.glyph_index = 7900},	/*Unicode: U+0055 (U)*/
  {.w_px = 9,	.glyph_index = 8060},	/*Unicode: U+0056 (V)*/
  {.w_px = 9,	.glyph_index = 8240},	/*Unicode: U+0057 (W)*/
  {.w_px = 10,	.glyph_index = 8420},	/*Unicode: U+0058 (X)*/
  {.w_px = 10,	.glyph_index = 8620},	/*Unicode: U+0059 (Y)*/
  {.w_px = 8,	.glyph_index = 8820},	/*Unicode: U+005a (Z)*/
  {.w_px = 6,	.glyph_index = 8980},	/*Unicode: U+005b ([)*/
  {.w_px = 8,	.glyph_index = 9100},	/*Unicode: U+005c (\)*/
  {.w_px = 6,	.glyph_index = 9260},	/*Unicode: U+005d (])*/
  {.w_px = 10,	.glyph_index = 9380},	/*Unicode: U+005e (^)*/
  {.w_px = 10,	.glyph_index = 9580},	/*Unicode: U+005f (_)*/
  {.w_px = 4,	.glyph_index = 9780},	/*Unicode: U+0060 (`)*/
  {.w_px = 8,	.glyph_index = 9860},	/*Unicode: U+0061 (a)*/
  {.w_px = 8,	.glyph_index = 10020},	/*Unicode: U+0062 (b)*/
  {.w_px = 8,	.glyph_index = 10180},	/*Unicode: U+0063 (c)*/
  {.w_px = 8,	.glyph_index = 10340},	/*Unicode: U+0064 (d)*/
  {.w_px = 9,	.glyph_index = 10500},	/*Unicode: U+0065 (e)*/
  {.w_px = 9,	.glyph_index = 10680},	/*Unicode: U+0066 (f)*/
  {.w_px = 8,	.glyph_index = 10860},	/*Unicode: U+0067 (g)*/
  {.w_px = 8,	.glyph_index = 11020},	/*Unicode: U+0068 (h)*/
  {.w_px = 9,	.glyph_index = 11180},	/*Unicode: U+0069 (i)*/
  {.w_px = 7,	.glyph_index = 11360},	/*Unicode: U+006a (j)*/
  {.w_px = 9,	.glyph_index = 11500},	/*Unicode: U+006b (k)*/
  {.w_px = 8,	.glyph_index = 11680},	/*Unicode: U+006c (l)*/
  {.w_px = 8,	.glyph_index = 11840},	/*Unicode: U+006d (m)*/
  {.w_px = 8,	.glyph_index = 12000},	/*Unicode: U+006e (n)*/
  {.w_px = 8,	.glyph_index = 12160},	/*Unicode: U+006f (o)*/
  {.w_px = 8,	.glyph_index = 12320},	/*Unicode: U+0070 (p)*/
  {.w_px = 8,	.glyph_index = 12480},	/*Unicode: U+0071 (q)*/
  {.w_px = 7,	.glyph_index = 12640},	/*Unicode: U+0072 (r)*/
  {.w_px = 8,	.glyph_index = 12780},	/*Unicode: U+0073 (s)*/
  {.w_px = 8,	.glyph_index = 12940},	/*Unicode: U+0074 (t)*/
  {.w_px = 8,	.glyph_index = 13100},	/*Unicode: U+0075 (u)*/
  {.w_px = 9,	.glyph_index = 13260},	/*Unicode: U+0076 (v)*/
  {.w_px = 10,	.glyph_index = 13440},	/*Unicode: U+0077 (w)*/
  {.w_px = 10,	.glyph_index = 13640},	/*Unicode: U+0078 (x)*/
  {.w_px = 9,	.glyph_index = 13840},	/*Unicode: U+0079 (y)*/
  {.w_px = 8,	.glyph_index = 14020},	/*Unicode: U+007a (z)*/
  {.w_px = 7,	.glyph_index = 14180},	/*Unicode: U+007b ({)*/
  {.w_px = 2,	.glyph_index = 14320},	/*Unicode: U+007c (|)*/
  {.w_px = 8,	.glyph_index = 14360},	/*Unicode: U+007d (})*/
  {.w_px = 9,	.glyph_index = 14520},	/*Unicode: U+007e (~)*/

#endif
};

lv_font_t ubuntu_mono =
{
    .unicode_first = 32,	/*First Unicode letter in this font*/
    .unicode_last = 126,	/*Last Unicode letter in this font*/
    .h_px = 20,				/*Font height in pixels*/
    //.glyph_bitmap = ubuntu_mono_glyph_bitmap,	/*Bitmap of glyphs*/
    .glyph_bitmap = (const uint8_t *)(NYX_RES_ADDR),
    .glyph_dsc = ubuntu_mono_glyph_dsc,		/*Description of glyphs*/
    .glyph_cnt = 95,			/*Number of glyphs in the font*/
    .unicode_list = NULL,	/*Every character in the font from 'unicode_first' to 'unicode_last'*/
    .get_bitmap = lv_font_get_bitmap_continuous,	/*Function pointer to get glyph's bitmap*/
    .get_width = lv_font_get_width_continuous,	/*Function pointer to get glyph's width*/
#if USE_UBUNTU_MONO == 4
    .bpp = 4,				/*Bit per pixel*/
#elif USE_UBUNTU_MONO == 8
    .bpp = 8,				/*Bit per pixel*/
#endif
    .monospace = 10,		/*Fix width (0: if not used)*/
    .next_page = NULL,		/*Pointer to a font extension*/
};

#endif /*USE_UBUNTU_MONO*/
