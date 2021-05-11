/*
 * BPMP-Lite Cache/MMU and Frequency driver for Tegra X1
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

#ifndef _BPMP_H_
#define _BPMP_H_

#include <utils/types.h>

typedef enum
{
	BPMP_MMU_MAINT_NOP                =  0,
	BPMP_MMU_MAINT_CLEAN_PHY          =  1,
	BPMP_MMU_MAINT_INVALID_PHY        =  2,
	BPMP_MMU_MAINT_CLEAN_INVALID_PHY  =  3,
	BPMP_MMU_MAINT_CLEAN_LINE         =  9,
	BPMP_MMU_MAINT_INVALID_LINE       = 10,
	BPMP_MMU_MAINT_CLEAN_INVALID_LINE = 11,
	BPMP_MMU_MAINT_CLEAN_WAY          = 17,
	BPMP_MMU_MAINT_INVALID_WAY        = 18,
	BPMP_MMU_MAINT_CLN_INV_WAY        = 19
} bpmp_maintenance_t;

typedef struct _bpmp_mmu_entry_t
{
	u32 start_addr;
	u32 end_addr;
	u32 attr;
	u32 enable;
} bpmp_mmu_entry_t;

typedef enum
{
	BPMP_CLK_NORMAL,      // 408MHz  0% - 136MHz APB.
	BPMP_CLK_HIGH_BOOST,  // 544MHz 33% - 136MHz APB.
	BPMP_CLK_SUPER_BOOST, // 576MHz 41% - 144MHz APB.
	BPMP_CLK_HYPER_BOOST, // 589MHz 44% - 147MHz APB.
	//BPMP_CLK_DEV_BOOST, // 608MHz 49% - 152MHz APB.
	BPMP_CLK_MAX
} bpmp_freq_t;

#define BPMP_CLK_LOWER_BOOST   BPMP_CLK_SUPER_BOOST
#define BPMP_CLK_DEFAULT_BOOST BPMP_CLK_HYPER_BOOST

void bpmp_mmu_maintenance(u32 op, bool force);
void bpmp_mmu_set_entry(int idx, bpmp_mmu_entry_t *entry, bool apply);
void bpmp_mmu_enable();
void bpmp_mmu_disable();
void bpmp_clk_rate_get();
bpmp_freq_t bpmp_clk_rate_set(bpmp_freq_t fid);
void bpmp_usleep(u32 us);
void bpmp_msleep(u32 ms);
void bpmp_halt();

#endif
