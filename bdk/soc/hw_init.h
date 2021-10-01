/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2021 CTCaer
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

#ifndef _HW_INIT_H_
#define _HW_INIT_H_

#include <utils/types.h>

#define BL_MAGIC_CRBOOT_SLD 0x30444C53 // SLD0, seamless display type 0.
#define BL_MAGIC_BROKEN_HWI 0xBAADF00D // Broken hwinit.

extern u32 hw_rst_status;
extern u32 hw_rst_reason;

void hw_init();
void hw_reinit_workaround(bool coreboot, u32 magic);
u32  hw_get_chip_id();

#endif
