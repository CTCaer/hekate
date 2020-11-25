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

#ifndef _FE_EMMC_TOOLS_H_
#define _FE_EMMC_TOOLS_H_

#include "gui.h"

typedef enum
{
	PART_BOOT =   BIT(0),
	PART_SYSTEM = BIT(1),
	PART_USER =   BIT(2),
	PART_RAW =    BIT(3),
	PART_GP_ALL = BIT(7)
} emmcPartType_t;

void dump_emmc_selected(emmcPartType_t dumpType, emmc_tool_gui_t *gui);
void restore_emmc_selected(emmcPartType_t restoreType, emmc_tool_gui_t *gui);

#endif
