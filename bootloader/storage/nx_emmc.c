/*
 * Copyright (c) 2018 naehrwert
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

#include "nx_emmc.h"
#include "emummc.h"
#include <mem/heap.h>
#include <soc/fuse.h>
#include <storage/mbr_gpt.h>
#include <utils/list.h>

sdmmc_t emmc_sdmmc;
sdmmc_storage_t emmc_storage;
FATFS emmc_fs;

void nx_emmc_gpt_parse(link_t *gpt, sdmmc_storage_t *storage)
{
	gpt_t *gpt_buf = (gpt_t *)calloc(NX_GPT_NUM_BLOCKS, NX_EMMC_BLOCKSIZE);

	emummc_storage_read(NX_GPT_FIRST_LBA, NX_GPT_NUM_BLOCKS, gpt_buf);

	// Check if no GPT or more than max allowed entries.
	if (memcmp(&gpt_buf->header.signature, "EFI PART", 8) || gpt_buf->header.num_part_ents > 128)
		goto out;

	for (u32 i = 0; i < gpt_buf->header.num_part_ents; i++)
	{
		emmc_part_t *part = (emmc_part_t *)calloc(sizeof(emmc_part_t), 1);

		if (gpt_buf->entries[i].lba_start < gpt_buf->header.first_use_lba)
			continue;

		part->index = i;
		part->lba_start = gpt_buf->entries[i].lba_start;
		part->lba_end = gpt_buf->entries[i].lba_end;
		part->attrs = gpt_buf->entries[i].attrs;

		// ASCII conversion. Copy only the LSByte of the UTF-16LE name.
		for (u32 j = 0; j < 36; j++)
			part->name[j] = gpt_buf->entries[i].name[j];
		part->name[35] = 0;

		list_append(gpt, &part->link);
	}

out:
	free(gpt_buf);
}

void nx_emmc_gpt_free(link_t *gpt)
{
	LIST_FOREACH_SAFE(iter, gpt)
		free(CONTAINER_OF(iter, emmc_part_t, link));
}

emmc_part_t *nx_emmc_part_find(link_t *gpt, const char *name)
{
	LIST_FOREACH_ENTRY(emmc_part_t, part, gpt, link)
		if (!strcmp(part->name, name))
			return part;

	return NULL;
}

int nx_emmc_part_read(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	// The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;

	return emummc_storage_read(part->lba_start + sector_off, num_sectors, buf);
}

int nx_emmc_part_write(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	// The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;

	return emummc_storage_write(part->lba_start + sector_off, num_sectors, buf);
}

void nx_emmc_get_autorcm_masks(u8 *mod0, u8 *mod1)
{
	if (fuse_read_hw_state() == FUSE_NX_HW_STATE_PROD)
	{
		*mod0 = 0xF7;
		*mod1 = 0x86;
	}
	else
	{
		*mod0 = 0x37;
		*mod1 = 0x84;
	}
}
