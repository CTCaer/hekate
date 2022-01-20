/*
 * Copyright (c) 2018 naehrwert
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

#include <string.h>

#include "emmc.h"
#include <mem/heap.h>
#include <soc/fuse.h>
#include <storage/mbr_gpt.h>
#include <utils/list.h>

static u16 emmc_errors[3] = { 0 }; // Init and Read/Write errors.
static u32 emmc_mode = EMMC_MMC_HS400;

sdmmc_t emmc_sdmmc;
sdmmc_storage_t emmc_storage;
FATFS emmc_fs;

#ifdef BDK_EMUMMC_ENABLE
int emummc_storage_read(u32 sector, u32 num_sectors, void *buf);
int emummc_storage_write(u32 sector, u32 num_sectors, void *buf);
#endif

void emmc_error_count_increment(u8 type)
{
	switch (type)
	{
	case EMMC_ERROR_INIT_FAIL:
		emmc_errors[0]++;
		break;
	case EMMC_ERROR_RW_FAIL:
		emmc_errors[1]++;
		break;
	case EMMC_ERROR_RW_RETRY:
		emmc_errors[2]++;
		break;
	}
}

u16 *emmc_get_error_count()
{
	return emmc_errors;
}

u32 emmc_get_mode()
{
	return emmc_mode;
}

int emmc_init_retry(bool power_cycle)
{
	u32 bus_width = SDMMC_BUS_WIDTH_8;
	u32 type = SDHCI_TIMING_MMC_HS400;

	// Power cycle SD eMMC.
	if (power_cycle)
	{
		emmc_mode--;
		sdmmc_storage_end(&emmc_storage);
	}

	// Get init parameters.
	switch (emmc_mode)
	{
	case EMMC_INIT_FAIL: // Reset to max.
		return 0;
	case EMMC_1BIT_HS52:
		bus_width = SDMMC_BUS_WIDTH_1;
		type = SDHCI_TIMING_MMC_HS52;
		break;
	case EMMC_8BIT_HS52:
		type = SDHCI_TIMING_MMC_HS52;
		break;
	case EMMC_MMC_HS200:
		type = SDHCI_TIMING_MMC_HS200;
		break;
	case EMMC_MMC_HS400:
		type = SDHCI_TIMING_MMC_HS400;
		break;
	default:
		emmc_mode = EMMC_MMC_HS400;
	}

	return sdmmc_storage_init_mmc(&emmc_storage, &emmc_sdmmc, bus_width, type);
}

bool emmc_initialize(bool power_cycle)
{
	// Reset mode in case of previous failure.
	if (emmc_mode == EMMC_INIT_FAIL)
		emmc_mode = EMMC_MMC_HS400;

	if (power_cycle)
		sdmmc_storage_end(&emmc_storage);

	int res = !emmc_init_retry(false);

	while (true)
	{
		if (!res)
			return true;
		else
		{
			emmc_errors[EMMC_ERROR_INIT_FAIL]++;

			if (emmc_mode == EMMC_INIT_FAIL)
				break;
			else
				res = !emmc_init_retry(true);
		}
	}

	sdmmc_storage_end(&emmc_storage);

	return false;
}

void emmc_gpt_parse(link_t *gpt)
{
	gpt_t *gpt_buf = (gpt_t *)calloc(GPT_NUM_BLOCKS, EMMC_BLOCKSIZE);

#ifdef BDK_EMUMMC_ENABLE
	emummc_storage_read(GPT_FIRST_LBA, GPT_NUM_BLOCKS, gpt_buf);
#else
	sdmmc_storage_read(&emmc_storage, GPT_FIRST_LBA, GPT_NUM_BLOCKS, gpt_buf);
#endif

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

void emmc_gpt_free(link_t *gpt)
{
	LIST_FOREACH_SAFE(iter, gpt)
		free(CONTAINER_OF(iter, emmc_part_t, link));
}

emmc_part_t *emmc_part_find(link_t *gpt, const char *name)
{
	LIST_FOREACH_ENTRY(emmc_part_t, part, gpt, link)
		if (!strcmp(part->name, name))
			return part;

	return NULL;
}

int emmc_part_read(emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	// The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;

#ifdef BDK_EMUMMC_ENABLE
	return emummc_storage_read(part->lba_start + sector_off, num_sectors, buf);
#else
	return sdmmc_storage_read(&emmc_storage, part->lba_start + sector_off, num_sectors, buf);
#endif
}

int emmc_part_write(emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	// The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;

#ifdef BDK_EMUMMC_ENABLE
	return emummc_storage_write(part->lba_start + sector_off, num_sectors, buf);
#else
	return sdmmc_storage_write(&emmc_storage, part->lba_start + sector_off, num_sectors, buf);
#endif
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
