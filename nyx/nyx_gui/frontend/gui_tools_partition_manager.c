/*
 * Copyright (c) 2019-2022 CTCaer
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

#include <stdlib.h>

#include <bdk.h>

#include "gui.h"
#include "gui_tools.h"
#include "gui_tools_partition_manager.h"
#include <libs/fatfs/diskio.h>
#include <libs/lvgl/lvgl.h>

extern volatile boot_cfg_t *b_cfg;
extern volatile nyx_storage_t *nyx_str;

typedef struct _partition_ctxt_t
{
	u32 total_sct;
	u32 alignment;
	int backup_possible;

	s32 hos_size;
	u32 emu_size;
	u32 l4t_size;
	u32 and_size;

	bool emu_double;
	bool emmc_is_64gb;

	mbr_t mbr_old;

	lv_obj_t *bar_hos;
	lv_obj_t *bar_emu;
	lv_obj_t *bar_l4t;
	lv_obj_t *bar_and;

	lv_obj_t *sep_emu;
	lv_obj_t *sep_l4t;
	lv_obj_t *sep_and;

	lv_obj_t *slider_bar_hos;
	lv_obj_t *slider_emu;
	lv_obj_t *slider_l4t;
	lv_obj_t *slider_and;

	lv_obj_t *lbl_hos;
	lv_obj_t *lbl_emu;
	lv_obj_t *lbl_l4t;
	lv_obj_t *lbl_and;
} partition_ctxt_t;

typedef struct _l4t_flasher_ctxt_t
{
	u32 offset_sct;
	u32 image_size_sct;
} l4t_flasher_ctxt_t;

partition_ctxt_t part_info;
l4t_flasher_ctxt_t l4t_flash_ctxt;

lv_obj_t *btn_flash_l4t;
lv_obj_t *btn_flash_android;

static int _backup_and_restore_files(char *path, u32 *total_files, u32 *total_size, const char *dst, const char *src, lv_obj_t **labels)
{
	FRESULT res;
	FIL fp_src;
	FIL fp_dst;
	DIR dir;
	u32 dirLength = 0;
	static FILINFO fno;

	if (src)
		f_chdrive(src);

	// Open directory.
	res = f_opendir(&dir, path);
	if (res != FR_OK)
		return res;

	if (labels)
		lv_label_set_text(labels[0], path);

	dirLength = strlen(path);
	for (;;)
	{
		// Clear file path.
		path[dirLength] = 0;

		// Read a directory item.
		res = f_readdir(&dir, &fno);

		// Break on error or end of dir.
		if (res != FR_OK || fno.fname[0] == 0)
			break;

		// Set new directory or file.
		memcpy(&path[dirLength], "/", 1);
		strcpy(&path[dirLength + 1], fno.fname);

		if (labels)
		{
			lv_label_set_text(labels[1], fno.fname);
			manual_system_maintenance(true);
		}

		// Copy file to destination disk.
		if (!(fno.fattrib & AM_DIR))
		{
			u32 file_size = fno.fsize > RAMDISK_CLUSTER_SZ ? fno.fsize : RAMDISK_CLUSTER_SZ; // Ramdisk cluster size.

			// Check for overflow.
			if ((file_size + *total_size) < *total_size)
			{
				// Set size to > 1GB, skip next folders and return.
				*total_size = SZ_2G;
				res = -1;
				break;
			}

			*total_size += file_size;
			*total_files += 1;

			if (src && dst)
			{
				u32 file_size = fno.fsize;

				// Open file for writing.
				f_chdrive(dst);
				f_open(&fp_dst, path, FA_CREATE_ALWAYS | FA_WRITE);
				f_lseek(&fp_dst, fno.fsize);
				f_lseek(&fp_dst, 0);

				// Open file for reading.
				f_chdrive(src);
				f_open(&fp_src, path, FA_READ);

				while (file_size)
				{
					u32 chunk_size = MIN(file_size, SZ_4M); // 4MB chunks.
					file_size -= chunk_size;

					// Copy file to buffer.
					f_read(&fp_src, (void *)SDXC_BUF_ALIGNED, chunk_size, NULL);
					manual_system_maintenance(true);

					// Write file to disk.
					f_write(&fp_dst, (void *)SDXC_BUF_ALIGNED, chunk_size, NULL);
				}
				f_close(&fp_src);

				// Finalize copied file.
				f_close(&fp_dst);
				f_chdrive(dst);
				f_chmod(path, fno.fattrib, 0xFF);

				f_chdrive(src);
			}

			// If total is > 1GB exit.
			if (*total_size > (RAM_DISK_SZ - SZ_16M)) // 0x2400000.
			{
				// Skip next folders and return.
				res = -1;
				break;
			}
		}
		else // It's a directory.
		{
			if (!memcmp("System Volume Information", fno.fname, 25))
				continue;

			// Create folder to destination.
			if (dst)
			{
				f_chdrive(dst);
				f_mkdir(path);
				f_chmod(path, fno.fattrib, 0xFF);
			}
			// Enter the directory.
			res = _backup_and_restore_files(path, total_files, total_size, dst, src, labels);
			if (res != FR_OK)
				break;

			if (labels)
			{
				// Clear folder path.
				path[dirLength] = 0;
				lv_label_set_text(labels[0], path);
			}
		}
	}

	f_closedir(&dir);

	return res;
}

static void _prepare_and_flash_mbr_gpt()
{
	mbr_t mbr;
	u8 random_number[16];

	// Read current MBR.
	sdmmc_storage_read(&sd_storage, 0, 1, &mbr);

	// Copy over metadata if they exist.
	if (*(u32 *)&part_info.mbr_old.bootstrap[0x80])
		memcpy(&mbr.bootstrap[0x80], &part_info.mbr_old.bootstrap[0x80], 304);

	// Clear the first 16MB.
	memset((void *)SDMMC_UPPER_BUFFER, 0, SZ_16M);
	sdmmc_storage_write(&sd_storage, 0, 0x8000, (void *)SDMMC_UPPER_BUFFER);

	u8 mbr_idx = 1;
	se_gen_prng128(random_number);
	memcpy(&mbr.signature, random_number, 4);

	// Apply L4T Linux second to MBR if no Android.
	if (part_info.l4t_size && !part_info.and_size)
	{
		mbr.partitions[mbr_idx].type = 0x83; // Linux system partition.
		mbr.partitions[mbr_idx].start_sct = 0x8000 + ((u32)part_info.hos_size << 11);
		mbr.partitions[mbr_idx].size_sct = part_info.l4t_size << 11;
		sdmmc_storage_write(&sd_storage, mbr.partitions[mbr_idx].start_sct, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		mbr_idx++;
	}

	// emuMMC goes second or third. Next to L4T if no Android.
	if (part_info.emu_size)
	{
		mbr.partitions[mbr_idx].type = 0xE0; // emuMMC partition.
		mbr.partitions[mbr_idx].start_sct = 0x8000 + ((u32)part_info.hos_size << 11) + (part_info.l4t_size << 11) + (part_info.and_size << 11);

		if (!part_info.emu_double)
			mbr.partitions[mbr_idx].size_sct = (part_info.emu_size << 11) - 0x800; // Reserve 1MB.
		else
		{
			mbr.partitions[mbr_idx].size_sct = part_info.emu_size << 10;
			mbr_idx++;

			// 2nd emuMMC.
			mbr.partitions[mbr_idx].type = 0xE0; // emuMMC partition.
			mbr.partitions[mbr_idx].start_sct = mbr.partitions[mbr_idx - 1].start_sct + (part_info.emu_size << 10);
			mbr.partitions[mbr_idx].size_sct = (part_info.emu_size << 10) - 0x800; // Reserve 1MB.
		}
		mbr_idx++;
	}

	if (part_info.and_size)
	{
		gpt_t *gpt = calloc(1, sizeof(gpt_t));
		gpt_header_t gpt_hdr_backup = { 0 };

		mbr.partitions[mbr_idx].type = 0xEE; // GPT protective partition.
		mbr.partitions[mbr_idx].start_sct = 1;
		mbr.partitions[mbr_idx].size_sct = sd_storage.sec_cnt - 1;
		mbr_idx++;

		// Set GPT header.
		memcpy(&gpt->header.signature, "EFI PART", 8);
		gpt->header.revision = 0x10000;
		gpt->header.size = 92;
		gpt->header.my_lba = 1;
		gpt->header.alt_lba = sd_storage.sec_cnt - 1;
		gpt->header.first_use_lba = (sizeof(mbr_t) + sizeof(gpt_t)) >> 9;
		gpt->header.last_use_lba = sd_storage.sec_cnt - 0x800 - 1; // sd_storage.sec_cnt - 33 is start of backup gpt partition entries.
		se_gen_prng128(random_number);
		memcpy(gpt->header.disk_guid, random_number, 10);
		memcpy(gpt->header.disk_guid + 10, "NYXGPT", 6);
		gpt->header.part_ent_lba = 2;
		gpt->header.part_ent_size = 128;

		// Set GPT partitions.
		u8 basic_part_guid[] = { 0xA2, 0xA0, 0xD0, 0xEB,  0xE5, 0xB9,  0x33, 0x44,  0x87, 0xC0,  0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 };
		memcpy(gpt->entries[0].type_guid, basic_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[0].part_guid, random_number, 16);

		// Clear non-standard Windows MBR attributes. bit4: Read only, bit5: Shadow copy, bit6: Hidden, bit7: No drive letter.
		gpt->entries[0].part_guid[7] = 0;

		gpt->entries[0].lba_start = mbr.partitions[0].start_sct;
		gpt->entries[0].lba_end = mbr.partitions[0].start_sct + mbr.partitions[0].size_sct - 1;
		memcpy(gpt->entries[0].name, (char[]) { 'h', 0, 'o', 0, 's', 0, '_', 0, 'd', 0, 'a', 0, 't', 0, 'a', 0 }, 16);

		u8 gpt_idx = 1;
		u32 curr_part_lba = 0x8000 + ((u32)part_info.hos_size << 11);
		u8 android_part_guid[] = { 0xAF, 0x3D, 0xC6, 0x0F,  0x83, 0x84,  0x72, 0x47,  0x8E, 0x79,  0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4 };
		if (part_info.l4t_size)
		{
			memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
			se_gen_prng128(random_number);
			memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
			gpt->entries[gpt_idx].lba_start = curr_part_lba;
			gpt->entries[gpt_idx].lba_end = curr_part_lba + (part_info.l4t_size << 11) - 1;
			memcpy(gpt->entries[gpt_idx].name, (char[]) { 'l', 0, '4', 0, 't', 0 }, 6);
			sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.

			curr_part_lba += (part_info.l4t_size << 11);
			gpt_idx++;
		}

		// Android Vendor partition.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + 0x200000 - 1; // 1GB.
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'v', 0, 'e', 0, 'n', 0, 'd', 0, 'o', 0, 'r', 0 }, 12);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		curr_part_lba += 0x200000;
		gpt_idx++;

		// Android System partition.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + 0x400000 - 1; // 2GB.
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'A', 0, 'P', 0, 'P', 0 }, 6);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		curr_part_lba += 0x400000;
		gpt_idx++;

		// Android Linux Kernel partition.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + 0x10000 - 1; // 32MB.
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'L', 0, 'N', 0, 'X', 0 }, 6);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		curr_part_lba += 0x10000;
		gpt_idx++;

		// Android Recovery partition.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + 0x20000 - 1; // 64MB.
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'S', 0, 'O', 0, 'S', 0 }, 6);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		curr_part_lba += 0x20000;
		gpt_idx++;

		// Android Device Tree Reference partition.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + 0x800 - 1; // 1MB.
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'D', 0, 'T', 0, 'B', 0 }, 6);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		curr_part_lba += 0x800;
		gpt_idx++;

		// Android Encryption partition.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + 0x8000 - 1; // 16MB.
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'M', 0, 'D', 0, 'A', 0 }, 6);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x8000, (void *)SDMMC_UPPER_BUFFER); // Clear 16MB.
		curr_part_lba += 0x8000;
		gpt_idx++;

		// Android Cache partition.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + 0x15E000 - 1; // 700MB.
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'C', 0, 'A', 0, 'C', 0 }, 6);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		curr_part_lba += 0x15E000;
		gpt_idx++;

		// Android Misc partition.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + 0x1800 - 1; // 3MB.
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'M', 0, 'S', 0, 'C', 0 }, 6);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		curr_part_lba += 0x1800;
		gpt_idx++;

		// Android Userdata partition.
		u32 user_size = (part_info.and_size << 11) - 0x798000; // Subtract the other partitions (3888MB).
		if (!part_info.emu_size)
			user_size -= 0x800; // Reserve 1MB.
		memcpy(gpt->entries[gpt_idx].type_guid, android_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
		gpt->entries[gpt_idx].lba_start = curr_part_lba;
		gpt->entries[gpt_idx].lba_end = curr_part_lba + user_size - 1;
		memcpy(gpt->entries[gpt_idx].name, (char[]) { 'U', 0, 'D', 0, 'A', 0 }, 6);
		sdmmc_storage_write(&sd_storage, curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		curr_part_lba += user_size;
		gpt_idx++;

		if (part_info.emu_size)
		{
			u8 emu_part_guid[] = { 0x00, 0x7E, 0xCA, 0x11,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  'e', 'm', 'u', 'M', 'M', 'C' };
			memcpy(gpt->entries[gpt_idx].type_guid, emu_part_guid, 16);
			se_gen_prng128(random_number);
			memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
			gpt->entries[gpt_idx].lba_start = curr_part_lba;
			if (!part_info.emu_double)
				gpt->entries[gpt_idx].lba_end = curr_part_lba + (part_info.emu_size << 11) - 0x800 - 1; // Reserve 1MB.
			else
				gpt->entries[gpt_idx].lba_end = curr_part_lba + (part_info.emu_size << 10) - 1;
			memcpy(gpt->entries[gpt_idx].name, (char[]) { 'e', 0, 'm', 0, 'u', 0, 'm', 0, 'm', 0, 'c', 0 }, 12);
			gpt_idx++;

			if (part_info.emu_double)
			{
				curr_part_lba += (part_info.emu_size << 10);
				memcpy(gpt->entries[gpt_idx].type_guid, emu_part_guid, 16);
				se_gen_prng128(random_number);
				memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
				gpt->entries[gpt_idx].lba_start = curr_part_lba;
				gpt->entries[gpt_idx].lba_end = curr_part_lba + (part_info.emu_size << 10) - 0x800 - 1; // Reserve 1MB.
				memcpy(gpt->entries[gpt_idx].name, (char[]) { 'e', 0, 'm', 0, 'u', 0, 'm', 0, 'm', 0, 'c', 0, '2', 0 }, 14);
				gpt_idx++;
			}
		}

		// Set final GPT header parameters.
		gpt->header.num_part_ents = 128;
		gpt->header.part_ents_crc32 = crc32_calc(0, (const u8 *)gpt->entries, sizeof(gpt_entry_t) * 128);
		gpt->header.crc32 = 0; // Set to 0 for calculation.
		gpt->header.crc32 = crc32_calc(0, (const u8 *)&gpt->header, gpt->header.size);

		memcpy(&gpt_hdr_backup, &gpt->header, sizeof(gpt_header_t));
		gpt_hdr_backup.my_lba = sd_storage.sec_cnt - 1;
		gpt_hdr_backup.alt_lba = 1;
		gpt_hdr_backup.part_ent_lba = sd_storage.sec_cnt - 33;
		gpt_hdr_backup.crc32 = 0; // Set to 0 for calculation.
		gpt_hdr_backup.crc32 = crc32_calc(0, (const u8 *)&gpt_hdr_backup, gpt_hdr_backup.size);

		// Write main GPT.
		sdmmc_storage_write(&sd_storage, gpt->header.my_lba, sizeof(gpt_t) >> 9, gpt);

		// Write backup GPT partition table.
		sdmmc_storage_write(&sd_storage, gpt_hdr_backup.part_ent_lba, ((sizeof(gpt_entry_t) * 128) >> 9), gpt->entries);

		// Write backup GPT header.
		sdmmc_storage_write(&sd_storage, gpt_hdr_backup.my_lba, 1, &gpt_hdr_backup);

		free(gpt);
	}

	// Write MBR.
	sdmmc_storage_write(&sd_storage, 0, 1, &mbr);
}

static lv_res_t _action_part_manager_ums_sd(lv_obj_t *btn)
{
	action_ums_sd(btn);

	// Close and reopen partition manager.
	lv_action_t close_btn_action = lv_btn_get_action(close_btn, LV_BTN_ACTION_CLICK);
	close_btn_action(close_btn);
	lv_obj_del(ums_mbox);
	create_window_partition_manager(NULL);

	return LV_RES_INV;
}

static lv_res_t _action_delete_linux_installer_files(lv_obj_t * btns, const char * txt)
{

	int btn_idx = lv_btnm_get_pressed(btns);

	// Delete parent mbox.
	mbox_action(btns, txt);

	// Flash Linux.
	if (!btn_idx)
	{
		char path[128];

		sd_mount();

		strcpy(path, "switchroot/install/l4t.");

		// Delete all l4t.xx files.
		u32 idx = 0;
		while (true)
		{
			if (idx < 10)
			{
				path[23] = '0';
				itoa(idx, &path[23 + 1], 10);
			}
			else
				itoa(idx, &path[23], 10);

			if (!f_stat(path, NULL))
			{
				f_unlink(path);
			}
			else
				break;

			idx++;
		}

		sd_unmount();
	}

	return LV_RES_INV;
}

static lv_res_t _action_flash_linux_data(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	// Delete parent mbox.
	mbox_action(btns, txt);

	bool succeeded = false;

	if (btn_idx)
		return LV_RES_INV;

	// Flash Linux.
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	static const char *mbox_btn_map2[] = { "\223Delete Installation Files", "\221OK", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 10 * 5);

	lv_mbox_set_text(mbox, "#FF8000 Linux Flasher#");

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);
	lv_label_set_text(lbl_status, "#C7EA46 Status:# Flashing Linux...");

	// Create container to keep content inside.
	lv_obj_t *h1 = lv_cont_create(mbox, NULL);
	lv_cont_set_fit(h1, true, true);
	lv_cont_set_style(h1, &lv_style_transp_tight);

	lv_obj_t *bar = lv_bar_create(h1, NULL);
	lv_obj_set_size(bar, LV_DPI * 30 / 10, LV_DPI / 5);
	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_value(bar, 0);

	lv_obj_t *label_pct = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_pct, true);
	lv_label_set_text(label_pct, " "SYMBOL_DOT" 0%");
	lv_label_set_style(label_pct, lv_theme_get_current()->label.prim);
	lv_obj_align(label_pct, bar, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 20, 0);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	sd_mount();

	int res = 0;
	char *path = malloc(1024);
	char *txt_buf = malloc(SZ_4K);
	strcpy(path, "switchroot/install/l4t.00");
	u32 path_len = strlen(path) - 2;

	FIL fp;

	res = f_open(&fp, path, FA_READ);
	if (res)
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to open 1st part!");

		goto exit;
	}

	u64 fileSize = (u64)f_size(&fp);

	u32 num = 0;
	u32 pct = 0;
	u32 lba_curr = 0;
	u32 bytesWritten = 0;
	u32 currPartIdx = 0;
	u32 prevPct = 200;
	int retryCount = 0;
	u32 total_size_sct = l4t_flash_ctxt.image_size_sct;

	u8 *buf = (u8 *)MIXD_BUF_ALIGNED;
	DWORD *clmt = f_expand_cltbl(&fp, SZ_4M, 0);

	while (total_size_sct > 0)
	{
		// If we have more than one part, check the size for the split parts and make sure that the bytes written is not more than that.
		if (bytesWritten >= fileSize)
		{
			// If we have more bytes written then close the file pointer and increase the part index we are using
			f_close(&fp);
			free(clmt);
			memset(&fp, 0, sizeof(fp));
			currPartIdx++;

			if (currPartIdx < 10)
			{
				path[path_len] = '0';
				itoa(currPartIdx, &path[path_len + 1], 10);
			}
			else
				itoa(currPartIdx, &path[path_len], 10);

			// Try to open the next file part
			res = f_open(&fp, path, FA_READ);
			if (res)
			{
				s_printf(txt_buf, "#FFDD00 Error:# Failed to open part %d#", currPartIdx);
				lv_label_set_text(lbl_status, txt_buf);
				manual_system_maintenance(true);

				goto exit;
			}
			fileSize = (u64)f_size(&fp);
			bytesWritten = 0;
			clmt = f_expand_cltbl(&fp, SZ_4M, 0);
		}

		retryCount = 0;
		num = MIN(total_size_sct, 8192);

		res = f_read_fast(&fp, buf, num << 9);
		manual_system_maintenance(false);

		if (res)
		{
			lv_label_set_text(lbl_status, "#FFDD00 Error:# Reading from SD!");
			manual_system_maintenance(true);

			f_close(&fp);
			free(clmt);
			goto exit;
		}
		res = !sdmmc_storage_write(&sd_storage, lba_curr + l4t_flash_ctxt.offset_sct, num, buf);

		manual_system_maintenance(false);

		while (res)
		{
			msleep(150);
			manual_system_maintenance(true);

			if (retryCount >= 3)
			{
				lv_label_set_text(lbl_status, "#FFDD00 Error:# Writing to SD!");
				manual_system_maintenance(true);

				f_close(&fp);
				free(clmt);
				goto exit;
			}

			res = !sdmmc_storage_write(&sd_storage, lba_curr + l4t_flash_ctxt.offset_sct, num, buf);
			manual_system_maintenance(false);
		}
		pct = (u64)((u64)lba_curr * 100u) / (u64)l4t_flash_ctxt.image_size_sct;
		if (pct != prevPct)
		{
			lv_bar_set_value(bar, pct);
			s_printf(txt_buf, " #DDDDDD "SYMBOL_DOT"# %d%%", pct);
			lv_label_set_text(label_pct, txt_buf);
			manual_system_maintenance(true);
			prevPct = pct;
		}

		lba_curr += num;
		total_size_sct -= num;
		bytesWritten += num * EMMC_BLOCKSIZE;
	}
	lv_bar_set_value(bar, 100);
	lv_label_set_text(label_pct, " "SYMBOL_DOT" 100%");
	manual_system_maintenance(true);

	// Restore operation ended successfully.
	f_close(&fp);
	free(clmt);

	succeeded = true;

exit:
	free(path);
	free(txt_buf);

	if (!succeeded)
		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	else
		lv_mbox_add_btns(mbox, mbox_btn_map2, _action_delete_linux_installer_files);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

	sd_unmount();

	return LV_RES_INV;
}

static u32 _get_available_l4t_partition()
{
	mbr_t mbr = { 0 };
	gpt_t *gpt = calloc(1, sizeof(gpt_t));

	memset(&l4t_flash_ctxt, 0, sizeof(l4t_flasher_ctxt_t));

	// Read MBR.
	sdmmc_storage_read(&sd_storage, 0, 1, &mbr);

	// Read main GPT.
	sdmmc_storage_read(&sd_storage, 1, sizeof(gpt_t) >> 9, gpt);

	// Search for a suitable partition.
	u32 size_sct = 0;
	if (!memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
	{
		for (u32 i = 0; i < gpt->header.num_part_ents; i++)
		{
			if (!memcmp(gpt->entries[i].name, (char[]) { 'l', 0, '4', 0, 't', 0 }, 6))
			{
				l4t_flash_ctxt.offset_sct = gpt->entries[i].lba_start;
				size_sct = (gpt->entries[i].lba_end + 1) - gpt->entries[i].lba_start;
				break;
			}

			if (i > 126)
				break;
		}
	}
	else
	{
		for (u32 i = 1; i < 4; i++)
		{
			if (mbr.partitions[i].type == 0x83)
			{
				l4t_flash_ctxt.offset_sct = mbr.partitions[i].start_sct;
				size_sct = mbr.partitions[i].size_sct;
				break;
			}
		}
	}

	free(gpt);

	return size_sct;
}

static bool _get_available_android_partition()
{
	gpt_t *gpt = calloc(1, sizeof(gpt_t));

	// Read main GPT.
	sdmmc_storage_read(&sd_storage, 1, sizeof(gpt_t) >> 9, gpt);

	// Check if GPT.
	if (memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
		goto out;

	// Find kernel partition.
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (gpt->entries[i].lba_start && !memcmp(gpt->entries[i].name, (char[]) { 'L', 0, 'N', 0, 'X', 0 }, 6))
		{
			free(gpt);

			return true;
		}

		if (i > 126)
			break;
	}

out:
	free(gpt);

	return false;
}

static lv_res_t _action_check_flash_linux(lv_obj_t *btn)
{
	FILINFO fno;
	char path[128];

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	static const char *mbox_btn_map2[] = { "\222Continue", "\222Cancel", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "#FF8000 Linux Flasher#");

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);
	lv_label_set_text(lbl_status, "#C7EA46 Status:# Searching for files and partitions...");

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	manual_system_maintenance(true);

	sd_mount();

	strcpy(path, "switchroot/install/l4t.00");
	if (f_stat(path, NULL))
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# Installation files not found!");
		goto error;
	}

	u32 size_sct = _get_available_l4t_partition();

	if (!l4t_flash_ctxt.offset_sct || !size_sct || size_sct < 0x800000)
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# No partition found!");
		goto error;
	}

	u32 idx = 0;
	path[23] = 0;
	while (true)
	{
		if (idx < 10)
		{
			path[23] = '0';
			itoa(idx, &path[23 + 1], 10);
		}
		else
			itoa(idx, &path[23], 10);

		// Check for alignment.
		if (f_stat(path, &fno))
			break;

		if ((u64)fno.fsize % SZ_4M)
		{
			// Check if last part.
			idx++;
			if (idx < 10)
			{
				path[23] = '0';
				itoa(idx, &path[23 + 1], 10);
			}
			else
				itoa(idx, &path[23], 10);

			// If not the last part, unaligned size is not permitted.
			if (!f_stat(path, NULL)) // NULL: Don't override current part fs info.
			{
				lv_label_set_text(lbl_status, "#FFDD00 Error:# The image is not aligned to 4 MiB!");
				goto error;
			}

			// Last part. Align size to LBA (512 bytes).
			fno.fsize = ALIGN((u64)fno.fsize, 512);
			idx--;
		}
		l4t_flash_ctxt.image_size_sct += (u64)fno.fsize >> 9;

		idx++;
	}

	if (l4t_flash_ctxt.image_size_sct > size_sct)
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# The image is bigger than the partition!");
		goto error;
	}

	char *txt_buf = malloc(SZ_4K);
	s_printf(txt_buf,
		"#C7EA46 Status:# Found installation files and partition.\n"
		"#00DDFF Offset:# %08x, #00DDFF Size:# %X, #00DDFF Image size:# %d MiB\n"
		"\nDo you want to continue?", l4t_flash_ctxt.offset_sct, size_sct, l4t_flash_ctxt.image_size_sct >> 11);
	lv_label_set_text(lbl_status, txt_buf);
	free(txt_buf);
	lv_mbox_add_btns(mbox, mbox_btn_map2, _action_flash_linux_data);
	goto exit;

error:
	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);

exit:
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

	sd_unmount();

	return LV_RES_OK;
}

static lv_res_t _action_reboot_twrp(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	// Delete parent mbox.
	mbox_action(btns, txt);

	if (!btn_idx)
	{
		PMC(APBDEV_PMC_SCRATCH0) |= PMC_SCRATCH0_MODE_RECOVERY;

		b_cfg->boot_cfg = BOOT_CFG_FROM_ID | BOOT_CFG_AUTOBOOT_EN;

		strcpy((char *)b_cfg->id, "SWANDR");

		void (*main_ptr)() = (void *)nyx_str->hekate;

		sd_end();

		hw_reinit_workaround(false, 0);

		(*main_ptr)();
	}

	return LV_RES_INV;
}

static lv_res_t _action_flash_android_data(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	// Delete parent mbox.
	mbox_action(btns, txt);

	if (btn_idx)
		return LV_RES_INV;

	// Flash Android components.
	char path[128];
	gpt_t *gpt = calloc(1, sizeof(gpt_t));
	char *txt_buf = malloc(SZ_4K);

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	static const char *mbox_btn_map2[] = { "\222Continue", "\222No", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "#FF8000 Android Flasher#");

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);
	lv_label_set_text(lbl_status, "#C7EA46 Status:# Searching for files and partitions...");

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	manual_system_maintenance(true);

	sd_mount();

	// Read main GPT.
	sdmmc_storage_read(&sd_storage, 1, sizeof(gpt_t) >> 9, gpt);

	bool boot_twrp = false;
	if (memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# No Android GPT was found!");
		goto error;
	}

	u32 offset_sct = 0;
	u32 size_sct = 0;

	strcpy(path, "switchroot/install/boot.img");
	if (f_stat(path, NULL))
	{
		s_printf(txt_buf, "#FF8000 Warning:# Kernel image not found!\n");
		goto boot_img_not_found;
	}

	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, (char[]) { 'L', 0, 'N', 0, 'X', 0 }, 6))
		{
			offset_sct = gpt->entries[i].lba_start;
			size_sct = (gpt->entries[i].lba_end + 1) - gpt->entries[i].lba_start;
			break;
		}

		if (i > 126)
			break;
	}

	if (offset_sct && size_sct)
	{
		u32 file_size = 0;
		u8 *buf = sd_file_read(path, &file_size);

		if (file_size % 0x200)
		{
			file_size = ALIGN(file_size, 0x200);
			u8 *buf_tmp = calloc(file_size, 1);
			memcpy(buf_tmp, buf, file_size);
			free(buf);
			buf = buf_tmp;
		}

		if ((file_size >> 9) > size_sct)
			s_printf(txt_buf, "#FF8000 Warning:# Kernel image too big!\n");
		else
		{
			sdmmc_storage_write(&sd_storage, offset_sct, file_size >> 9, buf);

			s_printf(txt_buf, "#C7EA46 Success:# Kernel image flashed!\n");
			f_unlink(path);
		}

		free(buf);
	}
	else
		s_printf(txt_buf, "#FF8000 Warning:# Kernel partition not found!\n");

boot_img_not_found:
	lv_label_set_text(lbl_status, txt_buf);
	manual_system_maintenance(true);

	strcpy(path, "switchroot/install/twrp.img");
	if (f_stat(path, NULL))
	{
		strcat(txt_buf, "#FF8000 Warning:# TWRP image not found!\n");
		goto twrp_not_found;
	}

	offset_sct = 0;
	size_sct = 0;
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, (char[]) { 'S', 0, 'O', 0, 'S', 0 }, 6))
		{
			offset_sct = gpt->entries[i].lba_start;
			size_sct = (gpt->entries[i].lba_end + 1) - gpt->entries[i].lba_start;
			break;
		}

		if (i > 126)
			break;
	}

	if (offset_sct && size_sct)
	{
		u32 file_size = 0;
		u8 *buf = sd_file_read(path, &file_size);

		if (file_size % 0x200)
		{
			file_size = ALIGN(file_size, 0x200);
			u8 *buf_tmp = calloc(file_size, 1);
			memcpy(buf_tmp, buf, file_size);
			free(buf);
			buf = buf_tmp;
		}

		if ((file_size >> 9) > size_sct)
			strcat(txt_buf, "#FF8000 Warning:# TWRP image too big!\n");
		else
		{
			sdmmc_storage_write(&sd_storage, offset_sct, file_size >> 9, buf);
			strcat(txt_buf, "#C7EA46 Success:# TWRP image flashed!\n");
			f_unlink(path);
		}

		free(buf);
	}
	else
		strcat(txt_buf, "#FF8000 Warning:# TWRP partition not found!\n");

twrp_not_found:
	lv_label_set_text(lbl_status, txt_buf);
	manual_system_maintenance(true);

	strcpy(path, "switchroot/install/tegra210-icosa.dtb");
	if (f_stat(path, NULL))
	{
		strcat(txt_buf, "#FF8000 Warning:# DTB image not found!");

		goto dtb_not_found;
	}

	offset_sct = 0;
	size_sct = 0;
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, (char[]) { 'D', 0, 'T', 0, 'B', 0 }, 6))
		{
			offset_sct = gpt->entries[i].lba_start;
			size_sct = (gpt->entries[i].lba_end + 1) - gpt->entries[i].lba_start;
			break;
		}

		if (i > 126)
			break;
	}

	if (offset_sct && size_sct)
	{
		u32 file_size = 0;
		u8 *buf = sd_file_read(path, &file_size);

		if (file_size % 0x200)
		{
			file_size = ALIGN(file_size, 0x200);
			u8 *buf_tmp = calloc(file_size, 1);
			memcpy(buf_tmp, buf, file_size);
			free(buf);
			buf = buf_tmp;
		}

		if ((file_size >> 9) > size_sct)
			strcat(txt_buf, "#FF8000 Warning:# DTB image too big!");
		else
		{
			sdmmc_storage_write(&sd_storage, offset_sct, file_size >> 9, buf);
			strcat(txt_buf, "#C7EA46 Success:# DTB image flashed!");
			f_unlink(path);
		}

		free(buf);
	}
	else
		strcat(txt_buf, "#FF8000 Warning:# DTB partition not found!");

dtb_not_found:
	lv_label_set_text(lbl_status, txt_buf);

	// Check if TWRP is flashed unconditionally.
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, (char[]) { 'S', 0, 'O', 0, 'S', 0 }, 6))
		{
			u8 *buf = malloc(512);
			sdmmc_storage_read(&sd_storage, gpt->entries[i].lba_start, 1, buf);
			if (!memcmp(buf, "ANDROID", 7))
				boot_twrp = true;
			free(buf);
			break;
		}

		if (i > 126)
			break;
	}

error:
	if (boot_twrp)
	{
		strcat(txt_buf,"\n\nDo you want to reboot into TWRP\nto finish Android installation?");
		lv_label_set_text(lbl_status, txt_buf);
		lv_mbox_add_btns(mbox, mbox_btn_map2, _action_reboot_twrp);
	}
	else
		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

	free(txt_buf);
	free(gpt);

	sd_unmount();

	return LV_RES_INV;
}

static lv_res_t _action_flash_android(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\222Continue", "\222Cancel", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "#FF8000 Android Flasher#");

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);
	lv_label_set_text(lbl_status,
		"This will flash #C7EA46 Kernel#, #C7EA46 DTB# and #C7EA46 TWRP# if found.\n"
		"Do you want to continue?");

	lv_mbox_add_btns(mbox, mbox_btn_map,  _action_flash_android_data);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _action_part_manager_flash_options0(lv_obj_t *btns, const char *txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	switch (btn_idx)
	{
	case 0:
		action_ums_sd(btns);
		lv_obj_del(ums_mbox);
		break;
	case 1:
		_action_check_flash_linux(btns);
		break;
	case 2:
		_action_flash_android(btns);
		break;
	case 3:
		mbox_action(btns, txt);
		return LV_RES_INV;
	}

	return LV_RES_OK;
}

static lv_res_t _action_part_manager_flash_options1(lv_obj_t *btns, const char *txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	switch (btn_idx)
	{
	case 0:
		action_ums_sd(btns);
		lv_obj_del(ums_mbox);
		break;
	case 1:
		mbox_action(btns, txt);
		_action_check_flash_linux(NULL);
		return LV_RES_INV;
	case 2:
		mbox_action(btns, txt);
		return LV_RES_INV;
	}

	return LV_RES_OK;
}

static lv_res_t _action_part_manager_flash_options2(lv_obj_t *btns, const char *txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	switch (btn_idx)
	{
	case 0:
		action_ums_sd(btns);
		lv_obj_del(ums_mbox);
		break;
	case 1:
		mbox_action(btns, txt);
		_action_flash_android(NULL);
		return LV_RES_INV;
	case 2:
		mbox_action(btns, txt);
		return LV_RES_INV;
	}

	return LV_RES_OK;
}

static lv_res_t _create_mbox_start_partitioning(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] =  { "\211", "\222OK", "\211", "" };
	static const char *mbox_btn_map1[] = { "\222SD UMS", "\222Flash Linux", "\222Flash Android", "\221OK", "" };
	static const char *mbox_btn_map2[] = { "\222SD UMS", "\222Flash Linux", "\221OK", "" };
	static const char *mbox_btn_map3[] = { "\222SD UMS", "\222Flash Android", "\221OK", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "#FF8000 Partition Manager#");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	bool buttons_set = false;

	if (!part_info.backup_possible)
	{
		char *txt_buf = malloc(SZ_4K);
		strcpy(txt_buf, "#FF8000 Partition Manager#\n\nSafety wait ends in ");
		lv_mbox_set_text(mbox, txt_buf);

		u32 seconds = 5;
		u32 text_idx = strlen(txt_buf);
		while (seconds)
		{
			s_printf(txt_buf + text_idx, "%d seconds...", seconds);
			lv_mbox_set_text(mbox, txt_buf);
			manual_system_maintenance(true);
			msleep(1000);
			seconds--;
		}

		lv_mbox_set_text(mbox,
			"#FF8000 Partition Manager#\n\n"
			"#FFDD00 Warning: Do you really want to continue?!#\n\n"
			"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.");
		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		manual_system_maintenance(true);

		free(txt_buf);

		if (!(btn_wait() & BTN_POWER))
			goto exit;
	}

	lv_mbox_set_text(mbox, "#FF8000 Partition Manager#");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	manual_system_maintenance(true);

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);

	lv_obj_t *lbl_paths[2];

	lbl_paths[0] = lv_label_create(mbox, NULL);
	lv_label_set_text(lbl_paths[0], "/");
	lv_label_set_long_mode(lbl_paths[0], LV_LABEL_LONG_DOT);
	lv_cont_set_fit(lbl_paths[0], false, true);
	lv_obj_set_width(lbl_paths[0], (LV_HOR_RES / 9 * 6) - LV_DPI / 2);
	lv_label_set_align(lbl_paths[0], LV_LABEL_ALIGN_CENTER);
	lbl_paths[1] = lv_label_create(mbox, NULL);
	lv_label_set_text(lbl_paths[1], " ");
	lv_label_set_long_mode(lbl_paths[1], LV_LABEL_LONG_DOT);
	lv_cont_set_fit(lbl_paths[1], false, true);
	lv_obj_set_width(lbl_paths[1], (LV_HOR_RES / 9 * 6) - LV_DPI / 2);
	lv_label_set_align(lbl_paths[1], LV_LABEL_ALIGN_CENTER);

	sd_mount();

	FATFS ram_fs;
	char *path = malloc(1024);
	u32 total_files = 0;
	u32 total_size = 0;

	// Read current MBR.
	sdmmc_storage_read(&sd_storage, 0, 1, &part_info.mbr_old);

	lv_label_set_text(lbl_status, "#00DDFF Status:# Initializing Ramdisk...");
	lv_label_set_text(lbl_paths[0], "Please wait...");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	manual_system_maintenance(true);
	if (ram_disk_init(&ram_fs, RAM_DISK_SZ))
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to initialize Ramdisk!");
		goto error;
	}

	if (!part_info.backup_possible)
	{
		strcpy(path, "bootloader");
		f_chdrive("ram:");
		f_mkdir(path);
	}
	else
		path[0] = 0;

	lv_label_set_text(lbl_status, "#00DDFF Status:# Backing up files...");
	manual_system_maintenance(true);
	if (_backup_and_restore_files(path, &total_files, &total_size, "ram:", "sd:", lbl_paths))
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to back up files!");
		goto error;
	}
	total_files = 0;
	total_size = 0;

	if (!part_info.backup_possible)
		strcpy(path, "bootloader");
	else
		path[0] = 0;

	f_mount(NULL, "sd:", 1); // Unmount SD card.

	lv_label_set_text(lbl_status, "#00DDFF Status:# Formatting FAT32 partition...");
	lv_label_set_text(lbl_paths[0], "Please wait...");
	lv_label_set_text(lbl_paths[1], " ");
	manual_system_maintenance(true);

	// Set reserved size.
	u32 part_rsvd_size = (part_info.emu_size << 11) + (part_info.l4t_size << 11) + (part_info.and_size << 11);
	part_rsvd_size += part_rsvd_size ? part_info.alignment : 0; // Do not reserve alignment space if no extra partitions.
	disk_set_info(DRIVE_SD, SET_SECTOR_COUNT, &part_rsvd_size);
	u8 *buf = malloc(SZ_4M);

	u32 cluster_size = 65536;
	u32 mkfs_error = f_mkfs("sd:", FM_FAT32, cluster_size, buf, SZ_4M);

	if (!mkfs_error)
		goto mkfs_no_error;

	// Retry by halving cluster size.
	while (cluster_size > 4096)
	{
		cluster_size /= 2;
		mkfs_error = f_mkfs("sd:", FM_FAT32, cluster_size, buf, SZ_4M);

		if (!mkfs_error)
			break;
	}

	if (mkfs_error)
	{
		// Failed to format.
		s_printf((char *)buf, "#FFDD00 Error:# Failed to format disk (%d)!\n\n"
			"Remove the SD card and check that is OK.\nIf not, format it, reinsert it and\npress #FF8000 POWER#!", mkfs_error);

		lv_label_set_text(lbl_status, (char *)buf);
		lv_label_set_text(lbl_paths[0], " ");
		manual_system_maintenance(true);

		sd_end();

		while (!(btn_wait() & BTN_POWER));

		sd_mount();

		if (!part_info.backup_possible)
		{
			f_chdrive("sd:");
			f_mkdir(path);
		}

		lv_label_set_text(lbl_status, "#00DDFF Status:# Restoring files...");
		manual_system_maintenance(true);
		if (_backup_and_restore_files(path, &total_files, &total_size, "sd:", "ram:", NULL))
		{
			lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to restore files!");
			free(buf);
			goto error;
		}

		lv_label_set_text(lbl_status, "#00DDFF Status:# Restored files but the operation failed!");
		f_mount(NULL, "ram:", 1); // Unmount ramdisk.
		free(buf);
		goto error;
	}

mkfs_no_error:
	free(buf);

	f_mount(&sd_fs, "sd:", 1); // Mount SD card.

	if (!part_info.backup_possible)
	{
		f_chdrive("sd:");
		f_mkdir(path);
	}

	lv_label_set_text(lbl_status, "#00DDFF Status:# Restoring files...");
	manual_system_maintenance(true);
	if (_backup_and_restore_files(path, &total_files, &total_size, "sd:", "ram:", lbl_paths))
	{
		total_files = 0;
		total_size = 0;

		if (!part_info.backup_possible)
		{
			strcpy(path, "bootloader");
			f_chdrive("sd:");
			f_mkdir(path);
		}
		else
			path[0] = 0;

		if (_backup_and_restore_files(path, &total_files, &total_size, "sd:", "ram:", NULL))
		{
			lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to restore files!");
			goto error;
		}
	}

	f_mount(NULL, "ram:", 1); // Unmount ramdisk.
	f_chdrive("sd:");

	// Set Volume label.
	f_setlabel("0:SWITCH SD");

	lv_label_set_text(lbl_status, "#00DDFF Status:# Flashing partition table...");
	lv_label_set_text(lbl_paths[0], "Please wait...");
	lv_label_set_text(lbl_paths[1], " ");
	manual_system_maintenance(true);
	_prepare_and_flash_mbr_gpt();

	// Enable/Disable buttons depending on partition layout.
	if (part_info.l4t_size)
	{
		lv_obj_set_click(btn_flash_l4t, true);
		lv_btn_set_state(btn_flash_l4t, LV_BTN_STATE_REL);
	}
	else
	{
		lv_obj_set_click(btn_flash_l4t, false);
		lv_btn_set_state(btn_flash_l4t, LV_BTN_STATE_INA);
	}

	// Enable/Disable buttons depending on partition layout.
	if (part_info.and_size)
	{
		lv_obj_set_click(btn_flash_android, true);
		lv_btn_set_state(btn_flash_android, LV_BTN_STATE_REL);
	}
	else
	{
		lv_obj_set_click(btn_flash_android, false);
		lv_btn_set_state(btn_flash_android, LV_BTN_STATE_INA);
	}

	sd_unmount();
	lv_label_set_text(lbl_status, "#00DDFF Status:# Done!");
	manual_system_maintenance(true);

	if (part_info.l4t_size && part_info.and_size)
		lv_mbox_add_btns(mbox, mbox_btn_map1, _action_part_manager_flash_options0);
	else if (part_info.l4t_size)
		lv_mbox_add_btns(mbox, mbox_btn_map2, _action_part_manager_flash_options1);
	else if (part_info.and_size)
		lv_mbox_add_btns(mbox, mbox_btn_map3, _action_part_manager_flash_options2);

	if (part_info.l4t_size || part_info.and_size)
		buttons_set = true;

	goto out;

error:
	f_chdrive("sd:");
	free(path);

out:
	lv_obj_del(lbl_paths[0]);
	lv_obj_del(lbl_paths[1]);
exit:
	if (!buttons_set)
		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	// Disable partitioning button.
	if (btn)
		lv_btn_set_state(btn, LV_BTN_STATE_INA);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_partitioning_option0(lv_obj_t *btns, const char *txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	switch (btn_idx)
	{
	case 0:
		action_ums_sd(btns);
		return LV_RES_OK;
	case 1:
		mbox_action(btns, txt);
		_create_mbox_start_partitioning(NULL);
		break;
	case 2:
		mbox_action(btns, txt);
		break;
	}

	return LV_RES_INV;
}

static lv_res_t _create_mbox_partitioning_option1(lv_obj_t *btns, const char *txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	mbox_action(btns, txt);

	if (!btn_idx)
	{
		mbox_action(btns, txt);
		_create_mbox_start_partitioning(NULL);
		return LV_RES_INV;
	}

	return LV_RES_OK;
}

static lv_res_t _create_mbox_partitioning_next(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\222SD UMS", "\222Start", "\222Cancel", "" };
	static const char *mbox_btn_map2[] = { "\222Start", "\222Cancel", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	char *txt_buf = malloc(SZ_4K);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);
	lv_mbox_set_text(mbox, "#FF8000 Partition Manager#");

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);

	s_printf(txt_buf, "#FFDD00 Warning: This will partition the SD Card!#\n\n");

	if (part_info.backup_possible)
	{
		strcat(txt_buf, "#C7EA46 Your files will be backed up and restored!#\n"
			"#FFDD00 Any other partition will be wiped!#");
	}
	else
	{
		strcat(txt_buf, "#FFDD00 Your files will be wiped!#\n"
			"#FFDD00 Any other partition will be also wiped!#\n"
			"#FFDD00 Use USB UMS to copy them over!#");
	}

	lv_label_set_text(lbl_status, txt_buf);

	if (part_info.backup_possible)
		lv_mbox_add_btns(mbox, mbox_btn_map2, _create_mbox_partitioning_option1);
	else
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_mbox_partitioning_option0);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	free(txt_buf);

	return LV_RES_OK;
}

static void _update_partition_bar()
{
	lv_obj_t *h1 = lv_obj_get_parent(part_info.bar_hos);
	u32 total_size = (part_info.total_sct - 0x8000) / 0x200000;
	u32 bar_hos_size = lv_obj_get_width(h1) * (part_info.hos_size >> 10) / total_size;
	u32 bar_emu_size = lv_obj_get_width(h1) * (part_info.emu_size >> 10) / total_size;
	u32 bar_l4t_size = lv_obj_get_width(h1) * (part_info.l4t_size >> 10) / total_size;
	u32 bar_and_size = lv_obj_get_width(h1) * (part_info.and_size >> 10) / total_size;

	lv_obj_set_size(part_info.bar_hos, bar_hos_size, LV_DPI / 2);
	lv_obj_set_size(part_info.bar_emu, bar_emu_size, LV_DPI / 2);
	lv_obj_set_size(part_info.bar_l4t, bar_l4t_size, LV_DPI / 2);
	lv_obj_set_size(part_info.bar_and, bar_and_size, LV_DPI / 2);

	lv_obj_align(part_info.bar_emu, part_info.bar_hos, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	lv_obj_align(part_info.bar_l4t, part_info.bar_emu, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	lv_obj_align(part_info.bar_and, part_info.bar_l4t, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_set_size(part_info.sep_emu, bar_emu_size ? 8 : 0, LV_DPI / 2);
	lv_obj_align(part_info.sep_emu, part_info.bar_hos, LV_ALIGN_OUT_RIGHT_MID, -4, 0);

	lv_obj_set_size(part_info.sep_l4t, bar_l4t_size ? 8 : 0, LV_DPI / 2);
	lv_obj_align(part_info.sep_l4t, part_info.bar_emu, LV_ALIGN_OUT_RIGHT_MID, -4, 0);

	lv_obj_set_size(part_info.sep_and, bar_and_size ? 8 : 0, LV_DPI / 2);
	lv_obj_align(part_info.sep_and, part_info.bar_l4t, LV_ALIGN_OUT_RIGHT_MID, -4, 0);
}

static lv_res_t _action_slider_emu(lv_obj_t *slider)
{
	#define EMUMMC_32GB_FULL 29856
	#define EMUMMC_64GB_FULL (59664 + 1) // 1MB extra for backup GPT.

	const u32 rsvd_mb = 4 + 4 + 16 + 8; // BOOT0 + BOOT1 + 16MB offset + 8MB alignment.
	u32 size;
	char lbl_text[64];
	bool prev_emu_double = part_info.emu_double;
	int slide_val = lv_slider_get_value(slider);
	u32 max_emmc_size = !part_info.emmc_is_64gb ? EMUMMC_32GB_FULL : EMUMMC_64GB_FULL;

	part_info.emu_double = false;

	size  = (slide_val > 10 ? (slide_val - 10) : slide_val) + 3; // Min 4GB.
	size *= 1024;    // Convert to GB.
	size += rsvd_mb; // Add reserved size.

	if (!slide_val)
		size = 0; // Reset if 0.
	else if (slide_val >= 11)
	{
		size *= 2;
		part_info.emu_double = true;
	}

	// Handle special cases. 2nd value is for 64GB Aula.
	if (slide_val == 10)
		size = max_emmc_size;
	else if (slide_val == 20)
		size = 2 * max_emmc_size;

	s32 hos_size = (part_info.total_sct >> 11) - 16 - size - part_info.l4t_size - part_info.and_size;
	if (hos_size > 2048)
	{
		part_info.emu_size = size;
		part_info.hos_size = hos_size;

		s_printf(lbl_text, "#96FF00 %d GiB#", hos_size >> 10);
		lv_label_set_text(part_info.lbl_hos, lbl_text);
		lv_bar_set_value(part_info.slider_bar_hos, hos_size >> 10);

		if (!part_info.emu_double)
		{
			if (slide_val != 10)
				s_printf(lbl_text, "#FF3C28 %d GiB#", size >> 10);
			else
				s_printf(lbl_text, "#FF3C28 %d FULL#", size >> 10);
		}
		else
			s_printf(lbl_text, "#FFDD00 2x##FF3C28 %d#", size >> 11);
		lv_label_set_text(part_info.lbl_emu, lbl_text);
	}
	else
	{
		u32 emu_size = part_info.emu_size;

		if (emu_size == max_emmc_size)
			emu_size = 10;
		else if (emu_size == 2 * max_emmc_size)
			emu_size = 20;
		else if (emu_size)
		{
			if (prev_emu_double)
				emu_size /= 2;
			emu_size -= rsvd_mb;
			emu_size /= 1024;
			emu_size -= 3;

			if (prev_emu_double)
				emu_size += 11;
		}

		int new_slider_val = emu_size;
		part_info.emu_double = prev_emu_double ? true : false;

		lv_slider_set_value(slider, new_slider_val);
	}

	_update_partition_bar();

	return LV_RES_OK;
}

static lv_res_t _action_slider_l4t(lv_obj_t *slider)
{
	char lbl_text[64];

	u32 size = (u32)lv_slider_get_value(slider) << 10;
	if (size < 4096)
		size = 0;
	else if (size < 8192)
		size = 8192;

	s32 hos_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - size - part_info.and_size;

	if (hos_size > 2048)
	{
		if (size <= 8192)
			lv_slider_set_value(slider, size >> 10);
	}
	else
	{
		size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.and_size - 2048;
		hos_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.and_size - size;
		if (hos_size < 2048 || size < 8192)
		{
			lv_slider_set_value(slider, part_info.l4t_size >> 10);
			goto out;
		}
		lv_slider_set_value(slider, size >> 10);
	}

	part_info.l4t_size = size;
	part_info.hos_size = hos_size;

	s_printf(lbl_text, "#96FF00 %d GiB#", hos_size >> 10);
	lv_label_set_text(part_info.lbl_hos, lbl_text);
	lv_bar_set_value(part_info.slider_bar_hos, hos_size >> 10);
	s_printf(lbl_text, "#00DDFF %d GiB#", size >> 10);
	lv_label_set_text(part_info.lbl_l4t, lbl_text);

	_update_partition_bar();

out:
	return LV_RES_OK;
}

static lv_res_t _action_slider_and(lv_obj_t *slider)
{
	char lbl_text[64];

	u32 user_size = (u32)lv_slider_get_value(slider) << 10;
	if (user_size < 2048)
		user_size = 0;
	else if (user_size < 4096)
		user_size = 4096;

	u32 and_size = user_size ? (user_size + 4096) : 0; // Add Android reserved partitions size.
	s32 hos_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.l4t_size - and_size;

	if (hos_size > 2048)
	{
		if (user_size <= 4096)
			lv_slider_set_value(slider, user_size >> 10);
	}
	else
	{
		and_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.l4t_size - 2048;
		hos_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.l4t_size - and_size;
		if (hos_size < 2048 || and_size < 8192)
		{
			lv_slider_set_value(slider, part_info.and_size >> 10);
			goto out;
		}
		user_size = and_size - 4096;
		lv_slider_set_value(slider, user_size >> 10);
	}

	part_info.and_size = and_size;
	part_info.hos_size = hos_size;

	s_printf(lbl_text, "#96FF00 %d GiB#", hos_size >> 10);
	lv_label_set_text(part_info.lbl_hos, lbl_text);
	lv_bar_set_value(part_info.slider_bar_hos, hos_size >> 10);
	s_printf(lbl_text, "#FF8000 %d GiB#", user_size >> 10);
	lv_label_set_text(part_info.lbl_and, lbl_text);

	_update_partition_bar();

out:
	return LV_RES_OK;
}

static void create_mbox_check_files_total_size()
{
	static lv_style_t bar_hos_ind, bar_emu_ind, bar_l4t_ind, bar_and_ind;
	static lv_style_t sep_emu_bg, sep_l4t_bg, sep_and_bg;

	lv_style_copy(&bar_hos_ind, lv_theme_get_current()->bar.indic);
	bar_hos_ind.body.main_color = LV_COLOR_HEX(0x96FF00);
	bar_hos_ind.body.grad_color = bar_hos_ind.body.main_color;

	lv_style_copy(&bar_emu_ind, lv_theme_get_current()->bar.indic);
	bar_emu_ind.body.main_color = LV_COLOR_HEX(0xFF3C28);
	bar_emu_ind.body.grad_color = bar_emu_ind.body.main_color;

	lv_style_copy(&bar_l4t_ind, lv_theme_get_current()->bar.indic);
	bar_l4t_ind.body.main_color = LV_COLOR_HEX(0x00DDFF);
	bar_l4t_ind.body.grad_color = bar_l4t_ind.body.main_color;

	lv_style_copy(&bar_and_ind, lv_theme_get_current()->bar.indic);
	bar_and_ind.body.main_color = LV_COLOR_HEX(0xFF8000);
	bar_and_ind.body.grad_color = bar_and_ind.body.main_color;

	lv_style_copy(&sep_emu_bg, lv_theme_get_current()->cont);
	sep_emu_bg.body.main_color = LV_COLOR_HEX(0xFF3C28);
	sep_emu_bg.body.grad_color = sep_emu_bg.body.main_color;
	sep_emu_bg.body.radius = 0;
	lv_style_copy(&sep_l4t_bg, &sep_emu_bg);
	sep_l4t_bg.body.main_color = LV_COLOR_HEX(0x00DDFF);
	sep_l4t_bg.body.grad_color = sep_l4t_bg.body.main_color;
	lv_style_copy(&sep_and_bg, &sep_emu_bg);
	sep_and_bg.body.main_color = LV_COLOR_HEX(0xFF8000);
	sep_and_bg.body.grad_color = sep_and_bg.body.main_color;

	char *txt_buf = malloc(SZ_8K);

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "Analyzing SD card usage. This might take a while...");

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);
	manual_system_maintenance(true);

	char *path = malloc(1024);
	u32 total_files = 0;
	u32 total_size = 0;
	path[0] = 0;

	// Check total size of files.
	int res = _backup_and_restore_files(path, &total_files, &total_size, NULL, NULL, NULL);

	// Not more than 1.0GB.
	part_info.backup_possible = !res && !(total_size > (RAM_DISK_SZ - SZ_16M)); // 0x2400000

	if (part_info.backup_possible)
	{
		s_printf(txt_buf,
			"#96FF00 The SD Card files will be backed up automatically!#\n"
			"#FFDD00 Any other partition will be wiped!#\n"
			"#00DDFF Total files:# %d, #00DDFF Total size:# %d MiB", total_files, total_size >> 20);
		lv_mbox_set_text(mbox, txt_buf);
	}
	else
	{
		lv_mbox_set_text(mbox,
			"#FFDD00 The SD Card cannot be backed up!#\n"
			"#FFDD00 Any other partition will be also wiped!#\n\n"
			"You will be asked to back up your files later via UMS.");
	}

	// Create container to keep content inside.
	lv_obj_t *h1 = lv_cont_create(mbox, NULL);
	lv_cont_set_fit(h1, false, true);
	lv_cont_set_style(h1, &lv_style_transp_tight);
	lv_obj_set_width(h1, lv_obj_get_width(mbox) - LV_DPI * 3);

	lv_obj_t *lbl_part = lv_label_create(h1, NULL);
	lv_label_set_recolor(lbl_part, true);
	lv_label_set_text(lbl_part, "#00DDFF Current MBR partition layout:#");

	// Read current MBR.
	mbr_t mbr = { 0 };
	sdmmc_storage_read(&sd_storage, 0, 1, &mbr);

	total_size = (sd_storage.sec_cnt - 0x8000) / 0x200000;
	u32 bar_hos_size = lv_obj_get_width(h1) * (mbr.partitions[0].size_sct / 0x200000) / total_size;
	u32 bar_emu_size = 0;
	for (u32 i = 1; i < 4; i++)
		if (mbr.partitions[i].type == 0xE0)
			bar_emu_size += mbr.partitions[i].size_sct;
	bar_emu_size = lv_obj_get_width(h1) * (bar_emu_size / 0x200000) / total_size;

	u32 bar_l4t_size = 0;
	for (u32 i = 1; i < 4; i++)
		if (mbr.partitions[i].type == 0x83)
			bar_l4t_size += mbr.partitions[i].size_sct;
	bar_l4t_size = lv_obj_get_width(h1) * (bar_l4t_size / 0x200000) / total_size;

	u32 bar_and_size = lv_obj_get_width(h1) - bar_hos_size - bar_emu_size - bar_l4t_size;

	// Create bar objects.
	lv_obj_t *bar_mbr_hos = lv_bar_create(h1, NULL);
	lv_obj_set_size(bar_mbr_hos, bar_hos_size, LV_DPI / 3);
	lv_bar_set_range(bar_mbr_hos, 0, 1);
	lv_bar_set_value(bar_mbr_hos, 1);
	lv_bar_set_style(bar_mbr_hos, LV_BAR_STYLE_INDIC, &bar_hos_ind);
	lv_obj_align(bar_mbr_hos, lbl_part, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 6);

	lv_obj_t *bar_mbr_emu = lv_bar_create(h1, bar_mbr_hos);
	lv_obj_set_size(bar_mbr_emu, bar_emu_size, LV_DPI / 3);
	lv_bar_set_style(bar_mbr_emu, LV_BAR_STYLE_INDIC, &bar_emu_ind);
	lv_obj_align(bar_mbr_emu, bar_mbr_hos, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_t *bar_mbr_l4t = lv_bar_create(h1, bar_mbr_hos);
	lv_obj_set_size(bar_mbr_l4t, bar_l4t_size, LV_DPI / 3);
	lv_bar_set_style(bar_mbr_l4t, LV_BAR_STYLE_INDIC, &bar_l4t_ind);
	lv_obj_align(bar_mbr_l4t, bar_mbr_emu, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	lv_obj_t *bar_mbr_rest = lv_bar_create(h1, bar_mbr_hos);
	lv_obj_set_size(bar_mbr_rest, bar_and_size > 1 ? bar_and_size : 0, LV_DPI / 3);
	lv_bar_set_style(bar_mbr_rest, LV_BAR_STYLE_INDIC, &bar_and_ind);
	lv_obj_align(bar_mbr_rest, bar_mbr_l4t, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	// Create separator objects.
	lv_obj_t *sep_mbr_emu = lv_cont_create(h1, NULL);
	lv_obj_set_size(sep_mbr_emu, bar_emu_size ? 8 : 0, LV_DPI / 3);
	lv_obj_set_style(sep_mbr_emu, &sep_emu_bg);
	lv_obj_align(sep_mbr_emu, bar_mbr_hos, LV_ALIGN_OUT_RIGHT_MID, -4, 0);

	lv_obj_t *sep_mbr_l4t = lv_cont_create(h1, sep_mbr_emu);
	lv_obj_set_size(sep_mbr_l4t, bar_l4t_size ? 8 : 0, LV_DPI / 3);
	lv_obj_set_style(sep_mbr_l4t, &sep_l4t_bg);
	lv_obj_align(sep_mbr_l4t, bar_mbr_emu, LV_ALIGN_OUT_RIGHT_MID, -4, 0);

	lv_obj_t *sep_mbr_rest = lv_cont_create(h1, sep_mbr_emu);
	lv_obj_set_size(sep_mbr_rest, bar_and_size ? (bar_and_size > 1 ? 8 : 0) : 0, LV_DPI / 3);
	lv_obj_set_style(sep_mbr_rest, &sep_and_bg);
	lv_obj_align(sep_mbr_rest, bar_mbr_l4t, LV_ALIGN_OUT_RIGHT_MID, -4, 0);

	// Print partition table info.
	s_printf(txt_buf,
		"Partition 0 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 1 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 2 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 3 - Type: %02x, Start: %08x, Size: %08x",
		mbr.partitions[0].type, mbr.partitions[0].start_sct, mbr.partitions[0].size_sct,
		mbr.partitions[1].type, mbr.partitions[1].start_sct, mbr.partitions[1].size_sct,
		mbr.partitions[2].type, mbr.partitions[2].start_sct, mbr.partitions[2].size_sct,
		mbr.partitions[3].type, mbr.partitions[3].start_sct, mbr.partitions[3].size_sct);

	lv_obj_t *lbl_table = lv_label_create(h1, NULL);
	lv_label_set_style(lbl_table, &monospace_text);
	lv_label_set_text(lbl_table, txt_buf);
	lv_obj_align(lbl_table, h1, LV_ALIGN_IN_TOP_MID, 0, LV_DPI);

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

	free(txt_buf);
	free(path);
}

static lv_res_t _action_fix_mbr(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\211", "\222OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);
	lv_mbox_set_text(mbox, "#FF8000 Fix Hybrid MBR#");

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);

	// Try to init sd card. No need for valid MBR.
	if (!sd_mount() && !sd_get_card_initialized())
	{
		lv_label_set_text(lbl_status, "#FFDD00 Failed to init SD!#");
		goto out;
	}

	mbr_t mbr[2] = { 0 };
	gpt_t *gpt = calloc(1, sizeof(gpt_t));

	sdmmc_storage_read(&sd_storage, 0, 1, &mbr[0]);
	sdmmc_storage_read(&sd_storage, 1, sizeof(gpt_t) >> 9, gpt);

	memcpy(&mbr[1], &mbr[0], sizeof(mbr_t));

	sd_unmount();

	if (memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
	{
		lv_label_set_text(lbl_status, "#FFDD00 Warning:# No valid GPT was found!");
		goto out;
	}

	// Parse GPT.
	LIST_INIT(gpt_parsed);
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		emmc_part_t *part = (emmc_part_t *)calloc(sizeof(emmc_part_t), 1);

		if (gpt->entries[i].lba_start < gpt->header.first_use_lba)
			continue;

		part->index = i;
		part->lba_start = gpt->entries[i].lba_start;
		part->lba_end = gpt->entries[i].lba_end;

		// ASCII conversion. Copy only the LSByte of the UTF-16LE name.
		for (u32 j = 0; j < 36; j++)
			part->name[j] = gpt->entries[i].name[j];
		part->name[35] = 0;

		list_append(&gpt_parsed, &part->link);
	}
	free(gpt);

	// Set FAT and emuMMC partitions.
	u32 mbr_idx = 1;
	bool found_hos_data = false;
	LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt_parsed, link)
	{
		// FatFS simple GPT found a fat partition, set it.
		if (sd_fs.part_type && !part->index)
		{
			mbr[1].partitions[0].type = sd_fs.fs_type == FS_EXFAT ? 0x7 : 0xC;
			mbr[1].partitions[0].start_sct = part->lba_start;
			mbr[1].partitions[0].size_sct = (part->lba_end - part->lba_start + 1);
		}

		// FatFS simple GPT didn't find a fat partition as the first one.
		if (!sd_fs.part_type && !found_hos_data && !strcmp(part->name, "hos_data"))
		{
			mbr[1].partitions[0].type = 0xC;
			mbr[1].partitions[0].start_sct = part->lba_start;
			mbr[1].partitions[0].size_sct = (part->lba_end - part->lba_start + 1);
			found_hos_data = true;
		}

		// Set up to max 2 emuMMC partitions.
		if (!strcmp(part->name, "emummc") || !strcmp(part->name, "emummc2"))
		{
			mbr[1].partitions[mbr_idx].type = 0xE0;
			mbr[1].partitions[mbr_idx].start_sct = part->lba_start;
			mbr[1].partitions[mbr_idx].size_sct = (part->lba_end - part->lba_start + 1);
			mbr_idx++;
		}

		// Total reached last slot.
		if (mbr_idx >= 3)
			break;
	}

	emmc_gpt_free(&gpt_parsed);

	// Set GPT protective partition.
	mbr[1].partitions[mbr_idx].type = 0xEE;
	mbr[1].partitions[mbr_idx].start_sct = 1;
	mbr[1].partitions[mbr_idx].size_sct = sd_storage.sec_cnt - 1;

	// Check for differences.
	bool changed = false;
	for (u32 i = 1; i < 4; i++)
	{
		if ((mbr[0].partitions[i].type      != mbr[1].partitions[i].type)      ||
			(mbr[0].partitions[i].start_sct != mbr[1].partitions[i].start_sct) ||
			(mbr[0].partitions[i].size_sct  != mbr[1].partitions[i].size_sct))
		{
			changed = true;
			break;
		}
	}

	if (!changed)
	{
		lv_label_set_text(lbl_status, "#96FF00 Warning:# The Hybrid MBR needs no change!#");
		goto out;
	}

	char *txt_buf = malloc(SZ_16K);

	s_printf(txt_buf, "#00DDFF Current MBR Layout:#\n");
	s_printf(txt_buf + strlen(txt_buf),
		"Partition 0 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 1 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 2 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 3 - Type: %02x, Start: %08x, Size: %08x\n\n",
		mbr[0].partitions[0].type, mbr[0].partitions[0].start_sct, mbr[0].partitions[0].size_sct,
		mbr[0].partitions[1].type, mbr[0].partitions[1].start_sct, mbr[0].partitions[1].size_sct,
		mbr[0].partitions[2].type, mbr[0].partitions[2].start_sct, mbr[0].partitions[2].size_sct,
		mbr[0].partitions[3].type, mbr[0].partitions[3].start_sct, mbr[0].partitions[3].size_sct);

	s_printf(txt_buf + strlen(txt_buf), "#00DDFF New MBR Layout:#\n");
	s_printf(txt_buf + strlen(txt_buf),
		"Partition 0 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 1 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 2 - Type: %02x, Start: %08x, Size: %08x\n"
		"Partition 3 - Type: %02x, Start: %08x, Size: %08x",
		mbr[1].partitions[0].type, mbr[1].partitions[0].start_sct, mbr[1].partitions[0].size_sct,
		mbr[1].partitions[1].type, mbr[1].partitions[1].start_sct, mbr[1].partitions[1].size_sct,
		mbr[1].partitions[2].type, mbr[1].partitions[2].start_sct, mbr[1].partitions[2].size_sct,
		mbr[1].partitions[3].type, mbr[1].partitions[3].start_sct, mbr[1].partitions[3].size_sct);

	lv_label_set_text(lbl_status, txt_buf);
	lv_label_set_style(lbl_status, &monospace_text);

	free(txt_buf);

	lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);
	lv_label_set_align(lbl_status, LV_LABEL_ALIGN_CENTER);

	lv_label_set_text(lbl_status,
		"#FF8000 Warning: Do you really want to continue?!#\n\n"
		"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.");

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	manual_system_maintenance(true);

	if (btn_wait() & BTN_POWER)
	{
		// Write MBR.
		sd_mount();
		sdmmc_storage_write(&sd_storage, 0, 1, &mbr[1]);
		sd_unmount();
		lv_label_set_text(lbl_status, "#96FF00 The new Hybrid MBR was written successfully!#");
	}
	else
		lv_label_set_text(lbl_status, "#FFDD00 Warning: The Hybrid MBR Fix was canceled!#");

out:
	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

lv_res_t create_window_partition_manager(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_SD" Partition Manager");

	lv_win_add_btn(win, NULL, SYMBOL_MODULES_ALT" Fix Hybrid MBR", _action_fix_mbr);

	static lv_style_t bar_hos_bg, bar_emu_bg, bar_l4t_bg, bar_and_bg;
	static lv_style_t bar_hos_ind, bar_emu_ind, bar_l4t_ind, bar_and_ind;
	static lv_style_t bar_hos_btn, bar_emu_btn, bar_l4t_btn, bar_and_btn;
	static lv_style_t sep_emu_bg, sep_l4t_bg, sep_and_bg;

	lv_style_copy(&bar_hos_bg, lv_theme_get_current()->bar.bg);
	bar_hos_bg.body.main_color = LV_COLOR_HEX(0x4A8000);
	bar_hos_bg.body.grad_color = bar_hos_bg.body.main_color;
	lv_style_copy(&bar_hos_ind, lv_theme_get_current()->bar.indic);
	bar_hos_ind.body.main_color = LV_COLOR_HEX(0x96FF00);
	bar_hos_ind.body.grad_color = bar_hos_ind.body.main_color;
	lv_style_copy(&bar_hos_btn, lv_theme_get_current()->slider.knob);
	bar_hos_btn.body.main_color = LV_COLOR_HEX(0x77CC00);
	bar_hos_btn.body.grad_color = bar_hos_btn.body.main_color;

	lv_style_copy(&bar_emu_bg, lv_theme_get_current()->bar.bg);
	bar_emu_bg.body.main_color = LV_COLOR_HEX(0x940F00);
	bar_emu_bg.body.grad_color = bar_emu_bg.body.main_color;
	lv_style_copy(&bar_emu_ind, lv_theme_get_current()->bar.indic);
	bar_emu_ind.body.main_color = LV_COLOR_HEX(0xFF3C28);
	bar_emu_ind.body.grad_color = bar_emu_ind.body.main_color;
	lv_style_copy(&bar_emu_btn, lv_theme_get_current()->slider.knob);
	bar_emu_btn.body.main_color = LV_COLOR_HEX(0xB31200);
	bar_emu_btn.body.grad_color = bar_emu_btn.body.main_color;
	lv_style_copy(&sep_emu_bg, lv_theme_get_current()->cont);
	sep_emu_bg.body.main_color = LV_COLOR_HEX(0xFF3C28);
	sep_emu_bg.body.grad_color = sep_emu_bg.body.main_color;
	sep_emu_bg.body.radius = 0;

	lv_style_copy(&bar_l4t_bg, lv_theme_get_current()->bar.bg);
	bar_l4t_bg.body.main_color = LV_COLOR_HEX(0x006E80);
	bar_l4t_bg.body.grad_color = bar_l4t_bg.body.main_color;
	lv_style_copy(&bar_l4t_ind, lv_theme_get_current()->bar.indic);
	bar_l4t_ind.body.main_color = LV_COLOR_HEX(0x00DDFF);
	bar_l4t_ind.body.grad_color = bar_l4t_ind.body.main_color;
	lv_style_copy(&bar_l4t_btn, lv_theme_get_current()->slider.knob);
	bar_l4t_btn.body.main_color = LV_COLOR_HEX(0x00B1CC);
	bar_l4t_btn.body.grad_color = bar_l4t_btn.body.main_color;
	lv_style_copy(&sep_l4t_bg, &sep_emu_bg);
	sep_l4t_bg.body.main_color = LV_COLOR_HEX(0x00DDFF);
	sep_l4t_bg.body.grad_color = sep_l4t_bg.body.main_color;

	lv_style_copy(&bar_and_bg, lv_theme_get_current()->bar.bg);
	bar_and_bg.body.main_color = LV_COLOR_HEX(0x804000);
	bar_and_bg.body.grad_color = bar_and_bg.body.main_color;
	lv_style_copy(&bar_and_ind, lv_theme_get_current()->bar.indic);
	bar_and_ind.body.main_color = LV_COLOR_HEX(0xFF8000);
	bar_and_ind.body.grad_color = bar_and_ind.body.main_color;
	lv_style_copy(&bar_and_btn, lv_theme_get_current()->slider.knob);
	bar_and_btn.body.main_color = LV_COLOR_HEX(0xCC6600);
	bar_and_btn.body.grad_color = bar_and_btn.body.main_color;
	lv_style_copy(&sep_and_bg, &sep_emu_bg);
	sep_and_bg.body.main_color = LV_COLOR_HEX(0xFF8000);
	sep_and_bg.body.grad_color = sep_and_bg.body.main_color;

	lv_obj_t *sep = lv_label_create(win, NULL);
	lv_label_set_static_text(sep, "");
	lv_obj_align(sep, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

	// Create container to keep content inside.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_obj_set_size(h1, LV_HOR_RES - (LV_DPI * 8 / 10), LV_VER_RES - LV_DPI);

	if (!sd_mount())
	{
		lv_obj_t *lbl = lv_label_create(h1, NULL);
		lv_label_set_text(lbl, "#FFDD00 Failed to init SD!#");
		return LV_RES_OK;
	}

	memset(&part_info, 0, sizeof(partition_ctxt_t));
	create_mbox_check_files_total_size();

	char *txt_buf = malloc(SZ_8K);

	part_info.total_sct = sd_storage.sec_cnt;

	// Align down total size to ensure alignment of all partitions after HOS one.
	part_info.alignment = part_info.total_sct - ALIGN_DOWN(part_info.total_sct, 0x8000);
	part_info.total_sct -= part_info.alignment;

	u32 extra_sct = 0x8000 + 0x400000; // Reserved 16MB alignment for FAT partition + 2GB.

	// Set initial HOS partition size, so the correct cluster size can be selected.
	part_info.hos_size = (part_info.total_sct >> 11) - 16; // Important if there's no slider change.

	// Check if eMMC should be 64GB (Aula).
	part_info.emmc_is_64gb = fuse_read_hw_type() == FUSE_NX_HW_TYPE_AULA;

	// Read current MBR.
	mbr_t mbr = { 0 };
	sdmmc_storage_read(&sd_storage, 0, 1, &mbr);

	u32 bar_hos_size = lv_obj_get_width(h1);
	u32 bar_emu_size = 0;
	u32 bar_l4t_size = 0;
	u32 bar_and_size = 0;

	lv_obj_t *lbl = lv_label_create(h1, NULL);
	lv_label_set_recolor(lbl, true);
	lv_label_set_text(lbl, "Choose #FFDD00 new# partition layout:");

	lv_obj_t *bar_hos = lv_bar_create(h1, NULL);
	lv_obj_set_size(bar_hos, bar_hos_size, LV_DPI / 2);
	lv_bar_set_range(bar_hos, 0, 1);
	lv_bar_set_value(bar_hos, 1);
	lv_bar_set_style(bar_hos, LV_BAR_STYLE_INDIC, &bar_hos_ind);
	lv_obj_align(bar_hos, lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 6);
	part_info.bar_hos = bar_hos;

	lv_obj_t *bar_emu = lv_bar_create(h1, bar_hos);
	lv_obj_set_size(bar_emu, bar_emu_size, LV_DPI / 2);
	lv_bar_set_style(bar_emu, LV_BAR_STYLE_INDIC, &bar_emu_ind);
	lv_obj_align(bar_emu, bar_hos, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	part_info.bar_emu = bar_emu;

	lv_obj_t *bar_l4t = lv_bar_create(h1, bar_hos);
	lv_obj_set_size(bar_l4t, bar_l4t_size, LV_DPI / 2);
	lv_bar_set_style(bar_l4t, LV_BAR_STYLE_INDIC, &bar_l4t_ind);
	lv_obj_align(bar_l4t, bar_emu, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	part_info.bar_l4t = bar_l4t;

	lv_obj_t *bar_and = lv_bar_create(h1, bar_hos);
	lv_obj_set_size(bar_and, bar_and_size, LV_DPI / 2);
	lv_bar_set_style(bar_and, LV_BAR_STYLE_INDIC, &bar_and_ind);
	lv_obj_align(bar_and, bar_l4t, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	part_info.bar_and = bar_and;

	lv_obj_t *sep_emu = lv_cont_create(h1, NULL);
	lv_cont_set_fit(sep_emu, false, false);
	lv_obj_set_size(sep_emu, 0, LV_DPI / 2); // 8.
	lv_obj_set_style(sep_emu, &sep_emu_bg);
	lv_obj_align(sep_emu, bar_hos, LV_ALIGN_OUT_RIGHT_MID, -4, 0);
	part_info.sep_emu = sep_emu;

	lv_obj_t *sep_l4t = lv_cont_create(h1, sep_emu);
	lv_obj_set_style(sep_l4t, &sep_l4t_bg);
	lv_obj_align(sep_l4t, bar_emu, LV_ALIGN_OUT_RIGHT_MID, -4, 0);
	part_info.sep_l4t = sep_l4t;

	lv_obj_t *sep_and = lv_cont_create(h1, sep_emu);
	lv_obj_set_style(sep_and, &sep_and_bg);
	lv_obj_align(sep_and, bar_l4t, LV_ALIGN_OUT_RIGHT_MID, -4, 0);
	part_info.sep_and = sep_and;

	lv_obj_t *lbl_hos = lv_label_create(h1, NULL);
	lv_label_set_recolor(lbl_hos, true);
	lv_label_set_static_text(lbl_hos, "#96FF00 "SYMBOL_DOT" HOS (FAT32):#");
	lv_obj_align(lbl_hos, bar_hos, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);

	lv_obj_t *lbl_emu = lv_label_create(h1, lbl_hos);
	lv_label_set_static_text(lbl_emu, "#FF3C28 "SYMBOL_DOT" emuMMC (RAW):#");
	lv_obj_align(lbl_emu, lbl_hos, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	lv_obj_t *lbl_l4t = lv_label_create(h1, lbl_hos);
	lv_label_set_static_text(lbl_l4t, "#00DDFF "SYMBOL_DOT" Linux (EXT4):#");
	lv_obj_align(lbl_l4t, lbl_emu, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	lv_obj_t *lbl_and = lv_label_create(h1, lbl_hos);
	lv_label_set_static_text(lbl_and, "#FF8000 "SYMBOL_DOT" Android (USER):#");
	lv_obj_align(lbl_and, lbl_l4t, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	lv_obj_t *slider_bar_hos = lv_bar_create(h1, NULL);
	lv_obj_set_size(slider_bar_hos, LV_DPI * 7, LV_DPI * 3 / 17);
	lv_bar_set_range(slider_bar_hos, 0, (part_info.total_sct - 0x8000) / 0x200000);
	lv_bar_set_value(slider_bar_hos, (part_info.total_sct - 0x8000) / 0x200000);
	lv_bar_set_style(slider_bar_hos, LV_SLIDER_STYLE_BG, &bar_hos_bg);
	lv_bar_set_style(slider_bar_hos, LV_SLIDER_STYLE_INDIC, &bar_hos_ind);
	lv_obj_align(slider_bar_hos, lbl_hos, LV_ALIGN_OUT_RIGHT_MID, LV_DPI * 6 / 4, 0);
	part_info.slider_bar_hos = slider_bar_hos;

	lv_obj_t *slider_emu = lv_slider_create(h1, NULL);
	lv_obj_set_size(slider_emu, LV_DPI * 7, LV_DPI / 3);
	lv_slider_set_range(slider_emu, 0, 20);
	lv_slider_set_value(slider_emu, 0);
	lv_slider_set_style(slider_emu, LV_SLIDER_STYLE_BG, &bar_emu_bg);
	lv_slider_set_style(slider_emu, LV_SLIDER_STYLE_INDIC, &bar_emu_ind);
	lv_slider_set_style(slider_emu, LV_SLIDER_STYLE_KNOB, &bar_emu_btn);
	lv_obj_align(slider_emu, slider_bar_hos, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3 + 5);
	lv_slider_set_action(slider_emu, _action_slider_emu);
	part_info.slider_emu = slider_bar_hos;

	lv_obj_t *slider_l4t = lv_slider_create(h1, NULL);
	lv_obj_set_size(slider_l4t, LV_DPI * 7, LV_DPI / 3);
	lv_slider_set_range(slider_l4t, 0, (part_info.total_sct - extra_sct) / 0x200000);
	lv_slider_set_value(slider_l4t, 0);
	lv_slider_set_style(slider_l4t, LV_SLIDER_STYLE_BG, &bar_l4t_bg);
	lv_slider_set_style(slider_l4t, LV_SLIDER_STYLE_INDIC, &bar_l4t_ind);
	lv_slider_set_style(slider_l4t, LV_SLIDER_STYLE_KNOB, &bar_l4t_btn);
	lv_obj_align(slider_l4t, slider_emu, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3 - 3);
	lv_slider_set_action(slider_l4t, _action_slider_l4t);
	part_info.slider_l4t = slider_l4t;

	lv_obj_t *slider_and = lv_slider_create(h1, NULL);
	lv_obj_set_size(slider_and, LV_DPI * 7, LV_DPI / 3);
	lv_slider_set_range(slider_and, 0, (part_info.total_sct - extra_sct) / 0x200000 - 4); // Subtract android reserved size.
	lv_slider_set_value(slider_and, 0);
	lv_slider_set_style(slider_and, LV_SLIDER_STYLE_BG, &bar_and_bg);
	lv_slider_set_style(slider_and, LV_SLIDER_STYLE_INDIC, &bar_and_ind);
	lv_slider_set_style(slider_and, LV_SLIDER_STYLE_KNOB, &bar_and_btn);
	lv_obj_align(slider_and, slider_l4t, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3 - 3);
	lv_slider_set_action(slider_and, _action_slider_and);
	part_info.slider_and = slider_and;

	lv_obj_t *lbl_sl_hos = lv_label_create(h1, NULL);
	lv_label_set_recolor(lbl_sl_hos, true);
	s_printf(txt_buf, "#96FF00 %d GiB#", (part_info.total_sct - 0x8000) >> 11 >> 10);
	lv_label_set_text(lbl_sl_hos, txt_buf);
	lv_obj_align(lbl_sl_hos, slider_bar_hos, LV_ALIGN_OUT_RIGHT_MID, LV_DPI * 4 / 7, 0);
	part_info.lbl_hos = lbl_sl_hos;

	lv_obj_t *lbl_sl_emu = lv_label_create(h1, lbl_sl_hos);
	lv_label_set_text(lbl_sl_emu, "#FF3C28 0 GiB#");
	lv_obj_align(lbl_sl_emu, lbl_sl_hos, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
	part_info.lbl_emu = lbl_sl_emu;

	lv_obj_t *lbl_sl_l4t = lv_label_create(h1, lbl_sl_hos);
	lv_label_set_text(lbl_sl_l4t, "#00DDFF 0 GiB#");
	lv_obj_align(lbl_sl_l4t, lbl_sl_emu, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
	part_info.lbl_l4t = lbl_sl_l4t;

	lv_obj_t *lbl_sl_and = lv_label_create(h1, lbl_sl_hos);
	lv_label_set_text(lbl_sl_and, "#FF8000 0 GiB#");
	lv_obj_align(lbl_sl_and, lbl_sl_l4t, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
	part_info.lbl_and = lbl_sl_and;

	lv_obj_t *lbl_notes = lv_label_create(h1, NULL);
	lv_label_set_recolor(lbl_notes, true);
	lv_label_set_static_text(lbl_notes,
		"Note 1: Only up to #C7EA46 1GB# can be backed up. If more, you will be asked to back them manually at the next step.\n"
		"Note 2: Resized emuMMC formats the USER partition. A save data manager can be used to move them over.\n"
		"Note 3: The #C7EA46 Flash Linux# and #C7EA46 Flash Android# will flash files if suitable partitions and installer files are found.\n"
		"Note 4: The installation folder is #C7EA46 switchroot/install#. Linux uses #C7EA46 l4t.XX# and Android uses #C7EA46 twrp.img# and #C7EA46 tegra210-icosa.dtb#.");
	lv_label_set_style(lbl_notes, &hint_small_style);
	lv_obj_align(lbl_notes, lbl_and, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 5);

	lv_obj_t *btn1 = lv_btn_create(h1, NULL);
	lv_obj_t *label_btn = lv_label_create(btn1, NULL);
	lv_btn_set_fit(btn1, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_USB"  SD UMS");
	lv_obj_align(btn1, h1, LV_ALIGN_IN_TOP_LEFT, 0, LV_DPI * 5);
	lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, _action_part_manager_ums_sd);

	btn_flash_l4t = lv_btn_create(h1, NULL);
	lv_obj_t *label_btn2 = lv_label_create(btn_flash_l4t, NULL);
	lv_btn_set_fit(btn_flash_l4t, true, true);
	lv_label_set_static_text(label_btn2, SYMBOL_DOWNLOAD"  Flash Linux");
	lv_obj_align(btn_flash_l4t, btn1, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 3, 0);
	lv_btn_set_action(btn_flash_l4t, LV_BTN_ACTION_CLICK, _action_check_flash_linux);

	// Disable Flash Linux button if partition not found.
	u32 size_sct = _get_available_l4t_partition();
	if (!l4t_flash_ctxt.offset_sct || !size_sct || size_sct < 0x800000)
	{
		lv_obj_set_click(btn_flash_l4t, false);
		lv_btn_set_state(btn_flash_l4t, LV_BTN_STATE_INA);
	}

	btn_flash_android = lv_btn_create(h1, NULL);
	label_btn = lv_label_create(btn_flash_android, NULL);
	lv_btn_set_fit(btn_flash_android, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_DOWNLOAD"  Flash Android");
	lv_obj_align(btn_flash_android, btn_flash_l4t, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 3, 0);
	lv_btn_set_action(btn_flash_android, LV_BTN_ACTION_CLICK, _action_flash_android);

	// Disable Flash Android button if partition not found.
	if (!_get_available_android_partition())
	{
		lv_obj_set_click(btn_flash_android, false);
		lv_btn_set_state(btn_flash_android, LV_BTN_STATE_INA);
	}

	btn1 = lv_btn_create(h1, NULL);
	label_btn = lv_label_create(btn1, NULL);
	lv_btn_set_fit(btn1, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_SD"  Next Step");
	lv_obj_align(btn1, h1, LV_ALIGN_IN_TOP_RIGHT, 0, LV_DPI * 5);
	lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, _create_mbox_partitioning_next);

	free(txt_buf);

	sd_unmount();

	return LV_RES_OK;
}
