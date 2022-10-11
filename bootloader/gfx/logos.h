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

// 68 x 192 @8bpp Grayscale RAW.
#define X_BOOTLOGO         68
#define Y_BOOTLOGO        192
#define SZ_BOOTLOGO     13056
#define SZ_BOOTLOGO_BLZ  3988
extern u8 BOOTLOGO_BLZ[SZ_BOOTLOGO_BLZ];

// 21 x 50 @8bpp RGB.
#define X_BATTERY_EMPTY      21
#define Y_BATTERY_EMPTY_BATT 38
#define Y_BATTERY_EMPTY_CHRG 12
#define SZ_BATTERY_EMPTY     3150
#define SZ_BATTERY_EMPTY_BLZ 740
extern u8 BATTERY_EMPTY_BLZ[SZ_BATTERY_EMPTY_BLZ];

u8  *render_static_bootlogo();
bool render_ticker_logo(u32 boot_wait, u32 backlight);
bool render_ticker(u32 boot_wait, u32 backlight, bool no_ticker);

#endif
