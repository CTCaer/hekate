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

#include <string.h>
#include <stdlib.h>

#include <bdk.h>

#include "config.h"
#include "gfx/tui.h"
#include <libs/fatfs/ff.h>

extern hekate_config h_cfg;

void set_default_configuration()
{
	h_cfg.autoboot = 0;
	h_cfg.autoboot_list = 0;
	h_cfg.bootwait = 3;
	h_cfg.backlight = 100;
	h_cfg.autohosoff = 0;
	h_cfg.autonogc = 1;
	h_cfg.updater2p = 0;
	h_cfg.bootprotect = 0;
	h_cfg.errors = 0;
	h_cfg.eks = NULL;
	h_cfg.rcm_patched = fuse_check_patched_rcm();
	h_cfg.emummc_force_disable = false;
	h_cfg.t210b01 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210B01;
}

int create_config_entry()
{
	char lbuf[64];
	FIL fp;
	bool mainIniFound = false;

	LIST_INIT(ini_sections);

	if (ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false))
		mainIniFound = true;
	else
	{
		u8 res = f_open(&fp, "bootloader/hekate_ipl.ini", FA_READ);
		if (res == FR_NO_FILE || res == FR_NO_PATH)
		{
			f_mkdir("bootloader");
			f_mkdir("bootloader/ini");
			f_mkdir("bootloader/payloads");
			f_mkdir("bootloader/sys");
		}
		else
		{
			if (!res)
				f_close(&fp);
			return 1;
		}
	}

	if (f_open(&fp, "bootloader/hekate_ipl.ini", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return 1;

	// Add config entry.
	f_puts("[config]\nautoboot=", &fp);
	itoa(h_cfg.autoboot, lbuf, 10);
	f_puts(lbuf, &fp);

	f_puts("\nautoboot_list=", &fp);
	itoa(h_cfg.autoboot_list, lbuf, 10);
	f_puts(lbuf, &fp);

	f_puts("\nbootwait=", &fp);
	itoa(h_cfg.bootwait, lbuf, 10);
	f_puts(lbuf, &fp);

	f_puts("\nbacklight=", &fp);
	itoa(h_cfg.backlight, lbuf, 10);
	f_puts(lbuf, &fp);

	f_puts("\nautohosoff=", &fp);
	itoa(h_cfg.autohosoff, lbuf, 10);
	f_puts(lbuf, &fp);

	f_puts("\nautonogc=", &fp);
	itoa(h_cfg.autonogc, lbuf, 10);
	f_puts(lbuf, &fp);

	f_puts("\nupdater2p=", &fp);
	itoa(h_cfg.updater2p, lbuf, 10);
	f_puts(lbuf, &fp);

	f_puts("\nbootprotect=", &fp);
	itoa(h_cfg.bootprotect, lbuf, 10);
	f_puts(lbuf, &fp);
	f_puts("\n", &fp);

	if (mainIniFound)
	{
		// Re-construct existing entries.
		LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
		{
			if (!strcmp(ini_sec->name, "config"))
				continue;

			switch (ini_sec->type)
			{
			case INI_CHOICE: // Re-construct Boot entry [ ].
				f_puts("[", &fp);
				f_puts(ini_sec->name, &fp);
				f_puts("]\n", &fp);
				// Re-construct boot entry's config.
				LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
				{
					f_puts(kv->key, &fp);
					f_puts("=", &fp);
					f_puts(kv->val, &fp);
					f_puts("\n", &fp);
				}
				break;
			case INI_CAPTION: // Re-construct caption entry { }.
				f_puts("{", &fp);
				f_puts(ini_sec->name, &fp);
				f_puts("}\n", &fp);
				break;
			case INI_NEWLINE: // Re-construct cosmetic newline \n.
				f_puts("\n", &fp);
				break;
			case INI_COMMENT: // Re-construct comment entry #.
				f_puts("#", &fp);
				f_puts(ini_sec->name, &fp);
				f_puts("\n", &fp);
				break;
			}
		}
	}

	f_close(&fp);

	return 0;
}
