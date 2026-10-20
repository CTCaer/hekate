/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 Rajko Stojadinovic
 * Copyright (c) 2018-2026 CTCaer
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

#include <bdk.h>

#include "gui.h"
#include "fe_emmc_tools.h"
#include "fe_emummc_tools.h"
#include "../config.h"
#include <libs/fatfs/ff.h>

#define VERIF_STATUS_OK    0
#define VERIF_STATUS_ERROR 1
#define VERIF_STATUS_ABORT 2

#define NUM_SECTORS_PER_ITER 8192 // 4MB Cache.
#define OUT_FILENAME_SZ 128
#define HASH_FILENAME_SZ (OUT_FILENAME_SZ + 11) // 11 == strlen(".sha256sums")
#define HASH_CHUNK_SIZE (NUM_SECTORS_PER_ITER * EMMC_BLOCKSIZE)

extern nyx_config n_cfg;

extern char *emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

static void _get_valid_partition(u32 *sector_start, u32 *sector_size, u32 *part_idx, bool backup)
{
	sd_mount();
	mbr_t *mbr = (mbr_t *)zalloc(sizeof(mbr_t));
	sdmmc_storage_read(&sd_storage, 0, 1, mbr);

	*part_idx = 0;
	int i = 0;
	u32 curr_part_size = 0;
	// Find first partition with emuMMC GPP.
	for (i = 1; i < 4; i++)
	{
		curr_part_size = mbr->partitions[i].size_sct;
		*sector_start = mbr->partitions[i].start_sct;
		u8 type = mbr->partitions[i].type;
		u32 sector_size_safe = backup ? 0x400000 : (*sector_size) + 0x8000; // 2GB min safe size for backup.
		if ((curr_part_size >= sector_size_safe) && (*sector_start) && type != 0x83 && (!backup || type == 0xE0))
		{
			if (backup)
			{
				u8 gpt_check[SD_BLOCKSIZE] = { 0 };
				sdmmc_storage_read(&sd_storage, (*sector_start) + 0xC001, 1, gpt_check);
				if (!memcmp(gpt_check, "EFI PART", 8))
				{
					*sector_size = curr_part_size;
					*sector_start = (*sector_start) + 0x8000;
					break;
				}
				sdmmc_storage_read(&sd_storage, (*sector_start) + 0x4001, 1, gpt_check);
				if (!memcmp(gpt_check, "EFI PART", 8))
				{
					*sector_size = curr_part_size;
					break;
				}
			}
			else
				break;
		}
	}
	free(mbr);

	if (i < 4)
		*part_idx = i;
	else
	{
		*sector_start = 0;
		*sector_size = 0;
		*part_idx = 0;
	}

	// Get emuMMC GPP size.
	if (backup && *part_idx && *sector_size)
	{
		gpt_t *gpt = (gpt_t *)zalloc(sizeof(gpt_t));
		sdmmc_storage_read(&sd_storage, (*sector_start) + 0x4001, 1, gpt);

		u32 new_size = gpt->header.alt_lba + 1;
		if (*sector_size > new_size)
			*sector_size = new_size;
		else
			*sector_size = 0;

		free(gpt);
	}
	else if (!backup && *part_idx)
		*sector_start = (*sector_start) + 0x8000;
}

static lv_obj_t *create_mbox_text(const char *text, bool button_ok)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, text);
	if (button_ok)
		lv_mbox_add_btns(mbox, mbox_btn_map, nyx_mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return dark_bg;
}

static void _update_filename(char *outFilename, u32 sdPathLen, u32 currPartIdx)
{
	if (currPartIdx < 10)
	{
		outFilename[sdPathLen] = '0';
		itoa(currPartIdx, &outFilename[sdPathLen + 1], 10);
	}
	else
		itoa(currPartIdx, &outFilename[sdPathLen], 10);
}

static int _emmc_sd_copy_verify(emmc_tool_gui_t *gui, sdmmc_storage_t *storage, u32 lba_curr, const char *outFilename, const emmc_part_t *part, bool save_hashes)
{
	FIL fp;
	FIL hashFp;
	bool saveHashes = save_hashes && n_cfg.verification == 3;
	u8 sparseShouldVerify = 4;
	u32 prevPct = 200;
	u32 sdFileSector = 0;
	int res = 0;
	static const char hexa[] = "0123456789abcdef";
	DWORD *clmt = NULL;
	char hashFilename[HASH_FILENAME_SZ];

	u8 hashEm[SE_SHA_256_SIZE];
	u8 hashSd[SE_SHA_256_SIZE];

	if (f_open(&fp, outFilename, FA_READ) == FR_OK)
	{
		if (saveHashes)
		{
			strcpy(hashFilename, outFilename);
			strcat(hashFilename, ".sha256sums");

			res = f_open(&hashFp, hashFilename, FA_CREATE_ALWAYS | FA_WRITE);
			if (res)
			{
				f_close(&fp);

				s_printf(gui->txt_buf,
						"\n#FF0000 Hash file could not be written (error %d)!#\n"
						"#FF0000 Aborting..#\n", res);
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return VERIF_STATUS_ERROR;
			}

			char chunkSizeAscii[10];
			itoa(NUM_SECTORS_PER_ITER * EMMC_BLOCKSIZE, chunkSizeAscii, 10);
			chunkSizeAscii[9] = '\0';

			f_puts("# chunksize: ", &hashFp);
			f_puts(chunkSizeAscii, &hashFp);
			f_puts("\n", &hashFp);
		}

		u32 totalSectorsVer = (u32)((u64)f_size(&fp) >> (u64)9);

		u8 *bufEm = (u8 *)EMMC_BUF_ALIGNED;
		u8 *bufSd = (u8 *)SDXC_BUF_ALIGNED;

		u32 pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(part->lba_end - part->lba_start);
		lv_bar_set_value(gui->bar, pct);
		lv_bar_set_style(gui->bar, LV_BAR_STYLE_BG, gui->bar_teal_bg);
		lv_bar_set_style(gui->bar, LV_BAR_STYLE_INDIC, gui->bar_teal_ind);
		s_printf(gui->txt_buf, " "SYMBOL_DOT" %d%%", pct);
		lv_label_set_text(gui->label_pct, gui->txt_buf);
		manual_system_maintenance(true);

		clmt = f_expand_cltbl(&fp, SZ_4M, 0);

		u32 num = 0;
		while (totalSectorsVer > 0)
		{
			num = MIN(totalSectorsVer, NUM_SECTORS_PER_ITER);

			// Check every time or every 4.
			// Every 4 protects from fake sd, sector corruption and frequent I/O corruption.
			// Full provides all that, plus protection from extremely rare I/O corruption.
			if ((n_cfg.verification >= 2) || !(sparseShouldVerify % 4))
			{
				if (sdmmc_storage_read(storage, lba_curr, num, bufEm))
				{
					s_printf(gui->txt_buf,
						"\n#FF0000 Failed to read %d blocks (@LBA %08X),#\n"
						"#FF0000 from eMMC! Verification failed..#\n",
						num, lba_curr);
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);

					free(clmt);
					f_close(&fp);
					if (saveHashes)
					{
						f_close(&hashFp);
						f_unlink(hashFilename);
					}

					return VERIF_STATUS_ERROR;
				}
				manual_system_maintenance(false);

				se_sha_hash_256_async(hashEm, bufEm, num << 9);

				f_lseek(&fp, (u64)sdFileSector << (u64)9);
				if (f_read_fast(&fp, bufSd, num << 9))
				{
					s_printf(gui->txt_buf,
						"\n#FF0000 Failed to read %d blocks (@LBA %08X),#\n"
						"#FF0000 from SD card! Verification failed..#\n",
						num, lba_curr);
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);

					free(clmt);
					f_close(&fp);
					if (saveHashes)
					{
						f_close(&hashFp);
						f_unlink(hashFilename);
					}

					return VERIF_STATUS_ERROR;
				}
				manual_system_maintenance(false);
				se_sha_hash_256_finalize(hashEm);
				se_sha_hash_256_oneshot(hashSd, bufSd, num << 9);
				res = memcmp(hashEm, hashSd, SE_SHA_256_SIZE / 2);

				if (res)
				{
					s_printf(gui->txt_buf,
						"\n#FF0000 SD & eMMC data (@LBA %08X) do not match!#\n"
						"\n#FF0000 Verification failed..#\n",
						lba_curr);
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);

					free(clmt);
					f_close(&fp);
					if (saveHashes)
					{
						f_close(&hashFp);
						f_unlink(hashFilename);
					}

					return VERIF_STATUS_ERROR;
				}

				if (saveHashes)
				{
					// Transform computed hash to readable hexadecimal
					char hashStr[SE_SHA_256_SIZE * 2 + 1];
					char *hashStrPtr = hashStr;
					for (int i = 0; i < SE_SHA_256_SIZE; i++)
					{
						*(hashStrPtr++) = hexa[hashSd[i] >> 4];
						*(hashStrPtr++) = hexa[hashSd[i] & 0x0F];
					}
					hashStr[SE_SHA_256_SIZE * 2] = '\0';

					f_puts(hashStr, &hashFp);
					f_puts("\n", &hashFp);
				}
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

			manual_system_maintenance(false);

			lba_curr += num;
			totalSectorsVer -= num;
			sdFileSector += num;
			sparseShouldVerify++;

			// Check for cancellation combo.
			if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			{
				strcpy(gui->txt_buf, "#FFDD00 Verification was cancelled!#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				msleep(1000);

				free(clmt);
				f_close(&fp);
				if (saveHashes)
				{
					// Remove incomplete hash file.
					f_close(&hashFp);
					f_unlink(hashFilename);
				}

				return VERIF_STATUS_ABORT;
			}
		}
		free(clmt);
		f_close(&fp);
		if (saveHashes)
			f_close(&hashFp);

		lv_bar_set_value(gui->bar, pct);
		s_printf(gui->txt_buf, " "SYMBOL_DOT" %d%%", pct);
		lv_label_set_text(gui->label_pct, gui->txt_buf);
		manual_system_maintenance(true);

		return VERIF_STATUS_OK;
	}
	else
	{
		strcpy(gui->txt_buf, "\n#FFDD00 File not found or could not be loaded!#\n#FFDD00 Verification failed..#\n");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		return VERIF_STATUS_ERROR;
	}
}

bool partial_sd_full_unmount = false;

static int _dump_emmc_part(emmc_tool_gui_t *gui, char *sd_path, int active_part, sdmmc_storage_t *storage, emmc_part_t *part)
{
	static const u32 FAT32_FILESIZE_LIMIT = 0xFFFFFFFF;
	static const u32 SECTORS_TO_MIB_COEFF = 11;

	partial_sd_full_unmount = false;

	u32 verification = n_cfg.verification;
	u32 multipartSplitSize = (1u << 31);
	u32 lba_end = part->lba_end;
	u32 totalSectors = part->lba_end - part->lba_start + 1;
	u32 currPartIdx = 0;
	u32 numSplitParts = 0;
	u32 maxSplitParts = 0;
	bool isSmallSdCard = false;
	bool partialDumpInProgress = false;
	int res = 0;
	char *outFilename = sd_path;
	u32 sdPathLen = strlen(sd_path);

	u32 sector_start = 0, part_idx = 0;
	u32 sector_size = totalSectors;
	u32 sd_sector_off = 0;

	FIL partialIdxFp;
	char partialIdxFilename[12];
	strcpy(partialIdxFilename, "partial.idx");

	if (gui->raw_emummc)
	{
		_get_valid_partition(&sector_start, &sector_size, &part_idx, true);
		if (!part_idx || !sector_size)
		{
			strcpy(gui->txt_buf, "\n#FFDD00 Failed to find a partition...#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 1;
		}
		sd_sector_off = sector_start + (0x2000 * active_part);
		if (active_part == 2)
		{
			// Set new total sectors and lba end sector for percentage calculations.
			totalSectors = sector_size;
			lba_end = sector_size + part->lba_start - 1;
		}
	}

	s_printf(gui->txt_buf, "#96FF00 SD Card free space:# %d MiB\n#96FF00 Total backup size:# %d MiB\n\n",
		(u32)(sd_fs.free_clst * sd_fs.csize >> SECTORS_TO_MIB_COEFF),
		totalSectors >> SECTORS_TO_MIB_COEFF);
	lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
	manual_system_maintenance(true);

	lv_bar_set_value(gui->bar, 0);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 0%");
	lv_bar_set_style(gui->bar, LV_BAR_STYLE_BG, gui->bar_white_bg);
	lv_bar_set_style(gui->bar, LV_BAR_STYLE_INDIC, gui->bar_white_ind);
	manual_system_maintenance(true);

	// 1GB parts for sd cards 8GB and less.
	if ((sd_storage.csd.capacity >> (20 - sd_storage.csd.read_blkbits)) <= 8192)
		multipartSplitSize = (1u << 30);
	// Maximum parts fitting the free space available.
	maxSplitParts = (sd_fs.free_clst * sd_fs.csize) / (multipartSplitSize / EMMC_BLOCKSIZE);

	// Check if the USER partition or the RAW eMMC fits the sd card free space.
	if (totalSectors > (sd_fs.free_clst * sd_fs.csize))
	{
		isSmallSdCard = true;

		strcpy(gui->txt_buf, "\n#FFBA00 Free space is smaller than backup size.#\n");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		if (!maxSplitParts)
		{
			strcpy(gui->txt_buf, "#FFDD00 Not enough free space for Partial Backup!#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 1;
		}
	}
	// Check if we are continuing a previous raw eMMC or USER partition backup in progress.
	if (f_open(&partialIdxFp, partialIdxFilename, FA_READ) == FR_OK && totalSectors > (FAT32_FILESIZE_LIMIT / EMMC_BLOCKSIZE))
	{
		strcpy(gui->txt_buf, "\n#AEFD14 Partial Backup in progress. Continuing...#\n");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		partialDumpInProgress = true;
		// Force partial dumping, even if the card is larger.
		isSmallSdCard = true;

		f_read(&partialIdxFp, &currPartIdx, 4, NULL);
		f_close(&partialIdxFp);

		if (!maxSplitParts)
		{
			strcpy(gui->txt_buf, "\n#FFDD00 Not enough free space for Partial Backup!#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 1;
		}

		// Increase maxSplitParts to accommodate previously backed up parts.
		maxSplitParts += currPartIdx;
	}
	else if (isSmallSdCard)
	{
		s_printf(gui->txt_buf, "\n#FFBA00 Partial Backup enabled (%d MiB parts)...#\n", multipartSplitSize >> 20);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);
	}

	// Check if filesystem is FAT32 or the free space is smaller and backup in parts.
	if (((sd_fs.fs_type != FS_EXFAT) && totalSectors > (FAT32_FILESIZE_LIMIT / EMMC_BLOCKSIZE)) || isSmallSdCard)
	{
		u32 multipartSplitSectors = multipartSplitSize / EMMC_BLOCKSIZE;
		numSplitParts = (totalSectors + multipartSplitSectors - 1) / multipartSplitSectors;

		outFilename[sdPathLen++] = '.';

		// Continue from where we left, if Partial Backup in progress.
		_update_filename(outFilename, sdPathLen, partialDumpInProgress ? currPartIdx : 0);
	}

	FIL fp;
	if (!f_open(&fp, outFilename, FA_READ))
	{
		f_close(&fp);

		lv_obj_t *warn_mbox_bg = create_mbox_text(
			"#FFDD00 An existing backup has been detected!#\n\n"
			"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.", false);
		manual_system_maintenance(true);

		if (!(btn_wait() & BTN_POWER))
		{
			lv_obj_del(warn_mbox_bg);
			return 1;
		}
		lv_obj_del(warn_mbox_bg);
	}

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

		return 1;
	}

	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;

	u32 lba_curr = part->lba_start;
	u32 lbaStartPart = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;
	DWORD *clmt = NULL;

	// Continue from where we left, if Partial Backup in progress.
	if (partialDumpInProgress)
	{
		lba_curr += currPartIdx * (multipartSplitSize / EMMC_BLOCKSIZE);
		totalSectors -= currPartIdx * (multipartSplitSize / EMMC_BLOCKSIZE);
		lbaStartPart = lba_curr; // Update the start LBA for verification.
	}
	u64 totalSize = (u64)((u64)totalSectors << 9);
	if (!isSmallSdCard && (sd_fs.fs_type == FS_EXFAT || totalSize <= FAT32_FILESIZE_LIMIT))
		clmt = f_expand_cltbl(&fp, SZ_4M, totalSize);
	else
		clmt = f_expand_cltbl(&fp, SZ_4M, MIN(totalSize, multipartSplitSize));

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

			if (verification && !gui->raw_emummc)
			{
				// Verify part.
				res = _emmc_sd_copy_verify(gui, storage, lbaStartPart, outFilename, part, true);
				switch (res)
				{
				case VERIF_STATUS_OK:
					break;
				case VERIF_STATUS_ERROR:
					strcpy(gui->txt_buf, "\n#FFDD00 Please try again...#\n");
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);
					return 1;
				case VERIF_STATUS_ABORT:
					verification = 0;
					break;
				}

				lv_bar_set_style(gui->bar, LV_BAR_STYLE_BG, gui->bar_white_bg);
				lv_bar_set_style(gui->bar, LV_BAR_STYLE_INDIC, gui->bar_white_ind);
			}

			_update_filename(outFilename, sdPathLen, currPartIdx);

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
					strcpy(gui->txt_buf, "\n#FF0000 Error creating partial.idx file!#\n");
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);

					return 1;
				}

				// More parts to backup that do not currently fit the sd card free space or fatal error.
				if (currPartIdx >= maxSplitParts)
				{
					create_mbox_text(
						"#96FF00 Partial Backup in progress!#\n\n"
						"#96FF00 1.# Press OK to unmount SD Card.\n"
						"#96FF00 2.# Remove SD Card and move files to free space.\n"
						"#FFDD00 Don\'t move the partial.idx file!#\n"
						"#96FF00 3.# Re-insert SD Card.\n"
						"#96FF00 4.# Select the SAME option again to continue.", true);

					partial_sd_full_unmount = true;

					return 0;
				}
			}

			// Create next part.
			s_printf(gui->txt_buf, "%s#", outFilename + strlen(gui->base_path));
			lv_label_cut_text(gui->label_info,
				strlen(lv_label_get_text(gui->label_info)) - strlen(outFilename + strlen(gui->base_path)) - 1,
				strlen(outFilename + strlen(gui->base_path)) + 1);
			lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);
			lbaStartPart = lba_curr;
			res = f_open(&fp, outFilename, FA_CREATE_ALWAYS | FA_WRITE);
			if (res)
			{
				s_printf(gui->txt_buf, "\n#FF0000 Error (%d) while creating#\n#FFDD00 %s#\n", res, outFilename);
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return 1;
			}

			bytesWritten = 0;

			totalSize = (u64)((u64)totalSectors << 9);
			clmt = f_expand_cltbl(&fp, SZ_4M, MIN(totalSize, multipartSplitSize));
		}

		retryCount = 0;
		num = MIN(totalSectors, NUM_SECTORS_PER_ITER);

		int res_read;
		if (!gui->raw_emummc)
			res_read = sdmmc_storage_read(storage, lba_curr, num, buf);
		else
			res_read = sdmmc_storage_read(&sd_storage, lba_curr + sd_sector_off, num, buf);

		while (res_read)
		{
			if (!gui->raw_emummc)
			{
				s_printf(gui->txt_buf,
					"\n#FFDD00 Error reading %d blocks @ LBA %08X,#\n"
					"#FFDD00 from eMMC (try %d). #",
					num, lba_curr, ++retryCount);
			}
			else
			{
				s_printf(gui->txt_buf,
					"\n#FFDD00 Error reading %d blocks @ LBA %08X,#\n"
					"#FFDD00 from emuMMC @ %08X (try %d). #",
					num, lba_curr + sd_sector_off, lba_curr, ++retryCount);
			}
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			msleep(150);
			if (retryCount >= 3)
			{
				strcpy(gui->txt_buf, "#FF0000 Aborting...#\nPlease try again...\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				f_close(&fp);
				free(clmt);
				f_unlink(outFilename);

				return 1;
			}
			else
			{
				strcpy(gui->txt_buf, "#FFDD00 Retrying...#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);
			}

			if (!gui->raw_emummc)
				res_read = sdmmc_storage_read(storage, lba_curr, num, buf);
			else
				res_read = sdmmc_storage_read(&sd_storage, lba_curr + sd_sector_off, num, buf);
			manual_system_maintenance(false);
		}

		res = f_write_fast(&fp, buf, EMMC_BLOCKSIZE * num);

		if (res)
		{
			s_printf(gui->txt_buf, "\n#FF0000 Fatal error (%d) when writing to SD Card#\nPlease try again...\n", res);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			f_close(&fp);
			free(clmt);
			f_unlink(outFilename);

			return 1;
		}

		manual_system_maintenance(false);

		pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(lba_end - part->lba_start);
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
		bytesWritten += num * EMMC_BLOCKSIZE;

		// Force a flush after a lot of data if not splitting.
		if (numSplitParts == 0 && bytesWritten >= multipartSplitSize)
		{
			f_sync(&fp);
			bytesWritten = 0;
		}

		// Check for cancellation combo.
		if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
		{
			strcpy(gui->txt_buf, "\n#FFDD00 The backup was cancelled!#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			msleep(1500);

			f_close(&fp);
			free(clmt);
			f_unlink(outFilename);

			return 1;
		}
	}
	lv_bar_set_value(gui->bar, 100);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 100%");
	manual_system_maintenance(true);

	// Backup operation ended successfully.
	f_close(&fp);
	free(clmt);

	if (verification && !gui->raw_emummc)
	{
		// Verify last part or single file backup.
		if (_emmc_sd_copy_verify(gui, storage, lbaStartPart, outFilename, part, true) == VERIF_STATUS_ERROR)
		{
			strcpy(gui->txt_buf, "\n#FFDD00 Please try again...#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 1;
		}
		lv_bar_set_value(gui->bar, 100);
		lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 100%");
		manual_system_maintenance(true);
	}

	// Remove partial backup index file if no fatal errors occurred.
	if (isSmallSdCard)
	{
		f_unlink(partialIdxFilename);

		create_mbox_text(
			"#96FF00 Partial Backup done!#\n\n"
			"You can now join the files if needed\nand get the complete eMMC RAW GPP backup.", true);

		partial_sd_full_unmount = true;
	}

	return 0;
}

void dump_emmc_selected(emmcPartType_t dumpType, emmc_tool_gui_t *gui)
{
	int res = 1;
	u32 timer = 0;

	char *txt_buf = (char *)malloc(SZ_16K);
	gui->txt_buf = txt_buf;

	txt_buf[0] = 0;
	lv_label_set_text(gui->label_log, txt_buf);

	lv_label_set_text(gui->label_info, "Checking for available free space...");
	manual_system_maintenance(true);

	if (sd_mount())
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init SD!#");
		goto out;
	}

	// Get SD Card free space for Partial Backup.
	f_getfree("", &sd_fs.free_clst, NULL);

	if (emmc_initialize(false))
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init eMMC!#");
		goto out;
	}

	int i = 0;
	char sdPath[OUT_FILENAME_SZ];
	// Create Restore folders, if they do not exist.
	emmcsn_path_impl(sdPath, "/restore", "", &emmc_storage);
	emmcsn_path_impl(sdPath, "/restore/emummc", "", &emmc_storage);
	emmcsn_path_impl(sdPath, "/restore/partitions", "", &emmc_storage);

	// Set folder to backup/{emmc_sn}.
	emmcsn_path_impl(sdPath, "", "", &emmc_storage);
	gui->base_path = (char *)malloc(strlen(sdPath) + 1);
	strcpy(gui->base_path, sdPath);

	timer = get_tmr_s();
	if (dumpType & PART_BOOT)
	{
		const u32 BOOT_PART_SIZE = emmc_storage.ext_csd.boot_mult << 17;
		const u32 BOOT_PART_SECTORS = 0x2000; // Force 4 MiB for emuMMC.

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		if (!gui->raw_emummc)
			bootPart.lba_end = (BOOT_PART_SIZE / EMMC_BLOCKSIZE) - 1;
		else
			bootPart.lba_end = BOOT_PART_SECTORS - 1;
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

			emmc_set_partition(i + 1);

			// Set filename to backup/{emmc_sn}/BOOT0/1 or backup/{emmc_sn}/emummc/BOOT0/1.
			if (!gui->raw_emummc)
				emmcsn_path_impl(sdPath, "",        bootPart.name, &emmc_storage);
			else
				emmcsn_path_impl(sdPath, "/emummc", bootPart.name, &emmc_storage);

			res = _dump_emmc_part(gui, sdPath, i, &emmc_storage, &bootPart);

			if (res)
				strcpy(txt_buf, "#FFDD00 Failed!#\n");
			else
				strcpy(txt_buf, "Done!\n");

			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
			manual_system_maintenance(true);
		}
	}

	if ((dumpType & PART_SYSTEM) || (dumpType & PART_USER) || (dumpType & PART_RAW))
	{
		emmc_set_partition(EMMC_GPP);

		if ((dumpType & PART_SYSTEM) || (dumpType & PART_USER))
		{
			emmcsn_path_impl(sdPath, "/partitions", "", &emmc_storage);
			gui->base_path = (char *)malloc(strlen(sdPath) + 1);
			strcpy(gui->base_path, sdPath);

			LIST_INIT(gpt);
			emmc_gpt_parse(&gpt);
			LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
			{
				if ((dumpType & PART_USER) == 0 && !strcmp(part->name, "USER"))
					continue;
				if ((dumpType & PART_SYSTEM) == 0 && strcmp(part->name, "USER"))
					continue;

				s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n",
					i, part->name, part->lba_start, part->lba_end);
				lv_label_set_text(gui->label_info, txt_buf);
				s_printf(txt_buf, "%02d: %s... ", i, part->name);
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
				manual_system_maintenance(true);
				i++;

				emmcsn_path_impl(sdPath, "/partitions", part->name, &emmc_storage);
				res = _dump_emmc_part(gui, sdPath, 0, &emmc_storage, part);
				// If a part failed, don't continue.
				if (res)
				{
					strcpy(txt_buf, "#FFDD00 Failed!#\n");
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
					break;
				}
				else
					strcpy(txt_buf, "Done!\n");

				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
				manual_system_maintenance(true);
			}
			emmc_gpt_free(&gpt);
		}

		if (dumpType & PART_RAW)
		{
			// Get GP partition size dynamically.
			const u32 RAW_AREA_NUM_SECTORS = emmc_storage.sec_cnt;

			emmc_part_t rawPart;
			memset(&rawPart, 0, sizeof(rawPart));
			rawPart.lba_start = 0;
			rawPart.lba_end = RAW_AREA_NUM_SECTORS - 1;
			strcpy(rawPart.name, "rawnand.bin");
			{
				s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n",
					i, rawPart.name, rawPart.lba_start, rawPart.lba_end);
				lv_label_set_text(gui->label_info, txt_buf);
				s_printf(txt_buf, "%02d: %s... ", i, rawPart.name);
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
				manual_system_maintenance(true);

				i++;

				// Set filename to backup/{emmc_sn}/rawnand.bin or backup/{emmc_sn}/emummc/rawnand.bin.
				if (!gui->raw_emummc)
					emmcsn_path_impl(sdPath, "",        rawPart.name, &emmc_storage);
				else
					emmcsn_path_impl(sdPath, "/emummc", rawPart.name, &emmc_storage);

				res = _dump_emmc_part(gui, sdPath, 2, &emmc_storage, &rawPart);

				if (res)
					strcpy(txt_buf, "#FFDD00 Failed!#\n");
				else
					strcpy(txt_buf, "Done!\n");

				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
				manual_system_maintenance(true);
			}
		}
	}

	timer = get_tmr_s() - timer;
	emmc_end();

	if (!res && n_cfg.verification && !gui->raw_emummc)
		s_printf(txt_buf, "Time taken: %dm %ds.\n#96FF00 Finished and verified!#", timer / 60, timer % 60);
	else if (!res)
		s_printf(txt_buf, "Time taken: %dm %ds.\nFinished!", timer / 60, timer % 60);
	else
		s_printf(txt_buf, "Time taken: %dm %ds.", timer / 60, timer % 60);

	lv_label_set_text(gui->label_finish, txt_buf);

out:
	free(txt_buf);
	free(gui->base_path);
	if (!partial_sd_full_unmount)
		sd_unmount();
	else
	{
		partial_sd_full_unmount = false;
		sd_end();
	}
}

static int _hash_line_parse(u8 *hash, const char *line)
{
	for (u32 i = 0; i < SE_SHA_256_SIZE * 2; i++)
	{
		char c = line[i];
		u8 nibble;
		if (c >= '0' && c <= '9')
			nibble = c - '0';
		else if (c >= 'a' && c <= 'f')
			nibble = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			nibble = c - 'A' + 10;
		else
			return 1;

		if (i & 1)
			hash[i >> 1] |= nibble;
		else
			hash[i >> 1] = nibble << 4;
	}

	// Line must end after the hash.
	char c = line[SE_SHA_256_SIZE * 2];
	return (c != '\0' && c != '\n');
}

static const char *_base_name(const char *filename)
{
	const char *name = strrchr(filename, '/');
	return name ? name + 1 : filename;
}

static bool _hash_file_exists(const char *filename)
{
	char hashFilename[HASH_FILENAME_SZ];
	FILINFO fno;

	strcpy(hashFilename, filename);
	strcat(hashFilename, ".sha256sums");

	return !f_stat(hashFilename, &fno);
}

// Open <filename>.sha256sums and check its header.
static int _hash_file_open(emmc_tool_gui_t *gui, FIL *hashFp, const char *filename)
{
	char line[80];
	char hashFilename[HASH_FILENAME_SZ];

	strcpy(hashFilename, filename);
	strcat(hashFilename, ".sha256sums");

	int res = f_open(hashFp, hashFilename, FA_READ);
	if (res)
	{
		s_printf(gui->txt_buf, "\n#FF0000 Error (%d) while opening hash file#\n#FFDD00 %s!#\n", res, _base_name(filename));
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		return VERIF_STATUS_ERROR;
	}

	if (!f_gets(line, sizeof(line), hashFp) || strncmp(line, "# chunksize: ", 13) || (u32)atoi(line + 13) != HASH_CHUNK_SIZE)
	{
		s_printf(gui->txt_buf, "\n#FF0000 Invalid hash file for#\n#FFDD00 %s#\n", _base_name(filename));
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		f_close(hashFp);

		return VERIF_STATUS_ERROR;
	}

	return VERIF_STATUS_OK;
}

// Read the next hash line.
static int _hash_line_read(emmc_tool_gui_t *gui, FIL *hashFp, u8 *hash, const char *filename)
{
	char line[80];

	if (f_gets(line, sizeof(line), hashFp) && !_hash_line_parse(hash, line))
		return 0;

	s_printf(gui->txt_buf, "\n#FF0000 Hash file is incomplete or invalid:#\n#FFDD00 %s#\n", _base_name(filename));
	lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
	manual_system_maintenance(true);

	return 1;
}

// Check a file against its .sha256sums.
static int _hash_file_check(emmc_tool_gui_t *gui, const char *filename, u64 doneBytes, u64 totalBytes)
{
	FIL fp;
	FIL hashFp;
	u8 hashFile[SE_SHA_256_SIZE];
	u8 hashSd[SE_SHA_256_SIZE];
	u8 *bufs[2] = { (u8 *)SDXC_BUF_ALIGNED, (u8 *)EMMC_BUF_ALIGNED };
	u32 prevPct = 200;
	u32 chunk = 0;
	bool hashPending = false;
	const char *baseName = _base_name(filename);

	int res = _hash_file_open(gui, &hashFp, filename);
	if (res)
		return res;

	res = f_open(&fp, filename, FA_READ);
	if (res)
	{
		s_printf(gui->txt_buf, "\n#FF0000 Error (%d) while opening file#\n#FFDD00 %s!#\n", res, baseName);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		f_close(&hashFp);

		return VERIF_STATUS_ERROR;
	}

	u64 fileSize = f_size(&fp);
	u64 offset = 0;
	DWORD *clmt = f_expand_cltbl(&fp, SZ_4M, 0);
	if (!clmt)
	{
		s_printf(gui->txt_buf, "\n#FF0000 Failed to create cluster table for#\n#FFDD00 %s#\n", baseName);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		f_close(&fp);
		f_close(&hashFp);

		return VERIF_STATUS_ERROR;
	}

	lv_bar_set_style(gui->bar, LV_BAR_STYLE_BG, gui->bar_teal_bg);
	lv_bar_set_style(gui->bar, LV_BAR_STYLE_INDIC, gui->bar_teal_ind);

	// Read the next chunk while SE hashes the previous one.
	while (true)
	{
		bool fileEnd = offset >= fileSize;
		u32 size = 0;
		u8 *buf = bufs[chunk & 1];

		if (!fileEnd)
		{
			size = (u32)MIN(fileSize - offset, (u64)HASH_CHUNK_SIZE);

			f_lseek(&fp, offset);
			if (f_read_fast(&fp, buf, size))
			{
				s_printf(gui->txt_buf,
					"\n#FF0000 Failed to read from SD card#\n"
					"#FF0000 (@chunk %d)! Validation failed..#\n",
					chunk);
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				res = VERIF_STATUS_ERROR;
				break;
			}
			manual_system_maintenance(false);
		}

		if (hashPending)
		{
			se_sha_hash_256_finalize(hashSd);
			hashPending = false;

			if (_hash_line_read(gui, &hashFp, hashFile, filename))
			{
				res = VERIF_STATUS_ERROR;
				break;
			}

			if (memcmp(hashFile, hashSd, SE_SHA_256_SIZE))
			{
				s_printf(gui->txt_buf,
					"\n#FF0000 SHA256 mismatch (@chunk %d) in#\n"
					"#FFDD00 %s#\n#FF0000 The backup is corrupted!#\n",
					chunk - 1, baseName);
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				res = VERIF_STATUS_ERROR;
				break;
			}
		}

		if (fileEnd)
			break;

		se_sha_hash_256_async(hashSd, buf, size);
		hashPending = true;

		offset += size;
		chunk++;

		u32 pct = (u32)(((doneBytes + offset) * 100u) / totalBytes);
		if (pct != prevPct)
		{
			lv_bar_set_value(gui->bar, pct);
			s_printf(gui->txt_buf, " "SYMBOL_DOT" %d%%", pct);
			lv_label_set_text(gui->label_pct, gui->txt_buf);
			manual_system_maintenance(true);
			prevPct = pct;
		}

		// Check for cancellation combo.
		if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
		{
			strcpy(gui->txt_buf, "\n#FFDD00 Hash validation was cancelled!#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			msleep(1000);

			res = VERIF_STATUS_ABORT;
			break;
		}
	}

	// Make sure SE is idle.
	if (hashPending)
		se_sha_hash_256_finalize(hashSd);

	free(clmt);
	f_close(&fp);
	f_close(&hashFp);

	return res;
}

// Check the hash file layout: one hash for each chunk of the file.
static int _hash_file_check_format(emmc_tool_gui_t *gui, const char *filename, u64 fileSize)
{
	FIL hashFp;
	char line[80];
	u8 hash[SE_SHA_256_SIZE];

	int res = _hash_file_open(gui, &hashFp, filename);
	if (res)
		return res;

	u64 numChunks = (fileSize + HASH_CHUNK_SIZE - 1) / HASH_CHUNK_SIZE;
	for (u64 i = 0; i < numChunks && !res; i++)
		res = _hash_line_read(gui, &hashFp, hash, filename);

	if (!res && f_gets(line, sizeof(line), &hashFp) && line[0] != '\n')
	{
		s_printf(gui->txt_buf, "\n#FF0000 Hash file has more hashes than#\n#FF0000 the file size:#\n#FFDD00 %s#\n", _base_name(filename));
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		res = VERIF_STATUS_ERROR;
	}

	f_close(&hashFp);

	return res;
}

// Validate the backup files before restoring anything.
static int _restore_files_hash_validate(emmc_tool_gui_t *gui, const char *outFilename, u32 sdPathLen, u32 numFiles, bool multipart)
{
	char filename[OUT_FILENAME_SZ];
	u32 filesMissing = 0;
	u32 filesChecked = 0;
	u64 totalBytes = 0;
	u64 doneBytes = 0;
	FILINFO fno;

	strcpy(filename, outFilename);

	// Check the hash files and get the total size.
	for (u32 i = 0; i < numFiles; i++)
	{
		if (multipart)
			_update_filename(filename, sdPathLen, i);

		// Stop at the last existing part.
		if (f_stat(filename, &fno))
			break;
		filesChecked++;

		if (!_hash_file_exists(filename))
			filesMissing++;
		else
		{
			if (_hash_file_check_format(gui, filename, fno.fsize))
			{
				strcpy(gui->txt_buf, "#FFDD00 Aborting...#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return 1;
			}

			totalBytes += fno.fsize;
		}
	}
	if (!totalBytes)
		totalBytes = 1;

	// No hash files and no Full Hashes: old behavior.
	if (filesMissing == filesChecked && n_cfg.verification != 3)
		return 0;

	if (filesMissing)
	{
		s_printf(gui->txt_buf, "\n#FFDD00 %d of %d files have no hash file!#\n", filesMissing, filesChecked);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		s_printf(gui->txt_buf,
			"#FF8000 %d of %d backup files have no .sha256sums file!#\n\n"
			"#FFDD00 These files can not be validated#\n#FFDD00 and might be corrupted!#\n"
			"Backup with #C7EA46 Full Hashes# verification to create them.\n\n"
			"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.", filesMissing, filesChecked);
		lv_obj_t *warn_mbox_bg = create_mbox_text(gui->txt_buf, false);
		manual_system_maintenance(true);

		if (!(btn_wait() & BTN_POWER))
		{
			lv_obj_del(warn_mbox_bg);
			strcpy(gui->txt_buf, "#FFDD00 Aborting...#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 1;
		}
		lv_obj_del(warn_mbox_bg);
	}

	// Nothing to validate.
	if (filesMissing == filesChecked)
		return 0;

	lv_bar_set_value(gui->bar, 0);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 0%");
	lv_obj_set_opa_scale(gui->bar, LV_OPA_COVER);
	lv_obj_set_opa_scale(gui->label_pct, LV_OPA_COVER);

	strcpy(gui->txt_buf, "\nValidating backup files hashes...\n");
	lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
	manual_system_maintenance(true);

	for (u32 i = 0; i < numFiles; i++)
	{
		if (multipart)
			_update_filename(filename, sdPathLen, i);

		// Stop at the last existing part.
		if (f_stat(filename, &fno))
			break;

		// Skip files without a hash file.
		if (!_hash_file_exists(filename))
			continue;

		if (multipart)
		{
			// Show the file being checked.
			s_printf(gui->txt_buf, "%s#", filename + strlen(gui->base_path));
			lv_label_cut_text(gui->label_info,
				strlen(lv_label_get_text(gui->label_info)) - strlen(filename + strlen(gui->base_path)) - 1,
				strlen(filename + strlen(gui->base_path)) + 1);
			lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
		}

		manual_system_maintenance(true);

		int res = _hash_file_check(gui, filename, doneBytes, totalBytes);
		doneBytes += fno.fsize;
		if (res == VERIF_STATUS_ERROR || res == VERIF_STATUS_ABORT)
		{
			strcpy(gui->txt_buf, "#FFDD00 Aborting...#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 1;
		}
	}

	if (!filesMissing)
	{
		strcpy(gui->txt_buf, "#96FF00 Hash validation passed!#\n");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);
	}

	lv_bar_set_value(gui->bar, 0);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 0%");
	lv_bar_set_style(gui->bar, LV_BAR_STYLE_BG, gui->bar_orange_bg);
	lv_bar_set_style(gui->bar, LV_BAR_STYLE_INDIC, gui->bar_orange_ind);
	manual_system_maintenance(true);

	return 0;
}

static int _restore_emmc_part(emmc_tool_gui_t *gui, char *sd_path, int active_part, sdmmc_storage_t *storage, emmc_part_t *part, bool allow_multi_part, bool hash_validate)
{
	static const u32 SECTORS_TO_MIB_COEFF = 11;

	u32 verification = n_cfg.verification;
	u32 lba_end = part->lba_end;
	u32 totalSectors = part->lba_end - part->lba_start + 1;
	u32 currPartIdx = 0;
	u32 numSplitParts = 0;
	u32 lbaStartPart = part->lba_start;
	int res = 0;
	char *outFilename = sd_path;
	u32 sdPathLen = strlen(sd_path);
	u64 fileSize = 0;
	u64 totalCheckFileSize = 0;

	FIL fp;
	FILINFO fno;

	lv_bar_set_value(gui->bar, 0);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 0%");
	lv_bar_set_style(gui->bar, LV_BAR_STYLE_BG, gui->bar_orange_bg);
	lv_bar_set_style(gui->bar, LV_BAR_STYLE_INDIC, gui->bar_orange_ind);
	manual_system_maintenance(true);

	bool use_multipart = false;
	bool check_4MB_aligned = true;

	if (!allow_multi_part)
		goto multipart_not_allowed;

	// Check to see if there is a combined file and if so then use that.
	if (f_stat(outFilename, &fno))
	{
		// If not, check if there are partial files and the total size matches.
		strcpy(gui->txt_buf, "\nNo single file, checking for part files...\n");
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		outFilename[sdPathLen++] = '.';

		_update_filename(outFilename, sdPathLen, numSplitParts);

		s_printf(gui->txt_buf, "#96FF00 Filepath:#\n%s\n#96FF00 Filename:# #FF8000 %s#",
			gui->base_path, outFilename + strlen(gui->base_path));
		lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);

		// Stat total size of the part files.
		while ((u32)((u64)totalCheckFileSize >> (u64)9) != totalSectors)
		{
			_update_filename(outFilename, sdPathLen, numSplitParts);

			s_printf(gui->txt_buf, "%s#", outFilename + strlen(gui->base_path));
			lv_label_cut_text(gui->label_info,
				strlen(lv_label_get_text(gui->label_info)) - strlen(outFilename + strlen(gui->base_path)) - 1,
				strlen(outFilename + strlen(gui->base_path)) + 1);
			lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			if ((u32)((u64)totalCheckFileSize >> (u64)9) > totalSectors)
			{
				strcpy(gui->txt_buf, "\n#FF8000 Size of SD Card split backup exceeds#\n#FF8000 eMMC's selected part size!#\n#FFDD00 Aborting...#");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return 1;
			}
			else if (f_stat(outFilename, &fno))
			{
				if (!gui->raw_emummc)
				{
					s_printf(gui->txt_buf, "\n#FFDD00 Error file not found#\n#FFDD00 %s.#\n\n", outFilename);
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);

					// Attempt a smaller restore.
					if (numSplitParts)
						break;
				}
				else
				{
					// Set new total sectors and lba end sector for percentage calculations.
					totalSectors = (u32)((u64)totalCheckFileSize >> (u64)9);
					part->lba_end = totalSectors + part->lba_start - 1;
					lba_end = part->lba_end;
				}

				// Restore folder is empty.
				if (!numSplitParts)
				{
					strcpy(gui->txt_buf, "\n#FFDD00 Restore folder is empty.#\n\n");
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);

					return 1;
				}
			}
			else
			{
				totalCheckFileSize += (u64)fno.fsize;

				if (check_4MB_aligned && (((u64)fno.fsize) % SZ_4M))
				{
					strcpy(gui->txt_buf, "\n#FFDD00 The split file must be a#\n#FFDD00 multiple of 4 MiB.#\n#FFDD00 Aborting...#");
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);

					return 1;
				}

				check_4MB_aligned = false;
			}

			numSplitParts++;
		}

		s_printf(gui->txt_buf, "%X sectors total.\n", (u32)((u64)totalCheckFileSize >> (u64)9));
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);

		if ((u32)((u64)totalCheckFileSize >> (u64)9) != totalSectors)
		{
			lv_obj_t *warn_mbox_bg = create_mbox_text(
				"#FF8000 Size of SD Card split backup does not match#\n#FF8000 eMMC's selected part size!#\n\n"
				"#FFDD00 The backup might be corrupted,#\n#FFDD00 or missing files!#\n#FFDD00 Aborting is suggested!#\n\n"
				"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.", false);
			manual_system_maintenance(true);

			if (!(btn_wait() & BTN_POWER))
			{
				lv_obj_del(warn_mbox_bg);
				strcpy(gui->txt_buf, "\n#FF0000 Size of SD Card split backup does not match#\n#FF0000 eMMC's selected part size!#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return 1;
			}
			lv_obj_del(warn_mbox_bg);

			// Set new total sectors and lba end sector for percentage calculations.
			totalSectors = (u32)((u64)totalCheckFileSize >> (u64)9);
			part->lba_end = totalSectors + part->lba_start - 1;
			lba_end = part->lba_end;
		}
		use_multipart = true;
		_update_filename(outFilename, sdPathLen, 0);
	}

multipart_not_allowed:
	// Validate all backup files against their hash files before writing anything.
	if (hash_validate && !f_stat(outFilename, &fno) &&
		_restore_files_hash_validate(gui, outFilename, sdPathLen, use_multipart ? numSplitParts : 1, use_multipart))
		return 1;

	res = f_open(&fp, outFilename, FA_READ);
	if (use_multipart)
	{
		s_printf(gui->txt_buf, "%s#", outFilename + strlen(gui->base_path));
		lv_label_cut_text(gui->label_info,
			strlen(lv_label_get_text(gui->label_info)) - strlen(outFilename + strlen(gui->base_path)) - 1,
			strlen(outFilename + strlen(gui->base_path)) + 1);
		lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);
	}
	else
	{
		s_printf(gui->txt_buf, "#96FF00 Filepath:#\n%s\n#96FF00 Filename:# #FF8000 %s#",
			gui->base_path, outFilename + strlen(gui->base_path));
			lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
	}
	manual_system_maintenance(true);
	if (res)
	{
		if (res != FR_NO_FILE)
		{
			s_printf(gui->txt_buf, "\n#FF0000 Error (%d) while opening file. Continuing...#\n", res);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);
		}
		else
		{
			s_printf(gui->txt_buf, "\n#FFDD00 Error (%d) file not found. Continuing...#\n", res);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);
		}

		return 2;
	}
	else if (!use_multipart && (((u32)((u64)f_size(&fp) >> (u64)9)) != totalSectors)) // Check total restore size vs emmc size.
	{
		if (((u32)((u64)f_size(&fp) >> (u64)9)) > totalSectors)
		{
			strcpy(gui->txt_buf, "\n#FF8000 Size of SD Card backup exceeds#\n#FF8000 eMMC's selected part size!#\n#FFDD00 Aborting...#");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			f_close(&fp);

			return 1;
		}
		else if (!gui->raw_emummc)
		{
			lv_obj_t *warn_mbox_bg = create_mbox_text(
				"#FF8000 Size of the SD Card backup does not match#\n#FF8000 eMMC's selected part size!#\n\n"
				"#FFDD00 The backup might be corrupted!#\n#FFDD00 Aborting is suggested!#\n\n"
				"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.", false);
			manual_system_maintenance(true);

			if (!(btn_wait() & BTN_POWER))
			{
				lv_obj_del(warn_mbox_bg);
				strcpy(gui->txt_buf, "\n#FF0000 Size of the SD Card backup does not match#\n#FF0000 eMMC's selected part size.#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				f_close(&fp);

				return 1;
			}
			lv_obj_del(warn_mbox_bg);
		}
		// Set new total sectors and lba end sector for percentage calculations.
		totalSectors = (u32)((u64)f_size(&fp) >> (u64)9);
		lba_end = totalSectors + part->lba_start - 1;
	}
	else
	{
		fileSize = (u64)f_size(&fp);
		s_printf(gui->txt_buf, "\nTotal restore size: %d MiB.\n",
			(u32)((use_multipart ? (u64)totalCheckFileSize : fileSize) >> (u64)9) >> SECTORS_TO_MIB_COEFF);
		lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
		manual_system_maintenance(true);
	}

	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;

	u32 lba_curr = part->lba_start;
	u32 bytesWritten = 0;
	u32 prevPct = 200;
	int retryCount = 0;

	u32 num = 0;
	u32 pct = 0;

	DWORD *clmt = f_expand_cltbl(&fp, SZ_4M, 0);

	u32 sector_start = 0, part_idx = 0;
	u32 sector_size = totalSectors;
	u32 sd_sector_off = 0;

	if (gui->raw_emummc)
	{
		_get_valid_partition(&sector_start, &sector_size, &part_idx, false);
		if (!part_idx || !sector_size)
		{
			strcpy(gui->txt_buf, "\n#FFDD00 Failed to find a partition...#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 1;
		}
		sd_sector_off = sector_start + (0x2000 * active_part);
	}

	lv_obj_set_opa_scale(gui->bar, LV_OPA_COVER);
	lv_obj_set_opa_scale(gui->label_pct, LV_OPA_COVER);
	while (totalSectors > 0)
	{
		// If we have more than one part, check the size for the split parts and make sure that the bytes written is not more than that.
		if (numSplitParts != 0 && bytesWritten >= fileSize)
		{
			// If we have more bytes written then close the file pointer and increase the part index we are using
			f_close(&fp);
			free(clmt);
			memset(&fp, 0, sizeof(fp));
			currPartIdx++;

			if (verification && !gui->raw_emummc)
			{
				// Verify part.
				res = _emmc_sd_copy_verify(gui, storage, lbaStartPart, outFilename, part, false);
				switch (res)
				{
				case VERIF_STATUS_OK:
					break;
				case VERIF_STATUS_ERROR:
					strcpy(gui->txt_buf, "\n#FFDD00 Please try again...#\n");
					lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
					manual_system_maintenance(true);
					return 1;
				case VERIF_STATUS_ABORT:
					verification = 0;
					break;
				}
				lv_bar_set_style(gui->bar, LV_BAR_STYLE_BG, gui->bar_orange_bg);
				lv_bar_set_style(gui->bar, LV_BAR_STYLE_INDIC, gui->bar_orange_ind);
			}

			_update_filename(outFilename, sdPathLen, currPartIdx);

			// Read from next part.
			s_printf(gui->txt_buf, "%s#", outFilename + strlen(gui->base_path));
			lv_label_cut_text(gui->label_info,
				strlen(lv_label_get_text(gui->label_info)) - strlen(outFilename + strlen(gui->base_path)) - 1,
				strlen(outFilename + strlen(gui->base_path)) + 1);
			lv_label_ins_text(gui->label_info, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			lbaStartPart = lba_curr;

			// Try to open the next file part
			res = f_open(&fp, outFilename, FA_READ);
			if (res)
			{
				s_printf(gui->txt_buf, "\n#FF0000 Error (%d) while opening file#\n#FFDD00 %s!#\n", res, outFilename);
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				return 1;
			}
			fileSize = (u64)f_size(&fp);
			bytesWritten = 0;
			clmt = f_expand_cltbl(&fp, SZ_4M, 0);
		}

		retryCount = 0;
		num = MIN(totalSectors, NUM_SECTORS_PER_ITER);

		res = f_read_fast(&fp, buf, num << 9);
		manual_system_maintenance(false);

		if (res)
		{
			s_printf(gui->txt_buf,
				"\n#FF0000 Fatal error (%d) when reading from SD!#\n"
				"#FF0000 This device may be in an inoperative state!#\n"
				"#FFDD00 Please try again now!#\n", res);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			f_close(&fp);
			free(clmt);
			return 1;
		}
		if (!gui->raw_emummc)
			res = sdmmc_storage_write(storage, lba_curr, num, buf);
		else
			res = sdmmc_storage_write(&sd_storage, lba_curr + sd_sector_off, num, buf);

		manual_system_maintenance(false);

		while (res)
		{
			s_printf(gui->txt_buf,
				"\n#FFDD00 Error writing %d blocks @ LBA %08X,#\n"
				"#FFDD00 from eMMC (try %d). #",
				num, lba_curr, ++retryCount);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			msleep(150);
			if (retryCount >= 3)
			{
				strcpy(gui->txt_buf, "#FF0000 Aborting...#\n"
					"#FF0000 This device may be in an inoperative state!#\n"
					"#FFDD00 Please try again now!#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);

				f_close(&fp);
				free(clmt);
				return 1;
			}
			else
			{
				strcpy(gui->txt_buf, "#FFDD00 Retrying...#\n");
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
				manual_system_maintenance(true);
			}
			if (!gui->raw_emummc)
				res = sdmmc_storage_write(storage, lba_curr, num, buf);
			else
				res = sdmmc_storage_write(&sd_storage, lba_curr + sd_sector_off, num, buf);
			manual_system_maintenance(false);
		}
		pct = (u64)((u64)(lba_curr - part->lba_start) * 100u) / (u64)(lba_end - part->lba_start);
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
		bytesWritten += num * EMMC_BLOCKSIZE;
	}
	lv_bar_set_value(gui->bar, 100);
	lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 100%");
	manual_system_maintenance(true);

	// Restore operation ended successfully.
	f_close(&fp);
	free(clmt);

	if (verification && !gui->raw_emummc)
	{
		// Verify restored data.
		if (_emmc_sd_copy_verify(gui, storage, lbaStartPart, outFilename, part, false) == VERIF_STATUS_ERROR)
		{
			strcpy(gui->txt_buf, "\n#FFDD00 Please try again...#\n");
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, gui->txt_buf);
			manual_system_maintenance(true);

			return 1;
		}
		lv_bar_set_value(gui->bar, 100);
		lv_label_set_text(gui->label_pct, " "SYMBOL_DOT" 100%");
		manual_system_maintenance(true);
	}

	if (gui->raw_emummc)
	{
		char sdPath[OUT_FILENAME_SZ];
		// Create Restore folders, if they do not exist.
		f_mkdir("emuMMC");
		s_printf(sdPath, "emuMMC/RAW%d", part_idx);
		f_mkdir(sdPath);
		strcat(sdPath, "/raw_based");
		FIL fp_raw;
		f_open(&fp_raw, sdPath, FA_CREATE_ALWAYS | FA_WRITE);
		f_write(&fp_raw, &sector_start, 4, NULL);
		f_close(&fp_raw);

		s_printf(sdPath, "emuMMC/RAW%d", part_idx);
		save_emummc_cfg(part_idx, sector_start, sdPath);
	}

	return 0;
}

void restore_emmc_selected(emmcPartType_t restoreType, emmc_tool_gui_t *gui)
{
	int res = 0;
	u32 timer = 0;

	char *txt_buf = (char *)malloc(SZ_16K);
	gui->txt_buf = txt_buf;

	txt_buf[0] = 0;
	lv_label_set_text(gui->label_log, txt_buf);

	manual_system_maintenance(true);

	if (!gui->raw_emummc)
		strcpy(txt_buf, "#FFDD00 This may render the device inoperative!#");
	else
		strcpy(txt_buf, "#FFDD00 This may render the emuMMC inoperative!#");
	strcat(txt_buf, "\n\n#FFDD00 Are you really sure?#");

	if (gui->raw_emummc)
		strcat(txt_buf, "\n\nOnly the 1st emuMMC found can be restored!");

	if ((restoreType & PART_BOOT) || (restoreType & PART_GP_ALL))
	{
		strcat(txt_buf,
			"\n\nThe mode you selected will only restore\nthe partitions that it can find.\n"
			"If it is not found, it will be skipped\nand continue with the next.");
	}

	u32 orig_msg_len = strlen(txt_buf);

	lv_obj_t *warn_mbox_bg = create_mbox_text(txt_buf, false);
	lv_obj_t *warn_mbox = lv_obj_get_child(warn_mbox_bg, NULL);

	u8 failsafe_wait = 6;
	while (failsafe_wait > 0)
	{
		s_printf(txt_buf + orig_msg_len, "\n\n#888888 Wait... (%ds)#", failsafe_wait);
		lv_mbox_set_text(warn_mbox, txt_buf);
		msleep(1000);
		manual_system_maintenance(true);
		failsafe_wait--;
	}

	s_printf(txt_buf + orig_msg_len, "\n\nPress #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.");
	lv_mbox_set_text(warn_mbox, txt_buf);
	manual_system_maintenance(true);

	if (!(btn_wait() & BTN_POWER))
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Restore operation was aborted!#");
		lv_obj_del(warn_mbox_bg);
		goto out;
	}
	lv_obj_del(warn_mbox_bg);
	manual_system_maintenance(true);

	if (sd_mount())
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init SD!#");
		goto out;
	}

	if (emmc_initialize(false))
	{
		lv_label_set_text(gui->label_info, "#FFDD00 Failed to init eMMC!#");
		goto out;
	}

	int i = 0;
	char sdPath[OUT_FILENAME_SZ];
	if (!gui->raw_emummc)
		emmcsn_path_impl(sdPath, "/restore", "", &emmc_storage);
	else
		emmcsn_path_impl(sdPath, "/restore/emummc", "", &emmc_storage);
	gui->base_path = (char *)malloc(strlen(sdPath) + 1);
	strcpy(gui->base_path, sdPath);

	timer = get_tmr_s();
	if (restoreType & PART_BOOT)
	{
		const u32 BOOT_PART_SIZE = emmc_storage.ext_csd.boot_mult << 17;
		const u32 BOOT_PART_SECTORS = 0x2000; // Force 4 MiB for emuMMC.

		emmc_part_t bootPart;
		memset(&bootPart, 0, sizeof(bootPart));
		bootPart.lba_start = 0;
		if (!gui->raw_emummc)
			bootPart.lba_end = (BOOT_PART_SIZE / EMMC_BLOCKSIZE) - 1;
		else
			bootPart.lba_end = BOOT_PART_SECTORS - 1;
		for (i = 0; i < 2; i++)
		{
			strcpy(bootPart.name, "BOOT");
			bootPart.name[4] = (u8)('0' + i);
			bootPart.name[5] = 0;

			s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n\n\n\n",
				i, bootPart.name, bootPart.lba_start, bootPart.lba_end);
			lv_label_set_text(gui->label_info, txt_buf);
			s_printf(txt_buf, "%02d: %s... ", i, bootPart.name);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
			manual_system_maintenance(true);

			emmc_set_partition(i + 1);

			if (!gui->raw_emummc)
				emmcsn_path_impl(sdPath, "/restore", bootPart.name, &emmc_storage);
			else
				emmcsn_path_impl(sdPath, "/restore/emummc", bootPart.name, &emmc_storage);
			res = _restore_emmc_part(gui, sdPath, i, &emmc_storage, &bootPart, false, true);

			if (res == 1)
				strcpy(txt_buf, "#FFDD00 Failed!#\n");
			else if (!res)
				strcpy(txt_buf, "Done!\n");

			if (res <= 1)
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);

			manual_system_maintenance(true);
		}
	}

	if (restoreType & PART_GP_ALL)
	{
		emmcsn_path_impl(sdPath, "/restore/partitions", "", &emmc_storage);
		gui->base_path = (char *)malloc(strlen(sdPath) + 1);
		strcpy(gui->base_path, sdPath);

		emmc_set_partition(EMMC_GPP);

		LIST_INIT(gpt);
		emmc_gpt_parse(&gpt);
		LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt, link)
		{
			s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n\n\n\n",
				i, part->name, part->lba_start, part->lba_end);
			lv_label_set_text(gui->label_info, txt_buf);
			s_printf(txt_buf, "%02d: %s... ", i, part->name);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
			manual_system_maintenance(true);
			i++;

			emmcsn_path_impl(sdPath, "/restore/partitions", part->name, &emmc_storage);
			res = _restore_emmc_part(gui, sdPath, 0, &emmc_storage, part, false, true);

			if (res == 1)
				strcpy(txt_buf, "#FFDD00 Failed!#\n");
			else if (!res)
				strcpy(txt_buf, "Done!\n");

			if (res <= 1)
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);

			manual_system_maintenance(true);
		}
		emmc_gpt_free(&gpt);
	}

	if (restoreType & PART_RAW)
	{
		// Get GP partition size dynamically.
		const u32 RAW_AREA_NUM_SECTORS = emmc_storage.sec_cnt;

		emmc_part_t rawPart;
		memset(&rawPart, 0, sizeof(rawPart));
		rawPart.lba_start = 0;
		rawPart.lba_end = RAW_AREA_NUM_SECTORS - 1;
		strcpy(rawPart.name, "rawnand.bin");
		{
			s_printf(txt_buf, "#00DDFF %02d: %s#\n#00DDFF Range: 0x%08X - 0x%08X#\n\n\n\n\n",
				i, rawPart.name, rawPart.lba_start, rawPart.lba_end);
			lv_label_set_text(gui->label_info, txt_buf);
			s_printf(txt_buf, "%02d: %s... ", i, rawPart.name);
			lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);
			manual_system_maintenance(true);
			i++;

			if (!gui->raw_emummc)
				emmcsn_path_impl(sdPath, "/restore", rawPart.name, &emmc_storage);
			else
				emmcsn_path_impl(sdPath, "/restore/emummc", rawPart.name, &emmc_storage);
			res = _restore_emmc_part(gui, sdPath, 2, &emmc_storage, &rawPart, true, true);

			if (res == 1)
				strcpy(txt_buf, "#FFDD00 Failed!#\n");
			else if (!res)
				strcpy(txt_buf, "Done!\n");

			if (res <= 1)
				lv_label_ins_text(gui->label_log, LV_LABEL_POS_LAST, txt_buf);

			manual_system_maintenance(true);
		}
	}

	timer = get_tmr_s() - timer;
	emmc_end();

	if (!res && n_cfg.verification && !gui->raw_emummc)
		s_printf(txt_buf, "Time taken: %dm %ds.\n#96FF00 Finished and verified!#", timer / 60, timer % 60);
	else if (!res)
		s_printf(txt_buf, "Time taken: %dm %ds.\nFinished!", timer / 60, timer % 60);
	else
		s_printf(txt_buf, "Time taken: %dm %ds.", timer / 60, timer % 60);

	lv_label_set_text(gui->label_finish, txt_buf);

out:
	free(txt_buf);
	free(gui->base_path);
	sd_unmount();
}
