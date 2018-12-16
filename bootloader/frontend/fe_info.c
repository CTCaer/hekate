/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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

extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;
extern sdmmc_storage_t sd_storage;
extern FATFS sd_fs;

extern bool sd_mount();
extern void sd_unmount();
extern int  sd_save_to_file(void *buf, u32 size, const char *filename);
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

//TODO: Create more macros (info, header, debug, etc) with different colors and utilize them for consistency.
#define EPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, 0xFFCCCCCC)
#define EPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, args, 0xFFCCCCCC)
#define WPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFFDD00, 0xFFCCCCCC)
#define WPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFFDD00, args, 0xFFCCCCCC)

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

	gfx_printf(&gfx_con, "\nSKU:         %X - ", FUSE(FUSE_SKU_INFO));
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
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY0)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY1)),
		byte_swap_32(FUSE(FUSE_PRIVATE_KEY2)), byte_swap_32(FUSE(FUSE_PRIVATE_KEY3)));

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
		u32 speed = 0;

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

	u32 retries = 0;
	u32 key_ver_max = 3;

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
		EPRINTFARGS("Unknown package1 version for reading\nTSEC firmware (= '%s').",
			(char *)pkg1 + 0x10);
		goto out_wait;
	}

	u8 keys[0x10 * 3];

	tsec_ctxt.size = 0xF00;
	tsec_ctxt.fw = (u8 *)pkg1 + pkg1_id->tsec_off;
	tsec_ctxt.pkg1 = pkg1;
	tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;
	tsec_ctxt.secmon_base = pkg1_id->secmon_base;

	if (pkg1_id->kb >= KB_FIRMWARE_VERSION_620)
	{
		tsec_ctxt.size = 0x2900;
		u8 *tsec_paged = (u8 *)page_alloc(3);
		memcpy(tsec_paged, (void *)tsec_ctxt.fw, tsec_ctxt.size);
		tsec_ctxt.fw = tsec_paged;

		key_ver_max = 1;
	}

	for (u32 i = 1; i <= key_ver_max; i++)
	{
		tsec_ctxt.key_ver = i;
		int res = 0;

		while (tsec_query(keys + ((i - 1) * 0x10), pkg1_id->kb, &tsec_ctxt) < 0)
		{
			if (pkg1_id->kb <= KB_FIRMWARE_VERSION_600)
				memset(keys + ((i - 1) * 0x10), 0x00, 0x10);
			else
				memset(keys, 0x00, 0x30);

			retries++;

			if (retries > 3)
			{
				res = -1;
				break;
			}
		}

		if (pkg1_id->kb <= KB_FIRMWARE_VERSION_600)
		{
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
		else
		{
			gfx_printf(&gfx_con, "%kTSEC key:  %k", 0xFF00DDFF, 0xFFCCCCCC);

			if (res >= 0)
			{
				for (u32 j = 0; j < 0x10; j++)
					gfx_printf(&gfx_con, "%02X", keys[j]);
				gfx_putc(&gfx_con, '\n');

				gfx_printf(&gfx_con, "%kTSEC root: %k", 0xFF00DDFF, 0xFFCCCCCC);
				for (u32 j = 0; j < 0x10; j++)
					gfx_printf(&gfx_con, "%02X", keys[0x10 + j]);
			}
			else
				EPRINTFARGS("ERROR %X", res);
			gfx_putc(&gfx_con, '\n');
		}
		
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

void _ipatch_process(u32 offset, u32 value)
{
	gfx_printf(&gfx_con, "%8x %8x", BOOTROM_BASE + offset, value);
	u8 lo = value & 0xff;
	switch (value >> 8)
	{
	case 0x20:
		gfx_printf(&gfx_con, "    MOVS R0, #0x%02X", lo);
		break;
	case 0xDF:
		gfx_printf(&gfx_con, "    SVC #0x%02X", lo);
		break;
	}
	gfx_puts(&gfx_con, "\n");
}

void bootrom_ipatches_info()
{
	gfx_clear_partial_grey(&gfx_ctxt, 0x1B, 0, 1256);
	gfx_con_setpos(&gfx_con, 0, 0);

	static const u32 BOOTROM_SIZE = 0x18000;
	
	u32 res = fuse_read_ipatch(_ipatch_process);
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
			
			u32 ipatch_backup[14];
			memcpy(ipatch_backup, (void *)IPATCH_BASE, sizeof(ipatch_backup));
			memset((void*)IPATCH_BASE, 0, sizeof(ipatch_backup));
			
			emmcsn_path_impl(path, "/dumps", "bootrom_unpatched.bin", NULL);
			if (!sd_save_to_file((u8 *)BOOTROM_BASE, BOOTROM_SIZE, path))
				gfx_puts(&gfx_con, "\nbootrom_unpatched.bin saved!\n");
			
			memcpy((void*)IPATCH_BASE, ipatch_backup, sizeof(ipatch_backup));
			
			sd_unmount();
		}

		btn_wait();
	}
}
