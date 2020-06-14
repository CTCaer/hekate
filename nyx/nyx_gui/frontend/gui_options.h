/*
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

#ifndef _GUI_OPTIONS_H_
#define _GUI_OPTIONS_H_

#include <libs/lvgl/lvgl.h>

void nyx_options_clear_ini_changes_made();
bool nyx_options_get_ini_changes_made();
void first_time_clock_edit(void *param);
lv_res_t create_win_nyx_options(lv_obj_t *parrent_btn);
void create_tab_options(lv_theme_t *th, lv_obj_t *parent);

#endif
