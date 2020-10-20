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

#include <string.h>

#include "bm92t36.h"
#include <soc/i2c.h>
#include <utils/util.h>

#define ALERT_STATUS_REG    0x2
#define STATUS1_REG         0x3
#define STATUS2_REG         0x4
#define COMMAND_REG         0x5
#define CONFIG1_REG         0x6
#define DEV_CAPS_REG        0x7
#define READ_PDOS_SRC_REG   0x8
#define CONFIG2_REG         0x17
#define DP_STATUS_REG       0x18
#define DP_ALERT_EN_REG     0x19
#define VENDOR_CONFIG_REG   0x1A
#define AUTO_NGT_FIXED_REG  0x20
#define AUTO_NGT_BATT_REG   0x23
#define SYS_CONFIG1_REG     0x26
#define SYS_CONFIG2_REG     0x27
#define CURRENT_PDO_REG     0x28
#define CURRENT_RDO_REG     0x2B
#define ALERT_ENABLE_REG    0x2E
#define SYS_CONFIG3_REG     0x2F
#define SET_RDO_REG         0x30
#define PDOS_SNK_REG        0x33
#define PDOS_SRC_PROV_REG   0x3C
#define FW_TYPE_REG         0x4B
#define FW_REVISION_REG     0x4C
#define MAN_ID_REG          0x4D
#define DEV_ID_REG          0x4E
#define REV_ID_REG          0x4F
#define INCOMING_VDM_REG    0x50
#define OUTGOING_VDM_REG    0x60

#define STATUS1_INSERT      BIT(7)  // Cable inserted.

typedef struct _pd_object_t {
	unsigned int amp:10;
	unsigned int volt:10;
	unsigned int info:10;
	unsigned int type:2;
} __attribute__((packed)) pd_object_t;

static int _bm92t36_read_reg(u8 *buf, u32 size, u32 reg)
{
	return i2c_recv_buf_big(buf, size, I2C_1, BM92T36_I2C_ADDR, reg);
}

void bm92t36_get_sink_info(bool *inserted, usb_pd_objects_t *usb_pd)
{
	u8 buf[32];
	pd_object_t pdos[7];

	if (inserted)
	{
		_bm92t36_read_reg(buf, 2, STATUS1_REG);
		*inserted = buf[0] & STATUS1_INSERT ? true : false;
	}

	if (usb_pd)
	{
		_bm92t36_read_reg(buf, 29, READ_PDOS_SRC_REG);
		memcpy(pdos, &buf[1], 28);

		memset(usb_pd, 0, sizeof(usb_pd_objects_t));
		usb_pd->pdo_no = buf[0] / sizeof(pd_object_t);

		for (u32 i = 0; i < usb_pd->pdo_no; i++)
		{
			usb_pd->pdos[i].amperage = pdos[i].amp * 10;
			usb_pd->pdos[i].voltage = (pdos[i].volt * 50) / 1000;
		}

		_bm92t36_read_reg(buf, 5, CURRENT_PDO_REG);
		memcpy(pdos, &buf[1], 4);
		usb_pd->selected_pdo.amperage = pdos[0].amp * 10;
		usb_pd->selected_pdo.voltage = (pdos[0].volt * 50) / 1000;
	}
}
