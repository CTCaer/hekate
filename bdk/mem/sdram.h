/*
 * Copyright (c) 2018 naehrwert
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

#ifndef _SDRAM_H_
#define _SDRAM_H_

#include <mem/emc.h>

/*
 * Tegra X1/X1+ EMC/DRAM Bandwidth Chart:
 *
 *   40.8 MHz:  0.61 GiB/s
 *   68.0 MHz:  1.01 GiB/s
 *  102.0 MHz:  1.52 GiB/s
 *  204.0 MHz:  3.04 GiB/s <-- Tegra X1/X1+ Init/SC7 Frequency
 *  408.0 MHz:  6.08 GiB/s
 *  665.6 MHz:  9.92 GiB/s
 *  800.0 MHz: 11.92 GiB/s <-- Tegra X1/X1+ Nvidia OS Boot Frequency
 * 1065.6 MHz: 15.89 GiB/s
 * 1331.2 MHz: 19.84 GiB/s
 * 1600.0 MHz: 23.84 GiB/s <-- Tegra X1  Official Max Frequency
 * 1862.4 MHz: 27.75 GiB/s <-- Tegra X1+ Official Max Frequency
 * 2131.2 MHz: 31.76 GiB/s
 *
 * Note: BWbits = Hz x bus width x channels = Hz x 64 x 2.
 */

enum sdram_ids_erista
{
	// LPDDR4 3200Mbps.
	LPDDR4_ICOSA_4GB_SAMSUNG_K4F6E304HB_MGCH        = 0,
	LPDDR4_ICOSA_4GB_HYNIX_H9HCNNNBPUMLHR_NLE       = 1,
	LPDDR4_ICOSA_4GB_MICRON_MT53B512M32D2NP_062_WT  = 2,
	LPDDR4_COPPER_4GB_SAMSUNG_K4F6E304HB_MGCH       = 3,
	LPDDR4_ICOSA_6GB_SAMSUNG_K4FHE3D4HM_MGCH        = 4,
	LPDDR4_COPPER_4GB_HYNIX_H9HCNNNBPUMLHR_NLE      = 5,
	LPDDR4_COPPER_4GB_MICRON_MT53B512M32D2NP_062_WT = 6,
};

enum sdram_ids_mariko
{
	// LPDDR4X 3733Mbps.
	LPDDR4X_IOWA_4GB_SAMSUNG_X1X2                   = 7,

	LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AM_MGCJ        = 8,
	LPDDR4X_IOWA_8GB_SAMSUNG_K4UBE3D4AM_MGCJ        = 9,
	LPDDR4X_IOWA_4GB_HYNIX_H9HCNNNBKMMLHR_NME       = 10,
	LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D2NP_046_WT  = 11, // 4266Mbps.

	LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AM_MGCJ        = 12,
	LPDDR4X_HOAG_8GB_SAMSUNG_K4UBE3D4AM_MGCJ        = 13,
	LPDDR4X_HOAG_4GB_HYNIX_H9HCNNNBKMMLHR_NME       = 14,
	LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WT  = 15, // 4266Mbps.

	// LPDDR4X 4266Mbps?
	LPDDR4X_IOWA_4GB_SAMSUNG_Y                      = 16,

	LPDDR4X_IOWA_4GB_SAMSUNG_1Y_X                   = 17,
	LPDDR4X_IOWA_8GB_SAMSUNG_1Y_X                   = 18,
	LPDDR4X_HOAG_4GB_SAMSUNG_1Y_X                   = 19,

	LPDDR4X_IOWA_4GB_SAMSUNG_1Y_Y                   = 20,
	LPDDR4X_IOWA_8GB_SAMSUNG_1Y_Y                   = 21,

	LPDDR4X_SDS_4GB_SAMSUNG_1Y_A                    = 22,

	LPDDR4X_SDS_8GB_SAMSUNG_1Y_X                    = 23,
	LPDDR4X_SDS_4GB_SAMSUNG_1Y_X                    = 24,

	LPDDR4X_IOWA_4GB_MICRON_1Y_A                    = 25,
	LPDDR4X_HOAG_4GB_MICRON_1Y_A                    = 26,
	LPDDR4X_SDS_4GB_MICRON_1Y_A                     = 27
};

void sdram_init();
void *sdram_get_params_patched();
void *sdram_get_params_t210b01();
void sdram_lp0_save_params(const void *params);
emc_mr_data_t sdram_read_mrx(emc_mr_t mrx);

#endif
