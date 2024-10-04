/*
 * VIC driver for Tegra X1
 *
 * Copyright (c) 2018-2019 CTCaer
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

#ifndef _VIC_H_
#define _VIC_H_

#include <utils/types.h>

#define VIC_THI_SLCG_OVERRIDE_LOW_A 0x8C

typedef enum _vic_rotation_t
{
	VIC_ROTATION_0   = 0,
	VIC_ROTATION_90  = 1,
	VIC_ROTATION_180 = 2,
	VIC_ROTATION_270 = 3,
} vic_rotation_t;

typedef enum _vic_pix_format_t
{
	VIC_PIX_FORMAT_L8       =  1, //  8-bit LUT.
	VIC_PIX_FORMAT_X1B5G5R5 = 21, // 16-bit XBGR.
	VIC_PIX_FORMAT_B5G5R5X1 = 23, // 16-bit BGRX.

	VIC_PIX_FORMAT_A8B8G8R8 = 31, // 32-bit ABGR.
	VIC_PIX_FORMAT_A8R8G8B8 = 32, // 32-bit ARGB.
	VIC_PIX_FORMAT_B8G8R8A8 = 33, // 32-bit BGRA.
	VIC_PIX_FORMAT_R8G8B8A8 = 34, // 32-bit RGBA.

	VIC_PIX_FORMAT_X8B8G8R8 = 35, // 32-bit XBGR.
	VIC_PIX_FORMAT_X8R8G8B8 = 36, // 32-bit XRGB.
	VIC_PIX_FORMAT_B8G8R8X8 = 37, // 32-bit BGRX.
	VIC_PIX_FORMAT_R8G8B8X8 = 38, // 32-bit RGBX.
} vic_pix_format_t;

typedef struct _vic_surface_t
{
	u32 src_buf;
	u32 dst_buf;
	u32 width;
	u32 height;
	u32 pix_fmt;
	u32 rotation;
} vic_surface_t;

void vic_set_surface(const vic_surface_t *sfc);
int  vic_compose();
int  vic_init();
void vic_end();

#endif
