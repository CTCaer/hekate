/*
 * Copyright (c) 2018 naehrwert
 *
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

#include <string.h>
#include <stdlib.h>

#include "gfx/gfx.h"
#include "config/config.h"
#include "gfx/di.h"

#include "gfx/tui.h"
#include "hos/hos.h"
#include "hos/secmon_exo.h"
#include "hos/sept.h"
#include "ianos/ianos.h"
#include "libs/compr/blz.h"
#include "libs/fatfs/ff.h"
#include "mem/heap.h"
#include "mem/minerva.h"
#include "mem/sdram.h"
#include "power/max77620.h"
#include "rtc/max77620-rtc.h"
#include "soc/bpmp.h"
#include "soc/fuse.h"
#include "soc/hw_init.h"
#include "soc/i2c.h"
#include "soc/t210.h"
#include "soc/uart.h"
#include "storage/emummc.h"
#include "storage/nx_emmc.h"
#include "storage/sdmmc.h"
#include "utils/btn.h"
#include "utils/dirlist.h"
#include "utils/list.h"
#include "utils/util.h"
#include "utils/browser.h"
#include "frontend/fe_emmc_tools.h"
#include "frontend/fe_tools.h"
#include "frontend/fe_info.h"

//TODO: ugly.
sdmmc_t sd_sdmmc;
sdmmc_storage_t sd_storage;
FATFS sd_fs;
static bool sd_mounted;

#ifdef MENU_LOGO_ENABLE
u8 *Kc_MENU_LOGO;
#endif //MENU_LOGO_ENABLE

hekate_config h_cfg;
boot_cfg_t __attribute__((section ("._boot_cfg"))) b_cfg;
const volatile ipl_ver_meta_t __attribute__((section ("._ipl_version"))) ipl_ver = {
	.magic = BL_MAGIC,
	.version = (BL_VER_MJ + '0') | ((BL_VER_MN + '0') << 8) | ((BL_VER_HF + '0') << 16),
	.rsvd0 = 0,
	.rsvd1 = 0
};

volatile nyx_storage_t *nyx_str = (nyx_storage_t *)NYX_STORAGE_ADDR;

bool sd_mount()
{
	if (sd_mounted)
		return true;

	if (!sdmmc_storage_init_sd(&sd_storage, &sd_sdmmc, SDMMC_1, SDMMC_BUS_WIDTH_4, 11))
	{
		gfx_con.mute = false;
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
			gfx_con.mute = false;
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

void *sd_file_read(const char *path, u32 *fsize)
{
	FIL fp;
	if (f_open(&fp, path, FA_READ) != FR_OK)
		return NULL;

	u32 size = f_size(&fp);
	if (fsize)
		*fsize = size;

	void *buf = malloc(size);

	if (f_read(&fp, buf, size, NULL) != FR_OK)
	{
		free(buf);
		f_close(&fp);

		return NULL;
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
		return res;
	}

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

void check_power_off_from_hos()
{
	// Power off on AutoRCM wakeup from HOS shutdown. For modchips/dongles.
	u8 hosWakeup = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_IRQTOP);
	if (hosWakeup & MAX77620_IRQ_TOP_RTC_MASK)
	{
		sd_unmount();

		// Stop the alarm, in case we injected too fast.
		max77620_rtc_stop_alarm();
		power_off();
	}
}

// This is a safe and unused DRAM region for our payloads.
#define RELOC_META_OFF      0x7C
#define PATCHED_RELOC_SZ    0x94
#define PATCHED_RELOC_STACK 0x40007000
#define PATCHED_RELOC_ENTRY 0x40010000
#define EXT_PAYLOAD_ADDR    0xC03C0000
#define RCM_PAYLOAD_ADDR    (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
#define COREBOOT_ADDR       (0xD0000000 - 0x100000)
#define CBFS_DRAM_EN_ADDR   0x4003e000
#define  CBFS_DRAM_MAGIC    0x4452414D // "DRAM"

void reloc_patcher(u32 payload_dst, u32 payload_src, u32 payload_size)
{
	memcpy((u8 *)payload_src, (u8 *)IPL_LOAD_ADDR, PATCHED_RELOC_SZ);

	volatile reloc_meta_t *relocator = (reloc_meta_t *)(payload_src + RELOC_META_OFF);

	relocator->start = payload_dst - ALIGN(PATCHED_RELOC_SZ, 0x10);
	relocator->stack = PATCHED_RELOC_STACK;
	relocator->end   = payload_dst + payload_size;
	relocator->ep    = payload_dst;

	if (payload_size == 0x7000)
	{
		memcpy((u8 *)(payload_src + ALIGN(PATCHED_RELOC_SZ, 0x10)), (u8 *)COREBOOT_ADDR, 0x7000); //Bootblock
		*(vu32 *)CBFS_DRAM_EN_ADDR = CBFS_DRAM_MAGIC;
	}
}

bool is_ipl_updated(void *buf)
{
	ipl_ver_meta_t *update_ft = (ipl_ver_meta_t *)(buf + PATCHED_RELOC_SZ + sizeof(boot_cfg_t));

	if (update_ft->magic == ipl_ver.magic)
	{
		if (byte_swap_32(update_ft->version) <= byte_swap_32(ipl_ver.version))
			return true;
		return false;

	}
	else
		return true;
}

int launch_payload(char *path, bool update)
{
	if (!update)
		gfx_clear(BG_COL);
	gfx_con_setpos(0, 0);
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

		if (update && is_ipl_updated(buf))
			return 1;

		sd_unmount();

		if (size < 0x30000)
		{
			if (update)
				memcpy((u8 *)(RCM_PAYLOAD_ADDR + PATCHED_RELOC_SZ), &b_cfg, sizeof(boot_cfg_t)); // Transfer boot cfg.
			else
				reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, ALIGN(size, 0x10));

			reconfig_hw_workaround(false, byte_swap_32(*(u32 *)(buf + size - sizeof(u32))));
		}
		else
		{
			reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, 0x7000);
			reconfig_hw_workaround(true, 0);
		}

		// Some cards (Sandisk U1), do not like a fast power cycle. Wait min 100ms.
		sdmmc_storage_init_wait_sd();

		void (*ext_payload_ptr)() = (void *)EXT_PAYLOAD_ADDR;
		void (*update_ptr)() = (void *)RCM_PAYLOAD_ADDR;

		// Launch our payload.
		if (!update)
			(*ext_payload_ptr)();
		else
		{
			EMC(EMC_SCRATCH0) |= EMC_HEKA_UPD;
			(*update_ptr)();
		}
	}

	return 1;
}

void auto_launch_update()
{
	if (EMC(EMC_SCRATCH0) & EMC_HEKA_UPD)
		EMC(EMC_SCRATCH0) &= ~EMC_HEKA_UPD;
	else if (sd_mount())
	{
		if (!f_stat("bootloader/update.bin", NULL))
			launch_payload("bootloader/update.bin", true);
	}
}

void launch_tools()
{
	char *file_sec = NULL;

	gfx_clear(BG_COL);
	gfx_con_setpos(0, 0);

	if (sd_mount())
	{
	//Start folder//extn//Menu caption//return containing dir only//return to root//return in ASCII order//
	file_sec = file_browser("bootloader/payloads", ".bin", "Select A Payload", false, true, true);
	if (!file_sec) return;
	}
	if (launch_payload(file_sec, false))
		{
			EPRINTF("Failed to launch payload.");
		}
	
	sd_unmount();
	free(file_sec);

	btn_wait();
}

void ini_list_launcher()
{
	u8 max_entries = 61;
	char *payload_path = NULL;
	char *path_sec = malloc(256);
	
	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_list_sections);

	gfx_clear(BG_COL);
	gfx_con_setpos(0, 0);

	if (sd_mount())
	{
		//Start folder//extn//Menu caption//return containing dir only//return to root//return in ASCII order//
		path_sec = file_browser("bootloader/ini", ".ini", "Choose folder / INI", true, false, true);
		if (!path_sec) return;
		u8 fr1 = 0; u8 fr2 = 0;
		
		fr1 = (ini_parse(&ini_list_sections, path_sec, true));
		if(!fr1) {fr2 = (ini_parse(&ini_list_sections, path_sec, false));}
		
		if(fr1 || fr2)
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

				cfg_sec = (ini_sec_t *)tui_do_menu(&menu);

				if (cfg_sec)
				{
					u32 non_cfg = 1;
					for (int j = 2; j < i; j++)
					{
						if (ments[j].type != INI_CHOICE)
							non_cfg++;

						if (ments[j].data == cfg_sec)
						{
							b_cfg.boot_cfg = BOOT_CFG_FROM_LAUNCH;
							b_cfg.autoboot = j - non_cfg;
							b_cfg.autoboot_list = 1;

							break;
						}
					}
				}

				payload_path = ini_check_payload_section(cfg_sec);

				if (cfg_sec && !payload_path)
					check_sept();

				if (!cfg_sec)
				{
					free(ments);

					return;
				}
			}
			else
				EPRINTF("No extra configs found.");
			free(ments);
		}
		else
			EPRINTF("Could not find any ini\nin bootloader/ini!");
	}

	if (!cfg_sec)
		goto out;

	if (payload_path)
	{
		if (launch_payload(payload_path, false))
		{
			EPRINTF("Failed to launch payload.");
			free(payload_path);
		}
	}
	else if (!hos_launch(cfg_sec))
	{
		EPRINTF("Failed to launch firmware.");
	}

out:

	btn_wait();
}

void launch_firmware()
{
	u8 max_entries = 61;
	char *payload_path = NULL;

	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_sections);

	gfx_clear(BG_COL);
	gfx_con_setpos(0, 0);

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
			ments[2].handler = launch_tools;
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
				ments[i].caption = "No main configs found...";
				ments[i].color = ATTNCOL;
				i++;
			}
			memset(&ments[i], 0, sizeof(ment_t));
			menu_t menu = {
				ments, "Launch configurations", 0, 0
			};

			cfg_sec = (ini_sec_t *)tui_do_menu(&menu);

			if (cfg_sec)
			{
				u8 non_cfg = 4;
				for (int j = 5; j < i; j++)
				{
					if (ments[j].type != INI_CHOICE)
						non_cfg++;
					if (ments[j].data == cfg_sec)
					{
						b_cfg.boot_cfg = BOOT_CFG_FROM_LAUNCH;
						b_cfg.autoboot = j - non_cfg;
						b_cfg.autoboot_list = 0;

						break;
					}
				}
			}

			payload_path = ini_check_payload_section(cfg_sec);

			if (cfg_sec)
			{
				LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
				{
					if (!strcmp("emummc_force_disable", kv->key))
						h_cfg.emummc_force_disable = atoi(kv->val);
				}
			}

			if (cfg_sec && !payload_path)
				check_sept();

			if (!cfg_sec)
			{
				free(ments);
				sd_unmount();
				return;
			}

			free(ments);
		}
		else
			EPRINTF("Could not open 'bootloader/hekate_ipl.ini'.\nMake sure it exists!");
	}

	if (!cfg_sec)
	{
		gfx_puts("\nUsing default launch configuration...\n");
		gfx_puts("\nPress POWER to Continue.\nPress VOL to go to the menu.");

		u32 btn = btn_wait();
		if (!(btn & BTN_POWER))
			goto out;
	}

	if (payload_path)
	{
		if (launch_payload(payload_path, false))
		{
			EPRINTF("Failed to launch payload.");
			free(payload_path);
		}
	}
	else if (!hos_launch(cfg_sec))
		EPRINTF("Failed to launch firmware.");

out:
	sd_unmount();

	h_cfg.emummc_force_disable = false;

	btn_wait();
}

void nyx_load_run()
{
	sd_mount();

	u8 *nyx = sd_file_read("bootloader/sys/nyx.bin", NULL);
	if (!nyx)
		return;

	sd_unmount();

	gfx_clear(BG_COL);
	display_backlight_brightness(h_cfg.backlight, 1000);

	nyx_str->cfg = 0;
	if (b_cfg.extra_cfg & EXTRA_CFG_NYX_DUMP)
	{
		b_cfg.extra_cfg &= ~(EXTRA_CFG_NYX_DUMP);
		nyx_str->cfg |= NYX_CFG_DUMP;
	}
	if (nyx_str->mtc_cfg.mtc_table)
		nyx_str->cfg |= NYX_CFG_MINERVA;

	nyx_str->version = ipl_ver.version - 0x303030;

	//memcpy((u8 *)nyx_str->irama, (void *)IRAM_BASE, 0x8000);
	volatile reloc_meta_t *reloc = (reloc_meta_t *)(IPL_LOAD_ADDR + RELOC_META_OFF);
	memcpy((u8 *)nyx_str->hekate, (u8 *)reloc->start, reloc->end - reloc->start);

	void (*nyx_ptr)() = (void *)nyx;

	bpmp_mmu_disable();
	bpmp_clk_rate_set(BPMP_CLK_NORMAL);
	minerva_periodic_training();

	// Some cards (Sandisk U1), do not like a fast power cycle. Wait min 100ms.
	sdmmc_storage_init_wait_sd();

	(*nyx_ptr)();
}

static ini_sec_t *get_ini_sec_from_id(ini_sec_t *ini_sec, char *bootlogoCustomEntry)
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
		if (!strcmp("logopath", kv->key))
			bootlogoCustomEntry = kv->val;
		if (!strcmp("emummc_force_disable", kv->key))
			h_cfg.emummc_force_disable = atoi(kv->val);
	}
	if (!cfg_sec)
	{
		bootlogoCustomEntry = NULL;
		h_cfg.emummc_force_disable = false;
	}

	return cfg_sec;
}

void auto_launch_firmware()
{
	if(b_cfg.extra_cfg & EXTRA_CFG_NYX_DUMP)
	{
		if (!h_cfg.sept_run)
			EMC(EMC_SCRATCH0) |= EMC_HEKA_UPD;
		check_sept();
	}

	if (!h_cfg.sept_run)
		auto_launch_update();

	u8 *BOOTLOGO = NULL;
	char *payload_path = NULL;
	u32 btn = 0;
	bool boot_from_id = (b_cfg.boot_cfg & BOOT_CFG_FROM_ID) && (b_cfg.boot_cfg & BOOT_CFG_AUTOBOOT_EN);
	if (boot_from_id)
		b_cfg.id[7] = 0;

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

	if (!(b_cfg.boot_cfg & BOOT_CFG_FROM_LAUNCH))
		gfx_con.mute = true;

	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_sections);
	LIST_INIT(ini_list_sections);

	if (sd_mount())
	{
		if (f_stat("bootloader/hekate_ipl.ini", NULL))
			create_config_entry();

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
							else if (!strcmp("verification", kv->key))
								h_cfg.verification = atoi(kv->val);
							else if (!strcmp("backlight", kv->key))
								h_cfg.backlight = atoi(kv->val);
							else if (!strcmp("autohosoff", kv->key))
								h_cfg.autohosoff = atoi(kv->val);
							else if (!strcmp("autonogc", kv->key))
								h_cfg.autonogc = atoi(kv->val);
							else if (!strcmp("brand", kv->key))
							{
								h_cfg.brand = malloc(strlen(kv->val) + 1);
								strcpy(h_cfg.brand, kv->val);
							}
							else if (!strcmp("tagline", kv->key))
							{
								h_cfg.tagline = malloc(strlen(kv->val) + 1);
								strcpy(h_cfg.tagline, kv->val);
							}
						}
						boot_entry_id++;

						// Override autoboot, otherwise save it for a possbile sept run.
						if (b_cfg.boot_cfg & BOOT_CFG_AUTOBOOT_EN)
						{
							h_cfg.autoboot = b_cfg.autoboot;
							h_cfg.autoboot_list = b_cfg.autoboot_list;
						}
						else
						{
							b_cfg.autoboot = h_cfg.autoboot;
							b_cfg.autoboot_list = h_cfg.autoboot_list;
						}

						continue;
					}

					if (boot_from_id)
						cfg_sec = get_ini_sec_from_id(ini_sec, bootlogoCustomEntry);				
					else if (h_cfg.autoboot == boot_entry_id && configEntry)
					{
						cfg_sec = ini_sec;
						LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
						{
							if (!strcmp("logopath", kv->key))
								bootlogoCustomEntry = kv->val;
							if (!strcmp("emummc_force_disable", kv->key))
								h_cfg.emummc_force_disable = atoi(kv->val);
						}
					}
					if (cfg_sec)
						break;
					boot_entry_id++;
				}
			}

			if (h_cfg.autohosoff && !(b_cfg.boot_cfg & BOOT_CFG_AUTOBOOT_EN))
				check_power_off_from_hos();

			if (h_cfg.autoboot_list || (boot_from_id && !cfg_sec))
			{
				if (boot_from_id && cfg_sec)
					goto skip_list;

				cfg_sec = NULL;
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

							if (boot_from_id)
								cfg_sec = get_ini_sec_from_id(ini_sec_list, bootlogoCustomEntry);
							else if (h_cfg.autoboot == boot_entry_id)
							{
								h_cfg.emummc_force_disable = false;
								cfg_sec = ini_sec_list;
								LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg_sec->kvs, link)
								{
									if (!strcmp("logopath", kv->key))
										bootlogoCustomEntry = kv->val;
									if (!strcmp("emummc_force_disable", kv->key))
										h_cfg.emummc_force_disable = atoi(kv->val);
								}
							}
							if (cfg_sec)
								break;
							boot_entry_id++;
						}
					}

				}

			}
skip_list:
			// Add missing configuration entry.
			if (!configEntry)
				create_config_entry();

			if (!cfg_sec)
				goto out; // No configurations or auto boot is disabled.
		}
		else
			goto out; // Can't load hekate_ipl.ini.
	}
	else
		goto out;

	u8 *bitmap = NULL;
	if (!(b_cfg.boot_cfg & BOOT_CFG_FROM_LAUNCH) && h_cfg.bootwait && !h_cfg.sept_run)
	{
		if (bootlogoCustomEntry) // Check if user set custom logo path at the boot entry.
			bitmap = (u8 *)sd_file_read(bootlogoCustomEntry, NULL);

		if (!bitmap) // Custom entry bootlogo not found, trying default custom one.
			bitmap = (u8 *)sd_file_read("bootloader/bootlogo.bmp", NULL);

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
						gfx_clear_color(*(u32 *)BOOTLOGO);

					bootlogoFound = true;
				}
			}
			else
				free(bitmap);
		}

		// Render boot logo.
		if (bootlogoFound)
		{
			gfx_render_bmp_argb((u32 *)BOOTLOGO, bmpData.size_x, bmpData.size_y,
				bmpData.pos_x, bmpData.pos_y);
		}
		else
		{
			gfx_clear_grey(0x1B);
			BOOTLOGO = (void *)malloc(0x4000);
			blz_uncompress_srcdest(BOOTLOGO_BLZ, SZ_BOOTLOGO_BLZ, BOOTLOGO, SZ_BOOTLOGO);
			gfx_set_rect_grey(BOOTLOGO, X_BOOTLOGO, Y_BOOTLOGO, 326, 544);
		}
		free(BOOTLOGO);
	}

	if (b_cfg.boot_cfg & BOOT_CFG_FROM_LAUNCH)
		display_backlight_brightness(h_cfg.backlight, 0);
	else if (!h_cfg.sept_run && h_cfg.bootwait)
		display_backlight_brightness(h_cfg.backlight, 1000);

	// Wait before booting. If VOL- is pressed go into bootloader menu.
	if (!h_cfg.sept_run && !(b_cfg.boot_cfg & BOOT_CFG_FROM_LAUNCH))
	{
		btn = btn_wait_timeout(h_cfg.bootwait * 1000, BTN_VOL_DOWN);

		if (btn & BTN_VOL_DOWN)
			goto out;
	}

	payload_path = ini_check_payload_section(cfg_sec);

	if (payload_path)
	{
		if (launch_payload(payload_path, false))
			free(payload_path);
	}
	else
	{
		check_sept();
		hos_launch(cfg_sec);

		EPRINTF("\nFailed to launch HOS!");
		msleep(500);
		btn_wait();
	}

out:
	gfx_con.mute = false;

	// Clear boot reasons from binary.
	if (b_cfg.boot_cfg & BOOT_CFG_FROM_ID)
		memset(b_cfg.xt_str, 0, sizeof(b_cfg.xt_str));
	b_cfg.boot_cfg &= ~(BOOT_CFG_AUTOBOOT_EN | BOOT_CFG_FROM_LAUNCH | BOOT_CFG_FROM_ID);
	h_cfg.emummc_force_disable = false;

	nyx_load_run();

	sd_unmount();
}

void patched_rcm_protection()
{
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	h_cfg.rcm_patched = fuse_check_patched_rcm();

	if (!h_cfg.rcm_patched)
		return;

	// Check if AutoRCM is enabled and protect from a permanent brick.
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
		return;

	u8 *tempbuf = (u8 *)malloc(0x200);
	sdmmc_storage_set_mmc_partition(&storage, 1);

	u8 corr_mod_byte0;
	int i, sect = 0;
	if ((fuse_read_odm(4) & 3) != 3)
		corr_mod_byte0 = 0xF7;
	else
		corr_mod_byte0 = 0x37;

	for (i = 0; i < 4; i++)
	{
		sect = (0x200 + (0x4000 * i)) / NX_EMMC_BLOCKSIZE;
		sdmmc_storage_read(&storage, sect, 1, tempbuf);

		// If AutoRCM is enabled, disable it.
		if (tempbuf[0x10] != corr_mod_byte0)
		{
			tempbuf[0x10] = corr_mod_byte0;

			sdmmc_storage_write(&storage, sect, 1, tempbuf);
		}
	}

	free(tempbuf);
	sdmmc_storage_end(&storage);
}

void about()
{
	static const char credits[] =
		"\nhekate     (c) 2018 naehrwert, st4rk\n\n"
		"CTCaer mod (c) 2018 CTCaer\n"
		" ___________________________________________\n\n"
		"Thanks to: %kderrek, nedwill, plutoo,\n"
		"           shuffle2, smea, thexyz, yellows8%k\n"
		" ___________________________________________\n\n"
		"Greetings to: fincs, hexkyz, SciresM,\n"
		"              Shiny Quagsire, WinterMute\n"
		" ___________________________________________\n\n"
		"Open source and free packages used:\n\n"
		" - FatFs R0.13b,\n"
		"   Copyright (c) 2018, ChaN\n\n"
		" - bcl-1.2.0,\n"
		"   Copyright (c) 2003-2006, Marcus Geelnard\n\n"
		" - Atmosphere (SE sha256, prc id patches),\n"
		"   Copyright (c) 2018, Atmosphere-NX\n\n"
		" - elfload,\n"
		"   Copyright (c) 2014, Owen Shepherd\n"
		"   Copyright (c) 2018, M4xw\n"
		" ___________________________________________\n\n";
		

	gfx_clear(BG_COL);
	gfx_con_setpos(0, 0);

	gfx_printf(credits, INFOCOL, MAINTXTCOL);
	

	btn_wait();
}

ment_t ment_options[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_HANDLER("Auto boot", config_autoboot),
	MDEF_HANDLER("Boot time delay", config_bootdelay),
	MDEF_HANDLER("Auto NoGC", config_nogc),
	MDEF_HANDLER("Auto HOS power off", config_auto_hos_poweroff),
	MDEF_HANDLER("Backlight", config_backlight),
	MDEF_END()
};

menu_t menu_options = { ment_options, "Launch Options", 0, 0 };

ment_t ment_cinfo[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- SoC Info ----", INFOCOL),
	MDEF_HANDLER("Ipatches & bootrom info", bootrom_ipatches_info),
	MDEF_HANDLER("Print fuse info", print_fuseinfo),
	MDEF_HANDLER("Print kfuse info", print_kfuseinfo),
	MDEF_HANDLER("Print TSEC keys", print_tsec_key),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- Storage Info --", INFOCOL),
	MDEF_HANDLER("Print eMMC info", print_mmc_info),
	MDEF_HANDLER("Print SD Card info", print_sdcard_info),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Misc ------", INFOCOL),
	MDEF_HANDLER("Print battery info", print_battery_info),
	MDEF_END()
};

menu_t menu_cinfo = { ment_cinfo, "Console Info", 0, 0 };

ment_t ment_restore[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Full --------", INFOCOL),
	MDEF_HANDLER("Restore eMMC BOOT0/1", restore_emmc_boot),
	MDEF_HANDLER("Restore eMMC RAW GPP", restore_emmc_rawnand),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- GPP Partitions --", INFOCOL),
	MDEF_HANDLER("Restore GPP partitions", restore_emmc_gpp_parts),
	MDEF_END()
};

menu_t menu_restore = { ment_restore, "Restore Options", 0, 0 };

ment_t ment_backup[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Full --------", INFOCOL),
	MDEF_HANDLER("Backup eMMC BOOT0/1", dump_emmc_boot),
	MDEF_HANDLER("Backup eMMC RAW GPP", dump_emmc_rawnand),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- GPP Partitions --", INFOCOL),
	MDEF_HANDLER("Backup eMMC SYS", dump_emmc_system),
	MDEF_HANDLER("Backup eMMC USER", dump_emmc_user),
	MDEF_END()
};

menu_t menu_backup = { ment_backup, "Backup Options", 0, 0 };

ment_t ment_tools[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- Backup & Restore --", INFOCOL),
	MDEF_MENU("Backup", &menu_backup),
	MDEF_MENU("Restore", &menu_restore),
	MDEF_HANDLER("Verification options", config_verification),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-------- Misc --------", INFOCOL),
	MDEF_HANDLER("Dump package1/2", dump_packages12),
	MDEF_HANDLER("Fix archive bit (except Nintendo)", fix_sd_all_attr),
	MDEF_HANDLER("Fix archive bit (Nintendo only)", fix_sd_nin_attr),
	//MDEF_HANDLER("Fix fuel gauge configuration", fix_fuel_gauge_configuration),
	//MDEF_HANDLER("Reset all battery cfg", reset_pmic_fuel_gauge_charger_config),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-------- Other -------", ATTNCOL),
	MDEF_HANDLER("AutoRCM", menu_autorcm),
	MDEF_END()
};

menu_t menu_tools = { ment_tools, "Tools", 0, 0 };

ment_t ment_top[] = {
	MDEF_HANDLER("Launch", launch_firmware),
	MDEF_MENU("Options", &menu_options),
	MDEF_CAPTION("---------------", MENUOUTLINECOL),
	MDEF_MENU("Tools", &menu_tools),
	MDEF_MENU("Console info", &menu_cinfo),
	MDEF_CAPTION("---------------", MENUOUTLINECOL),
	MDEF_HANDLER("Reboot (Normal)", reboot_normal),
	MDEF_HANDLER("Reboot (RCM)", reboot_rcm),
	MDEF_HANDLER("Power off", power_off),
	MDEF_CAPTION("---------------", MENUOUTLINECOL),
	MDEF_HANDLER("About", about),
	MDEF_END()
};

menu_t menu_top = { ment_top, "hekate - CTCaer mod v5.0.2", 0, 0 };

#define IPL_STACK_TOP  0x90010000
#define IPL_HEAP_START 0x90020000
#define IPL_HEAP_END   0xB5000000

extern void pivot_stack(u32 stack_top);

void ipl_main()
{
	// Do initial HW configuration. This is compatible with consecutive reruns without a reset.
	config_hw();

	//Pivot the stack so we have enough space.
	pivot_stack(IPL_STACK_TOP);

	//Tegra/Horizon configuration goes to 0x80000000+, package2 goes to 0xA9800000, we place our heap in between.
	heap_init(IPL_HEAP_START);

#ifdef DEBUG_UART_PORT
	uart_send(DEBUG_UART_PORT, (u8 *)"Hekate: Hello!\r\n", 16);
	uart_wait_idle(DEBUG_UART_PORT, UART_TX_IDLE);
#endif

	// Set bootloader's default configuration.
	set_default_configuration();

	sd_mount();

	// Save sdram lp0 config.
	if (!ianos_loader(false, "bootloader/sys/libsys_lp0.bso", DRAM_LIB, (void *)sdram_get_params_patched()))
		h_cfg.errors |= ERR_LIBSYS_LP0;

	// Train DRAM and switch to max frequency.
	minerva_init();
	minerva_change_freq(FREQ_1600);

	display_init();

	u32 *fb = display_init_framebuffer();
	gfx_init_ctxt(fb, 720, 1280, 720);

#ifdef MENU_LOGO_ENABLE
	Kc_MENU_LOGO = (u8 *)malloc(ALIGN(SZ_MENU_LOGO, 0x1000));
	blz_uncompress_srcdest(Kc_MENU_LOGO_blz, SZ_MENU_LOGO_BLZ, Kc_MENU_LOGO, SZ_MENU_LOGO);
#endif

	gfx_con_init();

	display_backlight_pwm_init();
	//display_backlight_brightness(h_cfg.backlight, 1000);

	// Overclock BPMP.
	bpmp_clk_rate_set(BPMP_CLK_SUPER_BOOST);

	// Check if we had a panic while in CFW.
	secmon_exo_check_panic();

	// Check if RCM is patched and protect from a possible brick.
	patched_rcm_protection();

	// Load emuMMC configuration from SD.
	emummc_load_cfg();

	// Load saved configuration and auto boot if enabled.
	auto_launch_firmware();

	minerva_change_freq(FREQ_800);

	while (true)
		tui_do_menu(&menu_top);

	// Halt BPMP if we managed to get out of execution.
	while (true)
		bpmp_halt();
}
