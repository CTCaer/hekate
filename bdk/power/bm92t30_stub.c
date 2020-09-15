/*
 * USB-PD driver for Nintendo Switch's TI BM92T30
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

/*
 * TODO:
 * This driver is a stub and relies on incorrect I2C requests.
 * The I2C driver needs to be updated with proper multi-xfer support.
 *
 * For now it replies its whole register range, according to request
 * size. That's because the IC does not support the classic single
 * transfer request-response style.
 */

#include <string.h>

#include "bm92t30_stub.h"
#include <soc/i2c.h>
#include <utils/util.h>

#define STATUS1_INSERT 0x80

typedef struct _pdo_t{
	u32 amp:10;
	u32 volt:10;
	u32 info:10;
	u32 type:2;
} pdo_t;

void bm92t30_get_charger_type(bool *inserted, usb_pd_objects_t *usb_pd)
{
	u8 buf[32];
	pdo_t pdos[7];
	i2c_recv_buf_big(buf, 32, I2C_1, BM92T30_I2C_ADDR, 0);

	if (inserted)
		*inserted = buf[6] & STATUS1_INSERT ? true : false;
	memcpy(pdos, &buf[17], 15); // We can only get max 4 objects.

	if (usb_pd)
	{
		memset(usb_pd, 0, sizeof(usb_pd_objects_t));
		usb_pd->pdo_no = MIN(buf[16] / sizeof(pdo_t), 4);  // We can only get max 4 objects.

		for (u32 i = 0; i < usb_pd->pdo_no; i++)
		{
			usb_pd->pdos[i].amperage = pdos[i].amp * 10;
			usb_pd->pdos[i].voltage = (pdos[i].volt * 50) / 1000;
		}
	}
}
