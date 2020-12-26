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
#include <soc/t210.h>
#include <utils/util.h>

void pmc_scratch_lock(pmc_sec_lock_t lock_mask)
{
	// Lock Private key disable, Fuse write enable, MC carveout, Warmboot PA id and Warmboot address.
	if (lock_mask & PMC_SEC_LOCK_MISC)
	{
		PMC(APBDEV_PMC_SEC_DISABLE)  |= 0x700FF0;   // RW lock: 0-3.
		PMC(APBDEV_PMC_SEC_DISABLE2) |= 0xFC000000; // RW lock: 21-23.
		PMC(APBDEV_PMC_SEC_DISABLE3) |= 0x3F0FFF00; // RW lock: 28-33, 36-38.
		PMC(APBDEV_PMC_SEC_DISABLE6) |= 0xC000000;  // RW lock: 85.
		PMC(APBDEV_PMC_SEC_DISABLE8) |= 0xFF00FF00; // RW lock: 108-111, 116-119.

		// SE2 context.
		if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210B01)
		{
			PMC(APBDEV_PMC_SEC_DISABLE9) |= 0x3FF;      // RW lock: 120-124. (0xB38)
			PMC(APBDEV_PMC_SEC_DISABLE10) = 0xFFFFFFFF; // RW lock: 135-150.
		}
 	}

	if (lock_mask & PMC_SEC_LOCK_LP0_PARAMS)
	{
		PMC(APBDEV_PMC_SEC_DISABLE2) |= 0x3FCFFFF;  // RW lock: 8-15, 17-20.
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

	if (lock_mask & PMC_SEC_LOCK_TZ_CMAC_W)
		PMC(APBDEV_PMC_SEC_DISABLE8) |= 0x550000;   // W lock: 112-115.

	if (lock_mask & PMC_SEC_LOCK_TZ_CMAC_R)
		PMC(APBDEV_PMC_SEC_DISABLE8) |= 0xAA0000;   // R lock: 112-115.

	if (lock_mask & PMC_SEC_LOCK_TZ_KEK_W)
		PMC(APBDEV_PMC_SEC_DISABLE3) |= 0x55;       // W lock: 24-27.

	if (lock_mask & PMC_SEC_LOCK_TZ_KEK_R)
		PMC(APBDEV_PMC_SEC_DISABLE3) |= 0xAA;       // R lock: 24-27.

	if (lock_mask & PMC_SEC_LOCK_SE_SRK)
		PMC(APBDEV_PMC_SEC_DISABLE)  |= 0xFF000;    // RW lock: 4-7
}

int pmc_enable_partition(pmc_power_rail_t part, u32 enable)
{
	u32 part_mask = BIT(part);
	u32 desired_state = enable << part;

	// Check if the partition has the state we want.
	if ((PMC(APBDEV_PMC_PWRGATE_STATUS) & part_mask) == desired_state)
		return 1;

	u32 i = 5001;
	while (PMC(APBDEV_PMC_PWRGATE_TOGGLE) & 0x100)
	{
		usleep(1);
		i--;
		if (i < 1)
			return 0;
	}

	// Toggle power gating.
	PMC(APBDEV_PMC_PWRGATE_TOGGLE) = part | 0x100;

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
