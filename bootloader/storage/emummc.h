/*
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

#ifndef EMUMMC_H
#define EMUMMC_H

#include <storage/sdmmc.h>
#include <utils/types.h>

typedef enum
{
	EMUMMC_TYPE_NONE      = 0,
	EMUMMC_TYPE_PARTITION = 1,
	EMUMMC_TYPE_FILES     = 2,
} emummc_type_t;

typedef enum {
	EMUMMC_MMC_NAND = 0,
	EMUMMC_MMC_SD   = 1,
	EMUMMC_MMC_GC   = 2,
} emummc_mmc_t;

typedef struct _emummc_cfg_t
{
	int   enabled;
	u64   sector;
	u32   id;
	char *path;
	char *nintendo_path;
	// Internal.
	char *emummc_file_based_path;
	u32 file_based_part_size;
	u32 active_part;
	int fs_ver;
} emummc_cfg_t;

extern emummc_cfg_t emu_cfg;

void emummc_load_cfg();
bool emummc_set_path(char *path);
int  emummc_storage_init_mmc();
int  emummc_storage_end();
int  emummc_storage_read(u32 sector, u32 num_sectors, void *buf);
int  emummc_storage_write(u32 sector, u32 num_sectors, void *buf);
int  emummc_storage_set_mmc_partition(u32 partition);

#endif