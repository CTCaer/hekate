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

#ifndef RAM_DISK_H
#define RAM_DISK_H

#include <libs/fatfs/ff.h>

#define RAMDISK_CLUSTER_SZ 32768

int ram_disk_init(FATFS *ram_fs, u32 ramdisk_size);
int ram_disk_read(u32 sector, u32 sector_count, void *buf);
int ram_disk_write(u32 sector, u32 sector_count, const void *buf);

#endif