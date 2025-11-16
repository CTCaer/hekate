/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2025 CTCaer
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

#include <bdk.h>

#include "gui.h"
#include "../config.h"
#include "../hos/hos.h"
#include "../hos/pkg1.h"
#include <libs/fatfs/ff.h>

#include <stdlib.h>

#define SECTORS_TO_MIB_COEFF 11

extern volatile boot_cfg_t *b_cfg;
extern volatile nyx_storage_t *nyx_str;

extern lv_res_t launch_payload(lv_obj_t *list);
extern char *emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

static lv_res_t _create_window_dump_done(int error, char *dump_filenames)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);

	char *txt_buf = (char *)malloc(SZ_4K);

	if (error)
		s_printf(txt_buf, "#FFDD00 Failed to dump to# %s#FFDD00 !#\nError: %d", dump_filenames, error);
	else
	{
		char *sn = emmcsn_path_impl(NULL, NULL, NULL, NULL);
		s_printf(txt_buf, "Dumping to SD card finished!\nFiles: #C7EA46 backup/%s/dumps/#\n%s", sn, dump_filenames);
	}
	lv_mbox_set_text(mbox, txt_buf);
	free(txt_buf);

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
			error = sd_save_to_file((u8 *)cal0_buf, SZ_32K, path);

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
		void *buf = malloc(0x100 * 2);

		max17050_dump_regs(buf);

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
		int res = sd_save_to_file((u8 *)IROM_BASE, BOOTROM_SIZE, path);
		if (!error)
			error = res;

		u32 ipatch_cam[IPATCH_CAM_ENTRIES + 1];
		memcpy(ipatch_cam, (void *)IPATCH_BASE, sizeof(ipatch_cam));
		memset((void*)IPATCH_BASE, 0, sizeof(ipatch_cam)); // Zeroing valid entries is enough but zero everything.

		emmcsn_path_impl(path, "/dumps", "bootrom_unpatched.bin", NULL);
		res = sd_save_to_file((u8 *)IROM_BASE, BOOTROM_SIZE, path);
		if (!error)
			error = res;

		memcpy((void*)IPATCH_BASE, ipatch_cam, sizeof(ipatch_cam));

		sd_unmount();
	}
	_create_window_dump_done(error, "evp_thunks.bin, bootrom_patched.bin, bootrom_unpatched.bin");

	return LV_RES_OK;
}

static lv_res_t _fuse_dump_window_action(lv_obj_t * btn)
{
	const u32 fuse_array_size = (h_cfg.t210b01 ? FUSE_ARRAY_WORDS_NUM_B01 : FUSE_ARRAY_WORDS_NUM) * sizeof(u32);

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

		u32 words[FUSE_ARRAY_WORDS_NUM_B01];
		fuse_read_array(words);
		if (!h_cfg.t210b01)
			emmcsn_path_impl(path, "/dumps", "fuse_array_raw_t210.bin", NULL);
		else
			emmcsn_path_impl(path, "/dumps", "fuse_array_raw_t210b01.bin", NULL);
		int res = sd_save_to_file((u8 *)words, fuse_array_size, path);
		if (!error)
			error = res;

		sd_unmount();
	}

	if (!h_cfg.t210b01)
		_create_window_dump_done(error, "fuse_cached_t210.bin, fuse_array_raw_t210.bin");
	else
		_create_window_dump_done(error, "fuse_cached_t210b01_x*.bin, fuse_array_raw_t210b01.bin");

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

static lv_res_t _create_mbox_cal0(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\251", "\222Dump", "\222Close", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);

	lv_mbox_set_text(mbox, "#C7EA46 CAL0 Info#");

	char *txt_buf = (char *)malloc(SZ_16K);
	txt_buf[0] = 0;

	lv_obj_t * lb_desc = lv_label_create(mbox, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_label_set_style(lb_desc, &monospace_text);
	lv_obj_set_width(lb_desc, LV_HOR_RES / 9 * 4);

	sd_mount();

	// Dump CAL0.
	int cal0_res = hos_dump_cal0();

	// Check result. Don't error if hash doesn't match.
	if (cal0_res == 1)
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init eMMC!#");

		goto out;
	}
	else if (cal0_res == 2)
	{
		lv_label_set_text(lb_desc, "#FFDD00 CAL0 is corrupt or wrong keys!#\n");
		goto out;
	}

	nx_emmc_cal0_t *cal0 = (nx_emmc_cal0_t *)cal0_buf;

	u32 hash[8];
	se_calc_sha256_oneshot(hash, (u8 *)&cal0->cfg_id1, cal0->body_size);

	s_printf(txt_buf,
		"#FF8000 CAL0 Version:#      %d\n"
		"#FF8000 Update Count:#      %d\n"
		"#FF8000 Serial Number:#     %s\n"
		"#FF8000 WLAN MAC:#          %02X:%02X:%02X:%02X:%02X:%02X\n"
		"#FF8000 Bluetooth MAC:#     %02X:%02X:%02X:%02X:%02X:%02X\n"
		"#FF8000 Battery LOT:#       %s (%d)\n"
		"#FF8000 LCD Vendor:#        ",
		cal0->version, cal0->update_cnt, cal0->serial_number,
		cal0->wlan_mac[0], cal0->wlan_mac[1], cal0->wlan_mac[2], cal0->wlan_mac[3], cal0->wlan_mac[4], cal0->wlan_mac[5],
		cal0->bd_mac[0], cal0->bd_mac[1], cal0->bd_mac[2], cal0->bd_mac[3], cal0->bd_mac[4], cal0->bd_mac[5],
		cal0->battery_lot, cal0->battery_ver);

	// Prepare display info.
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
		strcat(txt_buf, "InnoLux P062CCA-AZX");
		break;
	case PANEL_AUO_A062TAN01:
		strcat(txt_buf, "AUO A062TAN0X");
		break;
	case PANEL_INL_2J055IA_27A:
		strcat(txt_buf, "InnoLux 2J055IA-27A");
		break;
	case PANEL_AUO_A055TAN01:
		strcat(txt_buf, "AUO A055TAN0X");
		break;
	case PANEL_SHP_LQ055T1SW10:
		strcat(txt_buf, "Sharp LQ055T1SW10");
		break;
	case PANEL_SAM_AMS699VC01:
		strcat(txt_buf, "Samsung AMS699VC01");
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
		case (PANEL_SAM_AMS699VC01 & 0xFF):
			strcat(txt_buf, "Samsung ");
			break;
		}
		strcat(txt_buf, "Unknown");
		break;
	}

	s_printf(txt_buf + strlen(txt_buf),
		" (%06X)\n#FF8000 Touch Vendor:#      %d\n"
		"#FF8000 IMU Type/Mount:#    %d / %d\n"
		"#FF8000 Stick L/R Type:#    %02X / %02X\n",
		cal0->lcd_vendor, cal0->touch_ic_vendor_id,
		cal0->console_6axis_sensor_type, cal0->console_6axis_sensor_mount_type,
		cal0->analog_stick_type_l, cal0->analog_stick_type_r);

	bool valid_cal0 = !memcmp(hash, cal0->body_sha256, 0x20);
	s_printf(txt_buf + strlen(txt_buf), "#FF8000 SHA256 Hash Match:# %s", valid_cal0 ? "Pass" : "Failed");

	lv_label_set_text(lb_desc, txt_buf);

out:
	free(txt_buf);
	sd_unmount();

	lv_mbox_add_btns(mbox, mbox_btn_map, _cal0_dump_window_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_obj_t *hw_info_ver = NULL;
static lv_res_t _action_win_hw_info_status_close(lv_obj_t *btn)
{
	if (hw_info_ver)
	{
		lv_obj_del(hw_info_ver);
		hw_info_ver = NULL;
	}

	return nyx_win_close_action_custom(btn);
}

static lv_res_t _create_window_hw_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_window_custom_close_btn(SYMBOL_CHIP" HW & Fuses Info", _action_win_hw_info_status_close);
	lv_win_add_btn(win, NULL, SYMBOL_DOWNLOAD" Dump fuses", _fuse_dump_window_action);
	lv_win_add_btn(win, NULL, SYMBOL_INFO" CAL0 Info", _create_mbox_cal0);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 5 * 2, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_label_set_style(lb_desc, &monospace_text);

	char version[32];
	s_printf(version, "%s%d.%d.%d%c", NYX_VER_RL ? "v" : "", NYX_VER_MJ, NYX_VER_MN, NYX_VER_HF, NYX_VER_RL > 'A' ? NYX_VER_RL : 0);
	lv_obj_t * lbl_ver = lv_label_create(lv_scr_act(), NULL);
	lv_label_set_style(lbl_ver, &hint_small_style_white);
	lv_label_set_text(lbl_ver, version);
	lv_obj_align(lbl_ver, status_bar.bar_bg, LV_ALIGN_OUT_TOP_RIGHT, -LV_DPI * 9 / 23, -LV_DPI * 2 / 13);
	hw_info_ver = lbl_ver;

	lv_label_set_static_text(lb_desc,
		"#FF8000 SoC:#\n"
		"#FF8000 SKU:#\n"
		"#FF8000 DRAM ID:#\n"
		"#FF8000 Burnt Fuses (ODM 7/6):#\n"
		"ODM Fields (4, 6, 7):\n"
		"Secure Boot Key (SBK):\n"
		"Device Key (DK):\n"
		"Public Key (PK SHA256):\n\n"
		"HOS Keygen Revision:\n"
		"USB Stack:\n"
		"Final Test Revision:\n"
		"Chip Probing Revision:\n"
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
		"Wafer ID:\n"
		"X Coordinate:\n"
		"Y Coordinate:"
	);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	lv_obj_t *val = lv_cont_create(win, NULL);
	lv_obj_set_size(val, LV_HOR_RES / 7 * 2 + LV_DPI / 11, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_val = lv_label_create(val, lb_desc);

	char *txt_buf = (char *)malloc(SZ_16K);

	// Decode fuses.
	char *sku;
	char dram_model[64];
	char fuses_hos_version[64];
	u8 dram_id = fuse_read_dramid(true);

	switch (fuse_read_hw_type())
	{
	case FUSE_NX_HW_TYPE_ICOSA:
		sku = "Icosa - Odin";
		break;
	case FUSE_NX_HW_TYPE_IOWA:
		sku = "Iowa - Modin";
		break;
	case FUSE_NX_HW_TYPE_HOAG:
		sku = "Hoag - Vali";
		break;
	case FUSE_NX_HW_TYPE_AULA:
		sku = "Aula - Fric";
		break;
	default:
		sku = "#FF8000 Unknown#";
		break;
	}

	// Prepare dram id info.
	if (!h_cfg.t210b01)
	{
		switch (dram_id)
		{
		// LPDDR4 3200Mbps.
		case LPDDR4_ICOSA_4GB_SAMSUNG_K4F6E304HB_MGCH:
			strcpy(dram_model, "Samsung K4F6E304HB-MGCH 4GB");
			break;
		case LPDDR4_ICOSA_4GB_HYNIX_H9HCNNNBPUMLHR_NLE:
			strcpy(dram_model, "Hynix H9HCNNNBPUMLHR-NLE 4GB");
			break;
		case LPDDR4_ICOSA_4GB_MICRON_MT53B512M32D2NP_062_WTC:
			strcpy(dram_model, "Micron MT53B512M32D2NP-062 WT:C");
			break;
		case LPDDR4_ICOSA_6GB_SAMSUNG_K4FHE3D4HM_MGCH:
			strcpy(dram_model, "Samsung K4FHE3D4HM-MGCH 6GB");
			break;
		case LPDDR4_ICOSA_8GB_SAMSUNG_K4FBE3D4HM_MGXX:
			strcpy(dram_model, "Samsung K4FBE3D4HM-MGXX 8GB");
			break;
		default:
			strcpy(dram_model, "#FF8000 Unknown#");
			break;
		}
	}
	else
	{
		switch (dram_id)
		{
		// LPDDR4X 3733Mbps.
		case LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AM_MGCJ:
		case LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AM_MGCJ:
			strcpy(dram_model, "Samsung K4U6E3S4AM-MGCJ 4GB");
			break;
		case LPDDR4X_IOWA_8GB_SAMSUNG_K4UBE3D4AM_MGCJ:
		case LPDDR4X_HOAG_8GB_SAMSUNG_K4UBE3D4AM_MGCJ:
			strcpy(dram_model, "Samsung K4UBE3D4AM-MGCJ 8GB");
			break;
		case LPDDR4X_IOWA_4GB_HYNIX_H9HCNNNBKMMLHR_NME:
		case LPDDR4X_HOAG_4GB_HYNIX_H9HCNNNBKMMLHR_NME:
			strcpy(dram_model, "Hynix H9HCNNNBKMMLHR-NME 4GB");
			break;
		case LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D2NP_046_WTE: // 4266Mbps.
		case LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WTE: // 4266Mbps.
			strcpy(dram_model, "Micron MT53E512M32D2NP-046 WT:E");
			break;

		// LPDDR4X 4266Mbps
		case LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AA_MGCL:
		case LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AA_MGCL:
		case LPDDR4X_AULA_4GB_SAMSUNG_K4U6E3S4AA_MGCL:
			strcpy(dram_model, "Samsung K4U6E3S4AA-MGCL 4GB");
			break;
		case LPDDR4X_IOWA_8GB_SAMSUNG_K4UBE3D4AA_MGCL:
		case LPDDR4X_HOAG_8GB_SAMSUNG_K4UBE3D4AA_MGCL:
		case LPDDR4X_AULA_8GB_SAMSUNG_K4UBE3D4AA_MGCL:
			strcpy(dram_model, "Samsung K4UBE3D4AA-MGCL 8GB");
			break;
		case LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AB_MGCL:
		case LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AB_MGCL:
		case LPDDR4X_AULA_4GB_SAMSUNG_K4U6E3S4AB_MGCL:
			strcpy(dram_model, "Samsung K4U6E3S4AB-MGCL 4GB");
			break;
		case LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D2NP_046_WTF:
		case LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WTF:
		case LPDDR4X_AULA_4GB_MICRON_MT53E512M32D2NP_046_WTF:
			strcpy(dram_model, "Micron MT53E512M32D2NP-046 WT:F");
			break;
		case LPDDR4X_HOAG_4GB_HYNIX_H9HCNNNBKMMLXR_NEE: // Replaced from Copper.
		case LPDDR4X_AULA_4GB_HYNIX_H9HCNNNBKMMLXR_NEE: // Replaced from Copper.
		case LPDDR4X_IOWA_4GB_HYNIX_H9HCNNNBKMMLXR_NEE: // Replaced from Copper.
			strcpy(dram_model, "Hynix H9HCNNNBKMMLXR-NEE 4GB");
			break;
		case LPDDR4X_IOWA_4GB_HYNIX_H54G46CYRBX267:
		case LPDDR4X_HOAG_4GB_HYNIX_H54G46CYRBX267:
		case LPDDR4X_AULA_4GB_HYNIX_H54G46CYRBX267:
			strcpy(dram_model, "Hynix H54G46CYRBX267 4GB");
			break;
		case LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D1NP_046_WTB:
		case LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D1NP_046_WTB:
		case LPDDR4X_AULA_4GB_MICRON_MT53E512M32D1NP_046_WTB:
			strcpy(dram_model, "Micron MT53E512M32D1NP-046 WT:B");
			break;

		default:
			strcpy(dram_model, "#FF8000 Contact me!#");
			break;
		}
	}

	// Count burnt fuses.
	u8 burnt_fuses_7 = bit_count(fuse_read_odm(7));
	u8 burnt_fuses_6 = bit_count(fuse_read_odm(6));

	// Check if overburnt.
	u8 burnt_fuses_hos = (fuse_read_odm(7) & ~bit_count_mask(burnt_fuses_7)) ? 255 : burnt_fuses_7;

	//! TODO: Update on anti-downgrade fuses change.
	switch (burnt_fuses_hos)
	{
	case 0:
		strcpy(fuses_hos_version, "#96FF00 Golden sample#");
		break;
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
		strcpy(fuses_hos_version, "11.0.0 - 12.0.1");
		break;
	case 15:
		strcpy(fuses_hos_version, "12.0.2 - 13.2.0");
		break;
	case 16:
		strcpy(fuses_hos_version, "13.2.1 - 14.1.2");
		break;
	case 17:
		strcpy(fuses_hos_version, "15.0.0 - 15.0.1");
		break;
	case 18:
		strcpy(fuses_hos_version, "16.0.0 - 16.1.0");
		break;
	case 19:
		strcpy(fuses_hos_version, "17.0.0 - 18.1.0");
		break;
	case 20:
		strcpy(fuses_hos_version, "19.0.0 - 19.0.1");
		break;
	case 21:
		strcpy(fuses_hos_version, "20.0.0 - 20.5.0");
		break;
	case 22:
		strcpy(fuses_hos_version, "21.0.0+");
		break;
	case 255:
		strcpy(fuses_hos_version, "#FFD000 Overburnt#");
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

	char sbk_key[64];
	char dev_key[32];
	if (FUSE(FUSE_PRIVATE_KEY0) == 0xFFFFFFFF &&
		FUSE(FUSE_PRIVATE_KEY1) == 0xFFFFFFFF &&
		FUSE(FUSE_PRIVATE_KEY2) == 0xFFFFFFFF &&
		FUSE(FUSE_PRIVATE_KEY3) == 0xFFFFFFFF &&
		FUSE(FUSE_PRIVATE_KEY4) == 0xFFFFFFFF)
	{
		strcpy(sbk_key, "Can't be read (locked out)");
		strcpy(dev_key, "Can't be read (locked out)");
	}
	else
	{
		s_printf(sbk_key, "%08X%08X%08X%08X",
			byte_swap_32(FUSE(FUSE_PRIVATE_KEY0)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY1)),
			byte_swap_32(FUSE(FUSE_PRIVATE_KEY2)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY3)));
		s_printf(dev_key, "%08X",  byte_swap_32(FUSE(FUSE_PRIVATE_KEY4)));
	}

	u32 chip_id = APB_MISC(APB_MISC_GP_HIDREV);
	char *chip_name = hw_get_chip_id() == GP_HIDREV_MAJOR_T210 ? "T210 (Erista)" : "T210B01 (Mariko)";

	// Parse fuses and display them.
	s_printf(txt_buf,
		"%02X - %s - M%d A%02d\n"
		"%X - %s - %s\n"
		"%02d - %s\n"
		"%d | %d - HOS: %s\n"
		"%08X %08X %08X\n"
		"%s\n%s\n"
		"%08X%08X%08X%08X\n"
		"%08X%08X%08X%08X\n"
		"%d\n"
		"%s\n"
		"%d.%02d (0x%X)\n"
		"%d.%02d (0x%X)\n"
		"%d\n%d\n%d\n"
		"%d\n0x%X\n%d\n"
		"%d (%d)\n"
		"%d (%d)\n"
		"%d (%d)\n"
		"%d\n%d\n%d (0x%X)\n"
		"%d\n%d\n%d",
		(chip_id >> 8) & 0xFF, chip_name, (chip_id >> 4) & 0xF, (chip_id >> 16) & 0xF,
		FUSE(FUSE_SKU_INFO), sku, fuse_read_hw_state() ? "Dev" : "Retail",
		dram_id, dram_model,
		burnt_fuses_7, burnt_fuses_6, fuses_hos_version,
		fuse_read_odm(4), fuse_read_odm(6), fuse_read_odm(7),
		sbk_key, dev_key,
		byte_swap_32(FUSE(FUSE_PUBLIC_KEY0)), byte_swap_32(FUSE(FUSE_PUBLIC_KEY1)),
		byte_swap_32(FUSE(FUSE_PUBLIC_KEY2)), byte_swap_32(FUSE(FUSE_PUBLIC_KEY3)),
		byte_swap_32(FUSE(FUSE_PUBLIC_KEY4)), byte_swap_32(FUSE(FUSE_PUBLIC_KEY5)),
		byte_swap_32(FUSE(FUSE_PUBLIC_KEY6)), byte_swap_32(FUSE(FUSE_PUBLIC_KEY7)),
		fuse_read_odm_keygen_rev(),
		((FUSE(FUSE_RESERVED_SW) & 0x80) || h_cfg.t210b01) ? "XUSB" : "USB2",
		(FUSE(FUSE_OPT_FT_REV)  >> 5) & 0x3F, FUSE(FUSE_OPT_FT_REV) & 0x1F, FUSE(FUSE_OPT_FT_REV),
		(FUSE(FUSE_OPT_CP_REV)  >> 5) & 0x3F, FUSE(FUSE_OPT_CP_REV) & 0x1F, FUSE(FUSE_OPT_CP_REV),
		FUSE(FUSE_CPU_SPEEDO_0_CALIB), FUSE(FUSE_CPU_SPEEDO_1_CALIB), FUSE(FUSE_CPU_SPEEDO_2_CALIB),
		FUSE(FUSE_SOC_SPEEDO_0_CALIB), FUSE(FUSE_SOC_SPEEDO_1_CALIB), FUSE(FUSE_SOC_SPEEDO_2_CALIB),
		FUSE(FUSE_CPU_IDDQ_CALIB) * 4, FUSE(FUSE_CPU_IDDQ_CALIB),
		FUSE(FUSE_SOC_IDDQ_CALIB) * 4, FUSE(FUSE_SOC_IDDQ_CALIB),
		FUSE(FUSE_GPU_IDDQ_CALIB) * 5, FUSE(FUSE_GPU_IDDQ_CALIB),
		FUSE(FUSE_OPT_VENDOR_CODE), FUSE(FUSE_OPT_FAB_CODE), lot_bin, FUSE(FUSE_OPT_LOT_CODE_0),
		FUSE(FUSE_OPT_WAFER_ID), FUSE(FUSE_OPT_X_COORDINATE), FUSE(FUSE_OPT_Y_COORDINATE));

	lv_label_set_text(lb_val, txt_buf);

	lv_obj_set_width(lb_val, lv_obj_get_width(val));
	lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_t *desc2 = lv_cont_create(win, NULL);
	lv_obj_set_size(desc2, LV_HOR_RES / 2 / 5 * 4, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc2 = lv_label_create(desc2, NULL);
	lv_label_set_long_mode(lb_desc2, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc2, true);

	// Prepare DRAM info.
	emc_mr_data_t ram_vendor  = sdram_read_mrx(MR5_MAN_ID);
	emc_mr_data_t ram_rev0    = sdram_read_mrx(MR6_REV_ID1);
	emc_mr_data_t ram_rev1    = sdram_read_mrx(MR7_REV_ID2);
	emc_mr_data_t ram_density = sdram_read_mrx(MR8_DENSITY);
	u32 ranks    = EMC(EMC_ADR_CFG) + 1;
	u32 channels = (EMC(EMC_FBIO_CFG7) >> 1) & 3;
	channels = (channels & 1) + ((channels & 2) >> 1);
	s_printf(txt_buf, "#00DDFF %s SDRAM ##FF8000 (Module 0 | 1):#\n#FF8000 Vendor:# ", h_cfg.t210b01 ? "LPDDR4X" : "LPDDR4");
	switch (ram_vendor.chip0.rank0_ch0)
	{
	case 1:
		strcat(txt_buf, "Samsung");
		break;
/*
	case 5:
		strcat(txt_buf, "Nanya");
		break;
*/
	case 6:
		strcat(txt_buf, "Hynix");
		break;
/*
	case 8:
		strcat(txt_buf, "Winbond");
		break;
	case 9:
		strcat(txt_buf, "ESMT");
		break;
	case 19:
		strcat(txt_buf, "CXMT");
		break;
	case 26:
		strcat(txt_buf, "Xi'an UniIC");
		break;
	case 27:
		strcat(txt_buf, "ISSI");
		break;
	case 28:
		strcat(txt_buf, "JSC");
		break;
	case 197:
		strcat(txt_buf, "SINKER");
		break;
	case 229:
		strcat(txt_buf, "Dosilicon");
		break;
	case 248:
		strcat(txt_buf, "Fidelix");
		break;
	case 249:
		strcat(txt_buf, "Ultra Memory");
		break;
	case 253:
		strcat(txt_buf, "AP Memory");
		break;
 */
	case 255:
		strcat(txt_buf, "Micron");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "#FF8000 Unknown# (%d)", ram_vendor.chip0.rank0_ch0);
		break;
	}
	strcat(txt_buf, " #FF8000 |# ");
	switch (ram_vendor.chip1.rank0_ch0)
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
		s_printf(txt_buf + strlen(txt_buf), "#FF8000 Unknown# (%d)", ram_vendor.chip1.rank0_ch0);
		break;
	}

	s_printf(txt_buf + strlen(txt_buf), "\n#FF8000 Rev ID:#  %X.%02X #FF8000 |# %X.%02X\n#FF8000 Density:# ",
		ram_rev0.chip0.rank0_ch0, ram_rev1.chip0.rank0_ch0, ram_rev0.chip1.rank0_ch0, ram_rev1.chip1.rank0_ch0);

	u32 actual_ranks = (ram_vendor.chip0.rank0_ch0 == ram_vendor.chip0.rank1_ch0 &&
						ram_vendor.chip0.rank0_ch1 == ram_vendor.chip0.rank1_ch1 &&
						ram_rev0.chip0.rank0_ch0 == ram_rev0.chip0.rank1_ch0 &&
						ram_rev0.chip0.rank0_ch1 == ram_rev0.chip0.rank1_ch1 &&
						ram_rev1.chip0.rank0_ch0 == ram_rev1.chip0.rank1_ch0 &&
						ram_rev1.chip0.rank0_ch1 == ram_rev1.chip0.rank1_ch1 &&
						ram_density.chip0.rank0_ch0 == ram_density.chip0.rank1_ch0 &&
						ram_density.chip0.rank0_ch1 == ram_density.chip0.rank1_ch1)
					   ? 2 : 1;
	bool rank_bad = ranks != actual_ranks;
	s_printf(txt_buf + strlen(txt_buf), "%s %d x %s", rank_bad ? "#FFDD00" : "", actual_ranks * channels, rank_bad ? "#" : "");

	switch ((ram_density.chip0.rank0_ch0 & 0x3C) >> 2)
	{
	case 2:
		strcat(txt_buf, "512MB");
		break;
	case 3:
		strcat(txt_buf, "768MB");
		break;
	case 4:
		strcat(txt_buf, "1GB");
		break;
	case 5:
		strcat(txt_buf, "1.5GB");
		break;
	case 6:
		strcat(txt_buf, "2GB");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "Unk (%d)", (ram_density.chip0.rank0_ch0 & 0x3C) >> 2);
		break;
	}

	actual_ranks = (ram_vendor.chip1.rank0_ch0 == ram_vendor.chip1.rank1_ch0 &&
					ram_vendor.chip1.rank0_ch1 == ram_vendor.chip1.rank1_ch1 &&
					ram_rev0.chip1.rank0_ch0 == ram_rev0.chip1.rank1_ch0 &&
					ram_rev0.chip1.rank0_ch1 == ram_rev0.chip1.rank1_ch1 &&
					ram_rev1.chip1.rank0_ch0 == ram_rev1.chip1.rank1_ch0 &&
					ram_rev1.chip1.rank0_ch1 == ram_rev1.chip1.rank1_ch1 &&
					ram_density.chip1.rank0_ch0 == ram_density.chip1.rank1_ch0 &&
					ram_density.chip1.rank0_ch1 == ram_density.chip1.rank1_ch1)
				   ? 2 : 1;
	rank_bad = ranks != actual_ranks;

	s_printf(txt_buf + strlen(txt_buf), " #FF8000 |# %s %d x %s", rank_bad ? "#FFDD00" : "", actual_ranks * channels, rank_bad ? "#" : "");

	switch ((ram_density.chip1.rank0_ch0 & 0x3C) >> 2)
	{
	case 2:
		strcat(txt_buf, "512MB");
		break;
	case 3:
		strcat(txt_buf, "768MB");
		break;
	case 4:
		strcat(txt_buf, "1GB");
		break;
	case 5:
		strcat(txt_buf, "1.5GB");
		break;
	case 6:
		strcat(txt_buf, "2GB");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "Unk (%d)", (ram_density.chip1.rank0_ch0 & 0x3C) >> 2);
		break;
	}
	strcat(txt_buf, "\n\n");

	// Prepare display info.
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
		case 0x97:
			strcat(txt_buf, "-???");
			break;
		case 0x98:
			strcat(txt_buf, "-???");
			break;
		case 0x99:
			strcat(txt_buf, "-???");
			break;
		default:
			strcat(txt_buf, " #FFDD00 Contact me!#");
			break;
		}
		break;
	case PANEL_AUO_A062TAN01:
		strcat(txt_buf, "AUO A062TAN");
		switch (display_rev)
		{
		case 0x93:
			strcat(txt_buf, "00");
			break;
		case 0x94:
			strcat(txt_buf, "01");
			break;
		case 0x95:
			strcat(txt_buf, "02");
			break;
		case 0x96:
			strcat(txt_buf, "??");
			break;
		case 0x97:
			strcat(txt_buf, "??");
			break;
		case 0x98:
			strcat(txt_buf, "??");
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
		strcat(txt_buf, "AUO A055TAN");
		s_printf(txt_buf + strlen(txt_buf), "%02d", display_rev - 0x92);
		break;
	case PANEL_SHP_LQ055T1SW10:
		strcat(txt_buf, "Sharp LQ055T1SW10");
		break;
	case PANEL_SAM_AMS699VC01:
		strcat(txt_buf, "Samsung AMS699VC01");
		break;
	case PANEL_OEM_CLONE_6_2:
		strcat(txt_buf, "#FFDD00 OEM Clone 6.2\"#");
		break;
	case PANEL_OEM_CLONE_5_5:
		strcat(txt_buf, "#FFDD00 OEM Clone 5.5\"#");
		break;
	case PANEL_OEM_CLONE:
		strcat(txt_buf, "#FFDD00 OEM Clone#");
		break;
	case 0xCCCC:
		strcat(txt_buf, "#FFDD00 Failed to get info!#");
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
		case (PANEL_SAM_AMS699VC01 & 0xFF):
			strcat(txt_buf, "Samsung ");
			break;
		}
		strcat(txt_buf, "Unknown #FFDD00 Contact me!#");
		break;
	}

	s_printf(txt_buf + strlen(txt_buf), "\n#FF8000 ID:# #96FF00 %02X# %02X #96FF00 %02X#",
		nyx_str->info.disp_id & 0xFF, (nyx_str->info.disp_id >> 8) & 0xFF, (nyx_str->info.disp_id >> 16) & 0xFF);

	touch_fw_info_t touch_fw;
	touch_panel_info_t *touch_panel;
	bool panel_ic_paired = false;

	// Prepare touch panel/ic info.
	if (!touch_get_fw_info(&touch_fw))
	{
		strcat(txt_buf, "\n\n#00DDFF Touch Panel:#\n#FF8000 Model:# ");

		touch_panel = touch_get_panel_vendor();
		if (touch_panel)
		{
			if ((u8)touch_panel->idx == (u8)-2) // Touch panel not found, print gpios.
			{
				s_printf(txt_buf + strlen(txt_buf), "%2X%2X%2X #FFDD00 Contact me!#",
					touch_panel->gpio0, touch_panel->gpio1, touch_panel->gpio2);
				touch_panel = NULL;
			}
			else
				strcat(txt_buf, touch_panel->vendor);
		}
		else
			strcat(txt_buf, "#FFDD00 Error!#");

		s_printf(txt_buf + strlen(txt_buf), "\n#FF8000 ID:# %02X.%02X.%02X.%02X (",
			(touch_fw.fw_id >> 24) & 0xFF, (touch_fw.fw_id >> 16) & 0xFF, (touch_fw.fw_id >> 8) & 0xFF, touch_fw.fw_id & 0xFF);

		// Check panel pair info.
		switch (touch_fw.fw_id)
		{
		case 0x00100100:
			strcat(txt_buf, "4CD60D/0");
			if (touch_panel)
				panel_ic_paired = (u8)touch_panel->idx == (u8)-1;
			break;
		case 0x00100200: // 4CD 1602.
		case 0x00120100:
		case 0x32000001:
			strcat(txt_buf, "4CD60D/1");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 0; // NISSHA NFT-K12D.
			break;
		// case 0x98000004: // New 6.2" panel?
		// case 0x50000001:
		// case 0x50000002:
		// 	strcat(txt_buf, "FST2 UNK");
		// 	if (touch_panel)
		// 		panel_ic_paired = touch_panel->idx == 0;
		// 	break;
		case 0x001A0300:
		case 0x32000102:
			strcat(txt_buf, "4CD60D/2");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 1; // GiS GGM6 B2X.
			break;
		case 0x00290100:
		case 0x32000302:
			strcat(txt_buf, "4CD60D/3");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 2; // NISSHA NBF-K9A.
			break;
		case 0x31051820:
		case 0x32000402:
			strcat(txt_buf, "4CD60D/4");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 3; // GiS 5.5".
			break;
		case 0x32000501:
		case 0x33000502:
		case 0x33000503:
		case 0x33000510:
			strcat(txt_buf, "4CD60D/5");
			if (touch_panel)
				panel_ic_paired = touch_panel->idx == 4; // Samsung BH2109.
			break;
		default:
			strcat(txt_buf, "#FF8000 Contact me#");
			break;
		}

		s_printf(txt_buf + strlen(txt_buf), " - %s)\n#FF8000 FTB ver:# %04X\n#FF8000 FW rev:# %04X",
			panel_ic_paired ? "Paired" : "#FFDD00 Error#",
			touch_fw.ftb_ver,
			byte_swap_16(touch_fw.fw_rev)); // Byte swapping makes more sense here.
	}
	else
		strcat(txt_buf, "\n\n#FFDD00 Failed to get touch info!#");

	// Check if patched unit.
	if (!fuse_check_patched_rcm())
		strcat(txt_buf, "\n\n#96FF00 This unit is exploitable#\n#96FF00 to the RCM bug!#");
	else
		strcat(txt_buf, "\n\n#FF8000 This unit is patched#\n#FF8000 to the RCM bug!#");

	lv_label_set_text(lb_desc2, txt_buf);

	free(txt_buf);

	lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
	lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 4, 0);

	return LV_RES_OK;
}

static char *ipatches_txt;
static void _ipatch_process(u32 offset, u32 value)
{
	s_printf(ipatches_txt + strlen(ipatches_txt), "%6X     %4X    ", IROM_BASE + offset, value);
	u8 lo = value & 0xFF;
	switch (value >> 8)
	{
	case 0x20:
		s_printf(ipatches_txt + strlen(ipatches_txt), "MOVS R0, ##0x%02X", lo);
		break;
	case 0x21:
		s_printf(ipatches_txt + strlen(ipatches_txt), "MOVS R1, ##0x%02X", lo);
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

	char *txt_buf = (char *)malloc(SZ_4K);
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

static lv_res_t _launch_lockpick_action(lv_obj_t *btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	mbox_action(btns, txt);

	if (btn_idx == 1)
	{
		lv_obj_t *list = lv_list_create(NULL, NULL);
		lv_obj_set_size(list, 1, 1);
		lv_list_set_single_mode(list, true);
		lv_list_add(list, NULL, "Lockpick_RCM.bin", NULL);
		lv_obj_t *btn;
		btn = lv_list_get_prev_btn(list, NULL);
		launch_payload(btn);
		lv_obj_del(list);
	}

	return LV_RES_INV;
}

static lv_res_t _create_mbox_lockpick(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\251", "\222Continue", "\222Close", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	lv_mbox_set_text(mbox, "#FF8000 Lockpick RCM#\n\nThis will launch Lockpick RCM.\nDo you want to continue?\n\n"
		"To return back from lockpick use\n#96FF00 Reboot to hekate#.");

	lv_mbox_add_btns(mbox, mbox_btn_map, _launch_lockpick_action);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_emmc_sandisk_report(lv_obj_t * btn)
{
	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 6;

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\251", "\222Close", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 8);

	lv_mbox_set_text(mbox, "#C7EA46 Sandisk Device Report#");

	u8 *buf = zalloc(EMMC_BLOCKSIZE);
	char *txt_buf = (char *)malloc(SZ_32K);
	char *txt_buf2 = (char *)malloc(SZ_32K);
	txt_buf[0] = 0;
	txt_buf2[0] = 0;

	// Create SoC Info container.
	lv_obj_t *h1 = lv_cont_create(mbox, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 7);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t * lb_desc = lv_label_create(h1, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_label_set_style(lb_desc, &monospace_text);
	lv_obj_set_width(lb_desc, LV_HOR_RES / 9 * 4);

	lv_obj_t * lb_desc2 = lv_label_create(h1, NULL);
	lv_label_set_long_mode(lb_desc2, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc2, true);
	lv_label_set_style(lb_desc2, &monospace_text);
	lv_obj_set_width(lb_desc2, LV_HOR_RES / 9 * 3);
	lv_obj_align(lb_desc2, lb_desc, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);


	if (!emmc_initialize(false))
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init eMMC!#");

		goto out;
	}

	int res = sdmmc_storage_vendor_sandisk_report(&emmc_storage, buf);
	emmc_end();

	if (!res)
	{
		lv_label_set_text(lb_desc, "#FFDD00 Device Report not supported!#");
		lv_label_set_text(lb_desc2, " ");

		goto out;
	}

	mmc_sandisk_report_t *rpt = (mmc_sandisk_report_t *)buf;

	u8  fw_update_date[13] = {0};
	u8  fw_update_time[9] = {0};
	memcpy(fw_update_date, rpt->fw_update_date, sizeof(rpt->fw_update_date));
	memcpy(fw_update_time, rpt->fw_update_time, sizeof(rpt->fw_update_time));

	s_printf(txt_buf,
		"#00DDFF Device report#\n"
		//"#FF8000 Average Erases SYS:#    %d\n"
		"#FF8000 Average Erases SLC:#    %d\n"
		"#FF8000 Average Erases MLC:#    %d\n"
		//"#FF8000 Read Reclaims SYS:#     %d\n"
		"#FF8000 Read Reclaims SLC:#     %d\n"
		"#FF8000 Read Reclaims MLC:#     %d\n"
		"#FF8000 Bad Blocks Factory:#    %d\n"
		"#FF8000 Bad Blocks SYS:#        %d\n"
		"#FF8000 Bad Blocks SLC:#        %d\n"
		"#FF8000 Bad Blocks MLC:#        %d\n"
		"#FF8000 FW Updates:#            %d\n"
		"#FF8000 FW Buildtime:#          %s %s\n"
		"#FF8000 Total Writes:#          %d MB\n"
		//"#FF8000 Voltage Drops:#         %d\n"
		//"#FF8000 Voltage Droops:#        %d\n"
		//"#FF8000 VD Failed Recovers:#    %d\n"
		//"#FF8000 VD Recover Operations:# %d\n"
		"#FF8000 Total Writes SLC:#      %d MB\n"
		"#FF8000 Total Writes MLC:#      %d MB\n"
		"#FF8000 BigFile limit status:#  %d\n"
		"#FF8000 Average Erases Hybrid:# %d",

		//rpt->avg_erase_cycles_sys,
		rpt->avg_erase_cycles_slc,
		rpt->avg_erase_cycles_mlc,
		//rpt->read_reclaim_cnt_sys,
		rpt->read_reclaim_cnt_slc,
		rpt->read_reclaim_cnt_mlc,
		rpt->bad_blocks_factory,
		rpt->bad_blocks_sys,
		rpt->bad_blocks_slc,
		rpt->bad_blocks_mlc,
		rpt->fw_updates_cnt,
		fw_update_date,
		fw_update_time,
		rpt->total_writes_100mb * 100,
		//rpt->vdrops,
		//rpt->vdroops,
		//rpt->vdrops_failed_data_rec,
		//rpt->vdrops_data_rec_ops,
		rpt->total_writes_slc_100mb * 100,
		rpt->total_writes_mlc_100mb * 100,
		rpt->mlc_bigfile_mode_limit_exceeded,
		rpt->avg_erase_cycles_hybrid);

	u8 advanced_report = 0;
	for (u32 i = 0; i < sizeof(mmc_sandisk_advanced_report_t); i++)
		advanced_report |= *(u8 *)((u8 *)&rpt->advanced + i);

	if (advanced_report)
	{
		s_printf(txt_buf2,
			"#00DDFF Advanced Health Status#\n"
			"#FF8000 Power ups:#             %d\n"
			//"#FF8000 Maximum Erases SYS:#    %d\n"
			"#FF8000 Maximum Erases SLC:#    %d\n"
			"#FF8000 Maximum Erases MLC:#    %d\n"
			//"#FF8000 Minimum Erases SYS:#    %d\n"
			"#FF8000 Minimum Erases SLC:#    %d\n"
			"#FF8000 Minimum Erases MLC:#    %d\n"
			"#FF8000 Maximum Erases EUDA:#   %d\n"
			"#FF8000 Minimum Erases EUDA:#   %d\n"
			"#FF8000 Average Erases EUDA:#   %d\n"
			"#FF8000 Read Reclaims EUDA:#    %d\n"
			"#FF8000 Bad Blocks EUDA:#       %d\n"
			//"#FF8000 Pre EOL State EUDA:#    %d\n"
			//"#FF8000 Pre EOL State SYS:#     %d\n"
			//"#FF8000 Pre EOL State MLC:#     %d\n"
			"#FF8000 Uncorrectable ECC:#     %d\n"
			"#FF8000 Temperature Now:#       %d oC\n"
			//"#FF8000 Temperature Min:#       %d oC\n"
			"#FF8000 Temperature Max:#       %d oC\n"
			"#FF8000 Health Level EUDA:#     %d%%\n"
			//"#FF8000 Health Level SYS:#      %d%%\n"
			"#FF8000 Health Level MLC:#      %d%%",

			rpt->advanced.power_inits,
			//rpt->advanced.max_erase_cycles_sys,
			rpt->advanced.max_erase_cycles_slc,
			rpt->advanced.max_erase_cycles_mlc,
			//rpt->advanced.min_erase_cycles_sys,
			rpt->advanced.min_erase_cycles_slc,
			rpt->advanced.min_erase_cycles_mlc,
			rpt->advanced.max_erase_cycles_euda,
			rpt->advanced.min_erase_cycles_euda,
			rpt->advanced.avg_erase_cycles_euda,
			rpt->advanced.read_reclaim_cnt_euda,
			rpt->advanced.bad_blocks_euda,
			//rpt->advanced.pre_eol_euda,
			//rpt->advanced.pre_eol_sys,
			//rpt->advanced.pre_eol_mlc,
			rpt->advanced.uncorrectable_ecc,
			rpt->advanced.temperature_now,
			//rpt->advanced.temperature_min,
			rpt->advanced.temperature_max,
			rpt->advanced.health_pct_euda ? 101 - rpt->advanced.health_pct_euda : 0,
			//rpt->advanced.health_pct_sys ? 101 - rpt->advanced.health_pct_sys : 0,
			rpt->advanced.health_pct_mlc ? 101 - rpt->advanced.health_pct_mlc : 0);
	}
	else
		strcpy(txt_buf2, "#00DDFF Advanced Health Status#\n#FFDD00 Empty!#");

	lv_label_set_text(lb_desc, txt_buf);
	lv_label_set_text(lb_desc2, txt_buf2);

out:
	free(buf);
	free (txt_buf);
	free (txt_buf2);

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action); // Important. After set_text.
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_obj_t *bench_sd_err_label = NULL;

static lv_res_t _create_mbox_benchmark(bool sd_bench)
{
	sdmmc_storage_t *storage;

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES * 3 / 7);

	char *txt_buf = (char *)malloc(SZ_16K);

	s_printf(txt_buf, "#FF8000 %s Benchmark#\n[Raw Reads] Abort: VOL- & VOL+", sd_bench ? "SD Card" : "eMMC");

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

		// Re-initialize to update trimmers.
		sd_end();
		res = !sd_mount();
	}
	else
	{
		storage = &emmc_storage;
		res = !emmc_initialize(false);
		if (!res)
			emmc_set_partition(EMMC_GPP);
	}

	if (res)
	{
		lv_mbox_set_text(mbox, "#FFDD00 Failed to init Storage!#");
		goto out;
	}

	// Set benchmark parameters.
	const u32 sct_blk_seq = 0x8000;   // 16MB.  A2 spec denotes 4MB, but using older big AU.
	const u32 sct_blk_4kb = 8;        // 4KB.
	const u32 sct_rem_seq = 0x200000; // 1GB.   A2 spec.
	const u32 sct_rem_4kb = 0x80000;  // 256MB. A2 spec.
	const u32 sct_num_1mb = 0x800;    // 1MB.
	const u32 size_bytes_seq = sct_rem_seq * SDMMC_DAT_BLOCKSIZE;
	const u32 size_bytes_4kb = sct_rem_4kb * SDMMC_DAT_BLOCKSIZE;

	// Set calculation divider. 1000 or 1024. (Does not affect IOPS).
	u32 mb_div = 1000; // Unfortunately most software uses fake MB.

	char *mbs_text;
	switch (mb_div)
	{
	case 1000:
		mbs_text = "MB/s";
		break;
	case 1024:
		mbs_text = "MiB/s";
		break;
	}

	// Set actual div in MB/MiB.
	mb_div *= mb_div;

	int error = 0;
	u32 iters = 3;
	u32 offset_chunk_start = ALIGN_DOWN(storage->sec_cnt / 3, sct_blk_seq); // Align to block.
	if (storage->sec_cnt < 0xC00000)
		iters -= 2; // 4GB card.

	u32  rnd_off_cnt    = sct_rem_4kb / sct_blk_4kb;
	u32 *random_offsets = malloc(rnd_off_cnt * sizeof(u32));
	u32 *times_taken_4k = malloc(rnd_off_cnt * sizeof(u32));

	for (u32 iter_curr = 0; iter_curr < iters; iter_curr++)
	{
		u32 pct = 0;
		u32 prevPct = 200;
		u32 timer = 0;
		u32 lba_curr = 0;
		u32 sector_off = offset_chunk_start * iter_curr;
		u32 sector_num = sct_blk_seq;
		u32 data_remaining = sct_rem_seq;

		s_printf(txt_buf + strlen(txt_buf), "#C7EA46 %d/3# - Sector Offset #C7EA46 %08X#:\n", iter_curr + 1, sector_off);

		u32 render_min_ms = 66;
		u32 render_timer  = get_tmr_ms() + render_min_ms;
		while (data_remaining)
		{
			u32 time_taken = get_tmr_us();
			error = !sdmmc_storage_read(storage, sector_off + lba_curr, sector_num, (u8 *)MIXD_BUF_ALIGNED);
			time_taken = get_tmr_us() - time_taken;
			timer += time_taken;

			manual_system_maintenance(false);
			data_remaining -= sector_num;
			lba_curr += sector_num;

			pct = (lba_curr * 100) / sct_rem_seq;
			if (pct != prevPct && render_timer < get_tmr_ms())
			{
				lv_bar_set_value(bar, pct);
				manual_system_maintenance(true);
				render_timer = get_tmr_ms() + render_min_ms;

				prevPct = pct;

				if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
					error = -1;
			}

			if (error)
				goto error;
		}
		lv_bar_set_value(bar, 100);

		// Calculate rate for transfer.
		u32 rate_1k = (u64)size_bytes_seq * 1000 * 1000 * 1000 / mb_div / timer;
		s_printf(txt_buf + strlen(txt_buf), " SEQ 16MB - Rate: #C7EA46 %3d.%02d %s#",
			rate_1k / 1000, (rate_1k % 1000) / 10, mbs_text);
		lv_label_set_text(lbl_status, txt_buf);
		lv_obj_align(lbl_status, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		manual_system_maintenance(true);

		pct = 0;
		prevPct = 200;
		timer = 0;
		lba_curr = 0;
		sector_num = sct_blk_4kb;
		data_remaining = sct_rem_4kb;

		u32 loop_idx = 0;
		render_timer = get_tmr_ms() + render_min_ms;
		while (data_remaining)
		{
			u32 time_taken = get_tmr_us();
			error = !sdmmc_storage_read(storage, sector_off + lba_curr, sector_num, (u8 *)MIXD_BUF_ALIGNED);
			time_taken = get_tmr_us() - time_taken;

			timer += time_taken;
			times_taken_4k[loop_idx++] = time_taken;

			manual_system_maintenance(false);
			data_remaining -= sector_num;
			lba_curr += sector_num;

			pct = (lba_curr * 100) / sct_rem_4kb;
			if (pct != prevPct && render_timer < get_tmr_ms())
			{
				lv_bar_set_value(bar, pct);
				manual_system_maintenance(true);
				render_timer = get_tmr_ms() + render_min_ms;

				prevPct = pct;

				if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
					error = -1;
			}

			if (error)
				goto error;
		}
		lv_bar_set_value(bar, 100);

		qsort(times_taken_4k, loop_idx, sizeof(u32), qsort_compare_int); // Use int for faster compare. Value can't exceed 2s.

		u32 pct95 = 0;
		for (u32 i = 0; i < loop_idx * 95 / 100; i++)
			pct95 += times_taken_4k[i];
		pct95 /= loop_idx * 95 / 100;

		u32 pct05 = 0;
		for (u32 i = 0; i < loop_idx * 5 / 100; i++)
			pct05 += times_taken_4k[loop_idx - 1 - i];
		pct05 /= loop_idx * 5 / 100;

		// Calculate rate and IOPS for transfer.
		rate_1k = (u64)size_bytes_4kb * 1000 * 1000 * 1000 / mb_div / timer;
		u32 iops = ((u64)(sct_rem_4kb / sct_num_1mb) * 1024 * 1000 * 1000 * 1000) / (4096 / 1024) / timer / 1000;
		s_printf(txt_buf + strlen(txt_buf), "        AVG #C7EA46 95th#  #FF3C28 5th#\n");
		s_printf(txt_buf + strlen(txt_buf), " SEQ  4KB - Rate: #C7EA46 %3d.%02d %s# IOPS: #C7EA46 %4d# %4d %4d \n",
			rate_1k / 1000, (rate_1k % 1000) / 10, mbs_text, iops, 1000000 / pct95, 1000000 / pct05);
		lv_label_set_text(lbl_status, txt_buf);
		lv_obj_align(lbl_status, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		manual_system_maintenance(true);

		u32 lba_idx = 0;
		u32 random_numbers[4];
		for (u32 i = 0; i < rnd_off_cnt; i += 4)
		{
			// Generate new random numbers.
			while (!se_gen_prng128(random_numbers))
				;
			// Clamp offsets to 256MB range.
			random_offsets[i + 0] = random_numbers[0] % sct_rem_4kb;
			random_offsets[i + 1] = random_numbers[1] % sct_rem_4kb;
			random_offsets[i + 2] = random_numbers[2] % sct_rem_4kb;
			random_offsets[i + 3] = random_numbers[3] % sct_rem_4kb;
		}

		pct = 0;
		prevPct = 200;
		timer = 0;
		data_remaining = sct_rem_4kb;

		loop_idx = 0;
		render_timer = get_tmr_ms() + render_min_ms;
		while (data_remaining)
		{
			u32 time_taken = get_tmr_us();
			error = !sdmmc_storage_read(storage, sector_off + random_offsets[lba_idx], sector_num, (u8 *)MIXD_BUF_ALIGNED);
			time_taken = get_tmr_us() - time_taken;

			timer += time_taken;
			times_taken_4k[loop_idx++] = time_taken;

			manual_system_maintenance(false);
			data_remaining -= sector_num;
			lba_idx++;

			pct = (lba_idx * 100) / rnd_off_cnt;
			if (pct != prevPct && render_timer < get_tmr_ms())
			{
				lv_bar_set_value(bar, pct);
				manual_system_maintenance(true);
				render_timer = get_tmr_ms() + render_min_ms;

				prevPct = pct;

				if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
					error = -1;
			}

			if (error)
				goto error;
		}
		lv_bar_set_value(bar, 100);

		qsort(times_taken_4k, loop_idx, sizeof(u32), qsort_compare_int); // Use int for faster compare. Value can't exceed 2s.

		pct95 = 0;
		for (u32 i = 0; i < loop_idx * 95 / 100; i++)
			pct95 += times_taken_4k[i];
		pct95 /= loop_idx * 95 / 100;

		pct05 = 0;
		for (u32 i = 0; i < loop_idx * 5 / 100; i++)
			pct05 += times_taken_4k[loop_idx - 1 - i];
		pct05 /= loop_idx * 5 / 100;

		// Calculate rate and IOPS for transfer.
		rate_1k = (u64)size_bytes_4kb * 1000 * 1000 * 1000 / mb_div / timer;
		iops = ((u64)(sct_rem_4kb / sct_num_1mb) * 1024 * 1000 * 1000 * 1000) / (4096 / 1024) / timer / 1000;
		s_printf(txt_buf + strlen(txt_buf), " RND  4KB - Rate: #C7EA46 %3d.%02d %s# IOPS: #C7EA46 %4d# %4d %4d \n",
			rate_1k / 1000, (rate_1k % 1000) / 10, mbs_text, iops, 1000000 / pct95, 1000000 / pct05);
		if (iter_curr == iters - 1)
			txt_buf[strlen(txt_buf) - 1] = 0; // Cut off last new line.
		lv_label_set_text(lbl_status, txt_buf);
		lv_obj_align(lbl_status, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		manual_system_maintenance(true);
	}

error:
	free(random_offsets);
	free(times_taken_4k);

	if (error)
	{
		if (error == -1)
			s_printf(txt_buf + strlen(txt_buf), "\n#FFDD00 Aborted!#");
		else
			s_printf(txt_buf + strlen(txt_buf), "\n#FFDD00 IO Error occurred!#");

		lv_label_set_text(lbl_status, txt_buf);
		lv_obj_align(lbl_status, NULL, LV_ALIGN_CENTER, 0, 0);
	}

	lv_obj_del(bar);

	if (sd_bench && error && error != -1)
		sd_end();
	if (sd_bench)
	{
		if (error && error != -1)
			sd_end();
		else
			sd_unmount();
	}
	else
		emmc_end();

out:
	s_printf(txt_buf, "#FF8000 %s Benchmark#\n[Raw Reads]", sd_bench ? "SD Card" : "eMMC");
	lv_mbox_set_text(mbox, txt_buf);

	// Update SDMMC error info in case it changed.
	if (sd_bench && bench_sd_err_label)
	{
		u16 *sd_errors = sd_get_error_count();
		s_printf(txt_buf, "\n%d (%d)\n%d (%d)\n%d (%d)",
			sd_errors[SD_ERROR_INIT_FAIL], nyx_str->info.sd_errors[SD_ERROR_INIT_FAIL],
			sd_errors[SD_ERROR_RW_FAIL],   nyx_str->info.sd_errors[SD_ERROR_RW_FAIL],
			sd_errors[SD_ERROR_RW_RETRY],  nyx_str->info.sd_errors[SD_ERROR_RW_RETRY]);
		lv_label_set_text(bench_sd_err_label, txt_buf);
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

	char *txt_buf = (char *)malloc(SZ_16K);
	txt_buf[0] = '\n';
	txt_buf[1] = 0;
	u16 *emmc_errors;

	if (!emmc_initialize(false))
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init eMMC!#");
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));
		emmc_errors = emmc_get_error_count();

		goto out_error;
	}

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
		lv_win_add_btn(win, NULL, SYMBOL_FILE_ALT" Device Report", _create_mbox_emmc_sandisk_report);
		break;
	case 0x90:
		strcat(txt_buf, "SK Hynix ");
		break;
	default:
		strcat(txt_buf, "Unknown ");
		break;
	}

	s_printf(txt_buf + strlen(txt_buf), "(%02X)\n%c%c%c%c%c%c (%02X)\n%d.%d\n%04X\n%02d/%04d\n\n",
		emmc_storage.cid.manfid,
		emmc_storage.cid.prod_name[0], emmc_storage.cid.prod_name[1], emmc_storage.cid.prod_name[2],
		emmc_storage.cid.prod_name[3], emmc_storage.cid.prod_name[4], emmc_storage.cid.prod_name[5],
		emmc_storage.cid.oemid,
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
		emmc_storage.ext_csd.max_enh_mult * EMMC_BLOCKSIZE / 1024,
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
	s_printf(txt_buf + strlen(txt_buf), "1: #96FF00 BOOT0#  Size: %6d KiB  Sectors: 0x%08X\n", boot_size / 1024, boot_size / EMMC_BLOCKSIZE);
	s_printf(txt_buf + strlen(txt_buf), "2: #96FF00 BOOT1#  Size: %6d KiB  Sectors: 0x%08X\n", boot_size / 1024, boot_size / EMMC_BLOCKSIZE);
	s_printf(txt_buf + strlen(txt_buf), "3: #96FF00 RPMB#   Size: %6d KiB  Sectors: 0x%08X\n", rpmb_size / 1024, rpmb_size / EMMC_BLOCKSIZE);
	s_printf(txt_buf + strlen(txt_buf), "0: #96FF00 GPP#    Size: %6d MiB  Sectors: 0x%08X\n", emmc_storage.sec_cnt >> SECTORS_TO_MIB_COEFF, emmc_storage.sec_cnt);
	strcat(txt_buf, "\n#00DDFF GPP (eMMC USER) Partition Table:#\n");

	emmc_set_partition(EMMC_GPP);
	LIST_INIT(gpt);
	emmc_gpt_parse(&gpt);

	u32 idx = 0;
	int lines_left = 20;
	s_printf(txt_buf + strlen(txt_buf), "#FFBA00 Idx Name                            Size     Offset    Sectors#\n");
	LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
	{
		int lines = strlen(part->name) > 25 ? 2 : 1;
		if ((lines_left - lines) <= 0)
		{
			strcat(txt_buf, "#FFDD00 Table does not fit on screen...#");
			break;
		}

		if (lines == 2)
		{
			s_printf(txt_buf + strlen(txt_buf), "%02d: #96FF00 %s#\n                              %6d MiB  %8Xh  %8Xh\n",
				part->index, part->name, (part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
				part->lba_start, part->lba_end - part->lba_start + 1);
		}
		else
		{
			s_printf(txt_buf + strlen(txt_buf), "%02d: #96FF00 %.25s# %6d MiB  %8Xh  %8Xh\n",
				part->index, part->name, (part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
				part->lba_start, part->lba_end - part->lba_start + 1);
		}

		lines_left -= lines;
		idx++;
	}
	if (!idx)
		strcat(txt_buf, "#FFDD00 Partition table is empty!#");

	emmc_gpt_free(&gpt);

	lv_label_set_text(lb_desc2, txt_buf);
	lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
	lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 6, 0);

	emmc_errors = emmc_get_error_count();
	if (emmc_get_mode() < EMMC_MMC_HS400  ||
		emmc_errors[EMMC_ERROR_INIT_FAIL] ||
		emmc_errors[EMMC_ERROR_RW_FAIL]   ||
		emmc_errors[EMMC_ERROR_RW_RETRY])
	{
out_error:
		lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
		lv_obj_set_style(dark_bg, &mbox_darken);
		lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

		static const char * mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
		lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
		lv_mbox_set_recolor_text(mbox, true);

		s_printf(txt_buf,
			"#FF8000 eMMC Issues Warning#\n\n"
			"#FFDD00 Your eMMC is initialized in a slower mode,#\n"
			"#FFDD00 or init/read/write errors occurred!#\n"
			"#FFDD00 This might mean hardware issues!#\n\n"
			"#00DDFF Bus Speed:# %d MB/s\n\n"
			"#00DDFF SDMMC4 Errors:#\n"
			"Init fails: %d\n"
			"Read/Write fails: %d\n"
			"Read/Write errors: %d",
			emmc_storage.csd.busspeed,
			emmc_errors[EMMC_ERROR_INIT_FAIL],
			emmc_errors[EMMC_ERROR_RW_FAIL],
			emmc_errors[EMMC_ERROR_RW_RETRY]);

		lv_mbox_set_text(mbox, txt_buf);
		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
		lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_set_top(mbox, true);
	}

	emmc_end();
	free(txt_buf);

	return LV_RES_OK;
}

static lv_res_t _create_window_sdcard_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_SD" microSD Card Info");
	lv_win_add_btn(win, NULL, SYMBOL_SD" Benchmark", _create_mbox_sd_bench);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 6 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 5 / 2);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);

	lv_label_set_text(lb_desc, "#D4FF00 Please wait...#");
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	// Disable buttons.
	nyx_window_toggle_buttons(win, true);

	manual_system_maintenance(true);

	if (!sd_mount())
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init SD!#");
		goto failed;
	}

	lv_label_set_text(lb_desc,
		"#00DDFF Card ID:#\n"
		"Vendor ID:\n"
		"Model:\n"
		"OEM ID:\n"
		"HW rev:\n"
		"FW rev:\n"
		"S/N:\n"
		"Month/Year:\n\n"
		"Max Power:\n"
		"Initial bus:"
	);

	lv_obj_t *val = lv_cont_create(win, NULL);
	lv_obj_set_size(val, LV_HOR_RES / 12 * 3, LV_VER_RES - (LV_DPI * 11 / 8) * 5 / 2);

	lv_obj_t * lb_val = lv_label_create(val, lb_desc);

	char *txt_buf = (char *)malloc(SZ_16K);
	txt_buf[0] = '\n';
	txt_buf[1] = 0;

	// Identify manufacturer.
	switch (sd_storage.cid.manfid)
	{
	case 0x00:
		strcat(txt_buf, "#FFDD00 Fake# ");
		break;
	case 0x01:
		strcat(txt_buf, "Panasonic ");
		break;
	case 0x02:
		strcat(txt_buf, "Toshiba ");
		break;
	case 0x03:
		if (!memcmp(&sd_storage.cid.oemid, "DW", 2))
			strcat(txt_buf, "Western Digital "); // WD.
		else
			strcat(txt_buf, "SanDisk ");
		break;
	case 0x06:
		strcat(txt_buf, "Ritek ");
		break;
	case 0x09:
		strcat(txt_buf, "ATP ");
		break;
	case 0x13:
		strcat(txt_buf, "Kingmax ");
		break;
	case 0x19:
		strcat(txt_buf, "Dynacard ");
		break;
	case 0x1A:
		strcat(txt_buf, "Power Quotient ");
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
		strcat(txt_buf, "Barun Electronics ");
		break;
	case 0x31:
		strcat(txt_buf, "Silicon Power ");
		break;
	case 0x41:
		strcat(txt_buf, "Kingston ");
		break;
	case 0x51:
		strcat(txt_buf, "STEC ");
		break;
	case 0x5D:
		strcat(txt_buf, "SwissBit ");
		break;
	case 0x61:
		strcat(txt_buf, "Netlist ");
		break;
	case 0x63:
		strcat(txt_buf, "Cactus ");
		break;
	case 0x73:
		strcat(txt_buf, "Bongiovi ");
		break;
	case 0x74:
		strcat(txt_buf, "Jiaelec ");
		break;
	case 0x76:
		strcat(txt_buf, "Patriot ");
		break;
	case 0x82:
		strcat(txt_buf, "Jiang Tay ");
		break;
	case 0x83:
		strcat(txt_buf, "Netcom ");
		break;
	case 0x84:
		strcat(txt_buf, "Strontium ");
		break;
	case 0x9C:
		if (!memcmp(&sd_storage.cid.oemid, "OS", 2))
			strcat(txt_buf, "Sony "); // SO.
		else
			strcat(txt_buf, "Barun Electronics "); // BE.
		break;
	case 0x9F:
		strcat(txt_buf, "Taishin ");
		break;
	case 0xAD:
		strcat(txt_buf, "Longsys ");
		break;
	default:
		strcat(txt_buf, "Unknown ");
		break;
	}

	// UHS-I max power limit is 400mA, no matter what the card says.
	u32 max_power_nominal = sd_storage.max_power > 400 ? 400 : sd_storage.max_power;

	s_printf(txt_buf + strlen(txt_buf), "(%02X)\n%c%c%c%c%c\n%c%c (%04X)\n%X\n%X\n%08x\n%02d/%04d\n\n%d mW (%d mA)\n",
		sd_storage.cid.manfid,
		sd_storage.cid.prod_name[0], sd_storage.cid.prod_name[1], sd_storage.cid.prod_name[2],
		sd_storage.cid.prod_name[3], sd_storage.cid.prod_name[4],
		(sd_storage.cid.oemid >> 8) & 0xFF, sd_storage.cid.oemid & 0xFF, sd_storage.cid.oemid,
		sd_storage.cid.hwrev, sd_storage.cid.fwrev, sd_storage.cid.serial,
		sd_storage.cid.month, sd_storage.cid.year,
		max_power_nominal * 3600 / 1000, sd_storage.max_power);

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
	lv_obj_set_size(desc2, LV_HOR_RES / 2 / 11 * 5, LV_VER_RES - (LV_DPI * 11 / 8) * 5 / 2);

	lv_obj_t * lb_desc2 = lv_label_create(desc2, lb_desc);

	lv_label_set_static_text(lb_desc2,
		"#00DDFF Card-Specific Data#\n"
		"Cmd Classes:\n"
		"Capacity:\n"
		"Capacity (LBA):\n"
		"Bus Width:\n"
		"Current Rate:\n"
		"Speed Class:\n"
		"UHS Classes:\n"
		"Max Bus Speed:\n\n"
		"Write Protect:"
	);
	lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
	lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 5 * 3, 0);

	lv_obj_t *val2 = lv_cont_create(win, NULL);
	lv_obj_set_size(val2, LV_HOR_RES / 4, LV_VER_RES - (LV_DPI * 11 / 8) * 5 / 2);

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

	sd_func_modes_t fmodes = { 0 };
	sd_storage_get_fmodes(&sd_storage, NULL, &fmodes);

	char *bus_speed;
	if      (fmodes.cmd_system  & SD_MODE_UHS_DDR200)
		bus_speed = "DDR200";
	else if (fmodes.access_mode & SD_MODE_UHS_SDR104)
		bus_speed = "SDR104";
	else if (fmodes.access_mode & SD_MODE_UHS_SDR50)
		bus_speed = "SDR50";
	else if (fmodes.access_mode & SD_MODE_UHS_DDR50)
		bus_speed = "DDR50";
	else if (fmodes.access_mode & SD_MODE_UHS_SDR25)
		bus_speed = "SDR25";
	else
		bus_speed = "SDR12";

	char *cpe = NULL;
	if (sd_storage.ssr.app_class == 2)
	{
		u8 *buf = zalloc(512);

		// Directly get and parse ext reg for performance enhance.
		sd_storage_parse_perf_enhance(&sd_storage, 2, 0, 0, buf);

		bool has_perf_enhance = sd_storage.ser.cache &&
							sd_storage.ser.cmdq  &&
							sd_storage.ser.cache == sd_storage.ser.cache_ext &&
							sd_storage.ser.cmdq  == sd_storage.ser.cmdq_ext;
		if (has_perf_enhance)
			cpe = "#FFDD00 "; // CMDQ/CACHE support via a quirk.
		else
			cpe = "#FF3C28 "; // Broken.

		// Get and parse ext reg for performance enhance in spec.
		sd_storage_get_ext_regs(&sd_storage, buf);

		if (sd_storage.ser.valid)
		{
			has_perf_enhance = sd_storage.ser.cache &&
							   sd_storage.ser.cmdq  &&
							   sd_storage.ser.cache == sd_storage.ser.cache_ext &&
							   sd_storage.ser.cmdq  == sd_storage.ser.cmdq_ext;

			if (has_perf_enhance)
				cpe = NULL; // CMDQ/CACHE support in spec.
			else
				cpe = "#FF3C28 "; // Broken.
		}

		free(buf);
	}

	s_printf(txt_buf,
		"#00DDFF v%d.0#\n"
		"%02X\n"
		"%d MiB\n"
		"%X (CP %X)\n"
		"%d\n"
		"%d MB/s (%d MHz)\n"
		"%d (AU: %d %s\n"
		"U%d V%d %sA%d%s\n"
		"%s\n\n"
		"%s",
		sd_storage.csd.structure + 1,
		sd_storage.csd.cmdclass,
		sd_storage.sec_cnt >> 11,
		sd_storage.sec_cnt, sd_storage.ssr.protected_size >> 9,
		sd_storage.ssr.bus_width,
		sd_storage.csd.busspeed,
		(sd_storage.csd.busspeed > 10) ? (sd_storage.csd.busspeed * 2) : 50,
		sd_storage.ssr.speed_class, uhs_au_size, uhs_au_mb ? "MiB)" : "KiB)",
		sd_storage.ssr.uhs_grade, sd_storage.ssr.video_class, cpe ? cpe : "", sd_storage.ssr.app_class, cpe ? "#" : "",
		bus_speed,
		wp_info);

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
	lv_label_set_text(lb_desc3, "#D4FF00 Acquiring info...#");
	lv_obj_set_width(lb_desc3, lv_obj_get_width(desc3));

	lv_obj_align(desc3, desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);

	manual_system_maintenance(true);

	lv_obj_set_size(desc3, LV_HOR_RES / 2 / 6 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 4);
	lv_obj_align(desc3, desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);

	lv_obj_t *val3 = lv_cont_create(win, NULL);
	lv_obj_set_size(val3, LV_HOR_RES / 12 * 3, LV_VER_RES - (LV_DPI * 11 / 8) * 4);

	lv_obj_t * lb_val3 = lv_label_create(val3, lb_desc);
	lv_label_set_text(lb_val3, "");

	lv_obj_align(val3, desc3, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_t *desc4 = lv_cont_create(win, NULL);
	lv_obj_set_size(desc4, LV_HOR_RES / 2 / 2 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 4);

	lv_obj_t * lb_desc4 = lv_label_create(desc4, lb_desc);
	lv_label_set_text(lb_desc4, "                             ");
	lv_obj_set_width(lb_desc4, lv_obj_get_width(desc4));

	lv_label_set_text(lb_desc4,
		"#00DDFF SDMMC1 Errors:#\n"
		"Init fails:\n"
		"Read/Write fails:\n"
		"Read/Write errors:"
	);
	lv_obj_set_size(desc4, LV_HOR_RES / 2 / 11 * 5, LV_VER_RES - (LV_DPI * 11 / 8) * 4);
	lv_obj_set_width(lb_desc4, lv_obj_get_width(desc4));
	lv_obj_align(desc4, val3, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 5 * 3, 0);

	lv_obj_t *val4 = lv_cont_create(win, NULL);
	lv_obj_set_size(val4, LV_HOR_RES / 4, LV_VER_RES - (LV_DPI * 11 / 8) * 4);

	lv_obj_t *lb_val4 = lv_label_create(val4, lb_desc);
	bench_sd_err_label = lb_val4;

	u16 *sd_errors = sd_get_error_count();
	s_printf(txt_buf, "\n%d (%d)\n%d (%d)\n%d (%d)",
		sd_errors[SD_ERROR_INIT_FAIL], nyx_str->info.sd_errors[SD_ERROR_INIT_FAIL],
		sd_errors[SD_ERROR_RW_FAIL],   nyx_str->info.sd_errors[SD_ERROR_RW_FAIL],
		sd_errors[SD_ERROR_RW_RETRY],  nyx_str->info.sd_errors[SD_ERROR_RW_RETRY]);

	lv_label_set_text(lb_val4, txt_buf);

	lv_obj_set_width(lb_val4, lv_obj_get_width(val4));
	lv_obj_align(val4, desc4, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	manual_system_maintenance(true);

	f_getfree("", &sd_fs.free_clst, NULL);

	lv_label_set_text(lb_desc3,
		"#00DDFF Found FAT FS:#\n"
		"Filesystem:\n"
		"Cluster:\n"
		"Size free/total:"
	);

	lv_obj_set_width(lb_desc3, lv_obj_get_width(desc3));

	s_printf(txt_buf, "\n%s\n%d %s\n%d/%d MiB",
		sd_fs.fs_type == FS_EXFAT ? ("exFAT  "SYMBOL_SHRK) : ("FAT32"),
		(sd_fs.csize > 1) ? (sd_fs.csize >> 1) : SD_BLOCKSIZE,
		(sd_fs.csize > 1) ? "KiB" : "B",
		(u32)(sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF),
		(u32)(sd_fs.n_fatent  * sd_fs.csize >> SECTORS_TO_MIB_COEFF));

	lv_label_set_text(lb_val3, txt_buf);

	lv_obj_set_width(lb_val3, lv_obj_get_width(val3));

	free(txt_buf);
	sd_unmount();

failed:
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

	char *txt_buf = (char *)malloc(SZ_16K);
	int value = 0;

	// Fuel gauge IC info.
	if (!max17050_get_version(NULL))
	{
		int cap_pct = 0;
		max17050_get_property(MAX17050_RepSOC, &cap_pct);
		max17050_get_property(MAX17050_RepCap, &value);
		s_printf(txt_buf, "\n%d mAh [%d %%]\n", value, cap_pct >> 8);

		max17050_get_property(MAX17050_FullCAP, &value);
		s_printf(txt_buf + strlen(txt_buf), "%d mAh\n", value);

		max17050_get_property(MAX17050_DesignCap, &value);
		bool design_cap_init = value == 1000;
		s_printf(txt_buf + strlen(txt_buf), "%s%d mAh%s\n",
			design_cap_init ? "#FF8000 " : "", value,  design_cap_init ? " - Init "SYMBOL_WARNING"#" : "");

		max17050_get_property(MAX17050_Current, &value);
		s_printf(txt_buf + strlen(txt_buf), "%d mA\n", value / 1000);

		max17050_get_property(MAX17050_AvgCurrent, &value);
		s_printf(txt_buf + strlen(txt_buf), "%d mA\n", value / 1000);

		max17050_get_property(MAX17050_VCELL, &value);
		bool voltage_empty = value < 3200;
		s_printf(txt_buf + strlen(txt_buf), "%s%d mV%s\n",
			voltage_empty ? "#FF8000 " : "", value,  voltage_empty ? " - Low "SYMBOL_WARNING"#" : "");

		max17050_get_property(MAX17050_OCVInternal, &value);
		s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

		max17050_get_property(MAX17050_MinVolt, &value);
		s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

		max17050_get_property(MAX17050_MaxVolt, &value);
		s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

		max17050_get_property(MAX17050_V_empty, &value);
		s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

		max17050_get_property(MAX17050_TEMP, &value);
		s_printf(txt_buf + strlen(txt_buf), "%d.%d oC\n\n\n", value / 10, (value >= 0 ? value : (~value + 1)) % 10);
	}
	else
		strcpy(txt_buf, "\n#FF8000 "SYMBOL_WARNING" Error!#\n\n\n\n\n\n\n\n\n\n\n\n\n");

	// Main Pmic IC info.
	value = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CID4);
	u32 main_pmic_version = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CID3) & 0xF;

	if (value == 0x35)
		s_printf(txt_buf + strlen(txt_buf), "max77620 v%d\nErista OTP\n", main_pmic_version);
	else if (value == 0x53)
		s_printf(txt_buf + strlen(txt_buf), "max77620 v%d\nMariko OTP\n", main_pmic_version);
	else
		s_printf(txt_buf + strlen(txt_buf), "max77620 v%d\n#FF8000 Unknown OTP# (%02X)\n", main_pmic_version, value);

	// CPU/GPU/DRAM Pmic IC info.
	u32 cpu_gpu_pmic_type = h_cfg.t210b01 ? (FUSE(FUSE_RESERVED_ODM28_B01) & 1) + 1 : 0;
	switch (cpu_gpu_pmic_type)
	{
	case 0:
		s_printf(txt_buf + strlen(txt_buf), "max77621 v%d",
			i2c_recv_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_REG_CHIPID1));
		break;
	case 1:
		s_printf(txt_buf + strlen(txt_buf), "max77812-2 v%d",   // High power GPU. 2 Outputs, phases 3 1.
			i2c_recv_byte(I2C_5, MAX77812_PHASE31_CPU_I2C_ADDR, MAX77812_REG_VERSION) & 7);
		break;
	case 2:
		s_printf(txt_buf + strlen(txt_buf), "max77812-3 v%d.0", // Low  power GPU. 3 Outputs, phases 2 1 1.
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
		"Input current limit:\n"
		"System voltage limit:\n"
		"Charge current limit:\n"
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
	int iinlim = 0;

	// Charger IC info.
	if (!bq24193_get_version(NULL))
	{
		bq24193_get_property(BQ24193_InputCurrentLimit, &iinlim);
		s_printf(txt_buf, "\n%d mA\n", iinlim);

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
			strcat(txt_buf, "#FF8000 Cold#");
			break;
		case 6:
			strcat(txt_buf, "#FF8000 Hot#");
			break;
		default:
			s_printf(txt_buf + strlen(txt_buf), "Unknown (%d)", value);
			break;
		}
	}
	else
		strcpy(txt_buf, "\n#FF8000 "SYMBOL_WARNING" Error!#\n\n\n\n\n");

	strcat(txt_buf, "\n\n\n");

	// USB-PD IC info.
	if (!bm92t36_get_version(NULL))
	{
		bool inserted;
		u32 wattage = 0;
		usb_pd_objects_t usb_pd;
		bm92t36_get_sink_info(&inserted, &usb_pd);
		strcat(txt_buf, inserted ? "Connected" : "Disconnected");

		// Select 5V is no PD contract.
		wattage = iinlim * (usb_pd.pdo_no ? usb_pd.selected_pdo.voltage : 5);

		s_printf(txt_buf + strlen(txt_buf), "\n%d.%d W", wattage / 1000, (wattage % 1000) / 100);

		if (!usb_pd.pdo_no)
			strcat(txt_buf, "\nNon PD");

		// Limit to 6 profiles so it can fit.
		usb_pd.pdo_no = MIN(usb_pd.pdo_no, 6);

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
	}
	else
		strcat(txt_buf, "#FF8000 "SYMBOL_WARNING" Error!#");

	lv_label_set_text(lb_val2, txt_buf);

	lv_obj_set_width(lb_val2, lv_obj_get_width(val2));
	lv_obj_align(val2, desc2, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	free(txt_buf);

	return LV_RES_OK;
}

static bool _lockpick_exists_check()
{
	#define LOCKPICK_MAGIC_OFFSET   0x118
	#define LOCKPICK_VERSION_OFFSET 0x11C
	#define LOCKPICK_MAGIC          0x4B434F4C // LOCK.
	#define LOCKPICK_MIN_VERSION    0x1090500  // 1.9.5.

	bool found = false;
	void *buf = malloc(0x200);
	if (sd_mount())
	{
		FIL fp;
		if (f_open(&fp, "bootloader/payloads/Lockpick_RCM.bin", FA_READ))
			goto out;

		// Read Lockpick payload and check versioning.
		if (f_read(&fp, buf, 0x200, NULL))
		{
			f_close(&fp);

			goto out;
		}

		u32 magic = *(u32 *)(buf + LOCKPICK_MAGIC_OFFSET);
		u32 version = byte_swap_32(*(u32 *)(buf + LOCKPICK_VERSION_OFFSET) - 0x303030);

		if (magic == LOCKPICK_MAGIC && version >= LOCKPICK_MIN_VERSION)
			found = true;

		f_close(&fp);
	}

out:
	free(buf);
	sd_unmount();

	return found;
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
	lv_label_set_static_text(label_btn, SYMBOL_KEY"  Lockpick");
	lv_obj_align(btn2, btn, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 11 / 15, 0);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, _create_mbox_lockpick);

	bool lockpick_found = _lockpick_exists_check();
	if (!lockpick_found)
		lv_btn_set_state(btn2, LV_BTN_STATE_INA);

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);

	if (lockpick_found)
	{
		lv_label_set_static_text(label_txt2,
			"View Ipatches and dump the unpatched and patched versions\nof BootROM.\n"
			"Or dump every single key via #C7EA46 Lockpick RCM#.\n");
	}
	else
	{
		lv_label_set_static_text(label_txt2,
			"View Ipatches and dump the unpatched and patched versions\nof BootROM. Or dump every single key via #C7EA46 Lockpick RCM#.\n"
			"#FFDD00 bootloader/payloads/Lockpick_RCM.bin is missing or old!#\n");
	}

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
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _create_window_hw_info_status);

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
