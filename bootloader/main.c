/*
 * Copyright (c) 2018 naehrwert
 *
 * Copyright (c) 2018 Rajko Stojadinovic
 * Copyright (c) 2018 CTCaer
 * Copyright (c) 2018 Reisyukaku
 * Copyright (c) 2018 balika011
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

#include "soc/clock.h"
#include "soc/uart.h"
#include "soc/i2c.h"
#include "mem/sdram.h"
#include "gfx/di.h"
#include "mem/mc.h"
#include "soc/t210.h"
#include "soc/pmc.h"
#include "soc/pinmux.h"
#include "soc/fuse.h"
#include "utils/util.h"
#include "gfx/gfx.h"
#include "utils/btn.h"
#include "sec/tsec.h"
#include "soc/kfuse.h"
#include "power/max77620.h"
#include "power/max7762x.h"
#include "soc/gpio.h"
#include "storage/sdmmc.h"
#include "libs/fatfs/ff.h"
#include "gfx/logos.h"
#include "gfx/tui.h"
#include "mem/heap.h"
#include "utils/list.h"
#include "storage/nx_emmc.h"
#include "sec/se.h"
#include "sec/se_t210.h"
#include "hos/hos.h"
#include "hos/pkg1.h"
#include "hos/pkg2.h"
#include "storage/mmc.h"
#include "libs/compr/blz.h"
#include "power/max17050.h"
#include "power/bq24193.h"
#include "config/config.h"
#include "ianos/ianos.h"
#include "utils/dirlist.h"

#define BOOTLOADER_UPDATED_MAGIC_ADDR 0x4003E000
#define BOOTLOADER_UPDATED_MAGIC 0x424f4f54

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
	TMR(0x18C) = 0xC45A;
	TMR(0x80) = 0xC0000000;
	TMR(0x180) = 0x8019;
	TMR(0x188) = 1;
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
	PMC(0) |= 0x10;
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

		gfx_clear_grey(&gfx_ctxt, 0x1B);
		u8 *BOOTLOGO = (void *)malloc(0x4000);
		blz_uncompress_srcdest(BOOTLOGO_BLZ, SZ_BOOTLOGO_BLZ, BOOTLOGO, SZ_BOOTLOGO);
		gfx_set_rect_grey(&gfx_ctxt, BOOTLOGO, X_BOOTLOGO, Y_BOOTLOGO, 326, 544);

		display_backlight_brightness(10, 5000);
		display_backlight_brightness(100, 25000);
		usleep(600000);
		display_backlight_brightness(0, 20000);

		power_off();
	}
}

void config_oscillators()
{
	CLOCK(CLK_RST_CONTROLLER_SPARE_REG0) = (CLOCK(CLK_RST_CONTROLLER_SPARE_REG0) & 0xFFFFFFF3) | 4;
	SYSCTR0(SYSCTR0_CNTFID0) = 19200000;
	TMR(0x14) = 0x45F;
	CLOCK(CLK_RST_CONTROLLER_OSC_CTRL) = 0x50000071;
	PMC(APBDEV_PMC_OSC_EDPD_OVER) = (PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFFFFF81) | 0xE;
	PMC(APBDEV_PMC_OSC_EDPD_OVER) = (PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFBFFFFF) | 0x400000;
	PMC(APBDEV_PMC_CNTRL2) = (PMC(APBDEV_PMC_CNTRL2) & 0xFFFFEFFF) | 0x1000;
	PMC(APBDEV_PMC_SCRATCH188) = (PMC(APBDEV_PMC_SCRATCH188) & 0xFCFFFFFF) | 0x2000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 0x10;
	CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE) &= 0xBFFFFFFF;
	PMC(APBDEV_PMC_TSC_MULT) = (PMC(APBDEV_PMC_TSC_MULT) & 0xFFFF0000) | 0x249F; //0x249F = 19200000 * (16 / 32.768 kHz)
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20004444;
	CLOCK(CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER) = 0x80000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 2;
}

void config_gpios()
{
	PINMUX_AUX(PINMUX_AUX_UART2_TX) = 0;
	PINMUX_AUX(PINMUX_AUX_UART3_TX) = 0;

	PINMUX_AUX(PINMUX_AUX_GPIO_PE6) = PINMUX_INPUT_ENABLE;
	PINMUX_AUX(PINMUX_AUX_GPIO_PH6) = PINMUX_INPUT_ENABLE;

	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_G, GPIO_PIN_0, GPIO_OUTPUT_DISABLE);
	gpio_output_enable(GPIO_PORT_D, GPIO_PIN_1, GPIO_OUTPUT_DISABLE);
	gpio_output_enable(GPIO_PORT_E, GPIO_PIN_6, GPIO_OUTPUT_DISABLE);
	gpio_output_enable(GPIO_PORT_H, GPIO_PIN_6, GPIO_OUTPUT_DISABLE);

	pinmux_config_i2c(I2C_1);
	pinmux_config_i2c(I2C_5);
	pinmux_config_uart(UART_A);

	// Configure volume up/down as inputs.
	gpio_config(GPIO_PORT_X, GPIO_PIN_6, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_X, GPIO_PIN_7, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_X, GPIO_PIN_6, GPIO_OUTPUT_DISABLE);
	gpio_output_enable(GPIO_PORT_X, GPIO_PIN_7, GPIO_OUTPUT_DISABLE);
}

void config_pmc_scratch()
{
	PMC(APBDEV_PMC_SCRATCH20) &= 0xFFF3FFFF;
	PMC(APBDEV_PMC_SCRATCH190) &= 0xFFFFFFFE;
	PMC(APBDEV_PMC_SECURE_SCRATCH21) |= 0x10;
}

void mbist_workaround()
{
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) | 0x8000) & 0xFFFFBFFF;
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) |= 0x40800000u;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_CLR) = 0x40;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_X_CLR) = 0x40000;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = 0x18000000;
	usleep(2);

	I2S(0x0A0) |= 0x400;
	I2S(0x088) &= 0xFFFFFFFE;
	I2S(0x1A0) |= 0x400;
	I2S(0x188) &= 0xFFFFFFFE;
	I2S(0x2A0) |= 0x400;
	I2S(0x288) &= 0xFFFFFFFE;
	I2S(0x3A0) |= 0x400;
	I2S(0x388) &= 0xFFFFFFFE;
	I2S(0x4A0) |= 0x400;
	I2S(0x488) &= 0xFFFFFFFE;
	DISPLAY_A(_DIREG(DC_COM_DSC_TOP_CTL)) |= 4;
	VIC(0x8C) = 0xFFFFFFFF;
	usleep(2);

	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_SET) = 0x40;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = 0x18000000;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_X_SET) = 0x40000;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_H) = 0xC0;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) = 0x80000130;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_U) = 0x1F00200;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) = 0x80400808;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_W) = 0x402000FC;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X) = 0x23000780;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) = 0x300;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE) = 0;
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) &= 0x1F7FFFFF;
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) &= 0xFFFF3FFF;
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_VI) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_VI) & 0x1FFFFFFF) | 0x80000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X) & 0x1FFFFFFF) | 0x80000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_NVENC) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_NVENC) & 0x1FFFFFFF) | 0x80000000;
}

void config_se_brom()
{
	// Bootrom part we skipped.
	u32 sbk[4] = { FUSE(0x1A4), FUSE(0x1A8), FUSE(0x1AC), FUSE(0x1B0) };
	se_aes_key_set(14, sbk, 0x10);
	// Lock SBK from being read.
	SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 14 * 4) = 0x7E;
	// This memset needs to happen here, else TZRAM will behave weirdly later on.
	memset((void *)0x7C010000, 0, 0x10000);
	PMC(APBDEV_PMC_CRYPTO_OP) = 0;
	SE(SE_INT_STATUS_REG_OFFSET) = 0x1F;
	// Lock SSK (although it's not set and unused anyways).
	SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 15 * 4) = 0x7E;
	// Clear the boot reason to avoid problems later
	PMC(APBDEV_PMC_SCRATCH200) = 0x0;
	PMC(APBDEV_PMC_RST_STATUS) = 0x0;
	APB_MISC(APB_MISC_PP_STRAPPING_OPT_A) = 0x1C00;
}

void config_hw()
{
	// Bootrom stuff we skipped by going through rcm.
	config_se_brom();
	//FUSE(FUSE_PRIVATEKEYDISABLE) = 0x11;
	SYSREG(AHB_AHB_SPARE_REG) &= 0xFFFFFF9F;
	PMC(APBDEV_PMC_SCRATCH49) = ((PMC(APBDEV_PMC_SCRATCH49) >> 1) << 1) & 0xFFFFFFFD;

	mbist_workaround();
	clock_enable_se();

	// Enable fuse clock.
	clock_enable_fuse(true);
	// Disable fuse programming.
	fuse_disable_program();

	mc_enable();

	config_oscillators();
	APB_MISC(APB_MISC_PP_PINMUX_GLOBAL) = 0;
	config_gpios();

	//clock_enable_uart(UART_C);
	//uart_init(UART_C, 115200);

	clock_enable_cl_dvfs();

	clock_enable_i2c(I2C_1);
	clock_enable_i2c(I2C_5);

	clock_enable_unk2();

	i2c_init(I2C_1);
	i2c_init(I2C_5);

	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CNFGBBC, 0x40);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, 0x78);

	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG0, 0x38);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG1, 0x3A);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG2, 0x38);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_LDO4, 0xF);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_LDO8, 0xC7);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_SD0, 0x4F);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_SD1, 0x29);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_SD3, 0x1B);

	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_GPIO3, 0x22); // 3.x+

	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_SD0, 42); //42 = (1125000 - 600000) / 12500 -> 1.125V

	config_pmc_scratch(); // Missing from 4.x+

	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = (CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) & 0xFFFF8888) | 0x3333;

	mc_config_carveout(); // Missing from 4.x+

	sdram_init();
}

void reconfig_hw_workaround(bool extra_reconfig, u32 magic)
{
	// Re-enable clocks to Audio Processing Engine as a workaround to hanging.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) |= 0x400; // Enable AHUB clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) |= 0x40;  // Enable APE clock.

	if (extra_reconfig)
	{
		msleep(10);
		PMC(APBDEV_PMC_PWR_DET_VAL) |= (1 << 12);

		clock_disable_cl_dvfs();

		// Disable Joy-con GPIOs.
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_SPIO);
	}

	// Power off display.
	display_end();

	// Enable clock to USBD and init SDMMC1 to avoid hangs with bad hw inits.
	if (magic == 0xBAADF00D)
	{
		CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) |= (1 << 22);
		sdmmc_init(&sd_sdmmc, SDMMC_1, SDMMC_POWER_3_3, SDMMC_BUS_WIDTH_1, 5, 0);
		clock_disable_cl_dvfs();

		msleep(200);
	}
}

void print_fuseinfo()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	u32 burntFuses = 0;
	for (u32 i = 0; i < 32; i++)
	{
		if ((fuse_read_odm(7) >> i) & 1)
			burntFuses++;
	}

	gfx_printf(&gfx_con, "\nSKU:         %X - ", FUSE(0x110));
	switch (fuse_read_odm(4) & 3)
	{
	case 0:
		gfx_printf(&gfx_con, "Retail\n");
		break;
	case 3:
		gfx_printf(&gfx_con, "Dev\n");
		break;
	}
	gfx_printf(&gfx_con, "Sdram ID:    %d\n", (fuse_read_odm(4) >> 3) & 0x1F);
	gfx_printf(&gfx_con, "Burnt fuses: %d\n", burntFuses);
	gfx_printf(&gfx_con, "Secure key:  %08X%08X%08X%08X\n\n\n",
		byte_swap_32(FUSE(0x1A4)), byte_swap_32(FUSE(0x1A8)), byte_swap_32(FUSE(0x1AC)), byte_swap_32(FUSE(0x1B0)));

	gfx_printf(&gfx_con, "%k(Unlocked) fuse cache:\n\n%k", 0xFF00DDFF, 0xFFCCCCCC);
	gfx_hexdump(&gfx_con, 0x7000F900, (u8 *)0x7000F900, 0x2FC);

	gfx_puts(&gfx_con, "Press POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "fuses.bin", NULL);
			if (!sd_save_to_file((u8 *)0x7000F900, 0x2FC, path))
				gfx_puts(&gfx_con, "\nDone!\n");
			sd_unmount();
		}

		btn_wait();
	}
}

void print_kfuseinfo()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, "%kKFuse contents:\n\n%k", 0xFF00DDFF, 0xFFCCCCCC);
	u32 buf[KFUSE_NUM_WORDS];
	if (!kfuse_read(buf))
		EPRINTF("CRC fail.");
	else
		gfx_hexdump(&gfx_con, 0, (u8 *)buf, KFUSE_NUM_WORDS * 4);

	gfx_puts(&gfx_con, "\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "kfuses.bin", NULL);
			if (!sd_save_to_file((u8 *)buf, KFUSE_NUM_WORDS * 4, path))
				gfx_puts(&gfx_con, "\nDone!\n");
			sd_unmount();
		}

		btn_wait();
	}
}

void print_mmc_info()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	static const u32 SECTORS_TO_MIB_COEFF = 11;

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}
	else
	{
		u16 card_type;
		u32 speed;

		gfx_printf(&gfx_con, "%kCID:%k\n", 0xFF00DDFF, 0xFFCCCCCC);
		switch (storage.csd.mmca_vsn)
		{
		case 0: /* MMC v1.0 - v1.2 */
		case 1: /* MMC v1.4 */
			gfx_printf(&gfx_con,
				" Vendor ID:  %03X\n"
				" Model:      %c%c%c%c%c%c%c\n"
				" HW rev:     %X\n"
				" FW rev:     %X\n"
				" S/N:        %03X\n"
				" Month/Year: %02d/%04d\n\n",
				storage.cid.manfid,
				storage.cid.prod_name[0], storage.cid.prod_name[1],	storage.cid.prod_name[2],
				storage.cid.prod_name[3], storage.cid.prod_name[4],	storage.cid.prod_name[5],
				storage.cid.prod_name[6], storage.cid.hwrev, storage.cid.fwrev,
				storage.cid.serial, storage.cid.month, storage.cid.year);
			break;
		case 2: /* MMC v2.0 - v2.2 */
		case 3: /* MMC v3.1 - v3.3 */
		case 4: /* MMC v4 */
			gfx_printf(&gfx_con,
				" Vendor ID:  %X\n"
				" Card/BGA:   %X\n"
				" OEM ID:     %02X\n"
				" Model:      %c%c%c%c%c%c\n"
				" Prd Rev:    %X\n"
				" S/N:        %04X\n"
				" Month/Year: %02d/%04d\n\n",
				storage.cid.manfid, storage.cid.card_bga, storage.cid.oemid,
				storage.cid.prod_name[0], storage.cid.prod_name[1], storage.cid.prod_name[2],
				storage.cid.prod_name[3], storage.cid.prod_name[4],	storage.cid.prod_name[5],
				storage.cid.prv, storage.cid.serial, storage.cid.month, storage.cid.year);
			break;
		default:
			EPRINTFARGS("eMMC has unknown MMCA version %d", storage.csd.mmca_vsn);
			break;
		}

		if (storage.csd.structure == 0)
			EPRINTF("Unknown CSD structure.");
		else
		{
			gfx_printf(&gfx_con, "%kExtended CSD V1.%d:%k\n",
				0xFF00DDFF, storage.ext_csd.ext_struct, 0xFFCCCCCC);
			card_type = storage.ext_csd.card_type;
			u8 card_type_support[96];
			u8 pos_type = 0;
			card_type_support[0] = 0;
			if (card_type & EXT_CSD_CARD_TYPE_HS_26)
			{
				memcpy(card_type_support, "HS26", 4);
				speed = (26 << 16) | 26;
				pos_type += 4;
			}
			if (card_type & EXT_CSD_CARD_TYPE_HS_52)
			{
				memcpy(card_type_support + pos_type, ", HS52", 6);
				speed = (52 << 16) | 52;
				pos_type += 6;
			}
			if (card_type & EXT_CSD_CARD_TYPE_DDR_1_8V)
			{
				memcpy(card_type_support + pos_type, ", DDR52_1.8V", 12);
				speed = (52 << 16) | 104;
				pos_type += 12;
			}
			if (card_type & EXT_CSD_CARD_TYPE_HS200_1_8V)
			{
				memcpy(card_type_support + pos_type, ", HS200_1.8V", 12);
				speed = (200 << 16) | 200;
				pos_type += 12;
			}
			if (card_type & EXT_CSD_CARD_TYPE_HS400_1_8V)
			{
				memcpy(card_type_support + pos_type, ", HS400_1.8V", 12);
				speed = (200 << 16) | 400;
				pos_type += 12;
			}
			card_type_support[pos_type] = 0;

			gfx_printf(&gfx_con,
				" Spec Version:  %02X\n"
				" Extended Rev:  1.%d\n"
				" Dev Version:   %d\n"
				" Cmd Classes:   %02X\n"
				" Capacity:      %s\n"
				" Max Rate:      %d MB/s (%d MHz)\n"
				" Current Rate:  %d MB/s\n"
				" Type Support:  ",
				storage.csd.mmca_vsn, storage.ext_csd.rev, storage.ext_csd.dev_version, storage.csd.cmdclass,
				storage.csd.capacity == (4096 * 512) ? "High" : "Low", speed & 0xFFFF, (speed >> 16) & 0xFFFF,
				storage.csd.busspeed);
			gfx_con.fntsz = 8;
			gfx_printf(&gfx_con, "%s", card_type_support);
			gfx_con.fntsz = 16;
			gfx_printf(&gfx_con, "\n\n", card_type_support);

			u32 boot_size = storage.ext_csd.boot_mult << 17;
			u32 rpmb_size = storage.ext_csd.rpmb_mult << 17;
			gfx_printf(&gfx_con, "%keMMC Partitions:%k\n", 0xFF00DDFF, 0xFFCCCCCC);
			gfx_printf(&gfx_con, " 1: %kBOOT0      %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF96FF00, 0xFFCCCCCC,
				boot_size / 1024, boot_size / 1024 / 512);
			gfx_put_small_sep(&gfx_con);
			gfx_printf(&gfx_con, " 2: %kBOOT1      %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF96FF00, 0xFFCCCCCC,
				boot_size / 1024, boot_size / 1024 / 512);
			gfx_put_small_sep(&gfx_con);
			gfx_printf(&gfx_con, " 3: %kRPMB       %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF96FF00, 0xFFCCCCCC,
				rpmb_size / 1024, rpmb_size / 1024 / 512);
			gfx_put_small_sep(&gfx_con);
			gfx_printf(&gfx_con, " 0: %kGPP (USER) %k\n    Size: %5d MiB (LBA Sectors: 0x%07X)\n\n", 0xFF96FF00, 0xFFCCCCCC,
				storage.sec_cnt >> SECTORS_TO_MIB_COEFF, storage.sec_cnt);
			gfx_put_small_sep(&gfx_con);
			gfx_printf(&gfx_con, "%kGPP (eMMC USER) partition table:%k\n", 0xFF00DDFF, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&storage, 0);
			LIST_INIT(gpt);
			nx_emmc_gpt_parse(&gpt, &storage);
			int gpp_idx = 0;
			LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
			{
				gfx_printf(&gfx_con, " %02d: %k%s%k\n     Size: % 5d MiB (LBA Sectors 0x%07X)\n     LBA Range: %08X-%08X\n",
					gpp_idx++, 0xFFAEFD14, part->name, 0xFFCCCCCC, (part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
					part->lba_end - part->lba_start + 1, part->lba_start, part->lba_end);
				gfx_put_small_sep(&gfx_con);
			}
			nx_emmc_gpt_free(&gpt);
		}
	}

out:
	sdmmc_storage_end(&storage);

	btn_wait();
}

void print_sdcard_info()
{
	static const u32 SECTORS_TO_MIB_COEFF = 11;

	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (sd_mount())
	{
		u32 capacity;

		gfx_printf(&gfx_con, "%kCard IDentification:%k\n", 0xFF00DDFF, 0xFFCCCCCC);
		gfx_printf(&gfx_con,
			" Vendor ID:  %02x\n"
			" OEM ID:     %c%c\n"
			" Model:      %c%c%c%c%c\n"
			" HW rev:     %X\n"
			" FW rev:     %X\n"
			" S/N:        %08x\n"
			" Month/Year: %02d/%04d\n\n",
			sd_storage.cid.manfid, (sd_storage.cid.oemid >> 8) & 0xFF, sd_storage.cid.oemid & 0xFF,
			sd_storage.cid.prod_name[0], sd_storage.cid.prod_name[1], sd_storage.cid.prod_name[2],
			sd_storage.cid.prod_name[3], sd_storage.cid.prod_name[4],
			sd_storage.cid.hwrev, sd_storage.cid.fwrev, sd_storage.cid.serial,
			sd_storage.cid.month, sd_storage.cid.year);

		gfx_printf(&gfx_con, "%kCard-Specific Data V%d.0:%k\n", 0xFF00DDFF, sd_storage.csd.structure + 1, 0xFFCCCCCC);
		capacity = sd_storage.csd.capacity >> (20 - sd_storage.csd.read_blkbits);
		gfx_printf(&gfx_con,
			" Cmd Classes:    %02X\n"
			" Capacity:       %d MiB\n"
			" Bus Width:      %d\n"
			" Current Rate:   %d MB/s (%d MHz)\n"
			" Speed Class:    %d\n"
			" UHS Grade:      U%d\n"
			" Video Class:    V%d\n"
			" App perf class: A%d\n"
			" Write Protect:  %d\n\n",
			sd_storage.csd.cmdclass, capacity,
			sd_storage.ssr.bus_width, sd_storage.csd.busspeed, sd_storage.csd.busspeed * 2,
			sd_storage.ssr.speed_class, sd_storage.ssr.uhs_grade, sd_storage.ssr.video_class,
			sd_storage.ssr.app_class, sd_storage.csd.write_protect);

		gfx_puts(&gfx_con, "Acquiring FAT volume info...\n\n");
		f_getfree("", &sd_fs.free_clst, NULL);
		gfx_printf(&gfx_con, "%kFound %s volume:%k\n Free:    %d MiB\n Cluster: %d KiB\n",
				0xFF00DDFF, sd_fs.fs_type == FS_EXFAT ? "exFAT" : "FAT32", 0xFFCCCCCC,
				sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF, (sd_fs.csize > 1) ? (sd_fs.csize >> 1) : 512);
		sd_unmount();
	}

	btn_wait();
}

void print_tsec_key()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	// Read package1.
	u8 *pkg1 = (u8 *)malloc(0x40000);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	sdmmc_storage_end(&storage);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1);
	if (!pkg1_id)
	{
		EPRINTFARGS("Unknown package1 version for reading\nTSEC firmware (= '%s').",
			(char *)pkg1 + 0x10);
		goto out_wait;
	}

	u8 keys[0x10 * 3];
	for (u32 i = 1; i <= 3; i++)
	{
		int res = tsec_query(keys + ((i - 1) * 0x10), i, pkg1 + pkg1_id->tsec_off);

		gfx_printf(&gfx_con, "%kTSEC key %d: %k", 0xFF00DDFF, i, 0xFFCCCCCC);
		if (res >= 0)
		{
			for (u32 j = 0; j < 0x10; j++)
				gfx_printf(&gfx_con, "%02X", keys[((i - 1) * 0x10) + j]);
		}
		else
			EPRINTFARGS("ERROR %X", res);
		gfx_putc(&gfx_con, '\n');
	}

	gfx_puts(&gfx_con, "\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "tsec_keys.bin", NULL);
			if (!sd_save_to_file(keys, 0x10 * 3, path))
				gfx_puts(&gfx_con, "\nDone!\n");
			sd_unmount();
		}
	}
	else
		goto out;

out_wait:
	btn_wait();

out:
	free(pkg1);
}

int dump_emmc_verify(sdmmc_storage_t *storage, u32 lba_curr, char *outFilename, emmc_part_t *part)
{
	FIL fp;
	u32 prevPct = 200;
	int res = 0;

	u8 hashEm[0x20];
	u8 hashSd[0x20];

	if (f_open(&fp, outFilename, FA_READ) == FR_OK)
	{
		u32 totalSectorsVer = (u32)((u64)f_size(&fp) >> (u64)9);

		u32 numSectorsPerIter = 0;
		if (totalSectorsVer > 0x200000)
			numSectorsPerIter = 8192; //4MB Cache
		else
			numSectorsPerIter = 512;  //256KB Cache

		u8 *bufEm = (u8 *)calloc(numSectorsPerIter, NX_EMMC_BLOCKSIZE);
		u8 *bufSd = (u8 *)calloc(numSectorsPerIter, NX_EMMC_BLOCKSIZE);

		u32 pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		tui_pbar(&gfx_con, 0, gfx_con.y, pct, 0xFF96FF00, 0xFF155500);

		u32 num = 0;
		while (totalSectorsVer > 0)
		{
			num = MIN(totalSectorsVer, numSectorsPerIter);

			if (!sdmmc_storage_read(storage, lba_curr, num, bufEm))
			{
				gfx_con.fntsz = 16;
				EPRINTFARGS("\nFailed to read %d blocks (@LBA %08X),\nfrom eMMC!\n\nVerification failed..\n",
					num, lba_curr);

				free(bufEm);
				free(bufSd);
				f_close(&fp);
				return 1;
			}
			if (f_read(&fp, bufSd, num << 9, NULL))
			{
				gfx_con.fntsz = 16;
				EPRINTFARGS("\nFailed to read %d blocks (@LBA %08X),\nfrom sd card!\n\nVerification failed..\n", num, lba_curr);

				free(bufEm);
				free(bufSd);
				f_close(&fp);
				return 1;
			}

			switch (h_cfg.verification)
			{
			case 1:
				res = memcmp32sparse((u32 *)bufEm, (u32 *)bufSd, num << 9);
				break;
			case 2:
			default:
				se_calc_sha256(&hashEm, bufEm, num << 9);
				se_calc_sha256(&hashSd, bufSd, num << 9);
				res = memcmp(hashEm, hashSd, 0x20);
				break;
			}
			if (res)
			{
				gfx_con.fntsz = 16;
				EPRINTFARGS("\nSD card and eMMC data (@LBA %08X),\ndo not match!\n\nVerification failed..\n", lba_curr);

				free(bufEm);
				free(bufSd);
				f_close(&fp);
				return 1;
			}

			pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
			if (pct != prevPct)
			{
				tui_pbar(&gfx_con, 0, gfx_con.y, pct, 0xFF96FF00, 0xFF155500);
				prevPct = pct;
			}

			lba_curr += num;
			totalSectorsVer -= num;
		}
		free(bufEm);
		free(bufSd);
		f_close(&fp);

		tui_pbar(&gfx_con, 0, gfx_con.y, pct, 0xFFCCCCCC, 0xFF555555);

		return 0;
	}
	else
	{
		gfx_con.fntsz = 16;
		EPRINTF("\nFile not found or could not be loaded.\n\nVerification failed..\n");
		return 1;
	}
}

int dump_emmc_part(char *sd_path, sdmmc_storage_t *storage, emmc_part_t *part)
{
	static const u32 FAT32_FILESIZE_LIMIT = 0xFFFFFFFF;
	static const u32 SECTORS_TO_MIB_COEFF = 11;

	u32 multipartSplitSize = (1u << 31);
	u32 totalSectors = part->lba_end - part->lba_start + 1;
	u32 currPartIdx = 0;
	u32 numSplitParts = 0;
	u32 maxSplitParts = 0;
	u32 btn = 0;
	bool isSmallSdCard = false;
	bool partialDumpInProgress = false;
	int res = 0;
	char *outFilename = sd_path;
	u32 sdPathLen = strlen(sd_path);

	FIL partialIdxFp;
	char partialIdxFilename[12];
	memcpy(partialIdxFilename, "partial.idx", 12);

	gfx_con.fntsz = 8;
	gfx_printf(&gfx_con, "\nSD Card free space: %d MiB, Total backup size %d MiB\n\n",
		sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF,
		totalSectors >> SECTORS_TO_MIB_COEFF);

	// 1GB parts for sd cards 8GB and less.
	if ((sd_storage.csd.capacity >> (20 - sd_storage.csd.read_blkbits)) <= 8192)
		multipartSplitSize = (1u << 30);
	// Maximum parts fitting the free space available.
	maxSplitParts = (sd_fs.free_clst * sd_fs.csize) / (multipartSplitSize / 512);

	// Check if the USER partition or the RAW eMMC fits the sd card free space.
	if (totalSectors > (sd_fs.free_clst * sd_fs.csize))
	{
		isSmallSdCard = true;

		gfx_printf(&gfx_con, "%k\nSD card free space is smaller than total backup size.%k\n", 0xFFFFBA00, 0xFFCCCCCC);

		if (!maxSplitParts)
		{
			gfx_con.fntsz = 16;
			EPRINTF("Not enough free space for Partial Backup.");

			return 0;
		}
	}
	// Check if we are continuing a previous raw eMMC or USER partition backup in progress.
	if (f_open(&partialIdxFp, partialIdxFilename, FA_READ) == FR_OK && totalSectors > (FAT32_FILESIZE_LIMIT / NX_EMMC_BLOCKSIZE))
	{
		gfx_printf(&gfx_con, "%kFound Partial Backup in progress. Continuing...%k\n\n", 0xFFAEFD14, 0xFFCCCCCC);

		partialDumpInProgress = true;
		// Force partial dumping, even if the card is larger.
		isSmallSdCard = true;

		f_read(&partialIdxFp, &currPartIdx, 4, NULL);
		f_close(&partialIdxFp);

		if (!maxSplitParts)
		{
			gfx_con.fntsz = 16;
			EPRINTF("Not enough free space for Partial Backup.");

			return 0;
		}

		// Increase maxSplitParts to accommodate previously backed up parts.
		maxSplitParts += currPartIdx;
	}
	else if (isSmallSdCard)
		gfx_printf(&gfx_con, "%kPartial Backup enabled (with %d MiB parts)...%k\n\n", 0xFFFFBA00, multipartSplitSize >> 20, 0xFFCCCCCC);

	// Check if filesystem is FAT32 or the free space is smaller and backup in parts.
	if (((sd_fs.fs_type != FS_EXFAT) && totalSectors > (FAT32_FILESIZE_LIMIT / NX_EMMC_BLOCKSIZE)) | isSmallSdCard)
	{
		u32 multipartSplitSectors = multipartSplitSize / NX_EMMC_BLOCKSIZE;
		numSplitParts = (totalSectors + multipartSplitSectors - 1) / multipartSplitSectors;

		outFilename[sdPathLen++] = '.';

		if (!partialDumpInProgress)
		{
			outFilename[sdPathLen] = '0';
			if (numSplitParts >= 10)
			{
				outFilename[sdPathLen + 1] = '0';
				outFilename[sdPathLen + 2] = 0;
			}
			else
				outFilename[sdPathLen + 1] = 0;
		}
		// Continue from where we left, if Partial Backup in progress.
		else
		{
			if (numSplitParts >= 10 && currPartIdx < 10)
			{
				outFilename[sdPathLen] = '0';
				itoa(currPartIdx, &outFilename[sdPathLen + 1], 10);
			}
			else
				itoa(currPartIdx, &outFilename[sdPathLen], 10);
		}
	}

	FIL fp;
	gfx_con_getpos(&gfx_con, &gfx_con.savedx, &gfx_con.savedy);
	if (!f_open(&fp, outFilename, FA_READ))
	{
		f_close(&fp);
		gfx_con.fntsz = 16;
		
		WPRINTF("An existing backup has been detected!");
		WPRINTF("Press POWER to Continue.\nPress VOL to go to the menu.\n");
		msleep(500);

		u32 btn = btn_wait();
		if (btn & BTN_POWER)
			btn = 0;
		else
			return 0;
		gfx_con.fntsz = 8;
		gfx_clear_partial_grey(&gfx_ctxt, 0x1B, gfx_con.savedy, 48);
	}
	gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);
	gfx_printf(&gfx_con, "Filename: %s\n\n", outFilename);
	res = f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res)
	{
		gfx_con.fntsz = 16;
		EPRINTFARGS("Error (%d) creating file %s.\n", res, outFilename);

		return 0;
	}

	u32 numSectorsPerIter = 0;
	if (totalSectors > 0x200000)
		numSectorsPerIter = 8192;
	else
		numSectorsPerIter = 512;
	u8 *buf = (u8 *)calloc(numSectorsPerIter, NX_EMMC_BLOCKSIZE);

	u32 lba_curr = part->lba_start;
	u32 lbaStartPart = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;

	// Continue from where we left, if Partial Backup in progress.
	if (partialDumpInProgress)
	{
		lba_curr += currPartIdx * (multipartSplitSize / NX_EMMC_BLOCKSIZE);
		totalSectors -= currPartIdx * (multipartSplitSize / NX_EMMC_BLOCKSIZE);
		lbaStartPart = lba_curr; // Update the start LBA for verification.
	}
	u64 totalSize = (u64)((u64)totalSectors << 9);
	if (!isSmallSdCard && sd_fs.fs_type == FS_EXFAT)
		f_lseek(&fp, totalSize);
	else
		f_lseek(&fp, MIN(totalSize, multipartSplitSize));
	f_lseek(&fp, 0);

	u32 num = 0;
	u32 pct = 0;
	while (totalSectors > 0)
	{
		if (numSplitParts != 0 && bytesWritten >= multipartSplitSize)
		{
			f_close(&fp);
			memset(&fp, 0, sizeof(fp));
			currPartIdx++;

			if (h_cfg.verification)
			{
				// Verify part.
				if (dump_emmc_verify(storage, lbaStartPart, outFilename, part))
				{
					EPRINTF("\nPress any key and try again...\n");

					free(buf);
					return 0;
				}
			}

			if (numSplitParts >= 10 && currPartIdx < 10)
			{
				outFilename[sdPathLen] = '0';
				itoa(currPartIdx, &outFilename[sdPathLen + 1], 10);
			}
			else
				itoa(currPartIdx, &outFilename[sdPathLen], 10);

			// Always create partial.idx before next part, in case a fatal error occurs.
			if (isSmallSdCard)
			{
				// Create partial backup index file.
				if (f_open(&partialIdxFp, partialIdxFilename, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
				{
					f_write(&partialIdxFp, &currPartIdx, 4, NULL);
					f_close(&partialIdxFp);
				}
				else
				{
					gfx_con.fntsz = 16;
					EPRINTF("\nError creating partial.idx file.\n");

					free(buf);
					return 0;
				}

				// More parts to backup that do not currently fit the sd card free space or fatal error.
				if (currPartIdx >= maxSplitParts)
				{
					gfx_puts(&gfx_con, "\n\n1. Press any key to unmount SD Card.\n\
						2. Remove SD Card and move files to free space.\n\
						   Don\'t move the partial.idx file!\n\
						3. Re-insert SD Card.\n\
						4. Select the SAME option again to continue.\n");
					gfx_con.fntsz = 16;

					free(buf);
					return 1;
				}
			}

			// Create next part.
			gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);
			gfx_printf(&gfx_con, "Filename: %s\n\n", outFilename);
			lbaStartPart = lba_curr;
			res = f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE);
			if (res)
			{
				gfx_con.fntsz = 16;
				EPRINTFARGS("Error (%d) creating file %s.\n", res, outFilename);

				free(buf);
				return 0;
			}
			bytesWritten = 0;

			totalSize = (u64)((u64)totalSectors << 9);
			f_lseek(&fp, MIN(totalSize, multipartSplitSize));
			f_lseek(&fp, 0);
		}

		retryCount = 0;
		num = MIN(totalSectors, numSectorsPerIter);
		while (!sdmmc_storage_read(storage, lba_curr, num, buf))
		{
			EPRINTFARGS("Error reading %d blocks @ LBA %08X,\nfrom eMMC (try %d), retrying...",
				num, lba_curr, ++retryCount);

			msleep(150);
			if (retryCount >= 3)
			{
				gfx_con.fntsz = 16;
				EPRINTFARGS("\nFailed to read %d blocks @ LBA %08X\nfrom eMMC. Aborting..\n",
					num, lba_curr);
				EPRINTF("\nPress any key and try again...\n");

				free(buf);
				f_close(&fp);
				f_unlink(outFilename);

				return 0;
			}
		}
		res = f_write(&fp, buf, NX_EMMC_BLOCKSIZE * num, NULL);
		if (res)
		{
			gfx_con.fntsz = 16;
			EPRINTFARGS("\nFatal error (%d) when writing to SD Card", res);
			EPRINTF("\nPress any key and try again...\n");

			free(buf);
			f_close(&fp);
			f_unlink(outFilename);

			return 0;
		}
		pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		if (pct != prevPct)
		{
			tui_pbar(&gfx_con, 0, gfx_con.y, pct, 0xFFCCCCCC, 0xFF555555);
			prevPct = pct;
		}

		lba_curr += num;
		totalSectors -= num;
		bytesWritten += num * NX_EMMC_BLOCKSIZE;

		// Force a flush after a lot of data if not splitting.
		if (numSplitParts == 0 && bytesWritten >= multipartSplitSize)
		{
			f_sync(&fp);
			bytesWritten = 0;
		}

		btn = btn_wait_timeout(0, BTN_VOL_DOWN | BTN_VOL_UP);
		if ((btn & BTN_VOL_DOWN) && (btn & BTN_VOL_UP))
		{
			gfx_con.fntsz = 16;
			WPRINTF("\n\nThe backup was cancelled!");
			EPRINTF("\nPress any key...\n");
			msleep(1500);

			free(buf);
			f_close(&fp);
			f_unlink(outFilename);

			return 0;
		}
	}
	tui_pbar(&gfx_con, 0, gfx_con.y, 100, 0xFFCCCCCC, 0xFF555555);

	// Backup operation ended successfully.
	free(buf);
	f_close(&fp);

	if (h_cfg.verification)
	{
		// Verify last part or single file backup.
		if (dump_emmc_verify(storage, lbaStartPart, outFilename, part))
		{
			EPRINTF("\nPress any key and try again...\n");

			return 0;
		}
		else
			tui_pbar(&gfx_con, 0, gfx_con.y, 100, 0xFF96FF00, 0xFF155500);
	}

	gfx_con.fntsz = 16;
	// Remove partial backup index file if no fatal errors occurred.
	if (isSmallSdCard)
	{
		f_unlink(partialIdxFilename);
		gfx_printf(&gfx_con, "%k\n\nYou can now join the files\nand get the complete eMMC RAW GPP backup.", 0xFFCCCCCC);
	}
	gfx_puts(&gfx_con, "\n\n");

	return 1;
}

typedef enum
{
	PART_BOOT =   (1 << 0),
	PART_SYSTEM = (1 << 1),
	PART_USER =   (1 << 2),
	PART_RAW =    (1 << 3),
	PART_GP_ALL = (1 << 7)
} emmcPartType_t;

static void dump_emmc_selected(emmcPartType_t dumpType)
{
	int res = 0;
	u32 timer = 0;
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	tui_sbar(&gfx_con, true);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (!sd_mount())
		goto out;

	gfx_puts(&gfx_con, "Checking for available free space...\n\n");
	// Get SD Card free space for Partial Backup.
	f_getfree("", &sd_fs.free_clst, NULL);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}

	int i = 0;
	char sdPath[80];
	// Create Restore folders, if they do not exist.
	emmcsn_path_impl(sdPath, "/restore", "", &storage);
	emmcsn_path_impl(sdPath, "/restore/partitions", "", &storage);

	timer = get_tmr_s();
	if (dumpType & PART_BOOT)
	{
		const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;
		for (i = 0; i < 2; i++)
		{
			memcpy(bootPart.name, "BOOT", 5);
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&storage, i + 1);

			emmcsn_path_impl(sdPath, "", bootPart.name, &storage);
			res = dump_emmc_part(sdPath, &storage, &bootPart);
		}
	}

	if ((dumpType & PART_SYSTEM) || (dumpType & PART_USER) || (dumpType & PART_RAW))
	{
		sdmmc_storage_set_mmc_partition(&storage, 0);

		if ((dumpType & PART_SYSTEM) || (dumpType & PART_USER))
		{
			LIST_INIT(gpt);
			nx_emmc_gpt_parse(&gpt, &storage);
			LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
			{
				if ((dumpType & PART_USER) == 0 && !strcmp(part->name, "USER"))
					continue;
				if ((dumpType & PART_SYSTEM) == 0 && strcmp(part->name, "USER"))
					continue;

				gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
					part->name, part->lba_start, part->lba_end, 0xFFCCCCCC);

				emmcsn_path_impl(sdPath, "/partitions", part->name, &storage);
				res = dump_emmc_part(sdPath, &storage, part);
				// If a part failed, don't continue.
				if (!res)
					break;
			}
			nx_emmc_gpt_free(&gpt);
		}

		if (dumpType & PART_RAW)
		{
			// Get GP partition size dynamically.
			const u32 RAW_AREA_NUM_SECTORS = storage.sec_cnt;

			emmc_part_t rawPart;
			memset(&rawPart, 0, sizeof(rawPart));
			rawPart.lba_start = 0;
			rawPart.lba_end = RAW_AREA_NUM_SECTORS - 1;
			strcpy(rawPart.name, "rawnand.bin");
			{
				gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
					rawPart.name, rawPart.lba_start, rawPart.lba_end, 0xFFCCCCCC);

				emmcsn_path_impl(sdPath, "", rawPart.name, &storage);
				res = dump_emmc_part(sdPath, &storage, &rawPart);
			}
		}
	}

	gfx_putc(&gfx_con, '\n');
	timer = get_tmr_s() - timer;
	gfx_printf(&gfx_con, "Time taken: %dm %ds.\n", timer / 60, timer % 60);
	sdmmc_storage_end(&storage);
	if (res && h_cfg.verification)
		gfx_printf(&gfx_con, "\n%kFinished and verified!%k\nPress any key...\n", 0xFF96FF00, 0xFFCCCCCC);
	else if (res)
		gfx_printf(&gfx_con, "\nFinished! Press any key...\n");

out:
	sd_unmount();
	btn_wait();
}

void dump_emmc_system() { dump_emmc_selected(PART_SYSTEM); }
void dump_emmc_user() { dump_emmc_selected(PART_USER); }
void dump_emmc_boot() { dump_emmc_selected(PART_BOOT); }
void dump_emmc_rawnand() { dump_emmc_selected(PART_RAW); }

int restore_emmc_part(char *sd_path, sdmmc_storage_t *storage, emmc_part_t *part)
{
	static const u32 SECTORS_TO_MIB_COEFF = 11;

	u32 totalSectors = part->lba_end - part->lba_start + 1;
	u32 lbaStartPart = part->lba_start;
	int res = 0;
	char *outFilename = sd_path;

	gfx_con.fntsz = 8;

	FIL fp;
	gfx_printf(&gfx_con, "\nFilename: %s\n", outFilename);

	res = f_open(&fp, outFilename, FA_READ);
	if (res)
	{
		WPRINTFARGS("Error (%d) while opening backup. Continuing...\n", res);
		gfx_con.fntsz = 16;

		return 0;
	}
	//TODO: Should we keep this check?
	else if (((u32)((u64)f_size(&fp) >> (u64)9)) != totalSectors)
	{
		gfx_con.fntsz = 16;
		EPRINTF("Size of the SD Card backup does not match,\neMMC's selected part size.\n");
		f_close(&fp);

		return 0;
	}
	else
		gfx_printf(&gfx_con, "\nTotal restore size: %d MiB.\n\n", ((u32)((u64)f_size(&fp) >> (u64)9)) >> SECTORS_TO_MIB_COEFF);

	u32 numSectorsPerIter = 0;
	if (totalSectors > 0x200000)
		numSectorsPerIter = 8192; //4MB Cache
	else
		numSectorsPerIter = 512;  //256KB Cache

	u8 *buf = (u8 *)calloc(numSectorsPerIter, NX_EMMC_BLOCKSIZE);

	u32 lba_curr = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;

	u32 num = 0;
	u32 pct = 0;
	while (totalSectors > 0)
	{
		retryCount = 0;
		num = MIN(totalSectors, numSectorsPerIter);

		res = f_read(&fp, buf, NX_EMMC_BLOCKSIZE * num, NULL);
		if (res)
		{
			gfx_con.fntsz = 16;
			EPRINTFARGS("\nFatal error (%d) when reading from SD Card", res);
			EPRINTF("\nYour device may be in an inoperative state!\n\nPress any key and try again now...\n");

			free(buf);
			f_close(&fp);
			return 0;
		}
		while (!sdmmc_storage_write(storage, lba_curr, num, buf))
		{
			EPRINTFARGS("Error writing %d blocks @ LBA %08X\nto eMMC (try %d), retrying...",
				num, lba_curr, ++retryCount);

			msleep(150);
			if (retryCount >= 3)
			{
				gfx_con.fntsz = 16;
				EPRINTFARGS("\nFailed to write %d blocks @ LBA %08X\nfrom eMMC. Aborting..\n",
					num, lba_curr);
				EPRINTF("\nYour device may be in an inoperative state!\n\nPress any key and try again...\n");

				free(buf);
				f_close(&fp);
				return 0;
			}
		}
		pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		if (pct != prevPct)
		{
			tui_pbar(&gfx_con, 0, gfx_con.y, pct, 0xFFCCCCCC, 0xFF555555);
			prevPct = pct;
		}

		lba_curr += num;
		totalSectors -= num;
		bytesWritten += num * NX_EMMC_BLOCKSIZE;
	}
	tui_pbar(&gfx_con, 0, gfx_con.y, 100, 0xFFCCCCCC, 0xFF555555);

	// Restore operation ended successfully.
	free(buf);
	f_close(&fp);

	if (h_cfg.verification)
	{
		// Verify restored data.
		if (dump_emmc_verify(storage, lbaStartPart, outFilename, part))
		{
			EPRINTF("\nPress any key and try again...\n");

			return 0;
		}
		else
			tui_pbar(&gfx_con, 0, gfx_con.y, 100, 0xFF96FF00, 0xFF155500);
	}

	gfx_con.fntsz = 16;
	gfx_puts(&gfx_con, "\n\n");

	return 1;
}

static void restore_emmc_selected(emmcPartType_t restoreType)
{
	int res = 0;
	u32 timer = 0;
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	tui_sbar(&gfx_con, true);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, "%kThis is a dangerous operation\nand may render your device inoperative!\n\n", 0xFFFFDD00);
	gfx_printf(&gfx_con, "Are you really sure?\n\n%k", 0xFFCCCCCC);
	if ((restoreType & PART_BOOT) || (restoreType & PART_GP_ALL))
	{
		gfx_puts(&gfx_con, "The mode you selected will only restore\nthe ");
		if (restoreType & PART_BOOT)
			gfx_puts(&gfx_con, "boot ");
		gfx_puts(&gfx_con, "partitions that it can find.\n");
		gfx_puts(&gfx_con, "If it is not found, it will be skipped\nand continue with the next.\n\n");
	}
	gfx_con_getpos(&gfx_con, &gfx_con.savedx, &gfx_con.savedy);

	u8 value = 10;
	while (value > 0)
	{
		gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);
		gfx_printf(&gfx_con, "%kWait... (%ds)    %k", 0xFF888888, value, 0xFFCCCCCC);
		msleep(1000);
		value--;
	}
	gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);

	gfx_puts(&gfx_con, "Press POWER to Continue.\nPress VOL to go to the menu.\n\n\n");

	u32 btn = btn_wait();
	if (!(btn & BTN_POWER))
		goto out;

	if (!sd_mount())
		goto out;

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}

	int i = 0;
	char sdPath[80];

	timer = get_tmr_s();
	if (restoreType & PART_BOOT)
	{
		const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;
		for (i = 0; i < 2; i++)
		{
			memcpy(bootPart.name, "BOOT", 4);
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&storage, i + 1);

			emmcsn_path_impl(sdPath, "/restore", bootPart.name, &storage);
			res = restore_emmc_part(sdPath, &storage, &bootPart);
		}
	}

	if (restoreType & PART_GP_ALL)
	{
		sdmmc_storage_set_mmc_partition(&storage, 0);

		LIST_INIT(gpt);
		nx_emmc_gpt_parse(&gpt, &storage);
		LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
		{
			gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
				part->name, part->lba_start, part->lba_end, 0xFFCCCCCC);

			emmcsn_path_impl(sdPath, "/restore/partitions/", part->name, &storage);
			res = restore_emmc_part(sdPath, &storage, part);
		}
		nx_emmc_gpt_free(&gpt);
	}

	if (restoreType & PART_RAW)
	{
		// Get GP partition size dynamically.
		const u32 RAW_AREA_NUM_SECTORS = storage.sec_cnt;

		emmc_part_t rawPart;
		memset(&rawPart, 0, sizeof(rawPart));
		rawPart.lba_start = 0;
		rawPart.lba_end = RAW_AREA_NUM_SECTORS - 1;
		strcpy(rawPart.name, "rawnand.bin");
		{
			gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
				rawPart.name, rawPart.lba_start, rawPart.lba_end, 0xFFCCCCCC);

			emmcsn_path_impl(sdPath, "/restore", rawPart.name, &storage);
			res = restore_emmc_part(sdPath, &storage, &rawPart);
		}
	}

	gfx_putc(&gfx_con, '\n');
	timer = get_tmr_s() - timer;
	gfx_printf(&gfx_con, "Time taken: %dm %ds.\n", timer / 60, timer % 60);
	sdmmc_storage_end(&storage);
	if (res && h_cfg.verification)
		gfx_printf(&gfx_con, "\n%kFinished and verified!%k\nPress any key...\n", 0xFF96FF00, 0xFFCCCCCC);
	else if (res)
		gfx_printf(&gfx_con, "\nFinished! Press any key...\n");

out:
	sd_unmount();
	btn_wait();
}

void restore_emmc_boot() { restore_emmc_selected(PART_BOOT); }
void restore_emmc_rawnand() { restore_emmc_selected(PART_RAW); }
void restore_emmc_gpp_parts() { restore_emmc_selected(PART_GP_ALL); }

void dump_packages12()
{
	u8 *pkg1 = (u8 *)calloc(1, 0x40000);
	u8 *warmboot = (u8 *)calloc(1, 0x40000);
	u8 *secmon = (u8 *)calloc(1, 0x40000);
	u8 *loader = (u8 *)calloc(1, 0x40000);
	u8 *pkg2 = NULL;

	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (!sd_mount())
		goto out;

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}
	sdmmc_storage_set_mmc_partition(&storage, 1);

	// Read package1.
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1);
	const pk11_hdr_t *hdr = (pk11_hdr_t *)(pkg1 + pkg1_id->pkg11_off + 0x20);
	if (!pkg1_id)
	{
		gfx_con.fntsz = 8;
		EPRINTFARGS("Unknown package1 version for reading\nTSEC firmware (= '%s').", (char *)pkg1 + 0x10);
		goto out;
	}

	if (!h_cfg.se_keygen_done)
	{
		// Read keyblob.
		u8 *keyblob = (u8 *)calloc(NX_EMMC_BLOCKSIZE, 1);
		sdmmc_storage_read(&storage, 0x180000 / NX_EMMC_BLOCKSIZE + pkg1_id->kb, 1, keyblob);

		// Decrypt.
		keygen(keyblob, pkg1_id->kb, (u8 *)pkg1 + pkg1_id->tsec_off);

		h_cfg.se_keygen_done = 1;
		free(keyblob);
	}
	pkg1_decrypt(pkg1_id, pkg1);

	pkg1_unpack(warmboot, secmon, loader, pkg1_id, pkg1);

	// Display info.
	gfx_printf(&gfx_con, "%kNX Bootloader size:  %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->ldr_size);

	gfx_printf(&gfx_con, "%kSecure monitor addr: %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, pkg1_id->secmon_base);
	gfx_printf(&gfx_con, "%kSecure monitor size: %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->sm_size);

	gfx_printf(&gfx_con, "%kWarmboot addr:       %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, pkg1_id->warmboot_base);
	gfx_printf(&gfx_con, "%kWarmboot size:       %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->wb_size);

	char path[64];
	// Dump package1.1.
	emmcsn_path_impl(path, "/pkg1", "pkg1_decr.bin", &storage);
	if (sd_save_to_file(pkg1, 0x40000, path))
		goto out;
	gfx_puts(&gfx_con, "\nFull package1 dumped to pkg1_decr.bin\n");

	// Dump nxbootloader.
	emmcsn_path_impl(path, "/pkg1", "nxloader.bin", &storage);
	if (sd_save_to_file(loader, hdr->ldr_size, path))
		goto out;
	gfx_puts(&gfx_con, "NX Bootloader dumped to nxloader.bin\n");

	// Dump secmon.
	emmcsn_path_impl(path, "/pkg1", "secmon.bin", &storage);
	if (sd_save_to_file(secmon, hdr->sm_size, path))
		goto out;
	gfx_puts(&gfx_con, "Secure Monitor dumped to secmon.bin\n");

	// Dump warmboot.
	emmcsn_path_impl(path, "/pkg1", "warmboot.bin", &storage);
	if (sd_save_to_file(warmboot, hdr->wb_size, path))
		goto out;
	gfx_puts(&gfx_con, "Warmboot dumped to warmboot.bin\n\n\n");

	// Dump package2.1.
	sdmmc_storage_set_mmc_partition(&storage, 0);
	// Parse eMMC GPT.
	LIST_INIT(gpt);
	nx_emmc_gpt_parse(&gpt, &storage);
	// Find package2 partition.
	emmc_part_t *pkg2_part = nx_emmc_part_find(&gpt, "BCPKG2-1-Normal-Main");
	if (!pkg2_part)
		goto out;

	// Read in package2 header and get package2 real size.
	u8 *tmp = (u8 *)malloc(NX_EMMC_BLOCKSIZE);
	nx_emmc_part_read(&storage, pkg2_part, 0x4000 / NX_EMMC_BLOCKSIZE, 1, tmp);
	u32 *hdr_pkg2_raw = (u32 *)(tmp + 0x100);
	u32 pkg2_size = hdr_pkg2_raw[0] ^ hdr_pkg2_raw[2] ^ hdr_pkg2_raw[3];
	free(tmp);
	// Read in package2.
	u32 pkg2_size_aligned = ALIGN(pkg2_size, NX_EMMC_BLOCKSIZE);
	pkg2 = malloc(pkg2_size_aligned);
	nx_emmc_part_read(&storage, pkg2_part, 0x4000 / NX_EMMC_BLOCKSIZE, 
		pkg2_size_aligned / NX_EMMC_BLOCKSIZE, pkg2);
	// Decrypt package2 and parse KIP1 blobs in INI1 section.
	pkg2_hdr_t *pkg2_hdr = pkg2_decrypt(pkg2);

	// Display info.
	u32 kernel_crc32 = crc32c(pkg2_hdr->data, pkg2_hdr->sec_size[PKG2_SEC_KERNEL]);
	gfx_printf(&gfx_con, "\n%kKernel CRC32C: %k0x%08X\n\n", 0xFFC7EA46, 0xFFCCCCCC, kernel_crc32);
	gfx_printf(&gfx_con, "%kKernel size:   %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, pkg2_hdr->sec_size[PKG2_SEC_KERNEL]);
	gfx_printf(&gfx_con, "%kINI1 size:     %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, pkg2_hdr->sec_size[PKG2_SEC_INI1]);

	// Dump pkg2.1.
	emmcsn_path_impl(path, "/pkg2", "pkg2_decr.bin", &storage);
	if (sd_save_to_file(pkg2, pkg2_hdr->sec_size[PKG2_SEC_KERNEL] + pkg2_hdr->sec_size[PKG2_SEC_INI1], path))
		goto out;
	gfx_puts(&gfx_con, "\nFull package2 dumped to pkg2_decr.bin\n");

	// Dump kernel.
	emmcsn_path_impl(path, "/pkg2", "kernel.bin", &storage);
	if (sd_save_to_file(pkg2_hdr->data, pkg2_hdr->sec_size[PKG2_SEC_KERNEL], path))
		goto out;
	gfx_puts(&gfx_con, "Kernel dumped to kernel.bin\n");

	// Dump INI1.
	emmcsn_path_impl(path, "/pkg2", "ini1.bin", &storage);
	if (sd_save_to_file(pkg2_hdr->data + pkg2_hdr->sec_size[PKG2_SEC_KERNEL],
		pkg2_hdr->sec_size[PKG2_SEC_INI1], path))
		goto out;
	gfx_puts(&gfx_con, "INI1 kip1 package dumped to ini1.bin\n");

	gfx_puts(&gfx_con, "\nDone. Press any key...\n");

out:
	free(pkg1);
	free(secmon);
	free(warmboot);
	free(loader);
	free(pkg2);
	nx_emmc_gpt_free(&gpt);
	sdmmc_storage_end(&storage);
	sd_unmount();

	btn_wait();
}

// This is a safe and unused DRAM region for our payloads.
#define IPL_START 0x40008000
#define EXT_PAYLOAD_ADDR 0xC03C0000
#define PATCHED_RELOC_SZ 0x94
#define RCM_PAYLOAD_ADDR (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
#define PAYLOAD_ENTRY 0x40010000
#define CBFS_SDRAM_EN_ADDR 0x4003e000
#define COREBOOT_ADDR (0xD0000000 - 0x100000)

void (*ext_payload_ptr)() = (void *)EXT_PAYLOAD_ADDR;

void reloc_patcher(u32 payload_size)
{
	static const u32 START_OFF = 0x7C;
	static const u32 PAYLOAD_END_OFF = 0x84;
	static const u32 IPL_START_OFF = 0x88;

	memcpy((u8 *)EXT_PAYLOAD_ADDR, (u8 *)IPL_START, PATCHED_RELOC_SZ);

	*(vu32 *)(EXT_PAYLOAD_ADDR + START_OFF) = PAYLOAD_ENTRY - ALIGN(PATCHED_RELOC_SZ, 0x10);
	*(vu32 *)(EXT_PAYLOAD_ADDR + PAYLOAD_END_OFF) = PAYLOAD_ENTRY + payload_size;
	*(vu32 *)(EXT_PAYLOAD_ADDR + IPL_START_OFF) = PAYLOAD_ENTRY;

	if (payload_size == 0x7000)
	{
		memcpy((u8 *)(EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10)), (u8 *)COREBOOT_ADDR, 0x7000); //Bootblock
		*(vu32 *)CBFS_SDRAM_EN_ADDR = 0x4452414D;
	}
}

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
		(*ext_payload_ptr)();
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
	char *dir;

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

		filelist = dirlist(dir, NULL);

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
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	ini_free(&ini_sections);
	if (h_cfg.autoboot_list)
		ini_free(&ini_list_sections);
	ini_free_section(cfg_sec);

	sd_unmount();
	gfx_con.mute = false;
}

void toggle_autorcm(bool enable)
{
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	u8 randomXor = 0;

	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}

	u8 *tempbuf = (u8 *)malloc(0x200);
	sdmmc_storage_set_mmc_partition(&storage, 1);

	int i, sect = 0;
	for (i = 0; i < 4; i++)
	{
		sect = (0x200 + (0x4000 * i)) / NX_EMMC_BLOCKSIZE;
		sdmmc_storage_read(&storage, sect, 1, tempbuf);

		if (enable)
		{
			do
			{
				randomXor = get_tmr_us() & 0xFF; // Bricmii style of bricking.
			} while (!randomXor); // Avoid the lottery.

			tempbuf[0x10] ^= randomXor;
		}
		else
			tempbuf[0x10] = 0xF7;
		sdmmc_storage_write(&storage, sect, 1, tempbuf);
	}

	free(tempbuf);
	sdmmc_storage_end(&storage);

	if (enable)
		gfx_printf(&gfx_con, "%kAutoRCM mode enabled!%k", 0xFFFFBA00, 0xFFCCCCCC);
	else
		gfx_printf(&gfx_con, "%kAutoRCM mode disabled!%k", 0xFF96FF00, 0xFFCCCCCC);
	gfx_printf(&gfx_con, "\n\nPress any key...\n");

out:
	btn_wait();
}

void enable_autorcm() { toggle_autorcm(true); }
void disable_autorcm() { toggle_autorcm(false); }

void menu_autorcm()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	// Do a simple check on the main BCT.
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	bool disabled = true;

	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		btn_wait();

		return;
	}

	u8 *tempbuf = (u8 *)malloc(0x200);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x200 / NX_EMMC_BLOCKSIZE, 1, tempbuf);

	if (tempbuf[0x10] != 0xF7)
		disabled = false;

	free(tempbuf);
	sdmmc_storage_end(&storage);

	// Create AutoRCM menu.
	ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * 6);

	ments[0].type = MENT_BACK;
	ments[0].caption = "Back";

	ments[1].type = MENT_CHGLINE;

	ments[2].type = MENT_CAPTION;
	ments[3].type = MENT_CHGLINE;
	if (disabled)
	{
		ments[2].caption = "Status: Disabled!";
		ments[2].color = 0xFF96FF00;
		ments[4].caption = "Enable AutoRCM";
		ments[4].handler = enable_autorcm;
	}
	else
	{
		ments[2].caption = "Status: Enabled!";
		ments[2].color = 0xFFFFBA00;
		ments[4].caption = "Disable AutoRCM";
		ments[4].handler = disable_autorcm;
	}
	ments[4].type = MENT_HDLR_RE;

	memset(&ments[5], 0, sizeof(ment_t));
	menu_t menu = {ments, "This corrupts your BOOT0!", 0, 0};

	tui_do_menu(&gfx_con, &menu);
}

int fix_attributes(char *path, u32 *total, u32 is_root, u32 check_first_run)
{
	FRESULT res;
	DIR dir;
	u32 dirLength = 0;
	static FILINFO fno;

	if (check_first_run)
	{
		// Read file attributes.
		res = f_stat(path, &fno);
		if (res != FR_OK)
			return res;

		// Check if archive bit is set.
		if (fno.fattrib & AM_ARC)
		{
			*(u32 *)total = *(u32 *)total + 1;
			f_chmod(path, 0, AM_ARC);
		}
	}

	// Open directory.
	res = f_opendir(&dir, path);
	if (res != FR_OK)
		return res;

	dirLength = strlen(path);
	for (;;)
	{
		// Clear file or folder path.
		path[dirLength] = 0;

		// Read a directory item.
		res = f_readdir(&dir, &fno);

		// Break on error or end of dir.
		if (res != FR_OK || fno.fname[0] == 0)
			break;

		// Skip official Nintendo dir.
		if (is_root && !strcmp(fno.fname, "Nintendo"))
			continue;

		// Set new directory or file.
		memcpy(&path[dirLength], "/", 1);
		memcpy(&path[dirLength + 1], fno.fname, strlen(fno.fname) + 1);

		// Check if archive bit is set.
		if (fno.fattrib & AM_ARC)
		{
			*(u32 *)total = *(u32 *)total + 1;
			f_chmod(path, 0, AM_ARC);
		}

		// Is it a directory?
		if (fno.fattrib & AM_DIR)
		{
			// Enter the directory.
			res = fix_attributes(path, total, 0, 0);
			if (res != FR_OK)
				break;
		}
	}

	f_closedir(&dir);

	return res;
}

void fix_sd_attr(u32 type)
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	char path[256];
	char label[14];

	u32 total = 0;
	if (sd_mount())
	{
		switch (type)
		{
		case 0:
			memcpy(path, "/", 2);
			memcpy(label, "SD Card", 8);
			break;
		case 1:
		default:
			memcpy(path, "/switch", 8);
			memcpy(label, "switch folder", 14);
			break;
		}

		gfx_printf(&gfx_con, "Traversing all %s files!\nThis may take some time, please wait...\n\n", label);
		fix_attributes(path, &total, !type, type);
		gfx_printf(&gfx_con, "%kTotal archive bits cleared: %d!%k\n\nDone! Press any key...", 0xFF96FF00, total, 0xFFCCCCCC);
		sd_unmount();
	}
	btn_wait();
}

void fix_sd_all_attr() { fix_sd_attr(0); }
void fix_sd_switch_attr() { fix_sd_attr(1); }

void print_fuel_gauge_info()
{
	int value = 0;

	gfx_printf(&gfx_con, "%kFuel Gauge IC Info:\n%k", 0xFF00DDFF, 0xFFCCCCCC);

	max17050_get_property(MAX17050_Age, &value);
	gfx_printf(&gfx_con, "Age:                    %3d%\n", value);

	max17050_get_property(MAX17050_RepSOC, &value);
	gfx_printf(&gfx_con, "Capacity now:           %3d%\n", value >> 8);

	max17050_get_property(MAX17050_RepCap, &value);
	gfx_printf(&gfx_con, "Capacity now:           %4d mAh\n", value);

	max17050_get_property(MAX17050_FullCAP, &value);
	gfx_printf(&gfx_con, "Capacity full:          %4d mAh\n", value);

	max17050_get_property(MAX17050_DesignCap, &value);
	gfx_printf(&gfx_con, "Capacity (design):      %4d mAh\n", value);

	max17050_get_property(MAX17050_Current, &value);
	if (value >= 0)
		gfx_printf(&gfx_con, "Current now:            %d mA\n", value / 1000);
	else
		gfx_printf(&gfx_con, "Current now:            -%d mA\n", ~value / 1000);

	max17050_get_property(MAX17050_AvgCurrent, &value);
	if (value >= 0)
		gfx_printf(&gfx_con, "Current average:        %d mA\n", value / 1000);
	else
		gfx_printf(&gfx_con, "Current average:        -%d mA\n", ~value / 1000);

	max17050_get_property(MAX17050_VCELL, &value);
	gfx_printf(&gfx_con, "Voltage now:            %4d mV\n", value);

	max17050_get_property(MAX17050_OCVInternal, &value);
	gfx_printf(&gfx_con, "Voltage open-circuit:   %4d mV\n", value);

	max17050_get_property(MAX17050_MinVolt, &value);
	gfx_printf(&gfx_con, "Min voltage reached:    %4d mV\n", value);

	max17050_get_property(MAX17050_MaxVolt, &value);
	gfx_printf(&gfx_con, "Max voltage reached:    %4d mV\n", value);

	max17050_get_property(MAX17050_V_empty, &value);
	gfx_printf(&gfx_con, "Empty voltage (design): %4d mV\n", value);

	max17050_get_property(MAX17050_TEMP, &value);
	if (value >= 0)
		gfx_printf(&gfx_con, "Battery temperature:    %d.%d oC\n", value / 10, value % 10);
	else
		gfx_printf(&gfx_con, "Battery temperature:    -%d.%d oC\n", ~value / 10, (~value) % 10);
}

void print_battery_charger_info()
{
	int value = 0;

	gfx_printf(&gfx_con, "%k\n\nBattery Charger IC Info:\n%k", 0xFF00DDFF, 0xFFCCCCCC);

	bq24193_get_property(BQ24193_InputVoltageLimit, &value);
	gfx_printf(&gfx_con, "Input voltage limit:       %4d mV\n", value);

	bq24193_get_property(BQ24193_InputCurrentLimit, &value);
	gfx_printf(&gfx_con, "Input current limit:       %4d mA\n", value);

	bq24193_get_property(BQ24193_SystemMinimumVoltage, &value);
	gfx_printf(&gfx_con, "Min voltage limit:         %4d mV\n", value);

	bq24193_get_property(BQ24193_FastChargeCurrentLimit, &value);
	gfx_printf(&gfx_con, "Fast charge current limit: %4d mA\n", value);

	bq24193_get_property(BQ24193_ChargeVoltageLimit, &value);
	gfx_printf(&gfx_con, "Charge voltage limit:      %4d mV\n", value);

	bq24193_get_property(BQ24193_ChargeStatus, &value);
	gfx_printf(&gfx_con, "Charge status:             ");
	switch (value)
	{
	case 0:
		gfx_printf(&gfx_con, "Not charging\n");
		break;
	case 1:
		gfx_printf(&gfx_con, "Pre-charging\n");
		break;
	case 2:
		gfx_printf(&gfx_con, "Fast charging\n");
		break;
	case 3:
		gfx_printf(&gfx_con, "Charge terminated\n");
		break;
	default:
		gfx_printf(&gfx_con, "Unknown (%d)\n", value);
		break;
	}
	bq24193_get_property(BQ24193_TempStatus, &value);
	gfx_printf(&gfx_con, "Temperature status:        ");
	switch (value)
	{
	case 0:
		gfx_printf(&gfx_con, "Normal\n");
		break;
	case 2:
		gfx_printf(&gfx_con, "Warm\n");
		break;
	case 3:
		gfx_printf(&gfx_con, "Cool\n");
		break;
	case 5:
		gfx_printf(&gfx_con, "Cold\n");
		break;
	case 6:
		gfx_printf(&gfx_con, "Hot\n");
		break;
	default:
		gfx_printf(&gfx_con, "Unknown (%d)\n", value);
		break;
	}
}

void print_battery_info()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	print_fuel_gauge_info();

	print_battery_charger_info();

	u8 *buf = (u8 *)malloc(0x100 * 2);

	gfx_printf(&gfx_con, "%k\n\nBattery Fuel Gauge Registers:\n%k", 0xFF00DDFF, 0xFFCCCCCC);

	for (int i = 0; i < 0x200; i += 2)
	{
		i2c_recv_buf_small(buf + i, 2, I2C_1, 0x36, i >> 1);
		usleep(2500);
	}

	gfx_hexdump(&gfx_con, 0, (u8 *)buf, 0x200);

	gfx_puts(&gfx_con, "\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();

	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "fuel_gauge.bin", NULL);
			if (sd_save_to_file((u8 *)buf, 0x200, path))
				EPRINTF("\nError creating fuel.bin file.");
			else
				gfx_puts(&gfx_con, "\nDone!\n");
			sd_unmount();
		}

		btn_wait();
	}
	free(buf);
}

/* void fix_fuel_gauge_configuration()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	int battVoltage, avgCurrent;

	max17050_get_property(MAX17050_VCELL, &battVoltage);
	max17050_get_property(MAX17050_AvgCurrent, &avgCurrent);

	// Check if still charging. If not, check if battery is >= 95% (4.1V).
	if (avgCurrent < 0 && battVoltage > 4100)
	{
		if ((avgCurrent / 1000) < -10)
			EPRINTF("You need to be connected to a wall adapter,\nto apply this fix!");
		else
		{
			gfx_printf(&gfx_con, "%kAre you really sure?\nThis will reset your fuel gauge completely!\n", 0xFFFFDD00);
			gfx_printf(&gfx_con, "Additionally this will power off your console.\n%k", 0xFFCCCCCC);

			gfx_puts(&gfx_con, "\nPress POWER to Continue.\nPress VOL to go to the menu.\n\n\n");

			u32 btn = btn_wait();
			if (btn & BTN_POWER)
			{
				max17050_fix_configuration();
				msleep(1000);
				gfx_con_getpos(&gfx_con, &gfx_con.savedx,  &gfx_con.savedy);
				u16 value = 0;
				gfx_printf(&gfx_con, "%kThe console will power off in 45 seconds.\n%k", 0xFFFFDD00, 0xFFCCCCCC);
				while (value < 46)
				{
					gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);
					gfx_printf(&gfx_con, "%2ds elapsed", value);
					msleep(1000);
					value++;
				}
				msleep(2000);

				power_off();
			}
			return;
		}
	}
	else
		EPRINTF("You need a fully charged battery\nand connected to a wall adapter,\nto apply this fix!");

	msleep(500);
	btn_wait();
} */

/*void reset_pmic_fuel_gauge_charger_config()
{
	int avgCurrent;

	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, "%k\nThis will wipe your battery stats completely!\n"
		"%kAnd it may not power on without physically\nremoving and re-inserting the battery.\n%k"
		"\nAre you really sure?%k\n", 0xFFFFDD00, 0xFFFF0000, 0xFFFFDD00, 0xFFCCCCCC);

	gfx_puts(&gfx_con, "\nPress POWER to Continue.\nPress VOL to go to the menu.\n\n\n");
	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
		gfx_con_setpos(&gfx_con, 0, 0);
		gfx_printf(&gfx_con, "%kKeep the USB cable connected!%k\n\n", 0xFFFFDD00, 0xFFCCCCCC);
		gfx_con_getpos(&gfx_con, &gfx_con.savedx,  &gfx_con.savedy);

		u8 value = 30;
		while (value > 0)
		{
			gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);
			gfx_printf(&gfx_con, "%kWait... (%ds)   %k", 0xFF888888, value, 0xFFCCCCCC);
			msleep(1000);
			value--;
		}
		gfx_con_setpos(&gfx_con, gfx_con.savedx, gfx_con.savedy);

		//Check if still connected.
		max17050_get_property(MAX17050_AvgCurrent, &avgCurrent);
		if ((avgCurrent / 1000) < -10)
			EPRINTF("You need to be connected to a wall adapter\nor PC to apply this fix!");
		else
		{
			// Apply fix.
			bq24193_fake_battery_removal();
			gfx_printf(&gfx_con, "Done!               \n"
				"%k1. Remove the USB cable\n"
				"2. Press POWER for 15s.\n"
				"3. Reconnect the USB to power-on!%k\n", 0xFFFFDD00, 0xFFCCCCCC);
		}
		msleep(500);
		btn_wait();
	}
}*/

void fix_battery_desync()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	max77620_low_battery_monitor_config();

	gfx_puts(&gfx_con, "\nDone!\n");

	btn_wait();
}

void ipatch_process(u32 offset, u32 value)
{
	gfx_printf(&gfx_con, "%8x %8x", BOOTROM_BASE + offset, value);
	u8 lo = value & 0xff;
	switch (value >> 8)
	{
	case 0xdf:
		gfx_printf(&gfx_con, "    svc #0x%02x", lo);
		break;
	case 0x20:
		gfx_printf(&gfx_con, "    movs r0, #0x%02x", lo);
		break;
	}
	gfx_puts(&gfx_con, "\n");
}

void bootrom_ipatches_info()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	static const u32 BOOTROM_SIZE = 0x18000;
	
	u32 res = fuse_read_ipatch(ipatch_process);
	if (res != 0)
		EPRINTFARGS("Failed to read ipatches. Error: %d", res);
	
	gfx_puts(&gfx_con, "\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");
	
	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			u32 iram_evp_thunks[0x200];
			u32 iram_evp_thunks_len = sizeof(iram_evp_thunks);
			res = fuse_read_evp_thunk(iram_evp_thunks, &iram_evp_thunks_len);
			if (res == 0)
			{
				emmcsn_path_impl(path, "/dumps", "evp_thunks.bin", NULL);
				if (!sd_save_to_file((u8 *)iram_evp_thunks, iram_evp_thunks_len, path))
					gfx_puts(&gfx_con, "\nevp_thunks.bin saved!\n");
			}
			else
				EPRINTFARGS("Failed to read evp_thunks. Error: %d", res);
			
			u32 words[0x100];
			read_raw_ipatch_fuses(words);
			emmcsn_path_impl(path, "/dumps", "ipatches.bin", NULL);
			if (!sd_save_to_file((u8 *)words, sizeof(words), path))
				gfx_puts(&gfx_con, "\nipatches.bin saved!\n");
			
			emmcsn_path_impl(path, "/dumps", "bootrom_patched.bin", NULL);
			if (!sd_save_to_file((u8 *)BOOTROM_BASE, BOOTROM_SIZE, path))
				gfx_puts(&gfx_con, "\nbootrom_patched.bin saved!\n");
			
			u8 ipatch_backup[13];
			memcpy(ipatch_backup, (void *) IPATCH_BASE, 13);
			memset((void*)IPATCH_BASE, 0, 13);
			
			emmcsn_path_impl(path, "/dumps", "bootrom_unpatched.bin", NULL);
			if (!sd_save_to_file((u8 *)BOOTROM_BASE, BOOTROM_SIZE, path))
				gfx_puts(&gfx_con, "\nbootrom_unpatched.bin saved!\n");
			
			memcpy((void*)IPATCH_BASE, ipatch_backup, 13);
			
			sd_unmount();
		}

		btn_wait();
	}
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
	MDEF_HANDLER("Unset archive bit (switch folder)", fix_sd_switch_attr),
	MDEF_HANDLER("Unset archive bit (all sd files)", fix_sd_all_attr),
	//MDEF_HANDLER("Fix fuel gauge configuration", fix_fuel_gauge_configuration),
	//MDEF_HANDLER("Reset all battery cfg", reset_pmic_fuel_gauge_charger_config),
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
	"hekate - CTCaer mod v4.1", 0, 0
};

extern void pivot_stack(u32 stack_top);

void ipl_main()
{
	// Skip config if we just updated the bootloader.
	if (*(vu32 *)BOOTLOADER_UPDATED_MAGIC_ADDR != BOOTLOADER_UPDATED_MAGIC)
		config_hw();

	//Pivot the stack so we have enough space.
	pivot_stack(0x90010000);

	//Tegra/Horizon configuration goes to 0x80000000+, package2 goes to 0xA9800000, we place our heap in between.
	heap_init(0x90020000);

	//uart_send(UART_C, (u8 *)0x40000000, 0x10000);
	//uart_wait_idle(UART_C, UART_TX_IDLE);

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
