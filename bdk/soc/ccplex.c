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
#include <soc/fuse.h>
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
	u8 tmp = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_AME_GPIO); // Get current pinmuxing
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_AME_GPIO, tmp & ~BIT(5)); // Disable GPIO5 pinmuxing.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_GPIO5, MAX77620_CNFG_GPIO_DRV_PUSHPULL | MAX77620_CNFG_GPIO_OUTPUT_VAL_HIGH);

	// Enable cores power.
	// 1-3.x: MAX77621_NFSR_ENABLE.
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_CONTROL1_REG,
		MAX77621_AD_ENABLE | MAX77621_NFSR_ENABLE | MAX77621_SNS_ENABLE | MAX77621_RAMP_12mV_PER_US);
	// 1.0.0-3.x: MAX77621_T_JUNCTION_120 | MAX77621_CKKADV_TRIP_DISABLE | MAX77621_INDUCTOR_NOMINAL.
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_CONTROL2_REG,
		MAX77621_T_JUNCTION_120 | MAX77621_WDTMR_ENABLE | MAX77621_CKKADV_TRIP_75mV_PER_US| MAX77621_INDUCTOR_NOMINAL);
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_VOUT_REG, MAX77621_VOUT_ENABLE | MAX77621_VOUT_0_95V);
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_VOUT_DVS_REG, MAX77621_VOUT_ENABLE | MAX77621_VOUT_0_95V);
}

void _ccplex_enable_power_t210b01()
{
	u8 pmic_cpu_addr = !(FUSE(FUSE_RESERVED_ODM28) & 1) ? MAX77812_PHASE31_CPU_I2C_ADDR : MAX77812_PHASE211_CPU_I2C_ADDR;
	u8 tmp = i2c_recv_byte(I2C_5, pmic_cpu_addr, MAX77812_REG_EN_CTRL);
	i2c_send_byte(I2C_5, pmic_cpu_addr, MAX77812_REG_EN_CTRL, tmp | MAX77812_EN_CTRL_EN_M4);
	i2c_send_byte(I2C_5, pmic_cpu_addr, MAX77812_REG_M4_VOUT, MAX77812_M4_VOUT_0_80V);
}

void ccplex_boot_cpu0(u32 entry)
{
	// Set ACTIVE_CLUSER to FAST.
	FLOW_CTLR(FLOW_CTLR_BPMP_CLUSTER_CONTROL) &= 0xFFFFFFFE;

	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210)
		_ccplex_enable_power_t210();
	else
		_ccplex_enable_power_t210b01();

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
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) = (CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) & ~BIT(CLK_V_MSELECT)) | BIT(CLK_V_MSELECT);

	// Configure initial CPU clock frequency and enable clock.
	CLOCK(CLK_RST_CONTROLLER_CCLK_BURST_POLICY) = 0x20008888;
	CLOCK(CLK_RST_CONTROLLER_SUPER_CCLK_DIVIDER) = 0x80000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET) = BIT(CLK_V_CPUG);

	clock_enable_coresight();

	// CAR2PMC_CPU_ACK_WIDTH should be set to 0.
	CLOCK(CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2) &= 0xFFFFF000;

	// Enable CPU rail.
	pmc_enable_partition(0, 1);
	// Enable cluster 0 non-CPU rail.
	pmc_enable_partition(15, 1);
	// Enable CE0 rail.
	pmc_enable_partition(14, 1);

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
	CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_V) &= ~BIT(CLK_V_MSELECT);
	// Clear NONCPU reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = 0x20000000;
	// Clear CPU0 reset.
	// < 5.x: 0x411F000F, Clear CPU{0,1,2,3} POR and CORE, CX0, L2, and DBG reset.
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = 0x41010001;
}
