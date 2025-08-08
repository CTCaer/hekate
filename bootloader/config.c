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

#include <string.h>
#include <stdlib.h>

#include <bdk.h>

#include "config.h"
#include <libs/fatfs/ff.h>

void set_default_configuration()
{
	h_cfg.t210b01 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210B01;
	h_cfg.devmode = fuse_read_hw_state();

	h_cfg.autoboot      = 0;
	h_cfg.autoboot_list = 0;
	h_cfg.bootwait      = 3;
	h_cfg.noticker      = 0; //! TODO: Add GUI option.
	h_cfg.backlight     = 100;
	h_cfg.autohosoff    = h_cfg.t210b01 ? 1 : 0;
	h_cfg.autonogc      = 1;
	h_cfg.updater2p     = 0;
	h_cfg.bootprotect   = 0;

	h_cfg.errors = 0;
	h_cfg.eks = NULL;
	h_cfg.rcm_patched = fuse_check_patched_rcm();
	h_cfg.emummc_force_disable = false;
}
