/*
 * Copyright (c) 2019-2025 CTCaer
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
#include "fe_emummc_tools.h"
#include "gui_tools_partition_manager.h"
#include "../hos/hos.h"
#include <libs/fatfs/diskio.h>
#include <libs/lvgl/lvgl.h>

#define SECTORS_PER_GB   0x200000

#define AU_ALIGN_SECTORS 0x8000 // 16MB.
#define AU_ALIGN_BYTES   (AU_ALIGN_SECTORS * SD_BLOCKSIZE)

#define HOS_USER_SECTOR      0x53C000
#define HOS_FAT_MIN_SIZE_MB  2048
#define HOS_USER_MIN_SIZE_MB 1024

#define EMU_SLIDER_MIN     0
#define EMU_SLIDER_MAX     44 // 24 GB. Always an even number.
#define EMU_SLIDER_1X_MAX  (EMU_SLIDER_MAX / 2)
#define EMU_SLIDER_1X_FULL EMU_SLIDER_1X_MAX
#define EMU_SLIDER_2X_MIN  (EMU_SLIDER_1X_MAX + 1)
#define EMU_SLIDER_2X_FULL EMU_SLIDER_MAX
#define EMU_SLIDER_OFFSET  3 // Min 4GB.
#define EMU_RSVD_MB        (4 + 4 + 16 + 8) // BOOT0 + BOOT1 + 16MB offset + 8MB alignment.

#define EMU_32GB_FULL      29856 // Actual: 29820 MB.
#define EMU_64GB_FULL      59680 // Actual: 59640 MB.

#define AND_SYS_SIZE_MB     6144 // 6 GB. Fits both Legacy (4912MB) and Dynamic (6144MB) partition schemes.

extern volatile boot_cfg_t *b_cfg;
extern volatile nyx_storage_t *nyx_str;

typedef struct _partition_ctxt_t
{
	bool emmc;
	sdmmc_storage_t *storage;

	u32 total_sct;
	u32 alignment;
	u32 emmc_size_mb;
	int backup_possible;

	s32 hos_min_size;

	s32 hos_size;
	u32 emu_size;
	u32 l4t_size;
	u32 and_size;

	bool emu_double;
	bool emmc_is_64gb;

	bool and_dynamic;

	mbr_t mbr_old;

	lv_obj_t *bar_hos;
	lv_obj_t *bar_emu;
	lv_obj_t *bar_l4t;
	lv_obj_t *bar_and;

	lv_obj_t *sep_emu;
	lv_obj_t *sep_l4t;
	lv_obj_t *sep_and;

	lv_obj_t *slider_bar_hos;

	lv_obj_t *cont_lbl;

	lv_obj_t *lbl_hos;
	lv_obj_t *lbl_emu;
	lv_obj_t *lbl_l4t;
	lv_obj_t *lbl_and;

	lv_obj_t *partition_button;
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

int _copy_file(const char *src, const char *dst, const char *path)
{
	FIL fp_src;
	FIL fp_dst;
	int res;

	// Open file for reading.
	f_chdrive(src);
	res = f_open(&fp_src, path, FA_READ);
	if (res != FR_OK)
		return res;

	u32 file_bytes_left = f_size(&fp_src);

	// Open file for writing.
	f_chdrive(dst);
	f_open(&fp_dst, path, FA_CREATE_ALWAYS | FA_WRITE);
	f_lseek(&fp_dst, f_size(&fp_src));
	f_lseek(&fp_dst, 0);

	while (file_bytes_left)
	{
		u32 chunk_size = MIN(file_bytes_left, SZ_4M); // 4MB chunks.
		file_bytes_left -= chunk_size;

		// Copy file to buffer.
		f_read(&fp_src, (void *)SDXC_BUF_ALIGNED, chunk_size, NULL);

		// Write file to disk.
		f_write(&fp_dst, (void *)SDXC_BUF_ALIGNED, chunk_size, NULL);
	}

	f_close(&fp_dst);
	f_chdrive(src);
	f_close(&fp_src);

	return FR_OK;
}

static int _stat_and_copy_files(const char *src, const char *dst, char *path, u32 *total_files, u32 *total_size, lv_obj_t **labels)
{
	FRESULT res;
	FIL fp_src;
	FIL fp_dst;
	DIR dir;
	u32 dirLength = 0;
	static FILINFO fno;

	f_chdrive(src);

	// Open directory.
	res = f_opendir(&dir, path);
	if (res != FR_OK)
		return res;

	if (labels)
		lv_label_set_text(labels[0], path);

	dirLength = strlen(path);

	// Hard limit path to 1024 characters. Do not result to error.
	if (dirLength > 1024)
		goto out;

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

			if (dst)
			{
				u32 file_bytes_left = fno.fsize;

				// Open file for writing.
				f_chdrive(dst);
				f_open(&fp_dst, path, FA_CREATE_ALWAYS | FA_WRITE);
				f_lseek(&fp_dst, fno.fsize);
				f_lseek(&fp_dst, 0);

				// Open file for reading.
				f_chdrive(src);
				f_open(&fp_src, path, FA_READ);

				while (file_bytes_left)
				{
					u32 chunk_size = MIN(file_bytes_left, SZ_4M); // 4MB chunks.
					file_bytes_left -= chunk_size;

					// Copy file to buffer.
					f_read(&fp_src, (void *)SDXC_BUF_ALIGNED, chunk_size, NULL);
					manual_system_maintenance(true);

					// Write file to disk.
					f_write(&fp_dst, (void *)SDXC_BUF_ALIGNED, chunk_size, NULL);
				}

				// Finalize copied file.
				f_close(&fp_dst);
				f_chdrive(dst);
				f_chmod(path, fno.fattrib, 0xFF);

				f_chdrive(src);
				f_close(&fp_src);
			}

			// If total is > 1.2GB exit.
			if (*total_size > (RAM_DISK_SZ - SZ_16M)) // Account for alignment.
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
			res = _stat_and_copy_files(src, dst, path, total_files, total_size, labels);
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

out:
	f_closedir(&dir);

	return res;
}

static void _create_gpt_partition(gpt_t *gpt, u8 *gpt_idx, u32 *curr_part_lba, u32 size_lba, const char *name, int name_size)
{
	static const u8 linux_part_guid[] = { 0xAF, 0x3D, 0xC6, 0x0F,  0x83, 0x84,  0x72, 0x47,  0x8E, 0x79,  0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4 };
	u8 random_number[16];

	// Reset partition.
	memset(&gpt->entries[*gpt_idx], 0, sizeof(gpt_entry_t));

	// Create GPT partition.
	memcpy(gpt->entries[*gpt_idx].type_guid, linux_part_guid, 16);

	// Set randomly created GUID
	se_gen_prng128(random_number);
	memcpy(gpt->entries[*gpt_idx].part_guid, random_number, 16);

	// Set partition start and end.
	gpt->entries[*gpt_idx].lba_start = *curr_part_lba;
	gpt->entries[*gpt_idx].lba_end = *curr_part_lba + size_lba - 1;

	// Set name.
	u16 name_utf16[36] = {0};
	u32 name_lenth = strlen(name);
	for (u32 i = 0; i < name_lenth; i++)
		name_utf16[i] = name[i];
	memcpy(gpt->entries[*gpt_idx].name, name_utf16, name_lenth * sizeof(u16));

	// Wipe the first 1MB to sanitize it as raw-empty partition.
	sdmmc_storage_write(part_info.storage, *curr_part_lba, 0x800, (void *)SDMMC_UPPER_BUFFER);

	// Prepare for next.
	(*curr_part_lba) += size_lba;
	(*gpt_idx)++;
}

static void _sd_prepare_and_flash_mbr_gpt()
{
	mbr_t mbr;
	u8 random_number[16];

	// Read current MBR.
	sdmmc_storage_read(&sd_storage, 0, 1, &mbr);

	// Copy over metadata if they exist.
	if (*(u32 *)&part_info.mbr_old.bootstrap[0x80])
		memcpy(&mbr.bootstrap[0x80], &part_info.mbr_old.bootstrap[0x80], 64);
	if (*(u32 *)&part_info.mbr_old.bootstrap[0xE0])
		memcpy(&mbr.bootstrap[0xE0], &part_info.mbr_old.bootstrap[0xE0], 208);

	// Clear the first 16MB.
	memset((void *)SDMMC_UPPER_BUFFER, 0, AU_ALIGN_BYTES);
	sdmmc_storage_write(&sd_storage, 0, AU_ALIGN_SECTORS, (void *)SDMMC_UPPER_BUFFER);

	// Set disk signature.
	se_gen_prng128(random_number);
	memcpy(&mbr.signature, random_number, 4);

	// FAT partition as first.
	u8 mbr_idx = 1;

	// Apply L4T Linux second to MBR if no Android.
	if (part_info.l4t_size && !part_info.and_size)
	{
		mbr.partitions[mbr_idx].type = 0x83; // Linux system partition.
		mbr.partitions[mbr_idx].start_sct = AU_ALIGN_SECTORS + ((u32)part_info.hos_size << 11);
		mbr.partitions[mbr_idx].size_sct = part_info.l4t_size << 11;
		sdmmc_storage_write(&sd_storage, mbr.partitions[mbr_idx].start_sct, 0x800, (void *)SDMMC_UPPER_BUFFER); // Clear the first 1MB.
		mbr_idx++;
	}

	// emuMMC goes second or third. Next to L4T if no Android.
	if (part_info.emu_size)
	{
		mbr.partitions[mbr_idx].type = 0xE0; // emuMMC partition.
		mbr.partitions[mbr_idx].start_sct = AU_ALIGN_SECTORS + ((u32)part_info.hos_size << 11) + (part_info.l4t_size << 11) + (part_info.and_size << 11);

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
		gpt_t *gpt = zalloc(sizeof(gpt_t));
		gpt_header_t gpt_hdr_backup = { 0 };

		// Set GPT protective partition in MBR.
		mbr.partitions[mbr_idx].type = 0xEE;
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

		// Set FAT GPT partition manually.
		const u8 basic_part_guid[] = { 0xA2, 0xA0, 0xD0, 0xEB,  0xE5, 0xB9,  0x33, 0x44,  0x87, 0xC0,  0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 };
		memcpy(gpt->entries[0].type_guid, basic_part_guid, 16);
		se_gen_prng128(random_number);
		memcpy(gpt->entries[0].part_guid, random_number, 16);

		// Clear non-standard Windows MBR attributes. bit4: Read only, bit5: Shadow copy, bit6: Hidden, bit7: No drive letter.
		gpt->entries[0].part_guid[7] = 0;

		gpt->entries[0].lba_start = mbr.partitions[0].start_sct;
		gpt->entries[0].lba_end = mbr.partitions[0].start_sct + mbr.partitions[0].size_sct - 1;
		memcpy(gpt->entries[0].name, (u16[]) { 'h', 'o', 's', '_', 'd', 'a', 't', 'a' }, 16);

		// Set the rest of GPT partitions.
		u8  gpt_idx = 1;
		u32 curr_part_lba = AU_ALIGN_SECTORS + ((u32)part_info.hos_size << 11);

		// L4T partition.
		if (part_info.l4t_size)
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, part_info.l4t_size << 11, "l4t", 6);

		if (part_info.and_dynamic)
		{
			// Android Linux Kernel partition. 64MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x20000, "boot", 8);

			// Android Recovery partition. 64MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x20000, "recovery", 16);

			// Android Device Tree Reference partition. 1MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,    0x800, "dtb", 6);

			// Android Misc partition. 3MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,   0x1800, "misc", 8);

			// Android Cache partition. 60MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x1E000, "cache", 10);

			// Android Super dynamic partition. 5952MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, 0xBA0000, "super", 10);

			// Android Userdata partition.
			u32 uda_size = (part_info.and_size << 11) - 0xC00000; // Subtract the other partitions (6144MB).
			if (!part_info.emu_size)
				uda_size -= 0x800; // Reserve 1MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, uda_size, "userdata", 16);
		}
		else
		{
			// Android Vendor partition. 1GB
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, 0x200000, "vendor", 12);

			// Android System partition. 3GB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, 0x600000, "APP", 6);

			// Android Linux Kernel partition. 32MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x10000, "LNX", 6);

			// Android Recovery partition. 64MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x20000, "SOS", 6);

			// Android Device Tree Reference partition. 1MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,    0x800, "DTB", 6);

			// Android Encryption partition. 16MB.
			// Note: 16MB size is for aligning UDA. If any other tiny partition must be added, it should split the MDA one.
			sdmmc_storage_write(&sd_storage, curr_part_lba, 0x8000, (void *)SDMMC_UPPER_BUFFER); // Clear the whole of it.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,   0x8000, "MDA", 6);

			// Android Cache partition. 700MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, 0x15E000, "CAC", 6);

			// Android Misc partition. 3MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,   0x1800, "MSC", 6);

			// Android Userdata partition.
			u32 uda_size = (part_info.and_size << 11) - 0x998000; // Subtract the other partitions (4912MB).
			if (!part_info.emu_size)
				uda_size -= 0x800; // Reserve 1MB.
			_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, uda_size, "UDA", 6);
		}

		// Handle emuMMC partitions manually.
		if (part_info.emu_size)
		{
			// Set 1st emuMMC.
			u8 emu_part_guid[] = { 0x00, 0x7E, 0xCA, 0x11,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  'e', 'm', 'u', 'M', 'M', 'C' };
			memcpy(gpt->entries[gpt_idx].type_guid, emu_part_guid, 16);
			se_gen_prng128(random_number);
			memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
			gpt->entries[gpt_idx].lba_start = curr_part_lba;
			if (!part_info.emu_double)
				gpt->entries[gpt_idx].lba_end = curr_part_lba + (part_info.emu_size << 11) - 0x800 - 1; // Reserve 1MB.
			else
				gpt->entries[gpt_idx].lba_end = curr_part_lba + (part_info.emu_size << 10) - 1;
			memcpy(gpt->entries[gpt_idx].name, (u16[]) { 'e', 'm', 'u', 'm', 'm', 'c' }, 12);
			gpt_idx++;

			// Set 2nd emuMMC.
			if (part_info.emu_double)
			{
				curr_part_lba += (part_info.emu_size << 10);
				memcpy(gpt->entries[gpt_idx].type_guid, emu_part_guid, 16);
				se_gen_prng128(random_number);
				memcpy(gpt->entries[gpt_idx].part_guid, random_number, 16);
				gpt->entries[gpt_idx].lba_start = curr_part_lba;
				gpt->entries[gpt_idx].lba_end = curr_part_lba + (part_info.emu_size << 10) - 0x800 - 1; // Reserve 1MB.
				memcpy(gpt->entries[gpt_idx].name, (u16[]) { 'e', 'm', 'u', 'm', 'm', 'c', '2' }, 14);
				gpt_idx++;
			}
		}

		// Set final GPT header parameters.
		gpt->header.num_part_ents = gpt_idx;
		gpt->header.part_ents_crc32 = crc32_calc(0, (const u8 *)gpt->entries, sizeof(gpt_entry_t) * gpt->header.num_part_ents);
		gpt->header.crc32 = 0; // Set to 0 for calculation.
		gpt->header.crc32 = crc32_calc(0, (const u8 *)&gpt->header, gpt->header.size);

		// Set final backup GPT header parameters.
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

static int _emmc_prepare_and_flash_mbr_gpt()
{
	gpt_t *gpt = zalloc(sizeof(gpt_t));
	gpt_header_t gpt_hdr_backup = { 0 };

	// Read main GPT.
	sdmmc_storage_read(&emmc_storage, 1, sizeof(gpt_t) >> 9, gpt);

	// Set GPT header.
	gpt->header.alt_lba = emmc_storage.sec_cnt - 1;
	gpt->header.last_use_lba = emmc_storage.sec_cnt - 0x800 - 1; // emmc_storage.sec_cnt - 33 is start of backup gpt partition entries.

	// Calculate HOS USER partition
	u32 part_rsvd_size = (part_info.l4t_size << 11) + (part_info.and_size << 11);
	part_rsvd_size += part_rsvd_size ? part_info.alignment : 0x800; // Only reserve 1MB if no extra partitions.
	u32 hos_user_size = emmc_storage.sec_cnt - HOS_USER_SECTOR - part_rsvd_size;

	// Get HOS USER partition index.
	LIST_INIT(gpt_emmc);
	emmc_gpt_parse(&gpt_emmc);
	emmc_part_t *user_part = emmc_part_find(&gpt_emmc, "USER");
	if (!user_part)
	{
		emmc_gpt_free(&gpt_emmc);
		free(gpt);

		return 1;
	}
	u8 gpt_idx = user_part->index;
	emmc_gpt_free(&gpt_emmc);

	// HOS USER partition.
	u32 curr_part_lba = gpt->entries[gpt_idx].lba_start;
	gpt->entries[gpt_idx].lba_end = curr_part_lba + hos_user_size - 1;

	curr_part_lba += hos_user_size;
	gpt_idx++;

	// L4T partition.
	if (part_info.l4t_size)
	{
		u32 l4t_size = part_info.l4t_size << 11;
		if (!part_info.and_size)
			l4t_size -= 0x800; // Reserve 1MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, l4t_size, "l4t", 6);
	}

	if (part_info.and_size && part_info.and_dynamic)
	{
		// Android Linux Kernel partition. 64MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x20000, "boot", 8);

		// Android Recovery partition. 64MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x20000, "recovery", 16);

		// Android Device Tree Reference partition. 1MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,    0x800, "dtb", 6);

		// Android Misc partition. 3MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,   0x1800, "misc", 8);

		// Android Cache partition. 60MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x1E000, "cache", 10);

		// Android Super dynamic partition. 5952MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, 0xBA0000, "super", 10);

		// Android Userdata partition.
		u32 uda_size = (part_info.and_size << 11) - 0xC00000; // Subtract the other partitions (6144MB).
		uda_size -= 0x800; // Reserve 1MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, uda_size, "userdata", 16);
	}
	else if (part_info.and_size)
	{
		// Android Vendor partition. 1GB
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, 0x200000, "vendor", 12);

		// Android System partition. 3GB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, 0x600000, "APP", 6);

		// Android Linux Kernel partition. 32MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x10000, "LNX", 6);

		// Android Recovery partition. 64MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,  0x20000, "SOS", 6);

		// Android Device Tree Reference partition. 1MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,    0x800, "DTB", 6);

		// Android Encryption partition. 16MB.
		// Note: 16MB size is for aligning UDA. If any other tiny partition must be added, it should split the MDA one.
		sdmmc_storage_write(&emmc_storage, curr_part_lba, 0x8000, (void *)SDMMC_UPPER_BUFFER); // Clear the whole of it.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,   0x8000, "MDA", 6);

		// Android Cache partition. 700MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, 0x15E000, "CAC", 6);

		// Android Misc partition. 3MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba,   0x1800, "MSC", 6);

		// Android Userdata partition.
		u32 uda_size = (part_info.and_size << 11) - 0x998000; // Subtract the other partitions (4912MB).
		uda_size -= 0x800; // Reserve 1MB.
		_create_gpt_partition(gpt, &gpt_idx, &curr_part_lba, uda_size, "UDA", 6);
	}

	// Clear the rest of GPT partition table.
	for (u32 i = gpt_idx; i < 128; i++)
		memset(&gpt->entries[i], 0, sizeof(gpt_entry_t));

	// Set final GPT header parameters.
	gpt->header.num_part_ents = gpt_idx;
	gpt->header.part_ents_crc32 = crc32_calc(0, (const u8 *)gpt->entries, sizeof(gpt_entry_t) * gpt->header.num_part_ents);
	gpt->header.crc32 = 0; // Set to 0 for calculation.
	gpt->header.crc32 = crc32_calc(0, (const u8 *)&gpt->header, gpt->header.size);

	// Set final backup GPT header parameters.
	memcpy(&gpt_hdr_backup, &gpt->header, sizeof(gpt_header_t));
	gpt_hdr_backup.my_lba = emmc_storage.sec_cnt - 1;
	gpt_hdr_backup.alt_lba = 1;
	gpt_hdr_backup.part_ent_lba = emmc_storage.sec_cnt - 33;
	gpt_hdr_backup.crc32 = 0; // Set to 0 for calculation.
	gpt_hdr_backup.crc32 = crc32_calc(0, (const u8 *)&gpt_hdr_backup, gpt_hdr_backup.size);

	// Write main GPT.
	sdmmc_storage_write(&emmc_storage, gpt->header.my_lba, sizeof(gpt_t) >> 9, gpt);

	// Write backup GPT partition table.
	sdmmc_storage_write(&emmc_storage, gpt_hdr_backup.part_ent_lba, ((sizeof(gpt_entry_t) * 128) >> 9), gpt->entries);

	// Write backup GPT header.
	sdmmc_storage_write(&emmc_storage, gpt_hdr_backup.my_lba, 1, &gpt_hdr_backup);

	// Clear nand patrol.
	u8 *buf = (u8 *)gpt;
	memset(buf, 0, EMMC_BLOCKSIZE);
	emmc_set_partition(EMMC_BOOT0);
	sdmmc_storage_write(&emmc_storage, NAND_PATROL_SECTOR, 1, buf);
	emmc_set_partition(EMMC_GPP);

	free(gpt);

	return 0;
}

static lv_res_t _action_part_manager_ums_sd(lv_obj_t *btn)
{
	action_ums_sd(NULL);

	// Close and reopen partition manager.
	lv_action_t close_btn_action = lv_btn_get_action(close_btn, LV_BTN_ACTION_CLICK);
	close_btn_action(close_btn);
	lv_obj_del(ums_mbox);
	create_window_sd_partition_manager(NULL);

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

	static const char *mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
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
	if (part_info.emmc)
		emmc_initialize(false);

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

	// Start flashing L4T.
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

		// Read next data block from SD.
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

		// Write data block to L4T partition.
		res = !sdmmc_storage_write(part_info.storage, lba_curr + l4t_flash_ctxt.offset_sct, num, buf);

		manual_system_maintenance(false);

		// If failed, retry 3 more times.
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

			res = !sdmmc_storage_write(part_info.storage, lba_curr + l4t_flash_ctxt.offset_sct, num, buf);
			manual_system_maintenance(false);
		}

		// Update completion percentage.
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
	if (part_info.emmc)
		emmc_end();

	return LV_RES_INV;
}

static u32 _get_available_l4t_partition()
{
	mbr_t mbr = { 0 };
	gpt_t *gpt = zalloc(sizeof(gpt_t));

	memset(&l4t_flash_ctxt, 0, sizeof(l4t_flasher_ctxt_t));

	// Read MBR.
	sdmmc_storage_read(part_info.storage, 0, 1, &mbr);

	// Read main GPT.
	sdmmc_storage_read(part_info.storage, 1, sizeof(gpt_t) >> 9, gpt);

	// Search for a suitable partition.
	u32 size_sct = 0;
	if (!memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
	{
		for (u32 i = 0; i < gpt->header.num_part_ents; i++)
		{
			if (!memcmp(gpt->entries[i].name, (u16[]) { 'l', '4', 't' }, 6))
			{
				l4t_flash_ctxt.offset_sct = gpt->entries[i].lba_start;
				size_sct = (gpt->entries[i].lba_end + 1) - gpt->entries[i].lba_start;
				break;
			}
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

static int _get_available_android_partition()
{
	gpt_t *gpt = zalloc(sizeof(gpt_t));

	// Read main GPT.
	sdmmc_storage_read(part_info.storage, 1, sizeof(gpt_t) >> 9, gpt);

	// Check if GPT.
	if (memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
		goto out;

	// Find kernel partition.
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (gpt->entries[i].lba_start)
		{
			int found  = !memcmp(gpt->entries[i].name, (u16[]) { 'b', 'o', 'o', 't' }, 8) ? 2 : 0;
				found |= !memcmp(gpt->entries[i].name, (u16[]) { 'L', 'N', 'X' },      6) ? 1 : 0;

			if (found)
			{
				free(gpt);

				return found;
			}
		}
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

	static const char *mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
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
	if (part_info.emmc)
		emmc_initialize(false);

	// Check if L4T image exists.
	strcpy(path, "switchroot/install/l4t.00");
	if (f_stat(path, NULL))
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# Installation files not found!");
		goto error;
	}

	// Find an applicable partition for L4T.
	u32 size_sct = _get_available_l4t_partition();
	if (!l4t_flash_ctxt.offset_sct || size_sct < 0x800000)
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# No partition found!");
		goto error;
	}

	u32 idx = 0;
	path[23] = 0;

	// Validate L4T images and consolidate their info.
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

		// Check if current part is unaligned.
		if ((u64)fno.fsize % SZ_4M)
		{
			// Get next part filename.
			idx++;
			if (idx < 10)
			{
				path[23] = '0';
				itoa(idx, &path[23 + 1], 10);
			}
			else
				itoa(idx, &path[23], 10);

			// If it exists, unaligned size for current part is not permitted.
			if (!f_stat(path, NULL)) // NULL: Don't override current part fs info.
			{
				lv_label_set_text(lbl_status, "#FFDD00 Error:# The image is not aligned to 4 MiB!");
				goto error;
			}

			// Last part. Align size to LBA (SD_BLOCKSIZE).
			fno.fsize = ALIGN((u64)fno.fsize, SD_BLOCKSIZE);
			idx--;
		}
		l4t_flash_ctxt.image_size_sct += (u64)fno.fsize >> 9;

		idx++;
	}

	// Check if image size is bigger than the partition available.
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
	if (part_info.emmc)
		emmc_end();

	return LV_RES_OK;
}

static lv_res_t _action_reboot_recovery(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	// Delete parent mbox.
	mbox_action(btns, txt);

	if (!btn_idx)
	{
		// Set custom reboot type to Android Recovery.
		PMC(APBDEV_PMC_SCRATCH0) |= PMC_SCRATCH0_MODE_RECOVERY;

		// Enable hekate boot configuration.
		b_cfg->boot_cfg = BOOT_CFG_FROM_ID | BOOT_CFG_AUTOBOOT_EN;

		// Set id to Android.
		strcpy((char *)b_cfg->id, "SWANDR");

		void (*main_ptr)() = (void *)nyx_str->hekate;

		// Deinit hardware.
		sd_end();
		hw_deinit(false, 0);

		// Chainload to hekate main.
		(*main_ptr)();
	}

	return LV_RES_INV;
}

static lv_res_t _action_flash_android_data(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);
	bool boot_recovery = false;

	// Delete parent mbox.
	mbox_action(btns, txt);

	if (btn_idx)
		return LV_RES_INV;

	// Flash Android components.
	char path[128];
	gpt_t *gpt = zalloc(sizeof(gpt_t));
	char *txt_buf = malloc(SZ_4K);

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
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
	if (part_info.emmc)
		emmc_initialize(false);

	// Read main GPT.
	sdmmc_storage_read(part_info.storage, 1, sizeof(gpt_t) >> 9, gpt);

	// Validate GPT header.
	if (memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# No Android GPT was found!");
		goto error;
	}

	u32 offset_sct = 0;
	u32 size_sct = 0;

	// Check if Kernel image should be flashed.
	strcpy(path, "switchroot/install/boot.img");
	if (f_stat(path, NULL))
	{
		s_printf(txt_buf, "#FF8000 Warning:# Kernel image not found!\n");
		goto boot_img_not_found;
	}

	// Find Kernel partition.
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, (u16[]) { 'L', 'N', 'X' },      6) ||
			!memcmp(gpt->entries[i].name, (u16[]) { 'b', 'o', 'o', 't' }, 8))
		{
			offset_sct = gpt->entries[i].lba_start;
			size_sct = (gpt->entries[i].lba_end + 1) - gpt->entries[i].lba_start;
			break;
		}
	}

	// Flash Kernel.
	if (offset_sct && size_sct)
	{
		u32 file_size = 0;
		u8 *buf = sd_file_read(path, &file_size);

		if (file_size % 0x200)
		{
			file_size = ALIGN(file_size, 0x200);
			u8 *buf_tmp = zalloc(file_size);
			memcpy(buf_tmp, buf, file_size);
			free(buf);
			buf = buf_tmp;
		}

		if ((file_size >> 9) > size_sct)
			s_printf(txt_buf, "#FF8000 Warning:# Kernel image too big!\n");
		else
		{
			sdmmc_storage_write(part_info.storage, offset_sct, file_size >> 9, buf);

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

	// Check if Recovery should be flashed.
	strcpy(path, "switchroot/install/recovery.img");
	if (f_stat(path, NULL))
	{
		// Not found, try twrp.img instead.
		strcpy(path, "switchroot/install/twrp.img");
		if (f_stat(path, NULL))
		{
			strcat(txt_buf, "#FF8000 Warning:# Recovery image not found!\n");
			goto recovery_not_found;
		}
	}

	offset_sct = 0;
	size_sct = 0;

	// Find Recovery partition.
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, (u16[]) { 'S', 'O', 'S' },                           6) ||
			!memcmp(gpt->entries[i].name, (u16[]) { 'r', 'e', 'c', 'o', 'v', 'e', 'r', 'y' }, 16))
		{
			offset_sct = gpt->entries[i].lba_start;
			size_sct = (gpt->entries[i].lba_end + 1) - gpt->entries[i].lba_start;
			break;
		}
	}

	// Flash Recovery.
	if (offset_sct && size_sct)
	{
		u32 file_size = 0;
		u8 *buf = sd_file_read(path, &file_size);

		if (file_size % 0x200)
		{
			file_size = ALIGN(file_size, 0x200);
			u8 *buf_tmp = zalloc(file_size);
			memcpy(buf_tmp, buf, file_size);
			free(buf);
			buf = buf_tmp;
		}

		if ((file_size >> 9) > size_sct)
			strcat(txt_buf, "#FF8000 Warning:# Recovery image too big!\n");
		else
		{
			sdmmc_storage_write(part_info.storage, offset_sct, file_size >> 9, buf);
			strcat(txt_buf, "#C7EA46 Success:# Recovery image flashed!\n");
			f_unlink(path);
		}

		free(buf);
	}
	else
		strcat(txt_buf, "#FF8000 Warning:# Recovery partition not found!\n");

recovery_not_found:
	lv_label_set_text(lbl_status, txt_buf);
	manual_system_maintenance(true);

	// Check if Device Tree should be flashed.
	strcpy(path, "switchroot/install/nx-plat.dtimg");
	if (f_stat(path, NULL))
	{
		strcpy(path, "switchroot/install/tegra210-icosa.dtb");
		if (f_stat(path, NULL))
		{
			strcat(txt_buf, "#FF8000 Warning:# DTB image not found!");
			goto dtb_not_found;
		}
	}

	offset_sct = 0;
	size_sct = 0;

	// Find Device Tree partition.
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, (u16[]) { 'D', 'T', 'B' }, 6) ||
			!memcmp(gpt->entries[i].name, (u16[]) { 'd', 't', 'b' }, 6))
		{
			offset_sct = gpt->entries[i].lba_start;
			size_sct = (gpt->entries[i].lba_end + 1) - gpt->entries[i].lba_start;
			break;
		}
	}

	// Flash Device Tree.
	if (offset_sct && size_sct)
	{
		u32 file_size = 0;
		u8 *buf = sd_file_read(path, &file_size);

		if (file_size % 0x200)
		{
			file_size = ALIGN(file_size, 0x200);
			u8 *buf_tmp = zalloc(file_size);
			memcpy(buf_tmp, buf, file_size);
			free(buf);
			buf = buf_tmp;
		}

		if ((file_size >> 9) > size_sct)
			strcat(txt_buf, "#FF8000 Warning:# DTB image too big!");
		else
		{
			sdmmc_storage_write(part_info.storage, offset_sct, file_size >> 9, buf);
			strcat(txt_buf, "#C7EA46 Success:# DTB image flashed!");
			f_unlink(path);
		}

		free(buf);
	}
	else
		strcat(txt_buf, "#FF8000 Warning:# DTB partition not found!");

dtb_not_found:
	lv_label_set_text(lbl_status, txt_buf);

	// Check if Recovery is flashed unconditionally.
	u8 *rec = malloc(SD_BLOCKSIZE);
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		if (!memcmp(gpt->entries[i].name, (u16[]) { 'S', 'O', 'S' },                           6) ||
			!memcmp(gpt->entries[i].name, (u16[]) { 'r', 'e', 'c', 'o', 'v', 'e', 'r', 'y' }, 16))
		{
			sdmmc_storage_read(part_info.storage, gpt->entries[i].lba_start, 1, rec);
			if (!memcmp(rec, "ANDROID", 7))
				boot_recovery = true;
			break;
		}
	}
	free(rec);

error:
	if (boot_recovery)
	{
		// If a Recovery partition was found, ask user if rebooting into it is wanted.
		strcat(txt_buf,"\n\nDo you want to reboot into Recovery\nto finish Android installation?");
		lv_label_set_text(lbl_status, txt_buf);
		lv_mbox_add_btns(mbox, mbox_btn_map2, _action_reboot_recovery);
	}
	else
		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

	free(txt_buf);
	free(gpt);

	sd_unmount();
	if (part_info.emmc)
		emmc_end();

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
		"This will flash #C7EA46 Kernel#, #C7EA46 DTB# and #C7EA46 Recovery# if found.\n"
		"These will be deleted after a successful flash.\n"
		"Do you want to continue?");

	lv_mbox_add_btns(mbox, mbox_btn_map, _action_flash_android_data);
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
		action_ums_sd(NULL);
		lv_obj_del(ums_mbox);
		break;
	case 1:
		_action_check_flash_linux(NULL);
		break;
	case 2:
		_action_flash_android(NULL);
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
		action_ums_sd(NULL);
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
		action_ums_sd(NULL);
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

static int _backup_and_restore_files(bool backup, lv_obj_t **labels)
{
	const char *src_drv = backup ? "sd:"  : "ram:";
	const char *dst_drv = backup ? "ram:" : "sd:";

	int res = 0;
	u32 total_size = 0;
	u32 total_files = 0;
	char *path = malloc(0x1000);
	path[0] = 0; // Set default as root folder.

	// Check if Mariko Warmboot Storage exists in source drive.
	f_chdrive(src_drv);
	bool backup_mws = !part_info.backup_possible && !f_stat("warmboot_mariko", NULL);
	bool backup_pld = !part_info.backup_possible && !f_stat("payload.bin", NULL);

	if (!part_info.backup_possible)
	{
		// Change path to hekate/Nyx.
		strcpy(path, "bootloader");

		// Create hekate/Nyx/MWS folders in destination drive.
		f_chdrive(dst_drv);
		f_mkdir("bootloader");
		if (backup_mws)
			f_mkdir("warmboot_mariko");
	}

	// Copy all or hekate/Nyx files.
	res = _stat_and_copy_files(src_drv, dst_drv, path, &total_files, &total_size, labels);

	// If incomplete backup mode, copy MWS and payload.bin also.
	if (!res)
	{
		if (backup_mws)
		{
			strcpy(path, "warmboot_mariko");
			res = _stat_and_copy_files(src_drv, dst_drv, path, &total_files, &total_size, labels);
		}

		if (!res && backup_pld)
		{
			strcpy(path, "payload.bin");
			res = _copy_file(src_drv, dst_drv, path);
		}
	}

	free(path);

	return res;
}

static lv_res_t _sd_create_mbox_start_partitioning()
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] =  { "\251", "\222OK", "\251", "" };
	static const char *mbox_btn_map1[] = { "\222SD UMS", "\222Flash Linux", "\222Flash Android", "\221OK", "" };
	static const char *mbox_btn_map2[] = { "\222SD UMS", "\222Flash Linux", "\221OK", "" };
	static const char *mbox_btn_map3[] = { "\222SD UMS", "\222Flash Android", "\221OK", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "#FF8000 SD Partition Manager#");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	bool buttons_set = false;

	// Use safety wait if backup is not possible.
	char *txt_buf = malloc(SZ_4K);
	strcpy(txt_buf, "#FF8000 SD Partition Manager#\n\nSafety wait ends in ");
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
		"#FF8000 SD Partition Manager#\n\n"
		"#FFDD00 Warning: Do you really want to continue?!#\n\n"
		"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	manual_system_maintenance(true);

	free(txt_buf);

	if (!(btn_wait() & BTN_POWER))
		goto exit;

	// Start partitioning.
	lv_mbox_set_text(mbox, "#FF8000 SD Partition Manager#");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	manual_system_maintenance(true);

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);

	lv_obj_t *lbl_paths[2];

	// Create backup/restore paths labels.
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

	// Read current MBR.
	sdmmc_storage_read(part_info.storage, 0, 1, &part_info.mbr_old);

	lv_label_set_text(lbl_status, "#00DDFF Status:# Initializing Ramdisk...");
	lv_label_set_text(lbl_paths[0], "Please wait...");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	manual_system_maintenance(true);

	// Initialize RAM disk.
	if (ram_disk_init(&ram_fs, RAM_DISK_SZ))
	{
		lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to initialize Ramdisk!");
		goto error;
	}

	lv_label_set_text(lbl_status, "#00DDFF Status:# Backing up files...");
	manual_system_maintenance(true);

	// Do full or hekate/Nyx backup.
	if (_backup_and_restore_files(true, lbl_paths))
	{
		if (part_info.backup_possible)
			lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to back up files!");
		else
			lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to back up files!\nBootloader folder exceeds 1.2GB or corrupt!");

		goto error;
	}

	f_unmount("sd:"); // Unmount SD card.

	lv_label_set_text(lbl_status, "#00DDFF Status:# Formatting FAT32 partition...");
	lv_label_set_text(lbl_paths[0], "Please wait...");
	lv_label_set_text(lbl_paths[1], " ");
	manual_system_maintenance(true);

	// Set reserved size.
	u32 part_rsvd_size = (part_info.emu_size << 11) + (part_info.l4t_size << 11) + (part_info.and_size << 11);
	part_rsvd_size += part_rsvd_size ? part_info.alignment : 0; // Do not reserve alignment space if no extra partitions.
	disk_set_info(DRIVE_SD, SET_SECTOR_COUNT, &part_rsvd_size);
	u8 *buf = malloc(SZ_4M);

	// Set cluster size to 64KB and try to format.
	u32 cluster_size = 65536;
	u32 mkfs_error = f_mkfs("sd:", FM_FAT32, cluster_size, buf, SZ_4M);

	if (!mkfs_error)
		goto mkfs_no_error;

	// Retry formatting by halving cluster size, until one succeeds.
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

		lv_label_set_text(lbl_status, "#00DDFF Status:# Restoring files...");
		manual_system_maintenance(true);

		// Restore backed up files back to SD.
		if (_backup_and_restore_files(false, lbl_paths))
		{
			// Failed to restore files. Try again once more.
			if (_backup_and_restore_files(false, lbl_paths))
			{
				lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to restore files!");
				free(buf);
				goto error;
			}
		}

		lv_label_set_text(lbl_status, "#00DDFF Status:# Restored files but the operation failed!");
		f_unmount("ram:");
		free(buf);
		goto error;
	}

mkfs_no_error:
	free(buf);

	// Remount sd card as it was unmounted from formatting it.
	f_mount(&sd_fs, "sd:", 1); // Mount SD card.

	lv_label_set_text(lbl_status, "#00DDFF Status:# Restoring files...");
	manual_system_maintenance(true);

	// Restore backed up files back to SD.
	if (_backup_and_restore_files(false, lbl_paths))
	{
		// Failed to restore files. Try again once more.
		if (_backup_and_restore_files(false, lbl_paths))
		{
			lv_label_set_text(lbl_status, "#FFDD00 Error:# Failed to restore files!");
			goto error;
		}
	}

	 // Unmount ramdisk.
	f_unmount("ram:");
	f_chdrive("sd:");

	// Set Volume label.
	f_setlabel("0:SWITCH SD");

	lv_label_set_text(lbl_status, "#00DDFF Status:# Flashing partition table...");
	lv_label_set_text(lbl_paths[0], "Please wait...");
	lv_label_set_text(lbl_paths[1], " ");
	manual_system_maintenance(true);

	// Prepare MBR and GPT header and partition entries and flash them.
	_sd_prepare_and_flash_mbr_gpt();

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

	// Set buttons depending on what user chose to create.
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

out:
	lv_obj_del(lbl_paths[0]);
	lv_obj_del(lbl_paths[1]);
exit:
	if (!buttons_set)
		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	// Disable partitioning button.
	if (part_info.partition_button)
		lv_btn_set_state(part_info.partition_button, LV_BTN_STATE_INA);

	return LV_RES_OK;
}

static lv_res_t _emmc_create_mbox_start_partitioning()
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] =  { "\251", "\222OK", "\251", "" };
	static const char *mbox_btn_map1[] = { "\251", "\222Flash Linux", "\222Flash Android", "\221OK", "" };
	static const char *mbox_btn_map2[] = { "\251", "\222Flash Linux", "\221OK", "" };
	static const char *mbox_btn_map3[] = { "\251", "\222Flash Android", "\221OK", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "#FF8000 eMMC Partition Manager#");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	bool buttons_set = false;

	// Use safety wait if backup is not possible.
	char *txt_buf = malloc(SZ_4K);
	strcpy(txt_buf, "#FF8000 eMMC Partition Manager#\n\nSafety wait ends in ");
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
		"#FF8000 eMMC Partition Manager#\n\n"
		"#FFDD00 Warning: Do you really want to continue?!#\n\n"
		"Press #FF8000 POWER# to Continue.\nPress #FF8000 VOL# to abort.");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	manual_system_maintenance(true);

	if (!(btn_wait() & BTN_POWER))
		goto exit;

	// Start partitioning.
	lv_mbox_set_text(mbox, "#FF8000 eMMC Partition Manager#");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	manual_system_maintenance(true);

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);

	lv_obj_t *lbl_extra = lv_label_create(mbox, NULL);
	lv_label_set_long_mode(lbl_extra, LV_LABEL_LONG_DOT);
	lv_cont_set_fit(lbl_extra, false, true);
	lv_obj_set_width(lbl_extra, (LV_HOR_RES / 9 * 6) - LV_DPI / 2);
	lv_label_set_align(lbl_extra, LV_LABEL_ALIGN_CENTER);

	lv_label_set_text(lbl_status, "#00DDFF Status:# Initializing...");
	lv_label_set_text(lbl_extra, "Please wait...");
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	manual_system_maintenance(true);

	if (!emmc_initialize(false))
	{
		lv_label_set_text(lbl_extra, "#FFDD00 Failed to init eMMC!#");
		goto exit;
	}

	emmc_set_partition(EMMC_GPP);

	if (!emummc_raw_derive_bis_keys())
	{
		lv_label_set_text(lbl_extra, "#FFDD00 For formatting USER partition,#\n#FFDD00 BIS keys are needed!#");
		emmc_end();
		goto exit;
	}

	lv_label_set_text(lbl_status, "#00DDFF Status:# Flashing partition table...");
	lv_label_set_text(lbl_extra, "Please wait...");
	manual_system_maintenance(true);

	// Prepare MBR and GPT header and partition entries and flash them.
	if (_emmc_prepare_and_flash_mbr_gpt())
		goto no_hos_user_part;

	lv_label_set_text(lbl_status, "#00DDFF Status:# Formatting USER partition...");
	lv_label_set_text(lbl_extra, "Please wait...");
	manual_system_maintenance(true);

	// Get USER partition and configure BIS and FatFS.
	LIST_INIT(gpt);
	emmc_gpt_parse(&gpt);
	emmc_part_t *user_part = emmc_part_find(&gpt, "USER");

	if (!user_part)
	{
no_hos_user_part:
		s_printf(txt_buf, "#FF0000 HOS USER partition doesn't exist!#\nRestore HOS backup first...");
		lv_label_set_text(lbl_extra, txt_buf);

		emmc_gpt_free(&gpt);
		emmc_end();

		goto exit;
	}

	// Initialize BIS for eMMC. BIS keys should be already in place.
	nx_emmc_bis_init(user_part, true, 0);

	// Set BIS size for FatFS.
	u32 user_sectors = user_part->lba_end - user_part->lba_start + 1;
	disk_set_info(DRIVE_BIS, SET_SECTOR_COUNT,  &user_sectors);

	// Enable writing.
	bool allow_writes = true;
	disk_set_info(DRIVE_BIS, SET_WRITE_PROTECT, &allow_writes);

	// Format USER partition as FAT32 with 16KB cluster and PRF2SAFE.
	u8 *buff = malloc(SZ_4M);
	int mkfs_error = f_mkfs("bis:", FM_FAT32 | FM_SFD | FM_PRF2, 16384, buff, SZ_4M);

	if (mkfs_error)
	{
		s_printf(txt_buf, "#FF0000 Failed (%d)!#\nPlease try again...\n", mkfs_error);
		lv_label_set_text(lbl_extra, txt_buf);

		free(buff);
		emmc_end();

		goto exit;
	}

	// Disable writes to BIS.
	allow_writes = false;
	disk_set_info(DRIVE_BIS, SET_WRITE_PROTECT, &allow_writes);

	// Flush BIS cache, deinit, clear BIS keys slots and reinstate SBK.
	nx_emmc_bis_end();
	hos_bis_keys_clear();
	emmc_gpt_free(&gpt);
	emmc_end();

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

	lv_label_set_text(lbl_status, "#00DDFF Status:# Done!");
	manual_system_maintenance(true);

	// Set buttons depending on what user chose to create.
	if (part_info.l4t_size && part_info.and_size)
		lv_mbox_add_btns(mbox, mbox_btn_map1, _action_part_manager_flash_options0);
	else if (part_info.l4t_size)
		lv_mbox_add_btns(mbox, mbox_btn_map2, _action_part_manager_flash_options1);
	else if (part_info.and_size)
		lv_mbox_add_btns(mbox, mbox_btn_map3, _action_part_manager_flash_options2);

	if (part_info.l4t_size || part_info.and_size)
		buttons_set = true;

	lv_obj_del(lbl_extra);

exit:
	free(txt_buf);

	if (!buttons_set)
		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	// Disable partitioning button.
	if (part_info.partition_button)
		lv_btn_set_state(part_info.partition_button, LV_BTN_STATE_INA);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_partitioning_option0(lv_obj_t *btns, const char *txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	switch (btn_idx)
	{
	case 0:
		action_ums_sd(NULL);
		return LV_RES_OK;
	case 1:
		mbox_action(btns, txt);
		_sd_create_mbox_start_partitioning();
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
		if (!part_info.emmc)
			_sd_create_mbox_start_partitioning();
		else
			_emmc_create_mbox_start_partitioning();
		return LV_RES_INV;
	}

	return LV_RES_OK;
}

static lv_res_t _create_mbox_partitioning_warn()
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

	if (!part_info.emmc)
	{
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
	}
	else
	{
		s_printf(txt_buf, "#FFDD00 Warning: This will partition the eMMC!#\n\n"
						  "#FFDD00 The USER partition will also be formatted!#");
		lv_label_set_text(lbl_status, txt_buf);
		lv_mbox_add_btns(mbox, mbox_btn_map2, _create_mbox_partitioning_option1);
	}

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	free(txt_buf);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_partitioning_android(lv_obj_t *btns, const char *txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	mbox_action(btns, txt);

	part_info.and_dynamic = !btn_idx;
	_create_mbox_partitioning_warn();

	return LV_RES_INV;
}

static lv_res_t _create_mbox_partitioning_andr_part()
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\222Dynamic", "\222Legacy", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	lv_obj_set_width(mbox, LV_HOR_RES / 10 * 5);
	lv_mbox_set_text(mbox, "#FF8000 Android Partitioning#");

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);

	lv_label_set_text(lbl_status,
		"Please select a partition scheme:\n\n"
		"#C7EA46 Dynamic:# Android 13+\n"
		"#C7EA46 Legacy:# Android 10-11\n");

	lv_mbox_add_btns(mbox, mbox_btn_map, _create_mbox_partitioning_android);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_partitioning_next(lv_obj_t *btn) {
	if (part_info.and_size)
		return _create_mbox_partitioning_andr_part();
	else
		return _create_mbox_partitioning_warn();
}

static void _update_partition_bar()
{
	lv_obj_t *h1 = lv_obj_get_parent(part_info.bar_hos);

	// Set widths based on max bar width.
	u32 total_size = (part_info.total_sct - AU_ALIGN_SECTORS) / SECTORS_PER_GB;
	u32 bar_hos_size = lv_obj_get_width(h1) * (part_info.hos_size >> 10) / total_size;
	u32 bar_emu_size = lv_obj_get_width(h1) * (part_info.emu_size >> 10) / total_size;
	u32 bar_l4t_size = lv_obj_get_width(h1) * (part_info.l4t_size >> 10) / total_size;
	u32 bar_and_size = lv_obj_get_width(h1) * (part_info.and_size >> 10) / total_size;

	// Update bar widths.
	lv_obj_set_width(part_info.bar_hos, bar_hos_size);
	lv_obj_set_width(part_info.bar_emu, bar_emu_size);
	lv_obj_set_width(part_info.bar_l4t, bar_l4t_size);
	lv_obj_set_width(part_info.bar_and, bar_and_size);

	// Re-align bars.
	lv_obj_realign(part_info.bar_emu);
	lv_obj_realign(part_info.bar_l4t);
	lv_obj_realign(part_info.bar_and);

	// Set emuMMC blending separator sizes and realign.
	lv_obj_set_width(part_info.sep_emu, bar_emu_size ? 8 : 0);
	lv_obj_realign(part_info.sep_emu);

	// Set L4T blending separator sizes and realign.
	lv_obj_set_width(part_info.sep_l4t, bar_l4t_size ? 8 : 0);
	lv_obj_realign(part_info.sep_l4t);

	// Set Android blending separator sizes and realign.
	lv_obj_set_width(part_info.sep_and, bar_and_size ? 8 : 0);
	lv_obj_realign(part_info.sep_and);

	// Re-align size labels.
	lv_obj_realign(part_info.lbl_hos);
	lv_obj_realign(part_info.lbl_emu);
	lv_obj_realign(part_info.lbl_l4t);
	lv_obj_realign(part_info.lbl_and);
	lv_obj_align(part_info.cont_lbl, part_info.bar_hos, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI * 11 - LV_DPI / 2, LV_DPI * 9 / 23);
}

static lv_res_t _action_slider_emu(lv_obj_t *slider)
{
	u32 size;
	char lbl_text[64];
	bool prev_emu_double = part_info.emu_double;
	int slide_val = lv_slider_get_value(slider);
	u32 max_emmc_size = !part_info.emmc_is_64gb ? EMU_32GB_FULL : EMU_64GB_FULL;

	part_info.emu_double = false;

	// Check that eMMC exists.
	if (!part_info.emmc_size_mb)
	{
		lv_slider_set_value(slider, 0);
		return LV_RES_OK;
	}

	// In case of upgraded eMMC, do not allow FULL sizes. Max size is always bigger than official eMMCs.
	if (max_emmc_size < part_info.emmc_size_mb)
	{
		if (slide_val == EMU_SLIDER_1X_FULL)
		{
			if (prev_emu_double)
				slide_val--;
			else
				slide_val++;
			lv_slider_set_value(slider, slide_val);
		}
		else if (slide_val == EMU_SLIDER_2X_FULL)
		{
			slide_val--;
			lv_slider_set_value(slider, slide_val);
		}
	}

	size  = (slide_val > EMU_SLIDER_1X_MAX ? (slide_val - EMU_SLIDER_1X_MAX) : slide_val) + EMU_SLIDER_OFFSET;
	size *= 1024;        // Convert to GB.
	size += EMU_RSVD_MB; // Add reserved size.

	if (slide_val == EMU_SLIDER_MIN)
		size = 0; // Reset if 0.
	else if (slide_val >= EMU_SLIDER_2X_MIN)
	{
		size *= 2;
		part_info.emu_double = true;
	}

	// Handle special cases. 2nd value is for 64GB Aula. Values already include reserved space.
	if (slide_val == EMU_SLIDER_1X_FULL)
		size = max_emmc_size;
	else if (slide_val == EMU_SLIDER_2X_FULL)
		size = 2 * max_emmc_size;

	// Sanitize sizes based on new HOS size.
	s32 hos_size = (part_info.total_sct >> 11) - 16 - size - part_info.l4t_size - part_info.and_size;
	if (hos_size > part_info.hos_min_size)
	{
		part_info.emu_size = size;
		part_info.hos_size = hos_size;

		s_printf(lbl_text, "#96FF00 %4d GiB#", hos_size >> 10);
		lv_label_set_text(part_info.lbl_hos, lbl_text);
		lv_bar_set_value(part_info.slider_bar_hos, hos_size >> 10);

		if (!part_info.emu_double)
		{
			if (slide_val != EMU_SLIDER_1X_FULL)
				s_printf(lbl_text, "#FF3C28 %4d GiB#", size >> 10);
			else
				s_printf(lbl_text, "#FF3C28 %d FULL#", size >> 10);
		}
		else
			s_printf(lbl_text, "#FFDD00 2x##FF3C28 %d GiB#", size >> 11);
		lv_label_set_text(part_info.lbl_emu, lbl_text);
	}
	else
	{
		u32 emu_size = part_info.emu_size;

		if (emu_size == max_emmc_size)
			emu_size = EMU_SLIDER_1X_FULL;
		else if (emu_size == 2 * max_emmc_size)
			emu_size = EMU_SLIDER_2X_FULL;
		else if (emu_size)
		{
			if (prev_emu_double)
				emu_size /= 2;
			emu_size -= EMU_RSVD_MB;
			emu_size /= 1024;
			emu_size -= EMU_SLIDER_OFFSET;

			if (prev_emu_double)
				emu_size += EMU_SLIDER_2X_MIN;
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

	// Sanitize sizes based on new HOS size.
	if (hos_size > part_info.hos_min_size)
	{
		if (size <= 8192)
			lv_slider_set_value(slider, size >> 10);
	}
	else
	{
		size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.and_size - 2048;
		hos_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.and_size - size;
		if (hos_size < part_info.hos_min_size || size < 8192)
		{
			lv_slider_set_value(slider, part_info.l4t_size >> 10);
			goto out;
		}
		lv_slider_set_value(slider, size >> 10);
	}

	part_info.l4t_size = size;
	part_info.hos_size = hos_size;

	s_printf(lbl_text, "#96FF00 %4d GiB#", hos_size >> 10);
	lv_label_set_text(part_info.lbl_hos, lbl_text);
	lv_bar_set_value(part_info.slider_bar_hos, hos_size >> 10);
	s_printf(lbl_text, "#00DDFF %4d GiB#", size >> 10);
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

	u32 and_size = user_size ? (user_size + AND_SYS_SIZE_MB) : 0;
	s32 hos_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.l4t_size - and_size;

	// Sanitize sizes based on new HOS size.
	if (hos_size > part_info.hos_min_size)
	{
		if (user_size <= 4096)
			lv_slider_set_value(slider, user_size >> 10);
	}
	else
	{
		and_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.l4t_size - 2048;
		hos_size = (part_info.total_sct >> 11) - 16 - part_info.emu_size - part_info.l4t_size - and_size;
		if (hos_size < part_info.hos_min_size || and_size < 8192)
		{
			lv_slider_set_value(slider, part_info.and_size >> 10);
			goto out;
		}
		user_size = and_size - AND_SYS_SIZE_MB;
		lv_slider_set_value(slider, user_size >> 10);
	}

	part_info.and_size = and_size;
	part_info.hos_size = hos_size;

	s_printf(lbl_text, "#96FF00 %4d GiB#", hos_size >> 10);
	lv_label_set_text(part_info.lbl_hos, lbl_text);
	lv_bar_set_value(part_info.slider_bar_hos, hos_size >> 10);
	s_printf(lbl_text, "#FF8000 %4d GiB#", user_size >> 10);
	lv_label_set_text(part_info.lbl_and, lbl_text);

	_update_partition_bar();

out:
	return LV_RES_OK;
}

static lv_res_t _mbox_check_files_total_size_option(lv_obj_t *btns, const char *txt)
{
	// If "don't backup" button was pressed, disable backup/restore of files.
	if (!lv_btnm_get_pressed(btns))
		part_info.backup_possible = false;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static void _create_mbox_check_files_total_size()
{
	static lv_style_t bar_hos_ind, bar_emu_ind, bar_l4t_ind, bar_and_ind;
	static lv_style_t sep_emu_bg, sep_l4t_bg, sep_and_bg;

	// Set HOS bar style.
	lv_style_copy(&bar_hos_ind, lv_theme_get_current()->bar.indic);
	bar_hos_ind.body.main_color = LV_COLOR_HEX(0x96FF00);
	bar_hos_ind.body.grad_color = bar_hos_ind.body.main_color;

	// Set emuMMC bar style.
	lv_style_copy(&bar_emu_ind, lv_theme_get_current()->bar.indic);
	bar_emu_ind.body.main_color = LV_COLOR_HEX(0xFF3C28);
	bar_emu_ind.body.grad_color = bar_emu_ind.body.main_color;

	// Set L4T bar style.
	lv_style_copy(&bar_l4t_ind, lv_theme_get_current()->bar.indic);
	bar_l4t_ind.body.main_color = LV_COLOR_HEX(0x00DDFF);
	bar_l4t_ind.body.grad_color = bar_l4t_ind.body.main_color;

	// Set GPT bar style.
	lv_style_copy(&bar_and_ind, lv_theme_get_current()->bar.indic);
	bar_and_ind.body.main_color = LV_COLOR_HEX(0xC000FF);
	bar_and_ind.body.grad_color = bar_and_ind.body.main_color;

	// Set separator styles.
	lv_style_copy(&sep_emu_bg, lv_theme_get_current()->cont);
	sep_emu_bg.body.main_color = LV_COLOR_HEX(0xFF3C28);
	sep_emu_bg.body.grad_color = sep_emu_bg.body.main_color;
	sep_emu_bg.body.radius = 0;
	lv_style_copy(&sep_l4t_bg, &sep_emu_bg);
	sep_l4t_bg.body.main_color = LV_COLOR_HEX(0x00DDFF);
	sep_l4t_bg.body.grad_color = sep_l4t_bg.body.main_color;
	lv_style_copy(&sep_and_bg, &sep_emu_bg);
	sep_and_bg.body.main_color = LV_COLOR_HEX(0xC000FF);
	sep_and_bg.body.grad_color = sep_and_bg.body.main_color;

	char *txt_buf = malloc(SZ_8K);

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	static const char *mbox_btn_map2[] = { "\222Don't Backup", "\222OK", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "Analyzing SD card usage. This might take a while...");

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);
	manual_system_maintenance(true);

	char *path = malloc(0x1000);
	u32 total_files = 0;
	u32 total_size = 0;
	path[0] = 0;

	// Check total size of files.
	int res = _stat_and_copy_files("sd:", NULL, path, &total_files, &total_size, NULL);

	// Not more than 1.2GB.
	part_info.backup_possible = !res && !(total_size > (RAM_DISK_SZ - SZ_16M)); // Account for alignment.

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
			"#FFDD00 The SD Card cannot be backed up automatically!#\n"
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

	// Calculate MBR partitions size.
	total_size = (sd_storage.sec_cnt - AU_ALIGN_SECTORS) / SECTORS_PER_GB;
	u32 bar_hos_size = lv_obj_get_width(h1) * (mbr.partitions[0].size_sct / SECTORS_PER_GB) / total_size;
	u32 bar_emu_size = 0;
	for (u32 i = 1; i < 4; i++)
		if (mbr.partitions[i].type == 0xE0)
			bar_emu_size += mbr.partitions[i].size_sct;
	bar_emu_size = lv_obj_get_width(h1) * (bar_emu_size / SECTORS_PER_GB) / total_size;

	u32 bar_l4t_size = 0;
	for (u32 i = 1; i < 4; i++)
		if (mbr.partitions[i].type == 0x83)
			bar_l4t_size += mbr.partitions[i].size_sct;
	bar_l4t_size = lv_obj_get_width(h1) * (bar_l4t_size / SECTORS_PER_GB) / total_size;

	u32 bar_and_size = lv_obj_get_width(h1) - bar_hos_size - bar_emu_size - bar_l4t_size;

	// Create HOS bar.
	lv_obj_t *bar_mbr_hos = lv_bar_create(h1, NULL);
	lv_obj_set_size(bar_mbr_hos, bar_hos_size, LV_DPI / 3);
	lv_bar_set_range(bar_mbr_hos, 0, 1);
	lv_bar_set_value(bar_mbr_hos, 1);
	lv_bar_set_style(bar_mbr_hos, LV_BAR_STYLE_INDIC, &bar_hos_ind);
	lv_obj_align(bar_mbr_hos, lbl_part, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 6);

	// Create emuMMC bar.
	lv_obj_t *bar_mbr_emu = lv_bar_create(h1, bar_mbr_hos);
	lv_obj_set_size(bar_mbr_emu, bar_emu_size, LV_DPI / 3);
	lv_bar_set_style(bar_mbr_emu, LV_BAR_STYLE_INDIC, &bar_emu_ind);
	lv_obj_align(bar_mbr_emu, bar_mbr_hos, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	// Create L4T bar.
	lv_obj_t *bar_mbr_l4t = lv_bar_create(h1, bar_mbr_hos);
	lv_obj_set_size(bar_mbr_l4t, bar_l4t_size, LV_DPI / 3);
	lv_bar_set_style(bar_mbr_l4t, LV_BAR_STYLE_INDIC, &bar_l4t_ind);
	lv_obj_align(bar_mbr_l4t, bar_mbr_emu, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	// Create GPT bar.
	lv_obj_t *bar_mbr_gpt = lv_bar_create(h1, bar_mbr_hos);
	lv_obj_set_size(bar_mbr_gpt, bar_and_size > 1 ? bar_and_size : 0, LV_DPI / 3);
	lv_bar_set_style(bar_mbr_gpt, LV_BAR_STYLE_INDIC, &bar_and_ind);
	lv_obj_align(bar_mbr_gpt, bar_mbr_l4t, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	// Create emuMMC separator.
	lv_obj_t *sep_mbr_emu = lv_cont_create(h1, NULL);
	lv_obj_set_size(sep_mbr_emu, bar_emu_size ? 8 : 0, LV_DPI / 3);
	lv_obj_set_style(sep_mbr_emu, &sep_emu_bg);
	lv_obj_align(sep_mbr_emu, bar_mbr_hos, LV_ALIGN_OUT_RIGHT_MID, -4, 0);

	// Create L4T separator.
	lv_obj_t *sep_mbr_l4t = lv_cont_create(h1, sep_mbr_emu);
	lv_obj_set_size(sep_mbr_l4t, bar_l4t_size ? 8 : 0, LV_DPI / 3);
	lv_obj_set_style(sep_mbr_l4t, &sep_l4t_bg);
	lv_obj_align(sep_mbr_l4t, bar_mbr_emu, LV_ALIGN_OUT_RIGHT_MID, -4, 0);

	// Create GPT separator.
	lv_obj_t *sep_mbr_gpt = lv_cont_create(h1, sep_mbr_emu);
	lv_obj_set_size(sep_mbr_gpt, bar_and_size ? (bar_and_size > 1 ? 8 : 0) : 0, LV_DPI / 3);
	lv_obj_set_style(sep_mbr_gpt, &sep_and_bg);
	lv_obj_align(sep_mbr_gpt, bar_mbr_l4t, LV_ALIGN_OUT_RIGHT_MID, -4, 0);

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

	if (!part_info.backup_possible)
		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	else
		lv_mbox_add_btns(mbox, mbox_btn_map2, _mbox_check_files_total_size_option);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

	free(txt_buf);
	free(path);
}

static lv_res_t _action_fix_mbr_gpt(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);
	lv_mbox_set_text(mbox, "#FF8000 Fix Hybrid MBR#");

	lv_obj_t *lbl_status = lv_label_create(mbox, NULL);
	lv_label_set_recolor(lbl_status, true);

	mbr_t mbr[2] = { 0 };
	gpt_t *gpt = zalloc(sizeof(gpt_t));
	gpt_header_t gpt_hdr_backup = { 0 };

	bool has_mbr_attributes   = false;
	bool hybrid_mbr_changed   = false;
	bool gpt_partition_exists = false;
	int  gpt_oob_empty_part_no = 0;
	int  gpt_emummc_migrate_no = 0;

	// Try to init sd card. No need for valid MBR.
	if (!sd_mount() && !sd_get_card_initialized())
	{
		lv_label_set_text(lbl_status, "#FFDD00 Failed to init SD!#");
		goto out;
	}

	sdmmc_storage_read(&sd_storage, 0, 1, &mbr[0]);
	sdmmc_storage_read(&sd_storage, 1, sizeof(gpt_t) >> 9, gpt);

	memcpy(&mbr[1], &mbr[0], sizeof(mbr_t));

	sd_unmount();

	// Check for secret MBR attributes.
	if (gpt->entries[0].part_guid[7])
		has_mbr_attributes = true;

	// Check if there's a GPT Protective partition.
	for (u32 i = 0; i < 4; i++)
	{
		if (mbr[0].partitions[i].type == 0xEE)
			gpt_partition_exists = true;
	}

	// Check if GPT is valid.
	if (!gpt_partition_exists || memcmp(&gpt->header.signature, "EFI PART", 8) || gpt->header.num_part_ents > 128)
	{
		lv_label_set_text(lbl_status, "#FFDD00 Warning:# No valid GPT was found!");

		gpt_partition_exists = false;

		if (has_mbr_attributes)
			goto check_changes;
		else
			goto out;
	}

	sdmmc_storage_read(&sd_storage, gpt->header.alt_lba, 1, &gpt_hdr_backup);

	// Parse GPT.
	LIST_INIT(gpt_parsed);
	for (u32 i = 0; i < gpt->header.num_part_ents; i++)
	{
		// Check if partition is out of bounds or empty.
		if ( gpt->entries[i].lba_start <   gpt->header.first_use_lba ||
			 gpt->entries[i].lba_start >=  gpt->entries[i].lba_end   ||
			!gpt->entries[i].lba_end)
		{
			gpt_oob_empty_part_no++;
			continue;
		}

		emmc_part_t *part = (emmc_part_t *)zalloc(sizeof(emmc_part_t));

		part->index = i;
		part->lba_start = gpt->entries[i].lba_start;
		part->lba_end = gpt->entries[i].lba_end;

		// ASCII conversion. Copy only the LSByte of the UTF-16LE name.
		for (u32 j = 0; j < 36; j++)
			part->name[j] = gpt->entries[i].name[j];
		part->name[35] = 0;

		list_append(&gpt_parsed, &part->link);
	}

	// Set FAT and emuMMC partitions.
	u32 mbr_idx = 1;
	bool found_hos_data = false;
	u32 emummc_mbr_part_idx[2] = {0};
	LIST_FOREACH_ENTRY(emmc_part_t, part, &gpt_parsed, link)
	{
		// FatFS simple GPT found a fat partition, set it.
		if (sd_fs.part_type && !part->index)
		{
			mbr[1].partitions[0].type = sd_fs.fs_type == FS_EXFAT ? 0x7 : 0xC;
			mbr[1].partitions[0].start_sct = part->lba_start;
			mbr[1].partitions[0].size_sct  = part->lba_end - part->lba_start + 1;
		}

		// FatFS simple GPT didn't find a fat partition as the first one.
		if (!sd_fs.part_type && !found_hos_data && !strcmp(part->name, "hos_data"))
		{
			mbr[1].partitions[0].type = 0xC;
			mbr[1].partitions[0].start_sct = part->lba_start;
			mbr[1].partitions[0].size_sct  = part->lba_end - part->lba_start + 1;
			found_hos_data = true;
		}

		// Set up to max 2 emuMMC partitions.
		if (!strcmp(part->name, "emummc") || !strcmp(part->name, "emummc2"))
		{
			mbr[1].partitions[mbr_idx].type = 0xE0;
			mbr[1].partitions[mbr_idx].start_sct = part->lba_start;
			mbr[1].partitions[mbr_idx].size_sct  = part->lba_end - part->lba_start + 1;
			if (!strcmp(part->name, "emummc"))
				emummc_mbr_part_idx[0] = mbr_idx;
			else
				emummc_mbr_part_idx[1] = mbr_idx;
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
	for (u32 i = 1; i < 4; i++)
	{
		if ((mbr[0].partitions[i].type      != mbr[1].partitions[i].type)      ||
			(mbr[0].partitions[i].start_sct != mbr[1].partitions[i].start_sct) ||
			(mbr[0].partitions[i].size_sct  != mbr[1].partitions[i].size_sct))
		{
			// Check if original MBR already has an emuMMC and use it as source of truth.
			if (mbr[0].partitions[i].type == 0xE0)
			{
				memcpy(&mbr[1].partitions[i], &mbr[0].partitions[i], sizeof(mbr_part_t));
				gpt_emummc_migrate_no++;
				continue;
			}
			else
				hybrid_mbr_changed = true;
			break;
		}
	}

check_changes:
	if (!hybrid_mbr_changed && !has_mbr_attributes && !gpt_emummc_migrate_no)
	{
		lv_label_set_text(lbl_status, "#96FF00 Warning:# The Hybrid MBR needs no change!#");
		goto out;
	}

	char *txt_buf = malloc(SZ_16K);

	if (hybrid_mbr_changed)
	{
		// Current MBR info.
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

		// New MBR info.
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
	}
	else if (has_mbr_attributes || gpt_emummc_migrate_no || gpt_oob_empty_part_no)
	{
		s_printf(txt_buf, "#00DDFF The following need to be corrected:#\n");
		if (has_mbr_attributes)
			s_printf(txt_buf + strlen(txt_buf), "- MBR attributes\n");
		if (gpt_emummc_migrate_no)
			s_printf(txt_buf + strlen(txt_buf), "- emuMMC GPT Partition address and size\n");
		if (gpt_oob_empty_part_no)
			s_printf(txt_buf + strlen(txt_buf), "- GPT OOB/Empty Partitions (removal)\n");
	}

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
		bool has_gpt_changes = false;

		sd_mount();

		// Write MBR.
		if (hybrid_mbr_changed)
			sdmmc_storage_write(&sd_storage, 0, 1, &mbr[1]);

		// Fix MBR secret attributes.
		if (has_mbr_attributes)
		{
			// Clear secret attributes.
			gpt->entries[0].part_guid[7] = 0;
			has_gpt_changes = gpt_partition_exists;

			if (!has_gpt_changes)
			{
				// Only write the relevant sector if the only change is MBR attributes.
				sdmmc_storage_write(&sd_storage, 2, 1, &gpt->entries[0]);
			}
		}

		if (gpt_emummc_migrate_no)
		{
			u32 emu_idx = 0;
			for (u32 i = 0; i < gpt->header.num_part_ents; i++)
			{
				if (!memcmp(gpt->entries[i].name, (u16[]) { 'e', 'm', 'u', 'm', 'm', 'c' }, 12))
				{
					u32 idx = emummc_mbr_part_idx[emu_idx];
					gpt->entries[i].lba_start = mbr[0].partitions[idx].start_sct;
					gpt->entries[i].lba_end   = mbr[0].partitions[idx].start_sct + mbr[0].partitions[idx].size_sct - 1;
					gpt_emummc_migrate_no--;
					emu_idx++;

					has_gpt_changes = true;
				}

				if (i > 126 || !gpt_emummc_migrate_no)
					break;
			}
		}

		if (gpt_oob_empty_part_no)
		{
			u32 part_idx = 0;
			for (u32 i = 0; i < gpt->header.num_part_ents; i++)
			{
				if ( gpt->entries[i].lba_start <   gpt->header.first_use_lba ||
					 gpt->entries[i].lba_start >=  gpt->entries[i].lba_end   ||
					!gpt->entries[i].lba_end)
				{
					continue;
				}

				if (part_idx != i)
					memcpy(&gpt->entries[part_idx], &gpt->entries[i], sizeof(gpt_entry_t));
				part_idx++;
			}
			gpt->header.num_part_ents -= gpt_oob_empty_part_no;
			has_gpt_changes = true;
		}

		if (has_gpt_changes)
		{
			// Fix GPT CRC32s.
			u32 entries_size = sizeof(gpt_entry_t) * gpt->header.num_part_ents;
			gpt->header.part_ents_crc32 = crc32_calc(0, (const u8 *)gpt->entries, entries_size);
			gpt->header.crc32 = 0; // Set to 0 for calculation.
			gpt->header.crc32 = crc32_calc(0, (const u8 *)&gpt->header, gpt->header.size);

			gpt_hdr_backup.part_ents_crc32 = gpt->header.part_ents_crc32;
			gpt_hdr_backup.crc32 = 0; // Set to 0 for calculation.
			gpt_hdr_backup.crc32 = crc32_calc(0, (const u8 *)&gpt_hdr_backup, gpt_hdr_backup.size);

			// Write main GPT.
			u32 aligned_entries_size = ALIGN(entries_size, SD_BLOCKSIZE);
			sdmmc_storage_write(&sd_storage, gpt->header.my_lba, (sizeof(gpt_header_t) + aligned_entries_size) >> 9, gpt);

			// Write backup GPT partition table.
			sdmmc_storage_write(&sd_storage, gpt_hdr_backup.part_ent_lba, aligned_entries_size >> 9, gpt->entries);

			// Write backup GPT header.
			sdmmc_storage_write(&sd_storage, gpt_hdr_backup.my_lba, 1, &gpt_hdr_backup);
		}

		sd_unmount();

		lv_label_set_text(lbl_status, "#96FF00 The new Hybrid MBR was written successfully!#");
	}
	else
		lv_label_set_text(lbl_status, "#FFDD00 Warning: The Hybrid MBR Fix was canceled!#");

out:
	free(gpt);

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

lv_res_t create_window_partition_manager(bool emmc)
{
	lv_obj_t *win;

	if (!emmc)
	{
		win = nyx_create_standard_window(SYMBOL_SD" SD Partition Manager");
		lv_win_add_btn(win, NULL, SYMBOL_MODULES_ALT" Fix Hybrid MBR/GPT", _action_fix_mbr_gpt);
	}
	else
		win = nyx_create_standard_window(SYMBOL_CHIP" eMMC Partition Manager");

	static lv_style_t bar_hos_bg, bar_emu_bg, bar_l4t_bg, bar_and_bg;
	static lv_style_t bar_hos_ind, bar_emu_ind, bar_l4t_ind, bar_and_ind;
	static lv_style_t bar_emu_btn, bar_l4t_btn, bar_and_btn;
	static lv_style_t sep_emu_bg, sep_l4t_bg, sep_and_bg;

	// Set HOS bar styles.
	lv_style_copy(&bar_hos_bg, lv_theme_get_current()->bar.bg);
	bar_hos_bg.body.main_color = LV_COLOR_HEX(0x4A8000);
	bar_hos_bg.body.grad_color = bar_hos_bg.body.main_color;
	lv_style_copy(&bar_hos_ind, lv_theme_get_current()->bar.indic);
	bar_hos_ind.body.main_color = LV_COLOR_HEX(0x96FF00);
	bar_hos_ind.body.grad_color = bar_hos_ind.body.main_color;

	// Set eMUMMC bar styles.
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

	// Set L4T bar styles.
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

	// Set Android bar styles.
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

	u32 emmc_size = 0;
	if (!emmc)
	{
		if (!sd_mount())
		{
			lv_obj_t *lbl = lv_label_create(h1, NULL);
			lv_label_set_recolor(lbl, true);
			lv_label_set_text(lbl, "#FFDD00 Failed to init SD!#");
			return LV_RES_OK;
		}

		if (emmc_initialize(false))
		{
			emmc_set_partition(EMMC_GPP);
			emmc_size = emmc_storage.sec_cnt >> 11;
			emmc_end();
		}
	}
	else
	{
		if (!emmc_initialize(false))
		{
			lv_obj_t *lbl = lv_label_create(h1, NULL);
			lv_label_set_recolor(lbl, true);
			lv_label_set_text(lbl, "#FFDD00 Failed to init eMMC!#");
			return LV_RES_OK;
		}
		emmc_set_partition(EMMC_GPP);
	}

	memset(&part_info, 0, sizeof(partition_ctxt_t));
	if (!emmc)
		_create_mbox_check_files_total_size();

	char *txt_buf = malloc(SZ_8K);

	part_info.emmc      = emmc;
	part_info.storage   = !emmc ? &sd_storage : &emmc_storage;
	part_info.total_sct = part_info.storage->sec_cnt;
	if (emmc)
		part_info.total_sct -= HOS_USER_SECTOR; // Reserved HOS partitions.

	// Align down total size to ensure alignment of all partitions after HOS one.
	part_info.alignment  = part_info.total_sct - ALIGN_DOWN(part_info.total_sct, AU_ALIGN_SECTORS);
	part_info.total_sct -= part_info.alignment;

	u32 extra_sct = AU_ALIGN_SECTORS + 0x400000; // Reserved 16MB alignment for FAT partition + 2GB.

	// Set initial HOS partition size, so the correct cluster size can be selected.
	part_info.hos_size = (part_info.total_sct >> 11) - 16; // Important if there's no slider change.

	// Check if eMMC should be 64GB (Aula).
	part_info.emmc_is_64gb = fuse_read_hw_type() == FUSE_NX_HW_TYPE_AULA;

	// Set actual eMMC size.
	part_info.emmc_size_mb = emmc_size;

	// Set HOS FAT or USER minimum size.
	part_info.hos_min_size = !emmc? HOS_FAT_MIN_SIZE_MB : HOS_USER_MIN_SIZE_MB;

	// Read current MBR.
	mbr_t mbr = { 0 };
	sdmmc_storage_read(part_info.storage, 0, 1, &mbr);

	u32 bar_hos_size = lv_obj_get_width(h1);
	u32 bar_emu_size = 0;
	u32 bar_l4t_size = 0;
	u32 bar_and_size = 0;

	lv_obj_t *lbl = lv_label_create(h1, NULL);
	lv_label_set_recolor(lbl, true);
	lv_label_set_text(lbl, "Choose #FFDD00 new# partition layout:");

	// Create disk layout blocks.
	// HOS partition block.
	lv_obj_t *bar_hos = lv_bar_create(h1, NULL);
	lv_obj_set_size(bar_hos, bar_hos_size, LV_DPI / 2);
	lv_bar_set_range(bar_hos, 0, 1);
	lv_bar_set_value(bar_hos, 1);
	lv_bar_set_style(bar_hos, LV_BAR_STYLE_INDIC, &bar_hos_ind);
	lv_obj_align(bar_hos, lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 6);
	part_info.bar_hos = bar_hos;

	// emuMMC partition block.
	lv_obj_t *bar_emu = lv_bar_create(h1, bar_hos);
	lv_obj_set_size(bar_emu, bar_emu_size, LV_DPI / 2);
	lv_bar_set_style(bar_emu, LV_BAR_STYLE_INDIC, &bar_emu_ind);
	lv_obj_align(bar_emu, bar_hos, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	part_info.bar_emu = bar_emu;

	// L4T partition block.
	lv_obj_t *bar_l4t = lv_bar_create(h1, bar_hos);
	lv_obj_set_size(bar_l4t, bar_l4t_size, LV_DPI / 2);
	lv_bar_set_style(bar_l4t, LV_BAR_STYLE_INDIC, &bar_l4t_ind);
	lv_obj_align(bar_l4t, bar_emu, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	part_info.bar_l4t = bar_l4t;

	// Android partition block.
	lv_obj_t *bar_and = lv_bar_create(h1, bar_hos);
	lv_obj_set_size(bar_and, bar_and_size, LV_DPI / 2);
	lv_bar_set_style(bar_and, LV_BAR_STYLE_INDIC, &bar_and_ind);
	lv_obj_align(bar_and, bar_l4t, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	part_info.bar_and = bar_and;

	// HOS partition block.
	lv_obj_t *sep_emu = lv_cont_create(h1, NULL);
	lv_cont_set_fit(sep_emu, false, false);
	lv_obj_set_size(sep_emu, 0, LV_DPI / 2); // 8.
	lv_obj_set_style(sep_emu, &sep_emu_bg);
	lv_obj_align(sep_emu, bar_hos, LV_ALIGN_OUT_RIGHT_MID, -4, 0);
	part_info.sep_emu = sep_emu;

	// Create disk layout blending separators.
	lv_obj_t *sep_l4t = lv_cont_create(h1, sep_emu);
	lv_obj_set_style(sep_l4t, &sep_l4t_bg);
	lv_obj_align(sep_l4t, bar_emu, LV_ALIGN_OUT_RIGHT_MID, -4, 0);
	part_info.sep_l4t = sep_l4t;

	lv_obj_t *sep_and = lv_cont_create(h1, sep_emu);
	lv_obj_set_style(sep_and, &sep_and_bg);
	lv_obj_align(sep_and, bar_l4t, LV_ALIGN_OUT_RIGHT_MID, -4, 0);
	part_info.sep_and = sep_and;

	// Create slider type labels.
	lv_obj_t *cont_lbl_hos = lv_cont_create(h1, NULL);
	lv_cont_set_fit(cont_lbl_hos, false, true);
	lv_obj_set_width(cont_lbl_hos, LV_DPI * 17 / 7);
	lv_obj_t *lbl_hos = lv_label_create(cont_lbl_hos, NULL);
	lv_label_set_recolor(lbl_hos, true);
	lv_label_set_static_text(lbl_hos, !emmc ? "#96FF00 "SYMBOL_DOT" HOS (FAT32):#" :
											  "#96FF00 "SYMBOL_DOT" eMMC (USER):#");
	lv_obj_align(lbl_hos, bar_hos, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);

	lv_obj_t *lbl_emu = lbl_hos;
	if (!emmc)
	{
		lbl_emu = lv_label_create(h1, lbl_hos);
		lv_label_set_static_text(lbl_emu, "#FF3C28 "SYMBOL_DOT" emuMMC (RAW):#");
		lv_obj_align(lbl_emu, lbl_hos, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
	}

	lv_obj_t *lbl_l4t = lv_label_create(h1, lbl_hos);
	lv_label_set_static_text(lbl_l4t, "#00DDFF "SYMBOL_DOT" Linux (EXT4):#");
	lv_obj_align(lbl_l4t, lbl_emu, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	lv_obj_t *lbl_and = lv_label_create(h1, lbl_hos);
	lv_label_set_static_text(lbl_and, "#FF8000 "SYMBOL_DOT" Android (USER):#");
	lv_obj_align(lbl_and, lbl_l4t, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create HOS size slider. Non-interactive.
	lv_obj_t *slider_bar_hos = lv_bar_create(h1, NULL);
	lv_obj_set_size(slider_bar_hos, LV_DPI * 7, LV_DPI * 3 / 17);
	lv_bar_set_range(slider_bar_hos, 0, (part_info.total_sct - AU_ALIGN_SECTORS) / SECTORS_PER_GB);
	lv_bar_set_value(slider_bar_hos, (part_info.total_sct - AU_ALIGN_SECTORS) / SECTORS_PER_GB);
	lv_bar_set_style(slider_bar_hos, LV_SLIDER_STYLE_BG, &bar_hos_bg);
	lv_bar_set_style(slider_bar_hos, LV_SLIDER_STYLE_INDIC, &bar_hos_ind);
	lv_obj_align(slider_bar_hos, cont_lbl_hos, LV_ALIGN_OUT_RIGHT_MID, LV_DPI, 0);
	part_info.slider_bar_hos = slider_bar_hos;

	lv_obj_t *slider_emu = slider_bar_hos;
	if (!emmc)
	{
		// Create emuMMC size slider.
		slider_emu = lv_slider_create(h1, NULL);
		lv_obj_set_size(slider_emu, LV_DPI * 7, LV_DPI / 3);
		lv_slider_set_range(slider_emu, EMU_SLIDER_MIN, EMU_SLIDER_MAX);
		lv_slider_set_value(slider_emu, EMU_SLIDER_MIN);
		lv_slider_set_style(slider_emu, LV_SLIDER_STYLE_BG, &bar_emu_bg);
		lv_slider_set_style(slider_emu, LV_SLIDER_STYLE_INDIC, &bar_emu_ind);
		lv_slider_set_style(slider_emu, LV_SLIDER_STYLE_KNOB, &bar_emu_btn);
		lv_obj_align(slider_emu, slider_bar_hos, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3 + 5);
		lv_slider_set_action(slider_emu, _action_slider_emu);
	}

	// Create L4T size slider.
	lv_obj_t *slider_l4t = lv_slider_create(h1, NULL);
	lv_obj_set_size(slider_l4t, LV_DPI * 7, LV_DPI / 3);
	lv_slider_set_range(slider_l4t, 0, (part_info.total_sct - extra_sct) / SECTORS_PER_GB);
	lv_slider_set_value(slider_l4t, 0);
	lv_slider_set_style(slider_l4t, LV_SLIDER_STYLE_BG, &bar_l4t_bg);
	lv_slider_set_style(slider_l4t, LV_SLIDER_STYLE_INDIC, &bar_l4t_ind);
	lv_slider_set_style(slider_l4t, LV_SLIDER_STYLE_KNOB, &bar_l4t_btn);
	lv_obj_align(slider_l4t, slider_emu, LV_ALIGN_OUT_BOTTOM_LEFT, 0, !emmc ? (LV_DPI / 3 - 3) : (LV_DPI / 3 + 5));
	lv_slider_set_action(slider_l4t, _action_slider_l4t);

	// Create Android size slider.
	lv_obj_t *slider_and = lv_slider_create(h1, NULL);
	lv_obj_set_size(slider_and, LV_DPI * 7, LV_DPI / 3);
	lv_slider_set_range(slider_and, 0, (part_info.total_sct - extra_sct) / SECTORS_PER_GB - (AND_SYS_SIZE_MB / 1024)); // Subtract android reserved size.
	lv_slider_set_value(slider_and, 0);
	lv_slider_set_style(slider_and, LV_SLIDER_STYLE_BG, &bar_and_bg);
	lv_slider_set_style(slider_and, LV_SLIDER_STYLE_INDIC, &bar_and_ind);
	lv_slider_set_style(slider_and, LV_SLIDER_STYLE_KNOB, &bar_and_btn);
	lv_obj_align(slider_and, slider_l4t, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3 - 3);
	lv_slider_set_action(slider_and, _action_slider_and);

	// Create container for the labels.
	lv_obj_t *cont_lbl = lv_cont_create(h1, NULL);
	lv_cont_set_fit(cont_lbl, false, true);
	lv_obj_set_width(cont_lbl, LV_DPI * 3 / 2);
	part_info.cont_lbl = cont_lbl;

	// Create HOS size label.
	lv_obj_t *lbl_sl_hos = lv_label_create(cont_lbl, NULL);
	lv_label_set_recolor(lbl_sl_hos, true);
	lv_label_set_align(lbl_sl_hos, LV_LABEL_ALIGN_RIGHT);
	s_printf(txt_buf, "#96FF00 %4d GiB#", (part_info.total_sct - AU_ALIGN_SECTORS) >> 11 >> 10);
	lv_label_set_text(lbl_sl_hos, txt_buf);
	lv_obj_align(lbl_sl_hos, cont_lbl, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
	part_info.lbl_hos = lbl_sl_hos;

	// Create emuMMC size label.
	part_info.lbl_emu = lbl_sl_hos;
	if (!emmc)
	{
		lv_obj_t *lbl_sl_emu = lv_label_create(cont_lbl, lbl_sl_hos);
		lv_label_set_text(lbl_sl_emu, "#FF3C28    0 GiB#");
		lv_obj_align(lbl_sl_emu, lbl_sl_hos, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, LV_DPI / 3);
		part_info.lbl_emu = lbl_sl_emu;
	}

	// Create L4T size label.
	lv_obj_t *lbl_sl_l4t = lv_label_create(cont_lbl, lbl_sl_hos);
	lv_label_set_text(lbl_sl_l4t, "#00DDFF    0 GiB#");
	lv_obj_align(lbl_sl_l4t, part_info.lbl_emu, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, LV_DPI / 3);
	part_info.lbl_l4t = lbl_sl_l4t;

	// Create Android size label.
	lv_obj_t *lbl_sl_and = lv_label_create(cont_lbl, lbl_sl_hos);
	lv_label_set_text(lbl_sl_and, "#FF8000    0 GiB#");
	lv_obj_align(lbl_sl_and, lbl_sl_l4t, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, LV_DPI / 3);
	part_info.lbl_and = lbl_sl_and;

	lv_obj_align(cont_lbl, bar_hos, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI * 11 - LV_DPI / 2, LV_DPI * 9 / 23);

	// Set partition manager notes.
	lv_obj_t *lbl_notes = lv_label_create(h1, NULL);
	lv_label_set_recolor(lbl_notes, true);
	lv_label_set_style(lbl_notes, &hint_small_style);
	if (!emmc)
	{
		lv_label_set_static_text(lbl_notes,
			"Note 1: Only up to #C7EA46 1.2GB# can be backed up. If more, you will be asked to back them manually at the next step.\n"
			"Note 2: Resized emuMMC formats the USER partition. A save data manager can be used to move them over.\n"
			"Note 3: The #C7EA46 Flash Linux# and #C7EA46 Flash Android# will flash files if suitable partitions and installer files are found.\n");
		lv_obj_align(lbl_notes, lbl_and, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 6 * 2);
	}
	else
	{
		lv_label_set_static_text(lbl_notes,
			"Note 1: Any partition existing after the selected ones gets removed from the table.\n"
			"Note 2: The HOS USER partition gets formatted. A save data manager can be used to move them over.\n"
			"Note 3: The #C7EA46 Flash Linux# and #C7EA46 Flash Android# will flash files if suitable partitions and installer files are found.\n");
		lv_obj_align(lbl_notes, lbl_and, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 6 * 4);
	}

	lv_obj_t *btn1 = NULL;
	lv_obj_t *label_btn = NULL;
	if (!emmc)
	{
		// Create UMS button.
		btn1 = lv_btn_create(h1, NULL);
		lv_obj_t *label_btn = lv_label_create(btn1, NULL);
		lv_btn_set_fit(btn1, true, true);
		lv_label_set_static_text(label_btn, SYMBOL_USB"  SD UMS");
		lv_obj_align(btn1, h1, LV_ALIGN_IN_TOP_LEFT, 0, LV_DPI * 5);
		lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, _action_part_manager_ums_sd);
	}

	// Create Flash Linux button.
	btn_flash_l4t = lv_btn_create(h1, NULL);
	label_btn = lv_label_create(btn_flash_l4t, NULL);
	lv_btn_set_fit(btn_flash_l4t, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_DOWNLOAD"  Flash Linux");
	if (!emmc)
		lv_obj_align(btn_flash_l4t, btn1, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 3, 0);
	else
		lv_obj_align(btn_flash_l4t, h1, LV_ALIGN_IN_TOP_LEFT, 0, LV_DPI * 5);
	lv_btn_set_action(btn_flash_l4t, LV_BTN_ACTION_CLICK, _action_check_flash_linux);

	// Disable Flash Linux button if partition not found.
	u32 size_sct = _get_available_l4t_partition();
	if (!l4t_flash_ctxt.offset_sct || size_sct < 0x800000)
	{
		lv_obj_set_click(btn_flash_l4t, false);
		lv_btn_set_state(btn_flash_l4t, LV_BTN_STATE_INA);
	}

	int part_type_and = _get_available_android_partition();

	// Create Flash Android button.
	btn_flash_android = lv_btn_create(h1, NULL);
	label_btn = lv_label_create(btn_flash_android, NULL);
	lv_btn_set_fit(btn_flash_android, true, true);
	switch (part_type_and)
	{
	case 0: // Disable Flash Android button if partition not found.
		lv_label_set_static_text(label_btn, SYMBOL_DOWNLOAD"  Flash Android");
		lv_obj_set_click(btn_flash_android, false);
		lv_btn_set_state(btn_flash_android, LV_BTN_STATE_INA);
		break;
	case 1: // Android 10/11.
		lv_label_set_static_text(label_btn, SYMBOL_DOWNLOAD"  Flash Android 10/11");
		break;
	case 2: // Android 13+
		lv_label_set_static_text(label_btn, SYMBOL_DOWNLOAD"  Flash Android 13+");
		break;
	}
	lv_obj_align(btn_flash_android, btn_flash_l4t, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 3, 0);
	lv_btn_set_action(btn_flash_android, LV_BTN_ACTION_CLICK, _action_flash_android);

	// Create next step button.
	btn1 = lv_btn_create(h1, NULL);
	label_btn = lv_label_create(btn1, NULL);
	lv_btn_set_fit(btn1, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_SD"  Next Step");
	lv_obj_align(btn1, h1, LV_ALIGN_IN_TOP_RIGHT, 0, LV_DPI * 5);
	lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, _create_mbox_partitioning_next);
	part_info.partition_button = btn1;

	free(txt_buf);

	if (!emmc)
		sd_unmount();
	else
		emmc_end();

	return LV_RES_OK;
}

lv_res_t create_window_sd_partition_manager(lv_obj_t *btn)   { return create_window_partition_manager(false); }
lv_res_t create_window_emmc_partition_manager(lv_obj_t *btn) { return create_window_partition_manager(true);  }
