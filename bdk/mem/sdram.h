/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2020-2023 CTCaer
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
 * Note: Max BWbits = Hz x ddr x bus width x channels = Hz x 2 x 32 x 2.
 *       Max BWbits = Hz x ddr x bus width x channels = Hz x 2 x 64 x 1.
 *       Configurations supported: 1x32, 2x32, 1x64.
 *       x64 ram modules can be used by combining the 2 32-bit channels into one.
 *
 *  204.0 MHz:  3.04 <-- Tegra X1/X1+ Init/SC7 Frequency
 *  408.0 MHz:  6.08
 *  665.6 MHz:  9.92
 *  800.0 MHz: 11.92 <-- Tegra X1/X1+ Nvidia OS Boot Frequency
 * 1065.6 MHz: 15.89
 * 1331.2 MHz: 19.84
 * 1600.0 MHz: 23.84
 * 1862.4 MHz: 27.75 <-- Tegra X1  Official Max Frequency
 * 2131.2 MHz: 31.76 <-- Tegra X1+ Official Max Frequency. Not all regs have support for > 2046 MHz.
 */

enum sdram_ids_erista
{
	// LPDDR4 3200Mbps.
	LPDDR4_ICOSA_4GB_SAMSUNG_K4F6E304HB_MGCH        = 0,
	LPDDR4_ICOSA_4GB_HYNIX_H9HCNNNBPUMLHR_NLE       = 1,
	LPDDR4_ICOSA_4GB_MICRON_MT53B512M32D2NP_062_WT  = 2, // WT:C.

	LPDDR4_ICOSA_6GB_SAMSUNG_K4FHE3D4HM_MGCH        = 4,

	LPDDR4_ICOSA_8GB_SAMSUNG_K4FBE3D4HM_MGXX        = 7, // Custom 8GB. XX: CH/CJ/CL. 7 since it can be easily applied in fuses.
};

enum sdram_ids_mariko
{
	// LPDDR4X 4266Mbps.
	LPDDR4X_HOAG_4GB_HYNIX_H9HCNNNBKMMLXR_NEE       = 3, // Replaced from Copper. Die-M. (1y-01).
	LPDDR4X_AULA_4GB_HYNIX_H9HCNNNBKMMLXR_NEE       = 5, // Replaced from Copper. Die-M. (1y-01).
	LPDDR4X_IOWA_4GB_HYNIX_H9HCNNNBKMMLXR_NEE       = 6, // Replaced from Copper. Die-M. (1y-01).

	// LPDDR4X 3733Mbps.
	LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AM_MGCJ        = 8,  // Die-M. 1st gen. 8 banks. 3733Mbps.
	LPDDR4X_IOWA_8GB_SAMSUNG_K4UBE3D4AM_MGCJ        = 9,  // Die-M.
	LPDDR4X_IOWA_4GB_HYNIX_H9HCNNNBKMMLHR_NME       = 10, // Die-M.
	LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D2NP_046_WTE = 11, // 4266Mbps. Die-E. D9WGB.

	LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AM_MGCJ        = 12, // Die-M. 1st gen. 8 banks. 3733Mbps.
	LPDDR4X_HOAG_8GB_SAMSUNG_K4UBE3D4AM_MGCJ        = 13, // Die-M.
	LPDDR4X_HOAG_4GB_HYNIX_H9HCNNNBKMMLHR_NME       = 14, // Die-M.
	LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WTE = 15, // 4266Mbps. Die-E. D9WGB.

	// LPDDR4X 4266Mbps.
	LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AA_MGCL        = 17, // Die-A. (1y-X03). 2nd gen. 8 banks. 4266Mbps.
	LPDDR4X_IOWA_8GB_SAMSUNG_K4UBE3D4AA_MGCL        = 18, // Die-A. (1y-X03).
	LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AA_MGCL        = 19, // Die-A. (1y-X03). 2nd gen. 8 banks. 4266Mbps.

	LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AB_MGCL        = 20, // Die-B. 1z nm. 40% lower power usage. (1z-01).
	LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AB_MGCL        = 21, // Die-B. 1z nm. 40% lower power usage. (1z-01).
	LPDDR4X_AULA_4GB_SAMSUNG_K4U6E3S4AB_MGCL        = 22, // Die-B. 1z nm. 40% lower power usage. (1z-01).

	LPDDR4X_HOAG_8GB_SAMSUNG_K4UBE3D4AA_MGCL        = 23, // Die-A. (1y-X03).
	LPDDR4X_AULA_4GB_SAMSUNG_K4U6E3S4AA_MGCL        = 24, // Die-A. (1y-X03). 2nd gen. 8 banks. 4266Mbps.

	LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D2NP_046_WTF = 25, // 4266Mbps. Die-F. D9XRR. 10nm-class (1y-01).
	LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WTF = 26, // 4266Mbps. Die-F. D9XRR. 10nm-class (1y-01).
	LPDDR4X_AULA_4GB_MICRON_MT53E512M32D2NP_046_WTF = 27, // 4266Mbps. Die-F. D9XRR. 10nm-class (1y-01).

	LPDDR4X_AULA_8GB_SAMSUNG_K4UBE3D4AA_MGCL        = 28, // Die-A.

	LPDDR4X_UNK0_4GB_HYNIX_H9HCNNNBKMMLXR_NEI       = 29, // Die-M. 1a nm. 61% lower power usage. (1a-01).
	LPDDR4X_UNK1_4GB_HYNIX_H9HCNNNBKMMLXR_NEI       = 30, // Die-M. 1a nm. 61% lower power usage. (1a-01).
	LPDDR4X_UNK2_4GB_HYNIX_H9HCNNNBKMMLXR_NEI       = 31, // Die-M. 1a nm. 61% lower power usage. (1a-01).

	LPDDR4X_UNK0_4GB_MICRON_1A                      = 32, // 1a nm. 61% lower power usage. (1a-01).
	LPDDR4X_UNK1_4GB_MICRON_1A                      = 33, // 1a nm. 61% lower power usage. (1a-01).
	LPDDR4X_UNK2_4GB_MICRON_1A                      = 34, // 1a nm. 61% lower power usage. (1a-01).
};

enum sdram_codes_mariko
{
	LPDDR4X_NO_PATCH                           = 0,
	LPDDR4X_UNUSED                             = 0,

	// LPDDR4X_4GB_SAMSUNG_K4U6E3S4AM_MGCJ          DRAM IDs: 08, 12.
	// LPDDR4X_4GB_HYNIX_H9HCNNNBKMMLHR_NME         DRAM IDs: 10, 14.

	LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ        = 1, // DRAM IDs: 09, 13.
	LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE = 2, // DRAM IDs: 11, 15.
	LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL        = 3, // DRAM IDs: 17, 19, 24.
	LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL        = 4, // DRAM IDs: 18, 23, 28.
	LPDDR4X_4GB_SAMSUNG_K4U6E3S4AB_MGCL        = 5, // DRAM IDs: 20, 21, 22.
	LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF = 6, // DRAM IDs: 25, 26, 27.
	LPDDR4X_4GB_HYNIX_H9HCNNNBKMMLXR_NEE       = 7, // DRAM IDs: 03, 05, 06.

	LPDDR4X_4GB_HYNIX_H9HCNNNBKMMLXR_NEI       = 8, // DRAM IDs: 29, 30, 31.
	LPDDR4X_4GB_MICRON_1A                      = 9, // DRAM IDs: 32, 33, 34.
};

void sdram_init();
void *sdram_get_params_patched();
void *sdram_get_params_t210b01();
void sdram_lp0_save_params(const void *params);
emc_mr_data_t sdram_read_mrx(emc_mr_t mrx);

#endif
