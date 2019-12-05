/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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

#include "../../common/common_gfx.h"


#define U32WHITE 0xFFFFFFFF
#define U32DKWHITE 0xFFCCCCCC
#define U32BLACK 0xFF000000
#define U32GREEN 0xFF00FF00
#define U32DKGREEN 0xFF008800
#define U32RED 0xFFFF0000
#define U32DKRED 0xFF880000
#define U32BLUE 0xFF0000FF
#define U32YELLOW 0xFFFFFF00
#define U32DKYELLOW 0xFF888800
#define U32CYAN 0xFF00FFFF
#define U32DKCYAN 0xFF008888
#define U32DKGREY 0xFF303030
#define U32DKDKGREY 0xFF1B1B1B
#define U32GREY 0xFF888888

#define U8BLACK 0x00
#define U8DKGREY 0x30
#define U8DKDKGREY 0x1B

///////////////////////////////
//switchboot_uf2 col scheme
/*#define MAINTXTCOL U32WHITE
#define TXTBG_COL U32BLACK
#define ERRWARNCOL U32RED
#define ATTNCOL U32YELLOW
#define PBARCOL U32DKGREEN
#define PBARFGCOL U32GREEN
#define INFOCOL U32GREEN
#define BATTDECCOL U32RED
#define BATTINCCOL U32GREEN
#define BATTFGCOL U32WHITE
#define BATTBGCOL U32DKGREY
#define MENUOUTLINECOL U32DKGREY

#define BG_COL U8BLACK*/
///////////////////////////////
//hekate col scheme
#define MAINTXTCOL U32DKWHITE
#define TXTBG_COL U32DKDKGREY
#define ERRWARNCOL U32RED
#define ATTNCOL U32YELLOW
#define PBARCOL U32DKGREEN
#define PBARFGCOL U32GREEN
#define INFOCOL U32CYAN
#define BATTDECCOL U32DKRED
#define BATTINCCOL U32DKGREEN
#define BATTFGCOL U32GREY
#define BATTBGCOL U32DKGREY
#define MENUOUTLINECOL U32GREY

#define BG_COL U8DKDKGREY

#define EPRINTF(text) gfx_printf("%k"text"%k\n", ERRWARNCOL, MAINTXTCOL)
#define EPRINTFARGS(text, args...) gfx_printf("%k"text"%k\n", ERRWARNCOL, args, MAINTXTCOL)
#define WPRINTF(text) gfx_printf("%k"text"%k\n", ATTNCOL, MAINTXTCOL)
#define WPRINTFARGS(text, args...) gfx_printf("%k"text"%k\n", ATTNCOL, args, MAINTXTCOL)

void gfx_init_ctxt(u32 *fb, u32 width, u32 height, u32 stride);
void gfx_clear(u8 color);
void gfx_clear_partial(u8 color, u32 pos_x, u32 height);
void gfx_clear_color(u32 color);
void gfx_con_init();
void gfx_con_setcol(u32 fgcol, int fillbg, u32 bgcol);
void gfx_con_getpos(u32 *x, u32 *y);
void gfx_con_setpos(u32 x, u32 y);
void gfx_putc(char c);
void gfx_puts(const char *s);
void gfx_printf(const char *fmt, ...);
void gfx_hexdump(u32 base, const u8 *buf, u32 len);

void gfx_set_pixel(u32 x, u32 y, u32 color);
void gfx_line(int x0, int y0, int x1, int y1, u32 color);
void gfx_put_small_sep();
void gfx_put_big_sep();
void gfx_set_rect_grey(const u8 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_set_rect_rgb(const u8 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_set_rect_argb(const u32 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);
void gfx_render_bmp_argb(const u32 *buf, u32 size_x, u32 size_y, u32 pos_x, u32 pos_y);

// Global gfx console and context.
gfx_ctxt_t gfx_ctxt;
gfx_con_t gfx_con;

#endif
