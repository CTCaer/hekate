/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2020-2024 CTCaer
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

#include <mem/emc_t210.h>

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
	LPDDR4_ICOSA_4GB_SAMSUNG_K4F6E304HB_MGCH        = 0, // Die-B. (2y-01).
	LPDDR4_ICOSA_4GB_HYNIX_H9HCNNNBPUMLHR_NLE       = 1, // Die-M. (2y-01).
	LPDDR4_ICOSA_4GB_MICRON_MT53B512M32D2NP_062_WTC = 2, // Die-C. (2y-01).

	LPDDR4_ICOSA_6GB_SAMSUNG_K4FHE3D4HM_MGCH        = 4, // Die-C. (2y-01).

	/*
	 * Custom hekate/L4T supported 8GB. 7 dram id can be easily applied in fuses.
	 *
	 * 4GB modules:
	 * Samsung K4FBE3D4HM-MGCH/CJ/CL. MG/TF/GF/TH/GH: Package + Temperature.
	 * Hynix   H9HCNNNCPUMLXR-NME/NEE/NEI. E/I: Temperature.
	 * Hynix   H54G56BYYVX046/QX046/PX046. V/Q/P: Package + Temperature.
	 */
	LPDDR4_ICOSA_8GB_SAMSUNG_K4FBE3D4HM_MGXX        = 7, // XX: CH/CJ/CL.
};

enum sdram_ids_mariko
{
	/*
	 * Nintendo Switch LPDRR4X generations:
	 * - 1x   nm are 1st-gen
	 * - 1y   nm are 2nd-gen
	 * - 1z/a nm are 3rd-gen
	 */

	// LPDDR4X 4266Mbps.
	LPDDR4X_HOAG_4GB_HYNIX_H9HCNNNBKMMLXR_NEE       = 3, // Die-M. (1y-01).
	LPDDR4X_AULA_4GB_HYNIX_H9HCNNNBKMMLXR_NEE       = 5, // Die-M. (1y-01).
	LPDDR4X_IOWA_4GB_HYNIX_H9HCNNNBKMMLXR_NEE       = 6, // Die-M. (1y-01).

	// LPDDR4X 3733Mbps.
	LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AM_MGCJ        = 8,  // Die-M. (1x-03).
	LPDDR4X_IOWA_8GB_SAMSUNG_K4UBE3D4AM_MGCJ        = 9,  // Die-M. (1x-03).
	LPDDR4X_IOWA_4GB_HYNIX_H9HCNNNBKMMLHR_NME       = 10, // Die-M. (1x-03).
	LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D2NP_046_WTE = 11, // Die-E. (1x-03). D9WGB. 4266Mbps.

	LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AM_MGCJ        = 12, // Die-M. (1x-03).
	LPDDR4X_HOAG_8GB_SAMSUNG_K4UBE3D4AM_MGCJ        = 13, // Die-M. (1x-03).
	LPDDR4X_HOAG_4GB_HYNIX_H9HCNNNBKMMLHR_NME       = 14, // Die-M. (1x-03).
	LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WTE = 15, // Die-E. (1x-03). D9WGB. 4266Mbps.

	// LPDDR4X 4266Mbps.
	LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AA_MGCL        = 17, // Die-A. (1y-X03).
	LPDDR4X_IOWA_8GB_SAMSUNG_K4UBE3D4AA_MGCL        = 18, // Die-A. (1y-X03).
	LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AA_MGCL        = 19, // Die-A. (1y-X03).

	LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AB_MGCL        = 20, // Die-B. (1z-01). 40% lp.
	LPDDR4X_HOAG_4GB_SAMSUNG_K4U6E3S4AB_MGCL        = 21, // Die-B. (1z-01). 40% lp.
	LPDDR4X_AULA_4GB_SAMSUNG_K4U6E3S4AB_MGCL        = 22, // Die-B. (1z-01). 40% lp.

	LPDDR4X_HOAG_8GB_SAMSUNG_K4UBE3D4AA_MGCL        = 23, // Die-A. (1y-X03).
	LPDDR4X_AULA_4GB_SAMSUNG_K4U6E3S4AA_MGCL        = 24, // Die-A. (1y-X03).

	LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D2NP_046_WTF = 25, // Die-F. (1y-01). D9XRR.
	LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WTF = 26, // Die-F. (1y-01). D9XRR.
	LPDDR4X_AULA_4GB_MICRON_MT53E512M32D2NP_046_WTF = 27, // Die-F. (1y-01). D9XRR.

	LPDDR4X_AULA_8GB_SAMSUNG_K4UBE3D4AA_MGCL        = 28, // Die-A. (1y-X03). 2nd gen.

	// Old naming scheme: H9HCNNNBKMCLXR-NEE
	LPDDR4X_IOWA_4GB_HYNIX_H54G46CYRBX267           = 29, // Die-C. (1a-01). 61% lp.
	LPDDR4X_HOAG_4GB_HYNIX_H54G46CYRBX267           = 30, // Die-C. (1a-01). 61% lp.
	LPDDR4X_AULA_4GB_HYNIX_H54G46CYRBX267           = 31, // Die-C. (1a-01). 61% lp.

	LPDDR4X_IOWA_4GB_MICRON_MT53E512M32D1NP_046_WTB = 32, // Die-B. (1a-01). D8BQM. 61% lp.
	LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D1NP_046_WTB = 33, // Die-B. (1a-01). D8BQM. 61% lp.
	LPDDR4X_AULA_4GB_MICRON_MT53E512M32D1NP_046_WTB = 34, // Die-B. (1a-01). D8BQM. 61% lp.
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
	LPDDR4X_4GB_HYNIX_H54G46CYRBX267           = 8, // DRAM IDs: 29, 30, 31.
	LPDDR4X_4GB_MICRON_MT53E512M32D1NP_046_WTB = 9, // DRAM IDs: 32, 33, 34.
};

void sdram_init();
void *sdram_get_params_patched();
void *sdram_get_params_t210b01();
void sdram_lp0_save_params(const void *params);
void sdram_src_pllc(bool enable);
emc_mr_data_t sdram_read_mrx(emc_mr_t mrx);

#endif
