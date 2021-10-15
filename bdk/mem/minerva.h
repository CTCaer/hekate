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

#ifndef _FE_MINERVA_H_
#define _FE_MINERVA_H_

#include "mtc_table.h"
#include <utils/types.h>

#define MTC_INIT_MAGIC 0x3043544D
#define MTC_NEW_MAGIC  0x5243544D

#define EMC_PERIODIC_TRAIN_MS 250

typedef struct
{
	u32 rate_to;
	u32 rate_from;
	emc_table_t *mtc_table;
	u32 table_entries;
	emc_table_t *current_emc_table;
	u32 train_mode;
	u32 sdram_id;
	u32 prev_temp;
	bool emc_2X_clk_src_is_pllmb;
	bool fsp_for_src_freq;
	bool train_ram_patterns;
	bool init_done;
} mtc_config_t;

enum train_mode_t
{
	OP_SWITCH         = 0,
	OP_TRAIN          = 1,
	OP_TRAIN_SWITCH   = 2,
	OP_PERIODIC_TRAIN = 3,
	OP_TEMP_COMP      = 4
};

typedef enum
{
	FREQ_204  = 204000,
	FREQ_800  = 800000,
	FREQ_1600 = 1600000
} minerva_freq_t;

extern void (*minerva_cfg)(mtc_config_t *mtc_cfg, void *);
u32  minerva_init();
void minerva_change_freq(minerva_freq_t freq);
void minerva_prep_boot_freq();
void minerva_periodic_training();

#endif
