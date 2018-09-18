/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 st4rk
 * Copyright (c) 2018 Ced2911
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

#include <string.h>

#include "hos.h"
#include "../storage/sdmmc.h"
#include "../storage/nx_emmc.h"
#include "../soc/t210.h"
#include "../sec/se.h"
#include "../sec/se_t210.h"
#include "../soc/pmc.h"
#include "../soc/cluster.h"
#include "../mem/heap.h"
#include "../sec/tsec.h"
#include "../utils/util.h"
#include "pkg1.h"
#include "pkg2.h"
#include "../libs/fatfs/ff.h"
#include "../gfx/di.h"
#include "../config/config.h"
#include "../mem/mc.h"

#include "../gfx/gfx.h"
extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;
extern hekate_config h_cfg;

extern void sd_unmount();

//#define DPRINTF(...) gfx_printf(&gfx_con, __VA_ARGS__)
#define DPRINTF(...)

typedef struct _launch_ctxt_t
{
	void *keyblob;

	void *pkg1;
	const pkg1_id_t *pkg1_id;
	const pkg2_kernel_id_t *pkg2_kernel_id;

	void *warmboot;
	u32 warmboot_size;
	void *secmon;
	u32 secmon_size;

	void *pkg2;
	u32 pkg2_size;

	void *kernel;
	u32 kernel_size;
	link_t kip1_list;
	char* kip1_patches;

	bool svcperm;
	bool debugmode;
	bool atmosphere;
} launch_ctxt_t;

typedef struct _merge_kip_t
{
	void *kip1;
	link_t link;
} merge_kip_t;

#define KB_FIRMWARE_VERSION_100_200 0
#define KB_FIRMWARE_VERSION_300 1
#define KB_FIRMWARE_VERSION_301 2
#define KB_FIRMWARE_VERSION_400 3
#define KB_FIRMWARE_VERSION_500 4
#define KB_FIRMWARE_VERSION_600 5
#define KB_FIRMWARE_VERSION_MAX KB_FIRMWARE_VERSION_600

// Exosphère magic "XBC0".
#define MAGIC_EXOSPHERE 0x30434258

static const u8 keyblob_keyseeds[][0x10] = {
	{ 0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3 }, //1.0.0
	{ 0x0C, 0x25, 0x61, 0x5D, 0x68, 0x4C, 0xEB, 0x42, 0x1C, 0x23, 0x79, 0xEA, 0x82, 0x25, 0x12, 0xAC }, //3.0.0
	{ 0x33, 0x76, 0x85, 0xEE, 0x88, 0x4A, 0xAE, 0x0A, 0xC2, 0x8A, 0xFD, 0x7D, 0x63, 0xC0, 0x43, 0x3B }, //3.0.1
	{ 0x2D, 0x1F, 0x48, 0x80, 0xED, 0xEC, 0xED, 0x3E, 0x3C, 0xF2, 0x48, 0xB5, 0x65, 0x7D, 0xF7, 0xBE }, //4.0.0
	{ 0xBB, 0x5A, 0x01, 0xF9, 0x88, 0xAF, 0xF5, 0xFC, 0x6C, 0xFF, 0x07, 0x9E, 0x13, 0x3C, 0x39, 0x80 }, //5.0.0
	{ 0xD8, 0xCC, 0xE1, 0x26, 0x6A, 0x35, 0x3F, 0xCC, 0x20, 0xF3, 0x2D, 0x3B, 0x51, 0x7D, 0xE9, 0xC0 }  //6.0.0
};

static const u8 cmac_keyseed[0x10] =
	{ 0x59, 0xC7, 0xFB, 0x6F, 0xBE, 0x9B, 0xBE, 0x87, 0x65, 0x6B, 0x15, 0xC0, 0x53, 0x73, 0x36, 0xA5 };

static const u8 master_keyseed_retail[0x10] =
	{ 0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C };

static const u8 console_keyseed[0x10] =
	{ 0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78 };

static const u8 key8_keyseed[] =
	{ 0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7 };

static const u8 master_keyseed_4xx_5xx[0x10] = 
	{ 0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66 };

static const u8 console_keyseed_4xx_5xx[0x10] = 
	{ 0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28 };


static void _se_lock()
{
	for (u32 i = 0; i < 16; i++)
		se_key_acc_ctrl(i, 0x15);

	for (u32 i = 0; i < 2; i++)
		se_rsa_acc_ctrl(i, 1);

	SE(0x4) = 0; // Make this reg secure only.
	SE(SE_KEY_TABLE_ACCESS_LOCK_OFFSET) = 0; // Make all key access regs secure only.
	SE(SE_RSA_KEYTABLE_ACCESS_LOCK_OFFSET) = 0; // Make all RSA access regs secure only.
	SE(SE_SECURITY_0) &= 0xFFFFFFFB; // Make access lock regs secure only.

	// This is useful for documenting the bits in the SE config registers, so we can keep it around.
	/*gfx_printf(&gfx_con, "SE(SE_SECURITY_0) = %08X\n", SE(SE_SECURITY_0));
	gfx_printf(&gfx_con, "SE(0x4) = %08X\n", SE(0x4));
	gfx_printf(&gfx_con, "SE(SE_KEY_TABLE_ACCESS_LOCK_OFFSET) = %08X\n", SE(SE_KEY_TABLE_ACCESS_LOCK_OFFSET));
	gfx_printf(&gfx_con, "SE(SE_RSA_KEYTABLE_ACCESS_LOCK_OFFSET) = %08X\n", SE(SE_RSA_KEYTABLE_ACCESS_LOCK_OFFSET));
	for(u32 i = 0; i < 16; i++)
		gfx_printf(&gfx_con, "%02X ", SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + i * 4) & 0xFF);
	gfx_putc(&gfx_con, '\n');
	for(u32 i = 0; i < 2; i++)
		gfx_printf(&gfx_con, "%02X ", SE(SE_RSA_KEYTABLE_ACCESS_REG_OFFSET + i * 4) & 0xFF);
	gfx_putc(&gfx_con, '\n');
	gfx_hexdump(&gfx_con, SE_BASE, (void *)SE_BASE, 0x400);*/
}

int keygen(u8 *keyblob, u32 kb, void *tsec_fw)
{
	u8 tmp[0x10];

	if (kb > KB_FIRMWARE_VERSION_MAX)
		return 0;

	se_key_acc_ctrl(13, 0x15);
	se_key_acc_ctrl(14, 0x15);

	// Get TSEC key.
	if (tsec_query(tmp, 1, tsec_fw) < 0)
		return 0;

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
		se_aes_unwrap_key(14, 12, master_keyseed_4xx_5xx);
		se_aes_unwrap_key(12, 12, master_keyseed_retail);
		break;
	case KB_FIRMWARE_VERSION_500:
	case KB_FIRMWARE_VERSION_600:
		se_aes_unwrap_key(10, 15, console_keyseed_4xx_5xx);
		se_aes_unwrap_key(15, 15, console_keyseed);
		se_aes_unwrap_key(14, 12, master_keyseed_4xx_5xx);
		se_aes_unwrap_key(12, 12, master_keyseed_retail);
		break;
	}

	// Package2 key.
	se_key_acc_ctrl(8, 0x15);
	se_aes_unwrap_key(8, 12, key8_keyseed);

	return 1;
}

static void _copy_bootconfig()
{
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	// Read BCT.
	u8 *buf = (u8 *)0x4003D000;
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0, 0x3000 / NX_EMMC_BLOCKSIZE, buf);

	gfx_printf(&gfx_con, "Copied BCT to 0x4003D000\n");

	sdmmc_storage_end(&storage);
}

static int _read_emmc_pkg1(launch_ctxt_t *ctxt)
{
	int res = 0;
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	// Read package1.
	ctxt->pkg1 = (u8 *)malloc(0x40000);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, ctxt->pkg1);
	ctxt->pkg1_id = pkg1_identify(ctxt->pkg1);
	if (!ctxt->pkg1_id)
	{
		gfx_printf(&gfx_con, "%kUnknown package1,\nVersion (= '%s').%k\n", 0xFFFF0000, (char *)ctxt->pkg1 + 0x10, 0xFFCCCCCC);
		goto out;
	}
	gfx_printf(&gfx_con, "Identified package1 ('%s'),\nKeyblob version %d\n\n", (char *)(ctxt->pkg1 + 0x10), ctxt->pkg1_id->kb);

	// Read the correct keyblob.
	ctxt->keyblob = (u8 *)calloc(NX_EMMC_BLOCKSIZE, 1);
	sdmmc_storage_read(&storage, 0x180000 / NX_EMMC_BLOCKSIZE + ctxt->pkg1_id->kb, 1, ctxt->keyblob);

	res = 1;

out:;
	sdmmc_storage_end(&storage);
	return res;
}

static int _read_emmc_pkg2(launch_ctxt_t *ctxt)
{
	int res = 0;
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);
	sdmmc_storage_set_mmc_partition(&storage, 0);

	// Parse eMMC GPT.
	LIST_INIT(gpt);
	nx_emmc_gpt_parse(&gpt, &storage);
	DPRINTF("Parsed GPT\n");
	// Find package2 partition.
	emmc_part_t *pkg2_part = nx_emmc_part_find(&gpt, "BCPKG2-1-Normal-Main");
	if (!pkg2_part)
		goto out;

	// Read in package2 header and get package2 real size.
	//TODO: implement memalign for DMA buffers.
	u8 *tmp = (u8 *)malloc(NX_EMMC_BLOCKSIZE);
	nx_emmc_part_read(&storage, pkg2_part, 0x4000 / NX_EMMC_BLOCKSIZE, 1, tmp);
	u32 *hdr = (u32 *)(tmp + 0x100);
	u32 pkg2_size = hdr[0] ^ hdr[2] ^ hdr[3];
	free(tmp);
	DPRINTF("pkg2 size on emmc is %08X\n", pkg2_size);
	// Read in package2.
	u32 pkg2_size_aligned = ALIGN(pkg2_size, NX_EMMC_BLOCKSIZE);
	DPRINTF("pkg2 size aligned is %08X\n", pkg2_size_aligned);
	ctxt->pkg2 = malloc(pkg2_size_aligned);
	ctxt->pkg2_size = pkg2_size;
	nx_emmc_part_read(&storage, pkg2_part, 0x4000 / NX_EMMC_BLOCKSIZE, 
		pkg2_size_aligned / NX_EMMC_BLOCKSIZE, ctxt->pkg2);

	res = 1;

out:;
	nx_emmc_gpt_free(&gpt);
	sdmmc_storage_end(&storage);
	return res;
}

static int _config_warmboot(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;
	if (f_open(&fp, value, FA_READ) != FR_OK)
		return 0;
	ctxt->warmboot_size = f_size(&fp);
	ctxt->warmboot = malloc(ctxt->warmboot_size);
	f_read(&fp, ctxt->warmboot, ctxt->warmboot_size, NULL);
	f_close(&fp);
	return 1;
}

static int _config_secmon(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;
	if (f_open(&fp, value, FA_READ) != FR_OK)
		return 0;
	ctxt->secmon_size = f_size(&fp);
	ctxt->secmon = malloc(ctxt->secmon_size);
	f_read(&fp, ctxt->secmon, ctxt->secmon_size, NULL);
	f_close(&fp);
	return 1;
}

static int _config_kernel(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;
	if (f_open(&fp, value, FA_READ) != FR_OK)
		return 0;
	ctxt->kernel_size = f_size(&fp);
	ctxt->kernel = malloc(ctxt->kernel_size);
	f_read(&fp, ctxt->kernel, ctxt->kernel_size, NULL);
	f_close(&fp);
	return 1;
}

static int _config_kip1(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;
	if (f_open(&fp, value, FA_READ) != FR_OK)
		return 0;
	merge_kip_t *mkip1 = (merge_kip_t *)malloc(sizeof(merge_kip_t));
	mkip1->kip1 = malloc(f_size(&fp));
	f_read(&fp, mkip1->kip1, f_size(&fp), NULL);
	DPRINTF("Loaded kip1 from SD (size %08X)\n", f_size(&fp));
	f_close(&fp);
	list_append(&ctxt->kip1_list, &mkip1->link);
	return 1;
}

static int _config_svcperm(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Disabled SVC verification\n");
		ctxt->svcperm = true;
	}
	return 1;
}

static int _config_debugmode(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Enabled Debug mode\n");
		ctxt->debugmode = true;
	}
	return 1;
}

static int _config_atmosphere(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Enabled atmosphere patching\n");
		ctxt->atmosphere = true;
	}
	return 1;
}

static int _config_kip1patch(launch_ctxt_t *ctxt, const char *value)
{
	if (value == NULL)
		return 0;

	int valueLen = strlen(value);
	if (!valueLen)
		return 0;

	if (ctxt->kip1_patches == NULL)
	{
		ctxt->kip1_patches = malloc(valueLen + 1);
		memcpy(ctxt->kip1_patches, value, valueLen);
		ctxt->kip1_patches[valueLen] = 0;
	}
	else
	{
		char *oldAlloc = ctxt->kip1_patches;
		int oldSize = strlen(oldAlloc);
		ctxt->kip1_patches = malloc(oldSize + 1 + valueLen + 1);
		memcpy(ctxt->kip1_patches, oldAlloc, oldSize);
		free(oldAlloc);
		oldAlloc = NULL;
		ctxt->kip1_patches[oldSize++] = ',';
		memcpy(&ctxt->kip1_patches[oldSize], value, valueLen);
		ctxt->kip1_patches[oldSize + valueLen] = 0;
	}
	return 1;
}

typedef struct _cfg_handler_t
{
	const char *key;
	int (*handler)(launch_ctxt_t *ctxt, const char *value);
} cfg_handler_t;

static const cfg_handler_t _config_handlers[] = {
	{ "warmboot", _config_warmboot },
	{ "secmon", _config_secmon },
	{ "kernel", _config_kernel },
	{ "kip1", _config_kip1 },
	{ "kip1patch", _config_kip1patch },
	{ "fullsvcperm", _config_svcperm },
	{ "debugmode", _config_debugmode },
	{ "atmosphere", _config_atmosphere },
	{ NULL, NULL },
};

static int _config(launch_ctxt_t *ctxt, ini_sec_t *cfg)
{
	LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg->kvs, link)
	{
		for(u32 i = 0; _config_handlers[i].key; i++)
		{
			if (!strcmp(_config_handlers[i].key, kv->key))
				if (!_config_handlers[i].handler(ctxt, kv->val))
					return 0;
		}
	}

	return 1;
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

int hos_launch(ini_sec_t *cfg)
{
	int bootStateDramPkg2 = 0;
	int bootStatePkg2Continue = 0;
	int exoFwNumber = 0;
	launch_ctxt_t ctxt;

	memset(&ctxt, 0, sizeof(launch_ctxt_t));
	list_init(&ctxt.kip1_list);

	if (!gfx_con.mute)
		gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	// Try to parse config if present.
	if (cfg && !_config(&ctxt, cfg))
		return 0;

	gfx_printf(&gfx_con, "Initializing...\n\n");

	// Read package1 and the correct keyblob.
	if (!_read_emmc_pkg1(&ctxt))
		return 0;

	gfx_printf(&gfx_con, "Loaded package1 and keyblob\n");

	// Generate keys.
	if (!h_cfg.se_keygen_done)
	{
		keygen(ctxt.keyblob, ctxt.pkg1_id->kb, (u8 *)ctxt.pkg1 + ctxt.pkg1_id->tsec_off);
		h_cfg.se_keygen_done = 1;
		DPRINTF("Generated keys\n");
	}

	// Decrypt and unpack package1 if we require parts of it.
	if (!ctxt.warmboot || !ctxt.secmon)
	{
		pkg1_decrypt(ctxt.pkg1_id, ctxt.pkg1);
		pkg1_unpack((void *)ctxt.pkg1_id->warmboot_base, (void *)ctxt.pkg1_id->secmon_base, NULL, ctxt.pkg1_id, ctxt.pkg1);
		gfx_printf(&gfx_con, "Decrypted and unpacked package1\n");
	}

	// Replace 'warmboot.bin' if requested.
	if (ctxt.warmboot)
		memcpy((void *)ctxt.pkg1_id->warmboot_base, ctxt.warmboot, ctxt.warmboot_size);
	// Set warmboot address in PMC if required.
	if (ctxt.pkg1_id->set_warmboot)
		PMC(APBDEV_PMC_SCRATCH1) = ctxt.pkg1_id->warmboot_base;

	// Replace 'SecureMonitor' if requested.
	if (ctxt.secmon)
		memcpy((void *)ctxt.pkg1_id->secmon_base, ctxt.secmon, ctxt.secmon_size);
	else
	{
		// Else we patch it to allow for an unsigned package2 and patched kernel.
		patch_t *secmon_patchset = ctxt.pkg1_id->secmon_patchset;
		gfx_printf(&gfx_con, "%kPatching Security Monitor%k\n", 0xFFFFBA00, 0xFFCCCCCC);
		for (u32 i = 0; secmon_patchset[i].off != 0xFFFFFFFF; i++)
			*(vu32 *)(ctxt.pkg1_id->secmon_base + secmon_patchset[i].off) = secmon_patchset[i].val;
	}

	gfx_printf(&gfx_con, "Loaded warmboot.bin and secmon\n");

	// Read package2.
	if (!_read_emmc_pkg2(&ctxt))
		return 0;

	gfx_printf(&gfx_con, "Read package2\n");

	// Decrypt package2 and parse KIP1 blobs in INI1 section.
	pkg2_hdr_t *pkg2_hdr = pkg2_decrypt(ctxt.pkg2);

	LIST_INIT(kip1_info);
	pkg2_parse_kips(&kip1_info, pkg2_hdr);

	gfx_printf(&gfx_con, "Parsed ini1\n");

	// Use the kernel included in package2 in case we didn't load one already.
	if (!ctxt.kernel)
	{
		ctxt.kernel = pkg2_hdr->data;
		ctxt.kernel_size = pkg2_hdr->sec_size[PKG2_SEC_KERNEL];

		if (ctxt.svcperm || ctxt.debugmode || ctxt.atmosphere)
		{
			u32 kernel_crc32 = crc32c(ctxt.kernel, ctxt.kernel_size);
			ctxt.pkg2_kernel_id = pkg2_identify(kernel_crc32);

			// In case a kernel patch option is set; allows to disable SVC verification or/and enable debug mode.
			kernel_patch_t *kernel_patchset = ctxt.pkg2_kernel_id->kernel_patchset;
			if (kernel_patchset != NULL)
			{
				gfx_printf(&gfx_con, "%kPatching kernel%k\n", 0xFFFFBA00, 0xFFCCCCCC);
				u32 *temp;
				for (u32 i = 0; kernel_patchset[i].id != 0xFFFFFFFF; i++)
				{
					if ((ctxt.svcperm && kernel_patchset[i].id == SVC_VERIFY_DS)
					|| (ctxt.debugmode && kernel_patchset[i].id == DEBUG_MODE_EN)
					|| (ctxt.atmosphere && kernel_patchset[i].id == ATM_GEN_PATCH))
						*(vu32 *)(ctxt.kernel + kernel_patchset[i].off) = kernel_patchset[i].val;
					else if (ctxt.atmosphere && kernel_patchset[i].id == ATM_ARR_PATCH)
					{
						temp = (u32 *)kernel_patchset[i].ptr;
						for (u32 j = 0; j < kernel_patchset[i].val; j++)
							*(vu32 *)(ctxt.kernel + kernel_patchset[i].off + (j << 2)) = temp[j];
					}
				}
			}
		}
	}

	// Merge extra KIP1s into loaded ones.
	gfx_printf(&gfx_con, "%kPatching kernel initial processes%k\n", 0xFFFFBA00, 0xFFCCCCCC);
	LIST_FOREACH_ENTRY(merge_kip_t, mki, &ctxt.kip1_list, link)
		pkg2_merge_kip(&kip1_info, (pkg2_kip1_t *)mki->kip1);

	// Patch kip1s in memory if needed.
	const char* unappliedPatch = pkg2_patch_kips(&kip1_info, ctxt.kip1_patches);
	if (unappliedPatch != NULL)
	{
		gfx_printf(&gfx_con, "%kREQUESTED PATCH '%s' NOT APPLIED!%k\n", 0xFFFF0000, unappliedPatch, 0xFFCCCCCC);
		sd_unmount(); // Just exiting is not enough until pkg2_patch_kips stops modifying the string passed into it.
		while(1) {} // MUST stop here, because if user requests 'nogc' but it's not applied, their GC controller gets updated!
	}

	// Rebuild and encrypt package2.
	pkg2_build_encrypt((void *)0xA9800000, ctxt.kernel, ctxt.kernel_size, &kip1_info);
	gfx_printf(&gfx_con, "Rebuilt and loaded package2\n");

	// Unmount SD card.
	sd_unmount();

	gfx_printf(&gfx_con, "\n%kBooting...%k\n", 0xFF96FF00, 0xFFCCCCCC);

	se_aes_key_clear(8);
	se_aes_key_clear(11);

	// Final per firmware configuration.
	switch (ctxt.pkg1_id->kb)
	{
	case KB_FIRMWARE_VERSION_100_200:
		if (!exoFwNumber)
		{
			if (!strcmp(ctxt.pkg1_id->id, "20161121183008"))
				exoFwNumber = 1;
			else
				exoFwNumber = 2;
		}
	case KB_FIRMWARE_VERSION_300:
	case KB_FIRMWARE_VERSION_301:
		if (ctxt.pkg1_id->kb == KB_FIRMWARE_VERSION_300)
			PMC(APBDEV_PMC_SECURE_SCRATCH32) = 0xE3;  // Warmboot 3.0.0 security check.
		else if (ctxt.pkg1_id->kb == KB_FIRMWARE_VERSION_301)
			PMC(APBDEV_PMC_SECURE_SCRATCH32) = 0x104; // Warmboot 3.0.1/.2 security check.
		se_key_acc_ctrl(12, 0xFF);
		se_key_acc_ctrl(13, 0xFF);
		bootStateDramPkg2 = 2;
		bootStatePkg2Continue = 3;
		if (!exoFwNumber)
			exoFwNumber = 3;
		break;
	case KB_FIRMWARE_VERSION_400:
		if (!exoFwNumber)
			exoFwNumber = 4;
	case KB_FIRMWARE_VERSION_500:
		if (!exoFwNumber)
			exoFwNumber = 5;
	case KB_FIRMWARE_VERSION_600:
	default:
		se_key_acc_ctrl(12, 0xFF);
		se_key_acc_ctrl(15, 0xFF);
		bootStateDramPkg2 = 2;
		bootStatePkg2Continue = 4;
		if (!exoFwNumber)
			exoFwNumber = 6;
		break;
	}

	// Free allocated memory.
	ini_free_section(cfg);
	_free_launch_components(&ctxt);

	// Copy BCT if debug mode is enabled.
	if (ctxt.pkg1_id->kb < KB_FIRMWARE_VERSION_600)
		memset((void *)0x4003D000, 0, 0x3000);
	else
		memset((void *)0x4003F000, 0, 0x1000);
	
	if (ctxt.debugmode)
		_copy_bootconfig(&ctxt);

	// Config Exosphère if booting Atmosphère.
	if (ctxt.atmosphere)
	{
		vu32 *mb_exo_magic = (vu32 *)0x40002E40;
		vu32 *mb_exo_fw_no = (vu32 *)0x40002E44;

		*mb_exo_magic = MAGIC_EXOSPHERE;
		*mb_exo_fw_no = exoFwNumber;
	}

	// Finalize MC carveout and lock SE before starting 'SecureMonitor'.
	mc_config_carveout_finalize();
	_se_lock();

	//  < 4.0.0 Signals - 0: Nothing ready, 1: BCT ready, 2: DRAM and pkg2 ready, 3: Continue boot.
	// >= 4.0.0 Signals - 0: Nothing ready, 1: BCT ready, 2: DRAM ready, 4: pkg2 ready and continue boot.
	vu32 *mb_in = (vu32 *)0x40002EF8;
	// Non-zero: Secmon ready.
	vu32 *mb_out = (vu32 *)0x40002EFC;

	// Start from DRAM ready signal.
	*mb_in = bootStateDramPkg2;
	*mb_out = 0;

	// Disable display. This must be executed before secmon to provide support for all fw versions.
	display_end();

	// Wait for secmon to get ready.
	cluster_boot_cpu0(ctxt.pkg1_id->secmon_base);
	while (!*mb_out)
		usleep(1); // This only works when in IRAM or with a trained DRAM. 

	//TODO: pkg1.1 locks PMC scratches, we can do that too at some point.
	/*PMC(0x4) = 0x7FFFF3;
	PMC(0x2C4) = 0xFFFFFFFF;
	PMC(0x2D8) = 0xFFAFFFFF;
	PMC(0x5B0) = 0xFFFFFFFF;
	PMC(0x5B4) = 0xFFFFFFFF;
	PMC(0x5B8) = 0xFFFFFFFF;
	PMC(0x5BC) = 0xFFFFFFFF;
	PMC(0x5C0) = 0xFFAAFFFF;*/

	// Signal pkg2 ready and continue boot.
	*mb_in = bootStatePkg2Continue;

	// Halt ourselves in waitevent state and resume if there's JTAG activity.
	while (1)
		FLOW_CTLR(FLOW_CTLR_HALT_COP_EVENTS) = 0x50000000;

	return 0;
}
