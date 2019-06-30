/*
 * Copyright (c) 2018 Rajko Stojadinovic
 * Copyright (c) 2018 CTCaer
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

#ifndef _FE_EMUMMC_TOOLS_H_
#define _FE_EMUMMC_TOOLS_H_

#include "gui.h"

void save_emummc_cfg(u32 part_idx, u32 sector_start, const char *path);
void dump_emummc_file(emmc_tool_gui_t *gui);
void dump_emummc_raw(emmc_tool_gui_t *gui, int part_idx, u32 sector_start);

#endif
