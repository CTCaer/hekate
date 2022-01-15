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

#ifndef _TYPES_H_
#define _TYPES_H_

#include <assert.h>

/* Types */
typedef signed char s8;
typedef short s16;
typedef short SHORT;
typedef int s32;
typedef int INT;
typedef int bool;
typedef long LONG;
typedef long long int s64;

typedef unsigned char u8;
typedef unsigned char BYTE;
typedef unsigned short u16;
typedef unsigned short WORD;
typedef unsigned short WCHAR;
typedef unsigned int u32;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;
typedef unsigned long long int u64;

typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;

#ifdef __aarch64__
typedef unsigned long long uptr;
#else /* __arm__ or __thumb__ */
typedef unsigned long uptr;
#endif

/* Important */
#define false 0
#define true  1

#define NULL ((void *)0)

/* Misc */
#define DISABLE 0
#define ENABLE  1

/* Sizes */
#define SZ_1K   0x400
#define SZ_2K   0x800
#define SZ_4K   0x1000
#define SZ_8K   0x2000
#define SZ_16K  0x4000
#define SZ_32K  0x8000
#define SZ_64K  0x10000
#define SZ_128K 0x20000
#define SZ_256K 0x40000
#define SZ_512K 0x80000
#define SZ_1M   0x100000
#define SZ_2M   0x200000
#define SZ_4M   0x400000
#define SZ_8M   0x800000
#define SZ_16M  0x1000000
#define SZ_32M  0x2000000
#define SZ_64M  0x4000000
#define SZ_128M 0x8000000
#define SZ_256M 0x10000000
#define SZ_512M 0x20000000
#define SZ_1G   0x40000000
#define SZ_2G   0x80000000
#define SZ_PAGE SZ_4K

/* Macros */
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define BIT(n) (1U << (n))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define OFFSET_OF(t, m) ((uptr)&((t *)NULL)->m)
#define CONTAINER_OF(mp, t, mn) ((t *)((uptr)mp - OFFSET_OF(t, mn)))

#define byte_swap_16(num) ((((num) >> 8) & 0xff) | (((num) << 8) & 0xff00))
#define byte_swap_32(num) ((((num) >> 24) & 0xff) | (((num) << 8) & 0xff0000) | \
						(((num) >> 8 )& 0xff00) | (((num) << 24) & 0xff000000))


/* Bootloader/Nyx */
#define BOOT_CFG_AUTOBOOT_EN BIT(0)
#define BOOT_CFG_FROM_LAUNCH BIT(1)
#define BOOT_CFG_FROM_ID     BIT(2)
#define BOOT_CFG_TO_EMUMMC   BIT(3)

#define EXTRA_CFG_KEYS    BIT(0)
#define EXTRA_CFG_PAYLOAD BIT(1)
#define EXTRA_CFG_MODULE  BIT(2)

#define EXTRA_CFG_NYX_UMS    BIT(5)
#define EXTRA_CFG_NYX_RELOAD BIT(6)

typedef enum _nyx_ums_type
{
	NYX_UMS_SD_CARD = 0,
	NYX_UMS_EMMC_BOOT0,
	NYX_UMS_EMMC_BOOT1,
	NYX_UMS_EMMC_GPP,
	NYX_UMS_EMUMMC_BOOT0,
	NYX_UMS_EMUMMC_BOOT1,
	NYX_UMS_EMUMMC_GPP
} nyx_ums_type;

typedef struct __attribute__((__packed__)) _boot_cfg_t
{
	u8 boot_cfg;
	u8 autoboot;
	u8 autoboot_list;
	u8 extra_cfg;
	union
	{
		struct
		{
			char id[8]; // 7 char ASCII null terminated.
			char emummc_path[0x78]; // emuMMC/XXX, ASCII null terminated.
		};
		u8 ums; // nyx_ums_type.
		u8 xt_str[0x80];
	};
} boot_cfg_t;

static_assert(sizeof(boot_cfg_t) == 0x84, "Boot cfg storage size is wrong!");

typedef struct __attribute__((__packed__)) _ipl_ver_meta_t
{
	u32 magic;
	u32 version;
	u16 rsvd0;
	u16 rsvd1;
} ipl_ver_meta_t;

typedef struct __attribute__((__packed__)) _reloc_meta_t
{
	u32 start;
	u32 stack;
	u32 end;
	u32 ep;
} reloc_meta_t;

#endif
