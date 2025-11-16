/*
 * Copyright (c) 2018 naehrwert
 *
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
#include "gfx/logos.h"
#include "gfx/tui.h"
#include "hos/hos.h"
#include "hos/secmon_exo.h"
#include "l4t/l4t.h"
#include <ianos/ianos.h>
#include <libs/compr/blz.h>
#include <libs/fatfs/ff.h>
#include "storage/emummc.h"

#include "frontend/fe_tools.h"
#include "frontend/fe_info.h"

hekate_config h_cfg;
boot_cfg_t __attribute__((section ("._boot_cfg"))) b_cfg;
const volatile ipl_ver_meta_t __attribute__((section ("._ipl_version"))) ipl_ver = {
	.magic = BL_MAGIC,
	.version = (BL_VER_MJ + '0') | ((BL_VER_MN + '0') << 8) | ((BL_VER_HF + '0') << 16) | ((BL_VER_RL) << 24),
	.rsvd0 = 0,
	.rsvd1 = 0
};

volatile nyx_storage_t *nyx_str = (nyx_storage_t *)NYX_STORAGE_ADDR;

static void _check_power_off_from_hos()
{
	// Power off on alarm wakeup from HOS shutdown. For modchips/dongles.
	u8 hos_wakeup = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_IRQTOP);

	// Clear RTC interrupts.
	(void)i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_RTCINT_REG);

	// Stop the alarm, in case we injected and powered off too fast.
	max77620_rtc_stop_alarm();

	// Handle RTC wake up.
	if (hos_wakeup & MAX77620_IRQ_TOP_RTC_MASK)
	{
		if (h_cfg.autohosoff == 1)
		{
			render_static_bootlogo();

			if (display_get_decoded_panel_id() != PANEL_SAM_AMS699VC01)
			{
				// Slow fading for LCD panels.
				display_backlight_brightness(10,  5000);
				display_backlight_brightness(100, 25000);
				msleep(600);
				display_backlight_brightness(0,   20000);
			}
			else
			{
				// Blink 3 times for OLED panel.
				for (u32 i = 0; i < 3; i++)
				{
					msleep(150);
					display_backlight_brightness(100, 0);
					msleep(150);
					display_backlight_brightness(0,   0);
				}
			}
		}
		power_set_state(POWER_OFF_RESET);
	}
}

// This is a safe and unused DRAM region for our payloads.
#define RELOC_META_OFF      0x7C
#define PATCHED_RELOC_SZ    0x94
#define PATCHED_RELOC_STACK 0x40007000
#define PATCHED_RELOC_ENTRY 0x40010000
#define EXT_PAYLOAD_ADDR    0xC0000000
#define RCM_PAYLOAD_ADDR    (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
#define COREBOOT_END_ADDR   0xD0000000
#define COREBOOT_VER_OFF    0x41
#define CBFS_DRAM_EN_ADDR   0x4003E000
#define  CBFS_DRAM_MAGIC    0x4452414D // "DRAM"

static void *coreboot_addr;

static void _reloc_patcher(u32 payload_dst, u32 payload_src, u32 payload_size)
{
	memcpy((u8 *)payload_src, (u8 *)IPL_LOAD_ADDR, PATCHED_RELOC_SZ);

	reloc_meta_t *relocator = (reloc_meta_t *)(payload_src + RELOC_META_OFF);

	relocator->start = payload_dst - ALIGN(PATCHED_RELOC_SZ, 0x10);
	relocator->stack = PATCHED_RELOC_STACK;
	relocator->end   = payload_dst + payload_size;
	relocator->ep    = payload_dst;

	if (payload_size == 0x7000)
	{
		memcpy((u8 *)(payload_src + ALIGN(PATCHED_RELOC_SZ, 0x10)), coreboot_addr, 0x7000); // Bootblock.
		*(vu32 *)CBFS_DRAM_EN_ADDR = CBFS_DRAM_MAGIC;
	}
}

bool is_ipl_updated(void *buf, const char *path, bool force)
{
	ipl_ver_meta_t *update_ft = (ipl_ver_meta_t *)(buf + PATCHED_RELOC_SZ + sizeof(boot_cfg_t));

	bool magic_valid  = update_ft->magic == ipl_ver.magic;
	bool force_update = force && !magic_valid;
	bool is_valid_old = magic_valid && (byte_swap_32(update_ft->version) < byte_swap_32(ipl_ver.version));

	// Check if newer version.
	if (!force && magic_valid)
		if (byte_swap_32(update_ft->version) > byte_swap_32(ipl_ver.version))
			return false;

	// Update if old or broken.
	if (force_update || is_valid_old)
	{
		FIL fp;
		reloc_meta_t *reloc = (reloc_meta_t *)(IPL_LOAD_ADDR + RELOC_META_OFF);
		boot_cfg_t *tmp_cfg = zalloc(sizeof(boot_cfg_t));

		f_open(&fp, path, FA_WRITE | FA_CREATE_ALWAYS);
		f_write(&fp, (u8 *)reloc->start, reloc->end - reloc->start, NULL);

		// Write needed tag in case injected ipl uses old versioning.
		f_write(&fp, "ICTC49", 6, NULL);

		// Reset boot storage configuration.
		f_lseek(&fp, PATCHED_RELOC_SZ);
		f_write(&fp, tmp_cfg, sizeof(boot_cfg_t), NULL);

		f_close(&fp);
		free(tmp_cfg);
	}

	return true;
}

static void _launch_payload(char *path, bool update, bool clear_screen)
{
	if (clear_screen)
		gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	FIL fp;
	if (f_open(&fp, path, FA_READ))
	{
		gfx_con.mute = false;
		EPRINTFARGS("Payload file is missing!\n(%s)", path);

		goto out;
	}

	// Read and copy the payload to our chosen address
	void *buf;
	u32 size = f_size(&fp);

	if (size < 0x30000)
		buf = (void *)RCM_PAYLOAD_ADDR;
	else
	{
		coreboot_addr = (void *)(COREBOOT_END_ADDR - size);
		buf = coreboot_addr;
		if (h_cfg.t210b01)
		{
			f_close(&fp);

			gfx_con.mute = false;
			EPRINTF("Coreboot not allowed on Mariko!");

			goto out;
		}
	}

	if (f_read(&fp, buf, size, NULL))
	{
		f_close(&fp);

		goto out;
	}

	f_close(&fp);

	if (update && is_ipl_updated(buf, path, false))
		goto out;

	sd_end();

	if (size < 0x30000)
	{
		if (update)
			memcpy((u8 *)(RCM_PAYLOAD_ADDR + PATCHED_RELOC_SZ), &b_cfg, sizeof(boot_cfg_t)); // Transfer boot cfg.
		else
			_reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, ALIGN(size, 0x10));

		hw_deinit(false, byte_swap_32(*(u32 *)(buf + size - sizeof(u32))));
	}
	else
	{
		_reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, 0x7000);

		// Get coreboot seamless display magic.
		u32 magic = 0;
		char *magic_ptr = buf + COREBOOT_VER_OFF;
		memcpy(&magic, magic_ptr + strlen(magic_ptr) - 4, 4);
		hw_deinit(true, magic);
	}

	void (*update_ptr)()      = (void *)RCM_PAYLOAD_ADDR;
	void (*ext_payload_ptr)() = (void *)EXT_PAYLOAD_ADDR;

	// Launch our payload.
	if (!update)
	{
		// Some cards (Sandisk U1), do not like a fast power cycle. Wait min 100ms.
		sdmmc_storage_init_wait_sd();

		(*ext_payload_ptr)();
	}
	else
	{
		// Set updated flag to skip check on launch.
		EMC(EMC_SCRATCH0) |= EMC_HEKA_UPD;
		(*update_ptr)();
	}

out:
	if (!update)
	{
		gfx_con.mute = false;
		EPRINTF("Failed to launch payload!");
	}
}

static void _launch_payloads()
{
	u8 max_entries = 61;
	ment_t *ments  = NULL;
	char *file_sec = NULL;
	char *dir = NULL;
	dirlist_t *filelist = NULL;

	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	if (!sd_mount())
		goto failed_sd_mount;

	ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 3));

	dir = (char *)malloc(256);
	memcpy(dir, "bootloader/payloads", 20);

	filelist = dirlist(dir, NULL, 0);

	u32 i = 0;

	if (filelist)
	{
		// Build configuration menu.
		ments[0].type    = MENT_BACK;
		ments[0].caption = "Back";

		ments[1].type    = MENT_CHGLINE;

		while (true)
		{
			if (i > max_entries || !filelist->name[i])
				break;
			ments[i + 2].type    = INI_CHOICE;
			ments[i + 2].caption = filelist->name[i];
			ments[i + 2].data    = filelist->name[i];

			i++;
		}
	}

	if (i > 0)
	{
		memset(&ments[i + 2], 0, sizeof(ment_t));
		menu_t menu = { ments, "Choose a payload", 0, 0 };

		file_sec = (char *)tui_do_menu(&menu);

		if (!file_sec)
		{
			free(ments);
			free(dir);
			free(filelist);
			sd_end();

			return;
		}
	}
	else
		EPRINTF("No payloads found.");

	if (file_sec)
	{
		memcpy(dir + strlen(dir), "/", 2);
		memcpy(dir + strlen(dir), file_sec, strlen(file_sec) + 1);

		_launch_payload(dir, false, true);
	}

failed_sd_mount:
	free(dir);
	free(ments);
	free(filelist);
	sd_end();

	btn_wait();
}

static void _launch_ini_list()
{
	u8 max_entries = 61;
	char *special_path = NULL;
	char *emummc_path  = NULL;
	ment_t *ments      = NULL;
	ini_sec_t *cfg_sec = NULL;

	LIST_INIT(ini_list_sections);

	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	if (!sd_mount())
		goto parse_failed;

	// Check that ini files exist and parse them.
	if (!ini_parse(&ini_list_sections, "bootloader/ini", true))
	{
		EPRINTF("No .ini files in bootloader/ini!");
		goto parse_failed;
	}

	// Build configuration menu.
	ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 3));
	ments[0].type    = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type    = MENT_CHGLINE;

	u32 sec_idx = 2;
	LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_list_sections, link)
	{
		if (ini_sec->type == INI_COMMENT ||
			ini_sec->type == INI_NEWLINE ||
			!strcmp(ini_sec->name, "config"))
			continue;

		ments[sec_idx].type    = ini_sec->type;
		ments[sec_idx].caption = ini_sec->name;
		ments[sec_idx].data    = ini_sec;

		if (ini_sec->type == MENT_CAPTION)
			ments[sec_idx].color = ini_sec->color;
		sec_idx++;

		if ((sec_idx - 1) > max_entries)
			break;
	}

	if (sec_idx > 2)
	{
		memset(&ments[sec_idx], 0, sizeof(ment_t));
		menu_t menu = {
			ments, "Launch ini entries", 0, 0
		};

		cfg_sec = (ini_sec_t *)tui_do_menu(&menu);

		special_path = ini_check_special_section(cfg_sec);

		if (cfg_sec && !special_path)
		{
			LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
			{
				if (!strcmp("emummc_force_disable", kv->key))
					h_cfg.emummc_force_disable = atoi(kv->val);
				else if (!strcmp("emupath", kv->key))
					emummc_path = kv->val;
			}

			if (emummc_path && !emummc_set_path(emummc_path))
			{
				EPRINTF("emupath is wrong!");
				goto wrong_emupath;
			}
		}

		if (!cfg_sec)
		{
			free(ments);

			return;
		}
	}
	else
		EPRINTF("No extra configs found.");

parse_failed:
	if (!cfg_sec)
		goto out;

	if (special_path)
	{
		// Try to launch Payload or L4T.
		if (special_path != (char *)-1)
			_launch_payload(special_path, false, true);
		else
		{
			u32 entry_idx = 0;
			for (u32 i = 0; i < sec_idx; i++)
			{
				if (ments[i].data == cfg_sec)
				{
					entry_idx = i;
					break;
				}
			}
			launch_l4t(cfg_sec, entry_idx, 1, h_cfg.t210b01);
		}
	}
	else
	{
		hos_launch(cfg_sec);

wrong_emupath:
		if (emummc_path)
		{
			sd_mount();
			emummc_load_cfg(); // Reload emuMMC config in case of emupath.
		}
	}

out:
	free(ments);

	btn_wait();
}

static void _launch_config()
{
	u8 max_entries = 61;
	char *special_path = NULL;
	char *emummc_path  = NULL;

	ment_t *ments      = NULL;
	ini_sec_t *cfg_sec = NULL;

	LIST_INIT(ini_sections);

	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	if (!sd_mount())
		goto parse_failed;

	// Load emuMMC configuration.
	emummc_load_cfg();

	// Parse main configuration.
	ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false);

	// Build configuration menu.
	ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 6));
	ments[0].type    = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type    = MENT_CHGLINE;

	ments[2].type    = MENT_HANDLER;
	ments[2].caption = "Payloads...";
	ments[2].handler = _launch_payloads;

	ments[3].type    = MENT_HANDLER;
	ments[3].caption = "More configs...";
	ments[3].handler = _launch_ini_list;

	ments[4].type    = MENT_CHGLINE;

	u32 sec_idx = 5;
	LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
	{
		if (ini_sec->type == INI_COMMENT ||
			ini_sec->type == INI_NEWLINE ||
			!strcmp(ini_sec->name, "config"))
			continue;

		ments[sec_idx].type    = ini_sec->type;
		ments[sec_idx].caption = ini_sec->name;
		ments[sec_idx].data    = ini_sec;

		if (ini_sec->type == MENT_CAPTION)
			ments[sec_idx].color = ini_sec->color;
		sec_idx++;

		if ((sec_idx - 4) > max_entries)
			break;
	}

	if (sec_idx < 6)
	{
		ments[sec_idx].type    = MENT_CAPTION;
		ments[sec_idx].caption = "No main configs found...";
		ments[sec_idx].color   = TXT_CLR_WARNING;
		sec_idx++;
	}

	memset(&ments[sec_idx], 0, sizeof(ment_t));
	menu_t menu = {
		ments, "Launch configurations", 0, 0
	};

	cfg_sec = (ini_sec_t *)tui_do_menu(&menu);

	special_path = ini_check_special_section(cfg_sec);

	if (cfg_sec && !special_path)
	{
		LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
		{
			if (!strcmp("emummc_force_disable", kv->key))
				h_cfg.emummc_force_disable = atoi(kv->val);
			if (!strcmp("emupath", kv->key))
				emummc_path = kv->val;
		}

		if (emummc_path && !emummc_set_path(emummc_path))
		{
			EPRINTF("emupath is wrong!");
			goto wrong_emupath;
		}
	}

	if (!cfg_sec)
	{
		free(ments);
		sd_end();
		return;
	}

parse_failed:
	if (!cfg_sec)
	{
		gfx_printf("\nPress any key...\n");
		goto out;
	}

	if (special_path)
	{
		// Try to launch Payload or L4T.
		if (special_path != (char *)-1)
			_launch_payload(special_path, false, true);
		else
		{
			u32 entry_idx = 0;
			for (u32 i = 0; i < sec_idx; i++)
			{
				if (ments[i].data == cfg_sec)
				{
					entry_idx = i;
					break;
				}
			}
			launch_l4t(cfg_sec, entry_idx, 0, h_cfg.t210b01);
		}
	}
	else
	{
		hos_launch(cfg_sec);

wrong_emupath:
		if (emummc_path)
		{
			sd_mount();
			emummc_load_cfg(); // Reload emuMMC config in case of emupath.
		}
	}

out:
	sd_end();

	free(ments);

	h_cfg.emummc_force_disable = false;

	btn_wait();
}

#define NYX_VER_OFF 0x9C

static void _nyx_load_run()
{
	u8 *nyx = sd_file_read("bootloader/sys/nyx.bin", NULL);
	if (!nyx)
		return;

	sd_end();

	render_static_bootlogo();
	display_backlight_brightness(h_cfg.backlight, 1000);

	// Check if Nyx version is old.
	u32 expected_nyx_ver = ((NYX_VER_MJ + '0') << 24) | ((NYX_VER_MN + '0') << 16) | ((NYX_VER_HF + '0') << 8);
	u32 nyx_ver = byte_swap_32(*(u32 *)(nyx + NYX_VER_OFF)) & 0xFFFFFF00;
	if (nyx_ver < expected_nyx_ver)
	{
		h_cfg.errors |= ERR_SYSOLD_NYX;

		gfx_con_setpos(0, 0);
		WPRINTF("Old Nyx GUI found! There will be dragons!\n");
		WPRINTF("\nUpdate bootloader folder!\n\n");
		WPRINTF("Press any key...");

		msleep(1000);
		btn_wait();
	}

	// Set hekate errors.
	nyx_str->info.errors = h_cfg.errors;

	// Set Nyx mode.
	nyx_str->cfg = 0;
	if (b_cfg.extra_cfg & EXTRA_CFG_NYX_UMS)
	{
		b_cfg.extra_cfg &= ~(EXTRA_CFG_NYX_UMS);

		nyx_str->cfg |= NYX_CFG_UMS;
		nyx_str->cfg |= b_cfg.ums << 24;
	}

	// Set hekate version used to boot Nyx.
	nyx_str->version = ipl_ver.version - 0x303030; // Convert ASCII to numbers.

	// Set SD card initialization info.
	nyx_str->info.magic   = NYX_NEW_INFO;
	nyx_str->info.sd_init = sd_get_mode();

	// Set SD card error info.
	u16 *sd_errors = sd_get_error_count();
	for (u32 i = 0; i < 3; i++)
		nyx_str->info.sd_errors[i] = sd_errors[i];

	reloc_meta_t *reloc = (reloc_meta_t *)(IPL_LOAD_ADDR + RELOC_META_OFF);
	memcpy((u8 *)nyx_str->hekate, (u8 *)reloc->start, reloc->end - reloc->start);

	// Do one last training.
	minerva_periodic_training();

	bpmp_mmu_disable();
	bpmp_clk_rate_set(BPMP_CLK_NORMAL);

	// Some cards (Sandisk U1), do not like a fast power cycle.
	sdmmc_storage_init_wait_sd();

	void (*nyx_ptr)() = (void *)nyx;
	(*nyx_ptr)();
}

void launch_nyx()
{
	sd_mount();
	_nyx_load_run();
}

static ini_sec_t *_get_ini_sec_from_id(ini_sec_t *ini_sec, char **bootlogoCustomEntry, char **emummc_path)
{
	ini_sec_t *cfg_sec = NULL;

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
	{
		if (!strcmp("id", kv->key))
		{
			if (b_cfg.id[0] && kv->val[0] && !strcmp(b_cfg.id, kv->val))
				cfg_sec = ini_sec;
			else
				break;
		}
		if (!strcmp("emupath", kv->key))
			*emummc_path = kv->val;
		else if (!strcmp("logopath", kv->key))
			*bootlogoCustomEntry = kv->val;
		else if (!strcmp("emummc_force_disable", kv->key))
			h_cfg.emummc_force_disable = atoi(kv->val);
	}
	if (!cfg_sec)
	{
		*emummc_path               = NULL;
		*bootlogoCustomEntry       = NULL;
		h_cfg.emummc_force_disable = false;
	}

	return cfg_sec;
}

static void _bootloader_corruption_protect()
{
	FILINFO fno;
	if (!f_stat("bootloader", &fno))
	{
		if (!h_cfg.bootprotect && (fno.fattrib & AM_ARC))
			f_chmod("bootloader", 0,      AM_ARC);
		else if (h_cfg.bootprotect && !(fno.fattrib & AM_ARC))
			f_chmod("bootloader", AM_ARC, AM_ARC);
	}
}

static void _check_for_updated_bootloader()
{
	// Check if already chainloaded update and clear flag. Otherwise check for updates.
	if (EMC(EMC_SCRATCH0) & EMC_HEKA_UPD)
		EMC(EMC_SCRATCH0) &= ~EMC_HEKA_UPD;
	else
	{
		// Check if update.bin exists and is newer and launch it. Otherwise create it.
		if (!f_stat("bootloader/update.bin", NULL))
			_launch_payload("bootloader/update.bin", true, false);
		else
		{
			u8 *buf = zalloc(0x200);
			is_ipl_updated(buf, "bootloader/update.bin", true);
			free(buf);
		}
	}
}

static void _auto_launch()
{
	struct _bmp_data
	{
		u32 size;
		u32 size_x;
		u32 size_y;
		u32 offset;
		u32 pos_x;
		u32 pos_y;
	};

	u32 boot_wait             = h_cfg.bootwait;
	u32 boot_entry_id         = 0;
	ini_sec_t *cfg_sec        = NULL;
	char *emummc_path         = NULL;
	char *bootlogoCustomEntry = NULL;
	bool  config_entry_found  = false;

	bool boot_from_id = (b_cfg.boot_cfg & BOOT_CFG_FROM_ID) && (b_cfg.boot_cfg & BOOT_CFG_AUTOBOOT_EN);
	if (boot_from_id)
		b_cfg.id[7] = 0;

	if (!(b_cfg.boot_cfg & BOOT_CFG_FROM_LAUNCH))
		gfx_con.mute = true;

	LIST_INIT(ini_sections);
	LIST_INIT(ini_list_sections);

	// Load emuMMC configuration.
	emummc_load_cfg();

	// Parse hekate main configuration.
	if (!ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false))
		goto out; // Can't load hekate_ipl.ini.

	// Load configuration.
	LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
	{
		// Skip other ini entries for autoboot.
		if (ini_sec->type == INI_CHOICE)
		{
			if (!config_entry_found && !strcmp(ini_sec->name, "config"))
			{
				config_entry_found = true;
				LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
				{
					if      (!strcmp("autoboot",      kv->key))
						h_cfg.autoboot = atoi(kv->val);
					else if (!strcmp("autoboot_list", kv->key))
						h_cfg.autoboot_list = atoi(kv->val);
					else if (!strcmp("bootwait",      kv->key))
						boot_wait = atoi(kv->val);
					else if (!strcmp("backlight",     kv->key))
						h_cfg.backlight   = atoi(kv->val);
					else if (!strcmp("noticker",      kv->key))
						h_cfg.noticker    = atoi(kv->val);
					else if (!strcmp("autohosoff",    kv->key))
						h_cfg.autohosoff  = atoi(kv->val);
					else if (!strcmp("autonogc",      kv->key))
						h_cfg.autonogc    = atoi(kv->val);
					else if (!strcmp("updater2p",     kv->key))
						h_cfg.updater2p   = atoi(kv->val);
					else if (!strcmp("bootprotect",   kv->key))
						h_cfg.bootprotect = atoi(kv->val);
				}
				boot_entry_id++;

				// Override autoboot.
				if (b_cfg.boot_cfg & BOOT_CFG_AUTOBOOT_EN)
				{
					h_cfg.autoboot      = b_cfg.autoboot;
					h_cfg.autoboot_list = b_cfg.autoboot_list;
				}

				// Apply bootloader protection against corruption.
				_bootloader_corruption_protect();

				// If ini list, exit here.
				if (!boot_from_id && h_cfg.autoboot_list)
					break;

				continue;
			}

			if (boot_from_id)
				cfg_sec = _get_ini_sec_from_id(ini_sec, &bootlogoCustomEntry, &emummc_path);
			else if (h_cfg.autoboot == boot_entry_id && config_entry_found)
			{
				cfg_sec = ini_sec;
				LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
				{
					if (!strcmp("logopath", kv->key))
						bootlogoCustomEntry = kv->val;
					else if (!strcmp("emupath", kv->key))
						emummc_path = kv->val;
					else if (!strcmp("emummc_force_disable", kv->key))
						h_cfg.emummc_force_disable = atoi(kv->val);
					else if (!strcmp("bootwait", kv->key))
						boot_wait = atoi(kv->val);
				}
			}
			if (cfg_sec)
				break;
			boot_entry_id++;
		}
	}

	if (h_cfg.autohosoff && !(b_cfg.boot_cfg & BOOT_CFG_AUTOBOOT_EN))
		_check_power_off_from_hos();

	if (h_cfg.autoboot_list || (boot_from_id && !cfg_sec))
	{
		if (boot_from_id && cfg_sec)
			goto skip_list;

		cfg_sec = NULL;
		boot_entry_id = 1;
		bootlogoCustomEntry = NULL;

		if (!ini_parse(&ini_list_sections, "bootloader/ini", true))
			goto skip_list;

		LIST_FOREACH_ENTRY(ini_sec_t, ini_sec_list, &ini_list_sections, link)
		{
			if (ini_sec_list->type != INI_CHOICE)
				continue;

			if (!strcmp(ini_sec_list->name, "config"))
				continue;

			if (boot_from_id)
				cfg_sec = _get_ini_sec_from_id(ini_sec_list, &bootlogoCustomEntry, &emummc_path);
			else if (h_cfg.autoboot == boot_entry_id)
			{
				h_cfg.emummc_force_disable = false;
				cfg_sec = ini_sec_list;
				LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
				{
					if (!strcmp("logopath", kv->key))
						bootlogoCustomEntry = kv->val;
					else if (!strcmp("emupath", kv->key))
						emummc_path = kv->val;
					else if (!strcmp("emummc_force_disable", kv->key))
						h_cfg.emummc_force_disable = atoi(kv->val);
					else if (!strcmp("bootwait", kv->key))
						boot_wait = atoi(kv->val);
				}
			}
			if (cfg_sec)
				break;
			boot_entry_id++;
		}
	}

skip_list:
	if (!cfg_sec)
		goto out; // No configurations or auto boot is disabled.

	// Check if entry is payload or l4t special case.
	char *special_path = ini_check_special_section(cfg_sec);

	if ((!(b_cfg.boot_cfg & BOOT_CFG_FROM_LAUNCH) && boot_wait) || // Conditional for HOS/Payload.
		(special_path && special_path == (char *)-1))              // Always show for L4T.
	{
		u32 fsize;
		u8 *logo_buf = NULL;
		u8 *bitmap = NULL;
		struct _bmp_data bmpData;
		bool bootlogoFound = false;

		// Check if user set custom logo path at the boot entry.
		if (bootlogoCustomEntry)
			bitmap = (u8 *)sd_file_read(bootlogoCustomEntry, &fsize);

		// Custom entry bootlogo not found, trying default custom one.
		if (!bitmap)
			bitmap = (u8 *)sd_file_read("bootloader/bootlogo.bmp", &fsize);

		if (bitmap)
		{
			// Get values manually to avoid unaligned access.
			bmpData.size = bitmap[2] | bitmap[3] << 8 |
				bitmap[4] << 16 | bitmap[5] << 24;
			bmpData.offset = bitmap[10] | bitmap[11] << 8 |
				bitmap[12] << 16 | bitmap[13] << 24;
			bmpData.size_x = bitmap[18] | bitmap[19] << 8 |
				bitmap[20] << 16 | bitmap[21] << 24;
			bmpData.size_y = bitmap[22] | bitmap[23] << 8 |
				bitmap[24] << 16 | bitmap[25] << 24;
			// Sanity check.
			if (bitmap[0] == 'B' &&
				bitmap[1] == 'M' &&
				bitmap[28] == 32 && // Only 32 bit BMPs allowed.
				bmpData.size_x <= 720 &&
				bmpData.size_y <= 1280)
			{
				if (bmpData.size <= fsize && ((bmpData.size - bmpData.offset) < SZ_4M))
				{
					// Avoid unaligned access from BM 2-byte MAGIC and remove header.
					logo_buf = (u8 *)malloc(SZ_4M);
					memcpy(logo_buf, bitmap + bmpData.offset, bmpData.size - bmpData.offset);
					free(bitmap);
					// Center logo if res < 720x1280.
					bmpData.pos_x = (720  - bmpData.size_x) >> 1;
					bmpData.pos_y = (1280 - bmpData.size_y) >> 1;
					// Get background color from 1st pixel.
					if (bmpData.size_x < 720 || bmpData.size_y < 1280)
						gfx_clear_color(*(u32 *)logo_buf);

					bootlogoFound = true;
				}
			}
			else
				free(bitmap);
		}

		// Clamp value to default if it exceeds 20s to protect against corruption.
		if (boot_wait > 20)
			boot_wait = 3;

		// Render boot logo.
		if (bootlogoFound)
		{
			gfx_render_bmp_argb((u32 *)logo_buf, bmpData.size_x, bmpData.size_y,
				bmpData.pos_x, bmpData.pos_y);
			free(logo_buf);

			// Do animated waiting before booting. If VOL- is pressed go into bootloader menu.
			if (render_ticker(boot_wait, h_cfg.backlight, h_cfg.noticker))
				goto out;
		}
		else
		{
			// Do animated waiting before booting. If VOL- is pressed go into bootloader menu.
			if (render_ticker_logo(boot_wait, h_cfg.backlight))
				goto out;
		}
	}

	if (b_cfg.boot_cfg & BOOT_CFG_FROM_LAUNCH)
		display_backlight_brightness(h_cfg.backlight, 0);
	else if (btn_read_vol() == BTN_VOL_DOWN) // 0s bootwait VOL- check.
		goto out;

	if (special_path)
	{
		// Try to launch Payload or L4T.
		if (special_path != (char *)-1)
			_launch_payload(special_path, false, false);
		else
			launch_l4t(cfg_sec, h_cfg.autoboot, h_cfg.autoboot_list, h_cfg.t210b01);
		goto error;
	}
	else
	{
		if (b_cfg.boot_cfg & BOOT_CFG_TO_EMUMMC)
			emummc_set_path(b_cfg.emummc_path);
		else if (emummc_path && !emummc_set_path(emummc_path))
		{
			gfx_con.mute = false;
			EPRINTF("emupath is wrong!");
			goto wrong_emupath;
		}

		hos_launch(cfg_sec);

wrong_emupath:
		if (emummc_path || b_cfg.boot_cfg & BOOT_CFG_TO_EMUMMC)
		{
			sd_mount();
			emummc_load_cfg(); // Reload emuMMC config in case of emupath.
		}

error:
		gfx_con.mute = false;
		gfx_printf("\nPress any key...\n");
		display_backlight_brightness(h_cfg.backlight, 1000);
		msleep(500);
		btn_wait();
	}

out:
	gfx_con.mute = false;

	// Clear boot reasons from binary.
	if (b_cfg.boot_cfg & (BOOT_CFG_FROM_ID | BOOT_CFG_TO_EMUMMC))
		memset(b_cfg.xt_str, 0, sizeof(b_cfg.xt_str));
	h_cfg.emummc_force_disable = false;

	// L4T: Clear custom boot mode flags from PMC_SCRATCH0.
	PMC(APBDEV_PMC_SCRATCH0) &= ~PMC_SCRATCH0_MODE_CUSTOM_ALL;

	_nyx_load_run();
}

#define EXCP_EN_ADDR   0x4003FFFC
#define  EXCP_MAGIC       0x30505645 // "EVP0".
#define EXCP_TYPE_ADDR 0x4003FFF8
#define  EXCP_TYPE_RESET  0x545352   // "RST".
#define  EXCP_TYPE_UNDEF  0x464455   // "UDF".
#define  EXCP_TYPE_PABRT  0x54424150 // "PABT".
#define  EXCP_TYPE_DABRT  0x54424144 // "DABT".
#define  EXCP_TYPE_WDT    0x544457   // "WDT".
#define EXCP_LR_ADDR   0x4003FFF4

#define PSTORE_LOG_OFFSET 0x180000
#define PSTORE_RAM_SIG    0x43474244 // "DBGC".

typedef struct _pstore_buf {
	u32 sig;
	u32 start;
	u32 size;
} pstore_buf_t;

static void _show_errors()
{
	u32 *excp_lr = (u32 *)EXCP_LR_ADDR;
	u32 *excp_type = (u32 *)EXCP_TYPE_ADDR;
	u32 *excp_enabled = (u32 *)EXCP_EN_ADDR;

	u32 panic_status = hw_rst_status & 0xFFFFF;

	// Check for exception error.
	if (*excp_enabled == EXCP_MAGIC)
		h_cfg.errors |= ERR_EXCEPTION;

	// Check for L4T kernel panic.
	if (PMC(APBDEV_PMC_SCRATCH37) == PMC_SCRATCH37_KERNEL_PANIC_MAGIC)
	{
		// Set error and clear flag.
		h_cfg.errors |= ERR_L4T_KERNEL;
		PMC(APBDEV_PMC_SCRATCH37) = 0;
	}

	// Check for watchdog panic.
	if (hw_rst_reason == PMC_RST_STATUS_WATCHDOG && panic_status &&
		panic_status <= 0xFF && panic_status != 0x20 && panic_status != 0x21)
	{
		h_cfg.errors |= ERR_PANIC_CODE;
	}

	// Check if we had a panic while in CFW.
	secmon_exo_check_panic();

	// Handle errors.
	if (h_cfg.errors)
	{
		gfx_clear_grey(0x1B);
		gfx_con_setpos(0, 0);
		display_backlight_brightness(150, 1000);

		if (h_cfg.errors & ERR_SD_BOOT_EN)
		{
			WPRINTF("Failed to init or mount SD!\n");

			// Clear the module bits as to not cram the error screen.
			h_cfg.errors &= ~(ERR_LIBSYS_LP0 | ERR_LIBSYS_MTC);
		}

		if (h_cfg.errors & ERR_LIBSYS_LP0)
			WPRINTF("Missing LP0 (sleep) lib!\n");
		if (h_cfg.errors & ERR_LIBSYS_MTC)
			WPRINTF("Missing Minerva lib!\n");

		if (h_cfg.errors & (ERR_LIBSYS_LP0 | ERR_LIBSYS_MTC))
			WPRINTF("\nUpdate bootloader folder!\n\n");

		if (h_cfg.errors & ERR_EXCEPTION)
		{
			WPRINTFARGS("hekate exception occurred (LR %08X):\n", *excp_lr);
			switch (*excp_type)
			{
			case EXCP_TYPE_WDT:
				WPRINTF("Hang detected in LP0/Minerva!");
				break;
			case EXCP_TYPE_RESET:
				WPRINTF("RESET");
				break;
			case EXCP_TYPE_UNDEF:
				WPRINTF("UNDEF");
				break;
			case EXCP_TYPE_PABRT:
				WPRINTF("PABRT");
				break;
			case EXCP_TYPE_DABRT:
				WPRINTF("DABRT");
				break;
			}
			gfx_puts("\n");

			// Clear the exception.
			*excp_enabled = 0;
			*excp_type = 0;
		}

		if (h_cfg.errors & ERR_L4T_KERNEL)
		{
			WPRINTF("L4T Kernel panic occurred!\n");
			if (!(h_cfg.errors & ERR_SD_BOOT_EN))
			{
				if (!sd_save_to_file((void *)PSTORE_ADDR, PSTORE_SZ, "L4T_panic.bin"))
					WPRINTF("PSTORE saved to L4T_panic.bin");
				pstore_buf_t *buf = (pstore_buf_t *)(PSTORE_ADDR + PSTORE_LOG_OFFSET);
				if (buf->sig == PSTORE_RAM_SIG && buf->size && buf->size < 0x80000)
				{
					u32 log_offset = PSTORE_ADDR + PSTORE_LOG_OFFSET + sizeof(pstore_buf_t);
					if (!sd_save_to_file((void *)log_offset, buf->size, "L4T_panic.txt"))
						WPRINTF("Log saved to L4T_panic.txt");
				}
			}
			gfx_puts("\n");
		}

		if (h_cfg.errors & ERR_PANIC_CODE)
		{
			u32 r = (hw_rst_status >> 20) & 0xF;
			u32 g = (hw_rst_status >> 24) & 0xF;
			u32 b = (hw_rst_status >> 28) & 0xF;
			r = (r << 16) | (r << 20);
			g = (g << 8)  | (g << 12);
			b = (b << 0)  | (b << 4);
			u32 color = r | g | b;

			WPRINTF("HOS panic occurred!\n");
			gfx_printf("Color: %k####%k, Code: %02X\n\n", color, TXT_CLR_DEFAULT, panic_status);
		}

		WPRINTF("Press any key...");

		msleep(1000); // Guard against injection VOL+.
		btn_wait();
		msleep(500);  // Guard against force menu VOL-.
	}
}

static void _check_low_battery()
{
	if (h_cfg.devmode)
		goto out;

	int enough_battery;
	int batt_volt = 0;
	int charge_status = 0;

	// Enable charger in case it's disabled.
	bq24193_enable_charger();

	bq24193_get_property(BQ24193_ChargeStatus, &charge_status);
	max17050_get_property(MAX17050_AvgVCELL,   &batt_volt);

	enough_battery = charge_status ? 3300 : 3100;

	// If battery voltage is enough, exit.
	if (batt_volt > enough_battery || !batt_volt)
		goto out;

	// Prepare battery icon resources.
	u8 *battery_res = malloc(ALIGN(BATTERY_EMPTY_SIZE, SZ_4K));
	blz_uncompress_srcdest(battery_icons_blz, BATTERY_EMPTY_BLZ_SIZE, battery_res, BATTERY_EMPTY_SIZE);

	u8 *battery_icon     = malloc(0x95A); // 21x38x3
	u8 *charging_icon    = malloc(0x2F4); // 21x12x3
	u8 *no_charging_icon = zalloc(0x2F4);

	memcpy(charging_icon, battery_res, 0x2F4);
	memcpy(battery_icon, battery_res + 0x2F4, 0x95A);

	u32 battery_icon_y_pos  = 1280 - 16 - BATTERY_EMPTY_BATT_HEIGHT;
	u32 charging_icon_y_pos = 1280 - 16 - BATTERY_EMPTY_BATT_HEIGHT - 12 - BATTERY_EMPTY_CHRG_HEIGHT;
	free(battery_res);

	charge_status = !charge_status;

	u32 timer = 0;
	bool screen_on = false;
	while (true)
	{
		bpmp_msleep(250);

		// Refresh battery stats.
		int current_charge_status = 0;
		bq24193_get_property(BQ24193_ChargeStatus, &current_charge_status);
		max17050_get_property(MAX17050_AvgVCELL, &batt_volt);
		enough_battery = current_charge_status ? 3300 : 3100;

		// If battery voltage is enough, exit.
		if (batt_volt > enough_battery)
			break;

		// Refresh charging icon.
		if (screen_on && (charge_status != current_charge_status))
		{
			if (current_charge_status)
				gfx_set_rect_rgb(charging_icon,    BATTERY_EMPTY_WIDTH, BATTERY_EMPTY_CHRG_HEIGHT, 16, charging_icon_y_pos);
			else
				gfx_set_rect_rgb(no_charging_icon, BATTERY_EMPTY_WIDTH, BATTERY_EMPTY_CHRG_HEIGHT, 16, charging_icon_y_pos);
		}

		// Check if it's time to turn off display.
		if (screen_on && timer < get_tmr_ms())
		{
			// If battery is not charging, power off.
			if (!current_charge_status)
			{
				max77620_low_battery_monitor_config(true);

				// Handle full hw deinit and power off.
				power_set_state(POWER_OFF_RESET);
			}

			// If charging, just disable display.
			display_end();
			screen_on = false;
		}

		// Check if charging status changed or Power button was pressed and enable display.
		if ((charge_status != current_charge_status) || (btn_wait_timeout_single(0, BTN_POWER) & BTN_POWER))
		{
			if (!screen_on)
			{
				display_init();
				u32 *fb = display_init_window_a_pitch();
				gfx_init_ctxt(fb, 720, 1280, 720);

				gfx_set_rect_rgb(battery_icon,         BATTERY_EMPTY_WIDTH, BATTERY_EMPTY_BATT_HEIGHT, 16, battery_icon_y_pos);
				if (current_charge_status)
					gfx_set_rect_rgb(charging_icon,    BATTERY_EMPTY_WIDTH, BATTERY_EMPTY_CHRG_HEIGHT, 16, charging_icon_y_pos);
				else
					gfx_set_rect_rgb(no_charging_icon, BATTERY_EMPTY_WIDTH, BATTERY_EMPTY_CHRG_HEIGHT, 16, charging_icon_y_pos);

				display_backlight_pwm_init();
				display_backlight_brightness(100, 1000);

				screen_on = true;
			}

			timer = get_tmr_ms() + 15000;
		}

		// Check if forcefully continuing.
		if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			break;

		charge_status = current_charge_status;
	}

	if (screen_on)
		display_end();

	free(battery_icon);
	free(charging_icon);
	free(no_charging_icon);

out:
	// Re enable Low Battery Monitor shutdown.
	max77620_low_battery_monitor_config(true);
}

static void _r2c_get_config_t210b01()
{
	rtc_reboot_reason_t rr;
	if (!max77620_rtc_get_reboot_reason(&rr))
		return;

	// Check if reason is actually set.
	if (rr.dec.reason != REBOOT_REASON_NOP)
	{
		// Clear boot storage.
		memset(&b_cfg, 0, sizeof(boot_cfg_t));

		// Enable boot storage.
		b_cfg.boot_cfg |= BOOT_CFG_AUTOBOOT_EN;
	}

	switch (rr.dec.reason)
	{
	case REBOOT_REASON_NOP:
		break;
	case REBOOT_REASON_REC:
		PMC(APBDEV_PMC_SCRATCH0) |= PMC_SCRATCH0_MODE_RECOVERY;
	case REBOOT_REASON_SELF:
		b_cfg.autoboot      = rr.dec.autoboot_idx;
		b_cfg.autoboot_list = rr.dec.autoboot_list;
		break;
	case REBOOT_REASON_MENU:
		break;
	case REBOOT_REASON_UMS:
		b_cfg.extra_cfg |= EXTRA_CFG_NYX_UMS;
		b_cfg.ums = rr.dec.ums_idx;
		break;
	case REBOOT_REASON_PANIC:
		PMC(APBDEV_PMC_SCRATCH37) = PMC_SCRATCH37_KERNEL_PANIC_MAGIC;
		break;
	}
}

static void _ipl_reload()
{
	hw_deinit(false, 0);

	// Reload hekate.
	void (*ipl_ptr)() = (void *)IPL_LOAD_ADDR;
	(*ipl_ptr)();
}

static void _about()
{
	static const char credits[] =
		"\nhekate   (c) 2018,      naehrwert, st4rk\n\n"
		"         (c) 2018-2025, CTCaer\n\n"
		" ___________________________________________\n\n"
		"Thanks to: %kderrek, nedwill, plutoo,\n"
		"           shuffle2, smea, thexyz, yellows8%k\n"
		" ___________________________________________\n\n"
		"Greetings to: fincs, hexkyz, SciresM,\n"
		"              Shiny Quagsire, WinterMute\n"
		" ___________________________________________\n\n"
		"Open source and free packages used:\n\n"
		" - FatFs R0.13c\n"
		"   (c) 2006-2018, ChaN\n"
		"   (c) 2018-2022, CTCaer\n\n"
		" - bcl-1.2.0\n"
		"   (c) 2003-2006, Marcus Geelnard\n\n"
		" - blz\n"
		"   (c) 2018, SciresM\n\n"
		" - elfload\n"
		"   (c) 2014, Owen Shepherd\n"
		"   (c) 2018, M4xw\n"
		" ___________________________________________\n\n";
	static const char octopus[] =
		"                         %k___\n"
		"                      .-'   `'.\n"
		"                     /         \\\n"
		"                     |         ;\n"
		"                     |         |           ___.--,\n"
		"            _.._     |0) = (0) |    _.---'`__.-( (_.\n"
		"     __.--'`_.. '.__.\\    '--. \\_.-' ,.--'`     `\"\"`\n"
		"    ( ,.--'`   ',__ /./;   ;, '.__.'`    __\n"
		"    _`) )  .---.__.' / |   |\\   \\__..--\"\"  \"\"\"--.,_\n"
		"   `---' .'.''-._.-'`_./  /\\ '.  \\ _.--''````'''--._`-.__.'\n"
		"         | |  .' _.-' |  |  \\  \\  '.               `----`\n"
		"          \\ \\/ .'     \\  \\   '. '-._)\n"
		"           \\/ /        \\  \\    `=.__`'-.\n"
		"           / /\\         `) )    / / `\"\".`\\\n"
		"     , _.-'.'\\ \\        / /    ( (     / /\n"
		"      `--'`   ) )    .-'.'      '.'.  | (\n"
		"             (/`    ( (`          ) )  '-;   %k[switchbrew]%k\n"
		"              `      '-;         (-'%k";

	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	gfx_printf(credits, TXT_CLR_CYAN_L, TXT_CLR_DEFAULT);
	gfx_con.fntsz = 8;
	gfx_printf(octopus, TXT_CLR_CYAN_L, TXT_CLR_TURQUOISE, TXT_CLR_CYAN_L, TXT_CLR_DEFAULT);

	btn_wait();
}

ment_t ment_cinfo[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- SoC Info ----", TXT_CLR_CYAN_L),
	MDEF_HANDLER("Fuses", print_fuseinfo),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- Storage Info --", TXT_CLR_CYAN_L),
	MDEF_HANDLER("eMMC",    print_mmc_info),
	MDEF_HANDLER("SD Card", print_sdcard_info),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Misc ------", TXT_CLR_CYAN_L),
	MDEF_HANDLER("Battery", print_battery_info),
	MDEF_END()
};

menu_t menu_cinfo = { ment_cinfo, "Console Info", 0, 0 };

ment_t ment_tools[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-------- Other -------", TXT_CLR_WARNING),
	MDEF_HANDLER("AutoRCM", menu_autorcm),
	MDEF_END()
};

menu_t menu_tools = { ment_tools, "Tools", 0, 0 };

power_state_t STATE_POWER_OFF           = POWER_OFF_RESET;
power_state_t STATE_REBOOT_RCM          = REBOOT_RCM;
power_state_t STATE_REBOOT_BYPASS_FUSES = REBOOT_BYPASS_FUSES;

ment_t ment_top[] = {
	MDEF_HANDLER("Launch", _launch_config),
	MDEF_CAPTION("---------------", TXT_CLR_GREY_DM),
	MDEF_MENU("Tools",        &menu_tools),
	MDEF_MENU("Console info", &menu_cinfo),
	MDEF_CAPTION("---------------", TXT_CLR_GREY_DM),
	MDEF_HANDLER("Reload", _ipl_reload),
	MDEF_HANDLER("Load Nyx", launch_nyx),
	MDEF_HANDLER_EX("Reboot (OFW)", &STATE_REBOOT_BYPASS_FUSES, power_set_state_ex),
	MDEF_HANDLER_EX("Reboot (RCM)", &STATE_REBOOT_RCM,          power_set_state_ex),
	MDEF_HANDLER_EX("Power off",    &STATE_POWER_OFF,           power_set_state_ex),
	MDEF_CAPTION("---------------", TXT_CLR_GREY_DM),
	MDEF_HANDLER("About", _about),
	MDEF_END()
};

menu_t menu_top = { ment_top, "hekate v6.4.1", 0, 0 };

extern void pivot_stack(u32 stack_top);

void ipl_main()
{
	// Do initial HW configuration. This is compatible with consecutive reruns without a reset.
	hw_init();

	// Pivot the stack under IPL. (Only max 4KB is needed).
	pivot_stack(IPL_LOAD_ADDR);

	// Place heap at a place outside of L4T/HOS configuration and binaries.
	heap_init((void *)IPL_HEAP_START);

#ifdef DEBUG_UART_PORT
	uart_send(DEBUG_UART_PORT, (u8 *)"hekate: Hello!\r\n", 16);
	uart_wait_xfer(DEBUG_UART_PORT, UART_TX_IDLE);
#endif

	// Set bootloader's default configuration.
	set_default_configuration();

	// Check if battery is enough.
	_check_low_battery();

	// Prep RTC regs for read. Needed for T210B01 R2C.
	max77620_rtc_prep_read();

	// Initialize display.
	display_init();

	// Overclock BPMP.
	bpmp_clk_rate_set(h_cfg.t210b01 ? BPMP_CLK_DEFAULT_BOOST : BPMP_CLK_LOWER_BOOST);

	// Mount SD Card.
	h_cfg.errors |= !sd_mount() ? ERR_SD_BOOT_EN : 0;

	// Check if watchdog was fired previously.
	if (watchdog_fired())
		goto skip_lp0_minerva_config;

	// Enable watchdog protection to avoid SD corruption based hanging in LP0/Minerva config.
	watchdog_start(5000000 / 2, TIMER_FIQENABL_EN); // 5 seconds.

	// Save sdram lp0 config.
	void *sdram_params = h_cfg.t210b01 ? sdram_get_params_t210b01() : sdram_get_params_patched();
	if (!ianos_loader("bootloader/sys/libsys_lp0.bso", DRAM_LIB, sdram_params))
		h_cfg.errors |= ERR_LIBSYS_LP0;

	// Train DRAM and switch to max frequency.
	if (minerva_init()) //!TODO: Add Tegra210B01 support to minerva.
		h_cfg.errors |= ERR_LIBSYS_MTC;

	// Disable watchdog protection.
	watchdog_end();

skip_lp0_minerva_config:
	// Initialize display window, backlight and gfx console.
	u32 *fb = display_init_window_a_pitch();
	gfx_init_ctxt(fb, 720, 1280, 720);
	gfx_con_init();

	// Initialize backlight PWM.
	display_backlight_pwm_init();
	//display_backlight_brightness(h_cfg.backlight, 1000);

	// Get R2C config from RTC.
	if (h_cfg.t210b01)
		_r2c_get_config_t210b01();

	// Show exceptions, HOS errors, library errors and L4T kernel panics.
	_show_errors();

	// Load saved configuration and auto boot if enabled.
	if (!(h_cfg.errors & ERR_SD_BOOT_EN))
	{
		_check_for_updated_bootloader();
		_auto_launch();
	}

	// Failed to launch Nyx, unmount SD Card.
	sd_end();

	// Set ram to a freq that doesn't need periodic training.
	minerva_change_freq(FREQ_800);

	while (true)
		tui_do_menu(&menu_top);

	// Halt BPMP if we managed to get out of execution.
	while (true)
		bpmp_halt();
}
