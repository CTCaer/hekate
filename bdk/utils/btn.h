/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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

#ifndef _BTN_H_
#define _BTN_H_

#include <utils/types.h>

#define BTN_POWER    BIT(0)
#define BTN_VOL_DOWN BIT(1)
#define BTN_VOL_UP   BIT(2)
#define BTN_SINGLE   BIT(7)

u8 btn_read();
u8 btn_read_vol();
u8 btn_wait();
u8 btn_wait_timeout(u32 time_ms, u8 mask);
u8 btn_wait_timeout_single(u32 time_ms, u8 mask);

#endif
