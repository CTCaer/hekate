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

#include <string.h>
#include <stdlib.h>

#include "minerva.h"

#include <ianos/ianos.h>
#include <mem/emc_t210.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/hw_init.h>
#include <soc/t210.h>

#define FREQ_NO_TABLE_MAX FREQ_408

#define TABLE_FREQ_KHZ_OFFSET        0x40
#define TABLE_LA_REGS_T210_OFFSET    0x1284
#define TABLE_LA_REGS_T210B01_OFFSET 0xFA4
#define LA_SDMMC1_INDEX 6

static bool no_table = false;
static mtc_config_t *mtc_cfg = NULL;
void (*mtc_call)(mtc_config_t *mtc_cfg, void *);

u32 minerva_init(minerva_str_t *mtc_str)
{
	mtc_call = NULL;
	mtc_cfg  = (mtc_config_t *)&mtc_str->mtc_cfg;
	no_table = hw_get_chip_id() == GP_HIDREV_MAJOR_T210B01;

#ifdef BDK_MINERVA_CFG_FROM_RAM
	// Set table to nyx storage.
	mtc_cfg->mtc_table = (emc_table_t *)mtc_str->mtc_table;

	// Check if Minerva is already initialized.
	if (mtc_cfg->init_done == MTC_INIT_MAGIC)
	{
		// Load library and do a periodic training if needed.
		mtc_cfg->train_mode = OP_PERIODIC_TRAIN;
		u32 ep_addr = ianos_loader("bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)mtc_cfg);
		mtc_call = (void *)ep_addr;

		return !mtc_call ? 1 : 0;
	}
	else
	{
		mtc_config_t mtc_tmp;

		// Initialize table.
		mtc_tmp.mtc_table  = mtc_cfg->mtc_table;
		mtc_tmp.sdram_id   = fuse_read_dramid(false);
		mtc_tmp.init_done  = !no_table ? MTC_NEW_MAGIC : MTC_IRB_MAGIC;

		// Load library and get table.
		u32 ep_addr = ianos_loader("bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)&mtc_tmp);

		// Ensure that Minerva is initialized.
		if (mtc_tmp.init_done == MTC_INIT_MAGIC)
			mtc_call = (void *)ep_addr;
		else
			mtc_cfg->init_done = 0;

		// Copy Minerva context to Nyx storage.
		if (mtc_call)
			memcpy(mtc_cfg, (void *)&mtc_tmp, sizeof(mtc_config_t));
	}
#else
	// Fully initialize Minerva.
	memset(mtc_cfg, 0, sizeof(mtc_config_t));

	// Initialize mtc table.
	mtc_cfg->mtc_table  = mtc_str->mtc_table;
	mtc_cfg->sdram_id   = fuse_read_dramid(false);
	mtc_cfg->init_done  = !no_table ? MTC_NEW_MAGIC : MTC_IRB_MAGIC;

	u32 ep_addr = ianos_loader("bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)mtc_cfg);

	// Ensure that Minerva is initialized.
	if (mtc_cfg->init_done == MTC_INIT_MAGIC)
		mtc_call = (void *)ep_addr;
	else
		mtc_cfg->init_done = 0;
#endif

	if (!mtc_call)
		return 1;

	if (no_table)
	{
		mtc_cfg->train_mode = OP_SWITCH;
		mtc_cfg->rate_from = FREQ_204;
		mtc_cfg->rate_to = FREQ_NO_TABLE_MAX;
		mtc_call(mtc_cfg, NULL);

		return 0;
	}

	// Train frequencies.
	mtc_cfg->train_mode = OP_TRAIN;
	mtc_cfg->rate_from = FREQ_204;
	mtc_cfg->rate_to = FREQ_204;
	mtc_call(mtc_cfg, NULL);
	mtc_cfg->rate_to = FREQ_800;
	mtc_call(mtc_cfg, NULL);
	mtc_cfg->rate_to = FREQ_1600;
	mtc_call(mtc_cfg, NULL);

	// FSP WAR.
	mtc_cfg->train_mode = OP_SWITCH;
	mtc_cfg->rate_to = FREQ_800;
	mtc_call(mtc_cfg, NULL);

	// Switch to max.
	mtc_cfg->rate_to = FREQ_1600;
	mtc_call(mtc_cfg, NULL);

	return 0;
}

void minerva_change_freq(minerva_freq_t freq)
{
	if (!mtc_call)
		return;

	// Clamp max allowed frequency when no table exists.
	if (no_table && freq > FREQ_NO_TABLE_MAX)
		freq = FREQ_NO_TABLE_MAX;

	// Check if requested frequency is different. Do not allow otherwise because it will hang.
	if (mtc_cfg->rate_from != freq)
	{
		mtc_cfg->train_mode = OP_SWITCH;
		mtc_cfg->rate_to = freq;
		mtc_call(mtc_cfg, NULL);
	}
}

void minerva_deinit()
{
	if (!mtc_cfg)
		return;

	minerva_change_freq(FREQ_204);
	mtc_cfg->init_done = 0;
}

void minerva_sdmmc_la_program(void *table, bool t210b01)
{
	u32 freq = *(u32 *)(table + TABLE_FREQ_KHZ_OFFSET);
	u32 *la_scale_regs = (u32 *)(table + (t210b01 ? TABLE_LA_REGS_T210B01_OFFSET : TABLE_LA_REGS_T210_OFFSET));

	// Adjust SDMMC1 latency allowance.
	switch (freq)
	{
	case 204000:
		la_scale_regs[LA_SDMMC1_INDEX] = (la_scale_regs[LA_SDMMC1_INDEX] & 0xFF0000) | 50;
		break;
	case 408000:
		la_scale_regs[LA_SDMMC1_INDEX] = (la_scale_regs[LA_SDMMC1_INDEX] & 0xFF0000) | 25;
		break;
	default:
		la_scale_regs[LA_SDMMC1_INDEX] = (la_scale_regs[LA_SDMMC1_INDEX] & 0xFF0000) | 20;
		break;
	}
}

void minerva_prep_boot_hos()
{
	if (!mtc_call)
		return;

	// Restore boot frequency for no table mode.
	if (no_table)
	{
		minerva_deinit();

		return;
	}

	// Check if there's RAM OC. If not exit.
	if (mtc_cfg->mtc_table[mtc_cfg->table_entries - 1].rate_khz == FREQ_1600)
		return;

	// FSP WAR.
	minerva_change_freq(FREQ_204);
	// Scale down to 800 MHz boot freq.
	minerva_change_freq(FREQ_800);
}

void minerva_prep_boot_l4t(u32 oc_freq, u32 opt_custom, bool prg_sdmmc_la)
{
	if (!mtc_call || no_table)
		return;

	// Program SDMMC LA regs.
	if (prg_sdmmc_la)
		for (u32 i = 0; i < mtc_cfg->table_entries; i++)
			minerva_sdmmc_la_program(&mtc_cfg->mtc_table[i], false);

	// Add OC frequency.
	if (oc_freq && mtc_cfg->mtc_table[mtc_cfg->table_entries - 1].rate_khz == FREQ_1600)
	{
		memcpy(&mtc_cfg->mtc_table[mtc_cfg->table_entries],
			   &mtc_cfg->mtc_table[mtc_cfg->table_entries - 1],
			   sizeof(emc_table_t));

		mtc_cfg->mtc_table[mtc_cfg->table_entries].opt_custom = opt_custom;
		mtc_cfg->mtc_table[mtc_cfg->table_entries].rate_khz   = oc_freq;
		mtc_cfg->table_entries++;
	}

	// Trim table.
	int entries = 0;
	for (u32 i = 0; i < mtc_cfg->table_entries; i++)
	{
		// Copy frequencies from 204/408/800 MHz and 1333+ MHz.
		int rate = mtc_cfg->mtc_table[i].rate_khz;
		if (rate == FREQ_204 ||
			rate == FREQ_408 ||
			rate == FREQ_800 ||
			rate >= FREQ_1333)
		{
			memcpy(&mtc_cfg->mtc_table[entries], &mtc_cfg->mtc_table[i], sizeof(emc_table_t));
			entries++;
		}
	}
	mtc_cfg->table_entries = entries;

	// Set init frequency.
	minerva_change_freq(FREQ_204);

	// Train the rest of the frequencies.
	mtc_cfg->train_mode = OP_TRAIN;
	for (u32 i = 0; i < mtc_cfg->table_entries; i++)
	{
		// Skip already trained frequencies and OC freq (Arachne handles it).
		if (mtc_cfg->mtc_table[i].trained || mtc_cfg->rate_to == oc_freq)
			continue;

		// Train frequency.
		mtc_cfg->rate_to = mtc_cfg->mtc_table[i].rate_khz;
		mtc_call(mtc_cfg, NULL);
	}

	// Do FSP WAR and scale to 800 MHz as boot freq.
	bool fsp_opwr_disabled = !(EMC(EMC_MRW3) & 0xC0);
	if (fsp_opwr_disabled)
		minerva_change_freq(FREQ_1333);
	minerva_change_freq(FREQ_800);

	// Do not let other mtc ops.
	mtc_cfg->init_done = 0;
}

void minerva_periodic_training()
{
	if (!mtc_call)
		return;

	if (mtc_cfg->rate_from >= FREQ_1066)
	{
		mtc_cfg->train_mode = OP_PERIODIC_TRAIN;
		mtc_call(mtc_cfg, NULL);
	}
}

emc_table_t *minerva_get_mtc_table()
{
	if (!mtc_call || no_table)
		return NULL;

	return mtc_cfg->mtc_table;
}

int minerva_get_mtc_table_entries()
{
	if (!mtc_call || no_table)
		return 0;

	return mtc_cfg->table_entries;
}
