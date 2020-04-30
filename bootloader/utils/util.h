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

#ifndef _UTIL_H_
#define _UTIL_H_

#include "types.h"
#include "../mem/minerva.h"

typedef enum
{
	NYX_CFG_UMS     = (1 << 6),
	NYX_CFG_DUMP    = (1 << 7),
} nyx_cfg_t;

typedef enum
{
	ERR_LIBSYS_LP0 = (1 << 0),
	ERR_SYSOLD_NYX = (1 << 1),
	ERR_SYSOLD_MTC = (1 << 2),
	ERR_EXCEPT_ENB = (1 << 31),
} hekate_errors_t;

#define byte_swap_32(num) (((num >> 24) & 0xff) | ((num << 8) & 0xff0000) | \
						((num >> 8 )& 0xff00) | ((num << 24) & 0xff000000))

typedef struct _cfg_op_t
{
	u32 off;
	u32 val;
} cfg_op_t;

typedef struct _nyx_info_t
{
	u32 disp_id;
	u32 errors;
} nyx_info_t;

typedef struct _nyx_storage_t
{
	u32 version;
	u32 cfg;
	u8  irama[0x8000];
	u8  hekate[0x30000];
	u8  rsvd[0x800000 - sizeof(nyx_info_t)];
	nyx_info_t info;
	mtc_config_t mtc_cfg;
	emc_table_t mtc_table[10];
} nyx_storage_t;

u32 get_tmr_us();
u32 get_tmr_ms();
u32 get_tmr_s();
void usleep(u32 us);
void msleep(u32 ms);
void panic(u32 val);
void reboot_normal();
void reboot_rcm();
void power_off();
void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops);
u32  crc32_calc(u32 crc, const u8 *buf, u32 len);

#endif
