/*
 * Copyright (c) 2019-2020 CTCaer
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

#include <stdlib.h>

#include "gui.h"
#include "fe_emummc_tools.h"
#include "gui_tools_partition_manager.h"
#include <memory_map.h>
#include <utils/ini.h>
#include <libs/fatfs/ff.h>
#include <mem/heap.h>
#include <storage/mbr_gpt.h>
#include <storage/nx_sd.h>
#include <storage/sdmmc.h>
#include <utils/dirlist.h>
#include <utils/list.h>
#include <utils/sprintf.h>
#include <utils/types.h>
#include <libs/lvgl/lv_objx/lv_list.h>

extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

typedef struct _mbr_ctxt_t
{
	u32 available;
	u32 sector[3];
	int part_idx;
	u32 sector_start;
} mbr_ctxt_t;

static mbr_ctxt_t mbr_ctx;

static lv_obj_t *emummc_manage_window;
static lv_res_t (*emummc_tools)(lv_obj_t *btn);

static lv_res_t _action_emummc_window_close(lv_obj_t *btn)
{
	lv_win_close_action(btn);
	lv_obj_del(emummc_manage_window);

	(*emummc_tools)(NULL);

	close_btn = NULL;

	return LV_RES_INV;
}

static void _create_window_emummc()
{
	emmc_tool_gui_t emmc_tool_gui_ctxt;

	lv_obj_t *win;
	if (!mbr_ctx.part_idx)
		win = nyx_create_window_custom_close_btn(SYMBOL_DRIVE"  Create SD File emuMMC", _action_emummc_window_close);
	else
		win = nyx_create_window_custom_close_btn(SYMBOL_DRIVE"  Create SD Partition emuMMC", _action_emummc_window_close);

	//Disable buttons.
	nyx_window_toggle_buttons(win, true);

	// Create important info container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 5);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	static lv_style_t h_style;
	lv_style_copy(&h_style, lv_cont_get_style(h1));
	h_style.body.main_color = LV_COLOR_HEX(0x1d1d1d);
	h_style.body.grad_color = h_style.body.main_color;
	h_style.body.opa = LV_OPA_COVER;

	// Chreate log container.
	lv_obj_t *h2 = lv_cont_create(win, h1);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, false);
	lv_obj_set_size(h2, (LV_HOR_RES / 11) * 4, LV_DPI * 5);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, 0, LV_DPI / 5);

	lv_obj_t *label_log = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_log, true);
	lv_obj_set_style(label_log, &monospace_text);
	lv_label_set_long_mode(label_log, LV_LABEL_LONG_BREAK);
	lv_label_set_static_text(label_log, "");
	lv_obj_set_width(label_log, lv_obj_get_width(h2));
	lv_obj_align(label_log, h2, LV_ALIGN_IN_TOP_LEFT, LV_DPI / 10, LV_DPI / 10);
	emmc_tool_gui_ctxt.label_log = label_log;

	// Create elements for info container.
	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_info = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_info, true);
	lv_obj_set_width(label_info, lv_obj_get_width(h1));
	lv_label_set_static_text(label_info, "\n\n\n\n\n\n\n\n\n");
	lv_obj_set_style(label_info, lv_theme_get_current()->label.prim);
	lv_obj_align(label_info, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 10);
	emmc_tool_gui_ctxt.label_info = label_info;

	lv_obj_t *bar = lv_bar_create(h1, NULL);
	lv_obj_set_size(bar, LV_DPI * 38 / 10, LV_DPI / 5);
	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_value(bar, 0);
	lv_obj_align(bar, label_info, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 8);
	lv_obj_set_opa_scale(bar, LV_OPA_0);
	lv_obj_set_opa_scale_enable(bar, true);
	emmc_tool_gui_ctxt.bar = bar;

	lv_obj_t *label_pct= lv_label_create(h1, NULL);
	lv_label_set_recolor(label_pct, true);
	lv_label_set_static_text(label_pct, " "SYMBOL_DOT" 0%");
	lv_obj_set_style(label_pct, lv_theme_get_current()->label.prim);
	lv_obj_align(label_pct, bar, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 20, 0);
	lv_obj_set_opa_scale(label_pct, LV_OPA_0);
	lv_obj_set_opa_scale_enable(label_pct, true);
	emmc_tool_gui_ctxt.label_pct = label_pct;

	lv_obj_t *label_finish = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_finish, true);
	lv_label_set_static_text(label_finish, "");
	lv_obj_set_style(label_finish, lv_theme_get_current()->label.prim);
	lv_obj_align(label_finish, bar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 9 / 20);
	emmc_tool_gui_ctxt.label_finish = label_finish;

	if (!mbr_ctx.part_idx)
		dump_emummc_file(&emmc_tool_gui_ctxt);
	else
		dump_emummc_raw(&emmc_tool_gui_ctxt, mbr_ctx.part_idx, mbr_ctx.sector_start);

	nyx_window_toggle_buttons(win, false);
}

static lv_res_t _create_emummc_raw_format(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	// Delete parent mbox.
	mbox_action(btns, txt);

	// Create partition window.
	if (!btn_idx)
		create_window_partition_manager(btns);

	mbr_ctx.part_idx = 0;
	mbr_ctx.sector_start = 0;

	return LV_RES_INV;
}

static lv_res_t _create_emummc_raw_action(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);
	lv_obj_t *bg = lv_obj_get_parent(lv_obj_get_parent(btns));

	mbr_ctx.sector_start = 0x8000; // Protective offset.

	switch (btn_idx)
	{
	case 0:
		mbr_ctx.part_idx = 1;
		mbr_ctx.sector_start += mbr_ctx.sector[0];
		break;
	case 1:
		mbr_ctx.part_idx = 2;
		mbr_ctx.sector_start += mbr_ctx.sector[1];
		break;
	case 2:
		mbr_ctx.part_idx = 3;
		mbr_ctx.sector_start += mbr_ctx.sector[2];
		break;
	default:
		break;
	}

	if (btn_idx < 3)
	{
		lv_obj_set_style(bg, &lv_style_transp);
		_create_window_emummc();
	}

	mbr_ctx.part_idx = 0;
	mbr_ctx.sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static void _create_mbox_emummc_raw()
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_format[] = { "\222Continue", "\222Cancel", "" };
	static char *mbox_btn_parts[] = { "\262Part 1", "\262Part 2", "\262Part 3", "\222Cancel", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	char *txt_buf = (char *)malloc(0x500);
	mbr_t *mbr = (mbr_t *)malloc(sizeof(mbr_t));

	memset(&mbr_ctx, 0, sizeof(mbr_ctxt_t));

	sd_mount();
	sdmmc_storage_read(&sd_storage, 0, 1, mbr);
	sd_unmount();

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400);

	u32 emmc_size_safe = storage.sec_cnt + 0xC000; // eMMC GPP size + BOOT0/1.

	sdmmc_storage_end(&storage);

	for (int i = 1; i < 4; i++)
	{
		u32 part_size = mbr->partitions[i].size_sct;
		u32 part_start = mbr->partitions[i].start_sct;
		u8  part_type = mbr->partitions[i].type;

		// Skip Linux, GPT (Android) and SFD partitions.
		bool valid_part = (part_type != 0x83) && (part_type != 0xEE) && (part_type != 0xFF);

		if ((part_size >= emmc_size_safe) && part_start > 0x8000 && valid_part)
		{
			mbr_ctx.available |= (1 << (i - 1));
			mbr_ctx.sector[i - 1] = part_start;
		}
	}

	if (mbr_ctx.available)
	{
		s_printf(txt_buf,
			"#C7EA46 Found applicable partition(s)!#\n"
			"#FF8000 Choose a partition to continue:#\n\n");
	}
	else
		s_printf(txt_buf, "#FFDD00 Failed to find applicable partition!#\n\n");

	s_printf(txt_buf + strlen(txt_buf),
		"Partition table:\n"
		"#C0C0C0 Part 0: Type: %02x, Start: %08x, Size: %08x#\n"
		"#%s Part 1: Type: %02x, Start: %08x, Size: %08x#\n"
		"#%s Part 2: Type: %02x, Start: %08x, Size: %08x#\n"
		"#%s Part 3: Type: %02x, Start: %08x, Size: %08x#\n",
		mbr->partitions[0].type, mbr->partitions[0].start_sct, mbr->partitions[0].size_sct,
			(mbr_ctx.available & 1) ? "C7EA46" : "C0C0C0",
		mbr->partitions[1].type, mbr->partitions[1].start_sct, mbr->partitions[1].size_sct,
			(mbr_ctx.available & 2) ? "C7EA46" : "C0C0C0",
		mbr->partitions[2].type, mbr->partitions[2].start_sct, mbr->partitions[2].size_sct,
			(mbr_ctx.available & 4) ? "C7EA46" : "C0C0C0",
		mbr->partitions[3].type, mbr->partitions[3].start_sct, mbr->partitions[3].size_sct);

	if (!mbr_ctx.available)
		strcat(txt_buf,
			"\n#FF8000 Do you want to partition your SD card?#\n"
			"#FF8000 (You will be asked on how to proceed)#");

	lv_mbox_set_text(mbox, txt_buf);
	free(txt_buf);
	free(mbr);

	if (mbr_ctx.available)
	{
		// Check available partitions and enable the corresponding buttons.
		if (mbr_ctx.available & 1)
			mbox_btn_parts[0][0] = '\222';
		else
			mbox_btn_parts[0][0] = '\262';
		if (mbr_ctx.available & 2)
			mbox_btn_parts[1][0] = '\222';
		else
			mbox_btn_parts[1][0] = '\262';
		if (mbr_ctx.available & 4)
			mbox_btn_parts[2][0] = '\222';
		else
			mbox_btn_parts[2][0] = '\262';

		lv_mbox_add_btns(mbox, (const char **)mbox_btn_parts, _create_emummc_raw_action);
	}
	else
		lv_mbox_add_btns(mbox, mbox_btn_format, _create_emummc_raw_format);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);
}

static lv_res_t _create_emummc_action(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);
	lv_obj_t *bg = lv_obj_get_parent(lv_obj_get_parent(btns));

	mbr_ctx.part_idx = 0;
	mbr_ctx.sector_start = 0;

	switch (btn_idx)
	{
	case 0:
		lv_obj_set_style(bg, &lv_style_transp);
		_create_window_emummc();
		break;
	case 1:
		_create_mbox_emummc_raw();
		break;
	}

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_mbox_emummc_create(lv_obj_t *btn)
{
	if (!nyx_emmc_check_battery_enough())
		return LV_RES_OK;

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\222SD File", "\222SD Partition", "\222Cancel", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox,
		"Welcome to #C7EA46 emuMMC# creation tool!\n\n"
		"Please choose what type of emuMMC you want to create.\n"
		"#FF8000 SD File# is saved as files in your FAT partition.\n"
		"#FF8000 SD Partition# is saved as raw image in an available partition.");

	lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static void _change_raw_emummc_part_type()
{
	mbr_t *mbr = (mbr_t *)malloc(sizeof(mbr_t));
	sdmmc_storage_read(&sd_storage, 0, 1, mbr);
	mbr->partitions[mbr_ctx.part_idx].type = 0xE0;
	sdmmc_storage_write(&sd_storage, 0, 1, mbr);
	free(mbr);
}

static void _migrate_sd_raw_based()
{
	mbr_ctx.sector_start = 2;

	sd_mount();
	f_mkdir("emuMMC");
	f_mkdir("emuMMC/ER00");

	f_rename("Emutendo", "emuMMC/ER00/Nintendo");
	FIL fp;
	f_open(&fp, "emuMMC/ER00/raw_based", FA_CREATE_ALWAYS | FA_WRITE);
	f_write(&fp, &mbr_ctx.sector_start, 4, NULL);
	f_close(&fp);

	save_emummc_cfg(1, mbr_ctx.sector_start, "emuMMC/ER00");
	sd_unmount();
}

static void _migrate_sd_raw_emummc_based()
{
	char *tmp = (char *)malloc(0x80);
	s_printf(tmp, "emuMMC/RAW%d", mbr_ctx.part_idx);

	sd_mount();
	f_mkdir("emuMMC");
	f_mkdir(tmp);
	strcat(tmp, "/raw_based");

	FIL fp;
	if (!f_open(&fp, tmp, FA_CREATE_ALWAYS | FA_WRITE))
	{
		f_write(&fp, &mbr_ctx.sector_start, 4, NULL);
		f_close(&fp);
	}

	s_printf(tmp, "emuMMC/RAW%d", mbr_ctx.part_idx);

	_change_raw_emummc_part_type();

	save_emummc_cfg(mbr_ctx.part_idx, mbr_ctx.sector_start, tmp);

	free(tmp);

	sd_unmount();
}

static void _migrate_sd_file_based()
{
	sd_mount();
	f_mkdir("emuMMC");
	f_mkdir("emuMMC/EF00");

	f_rename("Emutendo", "emuMMC/EF00/Nintendo");
	FIL fp;
	f_open(&fp, "emuMMC/EF00/file_based", FA_CREATE_ALWAYS | FA_WRITE);
	f_close(&fp);

	char *path = (char *)malloc(128);
	char *path2 = (char *)malloc(128);
	s_printf(path, "%c%c%c%c%s", 's', 'x', 'o', 's', "/emunand");
	f_rename(path, "emuMMC/EF00/eMMC");

	for (int i = 0; i < 2; i++)
	{
		s_printf(path, "emuMMC/EF00/eMMC/boot%d.bin", i);
		s_printf(path2, "emuMMC/EF00/eMMC/BOOT%d", i);
		f_rename(path, path2);
	}
	for (int i = 0; i < 8; i++)
	{
		s_printf(path, "emuMMC/EF00/eMMC/full.%02d.bin", i);
		s_printf(path2, "emuMMC/EF00/eMMC/%02d", i);
		f_rename(path, path2);
	}

	free(path);
	free(path2);

	save_emummc_cfg(0, 0, "emuMMC/EF00");
	sd_unmount();
}

static void _migrate_sd_backup_file_based()
{
	char *emu_path = (char *)malloc(128);
	char *parts_path = (char *)malloc(128);
	char *backup_path = (char *)malloc(128);
	char *backup_file_path = (char *)malloc(128);

	sd_mount();
	f_mkdir("emuMMC");

	strcpy(emu_path, "emuMMC/BK");
	u32 base_len = strlen(emu_path);

	for (int j = 0; j < 100; j++)
	{
		update_emummc_base_folder(emu_path, base_len, j);
		if(f_stat(emu_path, NULL) == FR_NO_FILE)
			break;
	}
	base_len = strlen(emu_path);

	f_mkdir(emu_path);
	strcat(emu_path, "/eMMC");
	f_mkdir(emu_path);

	FIL fp;
	strcpy(emu_path + base_len, "/file_based");
	f_open(&fp, "emuMMC/BK00/file_based", FA_CREATE_ALWAYS | FA_WRITE);
	f_close(&fp);

	emmcsn_path_impl(backup_path, "", "", NULL);

	s_printf(backup_file_path, "%s/BOOT0", backup_path);
	strcpy(emu_path + base_len, "/eMMC/BOOT0");
	f_rename(backup_file_path, emu_path);

	s_printf(backup_file_path, "%s/BOOT1", backup_path);
	strcpy(emu_path + base_len, "/eMMC/BOOT1");
	f_rename(backup_file_path, emu_path);

	bool multipart = false;
	s_printf(backup_file_path, "%s/rawnand.bin", backup_path);

	if(f_stat(backup_file_path, NULL))
		multipart = true;

	if (!multipart)
	{
		strcpy(emu_path + base_len, "/eMMC/00");
		f_rename(backup_file_path, emu_path);
	}
	else
	{
		emu_path[base_len] = 0;
		for (int i = 0; i < 32; i++)
		{
			s_printf(backup_file_path, "%s/rawnand.bin.%02d", backup_path, i);
			s_printf(parts_path, "%s/eMMC/%02d", emu_path, i);
			if (f_rename(backup_file_path, parts_path))
				break;
		}
	}

	free(emu_path);
	free(parts_path);
	free(backup_path);
	free(backup_file_path);

	save_emummc_cfg(0, 0, "emuMMC/BK00");
	sd_unmount();
}

static lv_res_t _create_emummc_mig1_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_file_based();
		break;
	case 1:
		_migrate_sd_raw_based();
		break;
	}

	mbr_ctx.part_idx = 0;
	mbr_ctx.sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_emummc_mig0_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_file_based();
		break;
	}

	mbr_ctx.part_idx = 0;
	mbr_ctx.sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_emummc_mig2_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_raw_based();
		break;
	}

	mbr_ctx.part_idx = 0;
	mbr_ctx.sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_emummc_mig3_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_raw_emummc_based();
		break;
	}

	mbr_ctx.part_idx = 0;
	mbr_ctx.sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_emummc_mig4_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_backup_file_based();
		break;
	}

	mbr_ctx.part_idx = 0;
	mbr_ctx.sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_mbox_emummc_migrate(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\222Continue", "\222Cancel", "" };
	static const char *mbox_btn_map1[] = { "\222SD File", "\222SD Partition", "\222Cancel", "" };
	static const char *mbox_btn_map3[] = { "\211", "OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	char *txt_buf = (char *)malloc(0x500);
	mbr_t *mbr = (mbr_t *)malloc(sizeof(mbr_t));
	u8 *efi_part = (u8 *)malloc(0x200);

	sd_mount();
	sdmmc_storage_read(&sd_storage, 0, 1, mbr);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400);

	bool backup = false;
	bool emummc = false;
	bool file_based = false;
	bool em = false;
	mbr_ctx.sector_start = 0;
	mbr_ctx.part_idx = 0;

	for (int i = 1; i < 4; i++)
	{
		mbr_ctx.sector_start = mbr->partitions[i].start_sct;
		if (mbr_ctx.sector_start)
		{
			sdmmc_storage_read(&sd_storage, mbr_ctx.sector_start + 0xC001, 1, efi_part);
			if (!memcmp(efi_part, "EFI PART", 8))
			{
				mbr_ctx.sector_start += 0x8000;
				emummc = true;
				mbr_ctx.part_idx = i;
				break;
			}
			else
			{
				sdmmc_storage_read(&sd_storage, mbr_ctx.sector_start + 0x4001, 1, efi_part);
				if (!memcmp(efi_part, "EFI PART", 8))
				{
					emummc = true;
					mbr_ctx.part_idx = i;
					break;
				}
			}
		}
	}

	//! TODO: What about unallocated

	if (!mbr_ctx.part_idx)
	{
		sdmmc_storage_read(&sd_storage, 0x4003, 1, efi_part);
		if (!memcmp(efi_part, "EFI PART", 8))
			em = true;
	}

	s_printf(txt_buf, "%c%c%c%c%s", 's', 'x', 'o','s', "/emunand/boot0.bin");

	if(!f_stat(txt_buf, NULL))
		file_based = true;

	bool rawnand_backup_found = false;

	emmcsn_path_impl(txt_buf, "", "BOOT0", &storage);
	if(!f_stat(txt_buf, NULL))
		backup = true;

	emmcsn_path_impl(txt_buf, "", "rawnand.bin", &storage);
	if(!f_stat(txt_buf, NULL))
		rawnand_backup_found = true;

	emmcsn_path_impl(txt_buf, "", "rawnand.bin.00", &storage);
	if(!f_stat(txt_buf, NULL))
		rawnand_backup_found = true;

	if (backup && rawnand_backup_found)
		backup = true;
	else
		backup = false;

	sd_unmount();
	sdmmc_storage_end(&storage);

	if (backup)
	{
		s_printf(txt_buf,
			"#C7EA46 Found suitable backup for emuMMC!#\n"
			"#FF8000 Do you want to migrate it?#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_mig4_action);
	}
	else if (emummc)
	{
		s_printf(txt_buf,
			"#C7EA46 Found SD Partition based emuMMC!#\n"
			"#FF8000 Do you want to repair the config for it?#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_mig3_action);
	}
	else if (em && !file_based)
	{
		s_printf(txt_buf,
			"#C7EA46 Found foreign SD Partition emunand!#\n"
			"#FF8000 Do you want to migrate it?#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_mig2_action);
	}
	else if (!em && file_based)
	{
		s_printf(txt_buf,
			"#C7EA46 Found foreign SD File emunand!#\n"
			"#FF8000 Do you want to migrate it?#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_mig0_action);
	}
	else if (em && file_based)
	{
		s_printf(txt_buf,
			"#C7EA46 Found both foreign SD File and Partition emunand!#\n"
			"#FF8000 Choose what to migrate:#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map1, _create_emummc_mig1_action);
	}
	else
	{
		s_printf(txt_buf, "No emuMMC or foreign emunand found!\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map3, mbox_action);
	}

	lv_mbox_set_text(mbox, txt_buf);
	free(txt_buf);
	free(mbr);
	free(efi_part);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

typedef struct _emummc_images_t
{
	char *dirlist;
	char *dirlist2;
	u32 part_sector[127];
	u32 part_type[127];
	u32 part_end[127];
	u32 part_size[127];
	char part_path[127 * 128];
	lv_obj_t *win;
} emummc_images_t;

static emummc_images_t *emummc_img;

static lv_res_t _save_emummc_cfg_mbox_action(lv_obj_t *btns, const char *txt)
{
	free(emummc_img->dirlist);
	lv_obj_del(emummc_img->win);
	lv_obj_del(emummc_manage_window);
	free(emummc_img);

	mbox_action(btns, txt);

	(*emummc_tools)(NULL);

	return LV_RES_INV;
}

static void _create_emummc_saved_mbox()
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\211", "OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 4);

	lv_mbox_set_text(mbox,
		"#FF8000 emuMMC Configuration#\n\n"
		"#96FF00 The emuMMC configuration#\n#96FF00 was saved to sd card!#");

	lv_mbox_add_btns(mbox, mbox_btn_map, _save_emummc_cfg_mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);
}

lv_obj_t *list_raw_based;

static lv_res_t _save_raw_emummc_cfg_action(lv_obj_t * btn)
{
	int idx = lv_list_get_btn_index(list_raw_based, btn);
	char emupath[12];
	s_printf(emupath, "emuMMC/RAW%d", idx + 1);
	
	save_emummc_cfg(idx + 1, emummc_img->part_sector[idx], emupath);

	mbr_t *mbr = (mbr_t *)malloc(sizeof(mbr_t));
	gpt_t *gpt = (gpt_t *)malloc(sizeof(gpt_t));
	
	sdmmc_storage_read(&sd_storage, 0, 1, mbr);
	sdmmc_storage_read(&sd_storage, 1, 33, gpt);
	
	mbr->partitions[1].start_sct = emummc_img->part_sector[idx];
	mbr->partitions[1].size_sct = emummc_img->part_size[idx];
	sdmmc_storage_write(&sd_storage, 0, 1, mbr);
	
	free(mbr);
	free(gpt);
	
	_create_emummc_saved_mbox();
	sd_unmount();

	return LV_RES_INV;
}

static lv_res_t _save_disable_emummc_cfg_action(lv_obj_t * btn)
{
	save_emummc_cfg(0, 0, NULL);
	_create_emummc_saved_mbox();
	sd_unmount();

	return LV_RES_INV;
}

static lv_res_t _save_file_emummc_cfg_action(lv_obj_t *btn)
{
	save_emummc_cfg(0, 0, lv_list_get_btn_text(btn));
	_create_emummc_saved_mbox();
	sd_unmount();

	return LV_RES_INV;
}

static lv_res_t _create_change_emummc_window(lv_obj_t *btn_caller)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_SETTINGS"  Change emuMMC");
	lv_win_add_btn(win, NULL, SYMBOL_POWER"  Disable", _save_disable_emummc_cfg_action);

	sd_mount();

	emummc_img = malloc(sizeof(emummc_images_t));
	emummc_img->win = win;

	mbr_t *mbr = (mbr_t *)malloc(sizeof(mbr_t));
	gpt_t *gpt = (gpt_t *)malloc(sizeof(gpt_t));
	char *path = malloc(256);

	sdmmc_storage_read(&sd_storage, 1, 33, gpt);

	memset(emummc_img->part_path, 0, 127 * 128);
	int j = 0;
	int k;
	for (int i = 1; i < 128; i++)
	{
		char sect1[512];
		sdmmc_storage_read(&sd_storage, gpt->entries[i].lba_start, 1, sect1);
		
		if (sect1[1] == 0x02 && sect1[4] == 0x0F && sect1[5] == 0x0E)
		{
			emummc_img->part_sector[j] = gpt->entries[i].lba_start;
			emummc_img->part_end[j] = gpt->entries[i].lba_end;
			emummc_img->part_size[j] = (u32)(gpt->entries[i].lba_end - gpt->entries[i].lba_start);
			j++;
		}
		k = i;
	}
	
	free(mbr);
	free(gpt);

	emummc_img->dirlist = dirlist("emuMMC", NULL, false, true);
	emummc_img->dirlist2 = dirlist("emuMMC", NULL, false, true);

	if (!emummc_img->dirlist)
		goto out0;
	if (!emummc_img->dirlist2)
		goto out0;

	u32 emummc_idx = 0;

	FIL fp;

	// Check for sd raw partitions, based on the folders in /emuMMC.
	while (emummc_img->dirlist[emummc_idx * 256])
	{
		s_printf(path, "emuMMC/%s/raw_based", &emummc_img->dirlist[emummc_idx * 256]);

		if(!f_stat(path, NULL))
		{
			f_open(&fp, path, FA_READ);
			u32 curr_list_sector = 0;
			f_read(&fp, &curr_list_sector, 4, NULL);
			f_close(&fp);

			// Check if there's a HOS image there.
			if ((curr_list_sector == 2) || (emummc_img->part_sector[0] && curr_list_sector >= emummc_img->part_sector[0] &&
				curr_list_sector < emummc_img->part_end[0]))
			{
				s_printf(&emummc_img->part_path[0], "emuMMC/%s", &emummc_img->dirlist[emummc_idx * 256]);
				emummc_img->part_sector[0] = curr_list_sector;
				emummc_img->part_end[0] = 0;
			}
			else if (emummc_img->part_sector[1] && curr_list_sector >= emummc_img->part_sector[1] &&
				curr_list_sector < emummc_img->part_end[1])
			{
				s_printf(&emummc_img->part_path[1 * 128], "emuMMC/%s", &emummc_img->dirlist[emummc_idx * 256]);
				emummc_img->part_sector[1] = curr_list_sector;
				emummc_img->part_end[1] = 0;
			}
			else if (emummc_img->part_sector[2] && curr_list_sector >= emummc_img->part_sector[2] &&
				curr_list_sector < emummc_img->part_end[2])
			{
				s_printf(&emummc_img->part_path[2 * 128], "emuMMC/%s", &emummc_img->dirlist[emummc_idx * 256]);
				emummc_img->part_sector[2] = curr_list_sector;
				emummc_img->part_end[2] = 0;
			}
		}
		emummc_idx++;
	}

	emummc_idx = 0;
	u32 file_based_idx = 0;

	// Sanitize the directory list with sd file based ones.
	while (emummc_img->dirlist2[emummc_idx * 256])
	{
		s_printf(path, "emuMMC/%s/file_based", &emummc_img->dirlist2[emummc_idx * 256]);

		if(!f_stat(path, NULL))
		{
			char *tmp = &emummc_img->dirlist2[emummc_idx * 256];
			memcpy(&emummc_img->dirlist2[file_based_idx * 256], tmp, strlen(tmp) + 1);
			file_based_idx++;
		}
		emummc_idx++;
	}
	emummc_img->dirlist2[file_based_idx * 256] = 0;

out0:;
	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 6;

	// Create SD Raw Partitions container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "SD Raw Partitions");
	lv_obj_set_style(label_txt, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -(LV_DPI / 2));

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	lv_obj_t *btn = NULL;
	lv_btn_ext_t *ext;
	lv_obj_t *btn_label = NULL;
	lv_obj_t *lv_desc = NULL;
	char *txt_buf = malloc(0x500);

	// Create RAW buttons.
	list_raw_based = lv_list_create(h1, NULL);
	lv_obj_align(list_raw_based, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 2, LV_DPI / 4);

	lv_obj_set_size(list_raw_based, LV_HOR_RES * 4 / 10, LV_VER_RES * 6 / 10);
	lv_list_set_single_mode(list_raw_based, true);
	
	for (int i = 0; i < j; i++)
	{
		char sect1[512];
		sdmmc_storage_read(&sd_storage, emummc_img->part_sector[i], 1, sect1);
		
		char sectstart[26];
		s_printf(sectstart, "sector %x", emummc_img->part_sector[i]);

		if (sect1[1] == 0x02 && sect1[4] == 0x0F && sect1[5] == 0x0E)
		lv_list_add(list_raw_based, NULL, sectstart, _save_raw_emummc_cfg_action);
	}

	// Create SD File Based container.
	lv_obj_t *h2 = lv_cont_create(win, NULL);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, true);
	lv_obj_set_width(h2, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h2, false);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 17 / 29, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "SD File Based");
	lv_obj_set_style(label_txt3, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI / 7);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 2), LV_DPI / 8);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);

	lv_obj_t *list_sd_based = lv_list_create(h2, NULL);
	lv_obj_align(list_sd_based, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 2, LV_DPI / 4);

	lv_obj_set_size(list_sd_based, LV_HOR_RES * 4 / 10, LV_VER_RES * 6 / 10);
	lv_list_set_single_mode(list_sd_based, true);

	if (!emummc_img->dirlist)
		goto out1;

	emummc_idx = 0;

	// Add file based to the list.
	while (emummc_img->dirlist2[emummc_idx * 256])
	{
		s_printf(path, "emuMMC/%s", &emummc_img->dirlist2[emummc_idx * 256]);

		lv_list_add(list_sd_based, NULL, path, _save_file_emummc_cfg_action);

		emummc_idx++;
	}

out1:
	free(path);
	sd_unmount();

	return LV_RES_OK;
}

lv_res_t create_win_emummc_tools(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_EDIT"  emuMMC Manage");
	emummc_manage_window = win;

	emummc_tools = (void *)create_win_emummc_tools;

	sd_mount();

	emummc_cfg_t emu_info;
	load_emummc_cfg(&emu_info);

	sd_unmount();

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 9;

	// Create emuMMC Info & Selection container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "emuMMC Info & Selection");
	lv_obj_set_style(label_txt, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI / 9);

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create emuMMC info labels.
	lv_obj_t *label_btn = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_btn, true);
	lv_label_set_static_text(label_btn, emu_info.enabled ? "#96FF00 "SYMBOL_OK"  Enabled!#" : "#FF8000 "SYMBOL_CLOSE"  Disabled!#");
	lv_obj_align(label_btn, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	char *txt_buf = (char *)malloc(0x200);

	if (emu_info.enabled)
	{
		if (emu_info.sector)
			s_printf(txt_buf, "#00DDFF Type:# SD Raw Partition\n#00DDFF Sector:# 0x%08X\n#00DDFF Nintendo folder:# %s",
				emu_info.sector, emu_info.nintendo_path ? emu_info.nintendo_path : "");
		else
			s_printf(txt_buf, "#00DDFF Type:# SD File\n#00DDFF Base folder:# %s\n#00DDFF Nintendo folder:# %s",
				emu_info.path ? emu_info.path : "", emu_info.nintendo_path ? emu_info.nintendo_path : "");

		lv_label_set_text(label_txt2, txt_buf);
	}
	else
	{
		lv_label_set_static_text(label_txt2, "emuMMC is disabled and eMMC will be used for boot.\n\n");
	}

	free(txt_buf);

	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, label_btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Change emuMMC button.
	lv_obj_t *btn2 = lv_btn_create(h1, NULL);
	lv_btn_set_fit(btn2, true, true);
	label_btn = lv_label_create(btn2, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_SETTINGS"  Change emuMMC");
	lv_obj_align(btn2, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 6 / 10);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, _create_change_emummc_window);

	label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Choose between images created in the emuMMC folder\n"
		"or in SD card partitions. You can have as many emummc's\nboth partition and file based as your sd will fit.");

	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create emuMMC Tools container.
	lv_obj_t *h2 = lv_cont_create(win, NULL);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, true);
	lv_obj_set_width(h2, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h2, false);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 17 / 29, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "emuMMC Tools");
	lv_obj_set_style(label_txt3, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, 0);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create Create emuMMC button.
	lv_obj_t *btn3 = lv_btn_create(h2, btn2);
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_DRIVE"  Create emuMMC");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _create_mbox_emummc_create);

	lv_obj_t *label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"Allows you to create a new #C7EA46 SD File# or #C7EA46 SD Raw Partition#\n"
		"emuMMC. You can create it from eMMC or a eMMC Backup.");

	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	//! TODO: Move it to a window with multiple choices.
	// Create Migrate emuMMC button.
	lv_obj_t *btn4 = lv_btn_create(h2, btn2);
	label_btn = lv_label_create(btn4, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_SHUFFLE"  Migrate emuMMC");
	lv_obj_align(btn4, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, NULL);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _create_mbox_emummc_migrate);

	label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"Migrate a backup to a #C7EA46 SD File# or repair existing #C7EA46 SD Raw Partition#.\n"
		"Additionally it allows you to migrate from other emunand\nsolutions.");
		//"Move between #C7EA46 SD File# and #C7EA46 SD Raw Partition# emuMMC.\n"
		//"Additionally it allows you to migrate from other emunand\nsolutions.");
	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	return LV_RES_OK;
}
