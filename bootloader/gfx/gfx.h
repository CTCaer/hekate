/*
 * Copyright (c) 2018 naehrwert
 * Copyright (C) 2018 CTCaer
 * Copyright (C) 2018 M4xw
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

#include "../../common/common_gfx.h"

void gfx_init_ctxt(gfx_ctxt_t *ctxt, u32 *fb, u32 width, u32 height, u32 stride);
void gfx_clear_grey(gfx_ctxt_t *ctxt, u8 color);
void gfx_clear_partial_grey(gfx_ctxt_t *ctxt, u8 color, u32 pos_x, u32 height);
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
void gfx_put_small_sep(gfx_con_t *con);
void gfx_put_big_sep(gfx_con_t *con);
void gfx_set_rect_grey(gfx_ctxt_t *ctxt, const u8 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_set_rect_rgb(gfx_ctxt_t *ctxt, const u8 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_set_rect_argb(gfx_ctxt_t *ctxt, const u32 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_render_bmp_argb(gfx_ctxt_t *ctxt, const u32 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);

#endif
