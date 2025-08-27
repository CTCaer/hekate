/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2025 CTCaer
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

#include <memory_map.h>
#include <mem/mc.h>
#include <soc/bpmp.h>
#include <soc/timer.h>
#include <soc/t210.h>
#include <soc/clock.h>

#define HOS_WPR1_BASE 0x80020000

void mc_config_carveout_hos()
{
	// Enable ACR GSR3 and flush data to ram.
	*(u32 *)(HOS_WPR1_BASE + SZ_256K - sizeof(u32)) = ACR_GSC3_ENABLE_MAGIC;
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, false);

	// Set VPR CYA TRUSTED DEFAULT.
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_0) = VPR_OVR0_CYA_TRUST_DEFAULT;
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_1) = 0;

	// Disable VPR carveout.
	MC(MC_VIDEO_PROTECT_BOM)      = 0;
	MC(MC_VIDEO_PROTECT_SIZE_MB)  = 0;
	MC(MC_VIDEO_PROTECT_REG_CTRL) = VPR_CTRL_LOCKED;

	// Disable TZDRAM carveout.
	MC(MC_SEC_CARVEOUT_BOM)      = 0;
	MC(MC_SEC_CARVEOUT_SIZE_MB)  = 0;
	MC(MC_SEC_CARVEOUT_REG_CTRL) = BIT(0);

	// Disable CPU FW carveout.
	MC(MC_MTS_CARVEOUT_BOM)      = 0;
	MC(MC_MTS_CARVEOUT_SIZE_MB)  = 0;
	MC(MC_MTS_CARVEOUT_REG_CTRL) = BIT(0);

	// Disable GEN1 carveout.
	MC(MC_SECURITY_CARVEOUT1_SIZE_128KB) = 0;
	MC(MC_SECURITY_CARVEOUT1_CFG0) = SEC_CARVEOUT_CFG_LOCKED            |
									 SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY |
									 SEC_CARVEOUT_CFG_APERTURE_ID(0)    |
									 SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH;

	// Enable GEN2 carveout as WPR1.
	MC(MC_SECURITY_CARVEOUT2_BOM)        = HOS_WPR1_BASE;
	MC(MC_SECURITY_CARVEOUT2_SIZE_128KB) = SZ_256K / SZ_128K;
	MC(MC_SECURITY_CARVEOUT2_CLIENT_ACCESS2) = SEC_CARVEOUT_CA2_R_GPU  | SEC_CARVEOUT_CA2_W_GPU | SEC_CARVEOUT_CA2_R_TSEC;
	MC(MC_SECURITY_CARVEOUT2_CLIENT_ACCESS4) = SEC_CARVEOUT_CA4_R_GPU2 | SEC_CARVEOUT_CA4_W_GPU2;
	MC(MC_SECURITY_CARVEOUT2_CFG0) = SEC_CARVEOUT_CFG_LOCKED            |
									 SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY |
									 SEC_CARVEOUT_CFG_RD_NS             |
									 SEC_CARVEOUT_CFG_RD_SEC            |
									 SEC_CARVEOUT_CFG_RD_FALCON_LS      |
									 SEC_CARVEOUT_CFG_RD_FALCON_HS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_LS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_HS      |
									 SEC_CARVEOUT_CFG_APERTURE_ID(2)    |
									 SEC_CARVEOUT_CFG_SEND_CFG_TO_GPU   |
									 SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH;

	// Prepare GEN3 carveout as WPR2.
	MC(MC_SECURITY_CARVEOUT3_SIZE_128KB) = 0;
	MC(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS2) = SEC_CARVEOUT_CA2_R_GPU  | SEC_CARVEOUT_CA2_W_GPU;
	MC(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS4) = SEC_CARVEOUT_CA4_R_GPU2 | SEC_CARVEOUT_CA4_W_GPU2;
	MC(MC_SECURITY_CARVEOUT3_CFG0) = SEC_CARVEOUT_CFG_LOCKED            |
									 SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY |
									 SEC_CARVEOUT_CFG_RD_NS             |
									 SEC_CARVEOUT_CFG_RD_SEC            |
									 SEC_CARVEOUT_CFG_RD_FALCON_LS      |
									 SEC_CARVEOUT_CFG_RD_FALCON_HS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_LS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_HS      |
									 SEC_CARVEOUT_CFG_APERTURE_ID(3)    |
									 SEC_CARVEOUT_CFG_SEND_CFG_TO_GPU   |
									 SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH;

	// Disable GEN4 carveout.
	MC(MC_SECURITY_CARVEOUT4_SIZE_128KB) = 0;
	MC(MC_SECURITY_CARVEOUT4_CFG0) = SEC_CARVEOUT_CFG_TZ_SECURE         |
									 SEC_CARVEOUT_CFG_LOCKED            |
									 SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY |
									 SEC_CARVEOUT_CFG_RD_NS             |
									 SEC_CARVEOUT_CFG_WR_NS;

	// Disable GEN5 carveout.
	MC(MC_SECURITY_CARVEOUT5_SIZE_128KB) = 0;
	MC(MC_SECURITY_CARVEOUT5_CFG0) = SEC_CARVEOUT_CFG_TZ_SECURE         |
									 SEC_CARVEOUT_CFG_LOCKED            |
									 SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY |
									 SEC_CARVEOUT_CFG_RD_NS             |
									 SEC_CARVEOUT_CFG_WR_NS;
}

// SDMMC, TSEC, XUSB and probably more need it to access < DRAM_START.
void mc_enable_ahb_redirect()
{
	// Bypass ARC clock gating.
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD) |= BIT(19);
	//MC(MC_IRAM_REG_CTRL) &= ~BIT(0);
	MC(MC_IRAM_BOM) = IRAM_BASE;
	MC(MC_IRAM_TOM) = DRAM_START; // Default is only IRAM: 0x4003F000.
}

void mc_disable_ahb_redirect()
{
	MC(MC_IRAM_BOM) = 0xFFFFF000;
	MC(MC_IRAM_TOM) = 0;
	// Disable IRAM_CFG_WRITE_ACCESS (sticky).
	//MC(MC_IRAM_REG_CTRL) |= BIT(0);
	// Set ARC clock gating to automatic.
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD) &= ~BIT(19);
}

bool mc_client_has_access(void *address)
{
	// Check if address is in DRAM or if arbitration for IRAM is enabled.
	if ((u32)address >= DRAM_START)
		return true; // Access by default.
	else if ((u32)address >= IRAM_BASE && MC(MC_IRAM_BOM) == IRAM_BASE)
		return true; // Access by AHB arbitration.

	// No access to address space.
	return false;
}

void mc_enable()
{
	// Reset EMC source to PLLP.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) & 0x1FFFFFFF) | (2 << 29u);

	// Enable and clear reset for memory clocks.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = BIT(CLK_H_EMC) | BIT(CLK_H_MEM);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) = BIT(CLK_X_EMC_DLL);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_CLR) = BIT(CLK_H_EMC) | BIT(CLK_H_MEM);
	usleep(5);

#ifdef BDK_MC_ENABLE_AHB_REDIRECT
	mc_enable_ahb_redirect();
#else
	mc_disable_ahb_redirect();
#endif
}
