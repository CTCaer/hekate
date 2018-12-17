/*
 * Copyright (c) 2018 naehrwert
 *
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

#include "config/config.h"
#include "gfx/di.h"
#include "gfx/gfx.h"
#include "gfx/logos.h"
#include "gfx/tui.h"
#include "hos/hos.h"
#include "ianos/ianos.h"
#include "libs/compr/blz.h"
#include "libs/fatfs/ff.h"
#include "mem/heap.h"
#include "mem/sdram.h"
#include "power/max77620.h"
#include "soc/hw_init.h"
#include "soc/i2c.h"
#include "soc/pmc.h"
#include "soc/t210.h"
#include "soc/uart.h"
#include "storage/sdmmc.h"
#include "utils/btn.h"
#include "utils/dirlist.h"
#include "utils/list.h"
#include "utils/util.h"

#include "frontend/fe_emmc_tools.h"
#include "frontend/fe_tools.h"
#include "frontend/fe_info.h"

//TODO: ugly.
gfx_ctxt_t gfx_ctxt;
gfx_con_t gfx_con;

//TODO: Create more macros (info, header, debug, etc) with different colors and utilize them for consistency.
#define EPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, 0xFFCCCCCC)
#define EPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, args, 0xFFCCCCCC)
#define WPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFFDD00, 0xFFCCCCCC)
#define WPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFFDD00, args, 0xFFCCCCCC)

//TODO: ugly.
sdmmc_t sd_sdmmc;
sdmmc_storage_t sd_storage;
FATFS sd_fs;
static bool sd_mounted;

#ifdef MENU_LOGO_ENABLE
u8 *Kc_MENU_LOGO;
#endif //MENU_LOGO_ENABLE

hekate_config h_cfg;

bool sd_mount()
{
	if (sd_mounted)
		return true;

	if (!sdmmc_storage_init_sd(&sd_storage, &sd_sdmmc, SDMMC_1, SDMMC_BUS_WIDTH_4, 11))
	{
		EPRINTF("Failed to init SD card.\nMake sure that it is inserted.\nOr that SD reader is properly seated!");
	}
	else
	{
		int res = 0;
		res = f_mount(&sd_fs, "", 1);
		if (res == FR_OK)
		{
			sd_mounted = 1;
			return true;
		}
		else
		{
			EPRINTFARGS("Failed to mount SD card (FatFS Error %d).\nMake sure that a FAT partition exists..", res);
		}
	}

	return false;
}

void sd_unmount()
{
	if (sd_mounted)
	{
		f_mount(NULL, "", 1);
		sdmmc_storage_end(&sd_storage);
		sd_mounted = false;
	}
}

void *sd_file_read(char *path)
{
	FIL fp;
	if (f_open(&fp, path, FA_READ) != FR_OK)
		return NULL;

	u32 size = f_size(&fp);
	void *buf = malloc(size);

	u8 *ptr = buf;
	while (size > 0)
	{
		u32 rsize = MIN(size, 512 * 512);
		if (f_read(&fp, ptr, rsize, NULL) != FR_OK)
		{
			free(buf);
			return NULL;
		}

		ptr += rsize;
		size -= rsize;
	}

	f_close(&fp);

	return buf;
}

int sd_save_to_file(void *buf, u32 size, const char *filename)
{
	FIL fp;
	u32 res = 0;
	res = f_open(&fp, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res)
	{
		EPRINTFARGS("Error (%d) creating file\n%s.\n", res, filename);
		return 1;
	}

	f_sync(&fp);
	f_write(&fp, buf, size, NULL);
	f_close(&fp);

	return 0;
}

void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage)
{
	sdmmc_storage_t storage2;
	sdmmc_t sdmmc;
	char emmcSN[9];
	bool init_done = false;

	memcpy(path, "backup", 7);
	f_mkdir(path);

	if (!storage)
	{
		if (!sdmmc_storage_init_mmc(&storage2, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
			memcpy(emmcSN, "00000000", 9);
		else
		{
			init_done = true;
			itoa(storage2.cid.serial, emmcSN, 16);
		}
	}
	else
		itoa(storage->cid.serial, emmcSN, 16);

	u32 sub_dir_len = strlen(sub_dir);   // Can be a null-terminator.
	u32 filename_len = strlen(filename); // Can be a null-terminator.

	memcpy(path + strlen(path), "/", 2);
	memcpy(path + strlen(path), emmcSN, 9);
	f_mkdir(path);
	memcpy(path + strlen(path), sub_dir, sub_dir_len + 1);
	if (sub_dir_len)
		f_mkdir(path);
	memcpy(path + strlen(path), "/", 2);
	memcpy(path + strlen(path), filename, filename_len + 1);

	if (init_done)
		sdmmc_storage_end(&storage2);
}

void panic(u32 val)
{
	// Set panic code.
	PMC(APBDEV_PMC_SCRATCH200) = val;
	//PMC(APBDEV_PMC_CRYPTO_OP) = 1; // Disable SE.
	TMR(TIMER_WDT4_UNLOCK_PATTERN) = TIMER_MAGIC_PTRN;
	TMR(TIMER_TMR9_TMR_PTV) = TIMER_EN | TIMER_PER_EN;
	TMR(TIMER_WDT4_CONFIG)  = TIMER_SRC(9) | TIMER_PER(1) | TIMER_PMCRESET_EN;
	TMR(TIMER_WDT4_COMMAND) = TIMER_START_CNT;
	while (1)
		;
}

void reboot_normal()
{
	sd_unmount();
#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	display_end();
	panic(0x21); // Bypass fuse programming in package1.
}

void reboot_rcm()
{
	sd_unmount();
#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	display_end();
	PMC(APBDEV_PMC_SCRATCH0) = 2; // Reboot into rcm.
	PMC(APBDEV_PMC_CNTRL) |= PMC_CNTRL_MAIN_RST;
	while (true)
		usleep(1);
}

void power_off()
{
	sd_unmount();
#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	//TODO: we should probably make sure all regulators are powered off properly.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_PWR_OFF);
}

void check_power_off_from_hos()
{
	// Power off on AutoRCM wakeup from HOS shutdown. For modchips/dongles.
	u8 hosWakeup = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_IRQTOP);
	if (hosWakeup & MAX77620_IRQ_TOP_RTC_MASK)
	{
		sd_unmount();
		if (h_cfg.autohosoff == 1)
		{
			gfx_clear_grey(&gfx_ctxt, 0x1B);
			u8 *BOOTLOGO = (void *)malloc(0x4000);
			blz_uncompress_srcdest(BOOTLOGO_BLZ, SZ_BOOTLOGO_BLZ, BOOTLOGO, SZ_BOOTLOGO);
			gfx_set_rect_grey(&gfx_ctxt, BOOTLOGO, X_BOOTLOGO, Y_BOOTLOGO, 326, 544);

			display_backlight_brightness(10, 5000);
			display_backlight_brightness(100, 25000);
			usleep(600000);
			display_backlight_brightness(0, 20000);
		}
		power_off();
	}
}

// This is a safe and unused DRAM region for our payloads.
#define IPL_LOAD_ADDR      0x40008000
#define EXT_PAYLOAD_ADDR   0xC03C0000
#define PATCHED_RELOC_SZ   0x94
#define RCM_PAYLOAD_ADDR   (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
#define PAYLOAD_ENTRY      0x40010000
#define CBFS_SDRAM_EN_ADDR 0x4003e000
#define COREBOOT_ADDR      (0xD0000000 - 0x100000)

void (*ext_payload_ptr)() = (void *)EXT_PAYLOAD_ADDR;
void (*update_ptr)() = (void *)RCM_PAYLOAD_ADDR;

void reloc_patcher(u32 payload_size)
{
	static const u32 START_OFF = 0x7C;
	static const u32 PAYLOAD_END_OFF = 0x84;
	static const u32 IPL_START_OFF = 0x88;

	memcpy((u8 *)EXT_PAYLOAD_ADDR, (u8 *)IPL_LOAD_ADDR, PATCHED_RELOC_SZ);

	*(vu32 *)(EXT_PAYLOAD_ADDR + START_OFF) = PAYLOAD_ENTRY - ALIGN(PATCHED_RELOC_SZ, 0x10);
	*(vu32 *)(EXT_PAYLOAD_ADDR + PAYLOAD_END_OFF) = PAYLOAD_ENTRY + payload_size;
	*(vu32 *)(EXT_PAYLOAD_ADDR + IPL_START_OFF) = PAYLOAD_ENTRY;

	if (payload_size == 0x7000)
	{
		memcpy((u8 *)(EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10)), (u8 *)COREBOOT_ADDR, 0x7000); //Bootblock
		*(vu32 *)CBFS_SDRAM_EN_ADDR = 0x4452414D;
	}
}

#define BOOTLOADER_UPDATED_MAGIC      0x424F4F54 // "BOOT".
#define BOOTLOADER_UPDATED_MAGIC_ADDR 0x4003E000

int launch_payload(char *path, bool update)
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);
	if (!path)
		return 1;

	if (sd_mount())
	{
		FIL fp;
		if (f_open(&fp, path, FA_READ))
		{
			EPRINTFARGS("Payload file is missing!\n(%s)", path);
			sd_unmount();

			return 1;
		}

		// Read and copy the payload to our chosen address
		void *buf;
		u32 size = f_size(&fp);

		if (size < 0x30000)
			buf = (void *)RCM_PAYLOAD_ADDR;
		else
			buf = (void *)COREBOOT_ADDR;

		if (f_read(&fp, buf, size, NULL))
		{
			f_close(&fp);
			sd_unmount();

			return 1;
		}

		f_close(&fp);
		if (!update)
		{
			free(path);
			path = NULL;
		}
			

		if (update)
		{
			u8 *update_ft = calloc(1, 6);
			memcpy(update_ft, buf + size - 6, 6);
			update_ft[4] -= '0';
			update_ft[5] -= '0';
			if (*(u32 *)update_ft == 0x43544349)
			{
				if (update_ft[4] < BLVERSIONMJ || (update_ft[4] == BLVERSIONMJ && update_ft[5] <= BLVERSIONMN))
				{
					free(update_ft);
					return 1;
				}
				*(vu32 *)BOOTLOADER_UPDATED_MAGIC_ADDR = BOOTLOADER_UPDATED_MAGIC;
			}
			else
			{
				free(update_ft);
				return 1;
			}
				
			free(update_ft);
		}
		sd_unmount();

		if (size < 0x30000)
		{
			if (!update)
				reloc_patcher(ALIGN(size, 0x10));
			reconfig_hw_workaround(false, byte_swap_32(*(u32 *)(buf + size - sizeof(u32))));
		}
		else
		{
			reloc_patcher(0x7000);
			if (*(vu32 *)CBFS_SDRAM_EN_ADDR != 0x4452414D)
				return 1;
			reconfig_hw_workaround(true, 0);
		}

		// Launch our payload.
		if (!update)
			(*ext_payload_ptr)();
		else
			(*update_ptr)();
	}

	return 1;
}

void auto_launch_update()
{
	FIL fp;
	if (*(vu32 *)BOOTLOADER_UPDATED_MAGIC_ADDR == BOOTLOADER_UPDATED_MAGIC)
		*(vu32 *)BOOTLOADER_UPDATED_MAGIC_ADDR = 0;
	else
	{
		if (sd_mount())
		{
			if (f_open(&fp, "bootloader/update.bin", FA_READ))
				return;
			else
			{
				f_close(&fp);
				launch_payload("bootloader/update.bin", true);
			}

		}
	}
}

void launch_tools(u8 type)
{
	u8 max_entries = 61;
	char *filelist = NULL;
	char *file_sec = NULL;
	char *dir = NULL;

	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 3));

	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (sd_mount())
	{
		dir = (char *)malloc(256);

		if (!type)
			memcpy(dir, "bootloader/payloads", 20);
		else
			memcpy(dir, "bootloader/libtools", 20);

		filelist = dirlist(dir, NULL, false);

		u32 i = 0;

		if (filelist)
		{
			// Build configuration menu.
			ments[0].type = MENT_BACK;
			ments[0].caption = "Back";
			ments[1].type = MENT_CHGLINE;

			while (true)
			{
				if (i > max_entries || !filelist[i * 256])
					break;
				ments[i + 2].type = INI_CHOICE;
				ments[i + 2].caption = &filelist[i * 256];
				ments[i + 2].data = &filelist[i * 256];

				i++;
			}
		}
					
		if (i > 0)
		{
			memset(&ments[i + 2], 0, sizeof(ment_t));
			menu_t menu = {
					ments,
					"Choose a file to launch", 0, 0
			};
			
			file_sec = (char *)tui_do_menu(&gfx_con, &menu);

			if (!file_sec)
			{
				free(ments);
				free(dir);
				free(filelist);
				sd_unmount();
				return;
			}
		}
		else
			EPRINTF("No payloads or libraries found.");

		free(ments);
		free(filelist);
	}
	else
	{
		free(ments);
		goto out;
	}
		

	if (file_sec)
	{
#ifdef MENU_LOGO_ENABLE
		free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
		memcpy(dir + strlen(dir), "/", 2);
		memcpy(dir + strlen(dir), file_sec, strlen(file_sec) + 1);

		if (!type)
		{
			if (launch_payload(dir, false))
			{
				EPRINTF("Failed to launch payload.");
				free(dir);
			}
		}
		else
			ianos_loader(true, dir, DRAM_LIB, NULL);
#ifdef MENU_LOGO_ENABLE
		Kc_MENU_LOGO = (u8 *)malloc(0x6000);
		blz_uncompress_srcdest(Kc_MENU_LOGO_blz, SZ_MENU_LOGO_BLZ, Kc_MENU_LOGO, SZ_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	}

out:
	sd_unmount();
	free(dir);

	btn_wait();
}

void launch_tools_payload() { launch_tools(0); }
void launch_tools_library() { launch_tools(1); }

void ini_list_launcher()
{
	u8 max_entries = 61;
	char *payload_path = NULL;

	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_list_sections);

	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (sd_mount())
	{
		if (ini_parse(&ini_list_sections, "bootloader/ini", true))
		{
			// Build configuration menu.
			ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 3));
			ments[0].type = MENT_BACK;
			ments[0].caption = "Back";
			ments[1].type = MENT_CHGLINE;

			u32 i = 2;
			LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_list_sections, link)
			{
				if (!strcmp(ini_sec->name, "config") ||
					ini_sec->type == INI_COMMENT || ini_sec->type == INI_NEWLINE)
					continue;
				ments[i].type = ini_sec->type;
				ments[i].caption = ini_sec->name;
				ments[i].data = ini_sec;
				if (ini_sec->type == MENT_CAPTION)
					ments[i].color = ini_sec->color;
				i++;

				if ((i - 1) > max_entries)
					break;
			}
			if (i > 2)
			{
				memset(&ments[i], 0, sizeof(ment_t));
				menu_t menu = {
					ments, "Launch ini configurations", 0, 0
				};
				cfg_sec = ini_clone_section((ini_sec_t *)tui_do_menu(&gfx_con, &menu));
				if (!cfg_sec)
				{
					free(ments);
					ini_free(&ini_list_sections);

					return;
				}
			}
			else
				EPRINTF("No ini configurations found.");
			free(ments);
			ini_free(&ini_list_sections);
		}
		else
			EPRINTF("Could not find any ini\nin bootloader/ini folder!");
	}

	if (!cfg_sec)
		goto out;

#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

	payload_path = ini_check_payload_section(cfg_sec);

	if (payload_path)
	{
		ini_free_section(cfg_sec);
		if (launch_payload(payload_path, false))
		{
			EPRINTF("Failed to launch payload.");
			free(payload_path);
		}
	}
	else if (!hos_launch(cfg_sec))
	{
		EPRINTF("Failed to launch firmware.");
		btn_wait();
	}

#ifdef MENU_LOGO_ENABLE
	Kc_MENU_LOGO = (u8 *)malloc(0x6000);
	blz_uncompress_srcdest(Kc_MENU_LOGO_blz, SZ_MENU_LOGO_BLZ, Kc_MENU_LOGO, SZ_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

out:
	ini_free_section(cfg_sec);

	btn_wait();
}

void launch_firmware()
{
	u8 max_entries = 61;
	char *payload_path = NULL;

	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_sections);

	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (sd_mount())
	{
		if (ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false))
		{
			// Build configuration menu.
			ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 6));
			ments[0].type = MENT_BACK;
			ments[0].caption = "Back";
			ments[1].type = MENT_CHGLINE;

			ments[2].type = MENT_HANDLER;
			ments[2].caption = "Payloads...";
			ments[2].handler = launch_tools_payload;
			ments[3].type = MENT_HANDLER;
			ments[3].caption = "More configs...";
			ments[3].handler = ini_list_launcher;

			ments[4].type = MENT_CHGLINE;

			u32 i = 5;
			LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
			{
				if (!strcmp(ini_sec->name, "config") ||
					ini_sec->type == INI_COMMENT || ini_sec->type == INI_NEWLINE)
					continue;
				ments[i].type = ini_sec->type;
				ments[i].caption = ini_sec->name;
				ments[i].data = ini_sec;
				if (ini_sec->type == MENT_CAPTION)
					ments[i].color = ini_sec->color;
				i++;

				if ((i - 4) > max_entries)
					break;
			}
			if (i < 6)
			{
				ments[i].type = MENT_CAPTION;
				ments[i].caption = "No main configurations found...";
				ments[i].color = 0xFFFFDD00;
				i++;
			}
			memset(&ments[i], 0, sizeof(ment_t));
			menu_t menu = {
				ments, "Launch configurations", 0, 0
			};
			cfg_sec = ini_clone_section((ini_sec_t *)tui_do_menu(&gfx_con, &menu));
			if (!cfg_sec)
			{
				free(ments);
				ini_free(&ini_sections);
				sd_unmount();
				return;
			}

			free(ments);
			ini_free(&ini_sections);
		}
		else
			EPRINTF("Could not open 'bootloader/hekate_ipl.ini'.\nMake sure it exists in SD Card!");
	}

	if (!cfg_sec)
	{
		gfx_puts(&gfx_con, "\nPress POWER to Continue.\nPress VOL to go to the menu.\n\n");
		gfx_printf(&gfx_con, "\nUsing default launch configuration...\n\n\n");

		u32 btn = btn_wait();
		if (!(btn & BTN_POWER))
			goto out;
	}
#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

	payload_path = ini_check_payload_section(cfg_sec);

	if (payload_path)
	{
		ini_free_section(cfg_sec);
		if (launch_payload(payload_path, false))
		{
			EPRINTF("Failed to launch payload.");
			free(payload_path);
		}
	}
	else if (!hos_launch(cfg_sec))
		EPRINTF("Failed to launch firmware.");

#ifdef MENU_LOGO_ENABLE
	Kc_MENU_LOGO = (u8 *)malloc(0x6000);
	blz_uncompress_srcdest(Kc_MENU_LOGO_blz, SZ_MENU_LOGO_BLZ, Kc_MENU_LOGO, SZ_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

out:
	ini_free_section(cfg_sec);
	sd_unmount();

	btn_wait();
}

void auto_launch_firmware()
{
	auto_launch_update();

	u8 *BOOTLOGO = NULL;
	char *payload_path = NULL;
	FIL fp;

	struct _bmp_data
	{
		u32 size;
		u32 size_x;
		u32 size_y;
		u32 offset;
		u32 pos_x;
		u32 pos_y;
	};

	struct _bmp_data bmpData;
	bool bootlogoFound = false;
	char *bootlogoCustomEntry = NULL;

	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_sections);
	LIST_INIT(ini_list_sections);

	gfx_con.mute = true;

	if (sd_mount())
	{
		if (f_open(&fp, "bootloader/hekate_ipl.ini", FA_READ))
			create_config_entry();
		else
			f_close(&fp);

		if (ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false))
		{
			u32 configEntry = 0;
			u32 boot_entry_id = 0;

			// Load configuration.
			LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
			{
				// Skip other ini entries for autoboot.
				if (ini_sec->type == INI_CHOICE)
				{
					if (!strcmp(ini_sec->name, "config"))
					{
						configEntry = 1;
						LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
						{
							if (!strcmp("autoboot", kv->key))
								h_cfg.autoboot = atoi(kv->val);
							else if (!strcmp("autoboot_list", kv->key))
								h_cfg.autoboot_list = atoi(kv->val);
							else if (!strcmp("bootwait", kv->key))
								h_cfg.bootwait = atoi(kv->val);
							else if (!strcmp("customlogo", kv->key))
								h_cfg.customlogo = atoi(kv->val);
							else if (!strcmp("verification", kv->key))
								h_cfg.verification = atoi(kv->val);
							else if (!strcmp("backlight", kv->key))
								h_cfg.backlight = atoi(kv->val);
							else if (!strcmp("autohosoff", kv->key))
								h_cfg.autohosoff = atoi(kv->val);
						}
						boot_entry_id++;
						continue;
					}

					if (h_cfg.autoboot == boot_entry_id && configEntry)
					{
						cfg_sec = ini_clone_section(ini_sec);
						LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
						{
							if (!strcmp("logopath", kv->key))
								bootlogoCustomEntry = kv->val;
							gfx_printf(&gfx_con, "\n%s=%s\n\n", kv->key, kv->val);
						}
						break;
					}
					boot_entry_id++;
				}
			}

			if (h_cfg.autohosoff)
				check_power_off_from_hos();

			if (h_cfg.autoboot_list)
			{
				ini_free(&ini_sections);
				ini_free_section(cfg_sec);
				boot_entry_id = 1;
				bootlogoCustomEntry = NULL;

				if (ini_parse(&ini_list_sections, "bootloader/ini", true))
				{
					LIST_FOREACH_ENTRY(ini_sec_t, ini_sec_list, &ini_list_sections, link)
					{
						if (ini_sec_list->type == INI_CHOICE)
						{
							if (!strcmp(ini_sec_list->name, "config"))
								continue;

							if (h_cfg.autoboot == boot_entry_id)
							{
								cfg_sec = ini_clone_section(ini_sec_list);
								LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
								{
									if (!strcmp("logopath", kv->key))
										bootlogoCustomEntry = kv->val;
									gfx_printf(&gfx_con, "\n%s=%s\n\n", kv->key, kv->val);
								}
								break;
							}
							boot_entry_id++;
						}
						
					}

				}

			}

			// Add missing configuration entry.
			if (!configEntry)
				create_config_entry();

			if (!h_cfg.autoboot)
				goto out; // Auto boot is disabled.

			if (!cfg_sec)
				goto out; // No configurations.
		}
		else
			goto out; // Can't load hekate_ipl.ini.
	}
	else
		goto out;

	if (h_cfg.customlogo)
	{
		u8 *bitmap = NULL;
		if (bootlogoCustomEntry != NULL) // Check if user set custom logo path at the boot entry.
		{
			bitmap = (u8 *)sd_file_read(bootlogoCustomEntry);
			if (bitmap == NULL) // Custom entry bootlogo not found, trying default custom one.
				bitmap = (u8 *)sd_file_read("bootloader/bootlogo.bmp");
		}
		else // User has not set a custom logo path.
			bitmap = (u8 *)sd_file_read("bootloader/bootlogo.bmp");

		if (bitmap != NULL)
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
				bitmap[28] == 32 && //
				bmpData.size_x <= 720 &&
				bmpData.size_y <= 1280)
			{
				if ((bmpData.size - bmpData.offset) <= 0x400000)
				{
					// Avoid unaligned access from BM 2-byte MAGIC and remove header.
					BOOTLOGO = (u8 *)malloc(0x400000);
					memcpy(BOOTLOGO, bitmap + bmpData.offset, bmpData.size - bmpData.offset);
					free(bitmap);
					// Center logo if res < 720x1280.
					bmpData.pos_x = (720  - bmpData.size_x) >> 1;
					bmpData.pos_y = (1280 - bmpData.size_y) >> 1;
					// Get background color from 1st pixel.
					if (bmpData.size_x < 720 || bmpData.size_y < 1280)
						gfx_clear_color(&gfx_ctxt, *(u32 *)BOOTLOGO);

					bootlogoFound = true;
				}
			}
			else
				free(bitmap);
		}
	}

	// Render boot logo.
	if (bootlogoFound)
	{
		gfx_render_bmp_argb(&gfx_ctxt, (u32 *)BOOTLOGO, bmpData.size_x, bmpData.size_y,
			bmpData.pos_x, bmpData.pos_y);
	}
	else
	{
		gfx_clear_grey(&gfx_ctxt, 0x1B);
		BOOTLOGO = (void *)malloc(0x4000);
		blz_uncompress_srcdest(BOOTLOGO_BLZ, SZ_BOOTLOGO_BLZ, BOOTLOGO, SZ_BOOTLOGO);
		gfx_set_rect_grey(&gfx_ctxt, BOOTLOGO, X_BOOTLOGO, Y_BOOTLOGO, 326, 544);
	}
	free(BOOTLOGO);

	display_backlight_brightness(h_cfg.backlight, 1000);

	// Wait before booting. If VOL- is pressed go into bootloader menu.
	u32 btn = btn_wait_timeout(h_cfg.bootwait * 1000, BTN_VOL_DOWN);

	if (btn & BTN_VOL_DOWN)
		goto out;

	ini_free(&ini_sections);
	if (h_cfg.autoboot_list)
		ini_free(&ini_list_sections);

#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

	payload_path = ini_check_payload_section(cfg_sec);

	if (payload_path)
	{
		ini_free_section(cfg_sec);
		if (launch_payload(payload_path, false))
			free(payload_path);
	}
	else
		hos_launch(cfg_sec);

#ifdef MENU_LOGO_ENABLE
	Kc_MENU_LOGO = (u8 *)malloc(ALIGN(SZ_MENU_LOGO, 0x10));
	blz_uncompress_srcdest(Kc_MENU_LOGO_blz, SZ_MENU_LOGO_BLZ, Kc_MENU_LOGO, SZ_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

out:
	ini_free(&ini_sections);
	if (h_cfg.autoboot_list)
		ini_free(&ini_list_sections);
	ini_free_section(cfg_sec);

	sd_unmount();
	gfx_con.mute = false;
}

void about()
{
	static const char credits[] =
		"\nhekate     (C) 2018 naehrwert, st4rk\n\n"
		"CTCaer mod (C) 2018 CTCaer\n"
		" ___________________________________________\n\n"
		"Thanks to: %kderrek, nedwill, plutoo,\n"
		"           shuffle2, smea, thexyz, yellows8%k\n"
		" ___________________________________________\n\n"
		"Greetings to: fincs, hexkyz, SciresM,\n"
		"              Shiny Quagsire, WinterMute\n"
		" ___________________________________________\n\n"
		"Open source and free packages used:\n\n"
		" - FatFs R0.13b,\n"
		"   Copyright (C) 2018, ChaN\n\n"
		" - bcl-1.2.0,\n"
		"   Copyright (C) 2003-2006, Marcus Geelnard\n\n"
		" - Atmosphere (SE sha256, prc id patches),\n"
		"   Copyright (C) 2018, Atmosphere-NX\n\n"
		" - elfload,\n"
		"   Copyright (C) 2014, Owen Shepherd\n"
		"   Copyright (C) 2018, M4xw\n"
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

	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, credits, 0xFF00CCFF, 0xFFCCCCCC);
	gfx_con.fntsz = 8;
	gfx_printf(&gfx_con, octopus, 0xFF00CCFF, 0xFF00FFCC, 0xFF00CCFF, 0xFFCCCCCC);

	btn_wait();
}

ment_t ment_options[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Auto boot", config_autoboot),
	MDEF_HANDLER("Boot time delay", config_bootdelay),
	MDEF_HANDLER("Custom boot logo", config_customlogo),
	MDEF_HANDLER("Auto HOS power off", config_auto_hos_poweroff),
	MDEF_HANDLER("Backlight", config_backlight),
	MDEF_END()
};

menu_t menu_options = {
	ment_options,
	"Launch Options", 0, 0
};

ment_t ment_cinfo[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- SoC Info ----", 0xFF0AB9E6),
	MDEF_HANDLER("Ipatches & bootrom info", bootrom_ipatches_info),
	MDEF_HANDLER("Print fuse info", print_fuseinfo),
	MDEF_HANDLER("Print kfuse info", print_kfuseinfo),
	MDEF_HANDLER("Print TSEC keys", print_tsec_key),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- Storage Info --", 0xFF0AB9E6),
	MDEF_HANDLER("Print eMMC info", print_mmc_info),
	MDEF_HANDLER("Print SD Card info", print_sdcard_info),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Misc ------", 0xFF0AB9E6),
	MDEF_HANDLER("Print battery info", print_battery_info),
	MDEF_END()
};
menu_t menu_cinfo = {
	ment_cinfo,
	"Console Info", 0, 0
};

ment_t ment_restore[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Full --------", 0xFF0AB9E6),
	MDEF_HANDLER("Restore eMMC BOOT0/1", restore_emmc_boot),
	MDEF_HANDLER("Restore eMMC RAW GPP (exFAT only)", restore_emmc_rawnand),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- GPP Partitions --", 0xFF0AB9E6),
	MDEF_HANDLER("Restore GPP partitions", restore_emmc_gpp_parts),
	MDEF_END()
};

menu_t menu_restore = {
	ment_restore,
	"Restore Options", 0, 0
};

ment_t ment_backup[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Full --------", 0xFF0AB9E6),
	MDEF_HANDLER("Backup eMMC BOOT0/1", dump_emmc_boot),
	MDEF_HANDLER("Backup eMMC RAW GPP", dump_emmc_rawnand),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- GPP Partitions --", 0xFF0AB9E6),
	MDEF_HANDLER("Backup eMMC SYS", dump_emmc_system),
	MDEF_HANDLER("Backup eMMC USER", dump_emmc_user),
	MDEF_END()
};

menu_t menu_backup = {
	ment_backup,
	"Backup Options", 0, 0
};

ment_t ment_tools[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- Backup & Restore --", 0xFF0AB9E6),
	MDEF_MENU("Backup", &menu_backup),
	MDEF_MENU("Restore", &menu_restore),
	MDEF_HANDLER("Verification options", config_verification),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-------- Misc --------", 0xFF0AB9E6),
	MDEF_HANDLER("Dump package1/2", dump_packages12),
	MDEF_HANDLER("Fix battery de-sync", fix_battery_desync),
	MDEF_HANDLER("Fix archive bit (except Nintendo folder)", fix_sd_all_attr),
	MDEF_HANDLER("Fix archive bit (Nintendo folder)", fix_sd_nin_attr),
	//MDEF_HANDLER("Fix fuel gauge configuration", fix_fuel_gauge_configuration),
	//MDEF_HANDLER("Reset all battery cfg", reset_pmic_fuel_gauge_charger_config),
	//MDEF_HANDLER("Minerva", minerva), // Uncomment for testing Minerva Training Cell
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Dangerous -----", 0xFFFF0000),
	MDEF_HANDLER("AutoRCM", menu_autorcm),
	MDEF_END()
};

menu_t menu_tools = {
	ment_tools,
	"Tools", 0, 0
};

ment_t ment_top[] = {
	MDEF_HANDLER("Launch", launch_firmware),
	MDEF_MENU("Options", &menu_options),
	MDEF_CAPTION("---------------", 0xFF444444),
	MDEF_MENU("Tools", &menu_tools),
	MDEF_MENU("Console info", &menu_cinfo),
	MDEF_CAPTION("---------------", 0xFF444444),
	MDEF_HANDLER("Reboot (Normal)", reboot_normal),
	MDEF_HANDLER("Reboot (RCM)", reboot_rcm),
	MDEF_HANDLER("Power off", power_off),
	MDEF_CAPTION("---------------", 0xFF444444),
	MDEF_HANDLER("About", about),
	MDEF_END()
};
menu_t menu_top = {
	ment_top,
	"hekate - CTCaer mod v4.6", 0, 0
};

extern void pivot_stack(u32 stack_top);

void ipl_main()
{
	config_hw();

	//Pivot the stack so we have enough space.
	pivot_stack(0x90010000);

	//Tegra/Horizon configuration goes to 0x80000000+, package2 goes to 0xA9800000, we place our heap in between.
	heap_init(0x90020000);

#ifdef DEBUG_UART_PORT
	uart_send(DEBUG_UART_PORT, (u8 *)"Hekate: Hello!\r\n", 18);
	uart_wait_idle(DEBUG_UART_PORT, UART_TX_IDLE);
#endif

	// Set bootloader's default configuration.
	set_default_configuration();

	// Save sdram lp0 config.
	if (ianos_loader(true, "bootloader/sys/libsys_lp0.bso", DRAM_LIB, (void *)sdram_get_params()))
		h_cfg.errors |= ERR_LIBSYS_LP0;

	display_init();

	u32 *fb = display_init_framebuffer();
	gfx_init_ctxt(&gfx_ctxt, fb, 720, 1280, 720);

#ifdef MENU_LOGO_ENABLE
	Kc_MENU_LOGO = (u8 *)malloc(0x6000);
	blz_uncompress_srcdest(Kc_MENU_LOGO_blz, SZ_MENU_LOGO_BLZ, Kc_MENU_LOGO, SZ_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

	gfx_con_init(&gfx_con, &gfx_ctxt);

	display_backlight_pwm_init();
	//display_backlight_brightness(h_cfg.backlight, 1000);

	// Load saved configuration and auto boot if enabled.
	auto_launch_firmware();

	while (true)
		tui_do_menu(&gfx_con, &menu_top);

	while (true)
		;
}
