/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2021 CTCaer
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

#include "gui.h"
#include <display/di.h>
#include "../config.h"
#include "../hos/hos.h"
#include "../hos/pkg1.h"
#include "../hos/sept.h"
#include <libs/fatfs/ff.h>
#include <input/touch.h>
#include <mem/emc.h>
#include <mem/heap.h>
#include <mem/sdram.h>
#include <mem/smmu.h>
#include <power/bm92t36.h>
#include <power/bq24193.h>
#include <power/max17050.h>
#include <power/max77620.h>
#include <power/max7762x.h>
#include <power/max77812.h>
#include <sec/se.h>
#include <sec/tsec.h>
#include <soc/fuse.h>
#include <soc/kfuse.h>
#include <soc/i2c.h>
#include <soc/t210.h>
#include <storage/mmc.h>
#include "../storage/nx_emmc_bis.h"
#include <storage/nx_sd.h>
#include <storage/sdmmc.h>
#include <utils/btn.h>
#include <utils/sprintf.h>
#include <utils/util.h>

#define SECTORS_TO_MIB_COEFF 11

extern hekate_config h_cfg;
extern volatile boot_cfg_t *b_cfg;
extern volatile nyx_storage_t *nyx_str;

extern char *emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

static u8 *cal0_buf = NULL;

static lv_res_t _create_window_dump_done(int error, char *dump_filenames)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);

	char *txt_buf = (char *)malloc(0x1000);

	if (error)
		s_printf(txt_buf, "#FFDD00 Failed to dump to# %s#FFDD00 !#\nError: %d", dump_filenames, error);
	else
	{
		char *sn = emmcsn_path_impl(NULL, NULL, NULL, NULL);
		s_printf(txt_buf, "Dumping to SD card finished!\nFiles: #C7EA46 backup/%s/dumps/#\n%s", sn, dump_filenames);
	}
	lv_mbox_set_text(mbox, txt_buf);

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action); // Important. After set_text.

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _cal0_dump_window_action(lv_obj_t *btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	mbox_action(btns, txt);

	if (btn_idx == 1)
	{
		int error = !sd_mount();

		if (!error)
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "cal0.bin", NULL);
			error = sd_save_to_file((u8 *)cal0_buf, 0x8000, path);

			sd_unmount();
		}

		_create_window_dump_done(error, "cal0.bin");
	}

	return LV_RES_INV;
}


static lv_res_t _battery_dump_window_action(lv_obj_t * btn)
{
	int error = !sd_mount();

	if (!error)
	{
		char path[64];
		u8 *buf = (u8 *)malloc(0x100 * 2);

		// Unlock model table.
		u16 unlock = 0x59;
		i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_MODELEnable1, (u8 *)&unlock, 2);
		unlock = 0xC4;
		i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_MODELEnable2, (u8 *)&unlock, 2);

		// Dump all battery fuel gauge registers.
		for (int i = 0; i < 0x200; i += 2)
		{
			i2c_recv_buf_small(buf + i, 2, I2C_1, MAXIM17050_I2C_ADDR, i >> 1);
			msleep(1);
		}

		// Lock model table.
		unlock = 0;
		i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_MODELEnable1, (u8 *)&unlock, 2);
		i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_MODELEnable2, (u8 *)&unlock, 2);

		emmcsn_path_impl(path, "/dumps", "fuel_gauge.bin", NULL);
		error = sd_save_to_file((u8 *)buf, 0x200, path);

		sd_unmount();
	}

	_create_window_dump_done(error, "fuel_gauge.bin");

	return LV_RES_OK;
}

static lv_res_t _bootrom_dump_window_action(lv_obj_t * btn)
{
	static const u32 BOOTROM_SIZE = 0x18000;

	int error = !sd_mount();
	if (!error)
	{
		char path[64];
		u32 iram_evp_thunks[0x200];
		u32 iram_evp_thunks_len = sizeof(iram_evp_thunks);
		error = fuse_read_evp_thunk(iram_evp_thunks, &iram_evp_thunks_len);
		if (!error)
		{
			emmcsn_path_impl(path, "/dumps", "evp_thunks.bin", NULL);
			error = sd_save_to_file((u8 *)iram_evp_thunks, iram_evp_thunks_len, path);
		}
		else
			error = 255;

		emmcsn_path_impl(path, "/dumps", "bootrom_patched.bin", NULL);
		int res = sd_save_to_file((u8 *)BOOTROM_BASE, BOOTROM_SIZE, path);
		if (!error)
			error = res;

		u32 ipatch_backup[14];
		memcpy(ipatch_backup, (void *)IPATCH_BASE, sizeof(ipatch_backup));
		memset((void*)IPATCH_BASE, 0, sizeof(ipatch_backup));

		emmcsn_path_impl(path, "/dumps", "bootrom_unpatched.bin", NULL);
		res = sd_save_to_file((u8 *)BOOTROM_BASE, BOOTROM_SIZE, path);
		if (!error)
			error = res;

		memcpy((void*)IPATCH_BASE, ipatch_backup, sizeof(ipatch_backup));

		sd_unmount();
	}
	_create_window_dump_done(error, "evp_thunks.bin, bootrom_patched.bin, bootrom_unpatched.bin");

	return LV_RES_OK;
}

static lv_res_t _fuse_dump_window_action(lv_obj_t * btn)
{
	const u32 fuse_array_size_t210    = 192 * sizeof(u32);
	const u32 fuse_array_size_t210b01 = 256 * sizeof(u32);

	int error = !sd_mount();
	if (!error)
	{
		char path[128];
		if (!h_cfg.t210b01)
		{
			emmcsn_path_impl(path, "/dumps", "fuse_cached_t210.bin", NULL);
			error = sd_save_to_file((u8 *)0x7000F900, 0x300, path);
		}
		else
		{
			emmcsn_path_impl(path, "/dumps", "fuse_cached_t210b01_x898.bin", NULL);
			error = sd_save_to_file((u8 *)0x7000F898, 0x68, path);
			emmcsn_path_impl(path, "/dumps", "fuse_cached_t210b01_x900.bin", NULL);
			if (!error)
				error = sd_save_to_file((u8 *)0x7000F900, 0x300, path);
		}

		u32 words[fuse_array_size_t210b01 / sizeof(u32)];
		fuse_read_array(words);
		if (!h_cfg.t210b01)
			emmcsn_path_impl(path, "/dumps", "fuse_array_raw_t210.bin", NULL);
		else
			emmcsn_path_impl(path, "/dumps", "fuse_array_raw_t210b01.bin", NULL);
		int res = sd_save_to_file((u8 *)words, h_cfg.t210b01 ? fuse_array_size_t210b01 : fuse_array_size_t210, path);
		if (!error)
			error = res;

		sd_unmount();
	}

	if (!h_cfg.t210b01)
		_create_window_dump_done(error, "fuse_cached_t210.bin, fuse_array_raw_t210.bin");
	else
		_create_window_dump_done(error, "fuse_cached_t210b01_partX.bin, fuse_array_raw_t210b01.bin");

	return LV_RES_OK;
}

static lv_res_t _kfuse_dump_window_action(lv_obj_t * btn)
{
	u32 buf[KFUSE_NUM_WORDS];
	int error = !kfuse_read(buf);

	if (!error)
		error = !sd_mount();

	if (!error)
	{
		char path[64];
		emmcsn_path_impl(path, "/dumps", "kfuses.bin", NULL);
		error = sd_save_to_file((u8 *)buf, KFUSE_NUM_WORDS * 4, path);

		sd_unmount();
	}

	_create_window_dump_done(error, "kfuses.bin");

	return LV_RES_OK;
}

static u32 tsec_keys[8];

static lv_res_t _tsec_keys_dump_window_action(lv_obj_t * btn)
{
	int error = !sd_mount();
	if (!error)
	{
		char path[64];
		emmcsn_path_impl(path, "/dumps", "tsec_keys.bin", NULL);
		error = sd_save_to_file(tsec_keys, SE_KEY_128_SIZE * 2, path);

		sd_unmount();
	}
	_create_window_dump_done(error, "tsec_keys.bin");

	return LV_RES_OK;
}

static lv_res_t _create_mbox_cal0(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\211", "\222Dump", "\222Close", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);

	lv_mbox_set_text(mbox, "#C7EA46 CAL0 Info#");

	char *txt_buf = (char *)malloc(0x4000);
	txt_buf[0] = 0;

	lv_obj_t * lb_desc = lv_label_create(mbox, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_label_set_style(lb_desc, &monospace_text);
	lv_obj_set_width(lb_desc, LV_HOR_RES / 9 * 3);

	sd_mount();

	// Read package1.
	static const u32 BOOTLOADER_SIZE          = 0x40000;
	static const u32 BOOTLOADER_MAIN_OFFSET   = 0x100000;
	static const u32 BOOTLOADER_BACKUP_OFFSET = 0x140000;
	static const u32 HOS_KEYBLOBS_OFFSET      = 0x180000;

	u8 kb = 0;
	u32 bootloader_offset = BOOTLOADER_MAIN_OFFSET;
	u32 pk1_offset = h_cfg.t210b01 ? sizeof(bl_hdr_t210b01_t) : 0; // Skip T210B01 OEM header.
	u8 *pkg1 = (u8 *)malloc(BOOTLOADER_SIZE);

	if (!sdmmc_storage_init_mmc(&emmc_storage, &emmc_sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400))
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init eMMC!#");

		goto out;
	}
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_BOOT0);

try_load:
	sdmmc_storage_read(&emmc_storage, bootloader_offset / NX_EMMC_BLOCKSIZE, BOOTLOADER_SIZE / NX_EMMC_BLOCKSIZE, pkg1);

	char *build_date = malloc(32);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1 + pk1_offset, build_date);

	s_printf(txt_buf + strlen(txt_buf), "#00DDFF Found pkg1 ('%s')#\n", build_date);
	free(build_date);

	if (!pkg1_id)
	{
		strcat(txt_buf, "#FFDD00 Unknown pkg1 version!#\n");
		// Try backup bootloader.
		if (bootloader_offset != BOOTLOADER_BACKUP_OFFSET)
		{
			strcat(txt_buf, "Trying backup bootloader...\n");
			bootloader_offset = BOOTLOADER_BACKUP_OFFSET;
			goto try_load;
		}
		lv_label_set_text(lb_desc, txt_buf);

		goto out;
	}

	kb = pkg1_id->kb;

	// Skip if Mariko.
	if (h_cfg.t210b01)
		goto t210b01;

	tsec_ctxt_t tsec_ctxt;
	tsec_ctxt.fw = (u8 *)pkg1 + pkg1_id->tsec_off;
	tsec_ctxt.pkg1 = pkg1;
	tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;
	tsec_ctxt.secmon_base = pkg1_id->secmon_base;

	// Get keys.
	hos_eks_get();
	if (kb >= KB_FIRMWARE_VERSION_700 && !h_cfg.sept_run)
	{
		u32 key_idx = 0;
		if (kb >= KB_FIRMWARE_VERSION_810)
			key_idx = 1;

		if (h_cfg.eks && h_cfg.eks->enabled[key_idx] >= kb)
			h_cfg.sept_run = true;
		else
		{
			// Check that BCT is proper so sept can run.
			u8 *bct_bldr = (u8 *)calloc(1, 512);
			sdmmc_storage_read(&emmc_storage, 0x2200 / NX_EMMC_BLOCKSIZE, 1, bct_bldr);
			u32 bootloader_entrypoint = *(u32 *)&bct_bldr[0x144];
			free(bct_bldr);
			if (bootloader_entrypoint > SEPT_PRI_ENTRY)
			{
				lv_label_set_text(lb_desc, "#FFDD00 Main BCT is improper! Failed to run sept.#\n"
					"#FFDD00 Run sept with proper BCT at least once#\n#FFDD00 to cache keys.#\n");
				goto out;
			}

			// Set boot cfg.
			b_cfg->autoboot = 0;
			b_cfg->autoboot_list = 0;
			b_cfg->extra_cfg = EXTRA_CFG_NYX_SEPT;
			b_cfg->sept = NYX_SEPT_CAL0;

			if (!reboot_to_sept((u8 *)tsec_ctxt.fw, kb))
			{
				lv_label_set_text(lb_desc, "#FFDD00 Failed to run sept#\n");
				goto out;
			}
		}
	}

t210b01:;
	// Read the correct keyblob.
	u8 *keyblob = (u8 *)calloc(NX_EMMC_BLOCKSIZE, 1);
	sdmmc_storage_read(&emmc_storage, HOS_KEYBLOBS_OFFSET / NX_EMMC_BLOCKSIZE + kb, 1, keyblob);

	// Generate BIS keys
	hos_bis_keygen(keyblob, kb, &tsec_ctxt);

	free(keyblob);

	if (!cal0_buf)
		cal0_buf = malloc(0x10000);

	// Read and decrypt CAL0.
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);
	LIST_INIT(gpt);
	nx_emmc_gpt_parse(&gpt, &emmc_storage);
	emmc_part_t *cal0_part = nx_emmc_part_find(&gpt, "PRODINFO"); // check if null
	nx_emmc_bis_init(cal0_part, false, 0);
	nx_emmc_bis_read(0, 0x40, cal0_buf);
	nx_emmc_bis_end();
	nx_emmc_gpt_free(&gpt);

	// Clear BIS keys slots and reinstate SBK.
	hos_bis_keys_clear();

	nx_emmc_cal0_t *cal0 = (nx_emmc_cal0_t *)cal0_buf;

	// If successful, save BIS keys.
	if (memcmp(&cal0->magic, "CAL0", 4))
	{
		free(cal0_buf);
		cal0_buf = NULL;

		hos_eks_bis_clear();

		lv_label_set_text(lb_desc, "#FFDD00 CAL0 is corrupt or wrong keys!#\n");
		goto out;
	}
	else
		hos_eks_bis_save();

	u32 hash[8];
	se_calc_sha256_oneshot(hash, (u8 *)cal0 + 0x40, cal0->body_size);

	s_printf(txt_buf,
		"#FF8000 CAL0 Version:#      %d\n"
		"#FF8000 Update Count:#      %d\n"
		"#FF8000 Serial Number:#     %s\n"
		"#FF8000 WLAN MAC:#          %02X:%02X:%02X:%02X:%02X:%02X\n"
		"#FF8000 Bluetooth MAC:#     %02X:%02X:%02X:%02X:%02X:%02X\n"
		"#FF8000 Battery LOT:#       %s\n"
		"#FF8000 LCD Vendor:#        ",
		cal0->version, cal0->update_cnt, cal0->serial_number,
		cal0->wlan_mac[0], cal0->wlan_mac[1], cal0->wlan_mac[2], cal0->wlan_mac[3], cal0->wlan_mac[4], cal0->wlan_mac[5],
		cal0->bd_mac[0], cal0->bd_mac[1], cal0->bd_mac[2], cal0->bd_mac[3], cal0->bd_mac[4], cal0->bd_mac[5],
		cal0->battery_lot);

	u8  display_rev = (nyx_str->info.disp_id >> 8) & 0xFF;
	u32 display_id = (cal0->lcd_vendor & 0xFF) << 8 | (cal0->lcd_vendor & 0xFF0000) >> 16;
	switch (display_id)
	{
	case PANEL_JDI_LAM062M109A:
		strcat(txt_buf, "JDI LAM062M109A");
		break;
	case PANEL_JDI_LPM062M326A:
		strcat(txt_buf, "JDI LPM062M326A");
		break;
	case PANEL_INL_P062CCA_AZ1:
		strcat(txt_buf, "InnoLux P062CCA-AZ");
		switch (display_rev)
		{
		case 0x93:
			strcat(txt_buf, "1");
			break;
		case 0x95:
			strcat(txt_buf, "2");
			break;
		case 0x96:
			strcat(txt_buf, "3");
			break;
		default:
			strcat(txt_buf, "X");
			break;
		}
		break;
	case PANEL_AUO_A062TAN01:
		strcat(txt_buf, "AUO A062TAN0");
		switch (display_rev)
		{
		case 0x94:
			strcat(txt_buf, "1");
			break;
		case 0x95:
			strcat(txt_buf, "2");
			break;
		default:
			strcat(txt_buf, "X");
			break;
		}
		break;
	case PANEL_INL_2J055IA_27A:
		strcat(txt_buf, "InnoLux 2J055IA-27A");
		break;
	case PANEL_AUO_A055TAN01:
		strcat(txt_buf, "AUO A055TAN01");
		break;
	default:
		switch (cal0->lcd_vendor & 0xFF)
		{
		case 0:
		case PANEL_JDI_XXX062M:
			strcat(txt_buf, "JDI ");
			break;
		case (PANEL_INL_P062CCA_AZ1 & 0xFF):
			strcat(txt_buf, "InnoLux ");
			break;
		case (PANEL_AUO_A062TAN01 & 0xFF):
			strcat(txt_buf, "AUO ");
			break;
		}
		strcat(txt_buf, "Unknown");
		break;
	}

	bool valid_cal0 = !memcmp(hash, cal0->body_sha256, 0x20);
	s_printf(txt_buf + strlen(txt_buf), " (%06X)\n#FF8000 SHA256 Hash Match:# %s", cal0->lcd_vendor, valid_cal0 ? "Pass" : "Failed");

	lv_label_set_text(lb_desc, txt_buf);

out:
	free(pkg1);
	free(txt_buf);
	sd_unmount();
	sdmmc_storage_end(&emmc_storage);

	lv_mbox_add_btns(mbox, mbox_btn_map, _cal0_dump_window_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _create_window_fuses_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_CHIP" HW & Cached Fuses Info");
	lv_win_add_btn(win, NULL, SYMBOL_DOWNLOAD" Dump fuses", _fuse_dump_window_action);
	lv_win_add_btn(win, NULL, SYMBOL_INFO" CAL0 Info", _create_mbox_cal0);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 5 * 2, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_label_set_style(lb_desc, &monospace_text);

	lv_label_set_static_text(lb_desc,
		"SKU:\n"
		"DRAM ID:\n"
		"#FF8000 Burnt Fuses (ODM 7/6):#\n"
		"ODM Fields (4, 6, 7):\n"
		"Secure Boot key (SBK):\n"
		"Device key (DK):\n"
		"USB Stack:\n"
		"Final Test Revision:\n"
		"Chip Probing Revision:\n"
		"Bootrom ipatches size:\n"
		"CPU Speedo 0 (CPU Val):\n"
		"CPU Speedo 1:\n"
		"CPU Speedo 2 (GPU Val):\n"
		"SoC Speedo 0 (SoC Val):\n"
		"SoC Speedo 1 (BROM rev):\n"
		"SoC Speedo 2:\n"
		"CPU IDDQ Val:\n"
		"SoC IDDQ Val:\n"
		"Gpu IDDQ Val:\n"
		"Vendor Code:\n"
		"FAB Code:\n"
		"LOT Code 0:\n"
		"LOT Code 1:\n"
		"Wafer ID:\n"
		"X Coordinate:\n"
		"Y Coordinate:\n"
		"#FF8000 Chip ID Revision:#"
	);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	lv_obj_t *val = lv_cont_create(win, NULL);
	lv_obj_set_size(val, LV_HOR_RES / 11 * 3, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_val = lv_label_create(val, lb_desc);

	char *txt_buf = (char *)malloc(0x4000);

	// Decode fuses.
	char *sku;
	char dram_man[32];
	char fuses_hos_version[64];
	u8 dram_id = fuse_read_dramid(true);

	switch (fuse_read_hw_type())
	{
	case FUSE_NX_HW_TYPE_ICOSA:
		sku = "Icosa (Erista)";
		break;
	case FUSE_NX_HW_TYPE_IOWA:
		sku = "Iowa (Mariko)";
		break;
	case FUSE_NX_HW_TYPE_HOAG:
		sku = "Hoag (Mariko)";
		break;
	default:
		sku = "#FF8000 Unknown#";
		break;
	}

	switch (dram_id)
	{
	// LPDDR4 3200Mbps.
	case LPDDR4_ICOSA_4GB_SAMSUNG_K4F6E304HB_MGCH:
	case LPDDR4_COPPER_4GB_SAMSUNG_K4F6E304HB_MGCH:
		strcpy(dram_man, "Samsung K4F6E304HB-MGCH 4GB");
		break;
	case LPDDR4_ICOSA_4GB_HYNIX_H9HCNNNBPUMLHR_NLE:
	case LPDDR4_COPPER_4GB_HYNIX_H9HCNNNBPUMLHR_NLE:
		strcpy(dram_man, "Hynix H9HCNNNBPUMLHR-NLE 4GB");
		break;
	case LPDDR4_ICOSA_4GB_MICRON_MT53B512M32D2NP_062_WT:
	case LPDDR4_COPPER_4GB_MICRON_MT53B512M32D2NP_062_WT:
		strcpy(dram_man, "Micron MT53B512M32D2NP-062");
		break;
	case LPDDR4_ICOSA_6GB_SAMSUNG_K4FHE3D4HM_MGCH:
		strcpy(dram_man, "Samsung K4FHE3D4HM-MGCH 6GB");
		break;

	// LPDDR4X 3733Mbps.
	case LPDDR4X_IOWA_4GB_SAMSUNG_X1X2:
		strcpy(dram_man, "Samsung X1X2 4GB");
		break;
	case LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AM_MGCJ:
	case LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AM_MGCJ:
		strcpy(dram_man, "Samsung K4U6E3S4AM-MGCJ 4GB");
		break;
	case LPDDR4X_IOWA_8GB_SAMSUNG_K4UBE3D4AM_MGCJ:
	case LPDDR4X_HOAG_8GB_SAMSUNG_K4UBE3D4AM_MGCJ:
		strcpy(dram_man, "Samsung K4UBE3D4AM-MGCJ 8GB");
		break;
	case LPDDR4X_IOWA_4GB_HYNIX_H9HCNNNBKMMLHR_NME:
	case LPDDR4X_HOAG_4GB_HYNIX_H9HCNNNBKMMLHR_NME:
		strcpy(dram_man, "Hynix H9HCNNNBKMMLHR-NME 4GB");
		break;

	case LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D2NP_046_WT: // 4266Mbps.
	case LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WT: // 4266Mbps.
		strcpy(dram_man, "Micron MT53E512M32D2NP-046 4GB");
		break;
	// LPDDR4X 4266Mbps?
	case LPDDR4X_IOWA_4GB_SAMSUNG_Y:
		strcpy(dram_man, "Samsung Y 4GB");
		break;
	case LPDDR4X_IOWA_4GB_SAMSUNG_1Y_X:
	case LPDDR4X_HOAG_4GB_SAMSUNG_1Y_X:
	case LPDDR4X_AULA_4GB_SAMSUNG_1Y_X:
		strcpy(dram_man, "Samsung 1y X 4GB");
		break;
	case LPDDR4X_IOWA_8GB_SAMSUNG_1Y_X:
	case LPDDR4X_AULA_8GB_SAMSUNG_1Y_X:
		strcpy(dram_man, "Samsung 1y X 8GB");
		break;
	case LPDDR4X_IOWA_4GB_SAMSUNG_1Y_Y:
		strcpy(dram_man, "Samsung 1y Y 4GB");
		break;
	case LPDDR4X_IOWA_8GB_SAMSUNG_1Y_Y:
		strcpy(dram_man, "Samsung 1y Y 8GB");
		break;
	case LPDDR4X_AULA_4GB_SAMSUNG_1Y_A:
		strcpy(dram_man, "Samsung 1y A 4GB");
		break;
	case LPDDR4X_IOWA_4GB_MICRON_1Y_A:
	case LPDDR4X_HOAG_4GB_MICRON_1Y_A:
	case LPDDR4X_AULA_4GB_MICRON_1Y_A:
		strcpy(dram_man, "Micron 1y A 4GB");
		break;
	default:
		strcpy(dram_man, "#FF8000 Unknown#");
		break;
	}

	// Count burnt fuses.
	u8 burnt_fuses_7 = fuse_count_burnt(fuse_read_odm(7));
	u8 burnt_fuses_6 = fuse_count_burnt(fuse_read_odm(6));

	switch (burnt_fuses_7)
	{
	case 1:
		strcpy(fuses_hos_version, "1.0.0");
		break;
	case 2:
		strcpy(fuses_hos_version, "2.0.0 - 2.3.0");
		break;
	case 3:
		strcpy(fuses_hos_version, "3.0.0");
		break;
	case 4:
		strcpy(fuses_hos_version, "3.0.1 - 3.0.2");
		break;
	case 5:
		strcpy(fuses_hos_version, "4.0.0 - 4.1.0");
		break;
	case 6:
		strcpy(fuses_hos_version, "5.0.0 - 5.1.0");
		break;
	case 7:
		strcpy(fuses_hos_version, "6.0.0 - 6.1.0");
		break;
	case 8:
		strcpy(fuses_hos_version, "6.2.0");
		break;
	case 9:
		strcpy(fuses_hos_version, "7.0.0 - 8.0.1");
		break;
	case 10:
		strcpy(fuses_hos_version, "8.1.0 - 8.1.1");
		break;
	case 11:
		strcpy(fuses_hos_version, "9.0.0 - 9.0.1");
		break;
	case 12:
		strcpy(fuses_hos_version, "9.1.0 - 9.2.0");
		break;
	case 13:
		strcpy(fuses_hos_version, "10.0.0 - 10.2.0");
		break;
	case 14:
		strcpy(fuses_hos_version, "11.0.0+");
		break;
	default:
		strcpy(fuses_hos_version, "#FF8000 Unknown#");
		break;
	}

	// Calculate LOT.
	u32 lot_code0 = (FUSE(FUSE_OPT_LOT_CODE_0) & 0xFFFFFFF) << 2;
	u32 lot_bin = 0;
	for (int i = 0; i < 5; ++i)
	{
		u32 digit = (lot_code0 & 0xFC000000) >> 26;
		lot_bin *= 36;
		lot_bin += digit;
		lot_code0 <<= 6;
	}

	u32 chip_id = APB_MISC(APB_MISC_GP_HIDREV);
	// Parse fuses and display them.
	s_printf(txt_buf,
		"%X - %s - %s\n%02d: %s\n%d - %d (HOS: %s)\n%08X %08X %08X\n%08X%08X%08X%08X\n%08X\n"
		"%s\n%d.%02d (0x%X)\n%d.%02d (0x%X)\n%d\n%d\n%d\n%d\n%d\n0x%X\n%d\n%d\n%d\n%d\n"
		"%d\n%d\n%d (0x%X)\n%d\n%d\n%d\n%d\n"
		"ID: %02X, Major: A0%d, Minor: %d",
		FUSE(FUSE_SKU_INFO), sku, fuse_read_hw_state() ? "Dev" : "Retail",
		dram_id, dram_man, burnt_fuses_7, burnt_fuses_6, fuses_hos_version,
		fuse_read_odm(4), fuse_read_odm(6), fuse_read_odm(7),
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY0)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY1)),
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY2)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY3)),
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY4)),
		((FUSE(FUSE_RESERVED_SW) & 0x80) || h_cfg.t210b01) ? "XUSB" : "USB2",
		(FUSE(FUSE_OPT_FT_REV)  >> 5) & 0x3F, FUSE(FUSE_OPT_FT_REV) & 0x1F, FUSE(FUSE_OPT_FT_REV),
		(FUSE(FUSE_OPT_CP_REV)  >> 5) & 0x3F, FUSE(FUSE_OPT_CP_REV) & 0x1F, FUSE(FUSE_OPT_CP_REV),
		FUSE(FUSE_FIRST_BOOTROM_PATCH_SIZE) & 0x7F,
		FUSE(FUSE_CPU_SPEEDO_0_CALIB), FUSE(FUSE_CPU_SPEEDO_1_CALIB), FUSE(FUSE_CPU_SPEEDO_2_CALIB),
		FUSE(FUSE_SOC_SPEEDO_0_CALIB), FUSE(FUSE_SOC_SPEEDO_1_CALIB), FUSE(FUSE_SOC_SPEEDO_2_CALIB),
		FUSE(FUSE_CPU_IDDQ_CALIB), FUSE(FUSE_SOC_IDDQ_CALIB), FUSE(FUSE_GPU_IDDQ_CALIB),
		FUSE(FUSE_OPT_VENDOR_CODE), FUSE(FUSE_OPT_FAB_CODE), lot_bin, FUSE(FUSE_OPT_LOT_CODE_0),
		FUSE(FUSE_OPT_LOT_CODE_1), FUSE(FUSE_OPT_WAFER_ID), FUSE(FUSE_OPT_X_COORDINATE), FUSE(FUSE_OPT_Y_COORDINATE),
		(chip_id >> 8) & 0xFF, (chip_id >> 4) & 0xF, (chip_id >> 16) & 0xF);

	lv_label_set_text(lb_val, txt_buf);

	lv_obj_set_width(lb_val, lv_obj_get_width(val));
	lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_t *desc2 = lv_cont_create(win, NULL);
	lv_obj_set_size(desc2, LV_HOR_RES / 2 / 5 * 4, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc2 = lv_label_create(desc2, NULL);
	lv_label_set_long_mode(lb_desc2, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc2, true);

	// DRAM info.
	emc_mr_data_t ram_vendor = sdram_read_mrx(MR5_MAN_ID);
	emc_mr_data_t ram_rev0 = sdram_read_mrx(MR6_REV_ID1);
	emc_mr_data_t ram_rev1 = sdram_read_mrx(MR7_REV_ID2);
	emc_mr_data_t ram_density = sdram_read_mrx(MR8_DENSITY);
	u32 ranks = EMC(EMC_ADR_CFG) + 1;
	u32 channels = (EMC(EMC_FBIO_CFG7) >> 1) & 3;
	u32 die_channels = ranks * ((channels & 1) + ((channels & 2) >> 1));
	s_printf(txt_buf, "#00DDFF %s SDRAM ##FF8000 (Ch 0 | Ch 1):#\n#FF8000 Vendor:# ", dram_id > 6 ? "LPDDR4X" : "LPDDR4");
	switch (ram_vendor.rank0_ch0)
	{
	case 1:
		strcat(txt_buf, "Samsung");
		break;
	case 6:
		strcat(txt_buf, "Hynix");
		break;
	case 255:
		strcat(txt_buf, "Micron");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "#FF8000 Unknown# (%d)", ram_vendor.rank0_ch0);
		break;
	}
	strcat(txt_buf, " #FF8000 |# ");
	switch (ram_vendor.rank0_ch1)
	{
	case 1:
		strcat(txt_buf, "Samsung");
		break;
	case 6:
		strcat(txt_buf, "Hynix");
		break;
	case 255:
		strcat(txt_buf, "Micron");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "#FF8000 Unknown# (%d)", ram_vendor.rank0_ch1);
		break;
	}
	s_printf(txt_buf + strlen(txt_buf), "\n#FF8000 Rev ID:#  %X.%02X #FF8000 |# %X.%02X\n#FF8000 Density:# %d",
		ram_rev0.rank0_ch0, ram_rev1.rank0_ch0, ram_rev0.rank0_ch1, ram_rev1.rank0_ch1, die_channels);
	switch ((ram_density.rank0_ch0 & 0x3C) >> 2)
	{
	case 2:
		strcat(txt_buf, " x 512MB");
		break;
	case 3:
		strcat(txt_buf, " x 768MB");
		break;
	case 4:
		strcat(txt_buf, " x 1GB");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), " x Unk (%d)", (ram_density.rank0_ch0 & 0x3C) >> 2);
		break;
	}
	s_printf(txt_buf + strlen(txt_buf), " #FF8000 |# %d", die_channels);
	switch ((ram_density.rank0_ch1 & 0x3C) >> 2)
	{
	case 2:
		strcat(txt_buf, " x 512MB");
		break;
	case 3:
		strcat(txt_buf, " x 768MB");
		break;
	case 4:
		strcat(txt_buf, " x 1GB");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), " x Unk (%d)", (ram_density.rank0_ch1 & 0x3C) >> 2);
		break;
	}
	strcat(txt_buf, "\n\n");

	// Display info.
	u8  display_rev = (nyx_str->info.disp_id >> 8) & 0xFF;
	u32 display_id = ((nyx_str->info.disp_id >> 8) & 0xFF00) | (nyx_str->info.disp_id & 0xFF);

	strcat(txt_buf, "#00DDFF Display Panel:#\n#FF8000 Model:# ");

	switch (display_id)
	{
	case PANEL_JDI_LAM062M109A:
		strcat(txt_buf, "JDI LAM062M109A");
		break;
	case PANEL_JDI_LPM062M326A:
		strcat(txt_buf, "JDI LPM062M326A");
		break;
	case PANEL_INL_P062CCA_AZ1:
		strcat(txt_buf, "InnoLux P062CCA");
		switch (display_rev)
		{
		case 0x93:
			strcat(txt_buf, "-AZ1");
			break;
		case 0x95:
			strcat(txt_buf, "-AZ2");
			break;
		case 0x96:
			strcat(txt_buf, "-AZ3");
			break;
		default:
			strcat(txt_buf, " #FFDD00 Contact me!#");
			break;
		}
		break;
	case PANEL_AUO_A062TAN01:
		strcat(txt_buf, "AUO A062");
		switch (display_rev)
		{
		case 0x94:
			strcat(txt_buf, "TAN01");
			break;
		case 0x95:
			strcat(txt_buf, "TAN02");
			break;
		default:
			strcat(txt_buf, " #FFDD00 Contact me!#");
			break;
		}
		break;
	case PANEL_INL_2J055IA_27A:
		strcat(txt_buf, "InnoLux 2J055IA-27A");
		break;
	case PANEL_AUO_A055TAN01:
		strcat(txt_buf, "AUO A055TAN01");
		break;
	default:
		switch (display_id & 0xFF)
		{
		case PANEL_JDI_XXX062M:
			strcat(txt_buf, "JDI ");
			break;
		case (PANEL_INL_P062CCA_AZ1 & 0xFF):
			strcat(txt_buf, "InnoLux ");
			break;
		case (PANEL_AUO_A062TAN01 & 0xFF):
			strcat(txt_buf, "AUO ");
			break;
		}
		strcat(txt_buf, "Unknown #FFDD00 Contact me!#");
		break;
	}

	s_printf(txt_buf + strlen(txt_buf), "\n#FF8000 ID:# [%02X] %02X [%02X]",
		nyx_str->info.disp_id & 0xFF, (nyx_str->info.disp_id >> 8) & 0xFF, (nyx_str->info.disp_id >> 16) & 0xFF);

	touch_fw_info_t touch_fw;
	touch_panel_info_t *touch_panel;
	bool panel_ic_paired = false;

	if (!touch_get_fw_info(&touch_fw))
	{
		strcat(txt_buf, "\n\n#00DDFF Touch Panel:#\n#FF8000 Model:# ");

		touch_panel = touch_get_panel_vendor();
		if (touch_panel)
			strcat(txt_buf, touch_panel->vendor);
		else
			strcat(txt_buf, "Unknown #FFDD00 Contact me!#");

		s_printf(txt_buf + strlen(txt_buf), "\n#FF8000 ID:# %08X (", touch_fw.fw_id);

		switch (touch_fw.fw_id)
		{
		case 0x00100100:
			strcat(txt_buf, "4CD 1601");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == -1;
			break;
		case 0x00120100:
		case 0x32000001:
			strcat(txt_buf, "4CD 1801");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 0;
			break;
		case 0x001A0300:
		case 0x32000102:
			strcat(txt_buf, "4CD 2602");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 1;
			break;
		case 0x00290100:
		case 0x32000302:
			strcat(txt_buf, "4CD 3801");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 2;
			break;
		case 0x31051820:
		case 0x32000402:
			strcat(txt_buf, "4CD XXXX");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 3;
			break;
		case 0x32000501:
		case 0x33000502:
			strcat(txt_buf, "4CD UNKN");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 4;
			break;
		default:
			strcat(txt_buf, "#FF8000 Unknown#");
			break;
		}

		s_printf(txt_buf + strlen(txt_buf), " - %s)\n#FF8000 FTB ver:# %04X\n#FF8000 FW rev:# %04X",
			panel_ic_paired ? "Paired" : "#FFDD00 Error#", touch_fw.ftb_ver, touch_fw.fw_rev);
	}

	// Check if patched unit.
	if (!fuse_check_patched_rcm())
		strcat(txt_buf, "\n\n#96FF00 This unit is exploitable#\n#96FF00 to the RCM bug!#");
	else
		strcat(txt_buf, "\n\n#FF8000 This unit is patched#\n#FF8000 to the RCM bug!#");

	lv_label_set_text(lb_desc2, txt_buf);

	free(txt_buf);

	lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
	lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);

	if (!btn)
		_create_mbox_cal0(NULL);

	return LV_RES_OK;
}

void sept_run_cal0(void *param)
{
	_create_window_fuses_info_status(NULL);
}

static char *ipatches_txt;
static void _ipatch_process(u32 offset, u32 value)
{
	s_printf(ipatches_txt + strlen(ipatches_txt), "%6X     %4X    ", BOOTROM_BASE + offset, value);
	u8 lo = value & 0xFF;
	switch (value >> 8)
	{
	case 0x20:
		s_printf(ipatches_txt + strlen(ipatches_txt), "MOVS R0, ##0x%02X", lo);
		break;
	case 0xDF:
		s_printf(ipatches_txt + strlen(ipatches_txt), "SVC ##0x%02X", lo);
		break;
	}
	strcat(ipatches_txt, "\n");
}

static lv_res_t _create_window_bootrom_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_CHIP" Bootrom Info");
	lv_win_add_btn(win, NULL, SYMBOL_DOWNLOAD" Dump Bootrom", _bootrom_dump_window_action);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 3 * 2, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_label_set_style(lb_desc, &monospace_text);

	char *txt_buf = (char *)malloc(0x1000);
	ipatches_txt = txt_buf;
	s_printf(txt_buf, "#00DDFF Ipatches:#\n#FF8000 Address  "SYMBOL_DOT"  Val  "SYMBOL_DOT"  Instruction#\n");

	u32 res = fuse_read_ipatch(_ipatch_process);
	if (res != 0)
		s_printf(txt_buf + strlen(txt_buf), "#FFDD00 Failed to read ipatches. Error: %d#", res);

	lv_label_set_text(lb_desc, txt_buf);

	free(txt_buf);

	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	return LV_RES_OK;
}

static lv_res_t _create_window_tsec_keys_status(lv_obj_t *btn)
{
	u32 retries = 0;

	tsec_ctxt_t tsec_ctxt;

	lv_obj_t *win = nyx_create_standard_window(SYMBOL_CHIP" TSEC Keys");

	//Disable buttons.
	nyx_window_toggle_buttons(win, true);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 2, LV_VER_RES - (LV_DPI * 11 / 6));
	//lv_obj_align(desc, win, LV_ALIGN_ou_TOP_LEFT, 0, 40);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_label_set_style(lb_desc, &monospace_text);

	char *txt_buf  = (char *)malloc(0x1000);
	char *txt_buf2 = (char *)malloc(0x1000);
	txt_buf[0] = 0;

	// Read package1.
	static const u32 BOOTLOADER_SIZE          = 0x40000;
	static const u32 BOOTLOADER_MAIN_OFFSET   = 0x100000;
	static const u32 BOOTLOADER_BACKUP_OFFSET = 0x140000;

	u8 *pkg1 = (u8 *)malloc(0x40000);
	u32 bootloader_offset = BOOTLOADER_MAIN_OFFSET;

try_load:
	sdmmc_storage_init_mmc(&emmc_storage, &emmc_sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400);
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_BOOT0);
	sdmmc_storage_read(&emmc_storage, bootloader_offset / NX_EMMC_BLOCKSIZE, BOOTLOADER_SIZE / NX_EMMC_BLOCKSIZE, pkg1);
	sdmmc_storage_end(&emmc_storage);

	char *build_date = malloc(32);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1, build_date);

	s_printf(txt_buf + strlen(txt_buf), "#00DDFF Found pkg1 ('%s')#\n", build_date);
	free(build_date);

	if (!pkg1_id)
	{
		strcat(txt_buf, "#FFDD00 Unknown pkg1 version!#\n");
		// Try backup bootloader.
		if (bootloader_offset != BOOTLOADER_BACKUP_OFFSET)
		{
			strcat(txt_buf, "Trying backup bootloader...\n");
			bootloader_offset = BOOTLOADER_BACKUP_OFFSET;
			goto try_load;
		}
		lv_label_set_text(lb_desc, txt_buf);
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

		goto out;
	}
	lv_label_set_text(lb_desc, txt_buf);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	lv_obj_t *val = lv_cont_create(win, NULL);
	lv_obj_set_size(val, LV_HOR_RES / 11 * 3, LV_VER_RES - (LV_DPI * 11 / 6));

	lv_obj_t * lb_val = lv_label_create(val, lb_desc);
	lv_label_set_style(lb_val, &monospace_text);

	lv_label_set_text(lb_val, "Please wait...");
	lv_obj_set_width(lb_val, lv_obj_get_width(val));
	lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	manual_system_maintenance(true);

	tsec_ctxt.fw = (u8 *)pkg1 + pkg1_id->tsec_off;
	tsec_ctxt.pkg1 = pkg1;
	tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;
	tsec_ctxt.secmon_base = pkg1_id->secmon_base;

	if (pkg1_id->kb <= KB_FIRMWARE_VERSION_600)
		tsec_ctxt.size = 0xF00;
	else if (pkg1_id->kb == KB_FIRMWARE_VERSION_620)
		tsec_ctxt.size = 0x2900;
	else if (pkg1_id->kb == KB_FIRMWARE_VERSION_700)
	{
		tsec_ctxt.size = 0x3000;
		// Exit after TSEC key generation.
		*((vu16 *)((u32)tsec_ctxt.fw + 0x2DB5)) = 0x02F8;
	}
	else
		tsec_ctxt.size = 0x3300;

	if (pkg1_id->kb == KB_FIRMWARE_VERSION_620)
	{
		u8 *tsec_paged = (u8 *)page_alloc(3);
		memcpy(tsec_paged, (void *)tsec_ctxt.fw, tsec_ctxt.size);
		tsec_ctxt.fw = tsec_paged;
	}

	int res = 0;

	while (tsec_query((u8 *)tsec_keys, pkg1_id->kb, &tsec_ctxt) < 0)
	{
		memset(tsec_keys, 0x00, 0x20);

		retries++;

		if (retries > 3)
		{
			res = -1;
			break;
		}
	}

	strcat(txt_buf, "#C7EA46 TSEC Key:#\n");
	if (res >= 0)
	{
		s_printf(txt_buf2, "\n%08X%08X%08X%08X\n",
			byte_swap_32(tsec_keys[0]), byte_swap_32(tsec_keys[1]), byte_swap_32(tsec_keys[2]), byte_swap_32(tsec_keys[3]));

		if (pkg1_id->kb == KB_FIRMWARE_VERSION_620)
		{
			strcat(txt_buf, "#C7EA46 TSEC root:#\n");
			s_printf(txt_buf2 + strlen(txt_buf2), "%08X%08X%08X%08X\n",
				byte_swap_32(tsec_keys[4]), byte_swap_32(tsec_keys[5]), byte_swap_32(tsec_keys[6]), byte_swap_32(tsec_keys[7]));
		}
		lv_win_add_btn(win, NULL, SYMBOL_DOWNLOAD" Dump Keys", _tsec_keys_dump_window_action);
	}
	else
	{
		s_printf(txt_buf2, "Error: %x", res);
	}

	lv_label_set_text(lb_desc, txt_buf);

	lv_label_set_text(lb_val, txt_buf2);

out:
	free(pkg1);
	free(txt_buf);
	free(txt_buf2);

	nyx_window_toggle_buttons(win, false);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_benchmark(bool sd_bench)
{
	sdmmc_storage_t *storage;

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 7 * 4);

	char *txt_buf = (char *)malloc(0x4000);

	s_printf(txt_buf, "#FF8000 %s Benchmark#\n[Raw Reads] Abort: VOL- & VOL+",
		sd_bench ? "SD Card" : "eMMC");

	lv_mbox_set_text(mbox, txt_buf);
	txt_buf[0] = 0;

	lv_obj_t *h1 = lv_cont_create(mbox, NULL);
	lv_cont_set_fit(h1, false, true);
	lv_cont_set_style(h1, &lv_style_transp_tight);
	lv_obj_set_width(h1, lv_obj_get_width(mbox) - LV_DPI / 10);

	lv_obj_t *lbl_status = lv_label_create(h1, NULL);
	lv_label_set_style(lbl_status, &monospace_text);
	lv_label_set_recolor(lbl_status, true);
	lv_label_set_text(lbl_status, " ");
	lv_obj_align(lbl_status, h1, LV_ALIGN_IN_TOP_MID, 0, 0);

	lv_obj_t *bar = lv_bar_create(mbox, NULL);
	lv_obj_set_size(bar, LV_DPI * 2, LV_DPI / 5);
	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_value(bar, 0);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);
	manual_system_maintenance(true);

	int res = 0;

	if (sd_bench)
	{
		storage = &sd_storage;
		res = !sd_mount();
	}
	else
	{
		storage = &emmc_storage;
		res = !sdmmc_storage_init_mmc(&emmc_storage, &emmc_sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400);
		if (!res)
			sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);
	}

	if (res)
		lv_mbox_set_text(mbox, "#FFDD00 Failed to init Storage!#");
	else
	{
		u32 iters = 3;
		u32 offset_chunk_start = ALIGN_DOWN(storage->sec_cnt / 3, 0x8000); // Align to 16MB.
		if (storage->sec_cnt < 0xC00000)
			iters -= 2; // 4GB card.

		for (u32 iter_curr = 0; iter_curr < iters; iter_curr++)
		{
			u32 pct = 0;
			u32 prevPct = 200;
			u32 timer = 0;
			u32 lba_curr = 0;
			u32 sector = offset_chunk_start * iter_curr;
			u32 sector_num = 0x8000;       // 16MB chunks.
			u32 data_remaining = 0x200000; // 1GB.

			s_printf(txt_buf + strlen(txt_buf), "#C7EA46 %d/3# - Sector Offset #C7EA46 %08X#:\n", iter_curr + 1, sector);

			while (data_remaining)
			{
				u32 time_taken = get_tmr_us();
				sdmmc_storage_read(storage, sector + lba_curr, sector_num, (u8 *)MIXD_BUF_ALIGNED);
				time_taken = get_tmr_us() - time_taken;
				timer += time_taken;

				manual_system_maintenance(false);
				data_remaining -= sector_num;
				lba_curr += sector_num;

				pct = (lba_curr * 100) / 0x200000;
				if (pct != prevPct)
				{
					lv_bar_set_value(bar, pct);
					manual_system_maintenance(true);

					prevPct = pct;

					if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
						break;
				}
			}
			lv_bar_set_value(bar, 100);

			u32 rate_1k = ((u64)1024 * 1000 * 1000 * 1000) / timer;
			s_printf(txt_buf + strlen(txt_buf),
				" Sequential 16MiB - Rate: #C7EA46 %3d.%02d MiB/s#\n",
				rate_1k / 1000, (rate_1k % 1000) / 10);
			lv_label_set_text(lbl_status, txt_buf);
			lv_obj_align(lbl_status, NULL, LV_ALIGN_CENTER, 0, 0);
			lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
			manual_system_maintenance(true);

			pct = 0;
			prevPct = 200;
			timer = 0;
			lba_curr = 0;
			sector_num = 8;            // 4KB chunks.
			data_remaining = 0x100000; // 512MB.

			while (data_remaining)
			{
				u32 time_taken = get_tmr_us();
				sdmmc_storage_read(storage, sector + lba_curr, sector_num, (u8 *)MIXD_BUF_ALIGNED);
				time_taken = get_tmr_us() - time_taken;
				timer += time_taken;

				manual_system_maintenance(false);
				data_remaining -= sector_num;
				lba_curr += sector_num;

				pct = (lba_curr * 100) / 0x100000;
				if (pct != prevPct)
				{
					lv_bar_set_value(bar, pct);
					manual_system_maintenance(true);

					prevPct = pct;

					if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
						break;
				}
			}
			lv_bar_set_value(bar, 100);

			rate_1k = ((u64)512 * 1000 * 1000 * 1000) / timer;
			u32 iops_1k = ((u64)512 * 1024 * 1000 * 1000 * 1000) / (4096 / 1024) / timer / 1000;
			s_printf(txt_buf + strlen(txt_buf),
				" Sequential  4KiB - Rate: #C7EA46 %3d.%02d MiB/s#, IOPS: #C7EA46 %4d#\n",
				rate_1k / 1000, (rate_1k % 1000) / 10, iops_1k);
			lv_label_set_text(lbl_status, txt_buf);
			lv_obj_align(lbl_status, NULL, LV_ALIGN_CENTER, 0, 0);
			lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
			manual_system_maintenance(true);

			u32 lba_idx = 0;
			u32 *random_offsets = malloc(0x20000 * sizeof(u32));
			u32  random_numbers[4];
			for (u32 i = 0; i < 0x20000; i += 4)
			{
				// Generate new random numbers.
				while (!se_gen_prng128(random_numbers))
					;
				// Clamp offsets to 512MBrange.
				random_offsets[i + 0] = random_numbers[0] % 0x100000;
				random_offsets[i + 1] = random_numbers[1] % 0x100000;
				random_offsets[i + 2] = random_numbers[2] % 0x100000;
				random_offsets[i + 3] = random_numbers[3] % 0x100000;
			}

			pct = 0;
			prevPct = 200;
			timer = 0;
			data_remaining = 0x100000; // 512MB.

			while (data_remaining)
			{
				u32 time_taken = get_tmr_us();
				sdmmc_storage_read(storage, sector + random_offsets[lba_idx], sector_num, (u8 *)MIXD_BUF_ALIGNED);
				time_taken = get_tmr_us() - time_taken;
				timer += time_taken;

				manual_system_maintenance(false);
				data_remaining -= sector_num;
				lba_idx++;

				pct = (lba_idx * 100) / 0x20000;
				if (pct != prevPct)
				{
					lv_bar_set_value(bar, pct);
					manual_system_maintenance(true);

					prevPct = pct;

					if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
						break;
				}
			}
			lv_bar_set_value(bar, 100);

			// Calculate rate and IOPS for 512MB transfer.
			rate_1k = ((u64)512 * 1000 * 1000 * 1000) / timer;
			iops_1k = ((u64)512 * 1024 * 1000 * 1000 * 1000) / (4096 / 1024) / timer / 1000;
			s_printf(txt_buf + strlen(txt_buf),
				" Random      4KiB - Rate: #C7EA46 %3d.%02d MiB/s#, IOPS: #C7EA46 %4d#\n",
				rate_1k / 1000, (rate_1k % 1000) / 10, iops_1k);
			lv_label_set_text(lbl_status, txt_buf);
			lv_obj_align(lbl_status, NULL, LV_ALIGN_CENTER, 0, 0);
			lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
			manual_system_maintenance(true);
			free(random_offsets);
		}

		lv_obj_del(bar);

		if (sd_bench)
			sd_unmount();
		else
			sdmmc_storage_end(&emmc_storage);
	}
	free(txt_buf);

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action); // Important. After set_text.
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_emmc_bench(lv_obj_t * btn)
{
	_create_mbox_benchmark(false);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_sd_bench(lv_obj_t * btn)
{
	_create_mbox_benchmark(true);

	return LV_RES_OK;
}

static lv_res_t _create_window_emmc_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_CHIP" Internal eMMC Info");
	lv_win_add_btn(win, NULL, SYMBOL_CHIP" Benchmark", _create_mbox_emmc_bench);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 6 * 2, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);

	char *txt_buf = (char *)malloc(0x4000);
	txt_buf[0] = '\n';
	txt_buf[1] = 0;

	if (!sdmmc_storage_init_mmc(&emmc_storage, &emmc_sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400))
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init eMMC!#");
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));
	}
	else
	{
		u32 speed = 0;
		char *rsvd_blocks;
		char life_a_txt[8];
		char life_b_txt[8];
		u32 cache = emmc_storage.ext_csd.cache_size;
		u32 life_a = emmc_storage.ext_csd.dev_life_est_a;
		u32 life_b = emmc_storage.ext_csd.dev_life_est_b;
		u16 card_type = emmc_storage.ext_csd.card_type;
		char card_type_support[96];
		card_type_support[0] = 0;

		// Identify manufacturer. Only official eMMCs.
		switch (emmc_storage.cid.manfid)
		{
		case 0x11:
			strcat(txt_buf, "Toshiba ");
			break;
		case 0x15:
			strcat(txt_buf, "Samsung ");
			break;
		case 0x45: // Unofficial.
			strcat(txt_buf, "SanDisk ");
			break;
		case 0x90:
			strcat(txt_buf, "SK Hynix ");
			break;
		}

		s_printf(txt_buf + strlen(txt_buf), "(%02X)\n%c%c%c%c%c%c\n%d.%d\n%04X\n%02d/%04d\n\n",
			emmc_storage.cid.manfid,
			emmc_storage.cid.prod_name[0], emmc_storage.cid.prod_name[1], emmc_storage.cid.prod_name[2],
			emmc_storage.cid.prod_name[3], emmc_storage.cid.prod_name[4],	emmc_storage.cid.prod_name[5],
			emmc_storage.cid.prv & 0xF, emmc_storage.cid.prv >> 4,
			emmc_storage.cid.serial, emmc_storage.cid.month, emmc_storage.cid.year);

		if (card_type & EXT_CSD_CARD_TYPE_HS_26)
		{
			strcat(card_type_support, "HS26");
			speed = (26 << 16) | 26;
		}
		if (card_type & EXT_CSD_CARD_TYPE_HS_52)
		{
			strcat(card_type_support, ", HS52");
			speed = (52 << 16) | 52;
		}
		if (card_type & EXT_CSD_CARD_TYPE_DDR_1_8V)
		{
			strcat(card_type_support, ", DDR52 1.8V");
			speed = (52 << 16) | 104;
		}
		if (card_type & EXT_CSD_CARD_TYPE_HS200_1_8V)
		{
			strcat(card_type_support, ", HS200 1.8V");
			speed = (200 << 16) | 200;
		}
		if (card_type & EXT_CSD_CARD_TYPE_HS400_1_8V)
		{
			strcat(card_type_support, ", HS400 1.8V");
			speed = (200 << 16) | 400;
		}

		strcpy(life_a_txt, "-");
		strcpy(life_b_txt, "-");

		// Normalize cells life.
		if (life_a) // SK Hynix is 0 (undefined).
		{
			life_a--;
			life_a = (10 - life_a) * 10;
			s_printf(life_a_txt, "%d%%", life_a);
		}

		if (life_b) // Toshiba is 0 (undefined).
		{
			life_b--;
			life_b = (10 - life_b) * 10;
			s_printf(life_b_txt, "%d%%", life_b);
		}

		switch (emmc_storage.ext_csd.pre_eol_info)
		{
		case 1:
			rsvd_blocks = "Normal (< 80%)";
			break;
		case 2:
			rsvd_blocks = "Warning (> 80%)";
			break;
		case 3:
			rsvd_blocks = "Critical (> 90%)";
			break;
		default:
			rsvd_blocks = "#FF8000 Unknown#";
			break;
		}

		s_printf(txt_buf + strlen(txt_buf),
			"#00DDFF V1.%d (rev 1.%d)#\n%02X\n%d MB/s (%d MHz)\n%d MB/s\n%s\n%d %s\n%d MiB\nA: %s, B: %s\n%s",
			emmc_storage.ext_csd.ext_struct, emmc_storage.ext_csd.rev,
			emmc_storage.csd.cmdclass, speed & 0xFFFF, (speed >> 16) & 0xFFFF,
			emmc_storage.csd.busspeed, card_type_support,
			!(cache % 1024) ? (cache / 1024) : cache, !(cache % 1024) ? "MiB" : "KiB",
			emmc_storage.ext_csd.max_enh_mult * 512 / 1024,
			life_a_txt, life_b_txt, rsvd_blocks);

		lv_label_set_static_text(lb_desc,
			"#00DDFF CID:#\n"
			"Vendor ID:\n"
			"Model:\n"
			"Prod Rev:\n"
			"S/N:\n"
			"Month/Year:\n\n"
			"#00DDFF Ext CSD:#\n"
			"Cmd Classes:\n"
			"Max Rate:\n"
			"Current Rate:\n"
			"Type Support:\n\n"
			"Write Cache:\n"
			"Enhanced Area:\n"
			"Estimated Life:\n"
			"Reserved Used:"
		);
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

		lv_obj_t *val = lv_cont_create(win, NULL);
		lv_obj_set_size(val, LV_HOR_RES / 11 * 3, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

		lv_obj_t * lb_val = lv_label_create(val, lb_desc);

		lv_label_set_text(lb_val, txt_buf);

		lv_obj_set_width(lb_val, lv_obj_get_width(val));
		lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		lv_obj_t *desc2 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc2, LV_HOR_RES / 2 / 4 * 4, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

		lv_obj_t * lb_desc2 = lv_label_create(desc2, lb_desc);
		lv_label_set_style(lb_desc2, &monospace_text);

		u32 boot_size = emmc_storage.ext_csd.boot_mult << 17;
		u32 rpmb_size = emmc_storage.ext_csd.rpmb_mult << 17;
		strcpy(txt_buf, "#00DDFF eMMC Physical Partitions:#\n");
		s_printf(txt_buf + strlen(txt_buf), "1: #96FF00 BOOT0# Size: %6d KiB (Sect: 0x%08X)\n", boot_size / 1024, boot_size / 512);
		s_printf(txt_buf + strlen(txt_buf), "2: #96FF00 BOOT1# Size: %6d KiB (Sect: 0x%08X)\n", boot_size / 1024, boot_size / 512);
		s_printf(txt_buf + strlen(txt_buf), "3: #96FF00 RPMB#  Size: %6d KiB (Sect: 0x%08X)\n", rpmb_size / 1024, rpmb_size / 512);
		s_printf(txt_buf + strlen(txt_buf), "0: #96FF00 GPP#   Size: %6d MiB (Sect: 0x%08X)\n", emmc_storage.sec_cnt >> SECTORS_TO_MIB_COEFF, emmc_storage.sec_cnt);
		strcat(txt_buf, "\n#00DDFF GPP (eMMC USER) Partition Table:#\n");

		sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);
		LIST_INIT(gpt);
		nx_emmc_gpt_parse(&gpt, &emmc_storage);

		u32 idx = 0;
		LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
		{
			if (idx > 10)
			{
				strcat(txt_buf, "#FFDD00 Table does not fit on screen!#");
				break;
			}

			if (part->index < 2)
			{
				s_printf(txt_buf + strlen(txt_buf), "%02d: #96FF00 %s#%s Size: %d MiB (Sect: 0x%X), Start: %06X\n",
					part->index, part->name, !part->name[8] ? " " : "",
					(part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
					part->lba_end - part->lba_start + 1, part->lba_start);
			}
			else
			{
				s_printf(txt_buf + strlen(txt_buf), "%02d: #96FF00 %s#\n    Size: %7d MiB (Sect: 0x%07X), Start: %07X\n",
					part->index, part->name, (part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
					part->lba_end - part->lba_start + 1, part->lba_start);
			}

			idx++;
		}
		if (!idx)
			strcat(txt_buf, "#FFDD00 Partition table is empty!#");

		nx_emmc_gpt_free(&gpt);

		lv_label_set_text(lb_desc2, txt_buf);
		lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
		lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 6, 0);
	}

	sdmmc_storage_end(&emmc_storage);
	free(txt_buf);

	return LV_RES_OK;
}

static lv_res_t _create_window_sdcard_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_SD" microSD Card Info");
	lv_win_add_btn(win, NULL, SYMBOL_SD" Benchmark", _create_mbox_sd_bench);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 5 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 5 / 2);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);

	lv_label_set_text(lb_desc, "#D4FF00 Please wait...#");
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	// Disable buttons.
	nyx_window_toggle_buttons(win, true);

	manual_system_maintenance(true);

	if (!sd_mount())
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init SD!#");
	else
	{
		lv_label_set_text(lb_desc,
			"#00DDFF Card IDentification:#\n"
			"Vendor ID:\n"
			"Model:\n"
			"OEM ID:\n"
			"HW rev:\n"
			"FW rev:\n"
			"S/N:\n"
			"Month/Year:\n\n"
			"Bootloader bus:"
		);

		lv_obj_t *val = lv_cont_create(win, NULL);
		lv_obj_set_size(val, LV_HOR_RES / 9 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 5 / 2);

		lv_obj_t * lb_val = lv_label_create(val, lb_desc);

		char *txt_buf = (char *)malloc(0x4000);
		txt_buf[0] = '\n';
		txt_buf[1] = 0;

		// Identify manufacturer.
		switch (sd_storage.cid.manfid)
		{
		case 1:
			strcat(txt_buf, "Panasonic ");
			break;
		case 2:
			strcat(txt_buf, "Toshiba ");
			break;
		case 3:
			strcat(txt_buf, "SanDisk ");
			break;
		case 0x1B:
			strcat(txt_buf, "Samsung ");
			break;
		case 0x1D:
			strcat(txt_buf, "AData ");
			break;
		case 0x27:
			strcat(txt_buf, "Phison ");
			break;
		case 0x28:
			strcat(txt_buf, "Lexar ");
			break;
		case 0x31:
			strcat(txt_buf, "Silicon Power ");
			break;
		case 0x41:
			strcat(txt_buf, "Kingston ");
			break;
		case 0x74:
			strcat(txt_buf, "Transcend ");
			break;
		case 0x76:
			strcat(txt_buf, "Patriot ");
			break;
		case 0x82:
			strcat(txt_buf, "Sony ");
			break;
		}

		s_printf(txt_buf + strlen(txt_buf), "(%02X)\n%c%c%c%c%c\n%c%c\n%X\n%X\n%08x\n%02d/%04d\n\n",
			sd_storage.cid.manfid,
			sd_storage.cid.prod_name[0], sd_storage.cid.prod_name[1], sd_storage.cid.prod_name[2],
			sd_storage.cid.prod_name[3], sd_storage.cid.prod_name[4],
			(sd_storage.cid.oemid >> 8) & 0xFF, sd_storage.cid.oemid & 0xFF,
			sd_storage.cid.hwrev, sd_storage.cid.fwrev, sd_storage.cid.serial,
			sd_storage.cid.month, sd_storage.cid.year);

		switch (nyx_str->info.sd_init)
		{
		case SD_1BIT_HS25:
			strcat(txt_buf, "HS25 1bit");
			break;
		case SD_4BIT_HS25:
			strcat(txt_buf, "HS25");
			break;
		case SD_UHS_SDR82: // Report as SDR104.
		case SD_UHS_SDR104:
			strcat(txt_buf, "SDR104");
			break;
		case 0:
		default:
			strcat(txt_buf, "Undefined");
			break;
		}

		lv_label_set_text(lb_val, txt_buf);

		lv_obj_set_width(lb_val, lv_obj_get_width(val));
		lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		lv_obj_t *desc2 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc2, LV_HOR_RES / 2 / 4 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 5 / 2);

		lv_obj_t * lb_desc2 = lv_label_create(desc2, lb_desc);

		lv_label_set_static_text(lb_desc2,
			"#00DDFF Card-Specific Data#\n"
			"Cmd Classes:\n"
			"Capacity:\n"
			"Capacity (LBA):\n"
			"Bus Width:\n"
			"Current Rate:\n"
			"Speed Class:\n"
			"UHS Grade:\n"
			"Video Class:\n"
			"App perf class:\n"
			"Write Protect:"
		);
		lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
		lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);

		lv_obj_t *val2 = lv_cont_create(win, NULL);
		lv_obj_set_size(val2, LV_HOR_RES / 13 * 3, LV_VER_RES - (LV_DPI * 11 / 8) * 5 / 2);

		lv_obj_t * lb_val2 = lv_label_create(val2, lb_desc);

		char *wp_info;
		switch (sd_storage.csd.write_protect)
		{
		case 1:
			wp_info = "Temporary";
			break;
		case 2:
		case 3:
			wp_info = "Permanent";
			break;
		default:
			wp_info = "None";
			break;
		}

		bool uhs_au_mb = false;
		u32 uhs_au_size = sd_storage_get_ssr_au(&sd_storage);
		if (uhs_au_size >= 1024)
		{
			uhs_au_mb = true;
			uhs_au_size /= 1024;
		}

		s_printf(txt_buf,
			"#00DDFF v%d.0#\n%02X\n%d MiB\n%X (CP %X)\n%d\n%d MB/s (%d MHz)\n%d (AU: %d %s\nU%d\nV%d\nA%d\n%s",
			sd_storage.csd.structure + 1, sd_storage.csd.cmdclass,
			sd_storage.sec_cnt >> 11, sd_storage.sec_cnt, sd_storage.ssr.protected_size >> 9,
			sd_storage.ssr.bus_width, sd_storage.csd.busspeed,
			(sd_storage.csd.busspeed > 10) ? (sd_storage.csd.busspeed * 2) : 50,
			sd_storage.ssr.speed_class, uhs_au_size, uhs_au_mb ? "MiB)" : "KiB)", sd_storage.ssr.uhs_grade,
			sd_storage.ssr.video_class, sd_storage.ssr.app_class, wp_info);

		lv_label_set_text(lb_val2, txt_buf);

		lv_obj_set_width(lb_val2, lv_obj_get_width(val2));
		lv_obj_align(val2, desc2, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		lv_obj_t *line_sep = lv_line_create(win, NULL);
		static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 12, 0} };
		lv_line_set_points(line_sep, line_pp, 2);
		lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
		lv_obj_align(line_sep, desc, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI * 410 / 100, LV_DPI / 7);

		lv_obj_t *desc3 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc3, LV_HOR_RES / 2 / 2 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 4);

		lv_obj_t * lb_desc3 = lv_label_create(desc3, lb_desc);
		lv_label_set_text(lb_desc3, "#D4FF00 Acquiring FAT volume info...#");
		lv_obj_set_width(lb_desc3, lv_obj_get_width(desc3));

		lv_obj_align(desc3, desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);

		manual_system_maintenance(true);

		f_getfree("", &sd_fs.free_clst, NULL);

		lv_label_set_text(lb_desc3,
			"#00DDFF Found FAT volume:#\n"
			"Filesystem:\n"
			"Cluster:\n"
			"Free:"
		);
		lv_obj_set_size(desc3, LV_HOR_RES / 2 / 5 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 4);
		lv_obj_set_width(lb_desc3, lv_obj_get_width(desc3));
		lv_obj_align(desc3, desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);

		lv_obj_t *val3 = lv_cont_create(win, NULL);
		lv_obj_set_size(val3, LV_HOR_RES / 13 * 3, LV_VER_RES - (LV_DPI * 11 / 8) * 4);

		lv_obj_t * lb_val3 = lv_label_create(val3, lb_desc);

		s_printf(txt_buf, "\n%s\n%d %s\n%d MiB",
			sd_fs.fs_type == FS_EXFAT ? ("exFAT  "SYMBOL_SHRK) : ("FAT32"),
			(sd_fs.csize > 1) ? (sd_fs.csize >> 1) : 512,
			(sd_fs.csize > 1) ? "KiB" : "B",
			sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF);

		lv_label_set_text(lb_val3, txt_buf);

		lv_obj_set_width(lb_val3, lv_obj_get_width(val3));
		lv_obj_align(val3, desc3, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		lv_obj_t *desc4 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc4, LV_HOR_RES / 2 / 2 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 4);

		lv_obj_t * lb_desc4 = lv_label_create(desc4, lb_desc);
		lv_label_set_text(lb_desc4, "#D4FF00 Acquiring FAT volume info...#");
		lv_obj_set_width(lb_desc4, lv_obj_get_width(desc4));

		lv_label_set_text(lb_desc4,
			"#00DDFF SDMMC1 Errors:#\n"
			"Init fails:\n"
			"Read/Write fails:\n"
			"Read/Write errors:"
		);
		lv_obj_set_size(desc4, LV_HOR_RES / 2 / 5 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 4);
		lv_obj_set_width(lb_desc4, lv_obj_get_width(desc4));
		lv_obj_align(desc4, val3, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);

		lv_obj_t *val4 = lv_cont_create(win, NULL);
		lv_obj_set_size(val4, LV_HOR_RES / 13 * 3, LV_VER_RES - (LV_DPI * 11 / 8) * 4);

		lv_obj_t * lb_val4 = lv_label_create(val4, lb_desc);

		u16 *sd_errors = sd_get_error_count();
		s_printf(txt_buf, "\n%d (%d)\n%d (%d)\n%d (%d)",
			sd_errors[0], nyx_str->info.sd_errors[0], sd_errors[1], nyx_str->info.sd_errors[1], sd_errors[2], nyx_str->info.sd_errors[2]);

		lv_label_set_text(lb_val4, txt_buf);

		lv_obj_set_width(lb_val4, lv_obj_get_width(val4));
		lv_obj_align(val4, desc4, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);

		free(txt_buf);
		sd_unmount();
	}

	nyx_window_toggle_buttons(win, false);

	return LV_RES_OK;
}

static lv_res_t _create_window_battery_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_BATTERY_FULL" Battery Info");
	lv_win_add_btn(win, NULL, SYMBOL_DOWNLOAD" Dump Fuel Regs", _battery_dump_window_action);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 4 * 2, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);

	lv_label_set_static_text(lb_desc,
		"#00DDFF Fuel Gauge IC Info:#\n"
		"Capacity now:\n"
		"Capacity full:\n"
		"Capacity (design):\n"
		"Current now:\n"
		"Current average:\n"
		"Voltage now:\n"
		"Voltage open-circuit:\n"
		"Min voltage reached:\n"
		"Max voltage reached:\n"
		"Empty voltage:\n"
		"Battery temp:\n\n"
		"#00DDFF PMIC IC Info:#\n"
		"Main PMIC:\n\n"
		"CPU/GPU PMIC:\n"
	);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	lv_obj_t *val = lv_cont_create(win, NULL);
	lv_obj_set_size(val, LV_HOR_RES / 5, LV_VER_RES - (LV_DPI * 11 / 7));

	lv_obj_t * lb_val = lv_label_create(val, lb_desc);

	char *txt_buf = (char *)malloc(0x4000);
	int value = 0;
	int cap_pct = 0;

	max17050_get_property(MAX17050_RepSOC, &cap_pct);
	max17050_get_property(MAX17050_RepCap, &value);
	s_printf(txt_buf, "\n%d mAh [%d %]\n", value, cap_pct >> 8);

	max17050_get_property(MAX17050_FullCAP, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mAh\n", value);

	max17050_get_property(MAX17050_DesignCap, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mAh\n", value);

	max17050_get_property(MAX17050_Current, &value);
	if (value >= 0)
		s_printf(txt_buf + strlen(txt_buf), "%d mA\n", value / 1000);
	else
		s_printf(txt_buf + strlen(txt_buf), "-%d mA\n", (~value + 1) / 1000);

	max17050_get_property(MAX17050_AvgCurrent, &value);
	if (value >= 0)
		s_printf(txt_buf + strlen(txt_buf), "%d mA\n", value / 1000);
	else
		s_printf(txt_buf + strlen(txt_buf), "-%d mA\n", (~value + 1) / 1000);

	max17050_get_property(MAX17050_VCELL, &value);
	bool voltage_empty = value < 3200;
	s_printf(txt_buf + strlen(txt_buf), "%s%d mV%s\n",
		voltage_empty ? "#FF8000 " : "", value,  voltage_empty ? " "SYMBOL_WARNING"#" : "");

	max17050_get_property(MAX17050_OCVInternal, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

	max17050_get_property(MAX17050_MinVolt, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

	max17050_get_property(MAX17050_MaxVolt, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

	max17050_get_property(MAX17050_V_empty, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

	max17050_get_property(MAX17050_TEMP, &value);
	if (value >= 0)
		s_printf(txt_buf + strlen(txt_buf), "%d.%d oC\n\n\n", value / 10, value % 10);
	else
		s_printf(txt_buf + strlen(txt_buf), "-%d.%d oC\n\n\n", (~value + 1) / 10, (~value + 1) % 10);

	value = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CID4);
	u32 main_pmic_version = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CID3) & 0xF;

	if (value == 0x35)
		s_printf(txt_buf + strlen(txt_buf), "max77620 v%d\nErista OTP\n", main_pmic_version);
	else if (value == 0x53)
		s_printf(txt_buf + strlen(txt_buf), "max77620 v%d\nMariko OTP\n", main_pmic_version);
	else
		s_printf(txt_buf + strlen(txt_buf), "max77620 v%d\n#FF8000 Unknown OTP# (%02X)\n", main_pmic_version, value);

	u32 cpu_gpu_pmic_type = h_cfg.t210b01 ? (FUSE(FUSE_RESERVED_ODM28_T210B01) & 1) + 1 : 0;
	switch (cpu_gpu_pmic_type)
	{
	case 0:
		s_printf(txt_buf + strlen(txt_buf), "max77621 v%d",
			i2c_recv_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_CHIPID1_REG));
		break;
	case 1:
		s_printf(txt_buf + strlen(txt_buf), "max77812-2 v%d",
			i2c_recv_byte(I2C_5, MAX77812_PHASE31_CPU_I2C_ADDR, MAX77812_REG_VERSION) & 7);
		break;
	case 2:
		s_printf(txt_buf + strlen(txt_buf), "max77812-3 v%d.0",
			i2c_recv_byte(I2C_5, MAX77812_PHASE211_CPU_I2C_ADDR, MAX77812_REG_VERSION) & 7);
		break;
	}

	lv_label_set_text(lb_val, txt_buf);

	lv_obj_set_width(lb_val, lv_obj_get_width(val));
	lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_t *desc2 = lv_cont_create(win, NULL);
	lv_obj_set_size(desc2, LV_HOR_RES / 2 / 7 * 4, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc2 = lv_label_create(desc2, lb_desc);

	lv_label_set_static_text(lb_desc2,
		"#00DDFF Battery Charger IC Info:#\n"
		"Input voltage limit:\n"
		"Input current limit:\n"
		"Min voltage limit:\n"
		"Fast charge current limit:\n"
		"Charge voltage limit:\n"
		"Charge status:\n"
		"Temperature status:\n\n"
		"#00DDFF USB-PD IC Info:#\n"
		"Connection status:\n"
		"Input Wattage Limit:\n"
		"USB-PD Profiles:"
	);
	lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
	lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);

	lv_obj_t *val2 = lv_cont_create(win, NULL);
	lv_obj_set_size(val2, LV_HOR_RES / 2 / 3, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_val2 = lv_label_create(val2, lb_desc);

	bq24193_get_property(BQ24193_InputVoltageLimit, &value);
	s_printf(txt_buf, "\n%d mV\n", value);

	int iinlim = 0;
	bq24193_get_property(BQ24193_InputCurrentLimit, &iinlim);
	s_printf(txt_buf + strlen(txt_buf), "%d mA\n", iinlim);

	bq24193_get_property(BQ24193_SystemMinimumVoltage, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

	bq24193_get_property(BQ24193_FastChargeCurrentLimit, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mA\n", value);

	bq24193_get_property(BQ24193_ChargeVoltageLimit, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

	bq24193_get_property(BQ24193_ChargeStatus, &value);
	switch (value)
	{
	case 0:
		strcat(txt_buf, "Not charging\n");
		break;
	case 1:
		strcat(txt_buf, "Pre-charging\n");
		break;
	case 2:
		strcat(txt_buf, "Fast charging\n");
		break;
	case 3:
		strcat(txt_buf, "Charge terminated\n");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "Unknown (%d)\n", value);
		break;
	}

	bq24193_get_property(BQ24193_TempStatus, &value);
	switch (value)
	{
	case 0:
		strcat(txt_buf, "Normal");
		break;
	case 2:
		strcat(txt_buf, "Warm");
		break;
	case 3:
		strcat(txt_buf, "Cool");
		break;
	case 5:
		strcat(txt_buf, "Cold");
		break;
	case 6:
		strcat(txt_buf, "Hot");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "Unknown (%d)", value);
		break;
	}

	bool inserted;
	u32 wattage = 0;
	usb_pd_objects_t usb_pd;
	bm92t36_get_sink_info(&inserted, &usb_pd);
	strcat(txt_buf, "\n\n\n");
	strcat(txt_buf, inserted ? "Connected" : "Disconnected");

	// Select 5V is no PD contract.
	wattage = iinlim * (usb_pd.pdo_no ? usb_pd.selected_pdo.voltage : 5);

	s_printf(txt_buf + strlen(txt_buf), "\n%d.%d W", wattage / 1000, (wattage % 1000) / 100);

	if (!usb_pd.pdo_no)
		strcat(txt_buf, "\nNon PD");

	// Limit to 5 profiles so it can fit.
	usb_pd.pdo_no = MIN(usb_pd.pdo_no, 5);

	for (u32 i = 0; i < usb_pd.pdo_no; i++)
	{
		bool selected =
			usb_pd.pdos[i].amperage == usb_pd.selected_pdo.amperage &&
			usb_pd.pdos[i].voltage == usb_pd.selected_pdo.voltage;
		s_printf(txt_buf + strlen(txt_buf), "\n%s%d mA, %2d V%s",
			selected ? "#D4FF00 " : "",
			usb_pd.pdos[i].amperage, usb_pd.pdos[i].voltage,
			selected ? "#" : "");
	}

	lv_label_set_text(lb_val2, txt_buf);

	lv_obj_set_width(lb_val2, lv_obj_get_width(val2));
	lv_obj_align(val2, desc2, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	free(txt_buf);

	return LV_RES_OK;
}

void create_tab_info(lv_theme_t *th, lv_obj_t *parent)
{
	lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY);

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 6;

	// Create SoC Info container.
	lv_obj_t *h1 = lv_cont_create(parent, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "SoC & HW Info");
	lv_obj_set_style(label_txt, th->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, 0);

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, th->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create Bootrom button.
	lv_obj_t *btn = lv_btn_create(h1, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	lv_obj_t *label_btn = lv_label_create(btn, NULL);
	lv_btn_set_fit(btn, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_CHIP"  Bootrom");
	lv_obj_align(btn, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, _create_window_bootrom_info_status);

	// Create TSEC Keys button.
	lv_obj_t *btn2 = lv_btn_create(h1, btn);
	label_btn = lv_label_create(btn2, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_KEY"  TSEC Keys");
	lv_obj_align(btn2, btn, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 4 / 9, 0);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, _create_window_tsec_keys_status);
	if (h_cfg.t210b01)
		lv_btn_set_state(btn2, LV_BTN_STATE_INA);

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"View Ipatches and dump the unpatched and patched versions\nof BootROM.\n"
		"Or view and dump the device's TSEC Keys.\n");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	static lv_style_t line_style;
	lv_style_copy(&line_style, th->line.decor);
	line_style.line.color = LV_COLOR_HEX(0x444444);

	line_sep = lv_line_create(h1, line_sep);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 16);
	lv_line_set_style(line_sep, &line_style);

	// Create Fuses button.
	lv_obj_t *btn3 = lv_btn_create(h1, btn);
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_CIRCUIT"  HW & Fuses");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 2);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _create_window_fuses_info_status);

	// Create KFuses button.
	lv_obj_t *btn4 = lv_btn_create(h1, btn);
	label_btn = lv_label_create(btn4, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_SHUFFLE"  KFuses");
	lv_obj_align(btn4, btn3, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 46 / 100, 0);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _kfuse_dump_window_action);

	lv_obj_t *label_txt4 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"View and dump the cached #C7EA46 Fuses# and #C7EA46 KFuses#.\n"
		"Fuses contain info about the SoC/SKU and KFuses HDCP keys.\n"
		"You can also see info about #C7EA46 DRAM#, #C7EA46 Screen# and #C7EA46 Touch panel#.");
	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Storage & Battery Info container.
	lv_obj_t *h2 = lv_cont_create(parent, NULL);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, true);
	lv_obj_set_width(h2, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h2, false);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "Storage & Battery Info");
	lv_obj_set_style(label_txt3, th->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, 0);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 2), LV_DPI / 8);
	lv_line_set_style(line_sep, th->line.decor);

	// Create eMMC button.
	lv_obj_t *btn5 = lv_btn_create(h2, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn5, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn5, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	label_btn = lv_label_create(btn5, NULL);
	lv_btn_set_fit(btn5, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_CHIP"  eMMC  ");
	lv_obj_align(btn5, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 2, LV_DPI / 4);
	lv_btn_set_action(btn5, LV_BTN_ACTION_CLICK, _create_window_emmc_info_status);

	// Create microSD button.
	lv_obj_t *btn6 = lv_btn_create(h2, btn);
	label_btn = lv_label_create(btn6, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_SD"  microSD ");
	lv_obj_align(btn6, btn5, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 3 / 4, 0);
	lv_btn_set_action(btn6, LV_BTN_ACTION_CLICK, _create_window_sdcard_info_status);

	lv_obj_t *label_txt5 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt5, true);
	lv_label_set_static_text(label_txt5,
		"View info about the eMMC or microSD and their partition list.\n"
		"Additionally you can benchmark read speeds.");
	lv_obj_set_style(label_txt5, &hint_small_style);
	lv_obj_align(label_txt5, btn5, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt5, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 2);
	lv_line_set_style(line_sep, &line_style);

	// Create Battery button.
	lv_obj_t *btn7 = lv_btn_create(h2, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn7, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn7, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	label_btn = lv_label_create(btn7, NULL);
	lv_btn_set_fit(btn7, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_BATTERY_FULL"  Battery");
	lv_obj_align(btn7, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 2);
	lv_btn_set_action(btn7, LV_BTN_ACTION_CLICK, _create_window_battery_status);

	lv_obj_t *label_txt6 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt6, true);
	lv_label_set_static_text(label_txt6,
		"View battery and battery charger related info.\n"
		"Additionally you can dump battery charger's registers.\n");
	lv_obj_set_style(label_txt6, &hint_small_style);
	lv_obj_align(label_txt6, btn7, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
}
