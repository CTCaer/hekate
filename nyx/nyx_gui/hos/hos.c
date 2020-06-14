/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 st4rk
 * Copyright (c) 2018 Ced2911
 * Copyright (c) 2018-2020 CTCaer
 * Copyright (c) 2018 balika011
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

#include <string.h>

#include "hos.h"
#include "sept.h"
#include "../config/config.h"
#include "../gfx/di.h"
#include "../gfx/gfx.h"
#include "../mem/heap.h"
#include "../mem/mc.h"
#include "../sec/se.h"
#include "../sec/se_t210.h"
#include "../sec/tsec.h"
#include "../soc/bpmp.h"
#include "../soc/fuse.h"
#include "../soc/pmc.h"
#include "../soc/smmu.h"
#include "../soc/t210.h"
#include "../storage/mbr_gpt.h"
#include "../storage/nx_emmc.h"
#include "../storage/nx_sd.h"
#include "../storage/sdmmc.h"
#include "../utils/util.h"

extern hekate_config h_cfg;

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

#define PKG2_LOAD_ADDR 0xA9800000

 // Secmon mailbox.
#define SECMON_MB_ADDR  0x40002EF8
#define SECMON7_MB_ADDR 0x400000F8
typedef struct _secmon_mailbox_t
{
	//  < 4.0.0 Signals - 0: Not ready, 1: BCT ready, 2: DRAM and pkg2 ready, 3: Continue boot.
	// >= 4.0.0 Signals - 0: Not ready, 1: BCT ready, 2: DRAM ready, 4: pkg2 ready and continue boot.
	u32 in;
	// Non-zero: Secmon ready.
	u32 out;
} secmon_mailbox_t;

static const u8 keyblob_keyseeds[][0x10] = {
	{ 0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3 }, // 1.0.0.
	{ 0x0C, 0x25, 0x61, 0x5D, 0x68, 0x4C, 0xEB, 0x42, 0x1C, 0x23, 0x79, 0xEA, 0x82, 0x25, 0x12, 0xAC }, // 3.0.0.
	{ 0x33, 0x76, 0x85, 0xEE, 0x88, 0x4A, 0xAE, 0x0A, 0xC2, 0x8A, 0xFD, 0x7D, 0x63, 0xC0, 0x43, 0x3B }, // 3.0.1.
	{ 0x2D, 0x1F, 0x48, 0x80, 0xED, 0xEC, 0xED, 0x3E, 0x3C, 0xF2, 0x48, 0xB5, 0x65, 0x7D, 0xF7, 0xBE }, // 4.0.0.
	{ 0xBB, 0x5A, 0x01, 0xF9, 0x88, 0xAF, 0xF5, 0xFC, 0x6C, 0xFF, 0x07, 0x9E, 0x13, 0x3C, 0x39, 0x80 }, // 5.0.0.
	{ 0xD8, 0xCC, 0xE1, 0x26, 0x6A, 0x35, 0x3F, 0xCC, 0x20, 0xF3, 0x2D, 0x3B, 0x51, 0x7D, 0xE9, 0xC0 }  // 6.0.0.
};

static const u8 cmac_keyseed[0x10] =
	{ 0x59, 0xC7, 0xFB, 0x6F, 0xBE, 0x9B, 0xBE, 0x87, 0x65, 0x6B, 0x15, 0xC0, 0x53, 0x73, 0x36, 0xA5 };

static const u8 master_keyseed_retail[0x10] =
	{ 0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C };

static const u8 console_keyseed[0x10] =
	{ 0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78 };

const u8 package2_keyseed[0x10] =
	{ 0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7 };

static const u8 master_keyseed_4xx_5xx_610[0x10] =
	{ 0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66 };

static const u8 master_keyseed_620[0x10] =
	{ 0x37, 0x4B, 0x77, 0x29, 0x59, 0xB4, 0x04, 0x30, 0x81, 0xF6, 0xE5, 0x8C, 0x6D, 0x36, 0x17, 0x9A };

static const u8 console_keyseed_4xx_5xx[0x10] =
	{ 0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28 };

bool hos_eks_rw_try(u8 *buf, bool write)
{
	for (u32 i = 0; i < 3; i++)
	{
		if (!write)
		{
			if (sdmmc_storage_read(&sd_storage, 0, 1, buf))
				return true;
		}
		else
		{
			if (sdmmc_storage_write(&sd_storage, 0, 1, buf))
				return true;
		}
	}

	return false;
}

void hos_eks_get()
{
	// Check if EKS already found and parsed.
	if (!h_cfg.eks)
	{
		// Read EKS blob.
		u8 *mbr = calloc(512 , 1);
		if (!hos_eks_rw_try(mbr, false))
			goto out;

		// Decrypt EKS blob.
		hos_eks_mbr_t *eks = (hos_eks_mbr_t *)(mbr + 0x60);
		se_aes_crypt_ecb(14, 0, eks, sizeof(hos_eks_mbr_t), eks, sizeof(hos_eks_mbr_t));

		// Check if valid and for this unit.
		if (eks->magic == HOS_EKS_MAGIC &&
			eks->sbk_low == FUSE(FUSE_PRIVATE_KEY0))
		{
			h_cfg.eks = eks;
			return;
		}

out:
		free(mbr);
	}
}

void hos_eks_save(u32 kb)
{
	if (kb >= KB_FIRMWARE_VERSION_700)
	{
		u32 key_idx = 0;
		if (kb >= KB_FIRMWARE_VERSION_810)
			key_idx = 1;

		bool new_eks = false;
		if (!h_cfg.eks)
		{
			h_cfg.eks = calloc(512 , 1);
			new_eks = true;
		}

		// If matching blob doesn't exist, create it.
		bool update_eks = key_idx ? (h_cfg.eks->enabled[key_idx] < kb) : !h_cfg.eks->enabled[0];
		if (update_eks)
		{
			// Read EKS blob.
			u8 *mbr = calloc(512 , 1);
			if (!hos_eks_rw_try(mbr, false))
			{
				if (new_eks)
				{
					free(h_cfg.eks);
					h_cfg.eks = NULL;
				}

				goto out;
			}

			// Get keys.
			u8 *keys = (u8 *)calloc(0x1000, 1);
			se_get_aes_keys(keys + 0x800, keys, 0x10);

			// Set magic and personalized info.
			h_cfg.eks->magic = HOS_EKS_MAGIC;
			h_cfg.eks->enabled[key_idx] = kb;
			h_cfg.eks->sbk_low = FUSE(FUSE_PRIVATE_KEY0);

			// Copy new keys.
			memcpy(h_cfg.eks->dkg, keys + 10 * 0x10, 0x10);
			memcpy(h_cfg.eks->dkk, keys + 15 * 0x10, 0x10);

			memcpy(h_cfg.eks->keys[key_idx].mkk, keys + 12 * 0x10, 0x10);
			memcpy(h_cfg.eks->keys[key_idx].fdk, keys + 13 * 0x10, 0x10);

			// Encrypt EKS blob.
			u8 *eks = calloc(512 , 1);
			memcpy(eks, h_cfg.eks, sizeof(hos_eks_mbr_t));
			se_aes_crypt_ecb(14, 1, eks, sizeof(hos_eks_mbr_t), eks, sizeof(hos_eks_mbr_t));

			// Write EKS blob to SD.
			memcpy(mbr + 0x60, eks, sizeof(hos_eks_mbr_t));
			hos_eks_rw_try(mbr, true);


			free(eks);
			free(keys);
out:
			free(mbr);
		}
	}
}

void hos_eks_clear(u32 kb)
{
	if (h_cfg.eks && kb >= KB_FIRMWARE_VERSION_700)
	{
		u32 key_idx = 0;
		if (kb >= KB_FIRMWARE_VERSION_810)
			key_idx = 1;

		// Check if Current Master key is enabled.
		if (h_cfg.eks->enabled[key_idx])
		{
			// Read EKS blob.
			u8 *mbr = calloc(512 , 1);
			if (!hos_eks_rw_try(mbr, false))
				goto out;

			// Disable current Master key version.
			h_cfg.eks->enabled[key_idx] = 0;

			// Encrypt EKS blob.
			u8 *eks = calloc(512 , 1);
			memcpy(eks, h_cfg.eks, sizeof(hos_eks_mbr_t));
			se_aes_crypt_ecb(14, 1, eks, sizeof(hos_eks_mbr_t), eks, sizeof(hos_eks_mbr_t));

			// Write EKS blob to SD.
			memcpy(mbr + 0x60, eks, sizeof(hos_eks_mbr_t));
			hos_eks_rw_try(mbr, true);

			EMC(EMC_SCRATCH0) &= ~EMC_SEPT_RUN;
			h_cfg.sept_run = false;

			free(eks);
out:
			free(mbr);
		}
	}
}

int hos_keygen(u8 *keyblob, u32 kb, tsec_ctxt_t *tsec_ctxt)
{
	u8 tmp[0x20];
	u32 retries = 0;

	if (kb > KB_FIRMWARE_VERSION_MAX)
		return 0;

	if (kb <= KB_FIRMWARE_VERSION_600)
		tsec_ctxt->size = 0xF00;
	else if (kb == KB_FIRMWARE_VERSION_620)
		tsec_ctxt->size = 0x2900;
	else if (kb == KB_FIRMWARE_VERSION_700)
		tsec_ctxt->size = 0x3000;
	else
		tsec_ctxt->size = 0x3300;

	// Prepare smmu tsec page for 6.2.0.
	if (kb == KB_FIRMWARE_VERSION_620)
	{
		u8 *tsec_paged = (u8 *)page_alloc(3);
		memcpy(tsec_paged, (void *)tsec_ctxt->fw, tsec_ctxt->size);
		tsec_ctxt->fw = tsec_paged;
	}

	// Get TSEC key.
	if (kb <= KB_FIRMWARE_VERSION_620)
	{
		while (tsec_query(tmp, kb, tsec_ctxt) < 0)
		{
			memset(tmp, 0x00, 0x20);
			retries++;

			// We rely on racing conditions, make sure we cover even the unluckiest cases.
			if (retries > 15)
			{
				EPRINTF("\nFailed to get TSEC keys. Please try again.\n");
				return 0;
			}
		}
	}

	if (kb >= KB_FIRMWARE_VERSION_700)
	{
		// Use HOS EKS if it exists.
		u32 key_idx = 0;
		if (kb >= KB_FIRMWARE_VERSION_810)
			key_idx = 1;

		if (h_cfg.eks && h_cfg.eks->enabled[key_idx] >= kb)
		{
			// Set Device keygen key to slot 10.
			se_aes_key_set(10, h_cfg.eks->dkg, 0x10);
			// Set Master key to slot 12.
			se_aes_key_set(12, h_cfg.eks->keys[key_idx].mkk, 0x10);
			// Set FW Device key key to slot 13.
			se_aes_key_set(13, h_cfg.eks->keys[key_idx].fdk, 0x10);
			// Set Device key to slot 15.
			se_aes_key_set(15, h_cfg.eks->dkk, 0x10);
		}

		se_aes_key_clear(8);
		se_aes_unwrap_key(8, 12, package2_keyseed);
	}
	else if (kb == KB_FIRMWARE_VERSION_620)
	{
		// Set TSEC key.
		se_aes_key_set(12, tmp, 0x10);
		// Set TSEC root key.
		se_aes_key_set(13, tmp + 0x10, 0x10);

		// Package2 key.
		se_aes_key_set(8, tmp + 0x10, 0x10);
		se_aes_unwrap_key(8, 8, master_keyseed_620);
		se_aes_unwrap_key(8, 8, master_keyseed_retail);
		se_aes_unwrap_key(8, 8, package2_keyseed);
	}
	else
	{
		// Set TSEC key.
		se_aes_key_set(13, tmp, 0x10);

		// Derive keyblob keys from TSEC+SBK.
		se_aes_crypt_block_ecb(13, 0, tmp, keyblob_keyseeds[0]);
		se_aes_unwrap_key(15, 14, tmp);
		se_aes_crypt_block_ecb(13, 0, tmp, keyblob_keyseeds[kb]);
		se_aes_unwrap_key(13, 14, tmp);

		// Clear SBK.
		se_aes_key_clear(14);

		//TODO: verify keyblob CMAC.
		//se_aes_unwrap_key(11, 13, cmac_keyseed);
		//se_aes_cmac(tmp, 0x10, 11, keyblob + 0x10, 0xA0);
		//if (!memcmp(keyblob, tmp, 0x10))
		//	return 0;

		se_aes_crypt_block_ecb(13, 0, tmp, cmac_keyseed);
		se_aes_unwrap_key(11, 13, cmac_keyseed);

		// Decrypt keyblob and set keyslots.
		se_aes_crypt_ctr(13, keyblob + 0x20, 0x90, keyblob + 0x20, 0x90, keyblob + 0x10);
		se_aes_key_set(11, keyblob + 0x20 + 0x80, 0x10); // Package1 key.
		se_aes_key_set(12, keyblob + 0x20, 0x10);
		se_aes_key_set(13, keyblob + 0x20, 0x10);

		se_aes_crypt_block_ecb(12, 0, tmp, master_keyseed_retail);

		switch (kb)
		{
		case KB_FIRMWARE_VERSION_100_200:
		case KB_FIRMWARE_VERSION_300:
		case KB_FIRMWARE_VERSION_301:
			se_aes_unwrap_key(13, 15, console_keyseed);
			se_aes_unwrap_key(12, 12, master_keyseed_retail);
			break;
		case KB_FIRMWARE_VERSION_400:
			se_aes_unwrap_key(13, 15, console_keyseed_4xx_5xx);
			se_aes_unwrap_key(15, 15, console_keyseed);
			se_aes_unwrap_key(14, 12, master_keyseed_4xx_5xx_610);
			se_aes_unwrap_key(12, 12, master_keyseed_retail);
			break;
		case KB_FIRMWARE_VERSION_500:
		case KB_FIRMWARE_VERSION_600:
			se_aes_unwrap_key(10, 15, console_keyseed_4xx_5xx);
			se_aes_unwrap_key(15, 15, console_keyseed);
			se_aes_unwrap_key(14, 12, master_keyseed_4xx_5xx_610);
			se_aes_unwrap_key(12, 12, master_keyseed_retail);
			break;
		}

		// Package2 key.
		se_aes_unwrap_key(8, 12, package2_keyseed);
	}

	return 1;
}
