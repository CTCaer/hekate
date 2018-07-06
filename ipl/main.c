/*
* Copyright (c) 2018 naehrwert
*
* Copyright (c) 2018 Rajko Stojadinovic
* Copyright (c) 2018 CTCaer
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

#include "clock.h"
#include "uart.h"
#include "i2c.h"
#include "sdram.h"
#include "di.h"
#include "mc.h"
#include "t210.h"
#include "pmc.h"
#include "pinmux.h"
#include "fuse.h"
#include "util.h"
#include "gfx.h"
#include "btn.h"
#include "tsec.h"
#include "kfuse.h"
#include "max77620.h"
#include "max7762x.h"
#include "gpio.h"
#include "sdmmc.h"
#include "ff.h"
#include "hekate_logos.h"
#include "tui.h"
#include "heap.h"
#include "list.h"
#include "nx_emmc.h"
#include "se.h"
#include "se_t210.h"
#include "hos.h"
#include "pkg1.h"
#include "mmc.h"
#include "lz.h"
#include "max17050.h"
#include "bq24193.h"
#include "config.h"

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
int sd_mounted;

#ifdef MENU_LOGO_ENABLE
u8 *Kc_MENU_LOGO;
#endif //MENU_LOGO_ENABLE

hekate_config h_cfg;

int sd_mount()
{
	if (sd_mounted)
		return 1;

	if (!sdmmc_storage_init_sd(&sd_storage, &sd_sdmmc, SDMMC_1, SDMMC_BUS_WIDTH_4, 11))
	{
		EPRINTF("Failed to init SD card.\nMake sure that it is inserted.");
	}
	else
	{
		int res = 0;
		res = f_mount(&sd_fs, "", 1);
		if (res == FR_OK)
		{
			sd_mounted = 1;
			return 1;
		}
		else
		{
			EPRINTFARGS("Failed to mount SD card (FatFS Error %d).\n(make sure that a FAT type partition exists)", res);
		}
	}

	return 0;
}

void sd_unmount()
{
	if (sd_mounted)
	{
		f_mount(NULL, "", 1);
		sdmmc_storage_end(&sd_storage);
		sd_mounted = 0;
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

int sd_save_to_file(void * buf, u32 size, const char * filename)
{
	FIL fp;
	u32 res = 0;
	res = f_open(&fp, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res) {
		EPRINTFARGS("Error (%d) creating file\n%s.\n", res, filename);
		return 1;
	}

	f_sync(&fp);
	f_write(&fp, buf, size, NULL);
	f_close(&fp);

	return 0;
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
	DISPLAY_A(0xCF8) |= 4;
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
	//Lock SSK (although it's not set and unused anyways).
	SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 15 * 4) = 0x7E;
	// Clear the boot reason to avoid problems later
	PMC(APBDEV_PMC_SCRATCH200) = 0x0;
	PMC(APBDEV_PMC_RST_STATUS_0) = 0x0;
}

void config_hw()
{
	// Bootrom stuff we skipped by going through rcm.
	config_se_brom();
	//FUSE(FUSE_PRIVATEKEYDISABLE) = 0x11;
	SYSREG(0x110) &= 0xFFFFFF9F;
	PMC(0x244) = ((PMC(0x244) >> 1) << 1) & 0xFFFFFFFD;

	mbist_workaround();
	clock_enable_se();

	// Enable fuse clock.
	clock_enable_fuse(1);
	// Disable fuse programming.
	fuse_disable_program();

	mc_enable();

	config_oscillators();
	APB_MISC(0x40) = 0;
	config_gpios();

	//clock_enable_uart(UART_C);
	//uart_init(UART_C, 115200);

	clock_enable_cl_dvfs();

	clock_enable_i2c(I2C_1);
	clock_enable_i2c(I2C_5);

	static const clock_t clock_unk1 = { CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_CONTROLLER_CLK_OUT_ENB_V, 0x42C, 0x1F, 0, 0 };
	static const clock_t clock_unk2 = { CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_CONTROLLER_CLK_OUT_ENB_V, 0, 0x1E, 0, 0 };
	clock_enable(&clock_unk1);
	clock_enable(&clock_unk2);

	i2c_init(I2C_1);
	i2c_init(I2C_5);

	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_CNFGBBC, 0x40);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_ONOFFCNFG1, 0x78);

	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_FPS_CFG0, 0x38);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_FPS_CFG1, 0x3A);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_FPS_CFG2, 0x38);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_FPS_LDO4, 0xF);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_FPS_LDO8, 0xC7);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_FPS_SD0, 0x4F);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_FPS_SD1, 0x29);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_FPS_SD3, 0x1B);

	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_SD0, 42); //42 = (1125000 - 600000) / 12500 -> 1.125V

	config_pmc_scratch();

	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = (CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) & 0xFFFF8888) | 0x3333;

	mc_config_carveout();

	sdram_init();
	//TODO: test this with LP0 wakeup.
	sdram_lp0_save_params(sdram_get_params());
}

void print_fuseinfo()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
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
			char fuseFilename[23];
			f_mkdir("Backup");
			f_mkdir("Backup/Dumps");
			memcpy(fuseFilename, "Backup/Dumps/fuses.bin\0", 23);

			if (sd_save_to_file((u8 *)0x7000F900, 0x2FC, fuseFilename))
				EPRINTF("\nError creating fuse.bin file.");
			else
				gfx_puts(&gfx_con, "\nDone!\n");
			sd_unmount();
		}

		btn_wait();
	}
}

void print_kfuseinfo()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
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
			char kfuseFilename[24];
			f_mkdir("Backup");
			f_mkdir("Backup/Dumps");
			memcpy(kfuseFilename, "Backup/Dumps/kfuses.bin\0", 24);

			if (sd_save_to_file((u8 *)buf, KFUSE_NUM_WORDS * 4, kfuseFilename))
				EPRINTF("\nError creating kfuse.bin file.");
			else
				gfx_puts(&gfx_con, "\nDone!\n");
			sd_unmount();
		}

		btn_wait();
	}
}

void print_mmc_info()
{
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	static const u32 SECTORS_TO_MIB_COEFF  = 11;

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

		gfx_printf(&gfx_con, "%kCard IDentification:%k\n", 0xFF00DDFF, 0xFFCCCCCC);
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
			gfx_printf(&gfx_con, "%kExtended Card-Specific Data V1.%d:%k\n",
				0xFF00DDFF, storage.ext_csd.ext_struct, 0xFFCCCCCC);
			card_type = storage.ext_csd.card_type;
			u8 card_type_support[96];
			u8 pos_type = 0;
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
	static const u32 SECTORS_TO_MIB_COEFF  = 11;
	
	gfx_clear_grey(&gfx_ctxt, 0x1B);
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
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	// Read package1.
	u8 *pkg1 = (u8 *)malloc(0x40000);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1);
	if (!pkg1_id)
	{
		EPRINTFARGS("Could not identify package1 version\nto read TSEC firmware (= '%s').",
			(char *)pkg1 + 0x10);
		goto out;
	}

	for(u32 i = 1; i <= 3; i++)
	{
		u8 key[0x10];
		int res = tsec_query(key, i, pkg1 + pkg1_id->tsec_off);

		gfx_printf(&gfx_con, "%kTSEC key %d: %k", 0xFF00DDFF, i, 0xFFCCCCCC);
		if (res >= 0)
		{
			for (u32 i = 0; i < 0x10; i++)
				gfx_printf(&gfx_con, "%02X", key[i]);
		}
		else
			EPRINTFARGS("ERROR %X", res);
		gfx_putc(&gfx_con, '\n');
	}

out:;
	free(pkg1);
	sdmmc_storage_end(&storage);

	gfx_puts(&gfx_con, "\nPress any key...\n");
	btn_wait();
}

void reboot_normal()
{
	sd_unmount();
#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	panic(0x21); // Bypass fuse programming in package1.
}

void reboot_rcm()
{
	sd_unmount();
#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	PMC(APBDEV_PMC_SCRATCH0) = 2; // Reboot into rcm.
	PMC(0) |= 0x10;
	while (1)
		usleep(1);
}

void power_off()
{
	sd_unmount();
#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	//TODO: we should probably make sure all regulators are powered off properly.
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_PWR_OFF);
}

int dump_emmc_verify(sdmmc_storage_t *storage, u32 lba_curr, char* outFilename, emmc_part_t *part)
{
	FIL fp;
	u32 prevPct = 200;
	int res = 0;

	u8 hashEm[0x20];
	u8 hashSd[0x20];

	if (f_open(&fp, outFilename, FA_READ) == FR_OK)
	{
		u32 totalSectorsVer = (u32)((u64)f_size(&fp)>>(u64)9);
		
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
			if (f_read(&fp, bufSd, num << 9, NULL) != FR_OK)
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
	int isSmallSdCard = 0;
	int partialDumpInProgress = 0;
	int res = 0;
	char *outFilename = sd_path;
	u32 sdPathLen = strlen(sd_path);

	FIL partialIdxFp;
	char partialIdxFilename[12];
	memcpy(partialIdxFilename, "partial.idx\0", 12);

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
		isSmallSdCard = 1;

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

		partialDumpInProgress = 1;
		// Force partial dumping, even if the card is larger.
		isSmallSdCard = 1;

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
	gfx_con_getpos(&gfx_con, &gfx_con.savedx,  &gfx_con.savedy);
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
					gfx_puts(&gfx_con, "\n\n1. Press any key and Power off Switch from the main menu.\n\
						2. Move the files from SD card to free space.\n\
						   Don\'t move the partial.idx file!\n\
						3. Unplug and re-plug USB while pressing Vol+.\n\
						4. Run hekate again and press Backup eMMC RAW GPP (or eMMC USER) to continue.\n");
					gfx_con.fntsz = 16;

					free(buf);
					return 1;
				}
			}

			// Create next part.
			gfx_con_setpos(&gfx_con, gfx_con.savedx,  gfx_con.savedy);
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

			free(buf);
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
	gfx_clear_grey(&gfx_ctxt, 0x1B);
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
	char sdPath[64];
	memcpy(sdPath, "Backup/", 7);
	// Create Backup/Restore folders, if they do not exist.
	f_mkdir("Backup");
	f_mkdir("Backup/Partitions");
	f_mkdir("Backup/Restore");
	f_mkdir("Backup/Restore/Partitions");

	timer = get_tmr_s();
	if (dumpType & PART_BOOT)
	{
		const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE/NX_EMMC_BLOCKSIZE)-1;
		for (i = 0; i < 2; i++)
		{
			memcpy(bootPart.name, "BOOT", 4);
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&storage, i+1);

			strcpy(sdPath + 7, bootPart.name);
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

				memcpy(sdPath, "Backup/Partitions/", 18);
				strcpy(sdPath + 18, part->name);
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
			rawPart.lba_end = RAW_AREA_NUM_SECTORS-1;
			strcpy(rawPart.name, "rawnand.bin");
			{
				gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
					rawPart.name, rawPart.lba_start, rawPart.lba_end, 0xFFCCCCCC);

				strcpy(sdPath + 7, rawPart.name);
				res = dump_emmc_part(sdPath, &storage, &rawPart);
			}
		}
	}

	gfx_putc(&gfx_con, '\n');
	timer = get_tmr_s() - timer;
	gfx_printf(&gfx_con, "Time taken: %dm %ds.\n", timer / 60, timer % 60);
	sdmmc_storage_end(&storage);
	if (res && h_cfg.verification)
		gfx_printf(&gfx_con, "\n%kFinished and verified!%k\nPress any key...\n",0xFF96FF00, 0xFFCCCCCC);
	else if (res)
		gfx_printf(&gfx_con, "\nFinished! Press any key...\n");

out:;
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
	gfx_con_getpos(&gfx_con, &gfx_con.savedx,  &gfx_con.savedy);
	gfx_printf(&gfx_con, "\nFilename: %s\n", outFilename);

	res = f_open(&fp, outFilename, FA_READ);
	if (res)
	{
		WPRINTFARGS("Error (%d) while opening backup. Continuing...\n", res);
		gfx_con.fntsz = 16;

		return 0;
	}
	//TODO: Should we keep this check?
	else if (((u32)((u64)f_size(&fp)>>(u64)9)) != totalSectors)
	{
		gfx_con.fntsz = 16;
		EPRINTF("Size of sd card backup does not match,\neMMC's selected part size.\n");
		f_close(&fp);

		return 0;
	}
	else
		gfx_printf(&gfx_con, "\nTotal restore size: %d MiB.\n\n", ((u32)((u64)f_size(&fp)>>(u64)9)) >> SECTORS_TO_MIB_COEFF);

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

			free(buf);
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
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, "%kThis is a dangerous operation\nand may render your device inoperative!\n\n", 0xFFFFDD00);
	gfx_printf(&gfx_con, "Are you really sure?\n\n%k", 0xFFCCCCCC);
	if ((restoreType & PART_BOOT) || (restoreType & PART_GP_ALL))
	{
		gfx_puts(&gfx_con, "The mode you selected will only restore\nthe partitions that it can find.\n");
		gfx_puts(&gfx_con, "If the appropriate named file is not found,\nit will skip it and continue with the next.\n\n");
	}
	gfx_con_getpos(&gfx_con, &gfx_con.savedx,  &gfx_con.savedy);

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
	char sdPath[64];
	memcpy(sdPath, "Backup/Restore/", 15);

	timer = get_tmr_s();
	if (restoreType & PART_BOOT)
	{
		const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE/NX_EMMC_BLOCKSIZE)-1;
		for (i = 0; i < 2; i++)
		{
			memcpy(bootPart.name, "BOOT", 4);
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&storage, i+1);

			strcpy(sdPath + 15, bootPart.name);
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

			memcpy(sdPath, "Backup/Restore/Partitions/", 26);
			strcpy(sdPath + 26, part->name);
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
		rawPart.lba_end = RAW_AREA_NUM_SECTORS-1;
		strcpy(rawPart.name, "rawnand.bin");
		{
			gfx_printf(&gfx_con, "%k%02d: %s (%07X-%07X)%k\n", 0xFF00DDFF, i++,
				rawPart.name, rawPart.lba_start, rawPart.lba_end, 0xFFCCCCCC);

			strcpy(sdPath + 15, rawPart.name);
			res = restore_emmc_part(sdPath, &storage, &rawPart);
		}
	}

	gfx_putc(&gfx_con, '\n');
	timer = get_tmr_s() - timer;
	gfx_printf(&gfx_con, "Time taken: %dm %ds.\n", timer / 60, timer % 60);
	sdmmc_storage_end(&storage);
	if (res && h_cfg.verification)
		gfx_printf(&gfx_con, "\n%kFinished and verified!%k\nPress any key...\n",0xFF96FF00, 0xFFCCCCCC);
	else if (res)
		gfx_printf(&gfx_con, "\nFinished! Press any key...\n");

out:;
	sd_unmount();
	btn_wait();
}

void restore_emmc_boot() { restore_emmc_selected(PART_BOOT); }
void restore_emmc_rawnand() { restore_emmc_selected(PART_RAW); }
void restore_emmc_gpp_parts() { restore_emmc_selected(PART_GP_ALL); }

//TODO: dump_package2

void dump_package1()
{
	u8 *pkg1 = (u8 *)calloc(1, 0x40000);
	u8 *warmboot = (u8 *)calloc(1, 0x40000);
	u8 *secmon = (u8 *)calloc(1, 0x40000);
	u8 *loader = (u8 *)calloc(1, 0x40000);

	gfx_clear_grey(&gfx_ctxt, 0x1B);
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
		EPRINTFARGS("Could not identify package1 version to read TSEC firmware (= '%s').", (char *)pkg1 + 0x10);
		goto out;
	}

	// Read keyblob.
	u8 * keyblob = (u8 *)malloc(NX_EMMC_BLOCKSIZE);
	sdmmc_storage_read(&storage, 0x180000 / NX_EMMC_BLOCKSIZE + pkg1_id->kb, 1, keyblob);

	// Decrypt.
	keygen(keyblob, pkg1_id->kb, (u8 *)pkg1 + pkg1_id->tsec_off);
	pkg1_decrypt(pkg1_id, pkg1);

	pkg1_unpack(warmboot, secmon, loader, pkg1_id, pkg1);

	// Display info.
	gfx_printf(&gfx_con, "%kNX Bootloader size:  %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->ldr_size);
	gfx_printf(&gfx_con, "%kNX Bootloader ofst:  %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->ldr_off);

	gfx_printf(&gfx_con, "%kSecure monitor addr: %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, pkg1_id->secmon_base);
	gfx_printf(&gfx_con, "%kSecure monitor size: %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->sm_size);
	gfx_printf(&gfx_con, "%kSecure monitor ofst: %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->sm_off);

	gfx_printf(&gfx_con, "%kWarmboot addr:       %k0x%05X\n", 0xFFC7EA46, 0xFFCCCCCC, pkg1_id->warmboot_base);
	gfx_printf(&gfx_con, "%kWarmboot size:       %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->wb_size);
	gfx_printf(&gfx_con, "%kWarmboot ofst:       %k0x%05X\n\n", 0xFFC7EA46, 0xFFCCCCCC, hdr->wb_off);

	// Dump package1.
	f_mkdir("Backup");
	f_mkdir("Backup/pkg1");
	if (sd_save_to_file(pkg1, 0x40000, "Backup/pkg1/pkg1_decr.bin")) {
		EPRINTF("\nFailed to create pkg1_decr.bin");
		goto out;
	}
	gfx_puts(&gfx_con, "\nFull package1 dumped to pkg1_decr.bin\n");

	// Dump nxbootloader.
	if (sd_save_to_file(loader, hdr->ldr_size, "Backup/pkg1/nxloader.bin")) {
		EPRINTF("\nFailed to create nxloader.bin");
		goto out;
	}
	gfx_puts(&gfx_con, "NX Bootloader dumped to nxloader.bin\n");

	// Dump secmon.
	if (sd_save_to_file(secmon, hdr->sm_size, "Backup/pkg1/secmon.bin")) {
		EPRINTF("\nFailed to create secmon.bin");
		goto out;
	}
	gfx_puts(&gfx_con, "Secure Monitor dumped to secmon.bin\n");

	// Dump warmboot.
	if (sd_save_to_file(warmboot, hdr->wb_size, "Backup/pkg1/warmboot.bin")) {
		EPRINTF("\nFailed to create warmboot.bin");
		goto out;
	}
	gfx_puts(&gfx_con, "Warmboot dumped to warmboot.bin\n");


	sdmmc_storage_end(&storage);
	sd_unmount();
	gfx_puts(&gfx_con, "\nDone. Press any key...\n");

out:;
	free(pkg1);
	free(secmon);
	free(warmboot);
	free(loader);

	btn_wait();
}

void launch_firmware()
{
	u8 max_entries = 61;

	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_sections);

	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (sd_mount())
	{
		if (ini_parse(&ini_sections, "hekate_ipl.ini"))
		{
			// Build configuration menu.
			ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * (max_entries + 3));
			ments[0].type = MENT_BACK;
			ments[0].caption = "Back";
			ments[1].type = MENT_CHGLINE;

			u32 i = 2;
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

				if (i > max_entries)
					break;
			}
			if (i > 2)
			{
				memset(&ments[i], 0, sizeof(ment_t));
				menu_t menu = {
					ments, "Launch configurations", 0, 0
				};
				cfg_sec = ini_clone_section((ini_sec_t *)tui_do_menu(&gfx_con, &menu));
				if (!cfg_sec)
				{
					free(ments);
					ini_free(&ini_sections);
					return;
				}
			}
			else
				EPRINTF("No launch configurations found.");
			free(ments);
			ini_free(&ini_sections);
		}
		else
			EPRINTF("Could not find or open 'hekate_ipl.ini'.\nMake sure it exists in SD Card!.");
	}

	if (!cfg_sec)
	{
		gfx_printf(&gfx_con, "\nUsing default launch configuration...\n");
		msleep(3000);
	}
#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	if (!hos_launch(cfg_sec))
	{
#ifdef MENU_LOGO_ENABLE
		Kc_MENU_LOGO = (u8 *)malloc(0x6000);
		LZ_Uncompress(Kc_MENU_LOGOlz, Kc_MENU_LOGO, SZ_MENU_LOGOLZ);
#endif //MENU_LOGO_ENABLE
		EPRINTF("Failed to launch firmware.");
	}

	ini_free_section(cfg_sec);
	sd_unmount();

	btn_wait();
}

void auto_launch_firmware()
{
	u8 *BOOTLOGO = NULL;
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
	int ini_freed = 1;
	int backlightEnabled = 0;
	int bootlogoFound = 0;
	char *bootlogoCustomEntry = NULL;

	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_sections);

	gfx_con.mute = 1;

	if (sd_mount())
	{
		if (ini_parse(&ini_sections, "hekate_ipl.ini"))
		{
			ini_freed = 0;
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
							else if (!strcmp("bootwait", kv->key))
								h_cfg.bootwait = atoi(kv->val);
							else if (!strcmp("customlogo", kv->key))
								h_cfg.customlogo = atoi(kv->val);
							else if (!strcmp("verification", kv->key))
								h_cfg.verification = atoi(kv->val);
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
						}	
						break;	
					}
					boot_entry_id++;
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
				bitmap = (u8 *)sd_file_read("bootlogo.bmp");
		}
		else // User has not set a custom logo path.
			bitmap = (u8 *)sd_file_read("bootlogo.bmp");

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

					bootlogoFound = 1;
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
		BOOTLOGO = (void *)malloc(0x4000);
		LZ_Uncompress(BOOTLOGO_LZ, BOOTLOGO, SZ_BOOTLOGO_LZ);
		gfx_set_rect_grey(&gfx_ctxt, BOOTLOGO, X_BOOTLOGO, Y_BOOTLOGO, 326, 544);
		free(BOOTLOGO);
	}
	free(BOOTLOGO);

	display_backlight(1);
	backlightEnabled = 1;

	// Wait before booting. If VOL- is pressed go into bootloader menu.
	u32 btn = btn_wait_timeout(h_cfg.bootwait * 1000, BTN_VOL_DOWN);

	if (btn & BTN_VOL_DOWN)
		goto out;

	ini_free(&ini_sections);
	ini_freed = 1;

#ifdef MENU_LOGO_ENABLE
	free(Kc_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
	if (!hos_launch(cfg_sec))
	{
		// Failed to launch firmware.
#ifdef MENU_LOGO_ENABLE
		Kc_MENU_LOGO = (u8 *)malloc(ALIGN(SZ_MENU_LOGO, 0x10));
		LZ_Uncompress(Kc_MENU_LOGOlz, Kc_MENU_LOGO, SZ_MENU_LOGOLZ);
#endif //MENU_LOGO_ENABLE
	}

out:;
	if (!ini_freed)
		ini_free(&ini_sections);
	ini_free_section(cfg_sec);

	sd_unmount();
	gfx_con.mute = 0;

	if (!backlightEnabled)
		display_backlight(1);
}

void toggle_autorcm(){
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	gfx_clear_grey(&gfx_ctxt, 0x1B);
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
		tempbuf[0x10] ^= 0x77; // !IMPORTANT: DO NOT CHANGE! XOR by arbitrary number to corrupt.
		sdmmc_storage_write(&storage, sect, 1, tempbuf);
	}
	
	free(tempbuf);
	sdmmc_storage_end(&storage);
	
	gfx_printf(&gfx_con, "%kAutoRCM mode toggled!%k\n\nPress any key...\n", 0xFF96FF00, 0xFFCCCCCC);

out:;
	btn_wait();
}

int fix_attributes(char *path, u32 *total, u32 is_root, u32 check_first_run)
{
	FRESULT res;
	DIR dir;
	u32 dirLength = 0;
	static FILINFO fno;

	// Should we set the bit of the entry directory?
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
		memcpy(&path[dirLength+1], fno.fname, strlen(fno.fname) + 1);

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
	gfx_clear_grey(&gfx_ctxt, 0x1B);
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
			memcpy(label, "sd card", 8);
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

	max17050_get_property(MAX17050_Cycles, &value);
	gfx_printf(&gfx_con, "Charge cycle count:     %3d%\n", value);

	max17050_get_property(MAX17050_TEMP, &value);
	if (value >= 0)
		gfx_printf(&gfx_con, "Battery temperature:    %d.%d oC\n", value / 10, value % 10);
	else
		gfx_printf(&gfx_con, "Battery temperature:    -%d.%d oC\n", ~value / 10, (~value) % 10);

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

	max17050_get_property(MAX17050_MinVolt, &value);
	gfx_printf(&gfx_con, "Min voltage reached:    %4d mV\n", value);

	max17050_get_property(MAX17050_MaxVolt, &value);
	gfx_printf(&gfx_con, "Max voltage reached:    %4d mV\n", value);

	max17050_get_property(MAX17050_V_empty, &value);
	gfx_printf(&gfx_con, "Empty voltage (design): %4d mV\n", value);

	max17050_get_property(MAX17050_VCELL, &value);
	gfx_printf(&gfx_con, "Voltage now:            %4d mV\n", value);

	max17050_get_property(MAX17050_OCVInternal, &value);
	gfx_printf(&gfx_con, "Voltage open-circuit:   %4d mV\n", value);

	max17050_get_property(MAX17050_RepSOC, &value);
	gfx_printf(&gfx_con, "Capacity now:           %3d%\n", value >> 8);

	max17050_get_property(MAX17050_RepCap, &value);
	gfx_printf(&gfx_con, "Capacity now:           %4d mAh\n", value);

	max17050_get_property(MAX17050_FullCAP, &value);
	gfx_printf(&gfx_con, "Capacity full:          %4d mAh\n", value);

	max17050_get_property(MAX17050_DesignCap, &value);
	gfx_printf(&gfx_con, "Capacity (design):      %4d mAh\n", value);
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
	gfx_clear_grey(&gfx_ctxt, 0x1B);
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
			char fuelFilename[28];
			f_mkdir("Backup");
			f_mkdir("Backup/Dumps");
			memcpy(fuelFilename, "Backup/Dumps/fuel_gauge.bin\0", 28);

			if (sd_save_to_file((u8 *)buf, 0x200, fuelFilename))
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
	gfx_clear_grey(&gfx_ctxt, 0x1B);
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

	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, "%k\nThis will wipe your battery stats completely!\n"
		"%kAnd it may not power on without physically\nremoving and re-inserting the battery.\n%k"
		"\nAre you really sure?%k\n", 0xFFFFDD00, 0xFFFF0000, 0xFFFFDD00, 0xFFCCCCCC);

	gfx_puts(&gfx_con, "\nPress POWER to Continue.\nPress VOL to go to the menu.\n\n\n");
	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		gfx_clear_grey(&gfx_ctxt, 0x1B);
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
	gfx_clear_grey(&gfx_ctxt, 0x1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	max77620_low_battery_monitor_config();

	gfx_puts(&gfx_con, "\nDone!\n");

	btn_wait();
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
	"   Copyright (C) 2018, Atmosphere-NX\n"
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
	MDEF_END()
};

menu_t menu_options = {
	ment_options,
	"Launch options", 0, 0
};

ment_t ment_cinfo[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- SoC Info ----", 0xFF0AB9E6),
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
	"Console info", 0, 0
};

ment_t ment_autorcm[] = {
	MDEF_CAPTION("WARNING: This corrupts your BOOT0 partition!", 0xFFE6FF00),
	MDEF_CHGLINE(),
	MDEF_CAPTION("Do you want to continue?", 0xFFCCCCCC),
	MDEF_CHGLINE(),
	MDEF_BACK(),
	MDEF_BACK(),
	MDEF_BACK(),
	MDEF_BACK(),
	MDEF_HANDLER("Toggle AutoRCM", toggle_autorcm),
	MDEF_BACK(),
	MDEF_BACK(),
	MDEF_BACK(),
	MDEF_BACK(),
	MDEF_END()
};

menu_t menu_autorcm = {
	ment_autorcm,
	"Toggle AutoRCM ON/OFF", 0, 0
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
	"Restore options", 0, 0
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
	"Backup options", 0, 0
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
	MDEF_HANDLER("Dump package1", dump_package1),
	MDEF_HANDLER("Fix battery de-sync", fix_battery_desync),
	MDEF_HANDLER("Unset switch archive attributes", fix_sd_switch_attr),
	MDEF_HANDLER("Unset all archive attributes", fix_sd_all_attr),
	//MDEF_HANDLER("Fix fuel gauge configuration", fix_fuel_gauge_configuration),
	//MDEF_HANDLER("Reset all battery cfg", reset_pmic_fuel_gauge_charger_config),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Dangerous -----", 0xFFFF0000),
	MDEF_MENU("AutoRCM", &menu_autorcm),
	MDEF_END()
};

menu_t menu_tools = {
	ment_tools,
	"Tools", 0, 0
};

ment_t ment_top[] = {
	MDEF_HANDLER("Launch firmware", launch_firmware),
	MDEF_MENU("Launch options", &menu_options),
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
	"hekate - CTCaer mod v3.1", 0, 0
};

extern void pivot_stack(u32 stack_top);

void ipl_main()
{
	config_hw();

	//Pivot the stack so we have enough space.
	pivot_stack(0x90010000);

	//Tegra/Horizon configuration goes to 0x80000000+, package2 goes to 0xA9800000, we place our heap in between.
	heap_init(0x90020000);

	//uart_send(UART_C, (u8 *)0x40000000, 0x10000);
	//uart_wait_idle(UART_C, UART_TX_IDLE);

	display_init();
	//display_color_screen(0xAABBCCDD);
	u32 *fb = display_init_framebuffer();
	gfx_init_ctxt(&gfx_ctxt, fb, 720, 1280, 768);
	gfx_clear_grey(&gfx_ctxt, 0x1B);

#ifdef MENU_LOGO_ENABLE
	Kc_MENU_LOGO = (u8 *)malloc(0x6000);
	LZ_Uncompress(Kc_MENU_LOGOlz, Kc_MENU_LOGO, SZ_MENU_LOGOLZ);
#endif //MENU_LOGO_ENABLE

	gfx_con_init(&gfx_con, &gfx_ctxt);

	// Enable backlight after initializing gfx
	//display_backlight(1);
	set_default_configuration();
	// Load saved configuration and auto boot if enabled.
	auto_launch_firmware();

	while (1)
		tui_do_menu(&gfx_con, &menu_top);

	while (1)
		;
}
