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

#include "actmon.h"
#include "clock.h"
#include "t210.h"

/* Global registers */
#define ACTMON_GLB_STATUS          0x0
#define  ACTMON_MCCPU_MON_ACT      BIT(8)
#define  ACTMON_MCALL_MON_ACT      BIT(9)
#define  ACTMON_CPU_FREQ_MON_ACT   BIT(10)
#define  ACTMON_APB_MON_ACT        BIT(12)
#define  ACTMON_AHB_MON_ACT        BIT(13)
#define  ACTMON_BPMP_MON_ACT       BIT(14)
#define  ACTMON_CPU_MON_ACT        BIT(15)
#define  ACTMON_MCCPU_INTR         BIT(25)
#define  ACTMON_MCALL_INTR         BIT(26)
#define  ACTMON_CPU_FREQ_INTR      BIT(27)
#define  ACTMON_APB_INTR           BIT(28)
#define  ACTMON_AHB_INTR           BIT(29)
#define  ACTMON_BPMP_INTR          BIT(30)
#define  ACTMON_CPU_INTR           BIT(31)
#define ACTMON_GLB_PERIOD_CTRL     0x4
#define  ACTMON_GLB_PERIOD_USEC    BIT(8)
#define  ACTMON_GLB_PERIOD_SAMPLE(n) (((n) - 1) & 0xFF)

/* Device Registers */
#define ACTMON_DEV_BASE            ACTMON_BASE + 0x80
#define  ACTMON_DEV_SIZE           0x40
/* CTRL */
#define  ACTMON_DEV_CTRL_K_VAL(k)                       (((k) & 7) << 10)
#define  ACTMON_DEV_CTRL_ENB_PERIODIC                   BIT(18)
#define  ACTMON_DEV_CTRL_AT_END_EN                      BIT(19)
#define  ACTMON_DEV_CTRL_AVG_BELOW_WMARK_EN             BIT(20)
#define  ACTMON_DEV_CTRL_AVG_ABOVE_WMARK_EN             BIT(21)
#define  ACTMON_DEV_CTRL_WHEN_OVERFLOW_EN               BIT(22)
#define  ACTMON_DEV_CTRL_CONSECUTIVE_BELOW_WMARK_NUM(n) (((n) & 7) << 23)
#define  ACTMON_DEV_CTRL_CONSECUTIVE_ABOVE_WMARK_NUM(n) (((n) & 7) << 26)
#define  ACTMON_DEV_CTRL_CONSECUTIVE_BELOW_WMARK_EN     BIT(29)
#define  ACTMON_DEV_CTRL_CONSECUTIVE_ABOVE_WMARK_EN     BIT(30)
#define  ACTMON_DEV_CTRL_ENB                            BIT(31)
/* INTR_STATUS */
#define  ACTMON_DEV_ISTS_AVG_ABOVE_WMARK   BIT(24)
#define  ACTMON_DEV_ISTS_AVG_BELOW_WMARK   BIT(25)
#define  ACTMON_DEV_ISTS_WHEN_OVERFLOW     BIT(26)
#define  ACTMON_DEV_ISTS_AT_END            BIT(29)
#define  ACTMON_DEV_ISTS_CONSECUTIVE_LOWER BIT(30)
#define  ACTMON_DEV_ISTS_CONSECUTIVE_UPPER BIT(31)

/* Histogram Registers */
#define ACTMON_HISTOGRAM_CONFIG    0x300
#define  ACTMON_HIST_CFG_ACTIVE                   BIT(0)
#define  ACTMON_HIST_CFG_LINEAR_MODE              BIT(1)
#define  ACTMON_HIST_CFG_NO_UNDERFLOW_BUCKET      BIT(2)
#define  ACTMON_HIST_CFG_STALL_ON_SINGLE_SATURATE BIT(3)
#define  ACTMON_HIST_CFG_SHIFT(s)                 (((s) & 0x1F) << 4)
#define  ACTMON_HIST_CFG_SOURCE(s)                (((s) & 0xF) << 12)
#define ACTMON_HISTOGRAM_CTRL      0x304
#define  ACTMON_HIST_CTRL_CLEAR_ALL               BIT(0)
#define ACTMON_HISTOGRAM_DATA_BASE 0x380
#define  ACTMON_HISTOGRAM_DATA_NUM 32

#define ACTMON_FREQ 19200000

typedef struct _actmon_dev_reg_t
{
	vu32 ctrl;
	vu32 upper_wnark;
	vu32 lower_wmark;
	vu32 init_avg;
	vu32 avg_upper_wmark;
	vu32 avg_lower_wmark;
	vu32 count_weight;
	vu32 count;
	vu32 avg_count;
	vu32 intr_status;
	vu32 ctrl2;
	vu32 unk[5];
} actmon_dev_reg_t;

u32 sample_period = 0;

void actmon_hist_enable(actmon_hist_src_t src)
{
	ACTMON(ACTMON_HISTOGRAM_CONFIG) = ACTMON_HIST_CFG_SOURCE(src) | ACTMON_HIST_CFG_ACTIVE;
	ACTMON(ACTMON_HISTOGRAM_CTRL) = ACTMON_HIST_CTRL_CLEAR_ALL;
}

void actmon_hist_disable()
{
	ACTMON(ACTMON_HISTOGRAM_CONFIG) = 0;
}

void actmon_hist_get(u32 *histogram)
{
	if (histogram)
	{
		for (u32 i = 0; i < ACTMON_HISTOGRAM_DATA_NUM; i++)
			histogram[i] = ACTMON(ACTMON_HISTOGRAM_DATA_BASE + i * sizeof(u32));
	}
}

void actmon_dev_enable(actmon_dev_t dev)
{
	actmon_dev_reg_t *regs = (actmon_dev_reg_t *)(ACTMON_DEV_BASE + (dev * ACTMON_DEV_SIZE));

	regs->init_avg = 0;
	regs->count_weight = 5;

	regs->ctrl = ACTMON_DEV_CTRL_ENB | ACTMON_DEV_CTRL_ENB_PERIODIC;
}

void actmon_dev_disable(actmon_dev_t dev)
{
	actmon_dev_reg_t *regs = (actmon_dev_reg_t *)(ACTMON_DEV_BASE + (dev * ACTMON_DEV_SIZE));

	regs->ctrl = 0;
}

u32 actmon_dev_get_load(actmon_dev_t dev)
{
	actmon_dev_reg_t *regs = (actmon_dev_reg_t *)(ACTMON_DEV_BASE + (dev * ACTMON_DEV_SIZE));

	// Get load-based sampling. 1 decimal point precision.
	u32 load = regs->count / (ACTMON_FREQ / 1000);

	return load;
}

u32 actmon_dev_get_load_avg(actmon_dev_t dev)
{
	actmon_dev_reg_t *regs = (actmon_dev_reg_t *)(ACTMON_DEV_BASE + (dev * ACTMON_DEV_SIZE));

	// Get load-based sampling. 1 decimal point precision.
	u32 avg_load = regs->avg_count / (ACTMON_FREQ / 1000);

	return avg_load;
}

void atmon_dev_all_disable()
{
	// TODO: do a global reset?
}

void actmon_init()
{
	clock_enable_actmon();

	// Set period to 200ms.
	ACTMON(ACTMON_GLB_PERIOD_CTRL) &= ~ACTMON_GLB_PERIOD_USEC;
	ACTMON(ACTMON_GLB_PERIOD_CTRL) |= ACTMON_GLB_PERIOD_SAMPLE(200);
}

void actmon_end()
{
	clock_disable_actmon();
}