/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 CTCaer
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

#include <soc/ccplex.h>
#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/clock.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <power/max77620.h>
#include <power/max7762x.h>
#include <power/max77812.h>
#include <utils/util.h>

void _ccplex_enable_power_t210()
{
	// Configure GPIO5 and enable output in order to power CPU pmic.
	max77620_config_gpio(5, MAX77620_GPIO_OUTPUT_ENABLE);

	// Configure CPU pmic.
	// 1-3.x: MAX77621_NFSR_ENABLE.
	// 1.0.0-3.x: MAX77621_T_JUNCTION_120 | MAX77621_CKKADV_TRIP_DISABLE | MAX77621_INDUCTOR_NOMINAL.
	max77621_config_default(REGULATOR_CPU0, MAX77621_CTRL_HOS_CFG);

	// Set voltage and enable cores power.
	max7762x_regulator_set_voltage(REGULATOR_CPU0, 950000);
	max7762x_regulator_enable(REGULATOR_CPU0, true);
}

void _ccplex_enable_power_t210b01()
{
	// Set voltage and enable cores power.
	max7762x_regulator_set_voltage(REGULATOR_CPU1, 800000);
	max7762x_regulator_enable(REGULATOR_CPU1, true);
}

void ccplex_boot_cpu0(u32 entry)
{
	// Set ACTIVE_CLUSER to FAST.
	FLOW_CTLR(FLOW_CTLR_BPMP_CLUSTER_CONTROL) &= 0xFFFFFFFE;

	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210)
		_ccplex_enable_power_t210();
	else
		_ccplex_enable_power_t210b01();

	clock_enable_pllx();

	// Configure MSELECT source and enable clock to 102MHz.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT) & 0x1FFFFF00) | 6;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET) = BIT(CLK_V_MSELECT);

	// Configure initial CPU clock frequency and enable clock.
	CLOCK(CLK_RST_CONTROLLER_CCLK_BURST_POLICY)  = 0x20008888; // PLLX_OUT0_LJ.
	CLOCK(CLK_RST_CONTROLLER_SUPER_CCLK_DIVIDER) = 0x80000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET) = BIT(CLK_V_CPUG);

	clock_enable_coresight();

	// CAR2PMC_CPU_ACK_WIDTH should be set to 0.
	CLOCK(CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2) &= 0xFFFFF000;

	// Enable CPU main rail.
	pmc_enable_partition(POWER_RAIL_CRAIL, ENABLE);
	// Enable cluster 0 non-CPU rail.
	pmc_enable_partition(POWER_RAIL_C0NC,  ENABLE);
	// Enable CPU0 rail.
	pmc_enable_partition(POWER_RAIL_CE0,   ENABLE);

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
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_V_CLR) = BIT(CLK_V_MSELECT);
	// Clear NONCPU reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = 0x20000000;
	// Clear CPU0 reset.
	// < 5.x: 0x411F000F, Clear CPU{0,1,2,3} POR and CORE, CX0, L2, and DBG reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = 0x41010001;
}
