/*
* Copyright (c) 2018 naehrwert
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
#include "sdmmc.h"
#include "nx_emmc.h"
#include "t210.h"
#include "se.h"
#include "se_t210.h"
#include "pmc.h"
#include "cluster.h"
#include "heap.h"
#include "tsec.h"
#include "pkg2.h"
#include "nx_emmc.h"
#include "util.h"
#include "pkg1.h"
#include "pkg2.h"
#include "ff.h"

#include "gfx.h"
extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;
#define DPRINTF(...) gfx_printf(&gfx_con, __VA_ARGS__)
//#define DPRINTF(...)

enum KB_FIRMWARE_VERSION {
	KB_FIRMWARE_VERSION_100_200 = 0,
	KB_FIRMWARE_VERSION_300 = 1,
	KB_FIRMWARE_VERSION_301 = 2,
	KB_FIRMWARE_VERSION_400 = 3,
	KB_FIRMWARE_VERSION_500 = 4,
	KB_FIRMWARE_VERSION_MAX
};

#define NUM_KEYBLOB_KEYS 5
static const u8 keyblob_keyseeds[NUM_KEYBLOB_KEYS][0x10] = {
	{ 0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3 }, //1.0.0
	{ 0x0C, 0x25, 0x61, 0x5D, 0x68, 0x4C, 0xEB, 0x42, 0x1C, 0x23, 0x79, 0xEA, 0x82, 0x25, 0x12, 0xAC }, //3.0.0
	{ 0x33, 0x76, 0x85, 0xEE, 0x88, 0x4A, 0xAE, 0x0A, 0xC2, 0x8A, 0xFD, 0x7D, 0x63, 0xC0, 0x43, 0x3B }, //3.0.1
	{ 0x2D, 0x1F, 0x48, 0x80, 0xED, 0xEC, 0xED, 0x3E, 0x3C, 0xF2, 0x48, 0xB5, 0x65, 0x7D, 0xF7, 0xBE }, //4.0.0
	{ 0xBB, 0x5A, 0x01, 0xF9, 0x88, 0xAF, 0xF5, 0xFC, 0x6C, 0xFF, 0x07, 0x9E, 0x13, 0x3C, 0x39, 0x80 }  //5.0.0
};

static const u8 cmac_keyseed[0x10] =
	{ 0x59, 0xC7, 0xFB, 0x6F, 0xBE, 0x9B, 0xBE, 0x87, 0x65, 0x6B, 0x15, 0xC0, 0x53, 0x73, 0x36, 0xA5 };

static const u8 master_keyseed_retail[0x10] =
	{ 0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C };

static const u8 console_keyseed[0x10] =
	{ 0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78 };

static const u8 key8_keyseed[] =
	{ 0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7 };

static const u8 master_keyseed_4xx[0x10] = 
	{ 0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66 };

static const u8 console_keyseed_4xx[0x10] = 
	{ 0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28 };


static void _se_lock()
{
	for (u32 i = 0; i < 16; i++)
		se_key_acc_ctrl(i, 0x15);

	for (u32 i = 0; i < 2; i++)
		se_rsa_acc_ctrl(i, 1);

	SE(0x4) = 0; //Make this reg secure only.
	SE(SE_KEY_TABLE_ACCESS_LOCK_OFFSET) = 0; //Make all key access regs secure only.
	SE(SE_RSA_KEYTABLE_ACCESS_LOCK_OFFSET) = 0; //Make all rsa access regs secure only.
	SE(SE_SECURITY_0) &= 0xFFFFFFFB; //Make access lock regs secure only.

	//This is useful for documenting the bits in the SE config registers, so we can keep it around.
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

// <-- key derivation algorithm
int keygen(u8 *keyblob, u32 kb, void *tsec_fw)
{
	u8 tmp[0x10];

	se_key_acc_ctrl(0x0D, 0x15);
	se_key_acc_ctrl(0x0E, 0x15);

	//Get TSEC key.
	if (tsec_query(tmp, 1, tsec_fw) < 0)
		return 0;

	se_aes_key_set(0x0D, tmp, 0x10);

	//Derive keyblob keys from TSEC+SBK.
	se_aes_crypt_block_ecb(0x0D, 0x00, tmp, keyblob_keyseeds[0]);
	se_aes_unwrap_key(0x0F, 0x0E, tmp);
	se_aes_crypt_block_ecb(0xD, 0x00, tmp, keyblob_keyseeds[kb]);
	se_aes_unwrap_key(0x0D, 0x0E, tmp);

	// Clear SBK
	se_aes_key_clear(0x0E);

	//TODO: verify keyblob CMAC.
	//se_aes_unwrap_key(11, 13, cmac_keyseed);
	//se_aes_cmac(tmp, 0x10, 11, keyblob + 0x10, 0xA0);
	//if (!memcmp(keyblob, tmp, 0x10))
	//	return 0;

	se_aes_crypt_block_ecb(0x0D, 0, tmp, cmac_keyseed);
	se_aes_unwrap_key(0x0B, 0x0D, cmac_keyseed);

	//Decrypt keyblob and set keyslots.
	se_aes_crypt_ctr(0x0D, keyblob + 0x20, 0x90, keyblob + 0x20, 0x90, keyblob + 0x10);
	se_aes_key_set(0x0B, keyblob + 0x20 + 0x80, 0x10); // package1 key
	se_aes_key_set(0x0C, keyblob + 0x20, 0x10);
	se_aes_key_set(0x0D, keyblob + 0x20, 0x10);

	se_aes_crypt_block_ecb(0x0C, 0, tmp, master_keyseed_retail);

	switch (kb) {
		case KB_FIRMWARE_VERSION_100_200:
		case KB_FIRMWARE_VERSION_300:
			se_aes_unwrap_key(0x0D, 0x0F, console_keyseed);
			se_aes_unwrap_key(0x0C, 0x0C, master_keyseed_retail);
		break;

		case KB_FIRMWARE_VERSION_400:
			se_aes_unwrap_key(0x0D, 0x0F, console_keyseed_4xx);
			se_aes_unwrap_key(0x0F, 0x0F, console_keyseed);
			se_aes_unwrap_key(0x0E, 0x0C, master_keyseed_4xx);
			se_aes_unwrap_key(0x0C, 0x0C, master_keyseed_retail);
		break;

		case KB_FIRMWARE_VERSION_500:
		default:
			se_aes_unwrap_key(0x0A, 0x0F, console_keyseed_4xx);
			se_aes_unwrap_key(0x0F, 0x0F, console_keyseed);
			se_aes_unwrap_key(0x0E, 0x0C, master_keyseed_4xx);
			se_aes_unwrap_key(0x0C, 0x0C, master_keyseed_retail);
		break;
	}

	// Package2 key 
	se_key_acc_ctrl(0x08, 0x15);
	se_aes_unwrap_key(0x08, 0x0C, key8_keyseed);
}


typedef struct _launch_ctxt_t
{
	void *keyblob;

	void *pkg1;
	const pkg1_id_t *pkg1_id;

	void *warmboot;
	u32 warmboot_size;
	void *secmon;
	u32 secmon_size;

	void *pkg2;
	u32 pkg2_size;

	void *kernel;
	u32 kernel_size;
	link_t kip1_list;
} launch_ctxt_t;

typedef struct _merge_kip_t
{
	void *kip1;
	link_t link;
} merge_kip_t;

static int _read_emmc_pkg1(launch_ctxt_t *ctxt)
{
	int res = 0;
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	//Read package1.
	ctxt->pkg1 = (u8 *)malloc(0x40000);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, ctxt->pkg1);
	ctxt->pkg1_id = pkg1_identify(ctxt->pkg1);
	if (!ctxt->pkg1_id)
	{
		DPRINTF("%kCould not identify package 1 version (= '%s').%k\n", 0xFF0000FF, (char *)ctxt->pkg1 + 0x10, 0xFFFFFFFF);
		goto out;
	}
	DPRINTF("Identified package1 ('%s'), keyblob version %d\n", (char *)(ctxt->pkg1 + 0x10), ctxt->pkg1_id->kb);

	//Read the correct keyblob.
	ctxt->keyblob = (u8 *)malloc(NX_EMMC_BLOCKSIZE);
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

	//Parse eMMC GPT.
	LIST_INIT(gpt);
	nx_emmc_gpt_parse(&gpt, &storage);
DPRINTF("parsed GPT\n");
	//Find package2 partition.
	emmc_part_t *pkg2_part = nx_emmc_part_find(&gpt, "BCPKG2-1-Normal-Main");
	if (!pkg2_part)
		goto out;

	//Read in package2 header and get package2 real size.
	//TODO: implement memalign for DMA buffers.
	u8 *tmp = (u8 *)malloc(NX_EMMC_BLOCKSIZE);
	nx_emmc_part_read(&storage, pkg2_part, 0x4000 / NX_EMMC_BLOCKSIZE, 1, tmp);
	u32 *hdr = (u32 *)(tmp + 0x100);
	u32 pkg2_size = hdr[0] ^ hdr[2] ^ hdr[3];
	free(tmp);
DPRINTF("pkg2 size on emmc is %08X\n", pkg2_size);
	//Read in package2.
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
DPRINTF("loaded kip from SD (size %08X)\n", f_size(&fp));
	f_close(&fp);
	list_append(&ctxt->kip1_list, &mkip1->link);
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
	{ NULL, NULL },
};

static int _config(launch_ctxt_t *ctxt, ini_sec_t *cfg)
{
	LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg->kvs, link)
		for(u32 i = 0; _config_handlers[i].key; i++)
			if (!strcmp(_config_handlers[i].key, kv->key) &&
				!_config_handlers[i].handler(ctxt, kv->val))
				return 0;
	return 1;
}

int hos_launch(ini_sec_t *cfg)
{
	launch_ctxt_t ctxt;
	memset(&ctxt, 0, sizeof(launch_ctxt_t));
	list_init(&ctxt.kip1_list);

	if (cfg && !_config(&ctxt, cfg))
		return 0;

	//Read package1 and the correct keyblob.
	if (!_read_emmc_pkg1(&ctxt))
		return 0;

	//XXX: remove this once we support 3+.
	//if (ctxt.pkg1_id->kb > 0)
	//	return 0;

DPRINTF("loaded pkg1 and keyblob\n");
	//Generate keys.
	keygen(ctxt.keyblob, ctxt.pkg1_id->kb, (u8 *)ctxt.pkg1 + ctxt.pkg1_id->tsec_off);
DPRINTF("generated keys\n");
	//Decrypt and unpack package1 if we require parts of it.
	if (!ctxt.warmboot || !ctxt.secmon)
	{
		pkg1_decrypt(ctxt.pkg1_id, ctxt.pkg1);
		pkg1_unpack((void *)0x8000D000, (void *)ctxt.pkg1_id->secmon_base, ctxt.pkg1_id, ctxt.pkg1);
		//gfx_hexdump(&gfx_con, 0x8000D000, (void *)0x8000D000, 0x100);
		//gfx_hexdump(&gfx_con, ctxt.pkg1_id->secmon_base, (void *)ctxt.pkg1_id->secmon_base, 0x100);
DPRINTF("decrypted and unpacked pkg1\n");
	}
	//Replace 'warmboot.bin' if requested.
	if (ctxt.warmboot)
		memcpy((void *)0x8000D000, ctxt.warmboot, ctxt.warmboot_size);
	//Set warmboot address in PMC.
	PMC(APBDEV_PMC_SCRATCH1) = 0x8000D000;
	//Replace 'SecureMonitor' if requested.
	if (ctxt.secmon) {
		memcpy((void *)ctxt.pkg1_id->secmon_base, ctxt.secmon, ctxt.secmon_size);
	}
	else
	{
		//Else we patch it to allow for an unsigned package2.
		patch_t *secmon_patchset = ctxt.pkg1_id->secmon_patchset;
		
		if (secmon_patchset != NULL) {
			for (u32 i = 0; secmon_patchset[i].off != 0xFFFFFFFF; i++)
				*(vu32 *)(ctxt.pkg1_id->secmon_base + secmon_patchset[i].off) = secmon_patchset[i].val;

			DPRINTF("loaded warmboot.bin and secmon\n");
			
			//Read package2.
			if (!_read_emmc_pkg2(&ctxt))
				return 0;

			DPRINTF("read pkg2\n");
			//Decrypt package2 and parse KIP1 blobs in INI1 section.
			pkg2_hdr_t *pkg2_hdr = pkg2_decrypt(ctxt.pkg2);

			LIST_INIT(kip1_info);
			pkg2_parse_kips(&kip1_info, pkg2_hdr);

			DPRINTF("parsed ini1\n");
			
			//Use the kernel included in package2 in case we didn't load one already.
			if (!ctxt.kernel)
			{
				ctxt.kernel = pkg2_hdr->data;
				ctxt.kernel_size = pkg2_hdr->sec_size[PKG2_SEC_KERNEL];
			}

			//Merge extra KIP1s into loaded ones.
			LIST_FOREACH_ENTRY(merge_kip_t, mki, &ctxt.kip1_list, link)
				pkg2_merge_kip(&kip1_info, (pkg2_kip1_t *)mki->kip1);

			//Rebuild and encrypt package2.
			pkg2_build_encrypt((void *)0xA9800000, ctxt.kernel, ctxt.kernel_size, &kip1_info);
			DPRINTF("rebuilt pkg2\n");
		} else {
			//Read package2.
			if (!_read_emmc_pkg2(&ctxt))
				return 0;

			DPRINTF("read pkg2\n");
			memcpy((void *)0xA9800000, ctxt.pkg2, ctxt.pkg2_size);
		}
	}

	se_aes_key_clear(8);
	se_aes_key_clear(11);
	//se_aes_key_clear(13);
	//se_key_acc_ctrl(10, 0xFF);
	se_key_acc_ctrl(12, 0xFF);
	//se_key_acc_ctrl(13, 0xFF);
	//se_key_acc_ctrl(14, 0xFF);
	se_key_acc_ctrl(15, 0xFF);

	//Clear 'BootConfig'.
	memset((void *)0x4003D000, 0, 0x3000);

	//pkg2_decrypt((void *)0xA9800000);
	//sleep(10000);
	//btn_wait();
	//return 0;

	//Lock SE before starting 'SecureMonitor'.
	_se_lock();

	vu32 *mb_in = (vu32 *)0x40002EF8;
	vu32 *mb_out = (vu32 *)0x40002EFC;

	*mb_in = 0;
	*mb_out = 0;

	//Wait for secmon to get ready.
	cluster_boot_cpu0(ctxt.pkg1_id->secmon_base);
	while (!*mb_out)
		sleep(1);

	//Signal 'BootConfig'.
	*mb_in = 1;
	sleep(100);

	//Signal package2 available.
	*mb_in = 2;
	sleep(100);
	*mb_in = 3;
	sleep(100);

	

	/*PMC(0x4) = 0x7FFFF3;
	PMC(0x2C4) = 0xFFFFFFFF;
	PMC(0x2D8) = 0xFFAFFFFF;
	PMC(0x5B0) = 0xFFFFFFFF;
	PMC(0x5B4) = 0xFFFFFFFF;
	PMC(0x5B8) = 0xFFFFFFFF;
	PMC(0x5BC) = 0xFFFFFFFF;
	PMC(0x5C0) = 0xFFAAFFFF;*/

	//TODO: Cleanup.
	//display_end();

	//Signal to continue boot.
	*mb_in = 4;
	sleep(100);

	//Halt ourselves in waitevent state.
	while (1)
		FLOW_CTLR(0x4) = 0x50000000;

	return 0;
}
