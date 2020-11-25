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

#include <soc/pmc.h>
#include <soc/t210.h>
#include <utils/util.h>

int pmc_enable_partition(u32 part, int enable)
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
