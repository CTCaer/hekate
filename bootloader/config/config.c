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

#include <string.h>
#include <stdlib.h>

#include "../utils/btn.h"
#include "config.h"
#include "../libs/fatfs/ff.h"
#include "ini.h"
#include "../utils/list.h"
#include "../gfx/tui.h"
#include "../utils/util.h"
#include "../gfx/gfx.h"

extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;
//TODO: Create more macros (info, header, debug, etc) with different colors and utilize them for consistency.
#define EPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, 0xFFCCCCCC)
#define EPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, args, 0xFFCCCCCC)

extern hekate_config h_cfg;
extern int sd_mount();
extern int sd_unmount();

void set_default_configuration()
{
	h_cfg.autoboot = 0;
	h_cfg.bootwait = 3;
	h_cfg.customlogo = 0;
	h_cfg.verification = 2;
	h_cfg.se_keygen_done = 0;
	h_cfg.sbar_time_keeping = 0;
}

int create_config_entry()
{
	if (!sd_mount())
		return 1;

	char lbuf[16];
	FIL fp;

	LIST_INIT(ini_sections);

	if (ini_parse(&ini_sections, "hekate_ipl.ini"))
	{
		if (f_open(&fp, "hekate_ipl.ini", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
			return 0;
		// Add config entry.
		f_puts("[config]\nautoboot=", &fp);
		itoa(h_cfg.autoboot, lbuf, 10);
		f_puts(lbuf, &fp);
		f_puts("\nbootwait=", &fp);
		itoa(h_cfg.bootwait, lbuf, 10);
		f_puts(lbuf, &fp);
		f_puts("\ncustomlogo=", &fp);
		itoa(h_cfg.customlogo, lbuf, 10);
		f_puts(lbuf, &fp);
		f_puts("\nverification=", &fp);
		itoa(h_cfg.verification, lbuf, 10);
		f_puts(lbuf, &fp);
		f_puts("\n", &fp);

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

		f_close(&fp);
		sd_unmount();
	}
	else
		return 1;

	ini_free(&ini_sections);

	return 0;
}

void config_autoboot()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	u32 *temp_autoboot = NULL;

	LIST_INIT(ini_sections);

	u8 max_entries = 29;
	int ini_freed = 1;

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 3));
	u32 *boot_values = (u32 *)malloc(sizeof(u32) * max_entries);
	char *boot_text = (char *)malloc(512 * max_entries);

	for (u32 j = 0; j < max_entries; j++)
		boot_values[j] = j;

	if (sd_mount())
	{
		if (ini_parse(&ini_sections, "hekate_ipl.ini"))
		{
			ini_freed = 0;

			// Build configuration menu.
			ments[0].type = MENT_BACK;
			ments[0].caption = "Back";

			ments[1].type = MENT_CHGLINE;

			ments[2].type = MENT_CHOICE;
			if (!h_cfg.autoboot)
				ments[2].caption = "*Disable";
			else
				ments[2].caption = " Disable";
			ments[2].data = &boot_values[0];

			u32 i = 3;
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
						if (h_cfg.autoboot != (i - 2))
							boot_text[(i - 2) * 512] = ' ';

						else
							boot_text[(i - 2) * 512] = '*';
						memcpy(boot_text + (i - 2) * 512 + 1, ini_sec->name, strlen(ini_sec->name));
						boot_text[strlen(ini_sec->name) + (i - 2) * 512 + 1] = 0;
						ments[i].caption = &boot_text[(i - 2) * 512];
					}
					ments[i].type = ini_sec->type;
					ments[i].data = &boot_values[i - 2];
					i++;

					if (i > max_entries)
						break;
				}
			}
			if (i < 4)
			{
				EPRINTF("No launch configurations found.");
				goto out;
			}

			memset(&ments[i], 0, sizeof(ment_t));
			menu_t menu = {ments, "Disable or select entry to auto boot", 0, 0};
			temp_autoboot = (u32 *)tui_do_menu(&gfx_con, &menu);
			if (temp_autoboot != NULL)
			{
				gfx_clear_grey(&gfx_ctxt, 0x1B);
				gfx_con_setpos(&gfx_con, 0, 0);

				h_cfg.autoboot = *(u32 *)temp_autoboot;
				// Save choice to ini file.
				if (!create_config_entry())
					gfx_puts(&gfx_con, "\nConfiguration was saved!\n");
				else
					EPRINTF("\nConfiguration saving failed!");
				gfx_puts(&gfx_con, "\nPress any key...");
			}
			else
				goto out2;
		}
		else
		{
			EPRINTF("Could not find or open 'hekate_ipl.ini'.\nMake sure it exists in SD Card!.");
			goto out;
		}
	}

out:;
	btn_wait();
out2:;
	free(ments);
	free(boot_values);
	free(boot_text);
	if (!ini_freed)
		ini_free(&ini_sections);

	sd_unmount();

	if (temp_autoboot == NULL)
		return;
}

void config_bootdelay()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	u32 delay_entries = 6;

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (delay_entries + 3));
	u32 *delay_values = (u32 *)malloc(sizeof(u32) * delay_entries);
	char *delay_text = (char *)malloc(32 * delay_entries);

	for (u32 j = 0; j < delay_entries; j++)
		delay_values[j] = j;

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	ments[2].type = MENT_CHOICE;
	if (h_cfg.bootwait)
		ments[2].caption = " 0 seconds (Bootlogo disabled)";
	else
		ments[2].caption = "*0 seconds (Bootlogo disabled)";
	ments[2].data = &delay_values[0];

	u32 i = 0;
	for (i = 1; i < delay_entries; i++)
	{
		if (h_cfg.bootwait != i)
			delay_text[i * 32] = ' ';
		else
			delay_text[i * 32] = '*';
		delay_text[i * 32 + 1] = i + '0';
		memcpy(delay_text + i * 32 + 2, " seconds", 9);

		ments[i + 2].type = MENT_CHOICE;
		ments[i + 2].caption = delay_text + i * 32;
		ments[i + 2].data = &delay_values[i];
	}

	memset(&ments[i + 2], 0, sizeof(ment_t));
	menu_t menu = {ments, "Time delay for entering bootloader menu", 0, 0};

	u32 *temp_bootwait = (u32 *)tui_do_menu(&gfx_con, &menu);
	if (temp_bootwait != NULL)
	{
		gfx_clear_grey(&gfx_ctxt, 0x1B);
		gfx_con_setpos(&gfx_con, 0, 0);

		h_cfg.bootwait = *(u32 *)temp_bootwait;
		// Save choice to ini file.
		if (!create_config_entry())
			gfx_puts(&gfx_con, "\nConfiguration was saved!\n");
		else
			EPRINTF("\nConfiguration saving failed!");
		gfx_puts(&gfx_con, "\nPress any key...");
	}

	free(ments);
	free(delay_values);
	free(delay_text);

	if (temp_bootwait == NULL)
		return;
	btn_wait();
}

void config_customlogo()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * 5);
	u32 *cb_values = (u32 *)malloc(sizeof(u32) * 2);

	for (u32 j = 0; j < 2; j++)
	{
		cb_values[j] = j;
		ments[j + 2].type = MENT_CHOICE;
		ments[j + 2].data = &cb_values[j];
	}

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	if (h_cfg.customlogo)
	{
		ments[2].caption = " Disable";
		ments[3].caption = "*Enable";

	}
	else
	{
		ments[2].caption = "*Disable";
		ments[3].caption = " Enable";
	}

	memset(&ments[4], 0, sizeof(ment_t));
	menu_t menu = {ments, "Custom bootlogo", 0, 0};

	u32 *temp_customlogo = (u32 *)tui_do_menu(&gfx_con, &menu);
	if (temp_customlogo != NULL)
	{
		gfx_clear_grey(&gfx_ctxt, 0x1B);
		gfx_con_setpos(&gfx_con, 0, 0);

		h_cfg.customlogo = *(u32 *)temp_customlogo;
		// Save choice to ini file.
		if (!create_config_entry())
			gfx_puts(&gfx_con, "\nConfiguration was saved!\n");
		else
			EPRINTF("\nConfiguration saving failed!");
		gfx_puts(&gfx_con, "\nPress any key...");
	}

	free(ments);
	free(cb_values);

	if (temp_customlogo == NULL)
		return;
	btn_wait();
}

void config_verification()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * 6);
	u32 *vr_values = (u32 *)malloc(sizeof(u32) * 3);
	char *vr_text = (char *)malloc(64 * 3);

	for (u32 j = 0; j < 3; j++)
	{
		vr_values[j] = j;
		ments[j + 2].type = MENT_CHOICE;
		ments[j + 2].data = &vr_values[j];
	}

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	memcpy(vr_text,       " Disable", 9);
	memcpy(vr_text + 64,  " Sparse (Fast - Not  reliable)", 31);
	memcpy(vr_text + 128, " Full   (Slow - 100% reliable)", 31);

	for (u32 i = 0; i < 3; i++)
	{
		if (h_cfg.verification != i)
		{
			vr_text[64 * i] = ' ';
			ments[2 + i].caption = vr_text + (i * 64);
		}
		else
		{
			vr_text[64 * i] = '*';
			ments[2 + i].caption = vr_text + (i * 64);
		}
	}

	memset(&ments[5], 0, sizeof(ment_t));
	menu_t menu = {ments, "Backup & Restore verification", 0, 0};

	u32 *temp_verification = (u32 *)tui_do_menu(&gfx_con, &menu);
	if (temp_verification != NULL)
	{
		gfx_clear_grey(&gfx_ctxt, 0x1B);
		gfx_con_setpos(&gfx_con, 0, 0);

		h_cfg.verification = *(u32 *)temp_verification;
		// Save choice to ini file.
		if (!create_config_entry())
			gfx_puts(&gfx_con, "\nConfiguration was saved!\n");
		else
			EPRINTF("\nConfiguration saving failed!");
		gfx_puts(&gfx_con, "\nPress any key...");
	}

	free(ments);
	free(vr_values);
	free(vr_text);

	if (temp_verification == NULL)
		return;
	btn_wait();
}
