/*
* Copyright (c) 2018 naehrwert
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

#include "types.h"

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
	u32 x;
	u32 y;
	u32 fgcol;
	int fillbg;
	u32 bgcol;
} gfx_con_t;

void gfx_init_ctxt(gfx_ctxt_t *ctxt, u32 *fb, u32 width, u32 height, u32 stride);
void gfx_clear_grey(gfx_ctxt_t *ctxt, u8 color);
void gfx_clear_color(gfx_ctxt_t *ctxt, u32 color);
void gfx_con_init(gfx_con_t *con, gfx_ctxt_t *ctxt);
void gfx_con_setcol(gfx_con_t *con, u32 fgcol, int fillbg, u32 bgcol);
void gfx_con_getpos(gfx_con_t *con, u32 *x, u32 *y);
void gfx_con_setpos(gfx_con_t *con, u32 x, u32 y);
void gfx_putc(gfx_con_t *con, char c);
void gfx_puts(gfx_con_t *con, const char *s);
void gfx_printf(gfx_con_t *con, const char *fmt, ...);
void gfx_hexdump(gfx_con_t *con, u32 base, const u8 *buf, u32 len);

void gfx_set_pixel(gfx_ctxt_t *ctxt, u32 x, u32 y, u32 color);
void gfx_line(gfx_ctxt_t *ctxt, int x0, int y0, int x1, int y1, u32 color);

#endif
