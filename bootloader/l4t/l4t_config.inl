/*
 * L4T Loader for Tegra X1
 *
 * Copyright (c) 2020-2022 CTCaer
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

// Set to 1 to enable early boot debugging.
#define DEBUG_LOG_ATF    0
#define DEBUG_LOG_BPMPFW 0 // Do not enable if UART setup is hindered during early boot.

// Set to 1 to lock PMC registers that contain LP0 parameters.
#define LOCK_PMC_REGISTERS 0

// Configurable carveout enable config. Only one can be enabled at a time.
#define CARVEOUT_NVDEC_TSEC_ENABLE 0 // Enable for NVDEC bl/prod and full TOS/DRM.
#define CARVEOUT_SECFW_ENABLE      1 // SECFW is always allocated even if carveout is disabled.

/*
 * WPR Carveout size config.
 *
 * L4T: 2MB or 13MB. On non SecureOS env, only 0x100 bytes are used, probably also on full TOS.
 * On 4GB+ systems, it's normally placed at BANK1_TOP - SIZE;
 */
#define CARVEOUT_GPUWPR_SIZE_CFG (SZ_8M + SZ_4M + SZ_1M) // Mandatory when CARVEOUT_NVDEC_TSEC_ENABLE is 1.
