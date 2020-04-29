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

#include "../../common/memory_map.h"

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
#include "storage/nx_sd.h"
#include "storage/sdmmc.h"
#include "utils/btn.h"
#include "utils/dirlist.h"
#include "utils/list.h"
#include "utils/util.h"

#include "frontend/fe_emmc_tools.h"
#include "frontend/gui.h"

#ifdef MENU_LOGO_ENABLE
u8 *Kc_MENU_LOGO;
#endif //MENU_LOGO_ENABLE

hekate_config h_cfg;

const volatile ipl_ver_meta_t __attribute__((section ("._ipl_version"))) ipl_ver = {
	.magic = NYX_MAGIC,
	.version = (NYX_VER_MJ + '0') | ((NYX_VER_MN + '0') << 8) | ((NYX_VER_HF + '0') << 16),
	.rsvd0 = 0,
	.rsvd1 = 0
};

volatile nyx_storage_t *nyx_str = (nyx_storage_t *)NYX_STORAGE_ADDR;
volatile boot_cfg_t *b_cfg;

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
		if (!sdmmc_storage_init_mmc(&storage2, &sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400))
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
#define COREBOOT_END_ADDR   0xD0000000
#define CBFS_DRAM_EN_ADDR   0x4003e000
#define  CBFS_DRAM_MAGIC    0x4452414D // "DRAM"

static void *coreboot_addr;

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
		memcpy((u8 *)(payload_src + ALIGN(PATCHED_RELOC_SZ, 0x10)), coreboot_addr, 0x7000); //Bootblock
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
		}

		if (f_read(&fp, buf, size, NULL))
		{
			f_close(&fp);

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
	sd_unmount(false);

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
						else if (!strcmp("updater2p", kv->key))
							h_cfg.updater2p = atoi(kv->val);
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

#define EXCP_EN_ADDR   0x4003FFFC
#define  EXCP_MAGIC 0x30505645 // EVP0
#define EXCP_TYPE_ADDR 0x4003FFF8
#define  EXCP_TYPE_RESET 0x545352 // RST
#define  EXCP_TYPE_UNDEF 0x464455 // UDF
#define  EXCP_TYPE_PABRT 0x54424150 // PABT
#define  EXCP_TYPE_DABRT 0x54424144 // DABT
#define EXCP_LR_ADDR   0x4003FFF4

static void _show_errors()
{
	u32 *excp_enabled = (u32 *)EXCP_EN_ADDR;
	u32 *excp_type = (u32 *)EXCP_TYPE_ADDR;
	u32 *excp_lr = (u32 *)EXCP_LR_ADDR;

	if (*excp_enabled == EXCP_MAGIC)
	{
		gfx_clear_grey(0);
		gfx_con_setpos(0, 0);
		display_backlight_brightness(100, 1000);

		WPRINTFARGS("An exception happened (LR %08X):\n", *excp_lr);
		switch (*excp_type)
		{
		case EXCP_TYPE_RESET:
			WPRINTF("Reset");
			break;
		case EXCP_TYPE_UNDEF:
			WPRINTF("Undefined instruction");
			break;
		case EXCP_TYPE_PABRT:
			WPRINTF("Prefetch abort");
			break;
		case EXCP_TYPE_DABRT:
			WPRINTF("Data abort");
			break;
		}
		WPRINTF("\n");

		// Clear the exception.
		*excp_lr = 0;
		*excp_type = 0;
		*excp_enabled = 0;

		WPRINTF("Press any key...");

		msleep(2000);
		btn_wait();

		reload_nyx();
	}
}

void nyx_init_load_res()
{
	bpmp_mmu_enable();
	bpmp_clk_rate_set(BPMP_CLK_DEFAULT_BOOST);

	// Set bootloader's default configuration.
	set_default_configuration();

	gfx_init_ctxt((u32 *)NYX_FB_ADDRESS, 720, 1280, 720);
	gfx_con_init();

	sd_mount();

	// Train DRAM and switch to max frequency.
	minerva_init();

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

	// Important: Preserve version header!
	__asm__ ("" : : "" (ipl_ver));

#if (LV_LOG_PRINTF == 1)
	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);
	pinmux_config_uart(UART_B);
	clock_enable_uart(UART_B);
	uart_init(UART_B, 115200);

	uart_send(UART_B, (u8 *)"hekate-NYX: Hello!\r\n", 20);
	uart_wait_idle(UART_B, UART_TX_IDLE);
#endif

	// Initialize the rest of hw and load nyx's resources.
	nyx_init_load_res();

	nyx_load_and_run();

	// Halt BPMP if we managed to get out of execution.
	while (true)
		bpmp_halt();
}
