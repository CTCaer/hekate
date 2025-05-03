/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2024 CTCaer
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

#ifndef _HOS_H_
#define _HOS_H_

#include <bdk.h>

#include "pkg1.h"
#include "pkg2.h"

#include <assert.h>

//!TODO: Update on mkey changes.
enum {
	HOS_KB_VERSION_100  = 0,
	HOS_KB_VERSION_300  = 1,
	HOS_KB_VERSION_301  = 2,
	HOS_KB_VERSION_400  = 3,
	HOS_KB_VERSION_500  = 4,
	HOS_KB_VERSION_600  = 5,
	HOS_KB_VERSION_620  = 6,
	HOS_KB_VERSION_700  = 7,
	HOS_KB_VERSION_810  = 8,
	HOS_KB_VERSION_900  = 9,
	HOS_KB_VERSION_910  = 10,
	HOS_KB_VERSION_1210 = 11,
	HOS_KB_VERSION_1300 = 12,
	HOS_KB_VERSION_1400 = 13,
	HOS_KB_VERSION_1500 = 14,
	HOS_KB_VERSION_1600 = 15,
	HOS_KB_VERSION_1700 = 16,
	HOS_KB_VERSION_1800 = 17,
	HOS_KB_VERSION_1900 = 18,
	HOS_KB_VERSION_2000 = 19,
	HOS_KB_VERSION_MAX  = HOS_KB_VERSION_2000
};

#define HOS_TSEC_VERSION 4 //! TODO: Update on TSEC Root Key changes.

#define HOS_PKG11_MAGIC  0x31314B50
#define HOS_EKS_MAGIC    0x31534B45 // EKS1.
#define HOS_EKS_TSEC_VER (HOS_KB_VERSION_700 + HOS_TSEC_VERSION)

// Use official Mariko secmon when in stock. Needs access to TZRAM.
//#define HOS_MARIKO_STOCK_SECMON

typedef struct _exo_ctxt_t
{
	u32  hos_revision;
	bool no_user_exceptions;
	bool user_pmu;
	bool *usb3_force;
	bool *cal0_blank;
	bool *cal0_allow_writes_sys;
} exo_ctxt_t;

typedef struct _hos_eks_mbr_t
{
	u32 magic;
	u32 enabled;
	u32 lot0;
	u32 rsvd;
	u8  tsec[SE_KEY_128_SIZE];
	u8  troot[SE_KEY_128_SIZE];
	u8  troot_dev[SE_KEY_128_SIZE];
} hos_eks_mbr_t;

static_assert(sizeof(hos_eks_mbr_t) == 64, "HOS EKS size is wrong!");

typedef struct _launch_ctxt_t
{
	void *keyblob;

	void *pkg1;
	const pkg1_id_t *pkg1_id;
	const pkg2_kernel_id_t *pkg2_kernel_id;

	void *warmboot;
	u32   warmboot_size;
	void *secmon;
	u32   secmon_size;
	void *exofatal;
	u32   exofatal_size;

	void *pkg2;
	u32   pkg2_size;
	bool  new_pkg2;

	void *kernel;
	u32   kernel_size;

	link_t kip1_list;
	char*  kip1_patches;

	bool svcperm;
	bool debugmode;
	bool stock;
	bool emummc_forced;

	void *fss0;
	u32   fss0_hosver;
	bool  atmosphere;

	int ucid;

	exo_ctxt_t exo_ctx;

	ini_sec_t *cfg;
} launch_ctxt_t;

typedef struct _merge_kip_t
{
	void *kip1;
	link_t link;
} merge_kip_t;

void hos_eks_clear(u32 kb);
int  hos_launch(ini_sec_t *cfg);
int  hos_keygen(void *keyblob, u32 kb, tsec_ctxt_t *tsec_ctxt, bool stock, bool is_exo);

#endif
