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

#define PKG2_LOAD_ADDR  0xA9800000

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

#define ATM_MESOSPHERE 0x3053534D

extern u32 pkg2_newkern_ini1_info;
extern u32 pkg2_newkern_ini1_start;
extern u32 pkg2_newkern_ini1_end;

typedef struct _kernel_patch_t
{
	u32  id;
	u32  off;
	u32  val;
	const u32 *ptr;
} kernel_patch_t;

#define KERNEL_PATCHSET_DEF(name, ...) \
	static const kernel_patch_t name[] = { \
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

typedef struct _pkg2_kernel_id_t
{
	u8 hash[8];
	const kernel_patch_t *kernel_patchset;
} pkg2_kernel_id_t;

#define KIP1_PATCH_SRC_NO_CHECK (char *)(-1)

typedef struct _kip1_patch_t
{
	u32   offset;   // section+offset of patch to apply.
	u32   length;   // In bytes, 0 means last patch.
	char *src_data; // That must match.
	char *dst_data; // That it gets replaced by.
} kip1_patch_t;

typedef struct _kip1_patchset_t
{
	char *name;            // NULL means end.
	const kip1_patch_t *patches; // NULL means not necessary.
} kip1_patchset_t;

typedef struct _kip1_id_t
{
	const char *name;
	u8 hash[8];
	const kip1_patchset_t *patchset;
} kip1_id_t;

/*
 * NX NC - Package2 BOOT Configuration
 *
 * This is taken from the first 1KB of a Package2 partition.
 * On retail, it's not zeroes, but on devkits it has actual data.
 * The signed part is reserved for special in-house devkits.
 */
#define NX_BC1_ADDR 0x4003D000
#define NX_BC6_ADDR 0x4003F800

#define NX_BC_DBG_FLAG00_DEV_MODE  BIT(1)
#define NX_BC_DBG_FLAG00_SERROR    BIT(2)
#define NX_BC_DBG_FLAG00_ALL_FLAGS 0xFF

#define NX_BC_HW_FLAG01_ALL_FLAGS  0xFF
#define NX_BC_HW_FLAG03_MEM_MODE   0xFF
#define NX_BC_HW_FLAG04_CNTCV_SET  BIT(0)

#define NX_BC_PKG2_FLAG_DECRYPTED  BIT(0)
#define NX_BC_PKG2_FLAG_UNSIGNED   BIT(1)

#define NX_BC_NCA_FLAG_UNSIGNED    BIT(0)

typedef struct _nx_bc_t {
	/* Unsigned section */
	u32 version;
	u32 rsvd0[3];
	u8  dbg_flags[0x10]; // Normally all are set to 0xFF if devkit.
	u8  hw_flags[0x10];
	u64 cntcv_value;
	u8  padding0[0x1C8];

	u8 rsa_pss_signature[0x100];

	/* Signed section */
	u32 version_signed;
	u32 rsvd1;
	u8  pkg2_flags;
	u8  padding1[7];
	u32 ecid[4];
	u8  nca_flags[0x10];
	u8  unk_flags[0x10];
	u8  padding2[0xC0];
} nx_bc_t;

bool pkg2_parse_kips(link_t *info, pkg2_hdr_t *pkg2, bool *new_pkg2);
int  pkg2_has_kip(link_t *info, u64 tid);
void pkg2_replace_kip(link_t *info, u64 tid, pkg2_kip1_t *kip1);
void pkg2_add_kip(link_t *info, pkg2_kip1_t *kip1);
void pkg2_merge_kip(link_t *info, pkg2_kip1_t *kip1);
void pkg2_get_ids(kip1_id_t **ids, u32 *entries);
const char *pkg2_patch_kips(link_t *info, char *patch_names);

const pkg2_kernel_id_t *pkg2_identify(const u8 *hash);
pkg2_hdr_t *pkg2_decrypt(void *data, u8 mkey, bool is_exo);
void pkg2_build_encrypt(void *dst, void *hos_ctxt, link_t *kips_info, bool is_exo);

#endif
