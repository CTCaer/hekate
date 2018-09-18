/*
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "../utils/types.h"

typedef struct _hekate_config
{
	u32 autoboot;
	u32 autoboot_list;
	u32 bootwait;
	u32 customlogo;
	u32 verification;
	// Global temporary config.
	int se_keygen_done;
	u32 sbar_time_keeping;
	u32 backlight;
} hekate_config;

void set_default_configuration();
int create_config_entry();
void config_autoboot();
void config_bootdelay();
void config_customlogo();
void config_verification();
void config_backlight();

#endif /* _CONFIG_H_ */
