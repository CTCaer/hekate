/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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

#include "fe_info.h"
#include "../gfx/gfx.h"
#include "../hos/hos.h"
#include "../hos/pkg1.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../power/bq24193.h"
#include "../power/max17050.h"
#include "../sec/tsec.h"
#include "../soc/fuse.h"
#include "../soc/i2c.h"
#include "../soc/kfuse.h"
#include "../soc/smmu.h"
#include "../soc/t210.h"
#include "../storage/mmc.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/util.h"

extern sdmmc_storage_t sd_storage;
extern FATFS sd_fs;

extern bool sd_mount();
extern void sd_unmount();
extern int  sd_save_to_file(void *buf, u32 size, const char *filename);
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

#pragma GCC push_options
#pragma GCC optimize ("Os")

void print_fuseinfo()
{
	gfx_clear_partial(BG_COL, 0, 1256);
	gfx_con_setpos(0, 0);

	u32 burntFuses = 0;
	for (u32 i = 0; i < 32; i++)
	{
		if ((fuse_read_odm(7) >> i) & 1)
			burntFuses++;
	}

	gfx_printf("\nSKU:         %X - ", FUSE(FUSE_SKU_INFO));
	switch (fuse_read_odm(4) & 3)
	{
	case 0:
		gfx_printf("Retail\n");
		break;
	case 3:
		gfx_printf("Dev\n");
		break;
	}
	gfx_printf("Sdram ID:    %d\n", (fuse_read_odm(4) >> 3) & 0x1F);
	gfx_printf("Burnt fuses: %d / 64\n", burntFuses);
	gfx_printf("Secure key:  %08X%08X%08X%08X\n\n\n",
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY0)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY1)),
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY2)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY3)));

	gfx_printf("%k(Unlocked) fuse cache:\n\n%k", INFOCOL, MAINTXTCOL);
	gfx_hexdump(0x7000F900, (u8 *)0x7000F900, 0x300);

	gfx_puts("\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "fuse_cached.bin", NULL);
			if (!sd_save_to_file((u8 *)0x7000F900, 0x300, path))
				gfx_puts("\nfuse_cached.bin saved!\n");

			u32 words[192];
			fuse_read_array(words);
			emmcsn_path_impl(path, "/dumps", "fuse_array_raw.bin", NULL);
			if (!sd_save_to_file((u8 *)words, sizeof(words), path))
				gfx_puts("\nfuse_array_raw.bin saved!\n");

			sd_unmount();
		}

		btn_wait();
	}
}

void print_kfuseinfo()
{
	gfx_clear_partial(BG_COL, 0, 1256);
	gfx_con_setpos(0, 0);

	gfx_printf("%kKFuse contents:\n\n%k", INFOCOL, MAINTXTCOL);
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
			sd_unmount();
		}

		btn_wait();
	}
}

void print_mmc_info()
{
	gfx_clear_partial(BG_COL, 0, 1256);
	gfx_con_setpos(0, 0);

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
		u32 speed = 0;

		gfx_printf("%kCID:%k\n", INFOCOL, MAINTXTCOL);
		switch (storage.csd.mmca_vsn)
		{
		case 0: /* MMC v1.0 - v1.2 */
		case 1: /* MMC v1.4 */
			gfx_printf(
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
			gfx_printf(
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
			gfx_printf("%kExtended CSD V1.%d:%k\n",
				INFOCOL, storage.ext_csd.ext_struct, MAINTXTCOL);
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

			gfx_printf(
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
			gfx_printf("%s", card_type_support);
			gfx_con.fntsz = 16;
			gfx_printf("\n\n", card_type_support);

			u32 boot_size = storage.ext_csd.boot_mult << 17;
			u32 rpmb_size = storage.ext_csd.rpmb_mult << 17;
			gfx_printf("%keMMC Partitions:%k\n", INFOCOL, MAINTXTCOL);
			gfx_printf(" 1: %kBOOT0      %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", ATTNCOL, MAINTXTCOL,
				boot_size / 1024, boot_size / 512);
			gfx_put_small_sep();
			gfx_printf(" 2: %kBOOT1      %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", ATTNCOL, MAINTXTCOL,
				boot_size / 1024, boot_size / 512);
			gfx_put_small_sep();
			gfx_printf(" 3: %kRPMB       %k\n    Size: %5d KiB (LBA Sectors: 0x%07X)\n", ATTNCOL, MAINTXTCOL,
				rpmb_size / 1024, rpmb_size / 512);
			gfx_put_small_sep();
			gfx_printf(" 0: %kGPP (USER) %k\n    Size: %5d MiB (LBA Sectors: 0x%07X)\n\n", ATTNCOL, MAINTXTCOL,
				storage.sec_cnt >> SECTORS_TO_MIB_COEFF, storage.sec_cnt);
			gfx_put_small_sep();
			gfx_printf("%kGPP (eMMC USER) partition table:%k\n", INFOCOL, MAINTXTCOL);

			sdmmc_storage_set_mmc_partition(&storage, 0);
			LIST_INIT(gpt);
			nx_emmc_gpt_parse(&gpt, &storage);
			int gpp_idx = 0;
			LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
			{
				gfx_printf(" %02d: %k%s%k\n     Size: % 5d MiB (LBA Sectors 0x%07X)\n     LBA Range: %08X-%08X\n",
					gpp_idx++, ATTNCOL, part->name, MAINTXTCOL, (part->lba_end - part->lba_start + 1) >> SECTORS_TO_MIB_COEFF,
					part->lba_end - part->lba_start + 1, part->lba_start, part->lba_end);
				gfx_put_small_sep();
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

	gfx_clear_partial(BG_COL, 0, 1256);
	gfx_con_setpos(0, 0);

	if (sd_mount())
	{
		u32 capacity;

		gfx_printf("%kCard IDentification:%k\n", INFOCOL, MAINTXTCOL);
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

		gfx_printf("%kCard-Specific Data V%d.0:%k\n", INFOCOL, sd_storage.csd.structure + 1, MAINTXTCOL);
		capacity = sd_storage.csd.capacity >> (20 - sd_storage.csd.read_blkbits);
		gfx_printf(
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

		gfx_puts("Acquiring FAT volume info...\n\n");
		f_getfree("", &sd_fs.free_clst, NULL);
		gfx_printf("%kFound %s volume:%k\n Free:    %d MiB\n Cluster: %d KiB\n",
				INFOCOL, sd_fs.fs_type == FS_EXFAT ? "exFAT" : "FAT32", MAINTXTCOL,
				sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF, (sd_fs.csize > 1) ? (sd_fs.csize >> 1) : 512);
		sd_unmount();
	}

	btn_wait();
}

void print_tsec_key()
{
	gfx_clear_partial(BG_COL, 0, 1256);
	gfx_con_setpos(0, 0);

	u32 retries = 0;

	tsec_ctxt_t tsec_ctxt;
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
		EPRINTF("Unknown pkg1 version for reading\nTSEC firmware.");
		goto out_wait;
	}

	u8 keys[0x10 * 2];
	memset(keys, 0x00, 0x20);

	tsec_ctxt.fw = (u8 *)pkg1 + pkg1_id->tsec_off;
	tsec_ctxt.pkg1 = pkg1;
	tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;
	tsec_ctxt.secmon_base = pkg1_id->secmon_base;

	if (pkg1_id->kb <= KB_FIRMWARE_VERSION_600)
		tsec_ctxt.size = 0xF00;
	else if (pkg1_id->kb == KB_FIRMWARE_VERSION_620)
		tsec_ctxt.size = 0x2900;
	else if (pkg1_id->kb == KB_FIRMWARE_VERSION_700)
	{
		tsec_ctxt.size = 0x3000;
		// Exit after TSEC key generation.
		*((vu16 *)((u32)tsec_ctxt.fw + 0x2DB5)) = 0x02F8;
	}
	else
		tsec_ctxt.size = 0x3300;

	if (pkg1_id->kb == KB_FIRMWARE_VERSION_620)
	{
		u8 *tsec_paged = (u8 *)page_alloc(3);
		memcpy(tsec_paged, (void *)tsec_ctxt.fw, tsec_ctxt.size);
		tsec_ctxt.fw = tsec_paged;
	}

	int res = 0;

	while (tsec_query(keys, pkg1_id->kb, &tsec_ctxt) < 0)
	{
		memset(keys, 0x00, 0x20);

		retries++;

		if (retries > 3)
		{
			res = -1;
			break;
		}
	}

	gfx_printf("%kTSEC key:  %k", INFOCOL, MAINTXTCOL);

	if (res >= 0)
	{
		for (u32 j = 0; j < 0x10; j++)
			gfx_printf("%02X", keys[j]);

		if (pkg1_id->kb == KB_FIRMWARE_VERSION_620)
		{
			gfx_printf("\n%kTSEC root: %k", INFOCOL, MAINTXTCOL);
			for (u32 j = 0; j < 0x10; j++)
				gfx_printf("%02X", keys[0x10 + j]);
		}
	}
	else
		EPRINTFARGS("ERROR %X\n", res);

	gfx_puts("\n\nPress POWER to dump them to SD Card.\nPress VOL to go to the menu.\n");

	u32 btn = btn_wait();
	if (btn & BTN_POWER)
	{
		if (sd_mount())
		{
			char path[64];
			emmcsn_path_impl(path, "/dumps", "tsec_keys.bin", NULL);
			if (!sd_save_to_file(keys, 0x10 * 2, path))
				gfx_puts("\nDone!\n");
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

void print_fuel_gauge_info()
{
	int value = 0;

	gfx_printf("%kFuel Gauge IC Info:\n%k", INFOCOL, MAINTXTCOL);

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

	gfx_printf("%k\n\nBattery Charger IC Info:\n%k", INFOCOL, MAINTXTCOL);

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
	gfx_clear_partial(BG_COL, 0, 1256);
	gfx_con_setpos(0, 0);

	print_fuel_gauge_info();

	print_battery_charger_info();

	u8 *buf = (u8 *)malloc(0x100 * 2);

	gfx_printf("%k\n\nBattery Fuel Gauge Registers:\n%k", INFOCOL, MAINTXTCOL);

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
			sd_unmount();
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
	gfx_clear_partial(BG_COL, 0, 1256);
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

			sd_unmount();
		}

		btn_wait();
	}
}

#pragma GCC pop_options
