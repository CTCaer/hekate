/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 Rajko Stojadinovic
 * Copyright (c) 2018-2021 CTCaer
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
#include "../hos/sept.h"
#include "../config.h"
#include <utils/ini.h>
#include <libs/fatfs/diskio.h>
#include <libs/fatfs/ff.h>
#include <mem/heap.h>
#include <sec/se.h>
#include <storage/mbr_gpt.h>
#include "../storage/nx_emmc.h"
#include "../storage/nx_emmc_bis.h"
#include <storage/nx_sd.h>
#include <storage/sdmmc.h>
#include <utils/btn.h>
#include <utils/sprintf.h>
#include <utils/util.h>

#define NUM_SECTORS_PER_ITER 8192 // 4MB Cache.
#define OUT_FILENAME_SZ 128

extern hekate_config h_cfg;
extern volatile boot_cfg_t *b_cfg;

void load_emummc_cfg(emummc_cfg_t *emu_info)
{
	memset(emu_info, 0, sizeof(emummc_cfg_t));

	// Parse emuMMC configuration.
	LIST_INIT(ini_sections);
	if (ini_parse(&ini_sections, "emuMMC/emummc.ini", false))
	{
		LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
		{
			if (!strcmp(ini_sec->name, "emummc"))
			{
				LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
				{
					if (!strcmp("enabled", kv->key))
						emu_info->enabled = atoi(kv->val);
					else if (!strcmp("sector", kv->key))
						emu_info->sector = strtol(kv->val, NULL, 16);
					else if (!strcmp("id", kv->key))
						emu_info->id = strtol(kv->val, NULL, 16);
					else if (!strcmp("path", kv->key))
						emu_info->path = kv->val;
					else if (!strcmp("nintendo_path", kv->key))
						emu_info->nintendo_path = kv->val;
				}
			}
		}
	}
}

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

void update_emummc_base_folder(char *outFilename, u32 sdPathLen, u32 currPartIdx)
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
		update_emummc_base_folder(outFilename, sdPathLen, 0);
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

			update_emummc_base_folder(outFilename, sdPathLen, currPartIdx);

			// Create next part.
			s_printf(gui->txt_buf, "%s#", outFilename + strlen(gui->base_path));
			lv_label_cut_text(gui->label_info, strlen(lv_label_get_text(gui->label_info)) - 3, 3);
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

			bytesWritten = 0;

			totalSize = (u64)((u64)totalSectors << 9);
			clmt = f_expand_cltbl(&fp, 0x400000, MIN(totalSize, multipartSplitSize));
		}

		// Check for cancellation combo.
		if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
		{
			s_printf(gui->txt_buf, "\n#FFDD00 The emuMMC was cancelled!#\n");
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
				"\n#FFDD00 Error reading %d blocks @ LBA %08X,#\n"
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
				s_printf(gui->txt_buf, "#FFDD00 Retrying...#");
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

	char *txt_buf = (char *)malloc(0x4000);
	gui->base_path = (char *)malloc(OUT_FILENAME_SZ);
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

	if (!sdmmc_storage_init_mmc(&emmc_storage, &emmc_sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400))
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

	for (int j = 0; j < 100; j++)
	{
		update_emummc_base_folder(sdPath, base_len, j);
		if(f_stat(sdPath, NULL) == FR_NO_FILE)
			break;
	}

	f_mkdir(sdPath);
	strcat(sdPath, "/eMMC");
	f_mkdir(sdPath);
	strcat(sdPath, "/");
	strcpy(gui->base_path, sdPath);

	timer = get_tmr_s();
	const u32 BOOT_PART_SIZE = emmc_storage.ext_csd.boot_mult << 17;

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

		sdmmc_storage_set_mmc_partition(&emmc_storage, i + 1);

		strcat(sdPath, bootPart.name);
		res = _dump_emummc_file_part(gui, sdPath, &emmc_storage, &bootPart);

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
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);

	// Get GP partition size dynamically.
	const u32 RAW_AREA_NUM_SECTORS = emmc_storage.sec_cnt;

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

		res = _dump_emummc_file_part(gui, sdPath, &emmc_storage, &rawPart);

		if (!res)
			s_printf(txt_buf, "#FFDD00 Failed!#\n");
		else
			s_printf(txt_buf, "Done!\n");

		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);
	}

out_failed:
	timer = get_tmr_s() - timer;
	sdmmc_storage_end(&emmc_storage);

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
	sd_unmount();
}

static int _dump_emummc_raw_part(emmc_tool_gui_t *gui, int active_part, int part_idx, u32 sd_part_off, sdmmc_storage_t *storage, emmc_part_t *part, u32 resized_count)
{
	u32 num = 0;
	u32 pct = 0;
	u32 prevPct = 200;
	int retryCount = 0;
	u32 sd_sector_off = sd_part_off + (0x2000 * active_part);
	u32 lba_curr = part->lba_start;
	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;

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

	lv_obj_set_opa_scale(gui->bar, LV_OPA_COVER);
	lv_obj_set_opa_scale(gui->label_pct, LV_OPA_COVER);

	u32 user_offset = 0;

	if (resized_count)
	{
		// Get USER partition info.
		LIST_INIT(gpt);
		nx_emmc_gpt_parse(&gpt, storage);
		emmc_part_t *user_part = nx_emmc_part_find(&gpt, "USER");
		if (!user_part)
		{
			s_printf(gui->txt_buf, "\n#FFDD00 USER partition not found!#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 0;
		}

		user_offset = user_part->lba_start;
		part->lba_end = user_offset - 1;
		nx_emmc_gpt_free(&gpt);
	}

	u32 totalSectors = part->lba_end - part->lba_start + 1;
	while (totalSectors > 0)
	{
		// Check for cancellation combo.
		if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
		{
			s_printf(gui->txt_buf, "\n#FFDD00 The emuMMC was cancelled!#\n");
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

	// Set partition type to emuMMC (0xE0).
	if (active_part == 2)
	{
		mbr_t mbr;
		sdmmc_storage_read(&sd_storage, 0, 1, &mbr);
		mbr.partitions[part_idx].type = 0xE0;
		sdmmc_storage_write(&sd_storage, 0, 1, &mbr);
	}

	if (resized_count)
	{
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, "Done!\n");

		// Calculate USER size and set it for FatFS.
		u32 user_sectors = resized_count - user_offset - 33;
		disk_set_info(DRIVE_EMU, SET_SECTOR_COUNT, &user_sectors);

		// Initialize BIS for emuMMC. BIS keys should be already in place.
		emmc_part_t user_part = {0};
		user_part.lba_start = user_offset;
		user_part.lba_end = user_offset + user_sectors - 1;
		strcpy(user_part.name, "USER");
		nx_emmc_bis_init(&user_part, true, sd_sector_off);

		s_printf(gui->txt_buf, "Formatting USER... \n");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		// Format USER partition.
		u8 *buf = malloc(0x400000);
		int mkfs_error = f_mkfs("emu:", FM_FAT32 | FM_SFD | FM_PRF2, 16384, buf, 0x400000);
		free(buf);

		// Mount sd card back.
		sd_mount();

		if (mkfs_error)
		{
			s_printf(gui->txt_buf, "#FF0000 Failed (%d)!#\nPlease try again...\n", mkfs_error);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);

			return 0;
		}
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, "Done!\n");

		// Flush BIS cache, deinit, clear BIS keys slots and reinstate SBK.
		nx_emmc_bis_end();
		hos_bis_keys_clear();

		s_printf(gui->txt_buf, "Writing new GPT... ");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		// Read MBR, GPT and backup GPT.
		mbr_t mbr;
		gpt_t gpt_main;
		gpt_header_t gpt_hdr_backup;
		sdmmc_storage_read(storage, 0, 1, &mbr);
		sdmmc_storage_read(storage, 1, sizeof(gpt_t) >> 9, &gpt_main);
		sdmmc_storage_read(storage, gpt_main.header.alt_lba, 1, &gpt_hdr_backup);

		// Find USER partition.
		u32 gpt_entry_idx = 0;
		for (gpt_entry_idx = 0; gpt_entry_idx < gpt_main.header.num_part_ents; gpt_entry_idx++)
			if (!memcmp(gpt_main.entries[gpt_entry_idx].name, (char[]) { 'U', 0, 'S', 0, 'E', 0, 'R', 0 }, 8))
				break;

		if (gpt_entry_idx >= gpt_main.header.num_part_ents)
		{
			s_printf(gui->txt_buf, "\n#FF0000 No USER partition...#\nPlease try again...\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);

			return 0;
		}

		// Set new emuMMC size and USER size.
		mbr.partitions[0].size_sct = resized_count;
		gpt_main.entries[gpt_entry_idx].lba_end = user_offset + user_sectors - 1;

		// Update Main GPT.
		gpt_main.header.alt_lba = resized_count - 1;
		gpt_main.header.last_use_lba = resized_count - 34;
		gpt_main.header.part_ents_crc32 = crc32_calc(0, (const u8 *)gpt_main.entries, sizeof(gpt_entry_t) * gpt_main.header.num_part_ents);
		gpt_main.header.crc32 = 0; // Set to 0 for calculation.
		gpt_main.header.crc32 = crc32_calc(0, (const u8 *)&gpt_main.header, gpt_main.header.size);

		// Update Backup GPT.
		gpt_hdr_backup.my_lba = resized_count - 1;
		gpt_hdr_backup.part_ent_lba = resized_count - 33;
		gpt_hdr_backup.part_ents_crc32 = gpt_main.header.part_ents_crc32;
		gpt_hdr_backup.crc32 = 0; // Set to 0 for calculation.
		gpt_hdr_backup.crc32 = crc32_calc(0, (const u8 *)&gpt_hdr_backup, gpt_hdr_backup.size);

		// Write main GPT.
		sdmmc_storage_write(&sd_storage, sd_sector_off + gpt_main.header.my_lba, sizeof(gpt_t) >> 9, &gpt_main);

		// Write backup GPT partition table.
		sdmmc_storage_write(&sd_storage, sd_sector_off + gpt_hdr_backup.part_ent_lba, ((sizeof(gpt_entry_t) * 128) >> 9), gpt_main.entries);

		// Write backup GPT header.
		sdmmc_storage_write(&sd_storage, sd_sector_off + gpt_hdr_backup.my_lba, 1, &gpt_hdr_backup);

		// Write MBR.
		sdmmc_storage_write(&sd_storage, sd_sector_off, 1, &mbr);
	}

	return 1;
}

u32 kb = 0;
u8 *tsec_fw = NULL;
bool sept_error = false;

static lv_res_t _emummc_raw_check_sept_action(lv_obj_t *btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	mbox_action(btns, txt);

	if (btn_idx == 1 && !sept_error)
	{
		// Set boot cfg.
		b_cfg->autoboot = 0;
		b_cfg->autoboot_list = 0;
		b_cfg->extra_cfg = EXTRA_CFG_NYX_SEPT;
		b_cfg->sept = NYX_SEPT_EMUF;

		sd_mount();
		reboot_to_sept(tsec_fw, kb);
	}

	return LV_RES_INV;
}

static int _emummc_raw_check_sept(emmc_tool_gui_t *gui, u32 resized_count)
{
	if (!resized_count)
		return 1;

	bool sept_needed = false;
	sept_error = false;
	tsec_fw = NULL;

	char *txt_buf = (char *)malloc(0x4000);
	txt_buf[0] = 0;

	// Read package1.
	static const u32 BOOTLOADER_SIZE          = 0x40000;
	static const u32 BOOTLOADER_MAIN_OFFSET   = 0x100000;
	static const u32 BOOTLOADER_BACKUP_OFFSET = 0x140000;
	static const u32 HOS_KEYBLOBS_OFFSET      = 0x180000;

	u32 bootloader_offset = BOOTLOADER_MAIN_OFFSET;
	u32 pk1_offset = h_cfg.t210b01 ? sizeof(bl_hdr_t210b01_t) : 0; // Skip T210B01 OEM header.
	u8 *pkg1 = (u8 *)malloc(BOOTLOADER_SIZE);

	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_BOOT0);

try_load:
	sdmmc_storage_read(&emmc_storage, bootloader_offset / NX_EMMC_BLOCKSIZE, BOOTLOADER_SIZE / NX_EMMC_BLOCKSIZE, pkg1);

	char *build_date = malloc(32);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1 + pk1_offset, build_date);

	s_printf(txt_buf + strlen(txt_buf), "#00DDFF Found pkg1 ('%s')#\n", build_date);
	free(build_date);

	if (!pkg1_id)
	{
		strcat(txt_buf, "#FFDD00 Unknown pkg1 version!#\n");
		// Try backup bootloader.
		if (bootloader_offset != BOOTLOADER_BACKUP_OFFSET)
		{
			strcat(txt_buf, "Trying backup bootloader...\n");
			bootloader_offset = BOOTLOADER_BACKUP_OFFSET;
			goto try_load;
		}

		sept_error = true;
		goto out;
	}

	kb = pkg1_id->kb;

	// Skip if Mariko.
	if (h_cfg.t210b01)
		goto bis_derivation;

	tsec_ctxt_t tsec_ctxt;
	tsec_ctxt.fw = (u8 *)pkg1 + pkg1_id->tsec_off;
	tsec_ctxt.pkg1 = pkg1;
	tsec_ctxt.pkg11_off = pkg1_id->pkg11_off;
	tsec_ctxt.secmon_base = pkg1_id->secmon_base;

	// Get keys.
	hos_eks_get();
	if (kb >= KB_FIRMWARE_VERSION_700 && !h_cfg.sept_run)
	{
		u32 key_idx = 0;
		if (kb >= KB_FIRMWARE_VERSION_810)
			key_idx = 1;

		if (h_cfg.eks && h_cfg.eks->enabled[key_idx] >= kb)
			h_cfg.sept_run = true;
		else
		{
			// Check that BCT is proper so sept can run.
			u8 *bct_bldr = (u8 *)calloc(1, 512);
			sdmmc_storage_read(&emmc_storage, 0x2200 / NX_EMMC_BLOCKSIZE, 1, bct_bldr);
			u32 bootloader_entrypoint = *(u32 *)&bct_bldr[0x144];
			free(bct_bldr);
			if (bootloader_entrypoint > SEPT_PRI_ENTRY)
			{
				strcpy(txt_buf, "#FFDD00 Failed to run sept because main BCT is improper!#\n"
					"#FFDD00 Run sept with proper BCT at least once to cache keys.#\n");
				sept_error = true;
				goto out;
			}

			// Set TSEC fw.
			tsec_fw = (u8 *)tsec_ctxt.fw;

			sept_needed = true;
			goto out;
		}
	}

bis_derivation:;
	// Read the correct keyblob.
	u8 *keyblob = (u8 *)calloc(NX_EMMC_BLOCKSIZE, 1);
	sdmmc_storage_read(&emmc_storage, HOS_KEYBLOBS_OFFSET / NX_EMMC_BLOCKSIZE + kb, 1, keyblob);

	// Generate BIS keys
	hos_bis_keygen(keyblob, kb, &tsec_ctxt);

	free(keyblob);

	u8 *cal0_buf = malloc(0x10000);

	// Read and decrypt CAL0 for validation of working BIS keys.
	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);
	LIST_INIT(gpt);
	nx_emmc_gpt_parse(&gpt, &emmc_storage);
	emmc_part_t *cal0_part = nx_emmc_part_find(&gpt, "PRODINFO"); // check if null
	nx_emmc_bis_init(cal0_part, false, 0);
	nx_emmc_bis_read(0, 0x40, cal0_buf);
	nx_emmc_bis_end();
	nx_emmc_gpt_free(&gpt);

	nx_emmc_cal0_t *cal0 = (nx_emmc_cal0_t *)cal0_buf;

	// If successful, save BIS keys.
	if (memcmp(&cal0->magic, "CAL0", 4))
	{
		hos_bis_keys_clear();
		hos_eks_bis_clear();

		strcpy(txt_buf, "#FFDD00 BIS keys validation failed!#\n");
		sept_error = true;
	}
	else
		hos_eks_bis_save();
	free(cal0_buf);

out:
	// Check if sept is not needed.
	if (!sept_needed)
		free(pkg1);

	if (sept_needed || sept_error)
	{
		lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
		lv_obj_set_style(dark_bg, &mbox_darken);
		lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

		static const char * mbox_btn_map[] = { "\211", "\222Launch", "\222Close", "\211", "" };
		static const char * mbox_btn_map2[] = { "\211", "\222Close", "\211", "" };
		lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
		lv_mbox_set_recolor_text(mbox, true);
		lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);

		lv_mbox_set_text(mbox, "#C7EA46 BIS Keys Generation#");

		lv_obj_t * lb_desc = lv_label_create(mbox, NULL);
		lv_label_set_long_mode(lb_desc, LV_LABEL_LONG_BREAK);
		lv_label_set_recolor(lb_desc, true);
		lv_label_set_style(lb_desc, &monospace_text);
		lv_obj_set_width(lb_desc, LV_HOR_RES / 9 * 4);

		if (sept_error)
		{
			lv_label_set_text(lb_desc, txt_buf);
			lv_mbox_add_btns(mbox, mbox_btn_map2, _emummc_raw_check_sept_action);
			free(pkg1);
		}
		else
		{
			lv_label_set_text(lb_desc, "Sept needs to launch in order to generate keys\nneeded for emuMMC resizing.\n"
				"After that enter this menu again.");
			lv_mbox_add_btns(mbox, mbox_btn_map, _emummc_raw_check_sept_action);
		}

		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_set_top(mbox, true);

		free(txt_buf);

		return 0;
	}

	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);

	return 1;
}

void dump_emummc_raw(emmc_tool_gui_t *gui, int part_idx, u32 sector_start, u32 resized_count)
{
	int res = 0;
	u32 timer = 0;

	char *txt_buf = (char *)malloc(0x4000);
	gui->base_path = (char *)malloc(OUT_FILENAME_SZ);
	gui->txt_buf = txt_buf;

	s_printf(txt_buf, "");
	lv_label_set_text(gui->label_log, txt_buf);

	manual_system_maintenance(true);

	if (!sd_mount())
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init SD!#");
		goto out;
	}

	if (!sdmmc_storage_init_mmc(&emmc_storage, &emmc_sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400))
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init eMMC!#");
		goto out;
	}

	if (!_emummc_raw_check_sept(gui, resized_count))
	{
		s_printf(gui->txt_buf, "#FFDD00 For formatting USER partition,#\n#FFDD00 BIS keys are needed!#\n");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		sdmmc_storage_end(&emmc_storage);
		goto out;
	}

	int i = 0;
	char sdPath[OUT_FILENAME_SZ];
	// Create Restore folders, if they do not exist.
	f_mkdir("emuMMC");
	s_printf(sdPath, "emuMMC/RAW%d", part_idx);
	f_mkdir(sdPath);
	strcat(sdPath, "/");
	strcpy(gui->base_path, sdPath);

	timer = get_tmr_s();
	const u32 BOOT_PART_SIZE = emmc_storage.ext_csd.boot_mult << 17;

	emmc_part_t bootPart;
	memset(&bootPart, 0, sizeof(bootPart));
	bootPart.lba_start = 0;
	bootPart.lba_end = (BOOT_PART_SIZE / NX_EMMC_BLOCKSIZE) - 1;

	// Clear partition start.
	memset((u8 *)MIXD_BUF_ALIGNED, 0, 0x1000000);
	sdmmc_storage_write(&sd_storage, sector_start - 0x8000, 0x8000, (u8 *)MIXD_BUF_ALIGNED);

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

		sdmmc_storage_set_mmc_partition(&emmc_storage, i + 1);

		strcat(sdPath, bootPart.name);
		res = _dump_emummc_raw_part(gui, i, part_idx, sector_start, &emmc_storage, &bootPart, 0);

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

	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_GPP);

	// Get GP partition size dynamically.
	const u32 RAW_AREA_NUM_SECTORS = emmc_storage.sec_cnt;

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

		res = _dump_emummc_raw_part(gui, 2, part_idx, sector_start, &emmc_storage, &rawPart, resized_count);

		if (!res)
			s_printf(txt_buf, "#FFDD00 Failed!#\n");
		else
			s_printf(txt_buf, "Done!\n");

		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
		manual_system_maintenance(true);
	}

out_failed:
	timer = get_tmr_s() - timer;
	sdmmc_storage_end(&emmc_storage);

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
	sd_unmount();
}
