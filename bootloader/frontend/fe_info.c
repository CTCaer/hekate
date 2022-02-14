/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2022 CTCaer
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

#include <bdk.h>

#include "fe_info.h"
#include "../config.h"
#include "../hos/hos.h"
#include "../hos/pkg1.h"
#include <libs/fatfs/ff.h>

extern hekate_config h_cfg;
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

#pragma GCC push_options
#pragma GCC optimize ("Os")

void print_fuseinfo()
{
	u32 fuse_size = h_cfg.t210b01 ? 0x368 : 0x300;
	u32 fuse_address = h_cfg.t210b01 ? 0x7000F898 : 0x7000F900;
	u32 fuse_array_size = (h_cfg.t210b01 ? 256 : 192) * sizeof(u32);

	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	gfx_printf("\nSKU:         %X - ", FUSE(FUSE_SKU_INFO));
	switch (fuse_read_hw_state())
	{
	case FUSE_NX_HW_STATE_PROD:
		gfx_printf("Retail\n");
		break;
	case FUSE_NX_HW_STATE_DEV:
		gfx_printf("Dev\n");
		break;
	}
	gfx_printf("Sdram ID:    %d\n", fuse_read_dramid(true));
	gfx_printf("Burnt fuses: %d / 64\n", bit_count(fuse_read_odm(7)));
	gfx_printf("Secure key:  %08X%08X%08X%08X\n\n\n",
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY0)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY1)),
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY2)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY3)));

	gfx_printf("%kFuse cache:\n\n%k", 0xFF00DDFF, 0xFFCCCCCC);
	gfx_hexdump(fuse_address, (u8 *)fuse_address, fuse_size);

	gfx_puts("\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "fuse_cached.bin", NULL);
			if (!sd_save_to_file((u8 *)fuse_address, fuse_size, path))
				gfx_puts("\nfuse_cached.bin saved!\n");

			u32 words[256];
			fuse_read_array(words);
			emmcsn_path_impl(path, "/dumps", "fuse_array_raw.bin", NULL);
			if (!sd_save_to_file((u8 *)words, fuse_array_size, path))
				gfx_puts("\nfuse_array_raw.bin saved!\n");

			sd_end();
		}

		btn_wait();
	}
}

void print_kfuseinfo()
{
	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	gfx_printf("%kKFuse contents:\n\n%k", 0xFF00DDFF, 0xFFCCCCCC);
	u32 buf[KFUSE_NUM_WORDS];
	if (!kfuse_read(buf))
		EPRINTF("CRC fail.");
	else
		gfx_hexdump(0, (u8 *)buf, KFUSE_NUM_WORDS * 4);

	gfx_puts("\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "kfuses.bin", NULL);
			if (!sd_save_to_file((u8 *)buf, KFUSE_NUM_WORDS * 4, path))
				gfx_puts("\nDone!\n");
			sd_end();
		}

		btn_wait();
	}
}

void print_mmc_info()
{
	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	static const u32 SECTORS_TO_MIB_COEFF = 11;

	if (!emmc_initialize(false))
	{
		EPRINTF("Failed to init eMMC.");
		goto out;
	}
	else
	{
		u16 card_type;
		u32 speed = 0;

		gfx_printf("%kCID:%k\n", 0xFF00DDFF, 0xFFCCCCCC);
		switch (emmc_storage.csd.mmca_vsn)
		{
		case 2: /* MMC v2.0 - v2.2 */
		case 3: /* MMC v3.1 - v3.3 */
		case 4: /* MMC v4 */
			gfx_printf(
				" Vendor ID:  %X\n"
				" OEM ID:     %02X\n"
				" Model:      %c%c%c%c%c%c\n"
				" Prd Rev:    %X\n"
				" S/N:        %04X\n"
				" Month/Year: %02d/%04d\n\n",
				emmc_storage.cid.manfid, emmc_storage.cid.oemid,
				emmc_storage.cid.prod_name[0], emmc_storage.cid.prod_name[1], emmc_storage.cid.prod_name[2],
				emmc_storage.cid.prod_name[3], emmc_storage.cid.prod_name[4],	emmc_storage.cid.prod_name[5],
				emmc_storage.cid.prv, emmc_storage.cid.serial, emmc_storage.cid.month, emmc_storage.cid.year);
			break;
		default:
			break;
		}

		if (emmc_storage.csd.structure == 0)
			EPRINTF("Unknown CSD structure.");
		else
		{
			gfx_printf("%kExtended CSD V1.%d:%k\n",
				0xFF00DDFF, emmc_storage.ext_csd.ext_struct, 0xFFCCCCCC);
			card_type = emmc_storage.ext_csd.card_type;
			char card_type_support[96];
			card_type_support[0] = 0;
			if (card_type & EXT_CSD_CARD_TYPE_HS_26)
			{
				strcat(card_type_support, "HS26");
				speed = (26 << 16) | 26;
			}
			if (card_type & EXT_CSD_CARD_TYPE_HS_52)
			{
				strcat(card_type_support, ", HS52");
				speed = (52 << 16) | 52;
			}
			if (card_type & EXT_CSD_CARD_TYPE_DDR_1_8V)
			{
				strcat(card_type_support, ", DDR52_1.8V");
				speed = (52 << 16) | 104;
			}
			if (card_type & EXT_CSD_CARD_TYPE_HS200_1_8V)
			{
				strcat(card_type_support, ", HS200_1.8V");
				speed = (200 << 16) | 200;
			}
			if (card_type & EXT_CSD_CARD_TYPE_HS400_1_8V)
			{
				strcat(card_type_support, ", HS400_1.8V");
				speed = (200 << 16) | 400;
			}

			gfx_printf(
				" Spec Version:  %02X\n"
				" Extended Rev:  1.%d\n"
				" Dev Version:   %d\n"
				" Cmd Classes:   %02X\n"
				" Capacity:      %s\n"
				" Max Rate:      %d MB/s (%d MHz)\n"
				" Current Rate:  %d MB/s\n"
				" Type Support:  ",
				emmc_storage.csd.mmca_vsn, emmc_storage.ext_csd.rev, emmc_storage.ext_csd.dev_version, emmc_storage.csd.cmdclass,
				emmc_storage.csd.capacity == (4096 * 512) ? "High" : "Low", speed & 0xFFFF, (speed >> 16) & 0xFFFF,
				emmc_storage.csd.busspeed);
			gfx_con.fntsz = 8;
			gfx_printf("%s", card_type_support);
			gfx_con.fntsz = 16;
			gfx_printf("\n\n", card_type_support);

			u32 boot_size = emmc_storage.ext_csd.boot_mult << 17;
			u32 rpmb_size = emmc_storage.ext_csd.rpmb_mult << 17;
			gfx_printf("%keMMC Partitions:%k\n", 0xFF00DDFF, 0xFFCCCCCC);
			gfx_printf(" 1: %kBOOT0      %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF96FF00, 0xFFCCCCCC,
				boot_size / 1024, boot_size / 512);
			gfx_put_small_sep();
			gfx_printf(" 2: %kBOOT1      %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF96FF00, 0xFFCCCCCC,
				boot_size / 1024, boot_size / 512);
			gfx_put_small_sep();
			gfx_printf(" 3: %kRPMB       %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", 0xFF96FF00, 0xFFCCCCCC,
				rpmb_size / 1024, rpmb_size / 512);
			gfx_put_small_sep();
			gfx_printf(" 0: %kGPP (USER) %k\n    Size: %5d MiB (LBA Sectors: 0x%07X)\n\n", 0xFF96FF00, 0xFFCCCCCC,
				emmc_storage.sec_cnt >> SECTORS_TO_MIB_COEFF, emmc_storage.sec_cnt);
			gfx_put_small_sep();
			gfx_printf("%kGPP (eMMC USER) partition table:%k\n", 0xFF00DDFF, 0xFFCCCCCC);

			sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);
			LIST_INIT(gpt);
			emmc_gpt_parse(&gpt);
			int gpp_idx = 0;
			LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
			{
				gfx_printf(" %02d: %k%s%k\n     Size: % 5d MiB (LBA Sectors 0x%07X)\n     LBA Range: %08X-%08X\n",
					gpp_idx++, 0xFFAEFD14, part->name, 0xFFCCCCCC, (part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
					part->lba_end - part->lba_start + 1, part->lba_start, part->lba_end);
				gfx_put_small_sep();
			}
			emmc_gpt_free(&gpt);
		}
	}

out:
	sdmmc_storage_end(&emmc_storage);

	btn_wait();
}

void print_sdcard_info()
{
	static const u32 SECTORS_TO_MIB_COEFF = 11;

	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	if (sd_initialize(false))
	{
		gfx_printf("%kCard IDentification:%k\n", 0xFF00DDFF, 0xFFCCCCCC);
		gfx_printf(
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

		u16 *sd_errors = sd_get_error_count();
		gfx_printf("%kCard-Specific Data V%d.0:%k\n", 0xFF00DDFF, sd_storage.csd.structure + 1, 0xFFCCCCCC);
		gfx_printf(
			" Cmd Classes:    %02X\n"
			" Capacity:       %d MiB\n"
			" Bus Width:      %d\n"
			" Current Rate:   %d MB/s (%d MHz)\n"
			" Speed Class:    %d\n"
			" UHS Grade:      U%d\n"
			" Video Class:    V%d\n"
			" App perf class: A%d\n"
			" Write Protect:  %d\n"
			" SDMMC Errors:   %d %d %d\n\n",
			sd_storage.csd.cmdclass, sd_storage.sec_cnt >> 11,
			sd_storage.ssr.bus_width, sd_storage.csd.busspeed, sd_storage.csd.busspeed * 2,
			sd_storage.ssr.speed_class, sd_storage.ssr.uhs_grade, sd_storage.ssr.video_class,
			sd_storage.ssr.app_class, sd_storage.csd.write_protect,
			sd_errors[0], sd_errors[1], sd_errors[2]); // SD_ERROR_INIT_FAIL, SD_ERROR_RW_FAIL, SD_ERROR_RW_RETRY.

		int res = f_mount(&sd_fs, "", 1);
		if (!res)
		{
			gfx_puts("Acquiring FAT volume info...\n\n");
			f_getfree("", &sd_fs.free_clst, NULL);
			gfx_printf("%kFound %s volume:%k\n Free:    %d MiB\n Cluster: %d KiB\n",
					0xFF00DDFF, sd_fs.fs_type == FS_EXFAT ? "exFAT" : "FAT32", 0xFFCCCCCC,
					sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF, (sd_fs.csize > 1) ? (sd_fs.csize >> 1) : 512);
			f_mount(NULL, "", 1);
		}
		else
		{
			EPRINTFARGS("Failed to mount SD card (FatFS Error %d).\n"
				"Make sure that a FAT partition exists..", res);
		}

		sdmmc_storage_end(&sd_storage);
	}
	else
	{
		EPRINTF("Failed to init SD card.");
		if (!sdmmc_get_sd_inserted())
			EPRINTF("Make sure that it is inserted.");
		else
			EPRINTF("SD Card Reader is not properly seated!");
		sd_end();
	}

	btn_wait();
}

void print_fuel_gauge_info()
{
	int value = 0;

	gfx_printf("%kFuel Gauge Info:\n%k", 0xFF00DDFF, 0xFFCCCCCC);

	max17050_get_property(MAX17050_RepSOC, &value);
	gfx_printf("Capacity now:           %3d%\n", value >> 8);

	max17050_get_property(MAX17050_RepCap, &value);
	gfx_printf("Capacity now:           %4d mAh\n", value);

	max17050_get_property(MAX17050_FullCAP, &value);
	gfx_printf("Capacity full:          %4d mAh\n", value);

	max17050_get_property(MAX17050_DesignCap, &value);
	gfx_printf("Capacity (design):      %4d mAh\n", value);

	max17050_get_property(MAX17050_Current, &value);
	if (value >= 0)
		gfx_printf("Current now:            %d mA\n", value / 1000);
	else
		gfx_printf("Current now:            -%d mA\n", ~value / 1000);

	max17050_get_property(MAX17050_AvgCurrent, &value);
	if (value >= 0)
		gfx_printf("Current average:        %d mA\n", value / 1000);
	else
		gfx_printf("Current average:        -%d mA\n", ~value / 1000);

	max17050_get_property(MAX17050_VCELL, &value);
	gfx_printf("Voltage now:            %4d mV\n", value);

	max17050_get_property(MAX17050_OCVInternal, &value);
	gfx_printf("Voltage open-circuit:   %4d mV\n", value);

	max17050_get_property(MAX17050_MinVolt, &value);
	gfx_printf("Min voltage reached:    %4d mV\n", value);

	max17050_get_property(MAX17050_MaxVolt, &value);
	gfx_printf("Max voltage reached:    %4d mV\n", value);

	max17050_get_property(MAX17050_V_empty, &value);
	gfx_printf("Empty voltage (design): %4d mV\n", value);

	max17050_get_property(MAX17050_TEMP, &value);
	if (value >= 0)
		gfx_printf("Battery temperature:    %d.%d oC\n", value / 10, value % 10);
	else
		gfx_printf("Battery temperature:    -%d.%d oC\n", ~value / 10, (~value) % 10);
}

void print_battery_charger_info()
{
	int value = 0;

	gfx_printf("%k\n\nBattery Charger Info:\n%k", 0xFF00DDFF, 0xFFCCCCCC);

	bq24193_get_property(BQ24193_InputVoltageLimit, &value);
	gfx_printf("Input voltage limit:       %4d mV\n", value);

	bq24193_get_property(BQ24193_InputCurrentLimit, &value);
	gfx_printf("Input current limit:       %4d mA\n", value);

	bq24193_get_property(BQ24193_SystemMinimumVoltage, &value);
	gfx_printf("Min voltage limit:         %4d mV\n", value);

	bq24193_get_property(BQ24193_FastChargeCurrentLimit, &value);
	gfx_printf("Fast charge current limit: %4d mA\n", value);

	bq24193_get_property(BQ24193_ChargeVoltageLimit, &value);
	gfx_printf("Charge voltage limit:      %4d mV\n", value);

	bq24193_get_property(BQ24193_ChargeStatus, &value);
	gfx_printf("Charge status:             ");
	switch (value)
	{
	case 0:
		gfx_printf("Not charging\n");
		break;
	case 1:
		gfx_printf("Pre-charging\n");
		break;
	case 2:
		gfx_printf("Fast charging\n");
		break;
	case 3:
		gfx_printf("Charge terminated\n");
		break;
	default:
		gfx_printf("Unknown (%d)\n", value);
		break;
	}
	bq24193_get_property(BQ24193_TempStatus, &value);
	gfx_printf("Temperature status:        ");
	switch (value)
	{
	case 0:
		gfx_printf("Normal\n");
		break;
	case 2:
		gfx_printf("Warm\n");
		break;
	case 3:
		gfx_printf("Cool\n");
		break;
	case 5:
		gfx_printf("Cold\n");
		break;
	case 6:
		gfx_printf("Hot\n");
		break;
	default:
		gfx_printf("Unknown (%d)\n", value);
		break;
	}
}

void print_battery_info()
{
	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	print_fuel_gauge_info();

	print_battery_charger_info();

	u8 *buf = (u8 *)malloc(0x100 * 2);

	gfx_printf("%k\n\nBattery Fuel Gauge Registers:\n%k", 0xFF00DDFF, 0xFFCCCCCC);

	for (int i = 0; i < 0x200; i += 2)
	{
		i2c_recv_buf_small(buf + i, 2, I2C_1, MAXIM17050_I2C_ADDR, i >> 1);
		usleep(2500);
	}

	gfx_hexdump(0, (u8 *)buf, 0x200);

	gfx_puts("\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

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
				gfx_puts("\nDone!\n");
			sd_end();
		}

		btn_wait();
	}
	free(buf);
}

void _ipatch_process(u32 offset, u32 value)
{
	gfx_printf("%8x %8x", BOOTROM_BASE + offset, value);
	u8 lo = value & 0xff;
	switch (value >> 8)
	{
	case 0x20:
		gfx_printf("    MOVS R0, #0x%02X", lo);
		break;
	case 0xDF:
		gfx_printf("    SVC #0x%02X", lo);
		break;
	}
	gfx_puts("\n");
}

void bootrom_ipatches_info()
{
	gfx_clear_partial_grey(0x1B, 0, 1256);
	gfx_con_setpos(0, 0);

	static const u32 BOOTROM_SIZE = 0x18000;

	u32 res = fuse_read_ipatch(_ipatch_process);
	if (res != 0)
		EPRINTFARGS("Failed to read ipatches. Error: %d", res);

	gfx_puts("\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

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
					gfx_puts("\nevp_thunks.bin saved!\n");
			}
			else
				EPRINTFARGS("Failed to read evp_thunks. Error: %d", res);

			emmcsn_path_impl(path, "/dumps", "bootrom_patched.bin", NULL);
			if (!sd_save_to_file((u8 *)BOOTROM_BASE, BOOTROM_SIZE, path))
				gfx_puts("\nbootrom_patched.bin saved!\n");

			u32 ipatch_backup[14];
			memcpy(ipatch_backup, (void *)IPATCH_BASE, sizeof(ipatch_backup));
			memset((void*)IPATCH_BASE, 0, sizeof(ipatch_backup));

			emmcsn_path_impl(path, "/dumps", "bootrom_unpatched.bin", NULL);
			if (!sd_save_to_file((u8 *)BOOTROM_BASE, BOOTROM_SIZE, path))
				gfx_puts("\nbootrom_unpatched.bin saved!\n");

			memcpy((void*)IPATCH_BASE, ipatch_backup, sizeof(ipatch_backup));

			sd_end();
		}

		btn_wait();
	}
}

#pragma GCC pop_options
