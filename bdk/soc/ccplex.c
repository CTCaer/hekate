/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2024 CTCaer
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

#define CCPLEX_FLOWCTRL_POWERGATING 0

static void _ccplex_enable_power_t210()
{
	// Configure GPIO5 and enable output in order to power CPU pmic.
	max77620_config_gpio(5, MAX77620_GPIO_OUTPUT_ENABLE);

	// Configure CPU pmic.
	// 1-3.x: MAX77621_NFSR_ENABLE.
	// 1.0.0-3.x: MAX77621_T_JUNCTION_120 | MAX77621_CKKADV_TRIP_DISABLE | MAX77621_INDUCTOR_NOMINAL.
	max77621_config_default(REGULATOR_CPU0, MAX77621_CTRL_HOS_CFG);

	// Set voltage and enable cluster power.
	max7762x_regulator_set_voltage(REGULATOR_CPU0, 950000);
	max7762x_regulator_enable(REGULATOR_CPU0, true);
}

static void _ccplex_enable_power_t210b01()
{
	// Set voltage and enable cluster power.
	max7762x_regulator_set_voltage(REGULATOR_CPU1, 800000);
	max7762x_regulator_enable(REGULATOR_CPU1, true);
}

static void _ccplex_disable_power()
{
	// Disable cluster power.
	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210)
	{
		max7762x_regulator_enable(REGULATOR_CPU0, false);
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_GPIO5, 0);
	}
	else
		max7762x_regulator_enable(REGULATOR_CPU1, false);
}

void ccplex_boot_cpu0(u32 entry, bool lock)
{
	// Set ACTIVE_CLUSER to FAST.
	FLOW_CTLR(FLOW_CTLR_BPMP_CLUSTER_CONTROL) &= ~CLUSTER_CTRL_ACTIVE_SLOW;

	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210)
		_ccplex_enable_power_t210();
	else
		_ccplex_enable_power_t210b01();

	clock_enable_pllx();

	// Configure MSELECT source and enable clock to 102MHz.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT) = (0 << 29) | CLK_SRC_DIV(4);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET)      = BIT(CLK_V_MSELECT);

	// Configure initial CPU clock frequency and enable clock.
	CLOCK(CLK_RST_CONTROLLER_CCLK_BURST_POLICY)  = 0x20008888; // PLLX_OUT0_LJ.
	CLOCK(CLK_RST_CONTROLLER_SUPER_CCLK_DIVIDER) = BIT(31);    // SUPER_CDIV_ENB.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET)      = BIT(CLK_V_CPUG);

	clock_enable_coresight();

	// CAR2PMC_CPU_ACK_WIDTH should be set to 0.
	CLOCK(CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2) &= 0xFFFFF000;

	// Enable CPU main rail.
	pmc_domain_pwrgate_set(POWER_RAIL_CRAIL, ENABLE);
	// Enable cluster 0 non-CPU rail.
	pmc_domain_pwrgate_set(POWER_RAIL_C0NC,  ENABLE);
	// Enable CPU0 rail.
	pmc_domain_pwrgate_set(POWER_RAIL_CE0,   ENABLE);

	// Request and wait for RAM repair. Needed for the Fast cluster.
	FLOW_CTLR(FLOW_CTLR_RAM_REPAIR) = RAM_REPAIR_REQ;
	while (!(FLOW_CTLR(FLOW_CTLR_RAM_REPAIR) & RAM_REPAIR_STS))
		;

	EXCP_VEC(EVP_CPU_RESET_VECTOR) = 0;

	// Set reset vector.
	SB(SB_AA64_RESET_LOW)  = entry | SB_AA64_RST_AARCH64_MODE_EN;
	SB(SB_AA64_RESET_HIGH) = 0;

	// Non-secure reset vector write disable.
	if (lock)
	{
		SB(SB_CSR) = SB_CSR_NS_RST_VEC_WR_DIS;
		(void)SB(SB_CSR);
	}

	// Tighten up the security aperture.
	// MC(MC_TZ_SECURITY_CTRL) = TZ_SEC_CTRL_CPU_STRICT_TZ_APERTURE_CHECK;

	// Clear MSELECT reset.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_V_CLR)      = BIT(CLK_V_MSELECT);
	// Clear NONCPU reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = BIT(29); // CLR_NONCPURESET.
	// Clear CPU0 reset.
	// < 5.x: 0x411F000F, Clear CPU{0,1,2,3} POR and CORE, CX0, L2, and DBG reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = BIT(30) | BIT(24) | BIT(16) | BIT(0);
}

void ccplex_powergate_cpu0()
{
#if CCPLEX_FLOWCTRL_POWERGATING
	// Halt CPU0.
	FLOW_CTLR(FLOW_CTLR_HALT_CPU0_EVENTS) = HALT_MODE_STOP_UNTIL_IRQ;

	// Powergate cluster via flow control without waiting for WFI.
	FLOW_CTLR(FLOW_CTLR_CPU0_CSR) = CSR_INTR_FLAG | CSR_EVENT_FLAG | CSR_ENABLE_EXT_CPU_RAIL | CSR_WAIT_WFI_NONE | CSR_ENABLE;

	// Wait for the rail power off to finish.
	while((FLOW_CTLR(FLOW_CTLR_CPU_PWR_CSR) & CPU_PWR_RAIL_STS_MASK) != CPU_PWR_RAIL_OFF);

	// Set CPU0 to waitevent.
	FLOW_CTLR(FLOW_CTLR_HALT_CPU0_EVENTS) = HALT_MODE_WAITEVENT;
#endif

	// Set CPU0 POR and CORE, CX0, L2, and DBG reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_SET) = BIT(30) | BIT(24) | BIT(16) | BIT(0);
	// Set NONCPU reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_SET) = BIT(29);
	// Set MSELECT reset.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_V_SET) = BIT(CLK_V_MSELECT);

	// Disable CE0.
	pmc_domain_pwrgate_set(POWER_RAIL_CE0,   DISABLE);
	// Disable cluster 0 non-CPU.
	pmc_domain_pwrgate_set(POWER_RAIL_C0NC,  DISABLE);
	// Disable CPU rail.
	pmc_domain_pwrgate_set(POWER_RAIL_CRAIL, DISABLE);

	clock_disable_coresight();

	// Clear out MSELECT and CPU clocks.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_CLR) = BIT(CLK_V_MSELECT) | BIT(CLK_V_CPUG);

	_ccplex_disable_power();
}
