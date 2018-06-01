/*
* Copyright (c) 2018 naehrwert
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
#include <alloca.h>

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
#include "tui.h"
#include "heap.h"
#include "list.h"
#include "nx_emmc.h"
#include "se.h"
#include "se_t210.h"
#include "hos.h"
#include "pkg1.h"
#include "mmc.h"

//TODO: ugly.
gfx_ctxt_t gfx_ctxt;
gfx_con_t gfx_con;

//TODO: Create more macros (info, header, debug, etc) with different colors and utilize them for consistency.
#define EPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFF0000FF, 0xFFCCCCCC)
#define EPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFF0000FF, args, 0xFFCCCCCC)

//TODO: ugly.
sdmmc_t sd_sdmmc;
sdmmc_storage_t sd_storage;
FATFS sd_fs;
int sd_mounted;

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
		gfx_puts(&gfx_con, "Unmounting SD card...\n");
		f_mount(NULL, "", 1);
		sdmmc_storage_end(&sd_storage);
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
		u32 rsize = MIN(size, 512);
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
	if (f_open(&fp, filename, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		return 1;
	}

	f_sync(&fp);
	f_write(&fp, buf, size, NULL);
	f_close(&fp);

	return 0;
}

void panic(u32 val)
{
	//Set panic code.
	PMC(APBDEV_PMC_SCRATCH200) = val;
	//PMC(APBDEV_PMC_CRYPTO_OP) = 1; //Disable SE.
	TMR(0x18C) = 0xC45A;
	TMR(0x80) = 0xC0000000;
	TMR(0x180) = 0x8019;
	TMR(0x188) = 1;
	while (1)
		;
}

void config_oscillators()
{
	CLOCK(CLK_RST_CONTROLLER_SPARE_REG0) = CLOCK(CLK_RST_CONTROLLER_SPARE_REG0) & 0xFFFFFFF3 | 4;
	SYSCTR0(SYSCTR0_CNTFID0) = 19200000;
	TMR(0x14) = 0x45F;
	CLOCK(CLK_RST_CONTROLLER_OSC_CTRL) = 0x50000071;
	PMC(APBDEV_PMC_OSC_EDPD_OVER) = PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFFFFF81 | 0xE;
	PMC(APBDEV_PMC_OSC_EDPD_OVER) = PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFBFFFFF | 0x400000;
	PMC(APBDEV_PMC_CNTRL2) = PMC(APBDEV_PMC_CNTRL2) & 0xFFFFEFFF | 0x1000;
	PMC(APBDEV_PMC_SCRATCH188) = PMC(APBDEV_PMC_SCRATCH188) & 0xFCFFFFFF | 0x2000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 0x10;
	CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE) &= 0xBFFFFFFF;
	PMC(APBDEV_PMC_TSC_MULT) = PMC(APBDEV_PMC_TSC_MULT) & 0xFFFF0000 | 0x249F; //0x249F = 19200000 * (16 / 32.768 kHz)
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20004444;
	CLOCK(CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER) = 0x80000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 2;
}

void config_gpios()
{
	PINMUX_AUX(PINMUX_AUX_UART2_TX) = 0;
	PINMUX_AUX(PINMUX_AUX_UART3_TX) = 0;

	PINMUX_AUX(PINMUX_AUX_GPIO_PE6) = 0x40;
	PINMUX_AUX(PINMUX_AUX_GPIO_PH6) = 0x40;

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

	//Configure volume up/down as inputs.
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
	CLOCK(0x410) = (CLOCK(0x410) | 0x8000) & 0xFFFFBFFF;
	CLOCK(0xD0) |= 0x40800000u;
	CLOCK(0x2AC) = 0x40;
	CLOCK(0x294) = 0x40000;
	CLOCK(0x304) = 0x18000000;
	sleep(2);

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
	sleep(2);

	CLOCK(0x2A8) = 0x40;
	CLOCK(0x300) = 0x18000000;
	CLOCK(0x290) = 0x40000;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_H) = 0xC0;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) = 0x80000130;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_U) = 0x1F00200;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) = 0x80400808;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_W) = 0x402000FC;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X) = 0x23000780;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) = 0x300;
	CLOCK(0xF8) = 0;
	CLOCK(0xFC) = 0;
	CLOCK(0x3A0) = 0;
	CLOCK(0x3A4) = 0;
	CLOCK(0x554) = 0;
	CLOCK(0xD0) &= 0x1F7FFFFF;
	CLOCK(0x410) &= 0xFFFF3FFF;
	CLOCK(0x148) = CLOCK(0x148) & 0x1FFFFFFF | 0x80000000;
	CLOCK(0x180) = CLOCK(0x180) & 0x1FFFFFFF | 0x80000000;
	CLOCK(0x6A0) = CLOCK(0x6A0) & 0x1FFFFFFF | 0x80000000;
}

void config_se_brom()
{
	//Bootrom part we skipped.
	u32 sbk[4] = { FUSE(0x1A4), FUSE(0x1A8), FUSE(0x1AC), FUSE(0x1B0) };
	se_aes_key_set(14, sbk, 0x10);
	//Lock SBK from being read.
	SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 14 * 4) = 0x7E;
	//This memset needs to happen here, else TZRAM will behave weirdly later on.
	memset((void *)0x7C010000, 0, 0x10000);
	PMC(APBDEV_PMC_CRYPTO_OP) = 0;
	SE(SE_INT_STATUS_REG_OFFSET) = 0x1F;
	//Lock SSK (although it's not set and unused anyways).
	SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 15 * 4) = 0x7E;
	// Clear the boot reason to avoid problems later
	PMC(APBDEV_PMC_SCRATCH200) = 0x0;
	PMC(APBDEV_PMC_RST_STATUS_0) = 0x0;
	PMC(APBDEV_PMC_SCRATCH49_0) = 0x0;
}

void config_hw()
{
	//Bootrom stuff we skipped by going through rcm.
	config_se_brom();
	//FUSE(FUSE_PRIVATEKEYDISABLE) = 0x11;
	SYSREG(0x110) &= 0xFFFFFF9F;
	PMC(0x244) = ((PMC(0x244) >> 1) << 1) & 0xFFFFFFFD;

	mbist_workaround();
	clock_enable_se();

	//Enable fuse clock.
	clock_enable_fuse(1);
	//Disable fuse programming.
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

	static const clock_t clock_unk1 = { 0x358, 0x360, 0x42C, 0x1F, 0, 0 };
	static const clock_t clock_unk2 = { 0x358, 0x360, 0, 0x1E, 0, 0 };
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

	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) & 0xFFFF8888 | 0x3333;

	mc_config_carveout();

	sdram_init();
	//TODO: test this with LP0 wakeup.
	sdram_lp0_save_params(sdram_get_params());
}

void print_fuseinfo()
{
	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, "%k(Unlocked) fuse cache:\n\n%k", 0xFFFFDD00, 0xFFCCCCCC);
	gfx_hexdump(&gfx_con, 0x7000F900, (u8 *)0x7000F900, 0x2FC);

	gfx_puts(&gfx_con, "\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char fuseFilename[9];
			memcpy(fuseFilename, "fuse.bin", 8);
			fuseFilename[8] = 0;

			if (sd_save_to_file((u8 *)0x7000F900, 0x2FC, fuseFilename))
				EPRINTF("\nError creating fuse.bin file.");
			else
				gfx_puts(&gfx_con, "\nDone!\n");				
		}

		btn_wait();
	}
}

void print_kfuseinfo()
{
	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, "%kKFuse contents:\n\n%k", 0xFFFFDD00, 0xFFCCCCCC);
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
			char kfuseFilename[10];
			memcpy(kfuseFilename, "kfuse.bin", 9);
			kfuseFilename[9] = 0;

			if (sd_save_to_file((u8 *)buf, KFUSE_NUM_WORDS * 4, kfuseFilename))
				EPRINTF("\nError creating kfuse.bin file.");
			else
				gfx_puts(&gfx_con, "\nDone!\n");
		}

		btn_wait();
	}
}

void print_mmc_info()
{
	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	static const u32 SECTORS_TO_MIB_COEFF  = 11;

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	if(!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}
	else
	{
		u16 card_type;
		u32 speed;

		gfx_printf(&gfx_con, "%kCard IDentification:%k\n", 0xFFFFDD00, 0xFFCCCCCC);
		switch (storage.csd.mmca_vsn)
		{
		case 0: /* MMC v1.0 - v1.2 */
		case 1: /* MMC v1.4 */
			gfx_printf(&gfx_con,
			" Vendor ID:  %03X\n\
			 Model:      %c%c%c%c%c%c%c\n\
			 HW rev:     %X\n\
			 FW rev:     %X\n\
			 S/N:        %03X\n\
			 Month/Year: %02d/%04d\n\n",
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
			" Vendor ID:  %X\n\
			 Card/BGA:   %X\n\
			 OEM ID:     %02X\n\
			 Model:      %c%c%c%c%c%c\n\
			 Prd Rev:    %X\n\
			 S/N:        %04X\n\
			 Month/Year: %02d/%04d\n\n",
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
				0xFFFFDD00, storage.ext_csd.ext_struct, 0xFFCCCCCC);
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
				" Spec Version:  %02X\n\
				 Extended Rev:  1.%d\n\
				 Dev Version:   %d\n\
				 Cmd Classes:   %02X\n\
				 Capacity:      %s\n\
				 Max Speed:     %d MB/s (%d MHz)\n\
				 Type Support:  %s\n\n",
				storage.csd.mmca_vsn, storage.ext_csd.rev, storage.ext_csd.dev_version, storage.csd.cmdclass,
				storage.csd.capacity == (4096 * 512) ? "High" : "Low", speed & 0xFFFF, (speed >> 16) & 0xFFFF, card_type_support);

			u32 boot_size = storage.ext_csd.boot_mult << 17;
			u32 rpmb_size = storage.ext_csd.rpmb_mult << 17;
			gfx_printf(&gfx_con, "%keMMC Partitions:%k\n", 0xFFFFDD00, 0xFFCCCCCC);
			gfx_printf(&gfx_con, " 1: %kBOOT0      %kSize: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF00FF96, 0xFFCCCCCC,
				boot_size / 1024, boot_size / 1024 / 512);
			gfx_printf(&gfx_con, " 2: %kBOOT1      %kSize: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF00FF96, 0xFFCCCCCC,
				boot_size / 1024, boot_size / 1024 / 512);
			gfx_printf(&gfx_con, " 3: %kRPMB       %kSize: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF00FF96, 0xFFCCCCCC,
				rpmb_size / 1024, rpmb_size / 1024 / 512);
			gfx_printf(&gfx_con, " 0: %kGPP (USER) %kSize: %5d MiB (LBA Sectors: 0x%07X)\n\n", 0xFF00FF96, 0xFFCCCCCC,
				storage.sec_cnt >> SECTORS_TO_MIB_COEFF, storage.sec_cnt);
			gfx_printf(&gfx_con, "%kGPP (eMMC USER) partition table:%k\n", 0xFFFFDD00, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&storage, 0);
			LIST_INIT(gpt);
			nx_emmc_gpt_parse(&gpt, &storage);
			int gpp_idx = 0;
			LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
			{
				gfx_printf(&gfx_con, " %02d: %k%s%k\n     Size: % 5d MiB (LBA Sectors 0x%07X, LBA Range: %08X-%08X)\n",
					gpp_idx++, 0xFF14FDAE, part->name, 0xFFCCCCCC, (part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
					part->lba_end - part->lba_start + 1, part->lba_start, part->lba_end);
			}
		}
	}

out:
	sdmmc_storage_end(&storage);

	btn_wait();
}

void print_sdcard_info()
{
	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	static const u32 SECTORS_TO_MIB_COEFF  = 11;

	if (sd_mount())
	{
		u32 capacity;

		gfx_printf(&gfx_con, "%kCard IDentification:%k\n", 0xFFFFDD00, 0xFFCCCCCC);
		gfx_printf(&gfx_con,
			" Vendor ID:  %02x\n\
			 OEM ID:     %c%c\n\
			 Model:      %c%c%c%c%c\n\
			 HW rev:     %X\n\
			 FW rev:     %X\n\
			 S/N:        %08x\n\
			 Month/Year: %02d/%04d\n\n",
			sd_storage.cid.manfid, (sd_storage.cid.oemid >> 8) & 0xFF, sd_storage.cid.oemid & 0xFF,
			sd_storage.cid.prod_name[0], sd_storage.cid.prod_name[1], sd_storage.cid.prod_name[2],
			sd_storage.cid.prod_name[3], sd_storage.cid.prod_name[4],
			sd_storage.cid.hwrev, sd_storage.cid.fwrev, sd_storage.cid.serial,
			sd_storage.cid.month, sd_storage.cid.year);

		gfx_printf(&gfx_con, "%kCard-Specific Data V%d.0:%k\n", 0xFFFFDD00, sd_storage.csd.structure + 1, 0xFFCCCCCC);
		capacity = sd_storage.csd.capacity >> (20 - sd_storage.csd.read_blkbits);
		gfx_printf(&gfx_con,
			" Cmd Classes:    %02X\n\
			 Capacity:       %d MiB\n\
			 Bus Width:      %d\n\
			 Speed Class:    %d\n\
			 UHS Grade:      U%d\n\
			 Video Class:    V%d\n\
			 App perf class: A%d\n\
			 Write Protect:  %d\n\n",
			sd_storage.csd.cmdclass, capacity,
			sd_storage.ssr.bus_width, sd_storage.ssr.speed_class, sd_storage.ssr.uhs_grade,
			sd_storage.ssr.video_class, sd_storage.ssr.app_class, sd_storage.csd.write_protect);

		gfx_puts(&gfx_con, "Acquiring FAT volume info...\n\n");
		f_getfree("", &sd_fs.free_clst, NULL);
		gfx_printf(&gfx_con, "%kFound %s volume:%k\n Free:    %d MiB\n Cluster: %d KiB\n",
				0xFFFFDD00, sd_fs.fs_type == FS_EXFAT ? "exFAT" : "FAT32", 0xFFCCCCCC,
				sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF, (sd_fs.csize > 1) ? (sd_fs.csize >> 1) : 512);
	}

	btn_wait();
}

void print_tsec_key()
{
	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;

	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	//Read package1.
	u8 *pkg1 = (u8 *)malloc(0x40000);
	sdmmc_storage_set_mmc_partition(&storage, 1);
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1);
	if (!pkg1_id)
	{
		EPRINTFARGS("Could not identify package1 version to read TSEC firmware (= '%s').",
			(char *)pkg1 + 0x10);
		goto out;
	}

	for(u32 i = 1; i <= 3; i++)
	{
		u8 key[0x10];
		int res = tsec_query(key, i, pkg1 + pkg1_id->tsec_off);

		gfx_printf(&gfx_con, "%kTSEC key %d: %k", 0xFFFFDD00, i, 0xFFCCCCCC);
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
	panic(0x21); //Bypass fuse programming in package1.
}

void reboot_rcm()
{
	sd_unmount();
	PMC(APBDEV_PMC_SCRATCH0) = 2; //Reboot into rcm.
	PMC(0) |= 0x10;
	while (1)
		sleep(1);
}

void power_off()
{
	sd_unmount();
	//TODO: we should probably make sure all regulators are powered off properly.
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_PWR_OFF);
}

int dump_emmc_verify(sdmmc_storage_t *storage, u32 lba_curr, char* outFilename, u32 NUM_SECTORS_PER_ITER, emmc_part_t *part)
{
	FIL fp;
	u32 prevPct = 200;

	if (f_open(&fp, outFilename, FA_READ) == FR_OK)
	{
		u8 *bufEm = (u8 *)malloc(NX_EMMC_BLOCKSIZE * NUM_SECTORS_PER_ITER);
		u8 *bufSd = (u8 *)malloc(NX_EMMC_BLOCKSIZE * NUM_SECTORS_PER_ITER);

		u32 totalSectorsVer = (u32)(f_size(&fp) >> 9);
		u32 lbaCurrVer = lba_curr - totalSectorsVer;

		u32 pct = (u64)((u64)(lbaCurrVer - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		tui_pbar(&gfx_con, 0, gfx_con.y, pct, 0xFF00FF96, 0xFF005515);

		while (totalSectorsVer > 0)
		{
			u32 num = MIN(totalSectorsVer, NUM_SECTORS_PER_ITER);

			if(!sdmmc_storage_read(storage, lbaCurrVer, num, bufEm))
			{
				gfx_con_setfontsz(&gfx_con, 16);
				EPRINTFARGS("\nFailed to read %d blocks @ LBA %08X\nfrom eMMC. Aborting..\n",
				num, lbaCurrVer);

				free(bufEm);
				free(bufSd);
				f_close(&fp);
				return 1;
			}
			if (!(f_read(&fp, bufSd, num, NULL) == FR_OK))
			{
				gfx_con_setfontsz(&gfx_con, 16);
				EPRINTFARGS("\nFailed to read %d blocks from sd card.\nVerification failed..\n", num);

				free(bufEm);
				free(bufSd);
				f_close(&fp);
				return 1;
			}

			if(!memcmp(bufEm, bufSd, num << 9))
			{
				gfx_con_setfontsz(&gfx_con, 16);
				EPRINTFARGS("\nVerification failed.\nVerification failed..\n", num);

				free(bufEm);
				free(bufSd);
				f_close(&fp);
				return 1;
			}

			pct = (u64)((u64)(lbaCurrVer - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
			if (pct != prevPct)
			{
				tui_pbar(&gfx_con, 0, gfx_con.y, pct, 0xFF00FF96, 0xFF005515);
				prevPct = pct;
			}

			lbaCurrVer += num;
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
		EPRINTF("\nFile not found or could not be loaded.\nVerification failed..\n");
		gfx_con_setfontsz(&gfx_con, 16);
		return 1;
	}
}

int dump_emmc_part(char *sd_path, sdmmc_storage_t *storage, emmc_part_t *part)
{
	static const u32 FAT32_FILESIZE_LIMIT = 0xFFFFFFFF;
	static const u32 SECTORS_TO_MIB_COEFF  = 11;

	u32 multipartSplitSize = (1u << 31);
	u32 totalSectors = part->lba_end - part->lba_start + 1;
	u32 currPartIdx = 0;
	u32 numSplitParts = 0;
	u32 maxSplitParts = 0;
	int isSmallSdCard = 0;
	int partialDumpInProgress = 0;
	int res = 0;
	char* outFilename = sd_path;
	u32 sdPathLen = strlen(sd_path);

	FIL partialIdxFp;
	char partialIdxFilename[12];
	memcpy(partialIdxFilename, "partial.idx", 11);
	partialIdxFilename[11] = 0;

	gfx_printf(&gfx_con, "\nSD Card free space: %d MiB, Total dump size %d MiB\n\n",
		sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF,
		totalSectors >> SECTORS_TO_MIB_COEFF);

	// 1GB parts for sd cards 8GB and less
	if ((sd_storage.csd.capacity >> (20 - sd_storage.csd.read_blkbits)) <= 8192)
		multipartSplitSize = (1u << 30);
	// Maximum parts fitting the free space available
	maxSplitParts = (sd_fs.free_clst * sd_fs.csize) / (multipartSplitSize / 512);

	// Check if the USER partition or the RAW eMMC fits the sd card free space
	if (totalSectors > (sd_fs.free_clst * sd_fs.csize))
	{
		isSmallSdCard = 1;

		gfx_printf(&gfx_con, "%k\nSD card free space is smaller than dump total size.%k\n", 0xFF00BAFF, 0xFFCCCCCC);

		if (!maxSplitParts)
		{
			EPRINTF("Not enough free space for partial dumping.");

			return 0;
		}
	}
	// Check if we continuing a previous raw eMMC dump in progress.
	if (f_open(&partialIdxFp, partialIdxFilename, FA_READ) == FR_OK)
	{
		gfx_printf(&gfx_con, "%kFound partial dump in progress. Continuing...%k\n\n", 0xFF14FDAE, 0xFFCCCCCC);

		partialDumpInProgress = 1;
		// Force partial dumping, even if the card is larger.
		isSmallSdCard = 1;

		f_read(&partialIdxFp, &currPartIdx, 4, NULL);
		f_close(&partialIdxFp);

		if (!maxSplitParts)
		{
			EPRINTF("Not enough free space for partial dumping.");

			return 0;
		}

		// Increase maxSplitParts to accommodate previously dumped parts
		maxSplitParts += currPartIdx;
	}
	else if (isSmallSdCard)
		gfx_printf(&gfx_con, "%kPartial dumping enabled (with %d MiB parts)...%k\n\n", 0xFF00BAFF, multipartSplitSize >> 20, 0xFFCCCCCC);

	// Check if filesystem is FAT32 or the free space is smaller and dump in parts
	if (((sd_fs.fs_type != FS_EXFAT) && totalSectors > (FAT32_FILESIZE_LIMIT / NX_EMMC_BLOCKSIZE)) | isSmallSdCard)
	{
		u32 multipartSplitSectors = multipartSplitSize / NX_EMMC_BLOCKSIZE;
		numSplitParts = (totalSectors + multipartSplitSectors - 1) / multipartSplitSectors;

		outFilename = alloca(sdPathLen+4);
		memcpy(outFilename, sd_path, sdPathLen);
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
		// Continue from where we left, if partial dump in progress.
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
	if (f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
	{
		EPRINTFARGS("Error creating file %s.\n", outFilename);

		return 0;
	}

	static const u32 NUM_SECTORS_PER_ITER = 512;
	u8 *buf = (u8 *)malloc(NX_EMMC_BLOCKSIZE * NUM_SECTORS_PER_ITER);

	u32 lba_curr = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;

	// Continue from where we left, if partial dump in progress.
	if (partialDumpInProgress)
	{
		lba_curr += currPartIdx * (multipartSplitSize / NX_EMMC_BLOCKSIZE);
		totalSectors -= currPartIdx * (multipartSplitSize / NX_EMMC_BLOCKSIZE);
	}

	while(totalSectors > 0)
	{
		if (numSplitParts != 0 && bytesWritten >= multipartSplitSize)
		{
			f_close(&fp);
			memset(&fp, 0, sizeof(fp));
			currPartIdx++;

			// Verify part
			if (dump_emmc_verify(storage, lba_curr, outFilename, NUM_SECTORS_PER_ITER, part))
			{
				EPRINTF("\nPress any key and try again.\n");

				free(buf);
				return 0;
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
				// Create partial dump index file
				if (f_open(&partialIdxFp, partialIdxFilename, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
				{
					f_write(&partialIdxFp, &currPartIdx, 4, NULL);
					f_close(&partialIdxFp);
				}
				else
				{
					EPRINTF("\nError creating partial.idx file.\n");

					free(buf);
					return 0;
				}

				// More parts to dump that do not currently fit the sd card free space or fatal error
				if (currPartIdx >= maxSplitParts)
				{
					gfx_puts(&gfx_con, "\n\n1. Press any key and Power off Switch from the main menu.\n\
						2. Move the files from SD card to free space.\n\
						   Don\'t move the partial.idx file!\n\
						3. Unplug and re-plug USB while pressing Vol+.\n\
						4. Run hekate - ipl again and press Dump RAW eMMC or eMMC USER to continue\n");

					free(buf);
					return 1;
				}
			}

			// Create next part
			if (f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
			{
				EPRINTFARGS("Error creating file %s.\n", outFilename);

				free(buf);
				return 0;
			}
			bytesWritten = 0;
		}

		retryCount = 0;
		u32 num = MIN(totalSectors, NUM_SECTORS_PER_ITER);
		while(!sdmmc_storage_read(storage, lba_curr, num, buf))
		{
			EPRINTFARGS("Error reading %d blocks @ LBA %08X from eMMC (try %d), retrying...",
				num, lba_curr, ++retryCount);

			sleep(150000);
			if (retryCount >= 3)
			{
				EPRINTFARGS("\nFailed to read %d blocks @ LBA %08X from eMMC. Aborting..\n",
				num, lba_curr);
				EPRINTF("\nPress any key and try again.\n");

				free(buf);
				f_close(&fp);
				return 0;
			}
		}
		res = f_write(&fp, buf, NX_EMMC_BLOCKSIZE * num, NULL);
		if (res)
		{
			EPRINTFARGS("\nFatal error (%d) when writing to SD Card", res);
			EPRINTF("\nPress any key and try again.\n");

			free(buf);
			f_close(&fp);
			return 0;
		}
		u32 pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		if (pct != prevPct)
		{
			tui_pbar(&gfx_con, 0, gfx_con.y, pct, 0xFFCCCCCC, 0xFF555555);
			prevPct = pct;
		}

		lba_curr += num;
		totalSectors -= num;
		bytesWritten += num * NX_EMMC_BLOCKSIZE;

		// Force a flush after a lot of data if not splitting
		if (numSplitParts == 0 && bytesWritten >= multipartSplitSize)
		{
			f_sync(&fp);
			bytesWritten = 0;
		}
	}
	tui_pbar(&gfx_con, 0, gfx_con.y, 100, 0xFFCCCCCC, 0xFF555555);

	// Dumping process ended successfully
	free(buf);
	f_close(&fp);

	// Verify last part or single file dump
	if (dump_emmc_verify(storage, lba_curr, outFilename, NUM_SECTORS_PER_ITER, part))
	{
		EPRINTF("\nPress any key and try again.\n");

		free(buf);
		return 0;
	}
	else
		tui_pbar(&gfx_con, 0, gfx_con.y, 100, 0xFF00FF96, 0xFF005515);

	// Remove partial dump index file if no fatal errors occurred.
	if(isSmallSdCard)
	{
		f_unlink(partialIdxFilename);
		gfx_printf(&gfx_con, "%k%K\n\nYou can now join the files and get the complete raw eMMC dump.", 0xFFCCCCCC, 0xFF1B1B1B);
	}
	gfx_puts(&gfx_con, "\n\n");

	return 1;
}

typedef enum
{
	DUMP_BOOT = 1,
	DUMP_SYSTEM = 2,
	DUMP_USER = 4,
	DUMP_RAW = 8
} dumpType_t;

static void dump_emmc_selected(dumpType_t dumpType)
{
	int res = 0;
	u32 timer = 0;
	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (!sd_mount())
		goto out;

	gfx_puts(&gfx_con, "Checking for available free space...\n\n");
	// Get SD Card free space for partial dumping
	f_getfree("", &sd_fs.free_clst, NULL);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if(!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}

	int i = 0;
	timer = get_tmr();
	if (dumpType & DUMP_BOOT)
	{
		static const u32 BOOT_PART_SIZE = 0x400000;

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		bootPart.lba_end = (BOOT_PART_SIZE/NX_EMMC_BLOCKSIZE)-1;
		for (i = 0; i < 2; i++)
		{
			memcpy(bootPart.name, "BOOT", 4);
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			gfx_printf(&gfx_con, "%k%02d: %s (%08X-%08X)%k\n", 0xFFFFDD00, i,
				bootPart.name, bootPart.lba_start, bootPart.lba_end, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&storage, i+1);
			res = dump_emmc_part(bootPart.name, &storage, &bootPart);
		}
	}

	if ((dumpType & DUMP_SYSTEM) || (dumpType & DUMP_USER) || (dumpType & DUMP_RAW))
	{
		sdmmc_storage_set_mmc_partition(&storage, 0);

		if ((dumpType & DUMP_SYSTEM) || (dumpType & DUMP_USER))
		{
			LIST_INIT(gpt);
			nx_emmc_gpt_parse(&gpt, &storage);
			LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
			{
				if ((dumpType & DUMP_USER) == 0 && !strcmp(part->name, "USER"))
					continue;
				if ((dumpType & DUMP_SYSTEM) == 0 && strcmp(part->name, "USER"))
					continue;

				gfx_printf(&gfx_con, "%k%02d: %s (%08X-%08X)%k\n", 0xFFFFDD00, i++,
					part->name, part->lba_start, part->lba_end, 0xFFCCCCCC);

				res = dump_emmc_part(part->name, &storage, part);
			}
		}

		if (dumpType & DUMP_RAW)
		{
			static const u32 RAW_AREA_NUM_SECTORS = 0x3A3E000;

			emmc_part_t rawPart;
			memset(&rawPart, 0, sizeof(rawPart));
			rawPart.lba_start = 0;
			rawPart.lba_end = RAW_AREA_NUM_SECTORS-1;
			strcpy(rawPart.name, "rawnand.bin");
			{
				gfx_printf(&gfx_con, "%k%02d: %s (%08X-%08X)%k\n", 0xFFFFDD00, i++,
					rawPart.name, rawPart.lba_start, rawPart.lba_end, 0xFFCCCCCC);

				res = dump_emmc_part(rawPart.name, &storage, &rawPart);
			}
		}
	}

	gfx_putc(&gfx_con, '\n');
	gfx_printf(&gfx_con, "%kTime taken: %d seconds.%k\n", 0xFF00FF96, (get_tmr() - timer) / 1000000, 0xFFCCCCCC);
	sdmmc_storage_end(&storage);
	if (res)
		gfx_puts(&gfx_con, "\nFinished and verified!\nPress any key.\n");

out:;
	btn_wait();
}

void dump_emmc_system() { dump_emmc_selected(DUMP_SYSTEM); }
void dump_emmc_user() { dump_emmc_selected(DUMP_USER); }
void dump_emmc_boot() { dump_emmc_selected(DUMP_BOOT); }
void dump_emmc_rawnand() { dump_emmc_selected(DUMP_RAW); }

void dump_package1()
{
	u8 * pkg1 = (u8 *)malloc(0x40000);
	u8 * warmboot = (u8 *)malloc(0x40000);
	u8 * secmon = (u8 *)malloc(0x40000);

	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (!sd_mount())
		goto out;

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if(!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}
	sdmmc_storage_set_mmc_partition(&storage, 1);

	//Read package1.
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1);
	const pk11_hdr_t *hdr = (pk11_hdr_t *)(pkg1 + pkg1_id->pkg11_off + 0x20);
	if (!pkg1_id)
	{
		EPRINTFARGS("Could not identify package1 version to read TSEC firmware (= '%s').", (char *)pkg1 + 0x10);
		goto out;
	}


	// Read keyblob
	u8 * keyblob = (u8 *)malloc(NX_EMMC_BLOCKSIZE);
	sdmmc_storage_read(&storage, 0x180000 / NX_EMMC_BLOCKSIZE + pkg1_id->kb, 1, keyblob);

	// decrypt
	keygen(keyblob, pkg1_id->kb, (u8 *)pkg1 + pkg1_id->tsec_off);
	pkg1_decrypt(pkg1_id, pkg1);

	pkg1_unpack(warmboot, secmon, pkg1_id, pkg1);

	// display info
	gfx_printf(&gfx_con, "%kSecure monitor addr: %k%08X\n", 0xFF46EAC7, 0xFFCCCCCC, pkg1_id->secmon_base);
	gfx_printf(&gfx_con, "%kSecure monitor size: %k%08X\n", 0xFF46EAC7, 0xFFCCCCCC, hdr->sm_size);
	gfx_printf(&gfx_con, "%kSecure monitor ofst: %k%08X\n", 0xFF46EAC7, 0xFFCCCCCC, hdr->sm_off);

	// dump package1
	if (sd_save_to_file(pkg1, 0x40000, "pkg_decr.bin")) {
		EPRINTF("\nFailed to create pkg_decr.bin");
		goto out;
	}
	gfx_puts(&gfx_con, "\npackage1 dumped to pkg_decr.bin\n");

	// dump sm
	if (sd_save_to_file(secmon, 0x40000, "sm.bin")) {
		EPRINTF("\nFailed to create sm.bin");
		goto out;
	}
	gfx_puts(&gfx_con, "Secure Monitor dumped to sm.bin\n");

	// dump warmboot
	if (sd_save_to_file(warmboot, 0x40000, "warmboot.bin")) {
		EPRINTF("\nFailed to create warmboot.bin");
		goto out;
	}
	gfx_puts(&gfx_con, "Warmboot dumped to warmboot.bin\n");


	sdmmc_storage_end(&storage);
	gfx_puts(&gfx_con, "\nDone. Press any key.\n");

out:;
	free(pkg1);
	free(secmon);
	free(warmboot);

	btn_wait();
}

void launch_firmware()
{
	ini_sec_t *cfg_sec = NULL;
	LIST_INIT(ini_sections);

	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	if (sd_mount())
	{
		if (ini_parse(&ini_sections, "hekate_ipl.ini"))
		{
			//Build configuration menu.
			ment_t *ments = (ment_t *)malloc(sizeof(ment_t) * 16);
			ments[0].type = MENT_BACK;
			ments[0].caption = "Back";
			u32 i = 1;
			LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
			{
				if (!strcmp(ini_sec->name, "config"))
					continue;
				ments[i].type = MENT_CHOICE;
				ments[i].caption = ini_sec->name;
				ments[i].data = ini_sec;
				i++;
			}
			if (i > 1)
			{
				memset(&ments[i], 0, sizeof(ment_t));
				menu_t menu = {
					ments, "Launch configurations", 0, 0
				};
				cfg_sec = (ini_sec_t *)tui_do_menu(&gfx_con, &menu);
				if (!cfg_sec)
					return;
			}
			else
				EPRINTF("No launch configurations found.");
			free(ments);
		}
		else
			EPRINTF("Could not find or open 'hekate_ipl.ini'.\nMake sure it exists in SD Card!.");
	}

	if (!cfg_sec)
	{
		sleep(3000000);
		gfx_printf(&gfx_con, "Using default launch configuration...\n");
	}

	if (!hos_launch(cfg_sec))
		EPRINTF("Failed to launch firmware.");

	//TODO: free ini.

out:;
	btn_wait();
}

void toggle_autorcm(){
	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if(!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
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
		tempbuf[0x10] ^= 0x77; //xor by arbitrary number to corrupt
		sdmmc_storage_write(&storage, sect, 1, tempbuf);
	}
	
	free(tempbuf);
	sdmmc_storage_end(&storage);
	
	gfx_printf(&gfx_con, "%kAutoRCM mode toggled!%k\n", 0xFF00EE2C, 0xFFCCCCCC);

out:;
	btn_wait();
}

void about()
{
	static const char octopus[] =
	"hekate (C) 2018 naehrwert, st4rk\n\n"
	"Thanks to: %kderrek, nedwill, plutoo, shuffle2, smea, thexyz, yellows8%k\n\n"
	"Greetings to: fincs, hexkyz, SciresM, Shiny Quagsire, WinterMute\n\n"
	"Open source and free packages used:\n"
	" - FatFs R0.13a, (Copyright (C) 2017, ChaN)\n"
	" - bcl-1.2.0,    (Copyright (C) 2003-2006, Marcus Geelnard)\n\n"
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

	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_setpos(&gfx_con, 0, 0);

	gfx_printf(&gfx_con, octopus, 0xFFFFCC00, 0xFFCCCCCC,
		0xFFFFCC00, 0xFFCCFF00, 0xFFFFCC00, 0xFFCCCCCC);

	btn_wait();
}

ment_t ment_cinfo[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- SoC Info ----", 0xFFE6B90A),
	MDEF_HANDLER("Print fuse info", print_fuseinfo),
	MDEF_HANDLER("Print kfuse info", print_kfuseinfo),
	MDEF_HANDLER("Print TSEC keys", print_tsec_key),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- Storage Info --", 0xFFE6B90A),
	MDEF_HANDLER("Print eMMC info", print_mmc_info),
	MDEF_HANDLER("Print SD Card info", print_sdcard_info),
	MDEF_END()
};
menu_t menu_cinfo = {
	ment_cinfo,
	"Console info", 0, 0
};

ment_t ment_autorcm[] = {
	MDEF_CAPTION("WARNING: This corrupts your BOOT0 partition!", 0xFF00FFE6),
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
	"AutoRCM", 0, 0
};

ment_t ment_tools[] = {
	MDEF_BACK(),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Full --------", 0xFFE6B90A),
	MDEF_HANDLER("Dump RAW eMMC", dump_emmc_rawnand),
	MDEF_HANDLER("Dump eMMC BOOT", dump_emmc_boot),
	MDEF_CHGLINE(),
	MDEF_CAPTION("-- GP Partitions --", 0xFFE6B90A),
	MDEF_HANDLER("Dump eMMC SYS", dump_emmc_system),
	MDEF_HANDLER("Dump eMMC USER", dump_emmc_user),
	MDEF_CHGLINE(),
	MDEF_CAPTION("------ Misc -------", 0xFFE6B90A),
	MDEF_HANDLER("Dump package1", dump_package1),
	MDEF_CHGLINE(),
	MDEF_CAPTION("---- Dangerous ----", 0xFF0000FF),
	MDEF_MENU("AutoRCM", &menu_autorcm),
	MDEF_END()
};

menu_t menu_tools = {
	ment_tools,
	"Tools", 0, 0
};

ment_t ment_top[] = {
	MDEF_HANDLER("Launch firmware", launch_firmware),
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
	"hekate - ipl", 0, 0
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
	gfx_clear(&gfx_ctxt, 0xFF1B1B1B);
	gfx_con_init(&gfx_con, &gfx_ctxt);

	while (1)
		tui_do_menu(&gfx_con, &menu_top);

	while (1)
		;
}
