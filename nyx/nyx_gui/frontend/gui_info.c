/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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
#include "../hos/hos.h"
#include "../hos/pkg1.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../power/bq24193.h"
#include "../power/max17050.h"
#include "../sec/tsec.h"
#include "../soc/fuse.h"
#include "../soc/kfuse.h"
#include "../soc/i2c.h"
#include "../soc/smmu.h"
#include "../soc/t210.h"
#include "../storage/mmc.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/sprintf.h"
#include "../utils/util.h"

#define SECTORS_TO_MIB_COEFF 11

extern sdmmc_storage_t sd_storage;
extern FATFS sd_fs;

extern bool sd_mount();
extern void sd_unmount(bool deinit);
extern int  sd_save_to_file(void *buf, u32 size, const char *filename);
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

#pragma GCC push_options
#pragma GCC target ("thumb")

static lv_res_t _create_window_dump_done(int error, char *dump_filenames)
{
	lv_style_t *darken;
	darken = (lv_style_t *)malloc(sizeof(lv_style_t));
	lv_style_copy(darken, &lv_style_plain);
	darken->body.main_color = LV_COLOR_BLACK;
	darken->body.grad_color = darken->body.main_color;
	darken->body.opa = LV_OPA_30;

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);

	char *txt_buf = (char *)malloc(0x1000);

	if (error)
		s_printf(txt_buf, "#FFDD00 Failed to dump to# %s#FFDD00 !#\nError: %d", dump_filenames, error);
	else
		s_printf(txt_buf, "Dumping to SD card finished!\nFiles: #C7EA46 backup/{emmc_sn}/dumps/#%s", dump_filenames);
	lv_mbox_set_text(mbox, txt_buf);

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action); // Important. After set_text.

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _battery_dump_window_action(lv_obj_t * btn)
{
	int error = !sd_mount();

	if (!error)
	{
		char path[64];
		u8 *buf = (u8 *)malloc(0x100 * 2);

		// Dump all battery fuel gauge registers.
		for (int i = 0; i < 0x200; i += 2)
		{
			i2c_recv_buf_small(buf + i, 2, I2C_1, MAXIM17050_I2C_ADDR, i >> 1);
			msleep(1);
		}

		emmcsn_path_impl(path, "/dumps", "fuel_gauge.bin", NULL);
		error = sd_save_to_file((u8 *)buf, 0x200, path);

		sd_unmount(false);
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
		
		sd_unmount(false);
	}
	_create_window_dump_done(error, "evp_thunks.bin, bootrom_patched.bin, bootrom_unpatched.bin");


	return LV_RES_OK;
}

static lv_res_t _fuse_dump_window_action(lv_obj_t * btn)
{
	int error = !sd_mount();
	if (!error)
	{
		char path[64];
		emmcsn_path_impl(path, "/dumps", "fuse_cached.bin", NULL);
		error = sd_save_to_file((u8 *)0x7000F900, 0x300, path);

		u32 words[192];
		fuse_read_array(words);
		emmcsn_path_impl(path, "/dumps", "fuse_array_raw.bin", NULL);
		int res = sd_save_to_file((u8 *)words, sizeof(words), path);
		if (!error)
			error = res;

		sd_unmount(false);
	}
	_create_window_dump_done(error, "fuse_cached.bin, fuse_array_raw.bin");

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

		sd_unmount(false);
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
		error = sd_save_to_file(tsec_keys, 0x10 * 2, path);

		sd_unmount(false);
	}
	_create_window_dump_done(error, "tsec_keys.bin");

	return LV_RES_OK;
}

static lv_res_t _create_window_fuses_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_CHIP" Cached fuses Info");
	lv_win_add_btn(win, NULL, SYMBOL_DOWNLOAD" Dump fuses", _fuse_dump_window_action);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 5 * 2, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_ta_set_style(lb_desc, LV_TA_STYLE_BG, &monospace_text);

	lv_label_set_static_text(lb_desc,
		"#00DDFF Detailed Info:#\n"
		"SKU:\n"
		"DRAM ID:\n"
		"#FF8000 Burnt Fuses (ODM 7/6):#\n"
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
		"Y Coordinate:"
	);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	lv_obj_t *val = lv_cont_create(win, NULL);
	lv_obj_set_size(val, LV_HOR_RES / 11 * 3, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_val = lv_label_create(val, lb_desc);

	char *txt_buf = (char *)malloc(0x1000);

	// Decode fuses.
	u8 burntFuses7 = 0;
	u8 burntFuses6 = 0;
	u32 odm4 = fuse_read_odm(4);
	u8 dram_id = (odm4 >> 3) & 0x1F;
	char dram_man[16] = {0};
	switch (dram_id)
	{
	case 0:
	case 4:
		memcpy(dram_man, "Samsung ", 9);
		if (!dram_id)
			memcpy(dram_man + strlen(dram_man), "4GB", 4);
		else
			memcpy(dram_man + strlen(dram_man), "6GB", 4);
		break;
	case 1:
		memcpy(dram_man, "Hynix 4GB", 10);
		break;
	case 2:
		memcpy(dram_man, "Micron 4GB", 11);
		break;
	default:
		memcpy(dram_man, "Unknown", 8);
		break;
	}

	for (u32 i = 0; i < 32; i++)
	{
		if ((fuse_read_odm(7) >> i) & 1)
			burntFuses7++;
	}
	for (u32 i = 0; i < 32; i++)
	{
		if ((fuse_read_odm(6) >> i) & 1)
			burntFuses6++;
	}

	u32 lot_code0 = (FUSE(FUSE_OPT_LOT_CODE_0) & 0xFFFFFFF) << 2;
	u32 lot_bin = 0;
	for (int i = 0; i < 5; ++i)
	{
		u32 digit = (lot_code0 & 0xFC000000) >> 26;
		lot_bin *= 36;
		lot_bin += digit;
		lot_code0 <<= 6;
	}

	// Parse fuses and display them.
	s_printf(txt_buf,
		"\n%X - %s - %s\n%d - %s\n%d - %d\n%08X%08X%08X%08X\n%08X\n"
		"%s\n%d.%02d (0x%X)\n%d.%02d (0x%X)\n%d\n%d\n%d\n%d\n%d\n0x%X\n%d\n%d\n%d\n%d\n"
		"%d\n%d\n%d (0x%X)\n%d\n%d\n%d\n%d",
		FUSE(FUSE_SKU_INFO), odm4 & 0x10000 ? "Mariko" : "Erista", (fuse_read_odm(4) & 3) ? "Dev" : "Retail",
		dram_id, dram_man, burntFuses7, burntFuses6,
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY0)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY1)),
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY2)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY3)),
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY4)),
		(FUSE(FUSE_RESERVED_SW) & 0x80) ? "XUSB" : "USB 2.0",
		(FUSE(FUSE_OPT_FT_REV)  >> 5) & 0x3F, FUSE(FUSE_OPT_FT_REV) & 0x1F, FUSE(FUSE_OPT_FT_REV),
		(FUSE(FUSE_OPT_CP_REV)  >> 5) & 0x3F, FUSE(FUSE_OPT_CP_REV) & 0x1F, FUSE(FUSE_OPT_CP_REV),
		FUSE(FUSE_FIRST_BOOTROM_PATCH_SIZE) & 0x7F,
		FUSE(FUSE_CPU_SPEEDO_0_CALIB), FUSE(FUSE_CPU_SPEEDO_1_CALIB), FUSE(FUSE_CPU_SPEEDO_2_CALIB),
		FUSE(FUSE_SOC_SPEEDO_0_CALIB), FUSE(FUSE_SOC_SPEEDO_1_CALIB), FUSE(FUSE_SOC_SPEEDO_2_CALIB),
		FUSE(FUSE_CPU_IDDQ_CALIB), FUSE(FUSE_SOC_IDDQ_CALIB), FUSE(FUSE_GPU_IDDQ_CALIB),
		FUSE(FUSE_OPT_VENDOR_CODE), FUSE(FUSE_OPT_FAB_CODE), lot_bin, FUSE(FUSE_OPT_LOT_CODE_0),
		FUSE(FUSE_OPT_LOT_CODE_1), FUSE(FUSE_OPT_WAFER_ID), FUSE(FUSE_OPT_X_COORDINATE), FUSE(FUSE_OPT_Y_COORDINATE));

	lv_label_set_array_text(lb_val, txt_buf, 0x1000);

	free(txt_buf);

	lv_obj_set_width(lb_val, lv_obj_get_width(val));
	lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_t *desc2 = lv_cont_create(win, NULL);
	lv_obj_set_size(desc2, LV_HOR_RES / 2 / 5 * 4, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc2 = lv_label_create(desc2, NULL);
	lv_label_set_long_mode(lb_desc2, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc2, true);

	// Check if patched unit.
	if (!fuse_check_patched_rcm())
		lv_label_set_static_text(lb_desc2, "#96FF00 Your unit is exploitable#\n#96FF00 to the RCM bug!#");
	else
		lv_label_set_static_text(lb_desc2, "#FF8000 Your unit is patched#\n#FF8000 to the RCM bug!#");

	lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
	lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);
	lv_label_set_align(lb_desc2, LV_LABEL_ALIGN_CENTER);

	return LV_RES_OK;
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
	s_printf(ipatches_txt + strlen(ipatches_txt), "\n");
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
	lv_ta_set_style(lb_desc, LV_TA_STYLE_BG, &monospace_text);

	char *txt_buf = (char *)malloc(0x1000);
	ipatches_txt = txt_buf;
	s_printf(txt_buf, "#00DDFF Ipatches:#\n#FF8000 Address  "SYMBOL_DOT"  Val  "SYMBOL_DOT"  Instruction\n");

	u32 res = fuse_read_ipatch(_ipatch_process);
	if (res != 0)
		s_printf(txt_buf + strlen(txt_buf), "#FFDD00 Failed to read ipatches. Error: %d#", res);

	lv_label_set_array_text(lb_desc, txt_buf, 0x1000);

	free(txt_buf);

	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	return LV_RES_OK;
}

static lv_res_t _create_window_tsec_keys_status(lv_obj_t *btn)
{
	u32 retries = 0;

	tsec_ctxt_t tsec_ctxt;
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	lv_obj_t *win = nyx_create_standard_window(SYMBOL_CHIP" TSEC Keys");

	//Disable buttons.
	nyx_window_toggle_buttons(win, true);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 2, LV_VER_RES - (LV_DPI * 11 / 6));
	//lv_obj_align(desc, win, LV_ALIGN_ou_TOP_LEFT, 0, 40);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_ta_set_style(lb_desc, LV_TA_STYLE_BG, &monospace_text);

	// Read package1.
	char *build_date = malloc(32);
	u8 *pkg1 = (u8 *)malloc(0x40000);
	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	sdmmc_storage_end(&storage);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1, build_date);

	char *txt_buf  = (char *)malloc(0x500);
	char *txt_buf2 = (char *)malloc(0x500);
	s_printf(txt_buf, "#00DDFF Found pkg1 ('%s')#\n", build_date);
	free(build_date);

	if (!pkg1_id)
	{
		s_printf(txt_buf + strlen(txt_buf), "#FFDD00 Unknown pkg1 version for reading#\n#FFDD00 TSEC firmware!#");
		lv_label_set_array_text(lb_desc, txt_buf, 0x500);
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

		goto out;
	}
	lv_label_set_array_text(lb_desc, txt_buf, 0x500);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	lv_obj_t *val = lv_cont_create(win, NULL);
	lv_obj_set_size(val, LV_HOR_RES / 11 * 3, LV_VER_RES - (LV_DPI * 11 / 6));

	lv_obj_t * lb_val = lv_label_create(val, lb_desc);
	lv_ta_set_style(lb_val, LV_TA_STYLE_BG, &monospace_text);

	lv_label_set_static_text(lb_val, "Please wait...");
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

	s_printf(txt_buf + strlen(txt_buf), "#C7EA46 TSEC Key:#\n");
	if (res >= 0)
	{
		s_printf(txt_buf2, "\n%08X%08X%08X%08X\n", 
			byte_swap_32(tsec_keys[0]), byte_swap_32(tsec_keys[1]), byte_swap_32(tsec_keys[2]), byte_swap_32(tsec_keys[3]));		

		if (pkg1_id->kb == KB_FIRMWARE_VERSION_620)
		{
			s_printf(txt_buf + strlen(txt_buf), "#C7EA46 TSEC root:#\n");
			s_printf(txt_buf2 + strlen(txt_buf2), "%08X%08X%08X%08X\n", 
				byte_swap_32(tsec_keys[4]), byte_swap_32(tsec_keys[5]), byte_swap_32(tsec_keys[6]), byte_swap_32(tsec_keys[7]));
		}
		lv_win_add_btn(win, NULL, SYMBOL_DOWNLOAD" Dump Keys", _tsec_keys_dump_window_action);
	}
	else
	{
		s_printf(txt_buf2, "Error: %x", res);
	}

	lv_label_set_array_text(lb_desc, txt_buf, 0x500);

	lv_label_set_array_text(lb_val, txt_buf2, 0x500);

out:
	free(pkg1);
	free(txt_buf);
	free(txt_buf2);

	nyx_window_toggle_buttons(win, false);

	return LV_RES_OK;
}


static lv_res_t _create_window_emmc_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_CHIP" Internal eMMC Info");

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 6 * 2, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	char *txt_buf = (char *)malloc(0x1000);

	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		lv_label_set_static_text(lb_desc, "#FFDD00 Failed to init eMMC!#");
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));
	}
	else
	{
		u16 card_type;
		u32 speed = 0;

		s_printf(txt_buf, "\n%X\n%X\n%02X\n%c%c%c%c%c%c\n%X\n%04X\n%02d/%04d\n\n",
			storage.cid.manfid, storage.cid.card_bga, storage.cid.oemid,
			storage.cid.prod_name[0], storage.cid.prod_name[1], storage.cid.prod_name[2],
			storage.cid.prod_name[3], storage.cid.prod_name[4],	storage.cid.prod_name[5],
			storage.cid.prv, storage.cid.serial, storage.cid.month, storage.cid.year);

		card_type = storage.ext_csd.card_type;
		char card_type_support[96];
		card_type_support[0] = 0;
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

		s_printf(txt_buf + strlen(txt_buf),
			"#00DDFF V1.%d#\n%02X\n1.%d\n%02X\n%d MB/s (%d MHz)\n%d MB/s\n%s",
			storage.ext_csd.ext_struct, storage.csd.mmca_vsn, storage.ext_csd.rev,
			storage.csd.cmdclass, speed & 0xFFFF, (speed >> 16) & 0xFFFF,
			storage.csd.busspeed, card_type_support);

		lv_label_set_static_text(lb_desc,
			"#00DDFF CID:#\n"
			"Vendor ID:\n"
			"Card/BGA:\n"
			"OEM ID:\n"
			"Model:\n"
			"Prd Rev:\n"
			"S/N:\n"
			"Month/Year:\n\n"
			"#00DDFF Ext CSD#\n"
			"Spec Version:\n"
			"Extended Rev:\n"
			"Cmd Classes:\n"
			"Max Rate:\n"
			"Current Rate:\n"
			"Type Support:"
		);
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

		lv_obj_t *val = lv_cont_create(win, NULL);
		lv_obj_set_size(val, LV_HOR_RES / 11 * 3, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

		lv_obj_t * lb_val = lv_label_create(val, lb_desc);

		lv_label_set_array_text(lb_val, txt_buf, 0x1000);

		lv_obj_set_width(lb_val, lv_obj_get_width(val));
		lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		lv_obj_t *desc2 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc2, LV_HOR_RES / 2 / 4 * 4, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

		lv_obj_t * lb_desc2 = lv_label_create(desc2, lb_desc);
		lv_ta_set_style(lb_desc2, LV_TA_STYLE_BG, &monospace_text);

		u32 boot_size = storage.ext_csd.boot_mult << 17;
		u32 rpmb_size = storage.ext_csd.rpmb_mult << 17;
		s_printf(txt_buf, "#00DDFF eMMC Physical Partitions:#\n");
		s_printf(txt_buf + strlen(txt_buf), "1: #96FF00 BOOT0# Size: %5d KiB (Sect: 0x%08X)\n", boot_size / 1024, boot_size / 512);
		s_printf(txt_buf + strlen(txt_buf), "2: #96FF00 BOOT1# Size: %5d KiB (Sect: 0x%08X)\n", boot_size / 1024, boot_size / 512);
		s_printf(txt_buf + strlen(txt_buf), "3: #96FF00 RPMB#  Size: %5d KiB (Sect: 0x%08X)\n", rpmb_size / 1024, rpmb_size / 512);
		s_printf(txt_buf + strlen(txt_buf), "0: #96FF00 GPP#   Size: %5d MiB (Sect: 0x%08X)\n\n", storage.sec_cnt >> SECTORS_TO_MIB_COEFF, storage.sec_cnt);
		s_printf(txt_buf + strlen(txt_buf), "#00DDFF GPP (eMMC USER) Partition Table:#\n");

		sdmmc_storage_set_mmc_partition(&storage, 0);
		LIST_INIT(gpt);
		nx_emmc_gpt_parse(&gpt, &storage);
		int gpp_idx = 0;
		LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
		{
			if (gpp_idx < 2)
			{
				s_printf(txt_buf + strlen(txt_buf), "%02d: #96FF00 %s#", gpp_idx++, part->name);
				if (gpp_idx < 2)
					s_printf(txt_buf + strlen(txt_buf), " ");
				s_printf(txt_buf + strlen(txt_buf), " Size: %d MiB (Sect: 0x%4X), Range: %06X-%06X\n",
					(part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
					part->lba_end - part->lba_start + 1, part->lba_start, part->lba_end);
			}
			else
			{
				s_printf(txt_buf + strlen(txt_buf), "%02d: #96FF00 %s#\n    Size: %6d MiB (Sect: 0x%07X), Range: %07X-%07X\n",
					gpp_idx++, part->name, (part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
					part->lba_end - part->lba_start + 1, part->lba_start, part->lba_end);
			}

		}
		nx_emmc_gpt_free(&gpt);

		lv_label_set_array_text(lb_desc2, txt_buf, 0x1000);
		lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
		lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 6, 0);
	}

	sdmmc_storage_end(&storage);
	free(txt_buf);

	return LV_RES_OK;
}

static lv_res_t _create_window_sdcard_info_status(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_SD" microSD Card Info");

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES / 2 / 5 * 2, LV_VER_RES - (LV_DPI * 11 / 7) * 5 / 2);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);

	lv_label_set_static_text(lb_desc, "#D4FF00 Please wait...#");
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	// Disable buttons.
	nyx_window_toggle_buttons(win, true);

	manual_system_maintenance(true);

	if (!sd_mount())
		lv_label_set_static_text(lb_desc, "#FFDD00 Failed to init SD!#");
	else
	{
		lv_label_set_static_text(lb_desc,
			"#00DDFF Card IDentification:#\n"
			"Vendor ID:\n"
			"OEM ID:\n"
			"Model:\n"
			"HW rev:\n"
			"FW rev:\n"
			"S/N:\n"
			"Month/Year:"
		);

		lv_obj_t *val = lv_cont_create(win, NULL);
		lv_obj_set_size(val, LV_HOR_RES / 9 * 2, LV_VER_RES - (LV_DPI * 11 / 7) * 5 / 2);

		lv_obj_t * lb_val = lv_label_create(val, lb_desc);

		char *txt_buf = (char *)malloc(0x1000);

		s_printf(txt_buf,"\n%02x\n%c%c\n%c%c%c%c%c\n%X\n%X\n%08x\n%02d/%04d",
			sd_storage.cid.manfid, (sd_storage.cid.oemid >> 8) & 0xFF, sd_storage.cid.oemid & 0xFF,
			sd_storage.cid.prod_name[0], sd_storage.cid.prod_name[1], sd_storage.cid.prod_name[2],
			sd_storage.cid.prod_name[3], sd_storage.cid.prod_name[4],
			sd_storage.cid.hwrev, sd_storage.cid.fwrev, sd_storage.cid.serial,
			sd_storage.cid.month, sd_storage.cid.year);

		lv_label_set_array_text(lb_val, txt_buf, 0x1000);

		lv_obj_set_width(lb_val, lv_obj_get_width(val));
		lv_obj_align(val, desc, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		lv_obj_t *desc2 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc2, LV_HOR_RES / 2 / 4 * 2, LV_VER_RES - (LV_DPI * 11 / 7) * 5 / 2);

		lv_obj_t * lb_desc2 = lv_label_create(desc2, lb_desc);

		lv_label_set_static_text(lb_desc2,
			"#00DDFF Card-Specific Data#\n"
			"Cmd Classes:\n"
			"Capacity:\n"
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
		lv_obj_set_size(val2, LV_HOR_RES / 13 * 3, LV_VER_RES - (LV_DPI * 11 / 7) * 5 / 2);

		lv_obj_t * lb_val2 = lv_label_create(val2, lb_desc);


		u32 capacity = sd_storage.csd.capacity >> (20 - sd_storage.csd.read_blkbits);
		s_printf(txt_buf, "#00DDFF v%d.0#\n%02X\n%d MiB\n%d\n%d MB/s (%d MHz)\n%d\nU%d\nV%d\nA%d\n%d",
			sd_storage.csd.structure + 1, sd_storage.csd.cmdclass, capacity,
			sd_storage.ssr.bus_width, sd_storage.csd.busspeed, sd_storage.csd.busspeed * 2,
			sd_storage.ssr.speed_class, sd_storage.ssr.uhs_grade, sd_storage.ssr.video_class,
			sd_storage.ssr.app_class, sd_storage.csd.write_protect);

		lv_label_set_array_text(lb_val2, txt_buf, 0x1000);

		lv_obj_set_width(lb_val2, lv_obj_get_width(val2));
		lv_obj_align(val2, desc2, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		lv_obj_t *line_sep = lv_line_create(win, NULL);
		static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 12, 0} };
		lv_line_set_points(line_sep, line_pp, 2);
		lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
		lv_obj_align(line_sep, desc, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI * 410 / 100, LV_DPI / 5);

		lv_obj_t *desc3 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc3, LV_HOR_RES / 2 / 2 * 2, LV_VER_RES - (LV_DPI * 11 / 8) * 4);

		lv_obj_t * lb_desc3 = lv_label_create(desc3, lb_desc);

		lv_label_set_static_text(lb_desc3, "#D4FF00 Acquiring FAT volume info...#");

		lv_obj_set_width(lb_desc3, lv_obj_get_width(desc3));
		lv_obj_align(desc3, desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);

		manual_system_maintenance(true);

		f_getfree("", &sd_fs.free_clst, NULL);

		lv_label_set_static_text(lb_desc3,
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

		s_printf(txt_buf, "\n%s\n%d KiB\n%d MiB",
			sd_fs.fs_type == FS_EXFAT ? ("exFAT  "SYMBOL_SHRK) : ("FAT32"),
			(sd_fs.csize > 1) ? (sd_fs.csize >> 1) : 512,
			sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF);

		lv_label_set_array_text(lb_val3, txt_buf, 0x1000);

		lv_obj_set_width(lb_val3, lv_obj_get_width(val3));
		lv_obj_align(val3, desc3, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		free(txt_buf);
		sd_unmount(false);
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
	);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	lv_obj_t *val = lv_cont_create(win, NULL);
	lv_obj_set_size(val, LV_HOR_RES / 5, LV_VER_RES - (LV_DPI * 11 / 7));

	lv_obj_t * lb_val = lv_label_create(val, lb_desc);

	char *txt_buf = (char *)malloc(0x1000);
	int value = 0;

	max17050_get_property(MAX17050_RepSOC, &value);
	s_printf(txt_buf, "\n%d %\n", value >> 8);

	max17050_get_property(MAX17050_RepCap, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mAh\n", value);

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
	s_printf(txt_buf + strlen(txt_buf), "%d mV\n", value);

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
		s_printf(txt_buf + strlen(txt_buf), "%d.%d oC\n", value / 10, value % 10);
	else
		s_printf(txt_buf + strlen(txt_buf), "-%d.%d oC\n", (~value + 1) / 10, (~value + 1) % 10);

	lv_label_set_array_text(lb_val, txt_buf, 0x1000);

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
		"Temperature status:"
	);
	lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
	lv_obj_align(desc2, val, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 2, 0);

	lv_obj_t *val2 = lv_cont_create(win, NULL);
	lv_obj_set_size(val2, LV_HOR_RES / 2 / 3, LV_VER_RES - (LV_DPI * 11 / 7) - 5);

	lv_obj_t * lb_val2 = lv_label_create(val2, lb_desc);

	bq24193_get_property(BQ24193_InputVoltageLimit, &value);
	s_printf(txt_buf, "\n%d mV\n", value);

	bq24193_get_property(BQ24193_InputCurrentLimit, &value);
	s_printf(txt_buf + strlen(txt_buf), "%d mA\n", value);

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
		s_printf(txt_buf + strlen(txt_buf), "Not charging\n");
		break;
	case 1:
		s_printf(txt_buf + strlen(txt_buf), "Pre-charging\n");
		break;
	case 2:
		s_printf(txt_buf + strlen(txt_buf), "Fast charging\n");
		break;
	case 3:
		s_printf(txt_buf + strlen(txt_buf), "Charge terminated\n");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "Unknown (%d)\n", value);
		break;
	}

	bq24193_get_property(BQ24193_TempStatus, &value);
	switch (value)
	{
	case 0:
		s_printf(txt_buf + strlen(txt_buf), "Normal");
		break;
	case 2:
		s_printf(txt_buf + strlen(txt_buf), "Warm");
		break;
	case 3:
		s_printf(txt_buf + strlen(txt_buf), "Cool");
		break;
	case 5:
		s_printf(txt_buf + strlen(txt_buf), "Cold");
		break;
	case 6:
		s_printf(txt_buf + strlen(txt_buf), "Hot");
		break;
	default:
		s_printf(txt_buf + strlen(txt_buf), "Unknown (%d)", value);
		break;
	}

	lv_label_set_array_text(lb_val2, txt_buf, 0x1000);

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
	lv_label_set_static_text(label_txt, "SoC Info");
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

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"View Ipatches and dump the unpatched and patched versions\nof BootROM.\n"
		"Or view and dump the device's TSEC Keys.\n");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	static lv_style_t line_style;
	lv_style_copy(&line_style, th->line.decor);
	line_style.line.color = LV_COLOR_HEX3(0x444);

	line_sep = lv_line_create(h1, line_sep);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 16);
	lv_line_set_style(line_sep, &line_style);

	// Create Fuses button.
	lv_obj_t *btn3 = lv_btn_create(h1, NULL);
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_CIRCUIT"  Fuses ");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 2);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _create_window_fuses_info_status);
	
	// Create KFuses button.
	lv_obj_t *btn4 = lv_btn_create(h1, btn);
	label_btn = lv_label_create(btn4, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_SHUFFLE"  KFuses");
	lv_obj_align(btn4, btn3, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 123 / 100, 0);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _kfuse_dump_window_action);

	lv_obj_t *label_txt4 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"View and dump your cached fuses and KFuses.\n"
		"Fuses contain info about your SoC and device and KFuses\n"
		"contain downstream and upstream HDCP keys used by HDMI.");
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
		"View info about your eMMC or microSD and additionally\n"
		"view their partition list and info.");
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

#pragma GCC pop_options
