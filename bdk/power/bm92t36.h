/*
 * USB-PD driver for Nintendo Switch's TI BM92T36
 *
 * Copyright (c) 2020 CTCaer
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

#ifndef __BM92T36_H_
#define __BM92T36_H_

#include <utils/types.h>

#define BM92T36_I2C_ADDR 0x18

typedef struct _usb_pd_object_t
{
	u32 amperage;
	u32 voltage;
} usb_pd_object_t;

typedef struct _usb_pd_objects_t
{
	u32 pdo_no;
	usb_pd_object_t pdos[7];
	usb_pd_object_t selected_pdo;
} usb_pd_objects_t;

void bm92t36_get_sink_info(bool *inserted, usb_pd_objects_t *usb_pd);

#endif
