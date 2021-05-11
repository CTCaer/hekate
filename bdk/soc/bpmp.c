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

#include <soc/bpmp.h>
#include <soc/clock.h>
#include <soc/t210.h>
#include <memory_map.h>
#include <utils/util.h>

#define BPMP_MMU_CACHE_LINE_SIZE        0x20

#define BPMP_CACHE_CONFIG               0x0
#define  CFG_ENABLE_CACHE               BIT(0)
#define  CFG_ENABLE_SKEW_ASSOC          BIT(1)
#define  CFG_DISABLE_RANDOM_ALLOC       BIT(2)
#define  CFG_FORCE_WRITE_THROUGH        BIT(3)
#define  CFG_NEVER_ALLOCATE             BIT(6)
#define  CFG_ENABLE_INTERRUPT           BIT(7)
#define  CFG_MMU_TAG_MODE(x)            ((x) << 8)
#define   TAG_MODE_PARALLEL             0
#define   TAG_MODE_TAG_FIRST            1
#define   TAG_MODE_MMU_FIRST            2
#define  CFG_DISABLE_WRITE_BUFFER       BIT(10)
#define  CFG_DISABLE_READ_BUFFER        BIT(11)
#define  CFG_ENABLE_HANG_DETECT         BIT(12)
#define  CFG_FULL_LINE_DIRTY            BIT(13)
#define  CFG_TAG_CHK_ABRT_ON_ERR        BIT(14)
#define  CFG_TAG_CHK_CLR_ERR            BIT(15)
#define  CFG_DISABLE_SAMELINE           BIT(16)
#define  CFG_OBS_BUS_EN                 BIT(31)

#define BPMP_CACHE_LOCK                 0x4
#define  LOCK_LINE(x)                   BIT((x))

#define BPMP_CACHE_SIZE                 0xC
#define BPMP_CACHE_LFSR                 0x10

#define BPMP_CACHE_TAG_STATUS           0x14
#define  TAG_STATUS_TAG_CHECK_ERROR     BIT(0)
#define  TAG_STATUS_CONFLICT_ADDR_MASK  0xFFFFFFE0

#define BPMP_CACHE_CLKEN_OVERRIDE       0x18
#define  CLKEN_OVERRIDE_WR_MCCIF_CLKEN  BIT(0)
#define  CLKEN_OVERRIDE_RD_MCCIF_CLKEN  BIT(1)

#define BPMP_CACHE_MAINT_ADDR           0x20
#define BPMP_CACHE_MAINT_DATA           0x24

#define BPMP_CACHE_MAINT_REQ            0x28
#define  MAINT_REQ_WAY_BITMAP(x)        ((x) << 8)

#define BPMP_CACHE_INT_MASK             0x40
#define BPMP_CACHE_INT_CLEAR            0x44
#define BPMP_CACHE_INT_RAW_EVENT        0x48
#define BPMP_CACHE_INT_STATUS           0x4C
#define  INT_MAINT_DONE                 BIT(0)
#define  INT_MAINT_ERROR                BIT(1)

#define BPMP_CACHE_RB_CFG               0x80
#define BPMP_CACHE_WB_CFG               0x84

#define BPMP_CACHE_MMU_FALLBACK_ENTRY   0xA0
#define BPMP_CACHE_MMU_SHADOW_COPY_MASK 0xA4

#define BPMP_CACHE_MMU_CFG              0xAC
#define  MMU_CFG_BLOCK_MAIN_ENTRY_WR    BIT(0)
#define  MMU_CFG_SEQ_EN                 BIT(1)
#define  MMU_CFG_TLB_EN                 BIT(2)
#define  MMU_CFG_SEG_CHECK_ALL_ENTRIES  BIT(3)
#define  MMU_CFG_ABORT_STORE_LAST       BIT(4)
#define  MMU_CFG_CLR_ABORT              BIT(5)

#define BPMP_CACHE_MMU_CMD              0xB0
#define  MMU_CMD_NOP                    0
#define  MMU_CMD_INIT                   1
#define  MMU_CMD_COPY_SHADOW            2

#define BPMP_CACHE_MMU_ABORT_STAT       0xB4
#define  ABORT_STAT_UNIT_MASK           0x7
#define  ABORT_STAT_UNIT_NONE           0
#define  ABORT_STAT_UNIT_CACHE          1
#define  ABORT_STAT_UNIT_SEQ            2
#define  ABORT_STAT_UNIT_TLB            3
#define  ABORT_STAT_UNIT_SEG            4
#define  ABORT_STAT_UNIT_FALLBACK       5
#define  ABORT_STAT_OVERLAP             BIT(3)
#define  ABORT_STAT_ENTRY               (0x1F << 4)
#define  ABORT_STAT_TYPE_MASK           (3 << 16)
#define  ABORT_STAT_TYPE_EXE            (0 << 16)
#define  ABORT_STAT_TYPE_RD             (1 << 16)
#define  ABORT_STAT_TYPE_WR             (2 << 16)
#define  ABORT_STAT_SIZE                (3 << 18)
#define  ABORT_STAT_SEQ                 BIT(20)
#define  ABORT_STAT_PROT                BIT(21)

#define BPMP_CACHE_MMU_ABORT_ADDR       0xB8
#define BPMP_CACHE_MMU_ACTIVE_ENTRIES   0xBC

#define BPMP_MMU_SHADOW_ENTRY_BASE      (BPMP_CACHE_BASE + 0x400)
#define BPMP_MMU_MAIN_ENTRY_BASE        (BPMP_CACHE_BASE + 0x800)
#define  MMU_EN_CACHED                  BIT(0)
#define  MMU_EN_EXEC                    BIT(1)
#define  MMU_EN_READ                    BIT(2)
#define  MMU_EN_WRITE                   BIT(3)

bpmp_mmu_entry_t mmu_entries[] =
{
	{ DRAM_START, 0xFFFFFFFF, MMU_EN_READ | MMU_EN_WRITE | MMU_EN_EXEC | MMU_EN_CACHED, true },
	{ IRAM_BASE,  0x4003FFFF, MMU_EN_READ | MMU_EN_WRITE | MMU_EN_EXEC | MMU_EN_CACHED, true }
};

void bpmp_mmu_maintenance(u32 op, bool force)
{
	if (!force && !(BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) & CFG_ENABLE_CACHE))
		return;

	BPMP_CACHE_CTRL(BPMP_CACHE_INT_CLEAR) = INT_MAINT_DONE;

	// This is a blocking operation.
	BPMP_CACHE_CTRL(BPMP_CACHE_MAINT_REQ) = MAINT_REQ_WAY_BITMAP(0xF) | op;

	while(!(BPMP_CACHE_CTRL(BPMP_CACHE_INT_RAW_EVENT) & INT_MAINT_DONE))
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
		mmu_entry->start_addr = ALIGN(entry->start_addr, BPMP_MMU_CACHE_LINE_SIZE);
		mmu_entry->end_addr = ALIGN_DOWN(entry->end_addr, BPMP_MMU_CACHE_LINE_SIZE);
		mmu_entry->attr = entry->attr;

		BPMP_CACHE_CTRL(BPMP_CACHE_MMU_SHADOW_COPY_MASK) |= BIT(idx);

		if (apply)
			BPMP_CACHE_CTRL(BPMP_CACHE_MMU_CMD) = MMU_CMD_COPY_SHADOW;
	}
}

void bpmp_mmu_enable()
{
	if (BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) & CFG_ENABLE_CACHE)
		return;

	// Init BPMP MMU.
	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_CMD) = MMU_CMD_INIT;
	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_FALLBACK_ENTRY) = MMU_EN_READ | MMU_EN_WRITE | MMU_EN_EXEC; // RWX for non-defined regions.
	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_CFG) = MMU_CFG_SEQ_EN | MMU_CFG_TLB_EN | MMU_CFG_ABORT_STORE_LAST;

	// Init BPMP MMU entries.
	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_SHADOW_COPY_MASK) = 0;
	for (u32 idx = 0; idx < ARRAY_SIZE(mmu_entries); idx++)
		bpmp_mmu_set_entry(idx, &mmu_entries[idx], false);

	BPMP_CACHE_CTRL(BPMP_CACHE_MMU_CMD) = MMU_CMD_COPY_SHADOW;

	// Invalidate cache.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, true);

	// Enable cache.
	BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) = CFG_ENABLE_CACHE | CFG_FORCE_WRITE_THROUGH |
		CFG_MMU_TAG_MODE(TAG_MODE_PARALLEL) | CFG_TAG_CHK_ABRT_ON_ERR;

	// HW bug. Invalidate cache again.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, false);
}

void bpmp_mmu_disable()
{
	if (!(BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) & CFG_ENABLE_CACHE))
		return;

	// Clean and invalidate cache.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	// Disable cache.
	BPMP_CACHE_CTRL(BPMP_CACHE_CONFIG) = 0;
}

// APB clock affects RTC, PWM, MEMFETCH, APE, USB, SOR PWM,
// I2C host, DC/DSI/DISP. UART gives extra stress.
// 92: 100% success ratio. 93-94: 595-602MHz has 99% success ratio. 95: 608MHz less.
const u8 pll_divn[] = {
	0,   // BPMP_CLK_NORMAL:      408MHz  0% - 136MHz APB.
	85,  // BPMP_CLK_HIGH_BOOST:  544MHz 33% - 136MHz APB.
	90,  // BPMP_CLK_SUPER_BOOST: 576MHz 41% - 144MHz APB.
	92   // BPMP_CLK_HYPER_BOOST: 589MHz 44% - 147MHz APB.
	// Do not use for public releases!
	//95   // BPMP_CLK_DEV_BOOST: 608MHz 49% - 152MHz APB.
};

bpmp_freq_t bpmp_fid_current = BPMP_CLK_NORMAL;

void bpmp_clk_rate_get()
{
	bool clk_src_is_pllp = ((CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) >> 4) & 7) == 3;

	if (clk_src_is_pllp)
		bpmp_fid_current = BPMP_CLK_NORMAL;
	else
	{
		bpmp_fid_current = BPMP_CLK_HIGH_BOOST;

		u8 pll_divn_curr = (CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) >> 10) & 0xFF;
		for (u32 i = 1; i < sizeof(pll_divn); i++)
		{
			if (pll_divn[i] == pll_divn_curr)
			{
				bpmp_fid_current = i;
				break;
			}
		}
	}
}

bpmp_freq_t bpmp_clk_rate_set(bpmp_freq_t fid)
{
	bpmp_freq_t prev_fid = bpmp_fid_current;

	if (fid > (BPMP_CLK_MAX - 1))
		fid = BPMP_CLK_MAX - 1;

	if (prev_fid == fid)
		return prev_fid;

	if (fid)
	{
		if (prev_fid)
		{
			// Restore to PLLP source during PLLC configuration.
			CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003333; // PLLP_OUT.
			msleep(1); // Wait a bit for clock source change.
		}

		// Configure and enable PLLC.
		clock_enable_pllc(pll_divn[fid]);

		// Set SCLK / HCLK / PCLK.
		CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 3; // PCLK = HCLK / (3 + 1). HCLK == SCLK.
		CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003310; // PLLC_OUT1 for active and CLKM for idle.
	}
	else
	{
		CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003330; // PLLP_OUT for active and CLKM for idle.
		msleep(1); // Wait a bit for clock source change.
		CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 2; // PCLK = HCLK / (2 + 1). HCLK == SCLK.

		// Disable PLLC to save power.
		clock_disable_pllc();
	}
	bpmp_fid_current = fid;

	// Return old fid in case of temporary swap.
	return prev_fid;
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
