/*
 * Minerva Training Cell
 * DRAM Training for Tegra X1 SoC. Supports DDR2/3 and LPDDR3/4.
 *
 * Copyright (c) 2018-2022 CTCaer  <ctcaer@gmail.com>
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

#ifndef _MTC_H_
#define _MTC_H_

#include "mtc_table.h"
#include "types.h"

/* Address bases and access macros - Change these for mapped access */
#define TMR_BASE   0x60005000
#define CLOCK_BASE 0x60006000
#define MC_BASE    0x70019000
#define EMC_BASE   0x7001B000
#define EMC0_BASE  0x7001E000
#define EMC1_BASE  0x7001F000

#define MTC_INIT_MAGIC 0x3043544D
#define MTC_NEW_MAGIC  0x5243544D

#define _REG(base, off) *(vu32 *)((base) + (off))

#define TMR(off) _REG(TMR_BASE, off)
#define CLOCK(off) _REG(CLOCK_BASE, off)
#define MC(off) _REG(MC_BASE, off)
#define EMC(off) _REG(EMC_BASE, off)
#define EMC_CH0(off) _REG(EMC0_BASE, off)
#define EMC_CH1(off) _REG(EMC1_BASE, off)
/* End of addresses and access macros */

#define EMC_TABLE_SIZE_R7         49280
#define EMC_TABLE_ENTRY_SIZE_R7   4928
#define EMC_TABLE_ENTRY_SIZE_R3   4300
#define EMC_STATUS_UPDATE_TIMEOUT 1000
#define EMC_PERIODIC_TRAIN_MS     100
#define EMC_TEMP_COMP_MS          1000

/* Training types */
#define NEEDS_TRAINING_CA              BIT(0)
#define NEEDS_TRAINING_CA_VREF         BIT(1)
#define NEEDS_TRAINING_QUSE            BIT(2)
#define NEEDS_TRAINING_QUSE_VREF       BIT(3)
#define NEEDS_TRAINING_WR              BIT(4)
#define NEEDS_TRAINING_WR_VREF         BIT(5)
#define NEEDS_TRAINING_RD              BIT(6)
#define NEEDS_TRAINING_RD_VREF         BIT(7)
#define NEEDS_TRAINING_SWAP_RANK       BIT(8)
#define NEEDS_TRAINING_IN_SELF_REFRESH BIT(9)

#define NEEDS_TRISTATE_TRAINING   (NEEDS_TRAINING_CA | NEEDS_TRAINING_CA_VREF | \
								   NEEDS_TRAINING_QUSE | NEEDS_TRAINING_WR |    \
								   NEEDS_TRAINING_WR_VREF | NEEDS_TRAINING_RD | \
								   NEEDS_TRAINING_RD_VREF)
#define NEEDS_TRAINING_CA_COMBO   (NEEDS_TRAINING_CA | NEEDS_TRAINING_CA_VREF)
#define NEEDS_TRAINING_QUSE_COMBO (NEEDS_TRAINING_QUSE | NEEDS_TRAINING_QUSE_VREF)
#define NEEDS_TRAINING_WR_COMBO   (NEEDS_TRAINING_WR | NEEDS_TRAINING_WR_VREF)
#define NEEDS_TRAINING_RD_COMBO   (NEEDS_TRAINING_RD | NEEDS_TRAINING_RD_VREF)

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

enum comp_seq_t
{
	DVFS_SEQUENCE              = 1,
	WRITE_TRAINING_SEQUENCE    = 2,
	PERIODIC_TRAINING_SEQUENCE = 3
};

enum tree_update_mode_t
{
	DVFS_PT1                 = 10,
	DVFS_UPDATE              = 11,
	TRAINING_PT1             = 12,
	TRAINING_UPDATE          = 13,
	PERIODIC_TRAINING_UPDATE = 14
};

enum emc_channels
{
	EMC_CHANNEL0 = 0,
	EMC_CHANNEL1 = 1
};

enum EMC_2X_CLK_SRC
{
	PLLM_OUT0  = 0x0,
	PLLC_OUT0  = 0x1,
	PLLP_OUT0  = 0x2,
	CLK_M      = 0x3,
	PLLM_UD    = 0x4,
	PLLMB_UD   = 0x5,
	PLLMB_OUT0 = 0x6,
	PLLP_UD    = 0x7
};

enum DRAM_TYPE
{
	DRAM_TYPE_DDR3   = 0,
	DRAM_TYPE_LPDDR4 = 1,
	DRAM_TYPE_LPDDR2 = 2,
	DRAM_TYPE_DDR2   = 3
};

enum DRAM_DEV_NO
{
	ONE_RANK = 1,
	TWO_RANK = 2
};

enum DRAM_OVER_TEMP_REF
{
	REFRESH_X2 = 1,
	REFRESH_X4 = 2
};

/* Timers for the below two compensation functions should be paused when changing timings. */

/* Change refresh rate based on dram temps. Run every 1000ms. */
/* Timer should be run only when another component reports over temperature. */
void _minerva_do_over_temp_compensation(mtc_config_t *mtc_cfg);

/* Periodic compensation only for tight timings that need it. Run every 100ms. */
/* Over temp and periodic compensation, should not access EMC_MRR at the same time. */
u32  _minerva_do_periodic_compensation(emc_table_t *mtc_table_entry);

#endif
