/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 Rajko Stojadinovic
 * Copyright (c) 2018-2020 CTCaer
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

//! fix the dram stuff and the pop ups

#include <string.h>
#include <stdlib.h>

#include "gui.h"
#include "fe_emummc_tools.h"
#include "../config/config.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../sec/se.h"
#include "../storage/mbr_gpt.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/sprintf.h"
#include "../utils/util.h"

#define NUM_SECTORS_PER_ITER 8192 // 4MB Cache.
#define OUT_FILENAME_SZ 128

#define MBR_1ST_PART_TYPE_OFF 0x1C2

extern sdmmc_t sd_sdmmc;
extern sdmmc_storage_t sd_storage;
extern FATFS sd_fs;
extern hekate_config h_cfg;

extern bool sd_mount();
extern void sd_unmount(bool deinit);

void save_emummc_cfg(u32 part_idx, u32 sector_start, const char *path)
{
	sd_mount();

	char lbuf[16];
	FIL fp;

	if (f_open(&fp, "emuMMC/emummc.ini", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return;

	// Add config entry.
	f_puts("[emummc]\nenabled=", &fp);
	if (part_idx && sector_start)
	{
		itoa(part_idx, lbuf, 10);
		f_puts(lbuf, &fp);
	}
	else if(path)
		f_puts("1", &fp);
	else
		f_puts("0", &fp);

	if (!sector_start)
		f_puts("\nsector=0x0", &fp);
	else
	{
		f_puts("\nsector=0x", &fp);
		itoa(sector_start, lbuf, 16);
		f_puts(lbuf, &fp);
	}
	if (path)
	{
		f_puts("\npath=", &fp);
		f_puts(path, &fp);
	}
	f_puts("\nid=0x0000", &fp);
	f_puts("\nnintendo_path=", &fp);
	if (path)
	{
		f_puts(path, &fp);
		f_puts("/Nintendo\n", &fp);
	}
	else
		f_puts("\n", &fp);

	f_close(&fp);
}

static void _update_emummc_base_folder(char *outFilename, u32 sdPathLen, u32 currPartIdx)
{
	if (currPartIdx < 10)
	{
		outFilename[sdPathLen] = '0';
		itoa(currPartIdx, &outFilename[sdPathLen + 1], 10);
	}
	else
		itoa(currPartIdx, &outFilename[sdPathLen], 10);
}

static int _dump_emummc_file_part(emmc_tool_gui_t *gui, char *sd_path, sdmmc_storage_t *storage, emmc_part_t *part)
{
	static const u32 FAT32_FILESIZE_LIMIT = 0xFFFFFFFF;
	static const u32 SECTORS_TO_MIB_COEFF = 11;

	u32 multipartSplitSize = 0xFE000000;
	u32 totalSectors = part->lba_end - part->lba_start + 1;
	u32 currPartIdx = 0;
	u32 numSplitParts = 0;
	int res = 0;
	char *outFilename = sd_path;
	u32 sdPathLen = strlen(sd_path);

	s_printf(gui->txt_buf, "#96FF00 SD Card free space:# %d MiB\n#96FF00 Total backup size:# %d MiB\n\n",
		sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF,
		totalSectors >> SECTORS_TO_MIB_COEFF);
	lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
	manual_system_maintenance(true);

	lv_bar_set_value(gui->bar, 0);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 0%");
	manual_system_maintenance(true);

	// Check if the USER partition or the RAW eMMC fits the sd card free space.
	if (totalSectors > (sd_fs.free_clst * sd_fs.csize))
	{
		s_printf(gui->txt_buf, "\n#FFDD00 Not enough free space for Partial Backup!#\n");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		return 0;
	}

	// Check if filesystem is FAT32 or the free space is smaller and backup in parts.
	if (totalSectors > (FAT32_FILESIZE_LIMIT / NX_EMMC_BLOCKSIZE))
	{
		u32 multipartSplitSectors = multipartSplitSize / NX_EMMC_BLOCKSIZE;
		numSplitParts = (totalSectors + multipartSplitSectors - 1) / multipartSplitSectors;

		// Continue from where we left, if Partial Backup in progress.
		_update_emummc_base_folder(outFilename, sdPathLen, 0);
	}

	FIL fp;
	s_printf(gui->txt_buf, "#96FF00 Filepath:#\n%s\n#96FF00 Filename:# #FF8000 %s#",
		gui->base_path, outFilename + strlen(gui->base_path));
	lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
	manual_system_maintenance(true);

	res = f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res)
	{
		s_printf(gui->txt_buf, "\n#FF0000 Error (%d) while creating#\n#FFDD00 %s#\n", res, outFilename);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		return 0;
	}

	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;

	u32 lba_curr = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;
	DWORD *clmt = NULL;

	u64 totalSize = (u64)((u64)totalSectors << 9);
	if (totalSize <= FAT32_FILESIZE_LIMIT)
		clmt = f_expand_cltbl(&fp, 0x400000, totalSize);
	else
		clmt = f_expand_cltbl(&fp, 0x400000, MIN(totalSize, multipartSplitSize));

	u32 num = 0;
	u32 pct = 0;

	lv_obj_set_opa_scale(gui->bar, LV_OPA_COVER);
	lv_obj_set_opa_scale(gui->label_pct, LV_OPA_COVER);
	while (totalSectors > 0)
	{
		if (numSplitParts != 0 && bytesWritten >= multipartSplitSize)
		{
			f_close(&fp);
			free(clmt);
			memset(&fp, 0, sizeof(fp));
			currPartIdx++;

			_update_emummc_base_folder(outFilename, sdPathLen, currPartIdx);

			// Create next part.
			s_printf(gui->txt_buf, "%s#", outFilename + strlen(gui->base_path));
			lv_label_cut_text(gui->label_info, strlen(lv_label_get_text(gui->label_info)) - 3, 3);
			lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			res = f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE);
			if (res)
			{
				s_printf(gui->txt_buf, "#FF0000 Error (%d) while creating#\n#FFDD00 %s#\n", res, outFilename);
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return 0;
			}

			bytesWritten = 0;

			totalSize = (u64)((u64)totalSectors << 9);
			clmt = f_expand_cltbl(&fp, 0x400000, MIN(totalSize, multipartSplitSize));
		}

		// Check for cancellation combo.
		if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
		{
			s_printf(gui->txt_buf, "#FFDD00 The emuMMC was cancelled!#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			f_close(&fp);
			free(clmt);
			f_unlink(outFilename);

			msleep(1000);

			return 0;
		}

		retryCount = 0;
		num = MIN(totalSectors, NUM_SECTORS_PER_ITER);
		while (!sdmmc_storage_read(storage, lba_curr, num, buf))
		{
			s_printf(gui->txt_buf,
				"#FFDD00 Error reading %d blocks @ LBA %08X,#\n"
				"#FFDD00 from eMMC (try %d). #",
				num, lba_curr, ++retryCount);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			msleep(150);
			if (retryCount >= 3)
			{
				s_printf(gui->txt_buf, "#FF0000 Aborting...#\nPlease try again...\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				f_close(&fp);
				free(clmt);
				f_unlink(outFilename);

				return 0;
			}
			else
			{
				s_printf(gui->txt_buf, "#FFDD00 Retrying...#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);
			}
		}

		manual_system_maintenance(false);

		res = f_write_fast(&fp, buf, NX_EMMC_BLOCKSIZE * num);

		manual_system_maintenance(false);

		if (res)
		{
			s_printf(gui->txt_buf, "\n#FF0000 Fatal error (%d) when writing to SD Card#\nPlease try again...\n", res);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			f_close(&fp);
			free(clmt);
			f_unlink(outFilename);

			return 0;
		}
		pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		if (pct != prevPct)
		{
			lv_bar_set_value(gui->bar, pct);
			s_printf(gui->txt_buf, " "SYMBOL_DOT" %d%%", pct);
			lv_label_set_text(gui->label_pct, gui->txt_buf);
			manual_system_maintenance(true);

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

		manual_system_maintenance(false);
	}
	lv_bar_set_value(gui->bar, 100);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 100%");
	manual_system_maintenance(true);

	// Backup operation ended successfully.
	f_close(&fp);
	free(clmt);

	return 1;
}

void dump_emummc_file(emmc_tool_gui_t *gui)
{
	int res = 0;
	int base_len = 0;
	u32 timer = 0;

	char *txt_buf = (char *)malloc(0x1000);
	gui->txt_buf = txt_buf;

	s_printf(txt_buf, "");
	lv_label_set_text(gui->label_log, txt_buf);

	manual_system_maintenance(true);

	if (!sd_mount())
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init SD!#");
		goto out;
	}

	lv_label_set_text(gui->label_info, "Checking for available free space...");
	manual_system_maintenance(true);

	// Get SD Card free space for Partial Backup.
	f_getfree("", &sd_fs.free_clst, NULL);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init eMMC!#");
		goto out;
	}

	int i = 0;
	char sdPath[OUT_FILENAME_SZ];
	// Create Restore folders, if they do not exist.
	f_mkdir("emuMMC");
	strcpy(sdPath, "emuMMC/SD");
	base_len = strlen(sdPath);
	gui->base_path = (char *)malloc(OUT_FILENAME_SZ);

	for (int j = 0; j < 100; j++)
	{
		_update_emummc_base_folder(sdPath, base_len, j);
		if(f_stat(sdPath, NULL) == FR_NO_FILE)
			break;
	}

	f_mkdir(sdPath);
	strcat(sdPath, "/eMMC");
	f_mkdir(sdPath);
	strcat(sdPath, "/");
	strcpy(gui->base_path, sdPath);

	timer = get_tmr_s();
	const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

	emmc_part_t bootPart;
	memset(&bootPart, 0, sizeof(bootPart));
	bootPart.lba_start = 0;
	bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;
	for (i = 0; i < 2; i++)
	{
		strcpy(bootPart.name, "BOOT");
		bootPart.name[4] = (u8)('0' + i);
		bootPart.name[5] = 0;

		s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n",
			i, bootPart.name, bootPart.lba_start, bootPart.lba_end);
		lv_label_set_text(gui->label_info, txt_buf);
		s_printf(txt_buf, "%02d: %s... ", i, bootPart.name);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);

		sdmmc_storage_set_mmc_partition(&storage, i + 1);

		strcat(sdPath, bootPart.name);
		res = _dump_emummc_file_part(gui, sdPath, &storage, &bootPart);

		if (!res)
		{
			s_printf(txt_buf, "#FFDD00 Failed!#\n");
			goto out_failed;
		}
		else
			s_printf(txt_buf, "Done!\n");

		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);

		strcpy(sdPath, gui->base_path);
	}

	// Get GP partition size dynamically.
	sdmmc_storage_set_mmc_partition(&storage, 0);

	// Get GP partition size dynamically.
	const u32 RAW_AREA_NUM_SECTORS = storage.sec_cnt;

	emmc_part_t rawPart;
	memset(&rawPart, 0, sizeof(rawPart));
	rawPart.lba_start = 0;
	rawPart.lba_end = RAW_AREA_NUM_SECTORS - 1;
	strcpy(rawPart.name, "GPP");
	{
		s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n",
			i, rawPart.name, rawPart.lba_start, rawPart.lba_end);
		lv_label_set_text(gui->label_info, txt_buf);
		s_printf(txt_buf, "%02d: %s... ", i, rawPart.name);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);

		res = _dump_emummc_file_part(gui, sdPath, &storage, &rawPart);

		if (!res)
			s_printf(txt_buf, "#FFDD00 Failed!#\n");
		else
			s_printf(txt_buf, "Done!\n");

		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);
	}

out_failed:
	timer = get_tmr_s() - timer;
	sdmmc_storage_end(&storage);

	if (res)
	{
		s_printf(txt_buf, "Time taken: %dm %ds.\nFinished!", timer / 60, timer % 60);
		gui->base_path[strlen(gui->base_path) - 5] = '\0';
		strcpy(sdPath, gui->base_path);
		strcat(sdPath, "file_based");
		FIL fp;
		f_open(&fp, sdPath, FA_CREATE_ALWAYS | FA_WRITE);
		f_close(&fp);

		gui->base_path[strlen(gui->base_path) - 1] = 0;
		save_emummc_cfg(0, 0, gui->base_path);
	}
	else
		s_printf(txt_buf, "Time taken: %dm %ds.", timer / 60, timer % 60);

	lv_label_set_text(gui->label_finish, txt_buf);

out:
	free(txt_buf);
	free(gui->base_path);
	sd_unmount(false);
}

static int _dump_emummc_raw_part(emmc_tool_gui_t *gui, int active_part, int part_idx, u32 sd_part_off, sdmmc_storage_t *storage, emmc_part_t *part)
{
	u32 totalSectors = part->lba_end - part->lba_start + 1;

	s_printf(gui->txt_buf, "\n\n\n");
	lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
	manual_system_maintenance(true);

	lv_bar_set_value(gui->bar, 0);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 0%");
	manual_system_maintenance(true);

	s_printf(gui->txt_buf, "#96FF00 Base folder:#\n%s\n#96FF00 Partition offset:# #FF8000 0x%08X#",
		gui->base_path, sd_part_off);
	lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
	manual_system_maintenance(true);

	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;
	u32 sd_sector_off = sd_part_off + (0x2000 * active_part);
	u32 lba_curr = part->lba_start;
	u32 prevPct = 200;
	int retryCount = 0;

	u32 num = 0;
	u32 pct = 0;

	lv_obj_set_opa_scale(gui->bar, LV_OPA_COVER);
	lv_obj_set_opa_scale(gui->label_pct, LV_OPA_COVER);

	while (totalSectors > 0)
	{
		// Check for cancellation combo.
		if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
		{
			s_printf(gui->txt_buf, "#FFDD00 The emuMMC was cancelled!#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			msleep(1000);

			return 0;
		}

		retryCount = 0;
		num = MIN(totalSectors, NUM_SECTORS_PER_ITER);

		// Read data from eMMC.
		while (!sdmmc_storage_read(storage, lba_curr, num, buf))
		{
			s_printf(gui->txt_buf,
				"\n#FFDD00 Error reading %d blocks @LBA %08X,#\n"
				"#FFDD00 from eMMC (try %d). #",
				num, lba_curr, ++retryCount);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			msleep(150);
			if (retryCount >= 3)
			{
				s_printf(gui->txt_buf, "#FF0000 Aborting...#\nPlease try again...\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return 0;
			}
			else
			{
				s_printf(gui->txt_buf, "#FFDD00 Retrying...#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);
			}
		}

		manual_system_maintenance(false);

		retryCount = 0;

		// Write data to SD card.
		while (!sdmmc_storage_write(&sd_storage, sd_sector_off + lba_curr, num, buf))
		{
			s_printf(gui->txt_buf,
				"\n#FFDD00 Error writing %d blocks @LBA %08X,#\n"
				"#FFDD00 to SD (try %d). #",
				num, lba_curr, ++retryCount);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			msleep(150);
			if (retryCount >= 3)
			{
				s_printf(gui->txt_buf, "#FF0000 Aborting...#\nPlease try again...\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return 0;
			}
			else
			{
				s_printf(gui->txt_buf, "#FFDD00 Retrying...#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);
			}
		}

		manual_system_maintenance(false);

		pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		if (pct != prevPct)
		{
			lv_bar_set_value(gui->bar, pct);
			s_printf(gui->txt_buf, " "SYMBOL_DOT" %d%%", pct);
			lv_label_set_text(gui->label_pct, gui->txt_buf);
			manual_system_maintenance(true);

			prevPct = pct;
		}

		lba_curr += num;
		totalSectors -= num;
	}
	lv_bar_set_value(gui->bar, 100);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 100%");
	manual_system_maintenance(true);

	// Hide the partition.
	if (active_part == 2)
	{
		mbr_t *mbr = (mbr_t *)malloc(sizeof(mbr_t));
		sdmmc_storage_read(&sd_storage, 0, 1, mbr);
		mbr->partitions[part_idx].type = 0xE0;
		sdmmc_storage_write(&sd_storage, 0, 1, mbr);
		free(mbr);
	}

	return 1;
}

void dump_emummc_raw(emmc_tool_gui_t *gui, int part_idx, u32 sector_start)
{
	int res = 0;
	u32 timer = 0;

	char *txt_buf = (char *)malloc(0x1000);
	gui->txt_buf = txt_buf;

	s_printf(txt_buf, "");
	lv_label_set_text(gui->label_log, txt_buf);

	manual_system_maintenance(true);

	if (!sd_mount())
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init SD!#");
		goto out;
	}

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init eMMC!#");
		goto out;
	}

	int i = 0;
	char sdPath[OUT_FILENAME_SZ];
	// Create Restore folders, if they do not exist.
	f_mkdir("emuMMC");
	s_printf(sdPath, "emuMMC/RAW%d", part_idx);
	gui->base_path = (char *)malloc(OUT_FILENAME_SZ);
	f_mkdir(sdPath);
	strcat(sdPath, "/");
	strcpy(gui->base_path, sdPath);

	timer = get_tmr_s();
	const u32 BOOT_PART_SIZE = storage.ext_csd.boot_mult << 17;

	emmc_part_t bootPart;
	memset(&bootPart, 0, sizeof(bootPart));
	bootPart.lba_start = 0;
	bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;
	for (i = 0; i < 2; i++)
	{
		strcpy(bootPart.name, "BOOT");
		bootPart.name[4] = (u8)('0' + i);
		bootPart.name[5] = 0;

		s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n",
			i, bootPart.name, bootPart.lba_start, bootPart.lba_end);
		lv_label_set_text(gui->label_info, txt_buf);
		s_printf(txt_buf, "%02d: %s... ", i, bootPart.name);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);

		sdmmc_storage_set_mmc_partition(&storage, i + 1);

		strcat(sdPath, bootPart.name);
		res = _dump_emummc_raw_part(gui, i, part_idx, sector_start, &storage, &bootPart);

		if (!res)
		{
			s_printf(txt_buf, "#FFDD00 Failed!#\n");
			goto out_failed;
		}
		else
			s_printf(txt_buf, "Done!\n");

		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);

		strcpy(sdPath, gui->base_path);
	}

	sdmmc_storage_set_mmc_partition(&storage, 0);

	// Get GP partition size dynamically.
	const u32 RAW_AREA_NUM_SECTORS = storage.sec_cnt;

	emmc_part_t rawPart;
	memset(&rawPart, 0, sizeof(rawPart));
	rawPart.lba_start = 0;
	rawPart.lba_end = RAW_AREA_NUM_SECTORS - 1;
	strcpy(rawPart.name, "GPP");
	{
		s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n",
			i, rawPart.name, rawPart.lba_start, rawPart.lba_end);
		lv_label_set_text(gui->label_info, txt_buf);
		s_printf(txt_buf, "%02d: %s... ", i, rawPart.name);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);

		res = _dump_emummc_raw_part(gui, 2, part_idx, sector_start, &storage, &rawPart);

		if (!res)
			s_printf(txt_buf, "#FFDD00 Failed!#\n");
		else
			s_printf(txt_buf, "Done!\n");

		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);
	}

out_failed:
	timer = get_tmr_s() - timer;
	sdmmc_storage_end(&storage);

	if (res)
	{
		s_printf(txt_buf, "Time taken: %dm %ds.\nFinished!", timer / 60, timer % 60);
		strcpy(sdPath, gui->base_path);
		strcat(sdPath, "raw_based");
		FIL fp;
		f_open(&fp, sdPath, FA_CREATE_ALWAYS | FA_WRITE);
		f_write(&fp, &sector_start, 4, NULL);
		f_close(&fp);

		gui->base_path[strlen(gui->base_path) - 1] = 0;
		save_emummc_cfg(part_idx, sector_start, gui->base_path);
	}
	else
		s_printf(txt_buf, "Time taken: %dm %ds.", timer / 60, timer % 60);

	lv_label_set_text(gui->label_finish, txt_buf);

out:
	free(txt_buf);
	free(gui->base_path);
	sd_unmount(false);
}
