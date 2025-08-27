/*
 * Copyright (c) 2020 CTCaer
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

#include <soc/hw_init.h>
#include <soc/pmc.h>
#include <soc/timer.h>
#include <soc/t210.h>

void pmc_scratch_lock(pmc_sec_lock_t lock_mask)
{
	// Lock Private key disable, Fuse write enable, MC carveout, Warmboot PA id and Warmboot address.

	// Happens on T210B01 LP0 always.
	if (lock_mask & PMC_SEC_LOCK_MISC)
	{
		PMC(APBDEV_PMC_SEC_DISABLE)  |= 0x700FF0;   // RW lock: 0-3.
		PMC(APBDEV_PMC_SEC_DISABLE2) |= 0xFC000000; // RW lock: 21-23.
		PMC(APBDEV_PMC_SEC_DISABLE3) |= 0x3F0FFF00; // RW lock: 28-33, 36-38.
		PMC(APBDEV_PMC_SEC_DISABLE6) |= 0xC000000;  // RW lock: 85.
		// Default: 0xFF00FF00: RW lock: 108-111, 116-119. Gets locked in LP0.
		PMC(APBDEV_PMC_SEC_DISABLE8) |= 0xFF005500; // W lock: 108-111, RW lock: 116-119.
 	}

 	// Happens on T210B01 LP0 always.
	if (lock_mask & PMC_SEC_LOCK_LP0_PARAMS)
	{
		PMC(APBDEV_PMC_SEC_DISABLE2) |= 0x3FCFFFF;  // RW lock: 8-15, 17-20. L4T expects 8-15 as write locked only.
		PMC(APBDEV_PMC_SEC_DISABLE4) |= 0x3F3FFFFF; // RW lock: 40-50, 52-54.
		PMC(APBDEV_PMC_SEC_DISABLE5)  = 0xFFFFFFFF; // RW lock: 56-71.
		PMC(APBDEV_PMC_SEC_DISABLE6) |= 0xF3FFC00F; // RW lock: 72-73, 79-84, 86-87.
		PMC(APBDEV_PMC_SEC_DISABLE7) |= 0x3FFFFF;   // RW lock: 88-98.
		PMC(APBDEV_PMC_SEC_DISABLE8) |= 0xFF;       // RW lock: 104-107.
	}

	if (lock_mask & PMC_SEC_LOCK_RST_VECTOR)
		PMC(APBDEV_PMC_SEC_DISABLE3) |= 0xF00000;   // RW lock: 34-35.

	if (lock_mask & PMC_SEC_LOCK_CARVEOUTS)
	{
		PMC(APBDEV_PMC_SEC_DISABLE2) |= 0x30000;    // RW lock: 16.
		PMC(APBDEV_PMC_SEC_DISABLE3) |= 0xC0000000; // RW lock: 39.
		PMC(APBDEV_PMC_SEC_DISABLE4) |= 0xC0C00000; // RW lock: 51, 55.
		PMC(APBDEV_PMC_SEC_DISABLE6) |= 0x3FF0;     // RW lock: 74-78.
		PMC(APBDEV_PMC_SEC_DISABLE7) |= 0xFFC00000; // RW lock: 99-103.
	}

	// HOS specific.
	if (lock_mask & PMC_SEC_LOCK_TZ_CMAC_W)
		PMC(APBDEV_PMC_SEC_DISABLE8) |= 0x550000;   // W lock: 112-115.

	if (lock_mask & PMC_SEC_LOCK_TZ_CMAC_R)
		PMC(APBDEV_PMC_SEC_DISABLE8) |= 0xAA0000;   // R lock: 112-115.

	if (lock_mask & PMC_SEC_LOCK_TZ_KEK_W)
		PMC(APBDEV_PMC_SEC_DISABLE3) |= 0x55;       // W lock: 24-27.

	if (lock_mask & PMC_SEC_LOCK_TZ_KEK_R)
		PMC(APBDEV_PMC_SEC_DISABLE3) |= 0xAA;       // R lock: 24-27.
	// End of HOS specific.

	if (lock_mask & PMC_SEC_LOCK_SE_SRK)
		PMC(APBDEV_PMC_SEC_DISABLE)  |= 0xFF000;    // RW lock: 4-7

	if (lock_mask & PMC_SEC_LOCK_SE2_SRK_B01)
		PMC(APBDEV_PMC_SEC_DISABLE9)  |= 0x3FC;     // RW lock: 120-123 (T210B01). LP0 also sets global bits (b0-1).

	if (lock_mask & PMC_SEC_LOCK_MISC_B01)
		PMC(APBDEV_PMC_SEC_DISABLE10) = 0xFFFFFFFF; // RW lock: 135-150. Happens on T210B01 LP0 always.

	if (lock_mask & PMC_SEC_LOCK_CARVEOUTS_L4T)
		PMC(APBDEV_PMC_SEC_DISABLE2) |= 0x5555;     // W: 8-15 LP0 and Carveouts. Superseded by LP0 lock.

	// NVTBOOT misses APBDEV_PMC_SCRATCH_WRITE_LOCK_DISABLE_STICKY. bit0: SCRATCH_WR_DIS_ON.
	// They could also use the NS write disable registers instead.
	if (lock_mask & PMC_SEC_LOCK_LP0_PARAMS_B01)
	{
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE0) |= 0xCBCFE0;   // W lock: 5-11, 14-17, 19, 22-23.
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE1) |= 0x583FF;    // W lock: 24-33, 39-40, 42.
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE2) |= 0x1BE;      // W lock: 44-48, 50-51.
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE3)  = 0xFFFFFFFF; // W lock: 56-87.
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE4) |= 0xFFFFFFF;  // W lock: 88-115.
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE5) |= 0xFFFFFFF8; // W lock: 123-151.
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE6)  = 0xFFFFFFFF; // W lock: 152-183.
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE7) |= 0xFC00FFFF; // W lock: 184-199, 210-215.
		PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE8) |= 0xF;        // W lock: 216-219.
	}
}

/*
 * !TODO: Non CCPLEX power domains power gating/ungating.
 * Power gating: clock should be in reset if enabled and then
 * pmc_domain_pwrgate_set is run.
 * Power ungating: run pmc_domain_pwrgate_set, enable clocks and keep in
 * reset, remove clamping, remove reset, run mbist war if T210 and then clocks
 * can be disabled.
 */

int pmc_domain_pwrgate_set(pmc_power_rail_t part, u32 enable)
{
	u32 part_mask = BIT(part);
	u32 desired_state = enable << part;

	// Check if the power domain has the state we want.
	if ((PMC(APBDEV_PMC_PWRGATE_STATUS) & part_mask) == desired_state)
		return 1;

	u32 i = 5001;
	while (PMC(APBDEV_PMC_PWRGATE_TOGGLE) & PMC_PWRGATE_TOGGLE_START)
	{
		usleep(1);
		i--;
		if (i < 1)
			return 0;
	}

	// Toggle power gating.
	PMC(APBDEV_PMC_PWRGATE_TOGGLE) = part | PMC_PWRGATE_TOGGLE_START;

	i = 5001;
	while (i > 0)
	{
		if ((PMC(APBDEV_PMC_PWRGATE_STATUS) & part_mask) == desired_state)
			break;
		usleep(1);
		i--;
	}

	return 1;
}
