/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 st4rk
 * Copyright (c) 2018 Ced2911
 * Copyright (c) 2018-2022 CTCaer
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

#include <bdk.h>

#include "hos.h"
#include "hos_config.h"
#include "secmon_exo.h"
#include "../config.h"
#include "../storage/emummc.h"

extern hekate_config h_cfg;

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

#define EHPRINTFARGS(text, args...) \
	({  gfx_con.mute = false; \
		gfx_printf("%k"text"%k\n", 0xFFFF0000, args, 0xFFCCCCCC); })

#define PKG2_LOAD_ADDR 0xA9800000

#define SECMON_BCT_CFG_ADDR  0x4003D000
#define SECMON6_BCT_CFG_ADDR 0x4003F800

 // Secmon mailbox.
#define SECMON_MAILBOX_ADDR  0x40002E00
#define SECMON7_MAILBOX_ADDR 0x40000000
#define  SECMON_STATE_OFFSET 0xF8

typedef enum
{
	SECMON_STATE_NOT_READY    = 0,

	PKG1_STATE_NOT_READY      = 0,
	PKG1_STATE_BCT_COPIED     = 1,
	PKG1_STATE_DRAM_READY     = 2,
	PKG1_STATE_PKG2_READY_OLD = 3,
	PKG1_STATE_PKG2_READY     = 4
} pkg1_states_t;

typedef struct _secmon_mailbox_t
{
	//  < 4.0.0 Signals - 0: Not ready, 1: BCT ready, 2: DRAM and pkg2 ready, 3: Continue boot.
	// >= 4.0.0 Signals - 0: Not ready, 1: BCT ready, 2: DRAM ready, 4: pkg2 ready and continue boot.
	u32 in;
	// Non-zero: Secmon ready.
	u32 out;
} secmon_mailbox_t;

typedef struct _tsec_keys_t
{
	u8 tsec[SE_KEY_128_SIZE];
	u8 tsec_root[SE_KEY_128_SIZE];
	u8 tmp[SE_KEY_128_SIZE];
} tsec_keys_t;

typedef struct _kb_keys_t
{
	u8 master_kekseed[SE_KEY_128_SIZE];
	u8 random_data[0x70];
	u8 package1_key[SE_KEY_128_SIZE];
} kb_keys_t;

typedef struct _kb_t
{
	u8 cmac[SE_KEY_128_SIZE];
	u8 ctr[SE_AES_IV_SIZE];
	kb_keys_t keys;
	u8 padding[0x150];
} kb_t;

static const u8 keyblob_keyseeds[][SE_KEY_128_SIZE] = {
	{ 0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3 }, // 1.0.0.
	{ 0x0C, 0x25, 0x61, 0x5D, 0x68, 0x4C, 0xEB, 0x42, 0x1C, 0x23, 0x79, 0xEA, 0x82, 0x25, 0x12, 0xAC }, // 3.0.0.
	{ 0x33, 0x76, 0x85, 0xEE, 0x88, 0x4A, 0xAE, 0x0A, 0xC2, 0x8A, 0xFD, 0x7D, 0x63, 0xC0, 0x43, 0x3B }, // 3.0.1.
	{ 0x2D, 0x1F, 0x48, 0x80, 0xED, 0xEC, 0xED, 0x3E, 0x3C, 0xF2, 0x48, 0xB5, 0x65, 0x7D, 0xF7, 0xBE }, // 4.0.0.
	{ 0xBB, 0x5A, 0x01, 0xF9, 0x88, 0xAF, 0xF5, 0xFC, 0x6C, 0xFF, 0x07, 0x9E, 0x13, 0x3C, 0x39, 0x80 }, // 5.0.0.
	{ 0xD8, 0xCC, 0xE1, 0x26, 0x6A, 0x35, 0x3F, 0xCC, 0x20, 0xF3, 0x2D, 0x3B, 0x51, 0x7D, 0xE9, 0xC0 }  // 6.0.0.
};

static const u8 cmac_keyseed[SE_KEY_128_SIZE] =
	{ 0x59, 0xC7, 0xFB, 0x6F, 0xBE, 0x9B, 0xBE, 0x87, 0x65, 0x6B, 0x15, 0xC0, 0x53, 0x73, 0x36, 0xA5 };

static const u8 master_keyseed_retail[SE_KEY_128_SIZE] =
	{ 0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C };

static const u8 master_keyseed_4xx[SE_KEY_128_SIZE] =
	{ 0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66 };

static const u8 master_kekseed_620[SE_KEY_128_SIZE] =
	{ 0x37, 0x4B, 0x77, 0x29, 0x59, 0xB4, 0x04, 0x30, 0x81, 0xF6, 0xE5, 0x8C, 0x6D, 0x36, 0x17, 0x9A };

//!TODO: Update on tsec/mkey changes.
static const u8 master_kekseed_t210_tsec_v4[][SE_KEY_128_SIZE] = {
	{ 0xDE, 0xDC, 0xE3, 0x39, 0x30, 0x88, 0x16, 0xF8, 0xAE, 0x97, 0xAD, 0xEC, 0x64, 0x2D, 0x41, 0x41 }, // 8.1.0.
	{ 0x1A, 0xEC, 0x11, 0x82, 0x2B, 0x32, 0x38, 0x7A, 0x2B, 0xED, 0xBA, 0x01, 0x47, 0x7E, 0x3B, 0x67 }, // 9.0.0.
	{ 0x30, 0x3F, 0x02, 0x7E, 0xD8, 0x38, 0xEC, 0xD7, 0x93, 0x25, 0x34, 0xB5, 0x30, 0xEB, 0xCA, 0x7A }, // 9.1.0.
	{ 0x84, 0x67, 0xB6, 0x7F, 0x13, 0x11, 0xAE, 0xE6, 0x58, 0x9B, 0x19, 0xAF, 0x13, 0x6C, 0x80, 0x7A }, // 12.1.0.
	{ 0x68, 0x3B, 0xCA, 0x54, 0xB8, 0x6F, 0x92, 0x48, 0xC3, 0x05, 0x76, 0x87, 0x88, 0x70, 0x79, 0x23 }, // 13.0.0.
	{ 0xF0, 0x13, 0x37, 0x9A, 0xD5, 0x63, 0x51, 0xC3, 0xB4, 0x96, 0x35, 0xBC, 0x9C, 0xE8, 0x76, 0x81 }, // 14.0.0.
};

//!TODO: Update on mkey changes.
static const u8 master_kekseed_t210b01[][SE_KEY_128_SIZE] = {
	{ 0x77, 0x60, 0x5A, 0xD2, 0xEE, 0x6E, 0xF8, 0x3C, 0x3F, 0x72, 0xE2, 0x59, 0x9D, 0xAC, 0x5E, 0x56 }, // 6.0.0.
	{ 0x1E, 0x80, 0xB8, 0x17, 0x3E, 0xC0, 0x60, 0xAA, 0x11, 0xBE, 0x1A, 0x4A, 0xA6, 0x6F, 0xE4, 0xAE }, // 6.2.0.
	{ 0x94, 0x08, 0x67, 0xBD, 0x0A, 0x00, 0x38, 0x84, 0x11, 0xD3, 0x1A, 0xDB, 0xDD, 0x8D, 0xF1, 0x8A }, // 7.0.0.
	{ 0x5C, 0x24, 0xE3, 0xB8, 0xB4, 0xF7, 0x00, 0xC2, 0x3C, 0xFD, 0x0A, 0xCE, 0x13, 0xC3, 0xDC, 0x23 }, // 8.1.0.
	{ 0x86, 0x69, 0xF0, 0x09, 0x87, 0xC8, 0x05, 0xAE, 0xB5, 0x7B, 0x48, 0x74, 0xDE, 0x62, 0xA6, 0x13 }, // 9.0.0.
	{ 0x0E, 0x44, 0x0C, 0xED, 0xB4, 0x36, 0xC0, 0x3F, 0xAA, 0x1D, 0xAE, 0xBF, 0x62, 0xB1, 0x09, 0x82 }, // 9.1.0.
	{ 0xE5, 0x41, 0xAC, 0xEC, 0xD1, 0xA7, 0xD1, 0xAB, 0xED, 0x03, 0x77, 0xF1, 0x27, 0xCA, 0xF8, 0xF1 }, // 12.1.0.
	{ 0x52, 0x71, 0x9B, 0xDF, 0xA7, 0x8B, 0x61, 0xD8, 0xD5, 0x85, 0x11, 0xE4, 0x8E, 0x4F, 0x74, 0xC6 }, // 13.0.0.
	{ 0xD2, 0x68, 0xC6, 0x53, 0x9D, 0x94, 0xF9, 0xA8, 0xA5, 0xA8, 0xA7, 0xC8, 0x8F, 0x53, 0x4B, 0x7A }, // 14.0.0.
};

static const u8 console_keyseed[SE_KEY_128_SIZE] =
	{ 0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78 };

static const u8 console_keyseed_4xx[SE_KEY_128_SIZE] =
	{ 0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28 };

const u8 package2_keyseed[SE_KEY_128_SIZE] =
	{ 0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7 };

static void _hos_crit_error(const char *text)
{
	gfx_con.mute = false;
	gfx_printf("%k%s%k\n", 0xFFFF0000, text, 0xFFCCCCCC);
}

static void _se_lock(bool lock_se)
{
	if (lock_se)
	{
		// Disable aes key read.
		for (u32 i = 0; i < 16; i++)
			se_key_acc_ctrl(i, SE_KEY_TBL_DIS_KEYREAD_FLAG | SE_KEY_TBL_DIS_OIVREAD_FLAG | SE_KEY_TBL_DIS_UIVREAD_FLAG);

		// Disable RSA key read.
		for (u32 i = 0; i < 2; i++)
			se_rsa_acc_ctrl(i, SE_RSA_KEY_TBL_DIS_KEYREAD_FLAG);

		SE(SE_TZRAM_SECURITY_REG) = 0;                // Make SE TZRAM secure only.
		SE(SE_CRYPTO_SECURITY_PERKEY_REG) = 0;        // Make all AES keys access secure only.
		SE(SE_RSA_SECURITY_PERKEY_REG) = 0;           // Make all RSA keys access secure only.
		SE(SE_SE_SECURITY_REG) &= ~SE_PERKEY_SETTING; // Make access lock regs secure only.
	}

	memset((void *)IPATCH_BASE, 0, 14 * sizeof(u32));
	SB(SB_CSR) = SB_CSR_PIROM_DISABLE;

	// This is useful for documenting the bits in the SE config registers, so we can keep it around.
	/*gfx_printf("SE(SE_SE_SECURITY_REG) = %08X\n", SE(SE_SE_SECURITY_REG));
	gfx_printf("SE(0x4) = %08X\n", SE(0x4));
	gfx_printf("SE(SE_CRYPTO_SECURITY_PERKEY_REG) = %08X\n", SE(SE_CRYPTO_SECURITY_PERKEY_REG));
	gfx_printf("SE(SE_RSA_SECURITY_PERKEY_REG) = %08X\n", SE(SE_RSA_SECURITY_PERKEY_REG));
	for(u32 i = 0; i < 16; i++)
		gfx_printf("%02X ", SE(SE_CRYPTO_KEYTABLE_ACCESS_REG + i * 4) & 0xFF);
	gfx_putc('\n');
	for(u32 i = 0; i < 2; i++)
		gfx_printf("%02X ", SE(SE_RSA_KEYTABLE_ACCESS_REG + i * 4) & 0xFF);
	gfx_putc('\n');
	gfx_hexdump(SE_BASE, (void *)SE_BASE, 0x400);*/
}

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

static void _hos_eks_get()
{
	// Check if Erista based unit.
	if (h_cfg.t210b01)
		return;

	// Check if EKS already found and parsed.
	if (!h_cfg.eks)
	{
		// Read EKS blob.
		u8 *mbr = calloc(512 , 1);
		if (!hos_eks_rw_try(mbr, false))
			goto out;

		// Decrypt EKS blob.
		hos_eks_mbr_t *eks = (hos_eks_mbr_t *)(mbr + 0x80);
		se_aes_crypt_ecb(14, DECRYPT, eks, sizeof(hos_eks_mbr_t), eks, sizeof(hos_eks_mbr_t));

		// Check if valid and for this unit.
		if (eks->magic == HOS_EKS_MAGIC && eks->lot0 == FUSE(FUSE_OPT_LOT_CODE_0))
		{
			h_cfg.eks = eks;
			return;
		}

out:
		free(mbr);
	}
}

static void _hos_eks_save()
{
	// Check if Erista based unit.
	if (h_cfg.t210b01)
		return;

	// EKS save. Only for 7.0.0 and up.
	bool new_eks = false;
	if (!h_cfg.eks)
	{
		h_cfg.eks = calloc(512 , 1);
		new_eks = true;
	}

	// If matching blob doesn't exist, create it.
	if (h_cfg.eks->enabled != HOS_EKS_TSEC_VER)
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
		u8 *keys = (u8 *)calloc(SZ_4K, 2);
		se_get_aes_keys(keys + SZ_4K, keys, SE_KEY_128_SIZE);

		// Set magic and personalized info.
		h_cfg.eks->magic = HOS_EKS_MAGIC;
		h_cfg.eks->enabled = HOS_EKS_TSEC_VER;
		h_cfg.eks->lot0 = FUSE(FUSE_OPT_LOT_CODE_0);

		// Copy new keys.
		memcpy(h_cfg.eks->tsec,      keys + 12 * SE_KEY_128_SIZE, SE_KEY_128_SIZE);
		memcpy(h_cfg.eks->troot,     keys + 13 * SE_KEY_128_SIZE, SE_KEY_128_SIZE);
		memcpy(h_cfg.eks->troot_dev, keys + 11 * SE_KEY_128_SIZE, SE_KEY_128_SIZE);

		// Encrypt EKS blob.
		u8 *eks = calloc(512 , 1);
		memcpy(eks, h_cfg.eks, sizeof(hos_eks_mbr_t));
		se_aes_crypt_ecb(14, ENCRYPT, eks, sizeof(hos_eks_mbr_t), eks, sizeof(hos_eks_mbr_t));

		// Write EKS blob to SD.
		memcpy(mbr + 0x80, eks, sizeof(hos_eks_mbr_t));
		hos_eks_rw_try(mbr, true);

		free(eks);
		free(keys);
out:
		free(mbr);
	}
}

void hos_eks_clear(u32 kb)
{
	// Check if Erista based unit.
	if (h_cfg.t210b01)
		return;

	if (h_cfg.eks && kb >= KB_FIRMWARE_VERSION_700)
	{
		// Check if current Master key is enabled.
		if (h_cfg.eks->enabled)
		{
			// Read EKS blob.
			u8 *mbr = calloc(512 , 1);
			if (!hos_eks_rw_try(mbr, false))
				goto out;

			// Disable current Master key version.
			h_cfg.eks->enabled = 0;

			// Encrypt EKS blob.
			u8 *eks = calloc(512 , 1);
			memcpy(eks, h_cfg.eks, sizeof(hos_eks_mbr_t));
			se_aes_crypt_ecb(14, ENCRYPT, eks, sizeof(hos_eks_mbr_t), eks, sizeof(hos_eks_mbr_t));

			// Write EKS blob to SD.
			memcpy(mbr + 0x80, eks, sizeof(hos_eks_mbr_t));
			hos_eks_rw_try(mbr, true);

			free(eks);
out:
			free(mbr);
		}
	}
}

int hos_keygen_t210b01(u32 kb)
{
	// Use SBK as Device key 4x unsealer and KEK for mkey in T210B01 units.
	se_aes_unwrap_key(10, 14, console_keyseed_4xx);

	// Derive master key.
	se_aes_unwrap_key(7, 12, &master_kekseed_t210b01[kb - KB_FIRMWARE_VERSION_600]);
	se_aes_unwrap_key(7, 7, master_keyseed_retail);

	// Derive latest pkg2 key.
	se_aes_unwrap_key(8, 7, package2_keyseed);

	return 1;
}

int hos_keygen(void *keyblob, u32 kb, tsec_ctxt_t *tsec_ctxt, bool stock, bool is_exo)
{
	static bool sbk_wiped = false;

	u32 retries = 0;
	bool use_tsec = false;
	tsec_keys_t tsec_keys;
	kb_t *kb_data = (kb_t *)keyblob;

	if (kb > KB_FIRMWARE_VERSION_MAX)
		return 0;

	if (h_cfg.t210b01)
		return hos_keygen_t210b01(kb);

	// Do Erista keygen.

	// SBK is wiped. Try to restore it from fuses.
	if (sbk_wiped)
	{
		if (fuse_set_sbk())
			sbk_wiped = false;
		else
			return 1; // Continue with current SE keys.
	}

	// Use HOS EKS if it exists.
	_hos_eks_get();

	// Use tsec keygen for old firmware or if EKS keys does not exist for newer.
	if (kb <= KB_FIRMWARE_VERSION_620 || !h_cfg.eks || (h_cfg.eks && h_cfg.eks->enabled != HOS_EKS_TSEC_VER))
		use_tsec = true;

	if (kb <= KB_FIRMWARE_VERSION_600)
	{
		tsec_ctxt->size = 0xF00;
		tsec_ctxt->type = TSEC_FW_TYPE_OLD;
	}
	else if (kb == KB_FIRMWARE_VERSION_620)
	{
		tsec_ctxt->size = 0x2900;
		tsec_ctxt->type = TSEC_FW_TYPE_EMU;

		// Prepare smmu tsec page for 6.2.0.
		u8 *tsec_paged = (u8 *)page_alloc(3);
		memcpy(tsec_paged, (void *)tsec_ctxt->fw, tsec_ctxt->size);
		tsec_ctxt->fw = tsec_paged;
	}
	else if (use_tsec) // 7.0.0+
	{
		/*
		 * 7.0.0/8.1.0 tsec fw are 0x3000/0x3300.
		 * Unused here because of THK.
		 */

		// Use custom TSEC Hovi Keygen firmware.
		tsec_ctxt->fw = sd_file_read("bootloader/sys/thk.bin", NULL);
		if (!tsec_ctxt->fw)
		{
			_hos_crit_error("\nFailed to load thk.bin");
			return 0;
		}

		tsec_ctxt->size = 0x1F00;
		tsec_ctxt->type = TSEC_FW_TYPE_NEW;
	}
	else if (h_cfg.eks)
	{
		// EKS found. Set TSEC keys.
		se_aes_key_set(12, h_cfg.eks->tsec, SE_KEY_128_SIZE);
		se_aes_key_set(13, h_cfg.eks->troot, SE_KEY_128_SIZE);
		se_aes_key_set(11, h_cfg.eks->troot_dev, SE_KEY_128_SIZE);
	}

	// Get TSEC key.
	while (use_tsec && tsec_query(&tsec_keys, tsec_ctxt) < 0)
	{
		memset(&tsec_keys, 0x00, 0x20);
		retries++;

		// We rely on racing conditions, make sure we cover even the unluckiest cases.
		if (retries > 15)
		{
			_hos_crit_error("\nFailed to get TSEC keys. Please try again.");
			return 0;
		}
	}

	if (kb >= KB_FIRMWARE_VERSION_700)
	{
		// For 7.0.0 and up, save EKS slot if it doesn't exist.
		if (use_tsec)
		{
			_hos_eks_save();
			free(tsec_ctxt->fw);
		}

		// Use 8.1.0 for 7.0.0 otherwise the proper one.
		u32 mkey_idx = 0;
		if (kb >= KB_FIRMWARE_VERSION_810)
			mkey_idx = kb - KB_FIRMWARE_VERSION_810;

		if (!is_exo)
		{
			// Derive Package2 key in secmon compatible way.
			se_aes_unwrap_key(7, 13, master_kekseed_t210_tsec_v4[mkey_idx]);
			se_aes_unwrap_key(7, 7, master_keyseed_retail);
			se_aes_unwrap_key(8, 7, package2_keyseed);
		}
		else
		{
			se_aes_crypt_block_ecb(12, DECRYPT, tsec_keys.tmp, keyblob_keyseeds[0]);
			se_aes_unwrap_key(15, 14, tsec_keys.tmp);

			// Derive device keys.
			se_aes_unwrap_key(10, 15, console_keyseed_4xx);
			se_aes_unwrap_key(15, 15, console_keyseed);

			// Derive master kek.
			se_aes_unwrap_key(13, 13, master_kekseed_t210_tsec_v4[mkey_idx]);

			// Derive device master key and master key.
			se_aes_unwrap_key(12, 13, master_keyseed_4xx);
			se_aes_unwrap_key(13, 13, master_keyseed_retail);

			// Package2 key.
			se_aes_unwrap_key(8, 13, package2_keyseed);
		}
	}
	else if (kb == KB_FIRMWARE_VERSION_620)
	{
		// Set TSEC key.
		se_aes_key_set(12, tsec_keys.tsec, SE_KEY_128_SIZE);
		// Set TSEC root key.
		se_aes_key_set(13, tsec_keys.tsec_root, SE_KEY_128_SIZE);

		if (!is_exo)
		{
			// Derive Package2 key in secmon compatible way.
			se_aes_key_set(8, tsec_keys.tsec_root, SE_KEY_128_SIZE);
			se_aes_unwrap_key(8, 8, master_kekseed_620);
			se_aes_unwrap_key(8, 8, master_keyseed_retail);
			se_aes_unwrap_key(8, 8, package2_keyseed);
		}
		else
		{
			// Decrypt keyblob and set keyslots for Exosphere 2.
			se_aes_crypt_block_ecb(12, DECRYPT, tsec_keys.tmp, keyblob_keyseeds[0]);
			se_aes_unwrap_key(15, 14, tsec_keys.tmp);

			// Derive device keys.
			se_aes_unwrap_key(10, 15, console_keyseed_4xx);
			se_aes_unwrap_key(15, 15, console_keyseed);

			// Derive master kek.
			se_aes_unwrap_key(13, 13, master_kekseed_620);

			// Derive device master key and master key.
			se_aes_unwrap_key(12, 13, master_keyseed_4xx);
			se_aes_unwrap_key(13, 13, master_keyseed_retail);

			// Package2 key.
			se_aes_unwrap_key(8, 13, package2_keyseed);
		}
	}
	else
	{
		se_key_acc_ctrl(13, SE_KEY_TBL_DIS_KEYREAD_FLAG | SE_KEY_TBL_DIS_OIVREAD_FLAG | SE_KEY_TBL_DIS_UIVREAD_FLAG);
		se_key_acc_ctrl(14, SE_KEY_TBL_DIS_KEYREAD_FLAG | SE_KEY_TBL_DIS_OIVREAD_FLAG | SE_KEY_TBL_DIS_UIVREAD_FLAG);

		// Set TSEC key.
		se_aes_key_set(13, tsec_keys.tsec, SE_KEY_128_SIZE);

		// Derive keyblob keys from TSEC+SBK.
		se_aes_crypt_block_ecb(13, DECRYPT, tsec_keys.tsec, keyblob_keyseeds[0]);
		se_aes_unwrap_key(15, 14, tsec_keys.tsec);
		se_aes_crypt_block_ecb(13, DECRYPT, tsec_keys.tsec, keyblob_keyseeds[kb]);
		se_aes_unwrap_key(13, 14, tsec_keys.tsec);

		// Clear SBK.
		//se_aes_key_clear(14);

/*
		// Verify keyblob CMAC.
		u8 cmac[SE_KEY_128_SIZE];
		se_aes_unwrap_key(11, 13, cmac_keyseed);
		se_aes_cmac(cmac, SE_KEY_128_SIZE, 11, (void *)kb_data->ctr, sizeof(kb_data->ctr) + sizeof(kb_data->keys));
		if (!memcmp(kb_data->cmac, cmac, SE_KEY_128_SIZE))
			return 0;
*/

		se_aes_crypt_block_ecb(13, DECRYPT, tsec_keys.tsec, cmac_keyseed);
		se_aes_unwrap_key(11, 13, cmac_keyseed);

		// Decrypt keyblob and set keyslots.
		se_aes_crypt_ctr(13, &kb_data->keys, sizeof(kb_keys_t), &kb_data->keys, sizeof(kb_keys_t), kb_data->ctr);
		se_aes_key_set(11, kb_data->keys.package1_key, SE_KEY_128_SIZE);
		se_aes_key_set(12, kb_data->keys.master_kekseed, SE_KEY_128_SIZE);
		se_aes_key_set(13, kb_data->keys.master_kekseed, SE_KEY_128_SIZE);

		se_aes_crypt_block_ecb(12, DECRYPT, tsec_keys.tsec, master_keyseed_retail);

		if (!is_exo)
		{
			switch (kb)
			{
			case KB_FIRMWARE_VERSION_100:
			case KB_FIRMWARE_VERSION_300:
			case KB_FIRMWARE_VERSION_301:
				se_aes_unwrap_key(13, 15, console_keyseed);
				se_aes_unwrap_key(12, 12, master_keyseed_retail);
				break;
			case KB_FIRMWARE_VERSION_400:
				se_aes_unwrap_key(13, 15, console_keyseed_4xx);
				se_aes_unwrap_key(15, 15, console_keyseed);
				se_aes_unwrap_key(14, 12, master_keyseed_4xx);
				se_aes_unwrap_key(12, 12, master_keyseed_retail);
				sbk_wiped = true;
				break;
			case KB_FIRMWARE_VERSION_500:
			case KB_FIRMWARE_VERSION_600:
				se_aes_unwrap_key(10, 15, console_keyseed_4xx);
				se_aes_unwrap_key(15, 15, console_keyseed);
				se_aes_unwrap_key(14, 12, master_keyseed_4xx);
				se_aes_unwrap_key(12, 12, master_keyseed_retail);
				sbk_wiped = true;
				break;
			}
		}
		else // Exosphere 2.
		{
			se_aes_unwrap_key(10, 15, console_keyseed_4xx);
			se_aes_unwrap_key(15, 15, console_keyseed);
			se_aes_unwrap_key(13, 12, master_keyseed_retail);
			se_aes_unwrap_key(12, 12, master_keyseed_4xx);
		}

		// Package2 key.
		se_key_acc_ctrl(8, SE_KEY_TBL_DIS_KEYREAD_FLAG | SE_KEY_TBL_DIS_OIVREAD_FLAG | SE_KEY_TBL_DIS_UIVREAD_FLAG);
		se_aes_unwrap_key(8, !is_exo ? 12 : 13, package2_keyseed);
	}

	return 1;
}

static int _read_emmc_pkg1(launch_ctxt_t *ctxt)
{
	const u32 pk1_offset = h_cfg.t210b01 ? sizeof(bl_hdr_t210b01_t) : 0; // Skip T210B01 OEM header.
	u32 bootloader_offset = PKG1_BOOTLOADER_MAIN_OFFSET;
	ctxt->pkg1 = (void *)malloc(PKG1_BOOTLOADER_SIZE);

try_load:
	// Read package1.
	emummc_storage_set_mmc_partition(EMMC_BOOT0);
	emummc_storage_read(bootloader_offset / EMMC_BLOCKSIZE, PKG1_BOOTLOADER_SIZE / EMMC_BLOCKSIZE, ctxt->pkg1);

	ctxt->pkg1_id = pkg1_identify(ctxt->pkg1 + pk1_offset);
	if (!ctxt->pkg1_id)
	{
		// Check if wrong pkg1 was flashed.
		bool wrong_pkg1;

		const u32 pkg1_erista_check = ((bl_hdr_t210b01_t *)ctxt->pkg1)->entrypoint;
		const u32 pkg1_mariko_check = *(u32 *)(ctxt->pkg1 + sizeof(pk1_hdr_t) * 2);

		if (!h_cfg.t210b01) // For Erista check if start is 0 and entrypoint matches Mariko.
			wrong_pkg1 = *(u32 *)ctxt->pkg1 == 0 && pkg1_erista_check == PKG1_MARIKO_ON_ERISTA_MAGIC;
		else                // For Mariko check if start is not 0 and build id. It works for 8.0.0 Erista pkg1 and up.
			wrong_pkg1 = *(u32 *)ctxt->pkg1 != 0 && pkg1_mariko_check == PKG1_ERISTA_ON_MARIKO_MAGIC;

		if (wrong_pkg1)
		{
			_hos_crit_error("Wrong pkg1 flashed:");
			EPRINTFARGS("%s pkg1 on %s!",
				!h_cfg.t210b01 ? "Mariko" : "Erista", !h_cfg.t210b01 ? "Erista" : "Mariko");
		}
		else
		{
			_hos_crit_error("Unknown pkg1 version.");
			EPRINTFARGS("HOS version not supported!%s",
				(emu_cfg.enabled && !h_cfg.emummc_force_disable) ? "\nOr emuMMC corrupt!" : "");
		}

		// Try backup bootloader.
		if (bootloader_offset != PKG1_BOOTLOADER_BACKUP_OFFSET)
		{
			EPRINTF("\nTrying backup bootloader...");
			bootloader_offset = PKG1_BOOTLOADER_BACKUP_OFFSET;
			goto try_load;
		}

		return 0;
	}
	gfx_printf("Identified pkg1 and mkey %d\n\n", ctxt->pkg1_id->kb);

	// Read the correct keyblob for older HOS versions.
	if (ctxt->pkg1_id->kb <= KB_FIRMWARE_VERSION_600)
	{
		ctxt->keyblob = (u8 *)calloc(EMMC_BLOCKSIZE, 1);
		emummc_storage_read(PKG1_HOS_KEYBLOBS_OFFSET / EMMC_BLOCKSIZE + ctxt->pkg1_id->kb, 1, ctxt->keyblob);
	}

	return 1;
}

static u8 *_read_emmc_pkg2(launch_ctxt_t *ctxt)
{
	u8 *bctBuf = NULL;

	emummc_storage_set_mmc_partition(EMMC_GPP);

	// Parse eMMC GPT.
	LIST_INIT(gpt);
	emmc_gpt_parse(&gpt);
DPRINTF("Parsed GPT\n");
	// Find package2 partition.
	emmc_part_t *pkg2_part = emmc_part_find(&gpt, "BCPKG2-1-Normal-Main");
	if (!pkg2_part)
		goto out;

	// Read in package2 header and get package2 real size.
	const u32 BCT_SIZE = SZ_16K;
	bctBuf = (u8 *)malloc(BCT_SIZE);
	emmc_part_read(pkg2_part, BCT_SIZE / EMMC_BLOCKSIZE, 1, bctBuf);
	u32 *hdr = (u32 *)(bctBuf + 0x100);
	u32 pkg2_size = hdr[0] ^ hdr[2] ^ hdr[3];
DPRINTF("pkg2 size on emmc is %08X\n", pkg2_size);

	// Read in Boot Config.
	memset(bctBuf, 0, BCT_SIZE);
	emmc_part_read(pkg2_part, 0, BCT_SIZE / EMMC_BLOCKSIZE, bctBuf);

	// Read in package2.
	u32 pkg2_size_aligned = ALIGN(pkg2_size, EMMC_BLOCKSIZE);
DPRINTF("pkg2 size aligned is %08X\n", pkg2_size_aligned);
	ctxt->pkg2 = malloc(pkg2_size_aligned);
	ctxt->pkg2_size = pkg2_size;
	emmc_part_read(pkg2_part, BCT_SIZE / EMMC_BLOCKSIZE,
		pkg2_size_aligned / EMMC_BLOCKSIZE, ctxt->pkg2);
out:
	emmc_gpt_free(&gpt);

	return bctBuf;
}

static void _free_launch_components(launch_ctxt_t *ctxt)
{
	free(ctxt->keyblob);
	free(ctxt->pkg1);
	free(ctxt->pkg2);
	free(ctxt->warmboot);
	free(ctxt->secmon);
	free(ctxt->kernel);
	free(ctxt->kip1_patches);
}

static bool _get_fs_exfat_compatible(link_t *info, u32 *hos_revision)
{
	u32 fs_ids_cnt;
	u32 sha_buf[32 / sizeof(u32)];
	kip1_id_t *kip_ids;

	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, info, link)
	{
		if (strncmp((const char*)ki->kip1->name, "FS", sizeof(ki->kip1->name)))
			continue;

		if (!se_calc_sha256_oneshot(sha_buf, ki->kip1, ki->size))
			break;

		pkg2_get_ids(&kip_ids, &fs_ids_cnt);

		for (int fs_idx = fs_ids_cnt - 1; fs_idx >= 0; fs_idx--)
		{
			if (!memcmp(sha_buf, kip_ids[fs_idx].hash, 8))
			{
				// HOS Api special handling.
				if ((fs_idx & ~1) == 16)      // Check if it's 5.1.0.
					*hos_revision = 1;
				else if ((fs_idx & ~1) == 34) // Check if it's 10.2.0.
					*hos_revision = 2;

				// Check if FAT32-only.
				if (!(fs_idx & 1))
					return false;

				// FS is FAT32 + exFAT.
				break;
			}
		}

		break;
	}

	// FAT32 + exFAT or unknown FS version.
	return true;
}

int hos_launch(ini_sec_t *cfg)
{
	u8 kb;
	u32 secmon_base;
	u32 warmboot_base;
	bool is_exo = false;
	launch_ctxt_t ctxt = {0};
	tsec_ctxt_t tsec_ctxt = {0};
	volatile secmon_mailbox_t *secmon_mailbox;

	minerva_change_freq(FREQ_1600);
	list_init(&ctxt.kip1_list);

	ctxt.cfg = cfg;

	if (!gfx_con.mute)
		gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	gfx_puts("Initializing...\n\n");

	// Initialize eMMC/emuMMC.
	int res = emummc_storage_init_mmc();
	if (res)
	{
		if (res == 2)
			_hos_crit_error("Failed to init eMMC.");
		else
			_hos_crit_error("Failed to init emuMMC.");

		goto error;
	}

	// Check if SD Card is GPT.
	if (sd_is_gpt())
	{
		_hos_crit_error("SD has GPT only!");
		goto error;
	}

	// Read package1 and the correct keyblob.
	if (!_read_emmc_pkg1(&ctxt))
		goto error;

	kb = ctxt.pkg1_id->kb;

	// Try to parse config if present.
	if (ctxt.cfg && !parse_boot_config(&ctxt))
	{
		_hos_crit_error("Wrong ini cfg or missing/corrupt files!");
		goto error;
	}

	bool emummc_enabled = emu_cfg.enabled && !h_cfg.emummc_force_disable;

	// Enable emummc patching.
	if (emummc_enabled)
	{
		if (ctxt.stock)
		{
			_hos_crit_error("Stock emuMMC is not supported yet!");
			goto error;
		}

		ctxt.atmosphere = true; // Set atmosphere patching in case of no fss0.
		config_kip1patch(&ctxt, "emummc");
	}
	else if (!emu_cfg.enabled && ctxt.emummc_forced)
	{
		_hos_crit_error("emuMMC is forced but not enabled!");
		goto error;
	}

	// If Auto NOGC is enabled, check if burnt fuses lower than installed HOS fuses and apply NOGC patch.
	// For emuMMC, unconditionally enable NOGC when burnt fuses are higher than installed HOS fuses.
	// Disable Auto NOGC in stock to prevent black screen (fatal error). Use kip1patch=nogc to force it.
	if (!ctxt.stock)
	{
		u32 fuses = fuse_read_odm(7);
		if ((h_cfg.autonogc &&
			  (
				(!(fuses &    ~0xF) && (ctxt.pkg1_id->fuses >=  5)) || // LAFW v2,  4.0.0+
				(!(fuses &  ~0x3FF) && (ctxt.pkg1_id->fuses >= 11)) || // LAFW v3,  9.0.0+
				(!(fuses & ~0x1FFF) && (ctxt.pkg1_id->fuses >= 14)) || // LAFW v4, 11.0.0+
				// Detection broken! Use kip1patch=nogc                // LAFW v5, 12.0.0+
				(!(fuses & ~0x3FFF) && (ctxt.pkg1_id->fuses >= 15))    // LAFW v5, 12.0.2+
			  )
			)
		|| ((emummc_enabled) &&
			  (
				((fuses & 0x400)  && (ctxt.pkg1_id->fuses <= 10)) || // HOS  9.0.0+ fuses burnt.
				((fuses & 0x2000) && (ctxt.pkg1_id->fuses <= 13)) || // HOS 11.0.0+ fuses burnt.
				// Detection broken! Use kip1patch=nogc              // HOS 12.0.0+
				((fuses & 0x4000) && (ctxt.pkg1_id->fuses <= 14))    // HOS 12.0.2+ fuses burnt.
			  )
			))
			config_kip1patch(&ctxt, "nogc");
	}

	gfx_printf("Loaded config and pkg1\n%s mode\n", ctxt.stock ? "Stock" : "CFW");

	// Check if secmon is exosphere.
	if (ctxt.secmon)
		is_exo = !memcmp((void *)((u8 *)ctxt.secmon + ctxt.secmon_size - 4), "LENY", 4);
	const pkg1_id_t *pk1_latest = pkg1_get_latest();
	secmon_base = is_exo ? pk1_latest->secmon_base : ctxt.pkg1_id->secmon_base;
	warmboot_base = is_exo ? pk1_latest->warmboot_base : ctxt.pkg1_id->warmboot_base;

	// Generate keys.
	tsec_ctxt.fw = (u8 *)ctxt.pkg1 + ctxt.pkg1_id->tsec_off;
	tsec_ctxt.pkg1 = ctxt.pkg1;
	tsec_ctxt.pkg11_off = ctxt.pkg1_id->pkg11_off;
	tsec_ctxt.secmon_base = secmon_base;

	if (!hos_keygen(ctxt.keyblob, kb, &tsec_ctxt, ctxt.stock, is_exo))
		goto error;
	gfx_puts("Generated keys\n");

	// Decrypt and unpack package1 if we require parts of it.
	if (!ctxt.warmboot || !ctxt.secmon)
	{
		// Decrypt PK1 or PK11.
		if (kb <= KB_FIRMWARE_VERSION_600 || h_cfg.t210b01)
		{
			if (!pkg1_decrypt(ctxt.pkg1_id, ctxt.pkg1))
			{
				_hos_crit_error("Pkg1 decryption failed!");

				// Check if T210B01 BEK is missing or wrong.
				if (h_cfg.t210b01)
				{
					u32 bek_vector[4] = {0};
					se_aes_crypt_ecb(13, ENCRYPT, bek_vector, SE_KEY_128_SIZE, bek_vector, SE_KEY_128_SIZE);
					if (bek_vector[0] == 0x59C14895) // Encrypted zeroes first 32bits.
						EPRINTF("Pkg1 corrupt?");
					else
						EPRINTF("BEK is missing!");
				}
				goto error;
			}
		}

		// Unpack PK11.
		if (h_cfg.t210b01 || (kb <= KB_FIRMWARE_VERSION_620 && !emummc_enabled))
		{
			// Skip T210B01 OEM header.
			u32 pk1_offset = 0;
			if (h_cfg.t210b01)
				pk1_offset = sizeof(bl_hdr_t210b01_t);

			pkg1_unpack((void *)warmboot_base, &ctxt.warmboot_size,
				!is_exo ? (void *)ctxt.pkg1_id->secmon_base : NULL, NULL,
				ctxt.pkg1_id, ctxt.pkg1 + pk1_offset);

			gfx_puts("Decrypted & unpacked pkg1\n");
		}
		else
		{
			_hos_crit_error("No mandatory secmon or warmboot provided!");
			goto error;
		}
	}

	// Configure and manage Warmboot binary.
	if (!pkg1_warmboot_config(&ctxt, warmboot_base))
	{
		// Can only happen on T210B01.
		_hos_crit_error("Failed to match warmboot with fuses!\nIf you continue, sleep wont work!");

		gfx_puts("\nPress POWER to continue.\nPress VOL to go to the menu.\n");
		display_backlight_brightness(h_cfg.backlight, 1000);

		if (!(btn_wait() & BTN_POWER))
			goto error;
	}

	// Replace 'warmboot.bin' if requested.
	if (ctxt.warmboot)
		memcpy((void *)warmboot_base, ctxt.warmboot, ctxt.warmboot_size);
	else if (!h_cfg.t210b01)
	{
		// Patch warmboot on T210 to allow downgrading.
		if (kb >= KB_FIRMWARE_VERSION_700)
		{
			_hos_crit_error("No warmboot provided!");
			goto error;
		}

		pkg1_warmboot_patch((void *)&ctxt);
	}

	// Replace 'SecureMonitor' if requested or patch Pkg2 checks if needed.
	if (ctxt.secmon)
		memcpy((void *)secmon_base, ctxt.secmon, ctxt.secmon_size);
	else
		pkg1_secmon_patch((void *)&ctxt, secmon_base, h_cfg.t210b01);

	gfx_puts("Loaded warmboot and secmon\n");

	// Read package2.
	u8 *bootConfigBuf = _read_emmc_pkg2(&ctxt);
	if (!bootConfigBuf)
	{
		_hos_crit_error("Pkg2 read failed!");
		goto error;
	}

	gfx_puts("Read pkg2\n");

	// Decrypt package2 and parse KIP1 blobs in INI1 section.
	pkg2_hdr_t *pkg2_hdr = pkg2_decrypt(ctxt.pkg2, kb, is_exo);
	if (!pkg2_hdr)
	{
		_hos_crit_error("Pkg2 decryption failed!\npkg1/pkg2 mismatch or old hekate!");

		// Clear EKS slot, in case something went wrong with tsec keygen.
		hos_eks_clear(kb);
		goto error;
	}

	LIST_INIT(kip1_info);
	if (!pkg2_parse_kips(&kip1_info, pkg2_hdr, &ctxt.new_pkg2))
	{
		_hos_crit_error("INI1 parsing failed!");
		goto error;
	}

	gfx_puts("Parsed ini1\n");

	// Use the kernel included in package2 in case we didn't load one already.
	if (!ctxt.kernel)
	{
		ctxt.kernel = pkg2_hdr->data;
		ctxt.kernel_size = pkg2_hdr->sec_size[PKG2_SEC_KERNEL];

		if (!ctxt.stock && (ctxt.svcperm || ctxt.debugmode || ctxt.atmosphere))
		{
			// Hash only Kernel when it embeds INI1.
			u8 kernel_hash[0x20];
			if (!ctxt.new_pkg2)
				se_calc_sha256_oneshot(kernel_hash, ctxt.kernel, ctxt.kernel_size);
			else
				se_calc_sha256_oneshot(kernel_hash, ctxt.kernel + PKG2_NEWKERN_START,
					pkg2_newkern_ini1_start - PKG2_NEWKERN_START);

			ctxt.pkg2_kernel_id = pkg2_identify(kernel_hash);
			if (!ctxt.pkg2_kernel_id)
			{
				_hos_crit_error("Failed to identify kernel!");

				goto error;
			}

			// In case a kernel patch option is set; allows to disable SVC verification or/and enable debug mode.
			kernel_patch_t *kernel_patchset = ctxt.pkg2_kernel_id->kernel_patchset;
			if (kernel_patchset != NULL)
			{
				gfx_printf("%kPatching kernel%k\n", 0xFFFFBA00, 0xFFCCCCCC);
				u32 *temp;
				for (u32 i = 0; kernel_patchset[i].id != 0xFFFFFFFF; i++)
				{
					if ((ctxt.svcperm && kernel_patchset[i].id == SVC_VERIFY_DS)
					|| (ctxt.debugmode && kernel_patchset[i].id == DEBUG_MODE_EN && !(ctxt.atmosphere && ctxt.secmon))
					|| (ctxt.atmosphere && kernel_patchset[i].id == ATM_GEN_PATCH))
						*(vu32 *)(ctxt.kernel + kernel_patchset[i].off) = kernel_patchset[i].val;
					else if (ctxt.atmosphere && kernel_patchset[i].id == ATM_ARR_PATCH)
					{
						temp = (u32 *)kernel_patchset[i].ptr;
						for (u32 j = 0; j < kernel_patchset[i].val; j++)
							*(vu32 *)(ctxt.kernel + kernel_patchset[i].off + (j << 2)) = temp[j];
					}
					else if (kernel_patchset[i].id < SVC_VERIFY_DS)
						*(vu32 *)(ctxt.kernel + kernel_patchset[i].off) = kernel_patchset[i].val;
				}
			}
		}
	}

	// Merge extra KIP1s into loaded ones.
	LIST_FOREACH_ENTRY(merge_kip_t, mki, &ctxt.kip1_list, link)
		pkg2_merge_kip(&kip1_info, (pkg2_kip1_t *)mki->kip1);

	// Check if FS is compatible with exFAT and if 5.1.0.
	if (!ctxt.stock && (sd_fs.fs_type == FS_EXFAT || kb == KB_FIRMWARE_VERSION_500))
	{
		bool exfat_compat = _get_fs_exfat_compatible(&kip1_info, &ctxt.exo_ctx.hos_revision);

		if (sd_fs.fs_type == FS_EXFAT && !exfat_compat)
		{
			_hos_crit_error("SD Card is exFAT but installed HOS driver\nonly supports FAT32!");

			_free_launch_components(&ctxt);
			goto error;
		}
	}

	// Patch kip1s in memory if needed.
	if (ctxt.kip1_patches)
		gfx_printf("%kPatching kips%k\n", 0xFFFFBA00, 0xFFCCCCCC);
	const char* unappliedPatch = pkg2_patch_kips(&kip1_info, ctxt.kip1_patches);
	if (unappliedPatch != NULL)
	{
		EHPRINTFARGS("Failed to apply '%s'!", unappliedPatch);

		bool emmc_patch_failed = !strcmp(unappliedPatch, "emummc");
		if (!emmc_patch_failed)
		{
			gfx_puts("\nPress POWER to continue.\nPress VOL to go to the menu.\n");
			display_backlight_brightness(h_cfg.backlight, 1000);
		}

		if (emmc_patch_failed || !(btn_wait() & BTN_POWER))
		{
			_free_launch_components(&ctxt);
			goto error; // MUST stop here, because if user requests 'nogc' but it's not applied, their GC controller gets updated!
		}
	}

	// Rebuild and encrypt package2.
	pkg2_build_encrypt((void *)PKG2_LOAD_ADDR, &ctxt, &kip1_info, is_exo);

	// Configure Exosphere if secmon is replaced.
	if (is_exo)
		config_exosphere(&ctxt, warmboot_base);

	// Unmount SD card and eMMC.
	sd_end();
	sdmmc_storage_end(&emmc_storage);

	gfx_printf("Rebuilt & loaded pkg2\n\n%kBooting...%k\n", 0xFF96FF00, 0xFFCCCCCC);

	// Clear pkg1/pkg2 keys.
	se_aes_key_clear(8);
	se_aes_key_clear(11);

	// Clear derived master key in case of Erista and 7.0.0+
	se_aes_key_clear(9);

	// Set secmon mailbox pkg2 ready state.
	u32 pkg1_state_pkg2_ready = PKG1_STATE_PKG2_READY;

	// Finalize per firmware key access. Skip access control if Exosphere 2.
	switch (kb | (is_exo << 7))
	{
	case KB_FIRMWARE_VERSION_100:
	case KB_FIRMWARE_VERSION_300:
	case KB_FIRMWARE_VERSION_301:
		se_key_acc_ctrl(12, SE_KEY_TBL_DIS_KEY_ACCESS_FLAG | SE_KEY_LOCK_FLAG);
		se_key_acc_ctrl(13, SE_KEY_TBL_DIS_KEY_ACCESS_FLAG | SE_KEY_LOCK_FLAG);
		pkg1_state_pkg2_ready = PKG1_STATE_PKG2_READY_OLD;
		break;
	case KB_FIRMWARE_VERSION_400:
	case KB_FIRMWARE_VERSION_500:
	case KB_FIRMWARE_VERSION_600:
		se_key_acc_ctrl(12, SE_KEY_TBL_DIS_KEY_ACCESS_FLAG | SE_KEY_LOCK_FLAG);
		se_key_acc_ctrl(15, SE_KEY_TBL_DIS_KEY_ACCESS_FLAG | SE_KEY_LOCK_FLAG);
		break;
	}

	// Clear BCT area for retail units and copy it over if dev unit.
	if (kb <= KB_FIRMWARE_VERSION_500 && !is_exo)
	{
		memset((void *)SECMON_BCT_CFG_ADDR, 0, SZ_4K + SZ_8K);
		if (fuse_read_hw_state() == FUSE_NX_HW_STATE_DEV)
			memcpy((void *)SECMON_BCT_CFG_ADDR, bootConfigBuf, SZ_4K);
	}
	else
	{
		memset((void *)SECMON6_BCT_CFG_ADDR, 0, SZ_2K);
		if (fuse_read_hw_state() == FUSE_NX_HW_STATE_DEV)
			memcpy((void *)SECMON6_BCT_CFG_ADDR, bootConfigBuf, SZ_2K);
	}

	// Finalize MC carveout.
	if (kb <= KB_FIRMWARE_VERSION_301 && !is_exo)
		mc_config_carveout();

	// Lock SE before starting 'SecureMonitor' if < 6.2.0, otherwise lock bootrom and ipatches.
	_se_lock(kb <= KB_FIRMWARE_VERSION_600 && !is_exo);

	// Reset sysctr0 counters.
	if (kb >= KB_FIRMWARE_VERSION_620)
	{
		for (u32 i = 0; i < SYSCTR0_COUNTERS; i += sizeof(u32))
			SYSCTR0(SYSCTR0_COUNTERS_BASE + i) = 0;
	}

	// NX Bootloader locks LP0 Carveout secure scratch registers.
	//pmc_scratch_lock(PMC_SEC_LOCK_LP0_PARAMS);

	// Set secmon mailbox address and clear it.
	if (kb >= KB_FIRMWARE_VERSION_700 || is_exo)
	{
		memset((void *)SECMON7_MAILBOX_ADDR, 0, 0x200);
		secmon_mailbox = (secmon_mailbox_t *)(SECMON7_MAILBOX_ADDR + SECMON_STATE_OFFSET);
	}
	else
	{
		if (kb <= KB_FIRMWARE_VERSION_301)
			memset((void *)SECMON_MAILBOX_ADDR, 0, 0x200);
		secmon_mailbox = (secmon_mailbox_t *)(SECMON_MAILBOX_ADDR + SECMON_STATE_OFFSET);
	}

	// Start directly from PKG2 ready signal and reset outgoing value.
	secmon_mailbox->in = pkg1_state_pkg2_ready;
	secmon_mailbox->out = SECMON_STATE_NOT_READY;

	// Disable display. This must be executed before secmon to provide support for all fw versions.
	display_end();

	// Clear EMC_SCRATCH0.
	EMC(EMC_SCRATCH0) = 0;

	// Hold USBD, USB2, AHBDMA and APBDMA in reset for SoC state validation on sleep.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_USBD);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_SET) = BIT(CLK_H_AHBDMA) | BIT(CLK_H_APBDMA) | BIT(CLK_H_USB2);

	// Scale down RAM OC if enabled.
	if (ctxt.stock)
		minerva_prep_boot_freq();

	// Flush cache and disable MMU.
	bpmp_mmu_disable();
	bpmp_clk_rate_set(BPMP_CLK_NORMAL);

	// Launch secmon.
	if (smmu_is_used())
		smmu_exit();
	else
		ccplex_boot_cpu0(secmon_base);

	// Halt ourselves in waitevent state and resume if there's JTAG activity.
	while (true)
		bpmp_halt();

error:
	sdmmc_storage_end(&emmc_storage);

	return 0;
}
