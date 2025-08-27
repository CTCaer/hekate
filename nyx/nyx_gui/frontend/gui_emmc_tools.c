/*
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
#include "gui_emmc_tools.h"
#include "gui_tools.h"
#include "fe_emmc_tools.h"
#include "../config.h"
#include "../hos/pkg1.h"
#include "../hos/pkg2.h"
#include "../hos/hos.h"
#include <libs/fatfs/ff.h>

extern boot_cfg_t b_cfg;

extern char *emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

typedef struct _emmc_backup_buttons_t
{
	lv_obj_t *emmc_boot;
	lv_obj_t *emmc_raw_gpp;
	lv_obj_t *emmc_sys;
	lv_obj_t *emmc_usr;
	bool raw_emummc;
	bool restore;
} emmc_backup_buttons_t;

static emmc_backup_buttons_t emmc_btn_ctxt;

static void _create_window_backup_restore(emmcPartType_t type, const char* win_label)
{
	emmc_tool_gui_t emmc_tool_gui_ctxt;

	emmc_tool_gui_ctxt.raw_emummc = emmc_btn_ctxt.raw_emummc;

	char win_label_full[80];

	s_printf(win_label_full, "%s%s", emmc_btn_ctxt.restore ? SYMBOL_DOWNLOAD"  Restore " : SYMBOL_UPLOAD"  Backup ", win_label+3);

	lv_obj_t *win = nyx_create_standard_window(win_label_full);

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

	// Create log container.
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

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	// Create info elements.
	lv_obj_t *label_info = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_info, true);
	lv_obj_set_width(label_info, lv_obj_get_width(h1));
	lv_label_set_static_text(label_info, "\n\n\n\n\n\n\n\n\n");
	lv_obj_set_style(label_info, lv_theme_get_current()->label.prim);
	lv_obj_align(label_info, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 10);
	emmc_tool_gui_ctxt.label_info = label_info;

	static lv_style_t bar_teal_bg, bar_teal_ind, bar_orange_bg, bar_orange_ind, bar_white_ind;

	lv_style_copy(&bar_teal_bg, lv_theme_get_current()->bar.bg);
	bar_teal_bg.body.main_color = LV_COLOR_HEX(0x005a47);
	bar_teal_bg.body.grad_color = bar_teal_bg.body.main_color;

	lv_style_copy(&bar_teal_ind, lv_theme_get_current()->bar.indic);
	bar_teal_ind.body.main_color = LV_COLOR_HEX(0x00FFC9);
	bar_teal_ind.body.grad_color = bar_teal_ind.body.main_color;

	lv_style_copy(&bar_orange_bg, lv_theme_get_current()->bar.bg);
	bar_orange_bg.body.main_color = LV_COLOR_HEX(0x755000);
	bar_orange_bg.body.grad_color = bar_orange_bg.body.main_color;

	lv_style_copy(&bar_orange_ind, lv_theme_get_current()->bar.indic);
	bar_orange_ind.body.main_color = LV_COLOR_HEX(0xFFAE00);
	bar_orange_ind.body.grad_color = bar_orange_ind.body.main_color;

	lv_style_copy(&bar_white_ind, lv_theme_get_current()->bar.indic);
	bar_white_ind.body.main_color = LV_COLOR_HEX(0xF0F0F0);
	bar_white_ind.body.grad_color = bar_white_ind.body.main_color;

	emmc_tool_gui_ctxt.bar_teal_bg = &bar_teal_bg;
	emmc_tool_gui_ctxt.bar_teal_ind = &bar_teal_ind;
	emmc_tool_gui_ctxt.bar_orange_bg = &bar_orange_bg;
	emmc_tool_gui_ctxt.bar_orange_ind = &bar_orange_ind;
	emmc_tool_gui_ctxt.bar_white_bg = lv_theme_get_current()->bar.bg;
	emmc_tool_gui_ctxt.bar_white_ind = &bar_white_ind;

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

	if (!emmc_btn_ctxt.restore)
		dump_emmc_selected(type, &emmc_tool_gui_ctxt);
	else
		restore_emmc_selected(type, &emmc_tool_gui_ctxt);

	nyx_window_toggle_buttons(win, false);

	// Refresh AutoRCM button.
	if (emmc_btn_ctxt.restore && (type == PART_BOOT) && !emmc_btn_ctxt.raw_emummc)
	{
		if (get_set_autorcm_status(false))
			lv_btn_set_state(autorcm_btn, LV_BTN_STATE_TGL_REL);
		else
			lv_btn_set_state(autorcm_btn, LV_BTN_STATE_REL);
		nyx_generic_onoff_toggle(autorcm_btn);

		if (h_cfg.rcm_patched)
		{
			lv_obj_set_click(autorcm_btn, false);
			lv_btn_set_state(autorcm_btn, LV_BTN_STATE_INA);
		}
	}
}

static lv_res_t _emmc_backup_buttons_decider(lv_obj_t *btn)
{
	if (!nyx_emmc_check_battery_enough())
		return LV_RES_OK;

	char *win_label = lv_label_get_text(lv_obj_get_child(btn, NULL));

	if (emmc_btn_ctxt.emmc_boot == btn)
		_create_window_backup_restore(PART_BOOT, win_label);
	else if (emmc_btn_ctxt.emmc_raw_gpp == btn)
		_create_window_backup_restore(PART_RAW, win_label);
	else if (emmc_btn_ctxt.emmc_sys == btn)
	{
		if (!emmc_btn_ctxt.restore)
			_create_window_backup_restore(PART_SYSTEM, win_label);
		else
			_create_window_backup_restore(PART_GP_ALL, win_label);
	}
	else if (!emmc_btn_ctxt.restore && emmc_btn_ctxt.emmc_usr == btn)
		_create_window_backup_restore(PART_USER, win_label);

	return LV_RES_OK;
}

static lv_res_t _emmc_backup_buttons_raw_toggle(lv_obj_t *btn)
{
	nyx_generic_onoff_toggle(btn);

	lv_obj_set_click(emmc_btn_ctxt.emmc_boot, true);
	lv_btn_set_state(emmc_btn_ctxt.emmc_boot, LV_BTN_STATE_REL);
	lv_obj_set_click(emmc_btn_ctxt.emmc_raw_gpp, true);
	lv_btn_set_state(emmc_btn_ctxt.emmc_raw_gpp, LV_BTN_STATE_REL);

	lv_obj_set_click(emmc_btn_ctxt.emmc_sys, true);
	lv_btn_set_state(emmc_btn_ctxt.emmc_sys, LV_BTN_STATE_REL);

	// Backup/Restore from and to eMMC.
	if (!(lv_btn_get_state(btn) & LV_BTN_STATE_TGL_REL))
	{
		if (!emmc_btn_ctxt.restore)
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_boot, NULL), SYMBOL_UPLOAD"  eMMC BOOT0 & BOOT1");
		else
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_boot, NULL), SYMBOL_DOWNLOAD"  eMMC BOOT0 & BOOT1");
		lv_obj_realign(emmc_btn_ctxt.emmc_boot);

		if (!emmc_btn_ctxt.restore)
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_raw_gpp, NULL), SYMBOL_UPLOAD"  eMMC RAW GPP");
		else
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_raw_gpp, NULL), SYMBOL_DOWNLOAD"  eMMC RAW GPP");
		lv_obj_realign(emmc_btn_ctxt.emmc_raw_gpp);

		if (!emmc_btn_ctxt.restore)
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_sys, NULL), SYMBOL_MODULES"  eMMC SYS");
		else
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_sys, NULL), SYMBOL_MODULES"  eMMC ALL");
		lv_obj_realign(emmc_btn_ctxt.emmc_sys);

		if (!emmc_btn_ctxt.restore)
		{
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_usr, NULL), SYMBOL_MODULES_ALT"  eMMC USER");
			lv_obj_realign(emmc_btn_ctxt.emmc_usr);

			lv_obj_set_click(emmc_btn_ctxt.emmc_usr, true);
			lv_btn_set_state(emmc_btn_ctxt.emmc_usr, LV_BTN_STATE_REL);
		}

		emmc_btn_ctxt.raw_emummc = false;
	}
	else // Backup/Restore from and to emuMMC.
	{
		if (!emmc_btn_ctxt.restore)
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_boot, NULL), SYMBOL_UPLOAD"  SD emuMMC BOOT0 & BOOT1");
		else
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_boot, NULL), SYMBOL_DOWNLOAD"  SD emuMMC BOOT0 & BOOT1");
		lv_obj_realign(emmc_btn_ctxt.emmc_boot);

		if (!emmc_btn_ctxt.restore)
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_raw_gpp, NULL), SYMBOL_UPLOAD"  SD emuMMC RAW GPP");
		else
			lv_label_set_static_text(lv_obj_get_child(emmc_btn_ctxt.emmc_raw_gpp, NULL), SYMBOL_DOWNLOAD"  SD emuMMC RAW GPP");
		lv_obj_realign(emmc_btn_ctxt.emmc_raw_gpp);

		lv_obj_set_click(emmc_btn_ctxt.emmc_sys, false);
		lv_btn_set_state(emmc_btn_ctxt.emmc_sys, LV_BTN_STATE_INA);

		if (!emmc_btn_ctxt.restore)
		{
			lv_obj_set_click(emmc_btn_ctxt.emmc_usr, false);
			lv_btn_set_state(emmc_btn_ctxt.emmc_usr, LV_BTN_STATE_INA);
		}

		emmc_btn_ctxt.raw_emummc = true;
	}

	return LV_RES_OK;
}

lv_res_t create_window_backup_restore_tool(lv_obj_t *btn)
{
	lv_obj_t *win;

	emmc_btn_ctxt.restore = false;
	if (strcmp(lv_label_get_text(lv_obj_get_child(btn, NULL)), SYMBOL_UPLOAD"  Backup eMMC"))
		emmc_btn_ctxt.restore = true;

	if (!emmc_btn_ctxt.restore)
		win = nyx_create_standard_window(SYMBOL_SD" Backup");
	else
		win = nyx_create_standard_window(SYMBOL_SD" Restore");

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 9;

	// Create Full container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "Full");
	lv_obj_set_style(label_txt, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 3 / 10);

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create BOOT0 & BOOT1 button.
	lv_obj_t *btn1 = lv_btn_create(h1, NULL);
	lv_obj_t *label_btn = lv_label_create(btn1, NULL);
	lv_btn_set_fit(btn1, true, true);
	if (!emmc_btn_ctxt.restore)
		lv_label_set_static_text(label_btn, SYMBOL_UPLOAD"  eMMC BOOT0 & BOOT1");
	else
		lv_label_set_static_text(label_btn, SYMBOL_DOWNLOAD"  eMMC BOOT0 & BOOT1");

	lv_obj_align(btn1, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, _emmc_backup_buttons_decider);
	emmc_btn_ctxt.emmc_boot = btn1;

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	if (!emmc_btn_ctxt.restore)
	{
		lv_label_set_static_text(label_txt2,
			"Allows you to backup the BOOT physical partitions.\n"
			"They contain the BCT, keys and various package1.\n"
			"#FF8000 These are paired with the RAW GPP backup.#");
	}
	else
	{
		lv_label_set_static_text(label_txt2,
			"Allows you to restore the BOOT physical partitions.\n"
			"They contain the BCT, keys and various package1.\n"
			"#FF8000 These are paired with the RAW GPP restore.#");
	}
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create RAW GPP button.
	lv_obj_t *btn2 = lv_btn_create(h1, btn1);
	label_btn = lv_label_create(btn2, NULL);
	if (!emmc_btn_ctxt.restore)
		lv_label_set_static_text(label_btn, SYMBOL_UPLOAD"  eMMC RAW GPP");
	else
		lv_label_set_static_text(label_btn, SYMBOL_DOWNLOAD"  eMMC RAW GPP");
	lv_obj_align(btn2, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, _emmc_backup_buttons_decider);
	emmc_btn_ctxt.emmc_raw_gpp= btn2;

	label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	if (!emmc_btn_ctxt.restore)
	{
		lv_label_set_static_text(label_txt2,
			"Allows you to backup the GPP physical partition.\n"
			"It contains, CAL0, various package2, SYSTEM, USER, etc.\n"
			"#FF8000 This is paired with the BOOT0/1 backups.#");
	}
	else
	{
		lv_label_set_static_text(label_txt2,
			"Allows you to restore the GPP physical partition.\n"
			"It contains, CAL0, various package2, SYSTEM, USER, etc.\n"
			"#FF8000 This is paired with the BOOT0/1 restore.#");
	}
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create GPP Partitions container.
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
	lv_label_set_static_text(label_txt3, "GPP Partitions");
	lv_obj_set_style(label_txt3, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI * 4 / 21);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create SYS/ALL button.
	lv_obj_t *btn3 = lv_btn_create(h2, NULL);
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	if (!emmc_btn_ctxt.restore)
		lv_label_set_static_text(label_btn, SYMBOL_MODULES"  eMMC SYS");
	else
		lv_label_set_static_text(label_btn, SYMBOL_MODULES"  eMMC ALL");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _emmc_backup_buttons_decider);
	emmc_btn_ctxt.emmc_sys = btn3;

	lv_obj_t *label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	if (!emmc_btn_ctxt.restore)
	{
		lv_label_set_static_text(label_txt4,
			"Allows you to backup the partitions from RAW GPP except\n"
			"USER. It contains, CAL0, various package2, SYSTEM, etc.\n"
			"#FF8000 This is an incomplete backup.#");
	}
	else
	{
		lv_label_set_static_text(label_txt4,
			"Allows you to restore ALL partitions from RAW GPP\n"
			"It contains, CAL0, various package2, SYSTEM, USER, etc.\n");
	}

	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create USER button.
	if (!emmc_btn_ctxt.restore)
	{
		lv_obj_t *btn4 = lv_btn_create(h2, btn1);
		label_btn = lv_label_create(btn4, NULL);
		lv_label_set_static_text(label_btn, SYMBOL_MODULES_ALT"  eMMC USER");
		lv_obj_align(btn4, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
		lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _emmc_backup_buttons_decider);
		emmc_btn_ctxt.emmc_usr = btn4;

		label_txt4 = lv_label_create(h2, NULL);
		lv_label_set_recolor(label_txt4, true);
		lv_label_set_static_text(label_txt4,
			"Allows you to backup the USER partition from RAW GPP.\n"
			"#FF8000 This is an incomplete backup.#\n");
		lv_obj_set_style(label_txt4, &hint_small_style);
		lv_obj_align(label_txt4, btn4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
	}
	else
	{
		emmc_btn_ctxt.emmc_usr = NULL;
	}

	// Create eMMC/emuMMC On/Off button.
	lv_obj_t *h3 = lv_cont_create(win, NULL);
	lv_cont_set_style(h3, &h_style);
	lv_cont_set_fit(h3, false, true);
	lv_obj_set_width(h3, (LV_HOR_RES / 10) * 4);
	lv_obj_set_click(h3, false);
	lv_cont_set_layout(h3, LV_LAYOUT_OFF);
	lv_obj_align(h3, h1, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI * 38 / 11, LV_DPI / 7);

	lv_obj_t *sd_emummc_raw = lv_btn_create(h3, NULL);
	nyx_create_onoff_button(lv_theme_get_current(), h3,
		sd_emummc_raw, SYMBOL_SD" SD emuMMC Raw Partition", _emmc_backup_buttons_raw_toggle, false);
	emmc_btn_ctxt.raw_emummc = false;

	return LV_RES_OK;
}
