/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2025 CTCaer
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

#include <bdk.h>

#include "gui.h"
#include "gui_tools.h"
#include "gui_tools_partition_manager.h"
#include "gui_emmc_tools.h"
#include "fe_emummc_tools.h"
#include "../config.h"
#include "../hos/pkg1.h"
#include "../hos/pkg2.h"
#include "../hos/hos.h"
#include <libs/compr/blz.h>
#include <libs/fatfs/ff.h>

extern volatile boot_cfg_t *b_cfg;

lv_obj_t *ums_mbox;

extern char *emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

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

bool get_set_autorcm_status(bool toggle)
{
	u32 sector;
	u8 corr_mod0, mod1;
	bool enabled = false;

	if (h_cfg.t210b01)
		return false;

	emmc_initialize(false);

	u8 *tempbuf = (u8 *)malloc(0x200);
	emmc_set_partition(EMMC_BOOT0);
	sdmmc_storage_read(&emmc_storage, 0x200 / EMMC_BLOCKSIZE, 1, tempbuf);

	// Get the correct RSA modulus byte masks.
	nx_emmc_get_autorcm_masks(&corr_mod0, &mod1);

	// Check if 2nd byte of modulus is correct.
	if (tempbuf[0x11] != mod1)
		goto out;

	if (tempbuf[0x10] != corr_mod0)
		enabled = true;

	// Toggle autorcm status if requested.
	if (toggle)
	{
		// Iterate BCTs.
		for (u32 i = 0; i < 4; i++)
		{
			sector = (0x200 + (0x4000 * i)) / EMMC_BLOCKSIZE; // 0x4000 bct + 0x200 offset.
			sdmmc_storage_read(&emmc_storage, sector, 1, tempbuf);

			if (!enabled)
				tempbuf[0x10] = 0;
			else
				tempbuf[0x10] = corr_mod0;
			sdmmc_storage_write(&emmc_storage, sector, 1, tempbuf);
		}
		enabled = !enabled;
	}

	// Check if RCM is patched and protect from a possible brick.
	if (enabled && h_cfg.rcm_patched && hw_get_chip_id() != GP_HIDREV_MAJOR_T210B01)
	{
		// Iterate BCTs.
		for (u32 i = 0; i < 4; i++)
		{
			sector = (0x200 + (0x4000 * i)) / EMMC_BLOCKSIZE; // 0x4000 bct + 0x200 offset.
			sdmmc_storage_read(&emmc_storage, sector, 1, tempbuf);

			// Check if 2nd byte of modulus is correct.
			if (tempbuf[0x11] != mod1)
				continue;

			// If AutoRCM is enabled, disable it.
			if (tempbuf[0x10] != corr_mod0)
			{
				tempbuf[0x10] = corr_mod0;

				sdmmc_storage_write(&emmc_storage, sector, 1, tempbuf);
			}
		}

		enabled = false;
	}

out:
	free(tempbuf);
	emmc_end();

	h_cfg.autorcm_enabled = enabled;

	return enabled;
}

static lv_res_t _create_mbox_autorcm_status(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	bool enabled = get_set_autorcm_status(true);

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

static lv_res_t _create_mbox_hid(usb_ctxt_t *usbs)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\251", "\262Close", "\251", "" };
	static const char *mbox_btn_map2[] = { "\251", "\222Close", "\251", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	char *txt_buf = malloc(SZ_4K);

	s_printf(txt_buf, "#FF8000 HID Emulation#\n\n#C7EA46 Device:# ");

	if (usbs->type == USB_HID_GAMEPAD)
		strcat(txt_buf, "Gamepad");
	else
		strcat(txt_buf, "Touchpad");

	lv_mbox_set_text(mbox, txt_buf);
	free(txt_buf);

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);
	lv_label_set_text(lbl_status, " ");
	usbs->label = (void *)lbl_status;

	lv_obj_t *lbl_tip = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_tip, true);
	lv_label_set_static_text(lbl_tip, "Note: To end it, press #C7EA46 L3# + #C7EA46 HOME# or remove the cable.");
	lv_obj_set_style(lbl_tip, &hint_small_style);

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	usb_device_gadget_hid(usbs);

	lv_mbox_add_btns(mbox, mbox_btn_map2, mbox_action);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_ums(usb_ctxt_t *usbs)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\251", "\262Close", "\251", "" };
	static const char *mbox_btn_map2[] = { "\251", "\222Close", "\251", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	char *txt_buf = malloc(SZ_4K);

	s_printf(txt_buf, "#FF8000 USB Mass Storage#\n\n#C7EA46 Device:# ");

	if (usbs->type == MMC_SD)
	{
		switch (usbs->partition)
		{
		case 0:
			strcat(txt_buf, "SD Card");
			break;
		case EMMC_GPP + 1:
			strcat(txt_buf, "emuMMC GPP");
			break;
		case EMMC_BOOT0 + 1:
			strcat(txt_buf, "emuMMC BOOT0");
			break;
		case EMMC_BOOT1 + 1:
			strcat(txt_buf, "emuMMC BOOT1");
			break;
		}
	}
	else
	{
		switch (usbs->partition)
		{
		case EMMC_GPP + 1:
			strcat(txt_buf, "eMMC GPP");
			break;
		case EMMC_BOOT0 + 1:
			strcat(txt_buf, "eMMC BOOT0");
			break;
		case EMMC_BOOT1 + 1:
			strcat(txt_buf, "eMMC BOOT1");
			break;
		}
	}

	lv_mbox_set_text(mbox, txt_buf);
	free(txt_buf);

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);
	lv_label_set_text(lbl_status, " ");
	usbs->label = (void *)lbl_status;

	lv_obj_t *lbl_tip = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_tip, true);
	if (!usbs->ro)
	{
		if (usbs->type == MMC_SD)
		{
			lv_label_set_static_text(lbl_tip,
				"Note: To end it, #C7EA46 safely eject# from inside the OS.\n"
				"       #FFDD00 DO NOT remove the cable!#");
		}
		else
		{
			lv_label_set_static_text(lbl_tip,
				"Note: To end it, #C7EA46 safely eject# from inside the OS.\n"
				"       #FFDD00 If it's not mounted, you might need to remove the cable!#");
		}
	}
	else
	{
		lv_label_set_static_text(lbl_tip,
			"Note: To end it, #C7EA46 safely eject# from inside the OS\n"
			"       or by removing the cable!#");
	}
	lv_obj_set_style(lbl_tip, &hint_small_style);

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	// Dim backlight.
	display_backlight_brightness(20, 1000);

	usb_device_gadget_ums(usbs);

	// Restore backlight.
	display_backlight_brightness(h_cfg.backlight - 20, 1000);

	lv_mbox_add_btns(mbox, mbox_btn_map2, mbox_action);

	ums_mbox = dark_bg;

	return LV_RES_OK;
}

static lv_res_t _create_mbox_ums_error(int error)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	switch (error)
	{
	case 1:
		lv_mbox_set_text(mbox, "#FF8000 USB Mass Storage#\n\n#FFFF00 Error mounting SD Card!#");
		break;
	case 2:
		lv_mbox_set_text(mbox, "#FF8000 USB Mass Storage#\n\n#FFFF00 No emuMMC found active!#");
		break;
	case 3:
		lv_mbox_set_text(mbox, "#FF8000 USB Mass Storage#\n\n#FFFF00 Active emuMMC is not partition based!#");
		break;
	}

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static void usb_gadget_set_text(void *lbl, const char *text)
{
	lv_label_set_text((lv_obj_t *)lbl, text);
	manual_system_maintenance(true);
}

static lv_res_t _action_hid_jc(lv_obj_t *btn)
{
	// Reduce BPMP, RAM and backlight and power off SDMMC1 to conserve power.
	sd_end();
	minerva_change_freq(FREQ_800);
	bpmp_clk_rate_relaxed(true);
	display_backlight_brightness(10, 1000);

	usb_ctxt_t usbs;
	usbs.type = USB_HID_GAMEPAD;
	usbs.system_maintenance = &manual_system_maintenance;
	usbs.set_text = &usb_gadget_set_text;

	_create_mbox_hid(&usbs);

	// Restore BPMP, RAM and backlight.
	minerva_change_freq(FREQ_1600);
	bpmp_clk_rate_relaxed(false);
	display_backlight_brightness(h_cfg.backlight - 20, 1000);

	return LV_RES_OK;
}

/*
static lv_res_t _action_hid_touch(lv_obj_t *btn)
{
	// Reduce BPMP, RAM and backlight and power off SDMMC1 to conserve power.
	sd_end();
	minerva_change_freq(FREQ_800);
	bpmp_clk_rate_relaxed(true);
	display_backlight_brightness(10, 1000);

	usb_ctxt_t usbs;
	usbs.type = USB_HID_TOUCHPAD;
	usbs.system_maintenance = &manual_system_maintenance;
	usbs.set_text = &usb_gadget_set_text;

	_create_mbox_hid(&usbs);

	// Restore BPMP, RAM and backlight.
	minerva_change_freq(FREQ_1600);
	bpmp_clk_rate_relaxed(false);
	display_backlight_brightness(h_cfg.backlight - 20, 1000);

	return LV_RES_OK;
}
*/

static bool usb_msc_emmc_read_only;
lv_res_t action_ums_sd(lv_obj_t *btn)
{
	usb_ctxt_t usbs;
	usbs.type = MMC_SD;
	usbs.partition = 0;
	usbs.offset = 0;
	usbs.sectors = 0;
	usbs.ro = 0;
	usbs.system_maintenance = &manual_system_maintenance;
	usbs.set_text = &usb_gadget_set_text;

	_create_mbox_ums(&usbs);

	return LV_RES_OK;
}

static lv_res_t _action_ums_emmc_boot0(lv_obj_t *btn)
{
	if (!nyx_emmc_check_battery_enough())
		return LV_RES_OK;

	usb_ctxt_t usbs;
	usbs.type = MMC_EMMC;
	usbs.partition = EMMC_BOOT0 + 1;
	usbs.offset = 0;
	usbs.sectors = 0;
	usbs.ro = usb_msc_emmc_read_only;
	usbs.system_maintenance = &manual_system_maintenance;
	usbs.set_text = &usb_gadget_set_text;

	_create_mbox_ums(&usbs);

	return LV_RES_OK;
}

static lv_res_t _action_ums_emmc_boot1(lv_obj_t *btn)
{
	if (!nyx_emmc_check_battery_enough())
		return LV_RES_OK;

	usb_ctxt_t usbs;
	usbs.type = MMC_EMMC;
	usbs.partition = EMMC_BOOT1 + 1;
	usbs.offset = 0;
	usbs.sectors = 0;
	usbs.ro = usb_msc_emmc_read_only;
	usbs.system_maintenance = &manual_system_maintenance;
	usbs.set_text = &usb_gadget_set_text;

	_create_mbox_ums(&usbs);

	return LV_RES_OK;
}

static lv_res_t _action_ums_emmc_gpp(lv_obj_t *btn)
{
	if (!nyx_emmc_check_battery_enough())
		return LV_RES_OK;

	usb_ctxt_t usbs;
	usbs.type = MMC_EMMC;
	usbs.partition = EMMC_GPP + 1;
	usbs.offset = 0;
	usbs.sectors = 0;
	usbs.ro = usb_msc_emmc_read_only;
	usbs.system_maintenance = &manual_system_maintenance;
	usbs.set_text = &usb_gadget_set_text;

	_create_mbox_ums(&usbs);

	return LV_RES_OK;
}

static lv_res_t _action_ums_emuemmc_boot0(lv_obj_t *btn)
{
	if (!nyx_emmc_check_battery_enough())
		return LV_RES_OK;

	usb_ctxt_t usbs;

	int error = !sd_mount();
	if (!error)
	{
		emummc_cfg_t emu_info;
		load_emummc_cfg(&emu_info);

		error = 2;
		if (emu_info.enabled)
		{
			error = 3;
			if (emu_info.sector)
			{
				error = 0;
				usbs.offset = emu_info.sector;
			}
		}

		if (emu_info.path)
			free(emu_info.path);
		if (emu_info.nintendo_path)
			free(emu_info.nintendo_path);
	}
	sd_unmount();

	if (error)
		_create_mbox_ums_error(error);
	else
	{
		usbs.type = MMC_SD;
		usbs.partition = EMMC_BOOT0 + 1;
		usbs.sectors = 0x2000; // Forced 4MB.
		usbs.ro = usb_msc_emmc_read_only;
		usbs.system_maintenance = &manual_system_maintenance;
		usbs.set_text = &usb_gadget_set_text;
		_create_mbox_ums(&usbs);
	}

	return LV_RES_OK;
}

static lv_res_t _action_ums_emuemmc_boot1(lv_obj_t *btn)
{
	if (!nyx_emmc_check_battery_enough())
		return LV_RES_OK;

	usb_ctxt_t usbs;

	int error = !sd_mount();
	if (!error)
	{
		emummc_cfg_t emu_info;
		load_emummc_cfg(&emu_info);

		error = 2;
		if (emu_info.enabled)
		{
			error = 3;
			if (emu_info.sector)
			{
				error = 0;
				usbs.offset = emu_info.sector + 0x2000;
			}
		}

		if (emu_info.path)
			free(emu_info.path);
		if (emu_info.nintendo_path)
			free(emu_info.nintendo_path);
	}
	sd_unmount();

	if (error)
		_create_mbox_ums_error(error);
	else
	{
		usbs.type = MMC_SD;
		usbs.partition = EMMC_BOOT1 + 1;
		usbs.sectors = 0x2000; // Forced 4MB.
		usbs.ro = usb_msc_emmc_read_only;
		usbs.system_maintenance = &manual_system_maintenance;
		usbs.set_text = &usb_gadget_set_text;
		_create_mbox_ums(&usbs);
	}

	return LV_RES_OK;
}

static lv_res_t _action_ums_emuemmc_gpp(lv_obj_t *btn)
{
	if (!nyx_emmc_check_battery_enough())
		return LV_RES_OK;

	usb_ctxt_t usbs;

	int error = !sd_mount();
	if (!error)
	{
		emummc_cfg_t emu_info;
		load_emummc_cfg(&emu_info);

		error = 2;
		if (emu_info.enabled)
		{
			error = 3;
			if (emu_info.sector)
			{
				error = 1;
				usbs.offset = emu_info.sector + 0x4000;

				u8 *gpt = malloc(SD_BLOCKSIZE);
				if (sdmmc_storage_read(&sd_storage, usbs.offset + 1, 1, gpt))
				{
					if (!memcmp(gpt, "EFI PART", 8))
					{
						error = 0;
						usbs.sectors = *(u32 *)(gpt + 0x20) + 1; // Backup LBA + 1.
					}
				}
			}
		}

		if (emu_info.path)
			free(emu_info.path);
		if (emu_info.nintendo_path)
			free(emu_info.nintendo_path);
	}
	sd_unmount();

	if (error)
		_create_mbox_ums_error(error);
	else
	{
		usbs.type = MMC_SD;
		usbs.partition = EMMC_GPP + 1;
		usbs.ro = usb_msc_emmc_read_only;
		usbs.system_maintenance = &manual_system_maintenance;
		usbs.set_text = &usb_gadget_set_text;
		_create_mbox_ums(&usbs);
	}

	return LV_RES_OK;
}

void nyx_run_ums(void *param)
{
	u32 *cfg = (u32 *)param;

	u8 type = (*cfg) >> 24;
	*cfg = *cfg & (~NYX_CFG_EXTRA);

	// Disable read only flag.
	usb_msc_emmc_read_only = false;

	switch (type)
	{
	case NYX_UMS_SD_CARD:
		action_ums_sd(NULL);
		break;
	case NYX_UMS_EMMC_BOOT0:
		_action_ums_emmc_boot0(NULL);
		break;
	case NYX_UMS_EMMC_BOOT1:
		_action_ums_emmc_boot1(NULL);
		break;
	case NYX_UMS_EMMC_GPP:
		_action_ums_emmc_gpp(NULL);
		break;
	case NYX_UMS_EMUMMC_BOOT0:
		_action_ums_emuemmc_boot0(NULL);
		break;
	case NYX_UMS_EMUMMC_BOOT1:
		_action_ums_emuemmc_boot1(NULL);
		break;
	case NYX_UMS_EMUMMC_GPP:
		_action_ums_emuemmc_gpp(NULL);
		break;
	}
}

static lv_res_t _emmc_read_only_toggle(lv_obj_t *btn)
{
	nyx_generic_onoff_toggle(btn);

	usb_msc_emmc_read_only = lv_btn_get_state(btn) & LV_BTN_STATE_TGL_REL ? 1 : 0;

	return LV_RES_OK;
}

static lv_res_t _create_window_usb_tools(lv_obj_t *parent)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_USB" USB Tools");

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 9;

	// Create USB Mass Storage container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 5);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "USB Mass Storage");
	lv_obj_set_style(label_txt, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 3 / 10);

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create SD UMS button.
	lv_obj_t *btn1 = lv_btn_create(h1, NULL);
	lv_obj_t *label_btn = lv_label_create(btn1, NULL);
	lv_btn_set_fit(btn1, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_SD"  SD Card");

	lv_obj_align(btn1, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, action_ums_sd);

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to mount the SD Card to a PC/Phone.\n"
		"#C7EA46 All operating systems are supported. Access is# #FF8000 Read/Write.#");

	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create RAW GPP button.
	lv_obj_t *btn_gpp = lv_btn_create(h1, btn1);
	label_btn = lv_label_create(btn_gpp, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_CHIP"  eMMC RAW GPP");
	lv_obj_align(btn_gpp, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn_gpp, LV_BTN_ACTION_CLICK, _action_ums_emmc_gpp);

	// Create BOOT0 button.
	lv_obj_t *btn_boot0 = lv_btn_create(h1, btn1);
	label_btn = lv_label_create(btn_boot0, NULL);
	lv_label_set_static_text(label_btn, "BOOT0");
	lv_obj_align(btn_boot0, btn_gpp, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 10, 0);
	lv_btn_set_action(btn_boot0, LV_BTN_ACTION_CLICK, _action_ums_emmc_boot0);

	// Create BOOT1 button.
	lv_obj_t *btn_boot1 = lv_btn_create(h1, btn1);
	label_btn = lv_label_create(btn_boot1, NULL);
	lv_label_set_static_text(label_btn, "BOOT1");
	lv_obj_align(btn_boot1, btn_boot0, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 10, 0);
	lv_btn_set_action(btn_boot1, LV_BTN_ACTION_CLICK, _action_ums_emmc_boot1);

	// Create emuMMC RAW GPP button.
	lv_obj_t *btn_emu_gpp = lv_btn_create(h1, btn1);
	label_btn = lv_label_create(btn_emu_gpp, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_MODULES_ALT"  emu RAW GPP");
	lv_obj_align(btn_emu_gpp, btn_gpp, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn_emu_gpp, LV_BTN_ACTION_CLICK, _action_ums_emuemmc_gpp);

	// Create emuMMC BOOT0 button.
	lv_obj_t *btn_emu_boot0 = lv_btn_create(h1, btn1);
	label_btn = lv_label_create(btn_emu_boot0, NULL);
	lv_label_set_static_text(label_btn, "BOOT0");
	lv_obj_align(btn_emu_boot0, btn_boot0, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn_emu_boot0, LV_BTN_ACTION_CLICK, _action_ums_emuemmc_boot0);

	// Create emuMMC BOOT1 button.
	lv_obj_t *btn_emu_boot1 = lv_btn_create(h1, btn1);
	label_btn = lv_label_create(btn_emu_boot1, NULL);
	lv_label_set_static_text(label_btn, "BOOT1");
	lv_obj_align(btn_emu_boot1, btn_boot1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn_emu_boot1, LV_BTN_ACTION_CLICK, _action_ums_emuemmc_boot1);

	label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to mount the eMMC/emuMMC.\n"
		"#C7EA46 Default access is# #FF8000 read-only.#");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn_emu_gpp, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	lv_obj_t *h_write = lv_cont_create(win, NULL);
	lv_cont_set_style(h_write, &h_style);
	lv_cont_set_fit(h_write, false, true);
	lv_obj_set_width(h_write, (LV_HOR_RES / 9) * 2);
	lv_obj_set_click(h_write, false);
	lv_cont_set_layout(h_write, LV_LAYOUT_OFF);
	lv_obj_align(h_write, label_txt2, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 10, 0);

	// Create read/write access button.
	lv_obj_t *btn_write_access = lv_btn_create(h_write, NULL);
	nyx_create_onoff_button(lv_theme_get_current(), h_write,
		btn_write_access, SYMBOL_EDIT" Read-Only", _emmc_read_only_toggle, false);
	if (!n_cfg.ums_emmc_rw)
		lv_btn_set_state(btn_write_access, LV_BTN_STATE_TGL_REL);
	_emmc_read_only_toggle(btn_write_access);

	// Create USB Input Devices container.
	lv_obj_t *h2 = lv_cont_create(win, NULL);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, true);
	lv_obj_set_width(h2, (LV_HOR_RES / 9) * 3);
	lv_obj_set_click(h2, false);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 17 / 29, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "USB Input Devices");
	lv_obj_set_style(label_txt3, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 4 / 21);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create Gamepad button.
	lv_obj_t *btn3 = lv_btn_create(h2, NULL);
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_CIRCUIT"  Gamepad");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _action_hid_jc);

	lv_obj_t *label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"Plug-in the Joy-Con and convert the device\n"
		"into a gamepad for PC or Phone.\n"
		"#C7EA46 Needs both Joy-Con in order to function.#");

	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
/*
	// Create Touchpad button.
	lv_obj_t *btn4 = lv_btn_create(h2, btn1);
	label_btn = lv_label_create(btn4, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_KEYBOARD"  Touchpad");
	lv_obj_align(btn4, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _action_hid_touch);
	lv_btn_set_state(btn4, LV_BTN_STATE_INA);

	label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"Control the PC via the device\'s touchscreen.\n"
		"#C7EA46 Two fingers tap acts like a# #FF8000 Right click##C7EA46 .#\n");
	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
*/
	return LV_RES_OK;
}

static int _fix_attributes(lv_obj_t *lb_val, char *path, u32 *total)
{
	FRESULT res;
	DIR dir;
	u32 dirLength = 0;
	static FILINFO fno;

	// Open directory.
	res = f_opendir(&dir, path);
	if (res != FR_OK)
		return res;

	dirLength = strlen(path);

	// Hard limit path to 1024 characters. Do not result to error.
	if (dirLength > 1024)
	{
		total[2]++;
		goto out;
	}

	for (;;)
	{
		// Clear file or folder path.
		path[dirLength] = 0;

		// Read a directory item.
		res = f_readdir(&dir, &fno);

		// Break on error or end of dir.
		if (res != FR_OK || fno.fname[0] == 0)
			break;

		// Set new directory or file.
		memcpy(&path[dirLength], "/", 1);
		strcpy(&path[dirLength + 1], fno.fname);

		// Is it a directory?
		if (fno.fattrib & AM_DIR)
		{
			// Check if it's a HOS single file folder.
			strcat(path, "/00");
			bool is_hos_special = !f_stat(path, NULL);
			path[strlen(path) - 3] = 0;

			// Set archive bit to HOS single file folders.
			if (is_hos_special)
			{
				if (!(fno.fattrib & AM_ARC))
				{
					if (!f_chmod(path, AM_ARC, AM_ARC))
						total[0]++;
					else
						total[3]++;
				}
			}
			else if (fno.fattrib & AM_ARC) // If not, clear the archive bit.
			{
				if (!f_chmod(path, 0, AM_ARC))
					total[1]++;
				else
					total[3]++;
			}

			lv_label_set_text(lb_val, path);
			manual_system_maintenance(true);

			// Enter the directory.
			res = _fix_attributes(lb_val, path, total);
			if (res != FR_OK)
				break;
		}
	}

out:
	f_closedir(&dir);

	return res;
}

static lv_res_t _create_window_unset_abit_tool(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_COPY" Fix Archive Bit (All folders)");

	// Disable buttons.
	nyx_window_toggle_buttons(win, true);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES * 10 / 11, LV_VER_RES - (LV_DPI * 11 / 7) * 4);

	lv_obj_t * lb_desc = lv_label_create(desc, NULL);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);

	if (!sd_mount())
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init SD!#");
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));
	}
	else
	{
		lv_label_set_text(lb_desc, "#00DDFF Traversing all SD card files!#\nThis may take some time...");
		lv_obj_set_width(lb_desc, lv_obj_get_width(desc));

		lv_obj_t *val = lv_cont_create(win, NULL);
		lv_obj_set_size(val, LV_HOR_RES * 10 / 11, LV_VER_RES - (LV_DPI * 11 / 7) * 4);

		lv_obj_t * lb_val = lv_label_create(val, lb_desc);

		char *path = malloc(0x1000);
		path[0] = 0;

		lv_label_set_text(lb_val, "");
		lv_obj_set_width(lb_val, lv_obj_get_width(val));
		lv_obj_align(val, desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

		u32 total[4] = { 0 };
		_fix_attributes(lb_val, path, total);

		sd_unmount();

		lv_obj_t *desc2 = lv_cont_create(win, NULL);
		lv_obj_set_size(desc2, LV_HOR_RES * 10 / 11, LV_VER_RES - (LV_DPI * 11 / 7) * 4);
		lv_obj_t * lb_desc2 = lv_label_create(desc2, lb_desc);

		char *txt_buf = (char *)malloc(0x500);

		if (!total[0] && !total[1])
			s_printf(txt_buf, "#96FF00 Done! No change was needed.#");
		else
			s_printf(txt_buf, "#96FF00 Done! Archive bits fixed:# #FF8000 %d unset and %d set!#", total[1], total[0]);

		// Check errors.
		if (total[2] || total[3])
		{
			s_printf(txt_buf, "\n\n#FFDD00 Errors: folder accesses: %d, arc bit fixes: %d!#\n"
					          "#FFDD00 Filesystem should be checked for errors.#",
					          total[2], total[3]);
		}

		lv_label_set_text(lb_desc2, txt_buf);
		lv_obj_set_width(lb_desc2, lv_obj_get_width(desc2));
		lv_obj_align(desc2, val, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);

		free(path);
	}

	// Enable buttons.
	nyx_window_toggle_buttons(win, false);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_fix_touchscreen(lv_obj_t *btn)
{
	int res = 0;
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	char *txt_buf = malloc(SZ_16K);
	strcpy(txt_buf, "#FF8000 Don't touch the screen!#\n\nThe tuning process will start in ");
	u32 text_idx = strlen(txt_buf);
	lv_mbox_set_text(mbox, txt_buf);

	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	lv_mbox_set_text(mbox,
		"#FFDD00 Warning: Only run this if you really have issues!#\n\n"
		"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.");
	manual_system_maintenance(true);

	if (!(btn_wait() & BTN_POWER))
		goto out;

	manual_system_maintenance(true);
	lv_mbox_set_text(mbox, txt_buf);

	u32 seconds = 5;
	while (seconds)
	{
		s_printf(txt_buf + text_idx, "%d seconds...", seconds);
		lv_mbox_set_text(mbox, txt_buf);
		manual_system_maintenance(true);
		msleep(1000);
		seconds--;
	}

	u8 err[2];
	if (!touch_panel_ito_test(err))
		goto ito_failed;

	if (!err[0] && !err[1])
	{
		res = touch_execute_autotune();
		if (res)
			goto out;
	}
	else
	{
		touch_sense_enable();

		s_printf(txt_buf, "#FFFF00 ITO Test: ");
		switch (err[0])
		{
		case ITO_FORCE_OPEN:
			strcat(txt_buf, "Force Open");
			break;
		case ITO_SENSE_OPEN:
			strcat(txt_buf, "Sense Open");
			break;
		case ITO_FORCE_SHRT_GND:
			strcat(txt_buf, "Force Short to GND");
			break;
		case ITO_SENSE_SHRT_GND:
			strcat(txt_buf, "Sense Short to GND");
			break;
		case ITO_FORCE_SHRT_VCM:
			strcat(txt_buf, "Force Short to VDD");
			break;
		case ITO_SENSE_SHRT_VCM:
			strcat(txt_buf, "Sense Short to VDD");
			break;
		case ITO_FORCE_SHRT_FORCE:
			strcat(txt_buf, "Force Short to Force");
			break;
		case ITO_SENSE_SHRT_SENSE:
			strcat(txt_buf, "Sense Short to Sense");
			break;
		case ITO_F2E_SENSE:
			strcat(txt_buf, "Force Short to Sense");
			break;
		case ITO_FPC_FORCE_OPEN:
			strcat(txt_buf, "FPC Force Open");
			break;
		case ITO_FPC_SENSE_OPEN:
			strcat(txt_buf, "FPC Sense Open");
			break;
		default:
			strcat(txt_buf, "Unknown");
			break;

		}
		s_printf(txt_buf + strlen(txt_buf), " (%d), Chn: %d#\n\n", err[0], err[1]);
		strcat(txt_buf, "#FFFF00 The touchscreen calibration failed!");
		lv_mbox_set_text(mbox, txt_buf);
		goto out2;
	}

ito_failed:
	touch_sense_enable();

out:
	if (res)
		lv_mbox_set_text(mbox, "#C7EA46 The touchscreen calibration finished!");
	else
		lv_mbox_set_text(mbox, "#FFFF00 The touchscreen calibration failed!");

out2:
	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);

	free(txt_buf);

	return LV_RES_OK;
}

static lv_res_t _create_window_dump_pk12_tool(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_MODULES" Dump package1/2");

	// Disable buttons.
	nyx_window_toggle_buttons(win, true);

	lv_obj_t *desc = lv_cont_create(win, NULL);
	lv_obj_set_size(desc, LV_HOR_RES * 10 / 11, LV_VER_RES - (LV_DPI * 12 / 7));

	lv_obj_t *lb_desc = lv_label_create(desc, NULL);
	lv_obj_set_style(lb_desc, &monospace_text);
	lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc, true);
	lv_obj_set_width(lb_desc, lv_obj_get_width(desc) / 2);

	lv_obj_t *lb_desc2 = lv_label_create(desc, NULL);
	lv_obj_set_style(lb_desc2, &monospace_text);
	lv_label_set_long_mode(lb_desc2, LV_LABEL_LONG_BREAK);
	lv_label_set_recolor(lb_desc2, true);
	lv_obj_set_width(lb_desc2, lv_obj_get_width(desc) / 2);
	lv_label_set_text(lb_desc2, " ");

	lv_obj_align(lb_desc2, lb_desc, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);

	if (!sd_mount())
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init SD!#");

		goto out_end;
	}

	char path[128];

	u8 mkey = 0;
	u8 *pkg1 = (u8 *)zalloc(SZ_256K);
	u8 *warmboot = (u8 *)zalloc(SZ_256K);
	u8 *secmon = (u8 *)zalloc(SZ_256K);
	u8 *loader = (u8 *)zalloc(SZ_256K);
	u8 *pkg2   = (u8 *)zalloc(SZ_8M);

	char *txt_buf  = (char *)malloc(SZ_16K);

	if (!emmc_initialize(false))
	{
		lv_label_set_text(lb_desc, "#FFDD00 Failed to init eMMC!#");

		goto out_free;
	}

	char *bct_paths[2] = {
		"/pkg/main",
		"/pkg/safe"
	};

	char *pkg1_paths[2] = {
		"/pkg/main/pkg1",
		"/pkg/safe/pkg1"
	};

	char *pkg2_partitions[2] = {
		"BCPKG2-1-Normal-Main",
		"BCPKG2-3-SafeMode-Main"
	};

	char *pkg2_paths[2] = {
		"/pkg/main/pkg2",
		"/pkg/safe/pkg2"
	};

	char *pkg2ini_paths[2] = {
		"/pkg/main/pkg2/ini",
		"/pkg/safe/pkg2/ini"
	};

	// Create main directories.
	emmcsn_path_impl(path, "/pkg", "", &emmc_storage);
	emmcsn_path_impl(path, "/pkg/main", "", &emmc_storage);
	emmcsn_path_impl(path, "/pkg/safe", "", &emmc_storage);

	// Parse eMMC GPT.
	emmc_set_partition(EMMC_GPP);
	LIST_INIT(gpt);
	emmc_gpt_parse(&gpt);

	lv_obj_t *lb_log = lb_desc;
	for (u32 idx = 0; idx < 2; idx++)
	{
		if (idx)
			lb_log = lb_desc2;

		// Read package1.
		char *build_date = malloc(32);
		u32   pk1_offset = h_cfg.t210b01 ? sizeof(bl_hdr_t210b01_t) : 0; // Skip T210B01 OEM header.
		emmc_set_partition(!idx ? EMMC_BOOT0 : EMMC_BOOT1);
		sdmmc_storage_read(&emmc_storage,
			!idx ? PKG1_BOOTLOADER_MAIN_OFFSET : PKG1_BOOTLOADER_SAFE_OFFSET, PKG1_BOOTLOADER_SIZE / EMMC_BLOCKSIZE, pkg1);

		const pkg1_id_t *pkg1_id = pkg1_identify(pkg1 + pk1_offset, build_date);

		s_printf(txt_buf, "#00DDFF Found %s pkg1 ('%s')#\n\n", !idx ? "Main" : "Safe", build_date);
		lv_label_set_text(lb_log, txt_buf);
		manual_system_maintenance(true);
		free(build_date);

		// Dump package1 in its encrypted state.
		emmcsn_path_impl(path, pkg1_paths[idx], "pkg1_enc.bin", &emmc_storage);
		bool res = sd_save_to_file(pkg1, PKG1_BOOTLOADER_SIZE, path);

		// Exit if unknown.
		if (!pkg1_id)
		{
			strcat(txt_buf, "#FFDD00 Unknown pkg1 version!#");
			lv_label_set_text(lb_log, txt_buf);
			manual_system_maintenance(true);

			if (!res)
			{
				strcat(txt_buf, "\nEncrypted pkg1 extracted to pkg1_enc.bin");
				lv_label_set_text(lb_log, txt_buf);
				manual_system_maintenance(true);
			}

			goto out;
		}

		mkey = pkg1_id->mkey;

		tsec_ctxt_t tsec_ctxt = {0};
		tsec_ctxt.fw = (void *)(pkg1 + pkg1_id->tsec_off);
		tsec_ctxt.pkg1 = (void *)pkg1;
		tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;

		// Read the correct eks for older HOS versions.
		const u32 eks_size = sizeof(pkg1_eks_t);
		pkg1_eks_t *eks = (pkg1_eks_t *)malloc(eks_size);
		emmc_set_partition(EMMC_BOOT0);
		sdmmc_storage_read(&emmc_storage, PKG1_HOS_EKS_OFFSET + (mkey * eks_size) / EMMC_BLOCKSIZE,
										  eks_size / EMMC_BLOCKSIZE, eks);

		// Generate keys.
		hos_keygen(eks, mkey, &tsec_ctxt);
		free(eks);

		// Decrypt.
		if (h_cfg.t210b01 || mkey <= HOS_MKEY_VER_600)
		{
			if (!pkg1_decrypt(pkg1_id, pkg1))
			{
				strcat(txt_buf, "#FFDD00 Pkg1 decryption failed!#\n");
				if (h_cfg.t210b01)
					strcat(txt_buf, "#FFDD00 Is BEK missing?#\n");
				lv_label_set_text(lb_log, txt_buf);
				goto out;
			}
		}

		// Dump the BCTs from blocks 2/3 (backup) which are normally valid.
		static const u32 BCT_SIZE = 0x2800;
		static const u32 BLK_SIZE = SZ_16K / EMMC_BLOCKSIZE;
		u8 *bct = (u8 *)zalloc(BCT_SIZE);
		sdmmc_storage_read(&emmc_storage, BLK_SIZE * 2 + BLK_SIZE * idx, BCT_SIZE / EMMC_BLOCKSIZE, bct);
		emmcsn_path_impl(path, bct_paths[idx], "bct.bin", &emmc_storage);
		if (sd_save_to_file(bct, 0x2800, path))
			goto out;
		if (h_cfg.t210b01)
		{
			se_aes_iv_clear(13);
			se_aes_crypt_cbc(13, DECRYPT, bct + 0x480, BCT_SIZE - 0x480, bct + 0x480, BCT_SIZE - 0x480);
			emmcsn_path_impl(path, bct_paths[idx], "bct_decr.bin", &emmc_storage);
			if (sd_save_to_file(bct, 0x2800, path))
				goto out;
		}

		// Dump package1.1 contents.
		if (h_cfg.t210b01 || mkey <= HOS_MKEY_VER_620)
		{
			pkg1_unpack(warmboot, secmon, loader, pkg1_id, pkg1 + pk1_offset);
			pk11_hdr_t *hdr_pk11 = (pk11_hdr_t *)(pkg1 + pk1_offset + pkg1_id->pkg11_off + 0x20);

			// Display info.
			s_printf(txt_buf + strlen(txt_buf),
				"#C7EA46 NX Bootloader size:  #0x%05X\n"
				"#C7EA46 Secure monitor size: #0x%05X\n"
				"#C7EA46 Warmboot size:       #0x%05X\n\n",
				hdr_pk11->ldr_size, hdr_pk11->sm_size, hdr_pk11->wb_size);

			lv_label_set_text(lb_log, txt_buf);
			manual_system_maintenance(true);

			// Dump package1.1.
			emmcsn_path_impl(path, pkg1_paths[idx], "pkg1_decr.bin", &emmc_storage);
			if (sd_save_to_file(pkg1, SZ_256K, path))
				goto out;
			strcat(txt_buf, "Package1 extracted to pkg1_decr.bin\n");
			lv_label_set_text(lb_log, txt_buf);
			manual_system_maintenance(true);

			// Dump nxbootloader.
			emmcsn_path_impl(path, pkg1_paths[idx], "nxloader.bin", &emmc_storage);
			if (sd_save_to_file(loader, hdr_pk11->ldr_size, path))
				goto out;
			strcat(txt_buf, "NX Bootloader extracted to nxloader.bin\n");
			lv_label_set_text(lb_log, txt_buf);
			manual_system_maintenance(true);

			// Dump secmon.
			emmcsn_path_impl(path, pkg1_paths[idx], "secmon.bin", &emmc_storage);
			if (sd_save_to_file(secmon, hdr_pk11->sm_size, path))
				goto out;
			strcat(txt_buf, "Secure Monitor extracted to secmon.bin\n");
			lv_label_set_text(lb_log, txt_buf);
			manual_system_maintenance(true);

			// Dump warmboot.
			emmcsn_path_impl(path, pkg1_paths[idx], "warmboot.bin", &emmc_storage);
			if (sd_save_to_file(warmboot, hdr_pk11->wb_size, path))
				goto out;
			// If T210B01, save a copy of decrypted warmboot binary also.
			if (h_cfg.t210b01)
			{

				se_aes_iv_clear(13);
				se_aes_crypt_cbc(13, DECRYPT, warmboot + 0x330, hdr_pk11->wb_size - 0x330,
					warmboot + 0x330, hdr_pk11->wb_size - 0x330);
				emmcsn_path_impl(path, pkg1_paths[idx], "warmboot_dec.bin", &emmc_storage);
				if (sd_save_to_file(warmboot, hdr_pk11->wb_size, path))
					goto out;
			}
			strcat(txt_buf, "Warmboot extracted to warmboot.bin\n\n");
			lv_label_set_text(lb_log, txt_buf);
			manual_system_maintenance(true);
		}

		// Find and dump package2 partition.
		emmc_set_partition(EMMC_GPP);
		emmc_part_t *pkg2_part = emmc_part_find(&gpt, pkg2_partitions[idx]);
		if (!pkg2_part)
			goto out;

		// Read in package2 header and get package2 real size.
		static const u32 PKG2_OFFSET = 0x4000;
		u8 *tmp = (u8 *)malloc(EMMC_BLOCKSIZE);
		emmc_part_read(pkg2_part, PKG2_OFFSET / EMMC_BLOCKSIZE, 1, tmp);
		u32 *hdr_pkg2_raw = (u32 *)(tmp + 0x100);
		u32 pkg2_size = hdr_pkg2_raw[0] ^ hdr_pkg2_raw[2] ^ hdr_pkg2_raw[3];
		free(tmp);

		// Read in package2.
		u32 pkg2_size_aligned = ALIGN(pkg2_size, EMMC_BLOCKSIZE);
		emmc_part_read(pkg2_part, PKG2_OFFSET / EMMC_BLOCKSIZE, pkg2_size_aligned / EMMC_BLOCKSIZE, pkg2);

		// Dump encrypted package2.
		emmcsn_path_impl(path, pkg2_paths[idx], "pkg2_encr.bin", &emmc_storage);
		res = sd_save_to_file(pkg2, pkg2_size, path);

		// Decrypt package2 and parse KIP1 blobs in INI1 section.
		pkg2_hdr_t *pkg2_hdr = pkg2_decrypt(pkg2, mkey);
		if (!pkg2_hdr)
		{
			strcat(txt_buf, "#FFDD00 Pkg2 decryption failed!#");
			lv_label_set_text(lb_log, txt_buf);
			manual_system_maintenance(true);

			if (!res)
			{
				strcat(txt_buf, "\npkg2 encrypted extracted to pkg2_encr.bin\n");
				lv_label_set_text(lb_log, txt_buf);
				manual_system_maintenance(true);
			}

			// Clear EKS slot, in case something went wrong with tsec keygen.
			hos_eks_clear(mkey);

			goto out;
		}

		// Display info.
		s_printf(txt_buf + strlen(txt_buf),
			"#C7EA46 Kernel size:   #0x%06X\n"
			"#C7EA46 INI1 size:     #0x%06X\n\n",
			pkg2_hdr->sec_size[PKG2_SEC_KERNEL], pkg2_hdr->sec_size[PKG2_SEC_INI1]);

		lv_label_set_text(lb_log, txt_buf);
		manual_system_maintenance(true);

		// Dump pkg2.1.
		emmcsn_path_impl(path, pkg2_paths[idx], "pkg2_decr.bin", &emmc_storage);
		if (sd_save_to_file(pkg2, pkg2_hdr->sec_size[PKG2_SEC_KERNEL] + pkg2_hdr->sec_size[PKG2_SEC_INI1], path))
			goto out;
		strcat(txt_buf, "Package2 extracted to pkg2_decr.bin\n");
		lv_label_set_text(lb_log, txt_buf);
		manual_system_maintenance(true);

		// Dump kernel.
		emmcsn_path_impl(path, pkg2_paths[idx], "kernel.bin", &emmc_storage);
		if (sd_save_to_file(pkg2_hdr->data, pkg2_hdr->sec_size[PKG2_SEC_KERNEL], path))
			goto out;
		strcat(txt_buf, "Kernel extracted to kernel.bin\n");
		lv_label_set_text(lb_log, txt_buf);
		manual_system_maintenance(true);

		// Dump INI1.
		u32 ini1_off  = pkg2_hdr->sec_size[PKG2_SEC_KERNEL];
		u32 ini1_size = pkg2_hdr->sec_size[PKG2_SEC_INI1];
		if (!ini1_size)
		{
			pkg2_get_newkern_info(pkg2_hdr->data);
			ini1_off  = pkg2_newkern_ini1_start;
			ini1_size = pkg2_newkern_ini1_end - pkg2_newkern_ini1_start;
		}

		if (!ini1_off)
		{
			strcat(txt_buf, "#FFDD00 Failed to dump INI1 and kips!#\n");
			goto out;
		}

		pkg2_ini1_t *ini1 = (pkg2_ini1_t *)(pkg2_hdr->data + ini1_off);
		emmcsn_path_impl(path, pkg2_paths[idx], "ini1.bin", &emmc_storage);
		if (sd_save_to_file(ini1, ini1_size, path))
			goto out;

		strcat(txt_buf, "INI1 extracted to ini1.bin\n");
		lv_label_set_text(lb_log, txt_buf);
		manual_system_maintenance(true);

		char filename[32];
		u8 *ptr = (u8 *)ini1;
		ptr += sizeof(pkg2_ini1_t);

		// Dump all kips.
		for (u32 i = 0; i < ini1->num_procs; i++)
		{
			pkg2_kip1_t *kip1 = (pkg2_kip1_t *)ptr;
			u32 kip1_size = pkg2_calc_kip1_size(kip1);
			char *kip_name = kip1->name;

			// Check if FS supports exFAT.
			if (!strcmp("FS", kip_name))
			{
				u8 *ro_data = malloc(SZ_4M);
				u32 offset      = (kip1->flags & BIT(KIP_TEXT)) ? kip1->sections[KIP_TEXT].size_comp :
																  kip1->sections[KIP_TEXT].size_decomp;
				u32 size_comp   = kip1->sections[KIP_RODATA].size_comp;
				u32 size_decomp = kip1->sections[KIP_RODATA].size_decomp;
				if (kip1->flags & BIT(KIP_RODATA))
					blz_uncompress_srcdest(&kip1->data[offset], size_comp, ro_data, size_decomp);
				else
					memcpy(ro_data, &kip1->data[offset], size_decomp);

				for (u32 i = 0; i < 0x100; i+= sizeof(u32))
				{
					// Check size and name of nss matches.
					if (*(u32 *)&ro_data[i] == 8 && !memcmp("fs.exfat", &ro_data[i + 4], 8))
					{
						kip_name = "FS_exfat";
						break;
					}
				}

				free(ro_data);
			}

			s_printf(filename, "%s.kip1", kip_name);
			emmcsn_path_impl(path, pkg2ini_paths[idx], filename, &emmc_storage);
			if (sd_save_to_file(kip1, kip1_size, path))
				goto out;

			s_printf(txt_buf + strlen(txt_buf), "- Extracted %s.kip1\n", kip_name);
			lv_label_set_text(lb_log, txt_buf);
			manual_system_maintenance(true);

			ptr += kip1_size;
		}
	}

out:
	emmc_gpt_free(&gpt);
out_free:
	free(pkg1);
	free(secmon);
	free(warmboot);
	free(loader);
	free(pkg2);
	free(txt_buf);
	emmc_end();
	sd_unmount();

	if (mkey >= HOS_MKEY_VER_620)
		se_aes_key_clear(8);
out_end:
	// Enable buttons.
	nyx_window_toggle_buttons(win, false);

	return LV_RES_OK;
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
		"Allows you to backup eMMC/emuMMC partitions individually\n"
		"or as a whole raw image to the SD card.\n"
		"#C7EA46 Supports SD cards from# #FF8000 4GB# #C7EA46 and up. #"
		"#FF8000 FAT32# #C7EA46 and ##FF8000 exFAT##C7EA46 .#");
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
		"Allows you to restore eMMC/emuMMC partitions individually\n"
		"or as a whole raw image from the SD card.\n"
		"#C7EA46 Supports SD cards from# #FF8000 4GB# #C7EA46 and up. #"
		"#FF8000 FAT32# #C7EA46 and ##FF8000 exFAT##C7EA46 .#");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Misc container.
	lv_obj_t *h2 = _create_container(parent);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "SD Partitions & USB");
	lv_obj_set_style(label_txt3, th->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 3 / 10);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create Partition SD Card button.
	lv_obj_t *btn3 = lv_btn_create(h2, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn3, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn3, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_SD"  Partition SD Card");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, create_window_sd_partition_manager);
	lv_btn_set_action(btn3, LV_BTN_ACTION_LONG_PR, create_window_emmc_partition_manager);

	lv_obj_t *label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"Allows you to partition the SD Card for using it with #C7EA46 emuMMC#,\n"
		"#C7EA46 Android# and #C7EA46 Linux#. You can also flash Linux and Android.\n");
	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");
	lv_obj_align(label_sep, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 11 / 7);

	// Create USB Tools button.
	lv_obj_t *btn4 = lv_btn_create(h2, btn3);
	label_btn = lv_label_create(btn4, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_USB"  USB Tools");
	lv_obj_align(btn4, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _create_window_usb_tools);

	label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"#C7EA46 USB mass storage#, #C7EA46 gamepad# and other USB tools.\n"
		"Mass storage can mount SD, eMMC and emuMMC. The\n"
		"gamepad transforms the Switch into an input device.#");
	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
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

	// Create fix archive bit button.
	lv_obj_t *btn = lv_btn_create(h1, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	lv_obj_t *label_btn = lv_label_create(btn, NULL);
	lv_btn_set_fit(btn, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_DIRECTORY"  Fix Archive Bit");
	lv_obj_align(btn, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, _create_window_unset_abit_tool);

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to fix the archive bit for all folders including\n"
		"the root and emuMMC \'Nintendo\' folders.\n"
		"#C7EA46 It sets the archive bit to folders named with ##FF8000 .[ext]#\n"
		"#FF8000 Use that option when you have corruption messages.#");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Fix touch calibration button.
	lv_obj_t *btn2 = lv_btn_create(h1, btn);
	label_btn = lv_label_create(btn2, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_KEYBOARD"  Calibrate Touchscreen");
	lv_obj_align(btn2, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, _create_mbox_fix_touchscreen);

	label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to calibrate the touchscreen module.\n"
		"#FF8000 This fixes any issues with touchscreen in Nyx and HOS.#");
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
	lv_label_set_text(label_btn, SYMBOL_REFRESH"  AutoRCM #00FFC9   ON #");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _create_mbox_autorcm_status);

	// Set default state for AutoRCM and lock it out if patched unit.
	if (get_set_autorcm_status(false))
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

	char *txt_buf = (char *)malloc(SZ_4K);

	s_printf(txt_buf,
		"Allows you to enter RCM without using #C7EA46 VOL+# & #C7EA46 HOME# (jig).\n"
		"#FF8000 It can restore all versions of AutoRCM whenever requested.#\n"
		"#FF3C28 This corrupts the BCT and you can't boot without a custom#\n"
		"#FF3C28 bootloader.#");

	if (h_cfg.rcm_patched)
		strcat(txt_buf, " #FF8000 This is disabled because this unit is patched!#");

	lv_obj_t *label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_text(label_txt4, txt_buf);
	free(txt_buf);

	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");
	lv_obj_align(label_sep, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 11 / 7);

	// Create Dump Package1/2 button.
	lv_obj_t *btn4 = lv_btn_create(h2, btn);
	label_btn = lv_label_create(btn4, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_MODULES"  Dump Package1/2");
	lv_obj_align(btn4, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _create_window_dump_pk12_tool);

	label_txt2 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Allows you to dump and decrypt pkg1 and pkg2 and further\n"
		"split it up into their individual parts. It also dumps the kip1.");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
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

	lv_obj_t *tab1= lv_tabview_add_tab(tv, "eMMC "SYMBOL_DOT" SD Partitions "SYMBOL_DOT" USB");
	lv_obj_t *tab2 = lv_tabview_add_tab(tv, "Arch bit "SYMBOL_DOT" RCM "SYMBOL_DOT" Touch "SYMBOL_DOT" Pkg1/2");

	lv_obj_t *line_sep = lv_line_create(tv, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { 0, LV_DPI / 4} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
	lv_obj_align(line_sep, tv, LV_ALIGN_IN_BOTTOM_MID, -1, -LV_DPI * 2 / 12);

	_create_tab_tools_emmc_pkg12(th, tab1);
	_create_tab_tools_arc_autorcm(th, tab2);

	lv_tabview_set_tab_act(tv, 0, false);
}
