/*
 * Copyright (c) 2018-2020 CTCaer
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

#include "config.h"
#include "ini.h"
#include "../gfx/gfx.h"
#include "../gfx/tui.h"
#include "../libs/fatfs/ff.h"
#include "../soc/t210.h"
#include "../storage/nx_sd.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/list.h"
#include "../utils/util.h"

extern hekate_config h_cfg;

void set_default_configuration()
{
	h_cfg.autoboot = 0;
	h_cfg.autoboot_list = 0;
	h_cfg.bootwait = 3;
	h_cfg.se_keygen_done = 0;
	h_cfg.sbar_time_keeping = 0;
	h_cfg.backlight = 100;
	h_cfg.autohosoff = 0;
	h_cfg.autonogc = 1;
	h_cfg.updater2p = 0;
	h_cfg.brand = NULL;
	h_cfg.tagline = NULL;
	h_cfg.errors = 0;
	h_cfg.eks = NULL;
	h_cfg.sept_run = EMC(EMC_SCRATCH0) & EMC_SEPT_RUN;
	h_cfg.rcm_patched = true;
	h_cfg.emummc_force_disable = false;

	sd_power_cycle_time_start = 0;
}

int create_config_entry()
{
	if (!sd_mount())
		return 1;

	char lbuf[32];
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
	if (h_cfg.brand)
	{
		f_puts("\nbrand=", &fp);
		f_puts(h_cfg.brand, &fp);
	}
	if (h_cfg.tagline)
	{
		f_puts("\ntagline=", &fp);
		f_puts(h_cfg.tagline, &fp);
	}
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
	sd_unmount();

	return 0;
}

#pragma GCC push_options
#pragma GCC optimize ("Os")

static void _save_config()
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	if (!create_config_entry())
		gfx_puts("\nConfiguration was saved!\n");
	else
		EPRINTF("\nConfiguration saving failed!");
	gfx_puts("\nPress any key...");
}

static void _config_autoboot_list(void *ent)
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	u32 *temp_autoboot = NULL;

	LIST_INIT(ini_sections);

	u8 max_entries = 30;

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 3));
	u32 *boot_values = (u32 *)malloc(sizeof(u32) * max_entries);
	char *boot_text = (char *)malloc(512 * max_entries);

	for (u32 j = 0; j < max_entries; j++)
		boot_values[j] = j;

	if (sd_mount())
	{
		if (ini_parse(&ini_sections, "bootloader/ini", true))
		{
			// Build configuration menu.
			ments[0].type = MENT_BACK;
			ments[0].caption = "Back";

			ments[1].type = MENT_CHGLINE;

			u32 i = 2;
			LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
			{
				// Skip other ini entries for autoboot.
				if (ini_sec->type == INI_CHOICE)
				{
					if (!strcmp(ini_sec->name, "config"))
						continue;

					if (strlen(ini_sec->name) > 510)
						ments[i].caption = ini_sec->name;
					else
					{
						if (h_cfg.autoboot != (i - 1) || !h_cfg.autoboot_list)
							boot_text[(i - 1) * 512] = ' ';

						else
							boot_text[(i - 1) * 512] = '*';
						strcpy(boot_text + (i - 1) * 512 + 1, ini_sec->name);
						ments[i].caption = &boot_text[(i - 1) * 512];
					}
					ments[i].type = ini_sec->type;
					ments[i].data = &boot_values[i - 1];
					i++;

					if ((i - 1) > max_entries)
						break;
				}
			}

			memset(&ments[i], 0, sizeof(ment_t));
			menu_t menu = {ments, "Select an entry to auto boot", 0, 0};
			temp_autoboot = (u32 *)tui_do_menu(&menu);
			if (temp_autoboot != NULL)
			{
				h_cfg.autoboot = *(u32 *)temp_autoboot;
				h_cfg.autoboot_list = 1;
				_save_config();

				ment_t *tmp = (ment_t *)ent;
				tmp->data = NULL;
			}
			else
				goto out2;
		}
		else
		{
			EPRINTF("Could not open 'bootloader/hekate_ipl.ini'.\nMake sure it exists!.");
			goto out;
		}
	}

out:;
	btn_wait();
out2:;
	free(ments);
	free(boot_values);
	free(boot_text);

	sd_unmount();
}

void config_autoboot()
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	u32 *temp_autoboot = NULL;

	LIST_INIT(ini_sections);

	u8 max_entries = 30;
	u32 boot_text_size = 512;

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 5));
	u32 *boot_values = (u32 *)malloc(sizeof(u32) * max_entries);
	char *boot_text = (char *)malloc(boot_text_size * max_entries);

	for (u32 j = 0; j < max_entries; j++)
		boot_values[j] = j;

	if (sd_mount())
	{
		if (ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false))
		{
			// Build configuration menu.
			ments[0].type = MENT_BACK;
			ments[0].caption = "Back";

			ments[1].type = MENT_CHGLINE;

			ments[2].type = MENT_DATA;
			if (!h_cfg.autoboot)
				ments[2].caption = "*Disable";
			else
				ments[2].caption = " Disable";
			ments[2].data = &boot_values[0];

			ments[3].type = MENT_HDLR_RE;
			if (h_cfg.autoboot_list)
				ments[3].caption = "*More configs...";
			else
				ments[3].caption = " More configs...";
			ments[3].handler = _config_autoboot_list;
			ments[3].data = (void *)0xCAFE;

			ments[4].type = MENT_CHGLINE;

			u32 i = 5;
			LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
			{
				// Skip other ini entries for autoboot.
				if (ini_sec->type == INI_CHOICE)
				{
					if (!strcmp(ini_sec->name, "config"))
						continue;

					if (strlen(ini_sec->name) > 510)
						ments[i].caption = ini_sec->name;
					else
					{
						if (h_cfg.autoboot != (i - 4) || h_cfg.autoboot_list)
							boot_text[(i - 4) * boot_text_size] = ' ';

						else
							boot_text[(i - 4) * boot_text_size] = '*';
						strcpy(boot_text + (i - 4) * boot_text_size + 1, ini_sec->name);
						ments[i].caption = &boot_text[(i - 4) * boot_text_size];
					}
					ments[i].type = ini_sec->type;
					ments[i].data = &boot_values[i - 4];
					i++;

					if ((i - 4) > max_entries)
						break;
				}
			}
			if (i < 6 && !h_cfg.autoboot_list)
			{
				ments[i].type = MENT_CAPTION;
				ments[i].caption = "No main configurations found...";
				ments[i].color = 0xFFFFDD00;
				i++;
			}

			memset(&ments[i], 0, sizeof(ment_t));
			menu_t menu = {ments, "Disable or select entry to auto boot", 0, 0};
			temp_autoboot = (u32 *)tui_do_menu(&menu);
			if (temp_autoboot != NULL)
			{
				h_cfg.autoboot = *(u32 *)temp_autoboot;
				h_cfg.autoboot_list = 0;
				_save_config();
			}
			else
				goto out2;
		}
		else
		{
			EPRINTF("Could not open 'bootloader/hekate_ipl.ini'.\nMake sure it exists!.");
			goto out;
		}
	}

out:;
	btn_wait();
out2:;
	free(ments);
	free(boot_values);
	free(boot_text);

	sd_unmount();

	if (temp_autoboot == NULL)
		return;
}

void config_bootdelay()
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	u32 delay_entries = 6;
	u32 delay_text_size = 32;

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (delay_entries + 3));
	u32 *delay_values = (u32 *)malloc(sizeof(u32) * delay_entries);
	char *delay_text = (char *)malloc(delay_text_size * delay_entries);

	for (u32 j = 0; j < delay_entries; j++)
		delay_values[j] = j;

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	ments[2].type = MENT_DATA;
	if (h_cfg.bootwait)
		ments[2].caption = " 0 seconds (Bootlogo disabled)";
	else
		ments[2].caption = "*0 seconds (Bootlogo disabled)";
	ments[2].data = &delay_values[0];

	u32 i = 0;
	for (i = 1; i < delay_entries; i++)
	{
		if (h_cfg.bootwait != i)
			delay_text[i * delay_text_size] = ' ';
		else
			delay_text[i * delay_text_size] = '*';
		delay_text[i * delay_text_size + 1] = i + '0';
		strcpy(delay_text + i * delay_text_size + 2, " seconds");

		ments[i + 2].type = MENT_DATA;
		ments[i + 2].caption = delay_text + (i * delay_text_size);
		ments[i + 2].data = &delay_values[i];
	}

	memset(&ments[i + 2], 0, sizeof(ment_t));
	menu_t menu = {ments, "Time delay for entering bootloader menu", 0, 0};

	u32 *temp_bootwait = (u32 *)tui_do_menu(&menu);
	if (temp_bootwait != NULL)
	{
		h_cfg.bootwait = *(u32 *)temp_bootwait;
		_save_config();
	}

	free(ments);
	free(delay_values);
	free(delay_text);

	if (temp_bootwait == NULL)
		return;
	btn_wait();
}

void config_backlight()
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	u32 bri_text_size = 8;
	u32 bri_entries = 11;

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (bri_entries + 3));
	u32 *bri_values = (u32 *)malloc(sizeof(u32) * bri_entries);
	char *bri_text = (char *)malloc(bri_text_size * bri_entries);

	for (u32 j = 1; j < bri_entries; j++)
		bri_values[j] = j * 10;

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	u32 i = 0;
	for (i = 1; i < bri_entries; i++)
	{
		if ((h_cfg.backlight / 20) != i)
			bri_text[i * bri_text_size] = ' ';
		else
			bri_text[i * bri_text_size] = '*';

		if (i < 10)
		{
			bri_text[i * bri_text_size + 1] = i + '0';
			strcpy(bri_text + i * bri_text_size + 2, "0%");
		}
		else
			strcpy(bri_text + i * bri_text_size + 1, "100%");

		ments[i + 1].type = MENT_DATA;
		ments[i + 1].caption = bri_text + (i * bri_text_size);
		ments[i + 1].data = &bri_values[i];
	}

	memset(&ments[i + 1], 0, sizeof(ment_t));
	menu_t menu = {ments, "Backlight brightness", 0, 0};

	u32 *temp_backlight = (u32 *)tui_do_menu(&menu);
	if (temp_backlight != NULL)
	{
		h_cfg.backlight = (*(u32 *)temp_backlight) * 2;
		_save_config();
	}

	free(ments);
	free(bri_values);
	free(bri_text);

	if (temp_backlight == NULL)
		return;
	btn_wait();
}

void config_auto_hos_poweroff()
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * 6);
	u32 *hp_values = (u32 *)malloc(sizeof(u32) * 3);

	for (u32 j = 0; j < 3; j++)
	{
		hp_values[j] = j;
		ments[j + 2].type = MENT_DATA;
		ments[j + 2].data = &hp_values[j];
	}

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	if (h_cfg.autohosoff == 1)
	{
		ments[2].caption = " Disable";
		ments[3].caption = "*Enable";
		ments[4].caption = " Enable (No logo)";
	}
	else if (h_cfg.autohosoff >= 2)
	{
		ments[2].caption = " Disable";
		ments[3].caption = " Enable";
		ments[4].caption = "*Enable (No logo)";
	}
	else
	{
		ments[2].caption = "*Disable";
		ments[3].caption = " Enable";
		ments[4].caption = " Enable (No logo)";
	}

	memset(&ments[5], 0, sizeof(ment_t));
	menu_t menu = {ments, "Power off if woke up from HOS", 0, 0};

	u32 *temp_autohosoff = (u32 *)tui_do_menu(&menu);
	if (temp_autohosoff != NULL)
	{
		h_cfg.autohosoff = *(u32 *)temp_autohosoff;
		_save_config();
	}

	free(ments);
	free(hp_values);

	if (temp_autohosoff == NULL)
		return;
	btn_wait();
}

void config_nogc()
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * 5);
	u32 *cb_values = (u32 *)malloc(sizeof(u32) * 2);

	for (u32 j = 0; j < 2; j++)
	{
		cb_values[j] = j;
		ments[j + 2].type = MENT_DATA;
		ments[j + 2].data = &cb_values[j];
	}

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	if (h_cfg.autonogc)
	{
		ments[2].caption = " Disable";
		ments[3].caption = "*Auto";
	}
	else
	{
		ments[2].caption = "*Disable";
		ments[3].caption = " Auto";
	}

	memset(&ments[4], 0, sizeof(ment_t));
	menu_t menu = {ments, "No Gamecard", 0, 0};

	u32 *temp_nogc = (u32 *)tui_do_menu(&menu);
	if (temp_nogc != NULL)
	{
		h_cfg.autonogc = *(u32 *)temp_nogc;
		_save_config();
	}

	free(ments);
	free(cb_values);

	if (temp_nogc == NULL)
		return;
	btn_wait();
}

#pragma GCC pop_options
