/*
 * Copyright (c) 2018-2022 CTCaer
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

#ifndef _GFX_LOGOS_H_
#define _GFX_LOGOS_H_

// 21 x 50 @8bpp RGB.
#define BATTERY_EMPTY_WIDTH       21
#define BATTERY_EMPTY_BATT_HEIGHT 38
#define BATTERY_EMPTY_CHRG_HEIGHT 12
#define BATTERY_EMPTY_SIZE        3150
#define BATTERY_EMPTY_BLZ_SIZE    740
extern u8 battery_icons_blz[];

u8  *render_static_bootlogo();
bool render_ticker_logo(u32 boot_wait, u32 backlight);
bool render_ticker(u32 boot_wait, u32 backlight, bool no_ticker);

#endif
