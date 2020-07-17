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
#include <utils/types.h>
#include <utils/ini.h>
#include <sec/tsec.h>

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

typedef struct _hos_eks_keys_t
{
	u8 mkk[0x10];
	u8 fdk[0x10];
} hos_eks_keys_t;

typedef struct _hos_eks_bis_keys_t
{
	u8 crypt[0x10];
	u8 tweak[0x10];
} hos_eks_bis_keys_t;

typedef struct _hos_eks_mbr_t
{
	u32 magic;
	u8  enabled[5];
	u8  enabled_bis;
	u8  rsvd[2];
	u32 sbk_low;
	u8  dkg[0x10];
	u8  dkk[0x10];
	hos_eks_keys_t keys[5];
	hos_eks_bis_keys_t bis_keys[3];
} hos_eks_mbr_t;

static_assert(sizeof(hos_eks_mbr_t) == 304, "HOS EKS size is wrong!");

typedef struct _launch_ctxt_t
{
	void *keyblob;

	void *pkg1;
	const pkg1_id_t *pkg1_id;

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

	ini_sec_t *cfg;
} launch_ctxt_t;

void hos_eks_get();
void hos_eks_save(u32 kb);
void hos_eks_clear(u32 kb);
void hos_eks_bis_save();
void hos_eks_bis_clear();
int  hos_keygen(u8 *keyblob, u32 kb, tsec_ctxt_t *tsec_ctxt);
int  hos_bis_keygen(u8 *keyblob, u32 kb, tsec_ctxt_t *tsec_ctxt);
void hos_bis_keys_clear();

#endif
