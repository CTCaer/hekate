/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2019-2020 CTCaer
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

#ifndef _NX_EMMC_H_
#define _NX_EMMC_H_

#include <storage/sdmmc.h>
#include <libs/fatfs/ff.h>
#include <utils/types.h>
#include <utils/list.h>

#define NX_GPT_FIRST_LBA  1
#define NX_GPT_NUM_BLOCKS 33
#define NX_EMMC_BLOCKSIZE 512

typedef struct _emmc_part_t
{
	u32 index;
	u32 lba_start;
	u32 lba_end;
	u64 attrs;
	char name[37];
	link_t link;
} emmc_part_t;

extern sdmmc_t emmc_sdmmc;
extern sdmmc_storage_t emmc_storage;
extern FATFS emmc_fs;

void nx_emmc_gpt_parse(link_t *gpt, sdmmc_storage_t *storage);
void nx_emmc_gpt_free(link_t *gpt);
emmc_part_t *nx_emmc_part_find(link_t *gpt, const char *name);
int  nx_emmc_part_read(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf);
int  nx_emmc_part_write(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf);

void nx_emmc_get_autorcm_masks(u8 *mod0, u8 *mod1);

#endif
