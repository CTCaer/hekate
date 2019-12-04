/*
 * Copyright (c) 2018 naehrwert
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

#include "../soc/cluster.h"
#include "../soc/i2c.h"
#include "../soc/clock.h"
#include "../utils/util.h"
#include "../soc/pmc.h"
#include "../soc/t210.h"
#include "../power/max77620.h"
#include "../power/max7762x.h"

void _cluster_enable_power()
{
	u8 tmp = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_AME_GPIO); // Get current pinmuxing
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_AME_GPIO, tmp & ~(1 << 5)); // Disable GPIO5 pinmuxing.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_GPIO5, MAX77620_CNFG_GPIO_DRV_PUSHPULL | MAX77620_CNFG_GPIO_OUTPUT_VAL_HIGH);

	// Enable cores power.
	// 1-3.x: MAX77621_NFSR_ENABLE.
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_CONTROL1_REG,
		MAX77621_AD_ENABLE | MAX77621_NFSR_ENABLE | MAX77621_SNS_ENABLE | MAX77621_RAMP_12mV_PER_US);
	// 1.0.0-3.x: MAX77621_T_JUNCTION_120 | MAX77621_CKKADV_TRIP_DISABLE | MAX77621_INDUCTOR_NOMINAL.
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_CONTROL2_REG,
		MAX77621_T_JUNCTION_120 | MAX77621_WDTMR_ENABLE | MAX77621_CKKADV_TRIP_75mV_PER_US| MAX77621_INDUCTOR_NOMINAL);
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_VOUT_REG, MAX77621_VOUT_ENABLE | MAX77621_VOUT_0_95V);
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_VOUT_DVC_REG, MAX77621_VOUT_ENABLE | MAX77621_VOUT_0_95V);
}

int _cluster_pmc_enable_partition(u32 part, int enable)
{
	u32 part_mask = 1 << part;
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

void cluster_boot_cpu0(u32 entry)
{
	// Set ACTIVE_CLUSER to FAST.
	FLOW_CTLR(FLOW_CTLR_BPMP_CLUSTER_CONTROL) &= 0xFFFFFFFE;

	_cluster_enable_power();

	if (!(CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) & 0x40000000)) // PLLX_ENABLE.
	{
		CLOCK(CLK_RST_CONTROLLER_PLLX_MISC_3) &= 0xFFFFFFF7; // Disable IDDQ.
		usleep(2);
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = 0x80404E02;
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = 0x404E02;
		CLOCK(CLK_RST_CONTROLLER_PLLX_MISC) = (CLOCK(CLK_RST_CONTROLLER_PLLX_MISC) & 0xFFFBFFFF) | 0x40000;
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = 0x40404E02;
	}
	while (!(CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) & 0x8000000))
		;

	// Configure MSELECT source and enable clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT) & 0x1FFFFF00) | 6;
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) = (CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) & 0xFFFFFFF7) | 8;

	// Configure initial CPU clock frequency and enable clock.
	CLOCK(CLK_RST_CONTROLLER_CCLK_BURST_POLICY) = 0x20008888;
	CLOCK(CLK_RST_CONTROLLER_SUPER_CCLK_DIVIDER) = 0x80000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET) = 1;

	clock_enable_coresight();

	// CAR2PMC_CPU_ACK_WIDTH should be set to 0.
	CLOCK(CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2) &= 0xFFFFF000;

	// Enable CPU rail.
	_cluster_pmc_enable_partition(0, 1);
	// Enable cluster 0 non-CPU.
	_cluster_pmc_enable_partition(15, 1);
	// Enable CE0.
	_cluster_pmc_enable_partition(14, 1);

	// Request and wait for RAM repair.
	FLOW_CTLR(FLOW_CTLR_RAM_REPAIR) = 1;
	while (!(FLOW_CTLR(FLOW_CTLR_RAM_REPAIR) & 2))
		;

	EXCP_VEC(EVP_CPU_RESET_VECTOR) = 0;

	// Set reset vector.
	SB(SB_AA64_RESET_LOW) = entry | SB_AA64_RST_AARCH64_MODE_EN;
	SB(SB_AA64_RESET_HIGH) = 0;
	// Non-secure reset vector write disable.
	SB(SB_CSR) = SB_CSR_NS_RST_VEC_WR_DIS;
	(void)SB(SB_CSR);

	// Tighten up the security aperture.
	// MC(MC_TZ_SECURITY_CTRL) = 1;

	// Clear MSELECT reset.
	CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_V) &= 0xFFFFFFF7;
	// Clear NONCPU reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = 0x20000000;
	// Clear CPU0 reset.
	// < 5.x: 0x411F000F, Clear CPU{0,1,2,3} POR and CORE, CX0, L2, and DBG reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = 0x41010001;
}
