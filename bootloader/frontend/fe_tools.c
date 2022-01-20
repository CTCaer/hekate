/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2022 CTCaer
 * Copyright (c) 2018 Reisyukaku
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
#include <stdlib.h>

#include <bdk.h>

#include "fe_tools.h"
#include "../config.h"
#include "../gfx/tui.h"
#include "../hos/hos.h"
#include "../hos/pkg1.h"
#include "../hos/pkg2.h"
#include <libs/fatfs/ff.h>

extern boot_cfg_t b_cfg;
extern hekate_config h_cfg;

extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

#pragma GCC push_options
#pragma GCC optimize ("Os")

void dump_packages12()
{
	if (!sd_mount())
		return;

	char path[64];

	u8 *pkg1 = (u8 *)calloc(1, SZ_256K);
	u8 *warmboot = (u8 *)calloc(1, SZ_256K);
	u8 *secmon = (u8 *)calloc(1, SZ_256K);
	u8 *loader = (u8 *)calloc(1, SZ_256K);
	u8 *pkg2 = NULL;
	u8 kb = 0;

	tsec_ctxt_t tsec_ctxt;

	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	if (!emmc_initialize(false))
	{
		EPRINTF("Failed to init eMMC.");
		goto out_free;
	}
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_BOOT0);

	// Read package1.
	sdmmc_storage_read(&emmc_storage, 0x100000 / EMMC_BLOCKSIZE, SZ_256K / EMMC_BLOCKSIZE, pkg1);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1);
	if (!pkg1_id)
	{
		EPRINTF("Unknown pkg1 version for reading\nTSEC firmware.");
		// Dump package1.
		emmcsn_path_impl(path, "/pkg1", "pkg1_enc.bin", &emmc_storage);
		if (sd_save_to_file(pkg1, SZ_256K, path))
			goto out_free;
		gfx_puts("\nEnc pkg1 dumped to pkg1_enc.bin\n");

		goto out_free;
	}

	kb = pkg1_id->kb;

	tsec_ctxt.fw = (void *)pkg1 + pkg1_id->tsec_off;
	tsec_ctxt.pkg1 = (void *)pkg1;
	tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;
	tsec_ctxt.secmon_base = pkg1_id->secmon_base;

	// Read keyblob.
	u8 *keyblob = (u8 *)calloc(EMMC_BLOCKSIZE, 1);
	sdmmc_storage_read(&emmc_storage, 0x180000 / EMMC_BLOCKSIZE + kb, 1, keyblob);

	// Decrypt.
	hos_keygen(keyblob, kb, &tsec_ctxt, false, false);
	free(keyblob);

	if (kb <= KB_FIRMWARE_VERSION_600)
		pkg1_decrypt(pkg1_id, pkg1);

	if (kb <= KB_FIRMWARE_VERSION_620)
	{
		const u8 *sec_map = pkg1_unpack(warmboot, NULL, secmon, loader, pkg1_id, pkg1);

		pk11_hdr_t *hdr_pk11 = (pk11_hdr_t *)(pkg1 + pkg1_id->pkg11_off + 0x20);

		// Use correct sizes.
		u32 sec_size[3] = { hdr_pk11->wb_size, hdr_pk11->ldr_size, hdr_pk11->sm_size };
		for (u32 i = 0; i < 3; i++)
		{
			if (sec_map[i] == PK11_SECTION_WB)
				hdr_pk11->wb_size = sec_size[i];
			else if (sec_map[i] == PK11_SECTION_LD)
				hdr_pk11->ldr_size = sec_size[i];
			else if (sec_map[i] == PK11_SECTION_SM)
				hdr_pk11->sm_size = sec_size[i];
		}

		// Display info.
		gfx_printf("%kNX Bootloader size:  %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr_pk11->ldr_size);

		gfx_printf("%kSecure monitor addr: %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, pkg1_id->secmon_base);
		gfx_printf("%kSecure monitor size: %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr_pk11->sm_size);

		gfx_printf("%kWarmboot addr:       %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, pkg1_id->warmboot_base);
		gfx_printf("%kWarmboot size:       %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr_pk11->wb_size);

		// Dump package1.1.
		emmcsn_path_impl(path, "/pkg1", "pkg1_decr.bin", &emmc_storage);
		if (sd_save_to_file(pkg1, SZ_256K, path))
			goto out_free;
		gfx_puts("\npkg1 dumped to pkg1_decr.bin\n");

		// Dump nxbootloader.
		emmcsn_path_impl(path, "/pkg1", "nxloader.bin", &emmc_storage);
		if (sd_save_to_file(loader, hdr_pk11->ldr_size, path))
			goto out_free;
		gfx_puts("NX Bootloader dumped to nxloader.bin\n");

		// Dump secmon.
		emmcsn_path_impl(path, "/pkg1", "secmon.bin", &emmc_storage);
		if (sd_save_to_file(secmon, hdr_pk11->sm_size, path))
			goto out_free;
		gfx_puts("Secure Monitor dumped to secmon.bin\n");

		// Dump warmboot.
		emmcsn_path_impl(path, "/pkg1", "warmboot.bin", &emmc_storage);
		if (sd_save_to_file(warmboot, hdr_pk11->wb_size, path))
			goto out_free;
		gfx_puts("Warmboot dumped to warmboot.bin\n\n\n");
	}

	// Dump package2.1.
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);
	// Parse eMMC GPT.
	LIST_INIT(gpt);
	emmc_gpt_parse(&gpt);
	// Find package2 partition.
	emmc_part_t *pkg2_part = emmc_part_find(&gpt, "BCPKG2-1-Normal-Main");
	if (!pkg2_part)
		goto out;

	// Read in package2 header and get package2 real size.
	u8 *tmp = (u8 *)malloc(EMMC_BLOCKSIZE);
	emmc_part_read(pkg2_part, 0x4000 / EMMC_BLOCKSIZE, 1, tmp);
	u32 *hdr_pkg2_raw = (u32 *)(tmp + 0x100);
	u32 pkg2_size = hdr_pkg2_raw[0] ^ hdr_pkg2_raw[2] ^ hdr_pkg2_raw[3];
	free(tmp);
	// Read in package2.
	u32 pkg2_size_aligned = ALIGN(pkg2_size, EMMC_BLOCKSIZE);
	pkg2 = malloc(pkg2_size_aligned);
	emmc_part_read(pkg2_part, 0x4000 / EMMC_BLOCKSIZE,
		pkg2_size_aligned / EMMC_BLOCKSIZE, pkg2);

#if 0
	emmcsn_path_impl(path, "/pkg2", "pkg2_encr.bin", &emmc_storage);
	if (sd_save_to_file(pkg2, pkg2_size_aligned, path))
		goto out;
	gfx_puts("\npkg2 dumped to pkg2_encr.bin\n");
#endif

	// Decrypt package2 and parse KIP1 blobs in INI1 section.
	pkg2_hdr_t *pkg2_hdr = pkg2_decrypt(pkg2, kb, false);
	if (!pkg2_hdr)
	{
		gfx_printf("Pkg2 decryption failed!\n");
		goto out;
	}

	// Display info.
	gfx_printf("%kKernel size:   %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, pkg2_hdr->sec_size[PKG2_SEC_KERNEL]);
	gfx_printf("%kINI1 size:     %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, pkg2_hdr->sec_size[PKG2_SEC_INI1]);

	// Dump pkg2.1.
	emmcsn_path_impl(path, "/pkg2", "pkg2_decr.bin", &emmc_storage);
	if (sd_save_to_file(pkg2, pkg2_hdr->sec_size[PKG2_SEC_KERNEL] + pkg2_hdr->sec_size[PKG2_SEC_INI1], path))
		goto out;
	gfx_puts("\npkg2 dumped to pkg2_decr.bin\n");

	// Dump kernel.
	emmcsn_path_impl(path, "/pkg2", "kernel.bin", &emmc_storage);
	if (sd_save_to_file(pkg2_hdr->data, pkg2_hdr->sec_size[PKG2_SEC_KERNEL], path))
		goto out;
	gfx_puts("Kernel dumped to kernel.bin\n");

	// Dump INI1.
	emmcsn_path_impl(path, "/pkg2", "ini1.bin", &emmc_storage);
	u32 ini1_off = pkg2_hdr->sec_size[PKG2_SEC_KERNEL];
	u32 ini1_size = pkg2_hdr->sec_size[PKG2_SEC_INI1];
	if (!ini1_size)
	{
		pkg2_get_newkern_info(pkg2_hdr->data);
		ini1_off = pkg2_newkern_ini1_start;
		ini1_size = pkg2_newkern_ini1_end - pkg2_newkern_ini1_start;
	}
	if (ini1_off)
	{
		if (sd_save_to_file(pkg2_hdr->data + ini1_off, ini1_size, path))
			goto out;
		gfx_puts("INI1 dumped to ini1.bin\n");
	}
	else
	{
		gfx_puts("Failed to dump INI1!\n");
		goto out;
	}

	gfx_puts("\nDone. Press any key...\n");

out:
	emmc_gpt_free(&gpt);
out_free:
	free(pkg1);
	free(secmon);
	free(warmboot);
	free(loader);
	free(pkg2);
	sdmmc_storage_end(&emmc_storage);
	sd_end();

	if (kb >= KB_FIRMWARE_VERSION_620)
		se_aes_key_clear(8);

	btn_wait();
}

void _toggle_autorcm(bool enable)
{
	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	if (!emmc_initialize(false))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}

	u8 *tempbuf = (u8 *)malloc(0x200);
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_BOOT0);

	int i, sect = 0;
	u8 corr_mod0, mod1;

	// Get the correct RSA modulus byte masks.
	nx_emmc_get_autorcm_masks(&corr_mod0, &mod1);

	// Iterate BCTs.
	for (i = 0; i < 4; i++)
	{
		sect = (0x200 + (0x4000 * i)) / EMMC_BLOCKSIZE;
		sdmmc_storage_read(&emmc_storage, sect, 1, tempbuf);

		// Check if 2nd byte of modulus is correct.
		if (tempbuf[0x11] != mod1)
			continue;

		if (enable)
			tempbuf[0x10] = 0;
		else
			tempbuf[0x10] = corr_mod0;
		sdmmc_storage_write(&emmc_storage, sect, 1, tempbuf);
	}

	free(tempbuf);
	sdmmc_storage_end(&emmc_storage);

	if (enable)
		gfx_printf("%kAutoRCM mode enabled!%k", 0xFFFFBA00, 0xFFCCCCCC);
	else
		gfx_printf("%kAutoRCM mode disabled!%k", 0xFF96FF00, 0xFFCCCCCC);
	gfx_printf("\n\nPress any key...\n");

out:
	btn_wait();
}

void _enable_autorcm()  { _toggle_autorcm(true); }
void _disable_autorcm() { _toggle_autorcm(false); }

void menu_autorcm()
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	if (h_cfg.rcm_patched)
	{
		WPRINTF("This device is RCM patched and the\nfunction is disabled to avoid BRICKS!\n");
		btn_wait();

		return;
	}

	// Do a simple check on the main BCT.
	bool disabled = true;

	if (!emmc_initialize(false))
	{
		EPRINTF("Failed to init eMMC.");
		btn_wait();

		return;
	}

	u8 mod0, mod1;
	// Get the correct RSA modulus byte masks.
	nx_emmc_get_autorcm_masks(&mod0, &mod1);

	u8 *tempbuf = (u8 *)malloc(0x200);
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_BOOT0);
	sdmmc_storage_read(&emmc_storage, 0x200 / EMMC_BLOCKSIZE, 1, tempbuf);

	// Check if 2nd byte of modulus is correct.
	if (tempbuf[0x11] == mod1)
		if (tempbuf[0x10] != mod0)
			disabled = false;

	free(tempbuf);
	sdmmc_storage_end(&emmc_storage);

	// Create AutoRCM menu.
	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * 6);

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	ments[2].type = MENT_CAPTION;
	ments[3].type = MENT_CHGLINE;
	if (disabled)
	{
		ments[2].caption = "Status: Disabled!";
		ments[2].color = 0xFF96FF00;
		ments[4].caption = "Enable AutoRCM";
		ments[4].handler = _enable_autorcm;
	}
	else
	{
		ments[2].caption = "Status: Enabled!";
		ments[2].color = 0xFFFFBA00;
		ments[4].caption = "Disable AutoRCM";
		ments[4].handler = _disable_autorcm;
	}
	ments[4].type = MENT_HDLR_RE;
	ments[4].data = NULL;

	memset(&ments[5], 0, sizeof(ment_t));
	menu_t menu = {ments, "This corrupts BOOT0!", 0, 0};

	tui_do_menu(&menu);
}

#pragma GCC pop_options
