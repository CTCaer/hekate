/*
 * Copyright (c) 2018-2021 CTCaer
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

#ifndef _GUI_TOOLS_H_
#define _GUI_TOOLS_H_

#include <libs/lvgl/lvgl.h>

extern lv_obj_t *ums_mbox;

void create_tab_tools(lv_theme_t *th, lv_obj_t *parent);
void nyx_run_ums(void *param);
bool get_autorcm_status(bool change);
lv_res_t action_ums_sd(lv_obj_t *btn);

#endif
