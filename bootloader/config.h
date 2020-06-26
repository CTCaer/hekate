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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "hos/hos.h"
#include <utils/types.h>

typedef struct _hekate_config
{
	// Non-volatile config.
	u32 autoboot;
	u32 autoboot_list;
	u32 bootwait;
	u32 backlight;
	u32 autohosoff;
	u32 autonogc;
	u32 updater2p;
	u32 bootprotect;
	// Global temporary config.
	bool t210b01;
	bool se_keygen_done;
	bool sept_run;
	bool aes_slots_new;
	bool emummc_force_disable;
	bool rcm_patched;
	u32  errors;
	hos_eks_mbr_t *eks;
} hekate_config;

void set_default_configuration();
int create_config_entry();
void config_autoboot();
void config_bootdelay();
void config_backlight();
void config_auto_hos_poweroff();
void config_nogc();

#endif /* _CONFIG_H_ */
