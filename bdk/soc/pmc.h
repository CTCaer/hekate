/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 st4rk
 * Copyright (c) 2018-2026 CTCaer
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

#ifndef _PMC_H_
#define _PMC_H_

#include <utils/types.h>
#include <soc/pmc_t210.h>

typedef enum _pmc_sec_lock_t
{
	PMC_SEC_LOCK_MISC           = BIT(0),
	PMC_SEC_LOCK_LP0_PARAMS     = BIT(1),
	PMC_SEC_LOCK_RST_VECTOR     = BIT(2),
	PMC_SEC_LOCK_CARVEOUTS      = BIT(3),
	PMC_SEC_LOCK_TZ_CMAC_W      = BIT(4),
	PMC_SEC_LOCK_TZ_CMAC_R      = BIT(5),
	PMC_SEC_LOCK_TZ_KEK_W       = BIT(6),
	PMC_SEC_LOCK_TZ_KEK_R       = BIT(7),
	PMC_SEC_LOCK_SE_SRK         = BIT(8),
	PMC_SEC_LOCK_SE2_SRK_B01    = BIT(9),
	PMC_SEC_LOCK_MISC_B01       = BIT(10),
	PMC_SEC_LOCK_CARVEOUTS_L4T  = BIT(11),
	PMC_SEC_LOCK_LP0_PARAMS_B01 = BIT(12),
} pmc_sec_lock_t;

typedef enum _pmc_power_rail_t
{
	POWER_RAIL_CRAIL = 0,
	POWER_RAIL_VE    = 2,
	POWER_RAIL_PCIE  = 3,
	POWER_RAIL_NVENC = 6,
	POWER_RAIL_SATA  = 8,
	POWER_RAIL_CE1   = 9,
	POWER_RAIL_CE2   = 10,
	POWER_RAIL_CE3   = 11,
	POWER_RAIL_CELP  = 12,
	POWER_RAIL_CE0   = 14,
	POWER_RAIL_C0NC  = 15,
	POWER_RAIL_C1NC  = 16,
	POWER_RAIL_SOR   = 17,
	POWER_RAIL_DIS   = 18,
	POWER_RAIL_DISB  = 19,
	POWER_RAIL_XUSBA = 20,
	POWER_RAIL_XUSBB = 21,
	POWER_RAIL_XUSBC = 22,
	POWER_RAIL_VIC   = 23,
	POWER_RAIL_IRAM  = 24,
	POWER_RAIL_NVDEC = 25,
	POWER_RAIL_NVJPG = 26,
	POWER_RAIL_AUD   = 27,
	POWER_RAIL_DFD   = 28,
	POWER_RAIL_VE2   = 29
} pmc_power_rail_t;

void pmc_scratch_lock(pmc_sec_lock_t lock_mask);
int  pmc_domain_pwrgate_set(pmc_power_rail_t part, u32 enable);

#endif
