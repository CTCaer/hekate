/*
 * Copyright (c) 2019 CTCaer
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

#include <soc/clock.h>
#include <ianos/ianos.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/hw_init.h>
#include <soc/t210.h>
#include <utils/util.h>

extern volatile nyx_storage_t *nyx_str;

void (*minerva_cfg)(mtc_config_t *mtc_cfg, void *);

u32 minerva_init()
{
	u32 curr_ram_idx = 0;

	minerva_cfg = NULL;
	mtc_config_t *mtc_cfg = (mtc_config_t *)&nyx_str->mtc_cfg;

	//!TODO: Not supported on T210B01 yet.
	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210B01)
		return 0;

#ifdef NYX
	// Set table to nyx storage.
	mtc_cfg->mtc_table = (emc_table_t *)nyx_str->mtc_table;

	// Check if Minerva is already initialized.
	if (mtc_cfg->init_done == MTC_INIT_MAGIC)
	{
		mtc_cfg->train_mode = OP_PERIODIC_TRAIN; // Retrain if needed.
		u32 ep_addr = ianos_loader("bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)mtc_cfg);
		minerva_cfg = (void *)ep_addr;

		return !minerva_cfg ? 1 : 0;
	}
	else
	{
		mtc_config_t mtc_tmp;

		mtc_tmp.mtc_table = mtc_cfg->mtc_table;
		mtc_tmp.sdram_id  = fuse_read_dramid(false);
		mtc_tmp.init_done = MTC_NEW_MAGIC;

		u32 ep_addr = ianos_loader("bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)&mtc_tmp);

		// Ensure that Minerva is new.
		if (mtc_tmp.init_done == MTC_INIT_MAGIC)
			minerva_cfg = (void *)ep_addr;
		else
			mtc_cfg->init_done = 0;

		// Copy Minerva context to Nyx storage.
		if (minerva_cfg)
			memcpy(mtc_cfg, (void *)&mtc_tmp, sizeof(mtc_config_t));
	}
#else
	memset(mtc_cfg, 0, sizeof(mtc_config_t));

	// Set table to nyx storage.
	mtc_cfg->mtc_table = (emc_table_t *)nyx_str->mtc_table;

	mtc_cfg->sdram_id  = fuse_read_dramid(false);
	mtc_cfg->init_done = MTC_NEW_MAGIC; // Initialize mtc table.

	u32 ep_addr = ianos_loader("bootloader/sys/libsys_minerva.bso", DRAM_LIB, (void *)mtc_cfg);

	// Ensure that Minerva is new.
	if (mtc_cfg->init_done == MTC_INIT_MAGIC)
		minerva_cfg = (void *)ep_addr;
	else
		mtc_cfg->init_done = 0;
#endif

	if (!minerva_cfg)
		return 1;

	// Get current frequency
	for (curr_ram_idx = 0; curr_ram_idx < 10; curr_ram_idx++)
	{
		if (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) == mtc_cfg->mtc_table[curr_ram_idx].clk_src_emc)
			break;
	}

	mtc_cfg->rate_from = mtc_cfg->mtc_table[curr_ram_idx].rate_khz;
	mtc_cfg->rate_to = FREQ_204;
	mtc_cfg->train_mode = OP_TRAIN;
	minerva_cfg(mtc_cfg, NULL);
	mtc_cfg->rate_to = FREQ_800;
	minerva_cfg(mtc_cfg, NULL);
	mtc_cfg->rate_to = FREQ_1600;
	minerva_cfg(mtc_cfg, NULL);

	// FSP WAR.
	mtc_cfg->train_mode = OP_SWITCH;
	mtc_cfg->rate_to = FREQ_800;
	minerva_cfg(mtc_cfg, NULL);

	// Switch to max.
	mtc_cfg->rate_to = FREQ_1600;
	minerva_cfg(mtc_cfg, NULL);

	return 0;
}

void minerva_change_freq(minerva_freq_t freq)
{
	if (!minerva_cfg)
		return;

	// Check if requested frequency is different. Do not allow otherwise because it will hang.
	mtc_config_t *mtc_cfg = (mtc_config_t *)&nyx_str->mtc_cfg;
	if (mtc_cfg->rate_from != freq)
	{
		mtc_cfg->rate_to = freq;
		mtc_cfg->train_mode = OP_SWITCH;
		minerva_cfg(mtc_cfg, NULL);
	}
}

void minerva_prep_boot_freq()
{
	if (!minerva_cfg)
		return;

	mtc_config_t *mtc_cfg = (mtc_config_t *)&nyx_str->mtc_cfg;

	// Check if there's RAM OC. If not exit.
	if (mtc_cfg->mtc_table[mtc_cfg->table_entries - 1].rate_khz == FREQ_1600)
		return;

	// FSP WAR.
	minerva_change_freq(FREQ_204);
	// Scale down to 800 MHz boot freq.
	minerva_change_freq(FREQ_800);
}

void minerva_periodic_training()
{
	if (!minerva_cfg)
		return;

	mtc_config_t *mtc_cfg = (mtc_config_t *)&nyx_str->mtc_cfg;
	if (mtc_cfg->rate_from == FREQ_1600)
	{
		mtc_cfg->train_mode = OP_PERIODIC_TRAIN;
		minerva_cfg(mtc_cfg, NULL);
	}
}