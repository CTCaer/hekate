/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 CTCaer
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

#include "pkg1.h"
#include "pkg2.h"
#include "../utils/types.h"
#include "../config/ini.h"
#include "../sec/tsec.h"

#include <assert.h>

#define KB_FIRMWARE_VERSION_100_200 0
#define KB_FIRMWARE_VERSION_300 1
#define KB_FIRMWARE_VERSION_301 2
#define KB_FIRMWARE_VERSION_400 3
#define KB_FIRMWARE_VERSION_500 4
#define KB_FIRMWARE_VERSION_600 5
#define KB_FIRMWARE_VERSION_620 6
#define KB_FIRMWARE_VERSION_700 7
#define KB_FIRMWARE_VERSION_810 8
#define KB_FIRMWARE_VERSION_900 9
#define KB_FIRMWARE_VERSION_910 10
#define KB_FIRMWARE_VERSION_MAX KB_FIRMWARE_VERSION_910

#define HOS_PKG11_MAGIC 0x31314B50
#define HOS_EKS_MAGIC   0x30534B45

typedef struct _exo_ctxt_t
{
	bool no_user_exceptions;
	bool user_pmu;
	bool *cal0_blank;
	bool *cal0_allow_writes_sys;
} exo_ctxt_t;

typedef struct _hos_eks_keys_t
{
	u8 dkg[0x10];
	u8 mkk[0x10];
	u8 fdk[0x10];
	u8 dkk[0x10];
} hos_eks_keys_t;

typedef struct _hos_eks_mbr_t
{
	u32 magic;
	u32 enabled;
	u32 sbk_low[2];
	hos_eks_keys_t keys[6];
	u32 magic2;
	u32 rsvd2[3];
} hos_eks_mbr_t;

static_assert(sizeof(hos_eks_mbr_t) < 424, "HOS EKS storage bigger than MBR!");

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

	void *pkg2;
	u32   pkg2_size;
	bool  new_pkg2;

	void  *kernel;
	u32    kernel_size;
	link_t kip1_list;
	char*  kip1_patches;

	u32  fss0_hosver;
	bool svcperm;
	bool debugmode;
	bool stock;
	bool atmosphere;
	bool fss0_enable_experimental;
	bool emummc_forced;

	exo_ctxt_t exo_cfg;

	ini_sec_t *cfg;
} launch_ctxt_t;

typedef struct _merge_kip_t
{
	void *kip1;
	link_t link;
} merge_kip_t;

void hos_eks_get();
void hos_eks_save(u32 kb);
void hos_eks_clear(u32 kb);
int  hos_launch(ini_sec_t *cfg);
int  hos_keygen(u8 *keyblob, u32 kb, tsec_ctxt_t *tsec_ctxt, launch_ctxt_t *hos_ctxt);

#endif
