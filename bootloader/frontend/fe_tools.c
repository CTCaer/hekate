/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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

#include "fe_tools.h"
#include "../config/config.h"
#include "../gfx/gfx.h"
#include "../gfx/tui.h"
#include "../hos/hos.h"
#include "../hos/pkg1.h"
#include "../hos/pkg2.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../power/max7762x.h"
#include "../sec/se.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/util.h"

extern hekate_config h_cfg;
extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;
extern sdmmc_storage_t sd_storage;

extern bool sd_mount();
extern void sd_unmount();
extern int  sd_save_to_file(void *buf, u32 size, const char *filename);
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

//TODO: Create more macros (info, header, debug, etc) with different colors and utilize them for consistency.
#define EPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, 0xFFCCCCCC)
#define EPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, args, 0xFFCCCCCC)

void dump_packages12()
{
	if (!sd_mount())
		return;

	u8 *pkg1 = (u8 *)calloc(1, 0x40000);
	u8 *warmboot = (u8 *)calloc(1, 0x40000);
	u8 *secmon = (u8 *)calloc(1, 0x40000);
	u8 *loader = (u8 *)calloc(1, 0x40000);
	u8 *pkg2 = NULL;
	u8 kb = 0;

	tsec_ctxt_t tsec_ctxt;

	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out_free;
	}
	sdmmc_storage_set_mmc_partition(&storage, 1);

	// Read package1.
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1);
	const pk11_hdr_t *hdr = (pk11_hdr_t *)(pkg1 + pkg1_id->pkg11_off + 0x20);
	if (!pkg1_id)
	{
		gfx_con.fntsz = 8;
		EPRINTFARGS("Unknown package1 version for reading\nTSEC firmware (= '%s').", (char *)pkg1 + 0x10);
		goto out_free;
	}

	kb = pkg1_id->kb;

	if (!h_cfg.se_keygen_done || kb >= KB_FIRMWARE_VERSION_620)
	{
		tsec_ctxt.key_ver = 1;
		tsec_ctxt.fw = (void *)pkg1 + pkg1_id->tsec_off;
		tsec_ctxt.pkg1 = (void *)pkg1;
		tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;
		tsec_ctxt.secmon_base = pkg1_id->secmon_base;

		// Read keyblob.
		u8 *keyblob = (u8 *)calloc(NX_EMMC_BLOCKSIZE, 1);
		sdmmc_storage_read(&storage, 0x180000 / NX_EMMC_BLOCKSIZE + kb, 1, keyblob);

		// Decrypt.
		keygen(keyblob, kb, &tsec_ctxt);

		h_cfg.se_keygen_done = 1;
		free(keyblob);
	}

	if (kb <= KB_FIRMWARE_VERSION_600)
		pkg1_decrypt(pkg1_id, pkg1);

	pkg1_unpack(warmboot, secmon, loader, pkg1_id, pkg1);

	// Display info.
	gfx_printf(&gfx_con, "%kNX Bootloader size:  %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->ldr_size);

	gfx_printf(&gfx_con, "%kSecure monitor addr: %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, pkg1_id->secmon_base);
	gfx_printf(&gfx_con, "%kSecure monitor size: %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->sm_size);

	gfx_printf(&gfx_con, "%kWarmboot addr:       %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, pkg1_id->warmboot_base);
	gfx_printf(&gfx_con, "%kWarmboot size:       %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->wb_size);

	char path[64];
	// Dump package1.1.
	emmcsn_path_impl(path, "/pkg1", "pkg1_decr.bin", &storage);
	if (sd_save_to_file(pkg1, 0x40000, path))
		goto out_free;
	gfx_puts(&gfx_con, "\nFull package1 dumped to pkg1_decr.bin\n");

	// Dump nxbootloader.
	emmcsn_path_impl(path, "/pkg1", "nxloader.bin", &storage);
	if (sd_save_to_file(loader, hdr->ldr_size, path))
		goto out_free;
	gfx_puts(&gfx_con, "NX Bootloader dumped to nxloader.bin\n");

	// Dump secmon.
	emmcsn_path_impl(path, "/pkg1", "secmon.bin", &storage);
	if (sd_save_to_file(secmon, hdr->sm_size, path))
		goto out_free;
	gfx_puts(&gfx_con, "Secure Monitor dumped to secmon.bin\n");

	// Dump warmboot.
	emmcsn_path_impl(path, "/pkg1", "warmboot.bin", &storage);
	if (sd_save_to_file(warmboot, hdr->wb_size, path))
		goto out_free;
	gfx_puts(&gfx_con, "Warmboot dumped to warmboot.bin\n\n\n");

	// Dump package2.1.
	sdmmc_storage_set_mmc_partition(&storage, 0);
	// Parse eMMC GPT.
	LIST_INIT(gpt);
	nx_emmc_gpt_parse(&gpt, &storage);
	// Find package2 partition.
	emmc_part_t *pkg2_part = nx_emmc_part_find(&gpt, "BCPKG2-1-Normal-Main");
	if (!pkg2_part)
		goto out;

	// Read in package2 header and get package2 real size.
	u8 *tmp = (u8 *)malloc(NX_EMMC_BLOCKSIZE);
	nx_emmc_part_read(&storage, pkg2_part, 0x4000 / NX_EMMC_BLOCKSIZE, 1, tmp);
	u32 *hdr_pkg2_raw = (u32 *)(tmp + 0x100);
	u32 pkg2_size = hdr_pkg2_raw[0] ^ hdr_pkg2_raw[2] ^ hdr_pkg2_raw[3];
	free(tmp);
	// Read in package2.
	u32 pkg2_size_aligned = ALIGN(pkg2_size, NX_EMMC_BLOCKSIZE);
	pkg2 = malloc(pkg2_size_aligned);
	nx_emmc_part_read(&storage, pkg2_part, 0x4000 / NX_EMMC_BLOCKSIZE, 
		pkg2_size_aligned / NX_EMMC_BLOCKSIZE, pkg2);
	// Decrypt package2 and parse KIP1 blobs in INI1 section.
	pkg2_hdr_t *pkg2_hdr = pkg2_decrypt(pkg2);

	// Display info.
	u32 kernel_crc32 = crc32c(pkg2_hdr->data, pkg2_hdr->sec_size[PKG2_SEC_KERNEL]);
	gfx_printf(&gfx_con, "\n%kKernel CRC32C: %k0x%08X\n\n", 0xFFC7EA46, 0xFFCCCCCC, kernel_crc32);
	gfx_printf(&gfx_con, "%kKernel size:   %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, pkg2_hdr->sec_size[PKG2_SEC_KERNEL]);
	gfx_printf(&gfx_con, "%kINI1 size:     %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, pkg2_hdr->sec_size[PKG2_SEC_INI1]);

	// Dump pkg2.1.
	emmcsn_path_impl(path, "/pkg2", "pkg2_decr.bin", &storage);
	if (sd_save_to_file(pkg2, pkg2_hdr->sec_size[PKG2_SEC_KERNEL] + pkg2_hdr->sec_size[PKG2_SEC_INI1], path))
		goto out;
	gfx_puts(&gfx_con, "\nFull package2 dumped to pkg2_decr.bin\n");

	// Dump kernel.
	emmcsn_path_impl(path, "/pkg2", "kernel.bin", &storage);
	if (sd_save_to_file(pkg2_hdr->data, pkg2_hdr->sec_size[PKG2_SEC_KERNEL], path))
		goto out;
	gfx_puts(&gfx_con, "Kernel dumped to kernel.bin\n");

	// Dump INI1.
	emmcsn_path_impl(path, "/pkg2", "ini1.bin", &storage);
	if (sd_save_to_file(pkg2_hdr->data + pkg2_hdr->sec_size[PKG2_SEC_KERNEL],
		pkg2_hdr->sec_size[PKG2_SEC_INI1], path))
		goto out;
	gfx_puts(&gfx_con, "INI1 kip1 package dumped to ini1.bin\n");

	gfx_puts(&gfx_con, "\nDone. Press any key...\n");

out:
	nx_emmc_gpt_free(&gpt);
out_free:
	free(pkg1);
	free(secmon);
	free(warmboot);
	free(loader);
	free(pkg2);
	sdmmc_storage_end(&storage);
	sd_unmount();

	if (kb >= KB_FIRMWARE_VERSION_620)
		se_aes_key_clear(8);

	btn_wait();
}

void _toggle_autorcm(bool enable)
{
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	u8 randomXor = 0;

	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}

	u8 *tempbuf = (u8 *)malloc(0x200);
	sdmmc_storage_set_mmc_partition(&storage, 1);

	int i, sect = 0;
	for (i = 0; i < 4; i++)
	{
		sect = (0x200 + (0x4000 * i)) / NX_EMMC_BLOCKSIZE;
		sdmmc_storage_read(&storage, sect, 1, tempbuf);

		if (enable)
		{
			do
			{
				randomXor = get_tmr_us() & 0xFF; // Bricmii style of bricking.
			} while (!randomXor); // Avoid the lottery.

			tempbuf[0x10] ^= randomXor;
		}
		else
			tempbuf[0x10] = 0xF7;
		sdmmc_storage_write(&storage, sect, 1, tempbuf);
	}

	free(tempbuf);
	sdmmc_storage_end(&storage);

	if (enable)
		gfx_printf(&gfx_con, "%kAutoRCM mode enabled!%k", 0xFFFFBA00, 0xFFCCCCCC);
	else
		gfx_printf(&gfx_con, "%kAutoRCM mode disabled!%k", 0xFF96FF00, 0xFFCCCCCC);
	gfx_printf(&gfx_con, "\n\nPress any key...\n");

out:
	btn_wait();
}

void _enable_autorcm()  { _toggle_autorcm(true); }
void _disable_autorcm() { _toggle_autorcm(false); }

void menu_autorcm()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	// Do a simple check on the main BCT.
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	bool disabled = true;

	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		btn_wait();

		return;
	}

	u8 *tempbuf = (u8 *)malloc(0x200);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x200 / NX_EMMC_BLOCKSIZE, 1, tempbuf);

	if (tempbuf[0x10] != 0xF7)
		disabled = false;

	free(tempbuf);
	sdmmc_storage_end(&storage);

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
	menu_t menu = {ments, "This corrupts your BOOT0!", 0, 0};

	tui_do_menu(&gfx_con, &menu);
}

int _fix_attributes(char *path, u32 *total, u32 hos_folder, u32 check_first_run)
{
	FRESULT res;
	DIR dir;
	u32 dirLength = 0;
	static FILINFO fno;

	if (check_first_run)
	{
		// Read file attributes.
		res = f_stat(path, &fno);
		if (res != FR_OK)
			return res;

		// Check if archive bit is set.
		if (fno.fattrib & AM_ARC)
		{
			*(u32 *)total = *(u32 *)total + 1;
			f_chmod(path, 0, AM_ARC);
		}
	}

	// Open directory.
	res = f_opendir(&dir, path);
	if (res != FR_OK)
		return res;

	dirLength = strlen(path);
	for (;;)
	{
		// Clear file or folder path.
		path[dirLength] = 0;

		// Read a directory item.
		res = f_readdir(&dir, &fno);

		// Break on error or end of dir.
		if (res != FR_OK || fno.fname[0] == 0)
			break;

		// Skip official Nintendo dir if started from root.
		if (!hos_folder && !strcmp(fno.fname, "Nintendo"))
			continue;

		// Set new directory or file.
		memcpy(&path[dirLength], "/", 1);
		memcpy(&path[dirLength + 1], fno.fname, strlen(fno.fname) + 1);

		// Check if archive bit is set.
		if (fno.fattrib & AM_ARC)
		{
			*total = *total + 1;
			f_chmod(path, 0, AM_ARC);
		}

		// Is it a directory?
		if (fno.fattrib & AM_DIR)
		{
			// Set archive bit to NCA folders.
			if (hos_folder && !strcmp(fno.fname + strlen(fno.fname) - 4, ".nca"))
			{
				*total = *total + 1;
				f_chmod(path, AM_ARC, AM_ARC);
			}
			// Enter the directory.
			res = _fix_attributes(path, total, hos_folder, 0);
			if (res != FR_OK)
				break;
		}
	}

	f_closedir(&dir);

	return res;
}

void _fix_sd_attr(u32 type)
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	char path[256];
	char label[16];

	u32 total = 0;
	if (sd_mount())
	{
		switch (type)
		{
		case 0:
			memcpy(path, "/", 2);
			memcpy(label, "SD Card", 8);
			break;
		case 1:
		default:
			memcpy(path, "/Nintendo", 10);
			memcpy(label, "Nintendo folder", 16);
			break;
		}

		gfx_printf(&gfx_con, "Traversing all %s files!\nThis may take some time, please wait...\n\n", label);
		_fix_attributes(path, &total, type, type);
		gfx_printf(&gfx_con, "%kTotal archive bits cleared: %d!%k\n\nDone! Press any key...", 0xFF96FF00, total, 0xFFCCCCCC);
		sd_unmount();
	}
	btn_wait();
}

void fix_sd_all_attr() { _fix_sd_attr(0); }
void fix_sd_nin_attr() { _fix_sd_attr(1); }

void fix_battery_desync()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	max77620_low_battery_monitor_config();

	gfx_puts(&gfx_con, "\nDone!\n");

	btn_wait();
}

/* void fix_fuel_gauge_configuration()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	int battVoltage, avgCurrent;

	max17050_get_property(MAX17050_VCELL, &battVoltage);
	max17050_get_property(MAX17050_AvgCurrent, &avgCurrent);

	// Check if still charging. If not, check if battery is >= 95% (4.1V).
	if (avgCurrent < 0 && battVoltage > 4100)
	{
		if ((avgCurrent / 1000) < -10)
			EPRINTF("You need to be connected to a wall adapter,\nto apply this fix!");
		else
		{
			gfx_printf(&gfx_con, "%kAre you really sure?\nThis will reset your fuel gauge completely!\n", 0xFFFFDD00);
			gfx_printf(&gfx_con, "Additionally this will power off your console.\n%k", 0xFFCCCCCC);

			gfx_puts(&gfx_con, "\nPress POWER to Continue.\nPress VOL to go to the menu.\n\n\n");

			u32 btn = btn_wait();
			if (btn & BTN_POWER)
			{
				max17050_fix_configuration();
				msleep(1000);
				gfx_con_getpos(&gfx_con, &gfx_con.savedx,  &gfx_con.savedy);
				u16 value = 0;
				gfx_printf(&gfx_con, "%kThe console will power off in 45 seconds.\n%k", 0xFFFFDD00, 0xFFCCCCCC);
				while (value < 46)
				{
					gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);
					gfx_printf(&gfx_con, "%2ds elapsed", value);
					msleep(1000);
					value++;
				}
				msleep(2000);

				power_off();
			}
			return;
		}
	}
	else
		EPRINTF("You need a fully charged battery\nand connected to a wall adapter,\nto apply this fix!");

	msleep(500);
	btn_wait();
} */

/*void reset_pmic_fuel_gauge_charger_config()
{
	int avgCurrent;

	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, "%k\nThis will wipe your battery stats completely!\n"
		"%kAnd it may not power on without physically\nremoving and re-inserting the battery.\n%k"
		"\nAre you really sure?%k\n", 0xFFFFDD00, 0xFFFF0000, 0xFFFFDD00, 0xFFCCCCCC);

	gfx_puts(&gfx_con, "\nPress POWER to Continue.\nPress VOL to go to the menu.\n\n\n");
	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
		gfx_con_setpos(&gfx_con, 0, 0);
		gfx_printf(&gfx_con, "%kKeep the USB cable connected!%k\n\n", 0xFFFFDD00, 0xFFCCCCCC);
		gfx_con_getpos(&gfx_con, &gfx_con.savedx,  &gfx_con.savedy);

		u8 value = 30;
		while (value > 0)
		{
			gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);
			gfx_printf(&gfx_con, "%kWait... (%ds)   %k", 0xFF888888, value, 0xFFCCCCCC);
			msleep(1000);
			value--;
		}
		gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);

		//Check if still connected.
		max17050_get_property(MAX17050_AvgCurrent, &avgCurrent);
		if ((avgCurrent / 1000) < -10)
			EPRINTF("You need to be connected to a wall adapter\nor PC to apply this fix!");
		else
		{
			// Apply fix.
			bq24193_fake_battery_removal();
			gfx_printf(&gfx_con, "Done!               \n"
				"%k1. Remove the USB cable\n"
				"2. Press POWER for 15s.\n"
				"3. Reconnect the USB to power-on!%k\n", 0xFFFFDD00, 0xFFCCCCCC);
		}
		msleep(500);
		btn_wait();
	}
}*/

/*
//#include "../modules/hekate_libsys_minerva/mtc.h"
//mtc_config_t mtc_cfg;

void minerva()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	u32 curr_ram_idx = 0;

	if (!sd_mount())
		return;

	gfx_printf(&gfx_con, "-- Minerva Training Cell --\n\n");

	// Set table to ram.
	mtc_cfg.mtc_table = NULL;
	mtc_cfg.sdram_id = (fuse_read_odm(4) >> 3) & 0x1F;
	ianos_loader(false, "bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)&mtc_cfg);

	gfx_printf(&gfx_con, "\nStarting training process..\n\n");

	// Get current frequency
	for (curr_ram_idx = 0; curr_ram_idx < 10; curr_ram_idx++)
	{
		if (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) == mtc_cfg.mtc_table[curr_ram_idx].clk_src_emc)
			break;
	}

	mtc_cfg.rate_from = mtc_cfg.mtc_table[curr_ram_idx].rate_khz;
	mtc_cfg.rate_to = 800000;
	mtc_cfg.train_mode = OP_TRAIN_SWITCH;
	gfx_printf(&gfx_con, "Training and switching %7d -> %7d\n\n", mtc_cfg.mtc_table[curr_ram_idx].rate_khz, 800000);
	ianos_loader(false, "bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)&mtc_cfg);
	
	// Thefollowing frequency needs periodic training every 100ms.
	//msleep(200);
	
	//mtc_cfg.rate_to = 1600000;
	//gfx_printf(&gfx_con, "Training and switching  %7d -> %7d\n\n", mtc_cfg.current_emc_table->rate_khz, 1600000);
	//ianos_loader(false, "bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)&mtc_cfg);

	//mtc_cfg.train_mode = OP_PERIODIC_TRAIN;
	
	sd_unmount();
	
	gfx_printf(&gfx_con, "Finished!");

	btn_wait();
}
*/
