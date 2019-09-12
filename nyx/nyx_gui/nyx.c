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

#include "config/config.h"
#include "gfx/di.h"
#include "gfx/gfx.h"
#include "hos/hos.h"
#include "ianos/ianos.h"
#include "libs/compr/blz.h"
#include "libs/fatfs/ff.h"
#include "mem/heap.h"
#include "mem/minerva.h"
#include "mem/sdram.h"
#include "power/max77620.h"
#include "soc/bpmp.h"
#include "soc/fuse.h"
#include "soc/gpio.h"
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
#include "frontend/gui.h"

//TODO: ugly.
sdmmc_t sd_sdmmc;
sdmmc_storage_t sd_storage;
FATFS sd_fs;
static bool sd_mounted = false;
static bool sd_init_done = false;

#ifdef MENU_LOGO_ENABLE
u8 *Kc_MENU_LOGO;
#endif //MENU_LOGO_ENABLE

hekate_config h_cfg;

const volatile ipl_ver_meta_t __attribute__((section ("._ipl_version"))) ipl_ver = {
	.magic = BL_MAGIC,
	.version = (BL_VER_MJ + '0') | ((BL_VER_MN + '0') << 8) | ((BL_VER_HF + '0') << 16),
	.rsvd0 = 0,
	.rsvd1 = 0
};

volatile nyx_storage_t *nyx_str = (nyx_storage_t *)NYX_STORAGE_ADDR;
volatile boot_cfg_t *b_cfg;

bool get_sd_card_removed()
{
	if (sd_init_done && !!gpio_read(GPIO_PORT_Z, GPIO_PIN_1))
		return true;
	
	return false;
}

bool sd_mount()
{
	if (sd_mounted)
		return true;

	int res = 0;

	if (!sd_init_done)
	{
		res = !sdmmc_storage_init_sd(&sd_storage, &sd_sdmmc, SDMMC_1, SDMMC_BUS_WIDTH_4, 11);
		if (!res)
			sd_init_done = true;
	}

	if (res)
	{
		EPRINTF("Failed to init SD card.\nMake sure that it is inserted.\nOr that SD reader is properly seated!");
	}
	else
	{
		int res = f_mount(&sd_fs, "", 1);
		if (res == FR_OK)
		{
			sd_mounted = true;
			return true;
		}
		else
		{
			EPRINTFARGS("Failed to mount SD card (FatFS Error %d).\nMake sure that a FAT partition exists..", res);
		}
	}

	return false;
}

void sd_unmount(bool deinit)
{
	if (sd_init_done && sd_mounted)
	{
		f_mount(NULL, "", 1);
		sd_mounted = false;
	}
	if (sd_init_done && deinit)
	{
		sdmmc_storage_end(&sd_storage);
		sd_init_done = false;
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

#pragma GCC push_options
#pragma GCC target ("thumb")

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

// This is a safe and unused DRAM region for our payloads.
#define RELOC_META_OFF      0x7C
#define PATCHED_RELOC_SZ    0x94
#define PATCHED_RELOC_STACK 0x40007000
#define PATCHED_RELOC_ENTRY 0x40010000
#define EXT_PAYLOAD_ADDR    0xC0000000
#define RCM_PAYLOAD_ADDR    (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
#define COREBOOT_ADDR       (0xD0000000 - 0x100000)
#define CBFS_DRAM_EN_ADDR   0x4003e000
#define  CBFS_DRAM_MAGIC    0x4452414D // "DRAM"

void reloc_patcher(u32 payload_dst, u32 payload_src, u32 payload_size)
{
	memcpy((u8 *)payload_src, (u8 *)NYX_LOAD_ADDR, PATCHED_RELOC_SZ);

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

lv_res_t launch_payload(lv_obj_t *list)
{
	const char *filename = lv_list_get_btn_text(list);

	if (!filename || !filename[0])
		goto out;

	char path[128];
	
	strcpy(path,"bootloader/payloads/");
	strcat(path, filename);

	if (sd_mount())
	{
		FIL fp;
		if (f_open(&fp, path, FA_READ))
		{
			EPRINTFARGS("Payload file is missing!\n(%s)", path);
			sd_unmount(false);

			goto out;
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
			sd_unmount(false);

			goto out;
		}

		f_close(&fp);

		sd_unmount(true);

		if (size < 0x30000)
		{
			reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, ALIGN(size, 0x10));
			reconfig_hw_workaround(false, byte_swap_32(*(u32 *)(buf + size - sizeof(u32))));
		}
		else
		{
			reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, 0x7000);
			reconfig_hw_workaround(true, 0);
		}

		void (*ext_payload_ptr)() = (void *)EXT_PAYLOAD_ADDR;

		// Some cards (Sandisk U1), do not like a fast power cycle. Wait min 100ms.
		sdmmc_storage_init_wait_sd();

		// Launch our payload.
		(*ext_payload_ptr)();
	}

out:
	return LV_RES_OK;
}

void load_saved_configuration()
{
	LIST_INIT(ini_sections);

	if (ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false))
	{
		// Load configuration.
		LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
		{
			// Skip other ini entries.
			if (ini_sec->type == INI_CHOICE)
			{
				if (!strcmp(ini_sec->name, "config"))
				{
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
						{
							h_cfg.backlight = atoi(kv->val);
							if (h_cfg.backlight <= 20)
								h_cfg.backlight = 30;
						}
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

					continue;
				}
			}
		}
	}
}

void nyx_init_load_res()
{
	bpmp_mmu_enable();
	bpmp_clk_rate_set(BPMP_CLK_SUPER_BOOST);

	// Set bootloader's default configuration.
	set_default_configuration();

	gfx_init_ctxt((u32 *)FB_ADDRESS, 720, 1280, 720);
	gfx_con_init();

	sd_mount();

	minerva_init();
	minerva_change_freq(FREQ_1600);

	load_saved_configuration();
	
	FIL fp;
	f_open(&fp, "bootloader/sys/res.pak", FA_READ);
	f_read(&fp, (void *)NYX_RES_ADDR, f_size(&fp), NULL);
	f_close(&fp);

	icon_switch = bmp_to_lvimg_obj("bootloader/res/icon_switch.bmp");
	icon_payload = bmp_to_lvimg_obj("bootloader/res/icon_payload.bmp");
	icon_lakka = bmp_to_lvimg_obj("bootloader/res/icon_lakka.bmp");
	hekate_bg = bmp_to_lvimg_obj("bootloader/res/background.bmp");

	sd_unmount(false);

	h_cfg.rcm_patched = fuse_check_patched_rcm();
}

#define IPL_STACK_TOP  0x90010000
#define IPL_HEAP_START 0x90020000
#define IPL_HEAP_END   0xB5000000

extern void pivot_stack(u32 stack_top);

#if (LV_LOG_PRINTF == 1)
	#include "soc/clock.h"
	#include "soc/gpio.h"
	#include "soc/pinmux.h"
#endif

void ipl_main()
{
	//Tegra/Horizon configuration goes to 0x80000000+, package2 goes to 0xA9800000, we place our heap in between.
	heap_init(IPL_HEAP_START);

	b_cfg = (boot_cfg_t *)(nyx_str->hekate + 0x94);

#if (LV_LOG_PRINTF == 1)
	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);
	pinmux_config_uart(UART_B);
	clock_enable_uart(UART_B);
	uart_init(UART_B, 115200);

	uart_send(UART_B, (u8 *)"Hekate-NYX: Hello!\r\n", 20);
	uart_wait_idle(UART_B, UART_TX_IDLE);
#endif

	// Initialize the rest of hw and load nyx's resources.
	nyx_init_load_res();

	nyx_load_and_run();

	// Halt BPMP if we managed to get out of execution.
	while (true)
		bpmp_halt();
}

#pragma GCC pop_options
