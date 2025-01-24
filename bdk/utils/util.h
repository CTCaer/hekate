/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2025 CTCaer
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

#include <utils/types.h>
#include <mem/minerva.h>

#define NYX_NEW_INFO 0x3058594E

typedef enum
{
	REBOOT_RCM,          // PMC reset. Enter RCM mode.
	REBOOT_BYPASS_FUSES, // PMC reset via watchdog. Enter Normal mode. Bypass fuse programming in package1.

	POWER_OFF,           // Power off PMIC. Do not reset regulators.
	POWER_OFF_RESET,     // Power off PMIC. Reset regulators.
	POWER_OFF_REBOOT,    // Power off PMIC. Reset regulators. Power on.
} power_state_t;

typedef enum
{
	NYX_CFG_UMS  = BIT(6),

	NYX_CFG_EXTRA = 0xFF << 24
} nyx_cfg_t;

typedef enum
{
	ERR_LIBSYS_LP0 = BIT(0),
	ERR_SYSOLD_NYX = BIT(1),
	ERR_LIBSYS_MTC = BIT(2),
	ERR_SD_BOOT_EN = BIT(3),
	ERR_PANIC_CODE = BIT(4),
	ERR_L4T_KERNEL = BIT(24),
	ERR_EXCEPTION  = BIT(31),
} hekate_errors_t;

typedef struct _reg_cfg_t
{
	u32 idx;
	u32 val;
} reg_cfg_t;

typedef struct _nyx_info_t
{
	u32 magic;
	u32 sd_init;
	u32 sd_errors[3];
	u8  rsvd[0x1000];
	u32 disp_id;
	u32 errors;
} nyx_info_t;

typedef struct _nyx_storage_t
{
	u32 version;
	u32 cfg;
	u8  irama[0x8000];
	u8  hekate[0x30000];
	u8  rsvd[SZ_8M - sizeof(nyx_info_t)];
	nyx_info_t info;
	mtc_config_t mtc_cfg;
	emc_table_t mtc_table[11]; // 10 + 1.
} nyx_storage_t;

u8   bit_count(u32 val);
u32  bit_count_mask(u8 bits);
char *strcpy_ns(char *dst, char *src);
u64  sqrt64(u64 num);
long strtol(const char *nptr, char **endptr, register int base);
int  atoi(const char *nptr);

void reg_write_array(u32 *base, const reg_cfg_t *cfg, u32 num_cfg);
u32  crc32_calc(u32 crc, const u8 *buf, u32 len);

int qsort_compare_int(const void *a, const void *b);
int qsort_compare_char(const void *a, const void *b);
int qsort_compare_char_case(const void *a, const void *b);

void panic(u32 val);
void power_set_state(power_state_t state);
void power_set_state_ex(void *param);


#endif
