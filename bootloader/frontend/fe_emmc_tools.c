/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 Rajko Stojadinovic
 * Copyright (c) 2018 CTCaer
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

#include "fe_emmc_tools.h"
#include "../config/config.h"
#include "../gfx/gfx.h"
#include "../gfx/tui.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../sec/se.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/util.h"

extern sdmmc_t sd_sdmmc;
extern sdmmc_storage_t sd_storage;
extern FATFS sd_fs;
extern hekate_config h_cfg;
extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;

extern bool sd_mount();
extern void sd_unmount();
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

//TODO: Create more macros (info, header, debug, etc) with different colors and utilize them for consistency.
#define EPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, 0xFFCCCCCC)
#define EPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFF0000, args, 0xFFCCCCCC)
#define WPRINTF(text) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFFDD00, 0xFFCCCCCC)
#define WPRINTFARGS(text, args...) gfx_printf(&gfx_con, "%k"text"%k\n", 0xFFFFDD00, args, 0xFFCCCCCC)

static int _dump_emmc_verify(sdmmc_storage_t *storage, u32 lba_curr, char *outFilename, emmc_part_t *part)
{
	FIL fp;
	u32 btn = 0;
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
				res = memcmp(hashEm, hashSd, 0x10);
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

			btn = btn_wait_timeout(0, BTN_VOL_DOWN | BTN_VOL_UP);
			if ((btn & BTN_VOL_DOWN) && (btn & BTN_VOL_UP))
			{
				gfx_con.fntsz = 16;
				WPRINTF("\n\nThe verification was cancelled!");
				EPRINTF("\nPress any key...\n");
				msleep(1500);

				free(bufEm);
				free(bufSd);
				f_close(&fp);

				return 0;
			}
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

static int _dump_emmc_part(char *sd_path, sdmmc_storage_t *storage, emmc_part_t *part)
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

		if (!(btn_wait() & BTN_POWER))
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
				if (_dump_emmc_verify(storage, lbaStartPart, outFilename, part))
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
		if (_dump_emmc_verify(storage, lbaStartPart, outFilename, part))
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

static void _dump_emmc_selected(emmcPartType_t dumpType)
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
			res = _dump_emmc_part(sdPath, &storage, &bootPart);
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
				res = _dump_emmc_part(sdPath, &storage, part);
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
				res = _dump_emmc_part(sdPath, &storage, &rawPart);
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

void dump_emmc_system()  { _dump_emmc_selected(PART_SYSTEM); }
void dump_emmc_user()    { _dump_emmc_selected(PART_USER); }
void dump_emmc_boot()    { _dump_emmc_selected(PART_BOOT); }
void dump_emmc_rawnand() { _dump_emmc_selected(PART_RAW); }

static int _restore_emmc_part(char *sd_path, sdmmc_storage_t *storage, emmc_part_t *part)
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
		if (res != FR_NO_FILE)
			EPRINTFARGS("Error (%d) while opening backup. Continuing...\n", res);
		else
			WPRINTFARGS("Error (%d) file not found. Continuing...\n", res);
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
		if (_dump_emmc_verify(storage, lbaStartPart, outFilename, part))
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

static void _restore_emmc_selected(emmcPartType_t restoreType)
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
			res = _restore_emmc_part(sdPath, &storage, &bootPart);
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
			res = _restore_emmc_part(sdPath, &storage, part);
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
			res = _restore_emmc_part(sdPath, &storage, &rawPart);
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

void restore_emmc_boot()      { _restore_emmc_selected(PART_BOOT); }
void restore_emmc_rawnand()   { _restore_emmc_selected(PART_RAW); }
void restore_emmc_gpp_parts() { _restore_emmc_selected(PART_GP_ALL); }
