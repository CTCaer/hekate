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

#ifndef _PKG2_H_
#define _PKG2_H_

#include <bdk.h>

#define PKG2_MAGIC      0x31324B50
#define PKG2_SEC_BASE   0x80000000
#define PKG2_SEC_KERNEL 0
#define PKG2_SEC_INI1   1
#define PKG2_SEC_UNUSED 2

#define INI1_MAGIC 0x31494E49

//! TODO: Update on kernel change if needed.
// Offset of OP + 12 is the INI1 offset. On v2 with dynamic crt0 it's + 16.
#define PKG2_NEWKERN_GET_INI1_HEURISTIC 0xD2800015 // MOV X21, #0.
#define PKG2_NEWKERN_START 0x800

extern u32 pkg2_newkern_ini1_start;
extern u32 pkg2_newkern_ini1_end;

typedef struct _pkg2_hdr_t
{
/* 0x000 */	u8  ctr[0x10];
/* 0x010 */	u8  sec_ctr[0x40];
/* 0x050 */	u32 magic;
/* 0x054 */	u32 base;
/* 0x058 */	u32 pad0;
/* 0x05C */	u8  pkg2_ver;
/* 0x05D */	u8  bl_ver;
/* 0x05E */	u16 pad1;
/* 0x060 */	u32 sec_size[4];
/* 0x070 */	u32 sec_off[4];
/* 0x080 */	u8  sec_sha256[0x80];
/* 0x100 */	u8  data[];
} pkg2_hdr_t;

typedef struct _pkg2_ini1_t
{
	u32 magic;
	u32 size;
	u32 num_procs;
	u32 pad;
} pkg2_ini1_t;

enum kip_offset_section
{
	KIP_TEXT    = 0,
	KIP_RODATA  = 1,
	KIP_DATA    = 2,
	KIP_BSS     = 3,
	KIP_UNKSEC1 = 4,
	KIP_UNKSEC2 = 5
};

#define KIP1_NUM_SECTIONS 6

typedef struct _pkg2_kip1_sec_t
{
	u32 offset;
	u32 size_decomp;
	u32 size_comp;
	u32 attrib;
} pkg2_kip1_sec_t;

typedef struct _pkg2_kip1_t
{
/* 0x000 */	u32 magic;
/* 0x004 */	char name[12];
/* 0x010 */	u64 tid;
/* 0x018 */	u32 proc_cat;
/* 0x01C */	u8 main_thrd_prio;
/* 0x01D */	u8 def_cpu_core;
/* 0x01E */	u8 res;
/* 0x01F */	u8 flags;
/* 0x020 */	pkg2_kip1_sec_t sections[KIP1_NUM_SECTIONS];
/* 0x080 */	u32 caps[0x20];
/* 0x100 */	u8 data[];
} pkg2_kip1_t;

typedef struct _pkg2_kip1_info_t
{
	pkg2_kip1_t *kip1;
	u32 size;
	link_t link;
} pkg2_kip1_info_t;

void pkg2_get_newkern_info(u8 *kern_data);
u32  pkg2_calc_kip1_size(pkg2_kip1_t *kip1);

pkg2_hdr_t *pkg2_decrypt(void *data, u8 mkey);

#endif
