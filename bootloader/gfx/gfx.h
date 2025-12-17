/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2021 CTCaer
 * Copyright (c) 2018 M4xw
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

#ifndef _GFX_H_
#define _GFX_H_

#include <bdk.h>

#define TXT_CLR_BG        0xFF1B1B1B // Dark Grey.
#define TXT_CLR_DEFAULT   0xFFCCCCCC // Light Grey.
#define TXT_CLR_WARNING   0xFFFFDD00 // Yellow.
#define TXT_CLR_ERROR     0xFFFF0000 // Red.
#define TXT_CLR_CYAN_L    0xFF00CCFF // Light Cyan.
#define TXT_CLR_TURQUOISE 0xFF00FFCC // Turquoise.
#define TXT_CLR_ORANGE    0xFFFFBA00 // Orange.
#define TXT_CLR_GREENISH  0xFF96FF00 // Toxic Green.
#define TXT_CLR_GREEN_D   0xFF008800 // Dark Green.
#define TXT_CLR_RED_D     0xFF880000 // Dark Red.
#define TXT_CLR_GREY_D    0xFF303030 // Darkest Grey.
#define TXT_CLR_GREY_DM   0xFF444444 // Darker Grey.
#define TXT_CLR_GREY_M    0xFF555555 // Dark Grey.
#define TXT_CLR_GREY      0xFF888888 // Grey.

#define EPRINTF(text) gfx_eputs(text)
#define EPRINTFARGS(text, args...) gfx_printf("%k"text"%k\n", TXT_CLR_ERROR,   args, TXT_CLR_DEFAULT)
#define WPRINTF(text) gfx_wputs(text)
#define WPRINTFARGS(text, args...) gfx_printf("%k"text"%k\n", TXT_CLR_WARNING, args, TXT_CLR_DEFAULT)

typedef struct _gfx_ctxt_t
{
	u32 *fb;
	u32 width;
	u32 height;
	u32 stride;
} gfx_ctxt_t;

typedef struct _gfx_con_t
{
	gfx_ctxt_t *gfx_ctxt;
	u32 fntsz;
	u32 x;
	u32 y;
	u32 savedx;
	u32 savedy;
	u32 fgcol;
	int fillbg;
	u32 bgcol;
	bool mute;
} gfx_con_t;

// Global gfx console and context.
extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;

void gfx_init_ctxt(u32 *fb, u32 width, u32 height, u32 stride);
void gfx_clear_grey(u8 color);
void gfx_clear_partial_grey(u8 color, u32 pos_y, u32 height);
void gfx_clear_color(u32 color);
void gfx_con_init();
void gfx_con_setcol(u32 fgcol, int fillbg, u32 bgcol);
void gfx_con_getpos(u32 *x, u32 *y);
void gfx_con_setpos(u32 x, u32 y);
void gfx_putc(char c);
void gfx_puts(const char *s);
void gfx_wputs(const char *s);
void gfx_eputs(const char *s);
void gfx_printf(const char *fmt, ...) /* __attribute__((format(printf, 1, 2))) */;
void gfx_hexdump(u32 base, const void *buf, u32 len);

void gfx_set_pixel(u32 x, u32 y, u32 color);
void gfx_line(int x0, int y0, int x1, int y1, u32 color);
void gfx_put_small_sep();
void gfx_put_big_sep();
void gfx_set_rect_grey(const u8 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_set_rect_rgb(const u8 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_set_rect_argb(const u32 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_render_bmp_argb(const u32 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);

#endif
