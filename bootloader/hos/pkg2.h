/*
 * Copyright (c) 2018 naehrwert
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

#ifndef _PKG2_H_
#define _PKG2_H_

#include <bdk.h>

#define PKG2_MAGIC 0x31324B50
#define PKG2_SEC_BASE 0x80000000
#define PKG2_SEC_KERNEL 0
#define PKG2_SEC_INI1 1

#define INI1_MAGIC 0x31494E49
#define PKG2_NEWKERN_GET_INI1_HEURISTIC 0xD2800015 // Offset of OP + 12 is the INI1 offset.
#define PKG2_NEWKERN_START 0x800

#define ATM_MESOSPHERE 0x3053534D

extern u32 pkg2_newkern_ini1_val;
extern u32 pkg2_newkern_ini1_start;
extern u32 pkg2_newkern_ini1_end;

typedef struct _kernel_patch_t
{
	u32 id;
	u32 off;
	u32 val;
	u32 *ptr;
} kernel_patch_t;

#define KERNEL_PATCHSET_DEF(name, ...) \
	kernel_patch_t name[] = { \
		__VA_ARGS__, \
		{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, (u32 *)0xFFFFFFFF} \
	}

enum
{
	// Always applied.
	SVC_GENERIC   = 0,
	// Generic instruction patches.
	SVC_VERIFY_DS = 0x10,
	DEBUG_MODE_EN = 0x11,
	ATM_GEN_PATCH = 0x12,
	ATM_SYSM_INCR = ATM_GEN_PATCH,
	// >4 bytes patches. Value is a pointer of a u32 array.
	ATM_ARR_PATCH = 0x13,
};

typedef struct _pkg2_hdr_t
{
	u8 ctr[0x10];
	u8 sec_ctr[0x40];
	u32 magic;
	u32 base;
	u32 pad0;
	u8  pkg2_ver;
	u8  bl_ver;
	u16 pad1;
	u32 sec_size[4];
	u32 sec_off[4];
	u8 sec_sha256[0x80];
	u8 data[];
} pkg2_hdr_t;

typedef struct _pkg2_ini1_t
{
	u32 magic;
	u32 size;
	u32 num_procs;
	u32 pad;
} pkg2_ini1_t;

typedef struct _pkg2_kip1_sec_t
{
	u32 offset;
	u32 size_decomp;
	u32 size_comp;
	u32 attrib;
} pkg2_kip1_sec_t;

#define KIP1_NUM_SECTIONS 6

typedef struct _pkg2_kip1_t
{
/* 0x000 */	u32 magic;
/* 0x004*/	u8 name[12]; 
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

typedef struct _pkg2_kernel_id_t
{
	u8 hash[8];
	kernel_patch_t *kernel_patchset;
} pkg2_kernel_id_t;

typedef struct _kip1_patch_t
{
	u32 offset; // section+offset of patch to apply.
	u32 length; // In bytes, 0 means last patch.
	char* srcData; // That must match.
	char* dstData; // That it gets replaced by.
} kip1_patch_t;

typedef struct _kip1_patchset_t
{
	char* name; // NULL means end.
	kip1_patch_t* patches; // NULL means not necessary.
} kip1_patchset_t;

typedef struct _kip1_id_t
{
	const char* name;
	u8 hash[8];
	kip1_patchset_t* patchset;
} kip1_id_t;

void pkg2_get_newkern_info(u8 *kern_data);
bool pkg2_parse_kips(link_t *info, pkg2_hdr_t *pkg2, bool *new_pkg2);
int  pkg2_has_kip(link_t *info, u64 tid);
void pkg2_replace_kip(link_t *info, u64 tid, pkg2_kip1_t *kip1);
void pkg2_add_kip(link_t *info, pkg2_kip1_t *kip1);
void pkg2_merge_kip(link_t *info, pkg2_kip1_t *kip1);
void pkg2_get_ids(kip1_id_t **ids, u32 *entries);
const char* pkg2_patch_kips(link_t *info, char* patchNames);

const pkg2_kernel_id_t *pkg2_identify(u8 *hash);
pkg2_hdr_t *pkg2_decrypt(void *data, u8 kb, bool is_exo);
void pkg2_build_encrypt(void *dst, void *hos_ctxt, link_t *kips_info, bool is_exo);

#endif
