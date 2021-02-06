/*
 * Ramdisk driver for Tegra X1
 *
 * Copyright (c) 2019-2021 CTCaer
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

#include "ramdisk.h"
#include <libs/fatfs/diskio.h>
#include <mem/heap.h>
#include <utils/types.h>

#include <memory_map.h>

static u32 disk_size = 0;

int ram_disk_init(FATFS *ram_fs, u32 ramdisk_size)
{
	int res = 0;
	disk_size = ramdisk_size;

	// If ramdisk is not raw, format it.
	if (ram_fs)
	{
		u8 *buf = malloc(0x400000);

		// Set ramdisk size.
		ramdisk_size >>= 9;
		disk_set_info(DRIVE_RAM, SET_SECTOR_COUNT, &ramdisk_size);

		// Unmount ramdisk.
		f_mount(NULL, "ram:", 1);

		// Format as exFAT w/ 32KB cluster with no MBR.
		res = f_mkfs("ram:", FM_EXFAT | FM_SFD, RAMDISK_CLUSTER_SZ, buf, 0x400000);

		// Mount ramdisk.
		if (!res)
			res = f_mount(ram_fs, "ram:", 1);

		free(buf);
	}

	return res;
}

int ram_disk_read(u32 sector, u32 sector_count, void *buf)
{
	u32 sector_off = RAM_DISK_ADDR + (sector << 9);
	u32 bytes_count = sector_count << 9;

	if ((sector_off - RAM_DISK_ADDR) > disk_size)
		return 1;

	memcpy(buf, (void *)sector_off, bytes_count);

	return 0;
}

int ram_disk_write(u32 sector, u32 sector_count, const void *buf)
{
	u32 sector_off = RAM_DISK_ADDR + (sector << 9);
	u32 bytes_count = sector_count << 9;

	if ((sector_off - RAM_DISK_ADDR) > disk_size)
		return 1;

	memcpy((void *)sector_off, buf, bytes_count);

	return 0;
}
