/*
* Copyright (c) 2018 naehrwert
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
#include "heap.h"
#include "list.h"

void nx_emmc_gpt_parse(link_t *gpt, sdmmc_storage_t *storage)
{
	u8 *buf = (u8 *)malloc(NX_GPT_NUM_BLOCKS * NX_EMMC_BLOCKSIZE);

	sdmmc_storage_read(storage, NX_GPT_FIRST_LBA, NX_GPT_NUM_BLOCKS, buf);

	gpt_header_t *hdr = (gpt_header_t *)buf;
	for (u32 i = 0; i < hdr->num_part_ents; i++)
	{
		gpt_entry_t *ent = (gpt_entry_t *)(buf + (hdr->part_ent_lba - 1) * NX_EMMC_BLOCKSIZE + i * sizeof(gpt_entry_t));
		emmc_part_t *part = (emmc_part_t *)malloc(sizeof(emmc_part_t));
		part->lba_start = ent->lba_start;
		part->lba_end = ent->lba_end;
		part->attrs = ent->attrs;

		//HACK
		for (u32 i = 0; i < 36; i++)
			part->name[i] = ent->name[i];
		part->name[37] = 0;

		list_append(gpt, &part->link);
	}

	free(buf);
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
	//The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;
	return sdmmc_storage_read(storage, part->lba_start + sector_off, num_sectors, buf);
}

int nx_emmc_part_write(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	//The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;
	return sdmmc_storage_write(storage, part->lba_start + sector_off, num_sectors, buf);
}
