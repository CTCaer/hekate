/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2024 CTCaer
 * Copyright (c) 2018 Reisyukaku
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

#include "fe_tools.h"
#include "../config.h"
#include "../gfx/tui.h"
#include <libs/fatfs/ff.h>

extern boot_cfg_t b_cfg;

#pragma GCC push_options
#pragma GCC optimize ("Os")

static void _toggle_autorcm(bool enable)
{
	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	int i, sect = 0;
	u8 corr_mod0, mod1;
	u8 *tempbuf = (u8 *)malloc(0x200);

	// Get the correct RSA modulus byte masks.
	nx_emmc_get_autorcm_masks(&corr_mod0, &mod1);

	// Iterate BCTs.
	for (i = 0; i < 4; i++)
	{
		sect = (0x200 + (0x4000 * i)) / EMMC_BLOCKSIZE;
		sdmmc_storage_read(&emmc_storage, sect, 1, tempbuf);

		// Check if 2nd byte of modulus is correct.
		if (tempbuf[0x11] != mod1)
			continue;

		if (enable)
			tempbuf[0x10] = 0;
		else
			tempbuf[0x10] = corr_mod0;
		sdmmc_storage_write(&emmc_storage, sect, 1, tempbuf);
	}

	free(tempbuf);

	if (enable)
		gfx_printf("%kAutoRCM mode enabled!%k",  TXT_CLR_ORANGE, TXT_CLR_DEFAULT);
	else
		gfx_printf("%kAutoRCM mode disabled!%k", TXT_CLR_GREENISH, TXT_CLR_DEFAULT);
	gfx_printf("\n\nPress any key...\n");

	btn_wait();
}

static void _enable_autorcm()  { _toggle_autorcm(true); }
static void _disable_autorcm() { _toggle_autorcm(false); }

bool tools_autorcm_enabled()
{
	u8 mod0, mod1;
	u8 *tempbuf = (u8 *)malloc(0x200);

	// Get the correct RSA modulus byte masks.
	nx_emmc_get_autorcm_masks(&mod0, &mod1);

	// Get 1st RSA modulus.
	emmc_set_partition(EMMC_BOOT0);
	sdmmc_storage_read(&emmc_storage, 0x200 / EMMC_BLOCKSIZE, 1, tempbuf);

	// Check if 2nd byte of modulus is correct.
	bool enabled = false;
	if (tempbuf[0x11] == mod1)
		if (tempbuf[0x10] != mod0)
			enabled = true;

	free(tempbuf);

	return enabled;
}

void menu_autorcm()
{
	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	if (h_cfg.rcm_patched)
	{
		WPRINTF("This device is RCM patched and the\nfunction is disabled to avoid BRICKS!\n");
		btn_wait();

		return;
	}

	if (!emmc_initialize(false))
	{
		EPRINTF("Failed to init eMMC.");
		btn_wait();

		return;
	}

	// Do a simple check on the main BCT.
	bool enabled = tools_autorcm_enabled();

	// Create AutoRCM menu.
	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * 6);

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	ments[2].type = MENT_CAPTION;
	ments[3].type = MENT_CHGLINE;
	if (!enabled)
	{
		ments[2].caption = "Status: Disabled!";
		ments[2].color   = TXT_CLR_GREENISH;
		ments[4].caption = "Enable AutoRCM";
		ments[4].handler = _enable_autorcm;
	}
	else
	{
		ments[2].caption = "Status: Enabled!";
		ments[2].color   = TXT_CLR_ORANGE;
		ments[4].caption = "Disable AutoRCM";
		ments[4].handler = _disable_autorcm;
	}
	ments[4].type = MENT_HDLR_RE;
	ments[4].data = NULL;

	memset(&ments[5], 0, sizeof(ment_t));
	menu_t menu = {ments, "This corrupts BOOT0!", 0, 0};

	tui_do_menu(&menu);

	emmc_end();

	free(ments);
}

#pragma GCC pop_options
