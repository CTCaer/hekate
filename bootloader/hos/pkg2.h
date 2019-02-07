/*
 * Copyright (c) 2018 naehrwert
 * Copyright (C) 2018 CTCaer
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

#include "../utils/types.h"
#include "../utils/list.h"

#define PKG2_MAGIC 0x31324B50
#define PKG2_SEC_BASE 0x80000000
#define PKG2_SEC_KERNEL 0
#define PKG2_SEC_INI1 1

#define INI1_MAGIC 0x31494E49

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
	// Generic instruction patches
	SVC_VERIFY_DS = 0x10, // 0x0-0xF are RESERVED.
	DEBUG_MODE_EN,
	ATM_GEN_PATCH,
	// >4 bytes patches. Value is a pointer of a u32 array.
	ATM_ARR_PATCH,
	DEBUG_OUTPUT_GEN,
	DEBUG_OUTPUT_ARR,
};

typedef struct _pkg2_hdr_t
{
	u8 ctr[0x10];
	u8 sec_ctr[0x40];
	u32 magic;
	u32 base;
	u32 pad0;
	u16 version;
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
	u32 magic;
	u8 name[12];
	u64 tid;
	u32 proc_cat;
	u8 main_thrd_prio;
	u8 def_cpu_core;
	u8 res;
	u8 flags;
	pkg2_kip1_sec_t sections[KIP1_NUM_SECTIONS];
	u32 caps[0x20];
	u8 data[];
} pkg2_kip1_t;

typedef struct _pkg2_kip1_info_t
{
	pkg2_kip1_t *kip1;
	u32 size;
	link_t link;
} pkg2_kip1_info_t;

typedef struct _pkg2_kernel_id_t
{
	u32 crc32c_id;
	kernel_patch_t *kernel_patchset;
} pkg2_kernel_id_t;

typedef struct _kip1_patch_t
{
	u32 offset; //section+offset of patch to apply
	u32 length; //in bytes, 0 means last patch
	const char* srcData; //that must match
	const char* dstData; //that it gets replaced by
} kip1_patch_t;

typedef struct _kip1_patchset_t
{
	const char* name; //NULL means end
	kip1_patch_t* patches; //NULL means not necessary
} kip1_patchset_t;

typedef struct _kip1_id_t
{
	const char* name;
	u8 hash[16];
	kip1_patchset_t* patchset;
} kip1_id_t;

void pkg2_parse_kips(link_t *info, pkg2_hdr_t *pkg2);
int pkg2_has_kip(link_t *info, u64 tid);
void pkg2_replace_kip(link_t *info, u64 tid, pkg2_kip1_t *kip1);
void pkg2_add_kip(link_t *info, pkg2_kip1_t *kip1);
void pkg2_merge_kip(link_t *info, pkg2_kip1_t *kip1);
const char* pkg2_patch_kips(link_t *info, char* patchNames);

const pkg2_kernel_id_t *pkg2_identify(u32 id);
pkg2_hdr_t *pkg2_decrypt(void *data);
void pkg2_build_encrypt(void *dst, void *kernel, u32 kernel_size, link_t *kips_info);

#endif
