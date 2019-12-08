/*
 * BPMP-Lite Cache/MMU and Frequency driver for Tegra X1
 *
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

#include "bpmp.h"
#include "clock.h"
#include "t210.h"
#include "../../common/memory_map.h"
#include "../utils/util.h"

#define BPMP_CACHE_CONFIG               0x0
#define  CFG_ENABLE                     (1 << 0)
#define  CFG_FORCE_WRITE_THROUGH        (1 << 3)
#define  CFG_DISABLE_WRITE_BUFFER       (1 << 10)
#define  CFG_DISABLE_READ_BUFFER        (1 << 11)
#define  CFG_FULL_LINE_DIRTY            (1 << 13)
#define  CFG_TAG_CHK_ABRT_ON_ERR        (1 << 14)
#define BPMP_CACHE_LOCK                 0x4
#define BPMP_CACHE_SIZE                 0xC
#define BPMP_CACHE_LFSR                 0x10
#define BPMP_CACHE_TAG_STATUS           0x14
#define BPMP_CACHE_CLKEN_OVERRIDE       0x18
#define BPMP_CACHE_MAINT_ADDR           0x20
#define BPMP_CACHE_MAINT_DATA           0x24
#define BPMP_CACHE_MAINT_REQ            0x28
#define  MAINT_REQ_WAY_BITMAP(x)        ((x) << 8)

#define BPMP_CACHE_INT_MASK             0x40
#define BPMP_CACHE_INT_CLEAR            0x44
#define  INT_CLR_MAINT_DONE             (1 << 0)

#define BPMP_CACHE_INT_RAW_EVENT        0x48
#define  INT_RAW_EVENT_MAINT_DONE      (1 << 0)
#define BPMP_CACHE_INT_STATUS           0x4C

#define BPMP_CACHE_RB_CFG               0x80
#define BPMP_CACHE_WB_CFG               0x84

#define BPMP_CACHE_MMU_FALLBACK_ENTRY   0xA0
#define BPMP_CACHE_MMU_SHADOW_COPY_MASK 0xA4
#define BPMP_CACHE_MMU_CFG              0xAC
#define  MMU_CFG_SEQ_EN                 (1 << 1)
#define  MMU_CFG_TLB_EN                 (1 << 2)
#define  MMU_CFG_ABORT_STORE_LAST       (1 << 4)
#define BPMP_CACHE_MMU_CMD              0xB0
#define  MMU_CMD_NOP                    0
#define  MMU_CMD_INIT                   1
#define  MMU_CMD_COPY_SHADOW            2
#define BPMP_CACHE_MMU_ABORT_STAT       0xB4
#define BPMP_CACHE_MMU_ABORT_ADDR       0xB8
#define BPMP_CACHE_MMU_ACTIVE_ENTRIES   0xBC

#define BPMP_MMU_SHADOW_ENTRY_BASE     (BPMP_CACHE_BASE + 0x400)
#define BPMP_MMU_MAIN_ENTRY_BASE       (BPMP_CACHE_BASE + 0x800)
#define  MMU_ENTRY_ADDR_MASK           0xFFFFFFE0

#define  MMU_EN_CACHED                (1 << 0)
#define  MMU_EN_EXEC                  (1 << 1)
#define  MMU_EN_READ                  (1 << 2)
#define  MMU_EN_WRITE                 (1 << 3)

bpmp_mmu_entry_t mmu_entries[] =
{
	{ DRAM_START, 0xFFFFFFFF, MMU_EN_READ | MMU_EN_WRITE | MMU_EN_EXEC | MMU_EN_CACHED, true },
	{ IRAM_BASE,  0x4003FFFF, MMU_EN_READ | MMU_EN_WRITE | MMU_EN_EXEC | MMU_EN_CACHED, true }
};

void bpmp_mmu_maintenance(u32 op, bool force)
{
	if (!force && !(BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) & CFG_ENABLE))
		return;

	BPMP_CACHE_CTRL(BPMP_CACHE_INT_CLEAR) = INT_CLR_MAINT_DONE;

	// This is a blocking operation.
	BPMP_CACHE_CTRL(BPMP_CACHE_MAINT_REQ) = MAINT_REQ_WAY_BITMAP(0xF) | op;

	while(!(BPMP_CACHE_CTRL(BPMP_CACHE_INT_RAW_EVENT) & INT_RAW_EVENT_MAINT_DONE))
		;

	BPMP_CACHE_CTRL(BPMP_CACHE_INT_CLEAR) = BPMP_CACHE_CTRL(BPMP_CACHE_INT_RAW_EVENT);
}

void bpmp_mmu_set_entry(int idx, bpmp_mmu_entry_t *entry, bool apply)
{
	if (idx > 31)
		return;

	volatile bpmp_mmu_entry_t *mmu_entry = (bpmp_mmu_entry_t *)(BPMP_MMU_SHADOW_ENTRY_BASE + sizeof(bpmp_mmu_entry_t) * idx);

	if (entry->enable)
	{
		mmu_entry->min_addr = entry->min_addr & MMU_ENTRY_ADDR_MASK;
		mmu_entry->max_addr = entry->max_addr & MMU_ENTRY_ADDR_MASK;
		mmu_entry->attr = entry->attr;

		BPMP_CACHE_CTRL(BPMP_CACHE_MMU_SHADOW_COPY_MASK) |= (1 << idx);

		if (apply)
			BPMP_CACHE_CTRL(BPMP_CACHE_MMU_CMD) = MMU_CMD_COPY_SHADOW;
	}
}

void bpmp_mmu_enable()
{
	if (BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) & CFG_ENABLE)
		return;

	// Init BPMP MMU.
	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_CMD) = MMU_CMD_INIT;
	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_FALLBACK_ENTRY) = MMU_EN_READ | MMU_EN_WRITE | MMU_EN_EXEC; // RWX for non-defined regions.
	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_CFG) = MMU_CFG_SEQ_EN | MMU_CFG_TLB_EN | MMU_CFG_ABORT_STORE_LAST;

	// Init BPMP MMU entries.
	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_SHADOW_COPY_MASK) = 0;
	for (u32 idx = 0; idx < (sizeof(mmu_entries) / sizeof(bpmp_mmu_entry_t)); idx++)
		bpmp_mmu_set_entry(idx, &mmu_entries[idx], false);

	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_CMD) = MMU_CMD_COPY_SHADOW;

	// Invalidate cache.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, true);

	// Enable cache.
	BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) = CFG_ENABLE | CFG_FORCE_WRITE_THROUGH | CFG_TAG_CHK_ABRT_ON_ERR;

	// HW bug. Invalidate cache again.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, false);
}

void bpmp_mmu_disable()
{
	if (!(BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) & CFG_ENABLE))
		return;

	// Clean and invalidate cache.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	// Disable cache.
	BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) = 0;
}

const u8 pllc4_divn[] = {
	0,   // BPMP_CLK_NORMAL:      408MHz  0% - 136MHz APB.
	85,  // BPMP_CLK_HIGH_BOOST:  544MHz 33% - 136MHz APB.
	90,  // BPMP_CLK_SUPER_BOOST: 576MHz 41% - 144MHz APB.
	92   // BPMP_CLK_HYPER_BOOST: 589MHz 44% - 147MHz APB.
	// Do not use for public releases!
	//95   // BPMP_CLK_DEV_BOOST: 608MHz 49% - 152MHz APB.
};

bpmp_freq_t bpmp_clock_set = BPMP_CLK_NORMAL;

void bpmp_clk_rate_set(bpmp_freq_t fid)
{
	if (fid > (BPMP_CLK_MAX - 1))
		fid = BPMP_CLK_MAX - 1;

	if (bpmp_clock_set == fid)
		return;

	if (fid)
	{
		if (bpmp_clock_set)
		{
			// Restore to PLLP source during PLLC4 configuration.
			CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003333; // PLLP_OUT.
			// Wait a bit for clock source change.
			msleep(10);
		}

		CLOCK(CLK_RST_CONTROLLER_PLLC4_MISC) = PLLC4_MISC_EN_LCKDET;
		CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) = 4 | (pllc4_divn[fid] << 8) | PLL_BASE_ENABLE; // DIVM: 4, DIVP: 1.

		while (!(CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) & PLLC4_BASE_LOCK))
			;

		CLOCK(CLK_RST_CONTROLLER_PLLC4_OUT) = (1 << 8) | PLLC4_OUT3_CLKEN; // 1.5 div.
		CLOCK(CLK_RST_CONTROLLER_PLLC4_OUT) |= PLLC4_OUT3_RSTN_CLR; // Get divider out of reset.

		// Wait a bit for PLLC4 to stabilize.
		msleep(10);
		CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 3; // PCLK = HCLK / 4.
		CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003323; // PLLC4_OUT3.

		bpmp_clock_set = fid;
	}
	else
	{
		CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003333; // PLLP_OUT.

		// Wait a bit for clock source change.
		msleep(10);

		CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 2; // PCLK = HCLK / 3.
		CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLL_BASE_ENABLE;
		bpmp_clock_set = BPMP_CLK_NORMAL;
	}
}

// The following functions halt BPMP to reduce power while sleeping.
// They are not as accurate as RTC at big values but they guarantee time+ delay.
void bpmp_usleep(u32 us)
{
	u32 delay;

	// Each iteration takes 1us.
	while (us)
	{
		delay = (us > HALT_COP_MAX_CNT) ? HALT_COP_MAX_CNT : us;
		us -= delay;

		FLOW_CTLR(FLOW_CTLR_HALT_COP_EVENTS) = HALT_COP_WAIT_EVENT | HALT_COP_USEC | delay;
	}
}

void bpmp_msleep(u32 ms)
{
	u32 delay;

	// Iteration time is variable. ~200 - 1000us.
	while (ms)
	{
		delay = (ms > HALT_COP_MAX_CNT) ? HALT_COP_MAX_CNT : ms;
		ms -= delay;

		FLOW_CTLR(FLOW_CTLR_HALT_COP_EVENTS) = HALT_COP_WAIT_EVENT | HALT_COP_MSEC | delay;
	}
}

void bpmp_halt()
{
	FLOW_CTLR(FLOW_CTLR_HALT_COP_EVENTS) = HALT_COP_WAIT_EVENT | HALT_COP_JTAG;
}
