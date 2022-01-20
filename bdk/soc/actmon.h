/*
 * Activity Monitor driver for Tegra X1
 *
 * Copyright (c) 2021 CTCaer
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

#ifndef __ACTMON_H_
#define __ACTMON_H_

#include <utils/types.h>

typedef enum _actmon_dev_t
{
	ACTMON_DEV_CPU,
	ACTMON_DEV_BPMP,
	ACTMON_DEV_AHB,
	ACTMON_DEV_APB,
	ACTMON_DEV_CPU_FREQ,
	ACTMON_DEV_MC_ALL,
	ACTMON_DEV_MC_CPU,

	ACTMON_DEV_NUM,
} actmon_dev_t;

typedef enum _actmon_hist_src_t
{
	ACTMON_HIST_SRC_NONE     = 0,
	ACTMON_HIST_SRC_AHB      = 1,
	ACTMON_HIST_SRC_APB      = 2,
	ACTMON_HIST_SRC_BPMP     = 3,
	ACTMON_HIST_SRC_CPU      = 4,
	ACTMON_HIST_SRC_MC_ALL   = 5,
	ACTMON_HIST_SRC_MC_CPU   = 6,
	ACTMON_HIST_SRC_CPU_FREQ = 7,
	ACTMON_HIST_SRC_NA       = 8,
	ACTMON_HIST_SRC_APB_MMIO = 9,
} actmon_hist_src_t;

void actmon_hist_enable(actmon_hist_src_t src);
void actmon_hist_disable();
void actmon_hist_get(u32 *histogram);
void actmon_dev_enable(actmon_dev_t dev);
void actmon_dev_disable(actmon_dev_t dev);
u32  actmon_dev_get_load(actmon_dev_t dev);
u32  actmon_dev_get_load_avg(actmon_dev_t dev);
void atmon_dev_all_disable();
void actmon_init();
void actmon_end();

#endif