/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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
#include "gui_emmc_tools.h"
#include "../config/config.h"
#include "../hos/pkg1.h"
#include "../hos/pkg2.h"
#include "../hos/hos.h"
#include "../hos/sept.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../sec/se.h"
#include "../soc/fuse.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/sprintf.h"
#include "../utils/util.h"

extern volatile boot_cfg_t *b_cfg;
extern hekate_config h_cfg;

extern bool sd_mount();
extern void sd_unmount(bool deinit);
extern int  sd_save_to_file(void *buf, u32 size, const char *filename);
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

#pragma GCC push_options
#pragma GCC target ("thumb")

bool get_autorcm_status(bool change)
{
	u8 corr_mod_byte0;
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	bool enabled = false;

	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	u8 *tempbuf = (u8 *)malloc(0x200);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x200 / NX_EMMC_BLOCKSIZE, 1, tempbuf);

	if ((fuse_read_odm(4) & 3) != 3)
		corr_mod_byte0 = 0xF7;
	else
		corr_mod_byte0 = 0x37;

	if (tempbuf[0x10] != corr_mod_byte0)
		enabled = true;

	// Change autorcm status if requested.
	if (change)
	{
		int i, sect = 0;
		u8 randomXor = 0;

		for (i = 0; i < 4; i++)
		{
			sect = (0x200 + (0x4000 * i)) / NX_EMMC_BLOCKSIZE;
			sdmmc_storage_read(&storage, sect, 1, tempbuf);

			if (!enabled)
			{
				do
				{
					randomXor = get_tmr_us() & 0xFF; // Bricmii style of bricking.
				} while (!randomXor); // Avoid the lottery.

				tempbuf[0x10] ^= randomXor;
			}
			else
				tempbuf[0x10] = corr_mod_byte0;
			sdmmc_storage_write(&storage, sect, 1, tempbuf);
		}
		enabled = !(enabled);
	}

	free(tempbuf);
	sdmmc_storage_end(&storage);

	return enabled;
}

static lv_res_t _create_mbox_autorcm_status(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	bool enabled = get_autorcm_status(true);

	if (enabled)
	{
		lv_mbox_set_text(mbox,
			"AutoRCM is now #C7EA46 ENABLED!#\n\n"
			"You can now automatically enter RCM by only pressing #FF8000 POWER#.\n"
			"Use the AutoRCM button here again if you want to remove it later on.");
	}
	else
	{
		lv_mbox_set_text(mbox,
			"AutoRCM is now #FF8000 DISABLED!#\n\n"
			"The boot process is now normal and you need the #FF8000 VOL+# + #FF8000 HOME# (jig) combo to enter RCM.\n");
	}

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	if (enabled)
		lv_btn_set_state(btn, LV_BTN_STATE_TGL_REL);
	else
		lv_btn_set_state(btn, LV_BTN_STATE_REL);
	nyx_generic_onoff_toggle(btn);

	return LV_RES_OK;
}

static int _fix_attributes(u32 *ufidx, lv_obj_t *lb_val, char *path, u32 *total, u32 hos_folder, u32 check_first_run)
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

			if (*ufidx == 0)
				lv_label_set_array_text(lb_val, path, 256);
			*ufidx += 1;
			if (*ufidx > 9)
				*ufidx = 0;
		}

		manual_system_maintenance(true);

		// Is it a directory?
		if (fno.fattrib & AM_DIR)
		{
			// Set archive bit to NCA folders.
			if (hos_folder && !strcmp(fno.fname + strlen(fno.fname) - 4, ".nca"))
			{
				*total = *total + 1;
				f_chmod(path, AM_ARC, AM_ARC);
			}
			lv_label_set_array_text(lb_val, path, 256);
			manual_system_maintenance(true);

			// Enter the directory.
			res = _fix_attributes(ufidx, lb_val, path, total, hos_folder, 0);
			if (res != FR_OK)
				break;
		}
	}

	f_closedir(&dir);

	return res;
}

static lv_res_t _create_window_unset_abit_tool(lv_obj_t *btn)
{
	lv_obj_t *win;

	// Find which was called and set window's title.
	bool nintendo_folder = false;
	if (strcmp(lv_label_get_text(lv_obj_get_child(btn, NULL)), SYMBOL_COPY"  Unset archive bit"))
		nintendo_folder = true;

	if (!nintendo_folder)
		win = nyx_create_standard_window(SYMBOL_COPY" Unset archive bit (except Nintendo folder)");
	else
		win = nyx_create_standard_window(SYMBOL_COPY" Fix archive bit (Nintendo folder)");

	// Disable buttons.
	nyx_window_toggle_buttons(win, true);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES * 10 / 11, LV_VER_RES - (LV_DPI * 11 / 7) * 4);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);

	if (!sd_mount())
	{
		lv_label_set_static_text(lb_desc, "#FFDD00 Failed to init SD!#");
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));
	}
	else
	{
		if (!nintendo_folder)
			lv_label_set_static_text(lb_desc, "#00DDFF Traversing all SD card files!#\nThis may take some time...");
		else
			lv_label_set_static_text(lb_desc, "#00DDFF Traversing all Nintendo files!#\nThis may take some time...");
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

		lv_obj_t *val = lv_cont_create(win, NULL);
		lv_obj_set_size(val, LV_HOR_RES * 10 / 11, LV_VER_RES - (LV_DPI * 11 / 7) * 4);

		lv_obj_t * lb_val = lv_label_create(val, lb_desc);

		char path[256];
		path[0] = 0;

		lv_label_set_static_text(lb_val, "");
		lv_obj_set_width(lb_val, lv_obj_get_width(val));
		lv_obj_align(val, desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

		u32 total = 0;

		if (!nintendo_folder)
			path[0] = 0;
		else
			strcpy(path, "Nintendo");

		u32 ufidx = 0;

		_fix_attributes(&ufidx, lb_val, path, &total, nintendo_folder, nintendo_folder);

		// Also fix the emuMMC Nintendo folders.
		if (nintendo_folder)
		{
			strcpy(path, "emuMMC");
			_fix_attributes(&ufidx, lb_val, path, &total, nintendo_folder, nintendo_folder);
		}

		sd_unmount(false);

		lv_obj_t *desc2 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc2, LV_HOR_RES * 10 / 11, LV_VER_RES - (LV_DPI * 11 / 7) * 4);
		lv_obj_t * lb_desc2 = lv_label_create(desc2, lb_desc);

		char *txt_buf = (char *)malloc(0x500);

		s_printf(txt_buf, "#96FF00 Total archive bits fixed:# #FF8000 %d!#", total);

		lv_label_set_array_text(lb_desc2, txt_buf, 0x500);
		lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
		lv_obj_align(desc2, val, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
	}

	// Enable buttons.
	nyx_window_toggle_buttons(win, false);

	return LV_RES_OK;
}

static lv_res_t _create_window_dump_pk12_tool(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_MODULES" Dump package1/2");

	// Disable buttons.
	nyx_window_toggle_buttons(win, true);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES * 10 / 11, LV_VER_RES - (LV_DPI * 12 / 7));

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_obj_set_style(lb_desc, &monospace_text);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

	if (!sd_mount())
	{
		lv_label_set_static_text(lb_desc, "#FFDD00 Failed to init SD!#");

		goto out_end;
	}

	char path[128];

	u8 *pkg1 = (u8 *)calloc(1, 0x40000);
	u8 *warmboot = (u8 *)calloc(1, 0x40000);
	u8 *secmon = (u8 *)calloc(1, 0x40000);
	u8 *loader = (u8 *)calloc(1, 0x40000);
	u8 *pkg2 = NULL;
	u8 kb = 0;

	char *txt_buf  = (char *)malloc(0x1000);
	char *txt_buf2 = (char *)malloc(0x1000);

	tsec_ctxt_t tsec_ctxt;

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		lv_label_set_static_text(lb_desc, "#FFDD00 Failed to init eMMC!#");

		goto out_free;
	}

	sdmmc_storage_set_mmc_partition(&storage, 1);

	// Read package1.
	char *build_date = malloc(32);
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1, build_date);

	s_printf(txt_buf, "#00DDFF Found pkg1 ('%s')#\n\n", build_date);
	free(build_date);
	lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
	manual_system_maintenance(true);

	// Dump package1 in its encrypted state if unknown.
	if (!pkg1_id)
	{
		s_printf(txt_buf + strlen(txt_buf),
			"#FFDD00 Unknown pkg1 version for reading#\n#FFDD00 TSEC firmware!#");
		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);

		emmcsn_path_impl(path, "/pkg1", "pkg1_enc.bin", &storage);
		if (sd_save_to_file(pkg1, 0x40000, path))
			goto out_free;

		s_printf(txt_buf + strlen(txt_buf), "\nEncrypted pkg1 dumped to pkg1_enc.bin");
		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);

		goto out_free;
	}

	const pk11_hdr_t *hdr = (pk11_hdr_t *)(pkg1 + pkg1_id->pkg11_off + 0x20);

	kb = pkg1_id->kb;

	if (!h_cfg.se_keygen_done)
	{
		tsec_ctxt.fw = (void *)pkg1 + pkg1_id->tsec_off;
		tsec_ctxt.pkg1 = (void *)pkg1;
		tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;
		tsec_ctxt.secmon_base = pkg1_id->secmon_base;

		if (kb >= KB_FIRMWARE_VERSION_700 && !h_cfg.sept_run)
		{
			b_cfg->autoboot = 0;
			b_cfg->autoboot_list = 0;

			if (!reboot_to_sept((u8 *)tsec_ctxt.fw, kb))
			{
				lv_label_set_static_text(lb_desc, "#FFDD00 Failed to run sept#\n");
				goto out_free;
			}
		}

		// Read keyblob.
		u8 *keyblob = (u8 *)calloc(NX_EMMC_BLOCKSIZE, 1);
		sdmmc_storage_read(&storage, 0x180000 / NX_EMMC_BLOCKSIZE + kb, 1, keyblob);

		// Decrypt.
		keygen(keyblob, kb, &tsec_ctxt);
		if (kb <= KB_FIRMWARE_VERSION_600)
			h_cfg.se_keygen_done = 1;
		free(keyblob);
	}

	if (kb <= KB_FIRMWARE_VERSION_600)
		pkg1_decrypt(pkg1_id, pkg1);

	if (kb <= KB_FIRMWARE_VERSION_620)
	{
		pkg1_unpack(warmboot, secmon, loader, pkg1_id, pkg1);

		// Display info.
		s_printf(txt_buf + strlen(txt_buf),
			"#C7EA46 NX Bootloader size:  #0x%05X\n"
			"#C7EA46 Secure monitor addr: #0x%05X\n"
			"#C7EA46 Secure monitor size: #0x%05X\n"
			"#C7EA46 Warmboot addr:       #0x%05X\n"
			"#C7EA46 Warmboot size:       #0x%05X\n\n",
			hdr->ldr_size, pkg1_id->secmon_base, hdr->sm_size, pkg1_id->warmboot_base, hdr->wb_size);

		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);

		// Dump package1.1.
		emmcsn_path_impl(path, "/pkg1", "pkg1_decr.bin", &storage);
		if (sd_save_to_file(pkg1, 0x40000, path))
			goto out_free;
		s_printf(txt_buf + strlen(txt_buf), "pkg1 dumped to pkg1_decr.bin\n");
		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);

		// Dump nxbootloader.
		emmcsn_path_impl(path, "/pkg1", "nxloader.bin", &storage);
		if (sd_save_to_file(loader, hdr->ldr_size, path))
			goto out_free;
		s_printf(txt_buf + strlen(txt_buf), "NX Bootloader dumped to nxloader.bin\n");
		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);

		// Dump secmon.
		emmcsn_path_impl(path, "/pkg1", "secmon.bin", &storage);
		if (sd_save_to_file(secmon, hdr->sm_size, path))
			goto out_free;
		s_printf(txt_buf + strlen(txt_buf), "Secure Monitor dumped to secmon.bin\n");
		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);

		// Dump warmboot.
		emmcsn_path_impl(path, "/pkg1", "warmboot.bin", &storage);
		if (sd_save_to_file(warmboot, hdr->wb_size, path))
			goto out_free;
		s_printf(txt_buf + strlen(txt_buf), "Warmboot dumped to warmboot.bin\n\n");
		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);
	}

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
#if 0
	emmcsn_path_impl(path, "/pkg2", "pkg2_encr.bin", &storage);
	if (sd_save_to_file(pkg2, pkg2_size_aligned, path))
		goto out;
	gfx_puts("\npkg2 dumped to pkg2_encr.bin\n");
#endif

	// Decrypt package2 and parse KIP1 blobs in INI1 section.
	pkg2_hdr_t *pkg2_hdr = pkg2_decrypt(pkg2, kb);
	if (!pkg2_hdr)
	{
		s_printf(txt_buf + strlen(txt_buf), "#FFDD00 Pkg2 decryption failed!#");
		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);

		goto out;
	}

	// Display info.
	s_printf(txt_buf + strlen(txt_buf),
		"#C7EA46 Kernel size:   #0x%05X\n"
		"#C7EA46 INI1 size:     #0x%05X\n\n",
		pkg2_hdr->sec_size[PKG2_SEC_KERNEL], pkg2_hdr->sec_size[PKG2_SEC_INI1]);

	lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
	manual_system_maintenance(true);

	// Dump pkg2.1.
	emmcsn_path_impl(path, "/pkg2", "pkg2_decr.bin", &storage);
	if (sd_save_to_file(pkg2, pkg2_hdr->sec_size[PKG2_SEC_KERNEL] + pkg2_hdr->sec_size[PKG2_SEC_INI1], path))
		goto out;
	s_printf(txt_buf + strlen(txt_buf), "pkg2 dumped to pkg2_decr.bin\n");
	lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
	manual_system_maintenance(true);

	// Dump kernel.
	emmcsn_path_impl(path, "/pkg2", "kernel.bin", &storage);
	if (sd_save_to_file(pkg2_hdr->data, pkg2_hdr->sec_size[PKG2_SEC_KERNEL], path))
		goto out;
	s_printf(txt_buf + strlen(txt_buf), "Kernel dumped to kernel.bin\n");
	lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
	manual_system_maintenance(true);

	// Dump INI1.
	u32 ini1_off = pkg2_hdr->sec_size[PKG2_SEC_KERNEL];
	u32 ini1_size = pkg2_hdr->sec_size[PKG2_SEC_INI1];
	if (!ini1_size)
	{
		pkg2_get_newkern_info(pkg2_hdr->data);
		ini1_off = pkg2_newkern_ini1_start;
		ini1_size = pkg2_newkern_ini1_end - pkg2_newkern_ini1_start;
	}
	pkg2_ini1_t *ini1 = (pkg2_ini1_t *)(pkg2_hdr->data + ini1_off);
	emmcsn_path_impl(path, "/pkg2", "ini1.bin", &storage);
	if (sd_save_to_file(ini1, ini1_size, path))
		goto out;

	s_printf(txt_buf + strlen(txt_buf), "INI1 dumped to ini1.bin\n\n");
	lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
	manual_system_maintenance(true);

	char filename[32];
	u8 *ptr = (u8 *)ini1;
	ptr += sizeof(pkg2_ini1_t);

	// Dump all kips.
	u8 *kip_buffer = (u8 *)malloc(0x400000);

	for (u32 i = 0; i < ini1->num_procs; i++)
	{
		pkg2_kip1_t *kip1 = (pkg2_kip1_t *)ptr;
		u32 kip1_size = pkg2_calc_kip1_size(kip1);

		s_printf(filename, "%s.kip1", kip1->name);
		if ((u32)kip1 % 8)
		{
			memcpy(kip_buffer, kip1, kip1_size);
			kip1 = (pkg2_kip1_t *)kip_buffer;
		}

		emmcsn_path_impl(path, "/pkg2/ini1", filename, &storage);
		if (sd_save_to_file(kip1, kip1_size, path))
		{
			free(kip_buffer);
			goto out;
		}

		s_printf(txt_buf + strlen(txt_buf), "%s kip dumped to %s.kip1\n", kip1->name, kip1->name);
		lv_label_set_array_text(lb_desc, txt_buf, 0x1000);
		manual_system_maintenance(true);

		ptr += kip1_size;
	}
	free(kip_buffer);

out:
	nx_emmc_gpt_free(&gpt);
out_free:
	free(pkg1);
	free(secmon);
	free(warmboot);
	free(loader);
	free(pkg2);
	free(txt_buf);
	free(txt_buf2);
	sdmmc_storage_end(&storage);
	sd_unmount(false);

	if (kb >= KB_FIRMWARE_VERSION_620)
		se_aes_key_clear(8);
out_end:
	// Enable buttons.
	nyx_window_toggle_buttons(win, false);

	return LV_RES_OK;
}

void sept_run_dump()
{
	_create_window_dump_pk12_tool(NULL);
}

static lv_obj_t *_create_container(lv_obj_t *parent)
{
	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 6;

	lv_obj_t *h1 = lv_cont_create(parent, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	return h1;
}

static void _create_tab_tools_emmc_pkg12(lv_theme_t *th, lv_obj_t *parent)
{
	lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY);

	// Create Backup & Restore container.
	lv_obj_t *h1 = _create_container(parent);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "Backup & Restore");
	lv_obj_set_style(label_txt, th->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 3 / 10);

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, th->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create Backup eMMC button.
	lv_obj_t *btn = lv_btn_create(h1, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	lv_obj_t *label_btn = lv_label_create(btn, NULL);
	lv_btn_set_fit(btn, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_UPLOAD"  Backup eMMC");
	lv_obj_align(btn, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, create_window_backup_restore_tool);

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to backup your eMMC partitions individually or as\n"
		"a whole raw image to your SD card.\n"
		"#FF8000 Supports SD cards from 4GB and up. FAT32 and exFAT.#");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Restore eMMC button.
	lv_obj_t *btn2 = lv_btn_create(h1, btn);
	label_btn = lv_label_create(btn2, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_DOWNLOAD"  Restore eMMC");
	lv_obj_align(btn2, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, create_window_backup_restore_tool);

	label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to restore your eMMC/emuMMC partitions individually\n"
		"or as a whole raw image from your SD card.\n"
		"#FF8000 Supports SD cards from 4GB and up. FAT32 and exFAT. #");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Misc container.
	lv_obj_t *h2 = _create_container(parent);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "Misc");
	lv_obj_set_style(label_txt3, th->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 3 / 10);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create Dump Package1/2 button.
	lv_obj_t *btn3 = lv_btn_create(h2, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn3, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn3, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_MODULES"  Dump Package1/2");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _create_window_dump_pk12_tool);

	lv_obj_t *label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"Allows you to dump and decrypt pkg1 and pkg2 and further\n"
		"split it up into their individual parts. It also dumps the kip1.\n\n");
	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");
	lv_obj_align(label_sep, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 11 / 7);
}

static void _create_tab_tools_arc_autorcm(lv_theme_t *th, lv_obj_t *parent)
{
	lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY);

	// Create Misc container.
	lv_obj_t *h1 = _create_container(parent);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "Misc");
	lv_obj_set_style(label_txt, th->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 3 / 10);

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, th->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create Unset archive bit button.
	lv_obj_t *btn = lv_btn_create(h1, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	lv_obj_t *label_btn = lv_label_create(btn, NULL);
	lv_btn_set_fit(btn, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_COPY"  Unset archive bit");
	lv_obj_align(btn, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, _create_window_unset_abit_tool);

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to unset the archive bit for all folders except the\n"
		"root and emuMMC \'Nintendo\' folders.\n"
		"#FF8000 If you want the Nintendo folders, use the below option.#");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Fix archive bit - Nintendo button.
	lv_obj_t *btn2 = lv_btn_create(h1, btn);
	label_btn = lv_label_create(btn2, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_DIRECTORY"  Fix archive bit - Nintendo");
	lv_obj_align(btn2, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, _create_window_unset_abit_tool);

	label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to fix your \'Nintendo\' folder's archive bits.\n"
		"This will also fix the \'Nintendo\' folders found in emuMMC.\n"
		"#FF8000 Use that option when you have corruption messages.#");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Others container.
	lv_obj_t *h2 = _create_container(parent);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "Others");
	lv_obj_set_style(label_txt3, th->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 3 / 10);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create AutoRCM On/Off button.
	lv_obj_t *btn3 = lv_btn_create(h2, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn3, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn3, LV_BTN_STYLE_PR, &btn_transp_pr);
		lv_btn_set_style(btn3, LV_BTN_STYLE_TGL_REL, &btn_transp_tgl_rel);
		lv_btn_set_style(btn3, LV_BTN_STYLE_TGL_PR, &btn_transp_tgl_pr);
	}
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	lv_label_set_recolor(label_btn, true);
	lv_label_set_static_text(label_btn, SYMBOL_REFRESH"  AutoRCM #00ffc9   ON #");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _create_mbox_autorcm_status);

	// Set default state for AutoRCM and lock it out if patched unit.
	if (get_autorcm_status(false))
		lv_btn_set_state(btn3, LV_BTN_STATE_TGL_REL);
	else
		lv_btn_set_state(btn3, LV_BTN_STATE_REL);
	nyx_generic_onoff_toggle(btn3);

	if (h_cfg.rcm_patched)
	{
		lv_obj_set_click(btn3, false);
		lv_btn_set_state(btn3, LV_BTN_STATE_INA);
	}
	autorcm_btn = btn3;

	char *txt_buf = (char *)malloc(0x1000);

	s_printf(txt_buf,
		"Allows you to enter RCM without using #C7EA46 VOL-# + #C7EA46 HOME# (jig).\n"
		"#FF8000 It can restore all versions of AutoRCM whenever requested.#\n"
		"#FF3C28 This corrupts your BCT and you can't boot without a custom#\n"
		"#FF3C28 bootloader.#");

	if (h_cfg.rcm_patched)
		s_printf(txt_buf + strlen(txt_buf), " #FF8000 This is disabled because this unit is patched!#");

	lv_obj_t *label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_array_text(label_txt4, txt_buf, 0x1000);
	free(txt_buf);

	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");
	lv_obj_align(label_sep, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 11 / 7);
}

void create_tab_tools(lv_theme_t *th, lv_obj_t *parent)
{
	lv_obj_t *tv = lv_tabview_create(parent, NULL);

	lv_obj_set_size(tv, LV_HOR_RES, 572);

	static lv_style_t tabview_style;
	lv_style_copy(&tabview_style, th->tabview.btn.rel);
	tabview_style.body.padding.ver = LV_DPI / 8;

	lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_REL, &tabview_style);
	if (hekate_bg)
	{
		lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_PR, &tabview_btn_pr);
		lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_TGL_PR, &tabview_btn_tgl_pr);
	}

	lv_tabview_set_sliding(tv, false);
	lv_tabview_set_btns_pos(tv, LV_TABVIEW_BTNS_POS_BOTTOM);

	lv_obj_t *tab1= lv_tabview_add_tab(tv, "eMMC "SYMBOL_DOT" Package1/2");
	lv_obj_t *tab2 = lv_tabview_add_tab(tv, "Archive bit "SYMBOL_DOT" AutoRCM");

	_create_tab_tools_emmc_pkg12(th, tab1);
	_create_tab_tools_arc_autorcm(th, tab2);

	lv_tabview_set_tab_act(tv, 0, false);
}

#pragma GCC pop_options
