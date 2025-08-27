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

#include <string.h>

#include <soc/hw_init.h>
#include <display/di.h>
#include <display/vic.h>
#include <input/joycon.h>
#include <input/touch.h>
#include <sec/se.h>
#include <sec/se_t210.h>
#include <soc/bpmp.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/gpio.h>
#include <soc/i2c.h>
#include <soc/pinmux.h>
#include <soc/pmc.h>
#include <soc/uart.h>
#include <soc/timer.h>
#include <soc/t210.h>
#include <mem/mc.h>
#include <mem/minerva.h>
#include <mem/sdram.h>
#include <power/bq24193.h>
#include <power/max77620.h>
#include <power/max7762x.h>
#include <power/regulator_5v.h>
#include <storage/sd.h>
#include <storage/sdmmc.h>
#include <thermal/fan.h>
#include <thermal/tmp451.h>
#include <utils/util.h>

extern boot_cfg_t b_cfg;
extern volatile nyx_storage_t *nyx_str;

u32 hw_rst_status;
u32 hw_rst_reason;

u32 hw_get_chip_id()
{
	if (((APB_MISC(APB_MISC_GP_HIDREV) >> 4) & 0xF) >= GP_HIDREV_MAJOR_T210B01)
		return GP_HIDREV_MAJOR_T210B01;
	else
		return GP_HIDREV_MAJOR_T210;
}

/*
 * CLK_OSC - 38.4 MHz crystal.
 * CLK_M   - 19.2 MHz (osc/2).
 * CLK_S   - 32.768 KHz (from PMIC).
 * SCLK    - 204MHz init (-> 408MHz -> OC).
 * HCLK    - 204MHz init (-> 408MHz -> OC).
 * PCLK    - 68MHz  init (-> 136MHz -> OC/4).
 */

static void _config_oscillators()
{
	CLOCK(CLK_RST_CONTROLLER_SPARE_REG0) = (CLOCK(CLK_RST_CONTROLLER_SPARE_REG0) & 0xFFFFFFF3) | 4; // Set CLK_M_DIVISOR to 2.
	SYSCTR0(SYSCTR0_CNTFID0)             = 19200000;   // Set counter frequency.
	TMR(TIMERUS_USEC_CFG)                = 0x45F;      // For 19.2MHz clk_m.
	CLOCK(CLK_RST_CONTROLLER_OSC_CTRL)   = 0x50000071; // Set OSC to 38.4MHz and drive strength.

	PMC(APBDEV_PMC_OSC_EDPD_OVER)  = (PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFFFFF81) | 0xE; // Set LP0 OSC drive strength.
	PMC(APBDEV_PMC_OSC_EDPD_OVER)  = (PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFBFFFFF) | PMC_OSC_EDPD_OVER_OSC_CTRL_OVER;
	PMC(APBDEV_PMC_CNTRL2)         = (PMC(APBDEV_PMC_CNTRL2) & 0xFFFFEFFF) | PMC_CNTRL2_HOLD_CKE_LOW_EN;
	APB_MISC(APB_MISC_GP_ASDBGREG) = (APB_MISC(APB_MISC_GP_ASDBGREG) & 0xFCFFFFFF) | (2 << 24); // CFG2TMC_RAM_SVOP_PDP.

	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 0x10;       // Set HCLK div to 2 and PCLK div to 1.
	CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE)     &= 0xBFFFFFFF; // PLLMB disable.

	PMC(APBDEV_PMC_TSC_MULT) = (PMC(APBDEV_PMC_TSC_MULT) & 0xFFFF0000) | 0x249F; // 0x249F = 19200000 * (16 / 32.768 kHz).

	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SYS)     = 0;          // Set BPMP/SCLK div to 1.
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY)  = 0x20004444; // Set BPMP/SCLK source to Run and PLLP_OUT2 (204MHz).
	CLOCK(CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER) = 0x80000000; // Enable SUPER_SDIV to 1.
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE)    = 2;          // Set HCLK div to 1 and PCLK div to 3.
}

void hw_config_arbiter(bool reset)
{
	if (reset)
	{
		ARB_PRI(ARB_PRIO_CPU_PRIORITY) = 0x0040090;
		ARB_PRI(ARB_PRIO_COP_PRIORITY) = 0x12024C2;
		ARB_PRI(ARB_PRIO_VCP_PRIORITY) = 0x2201209;
		ARB_PRI(ARB_PRIO_DMA_PRIORITY) = 0x320365B;
	}
	else
	{
		ARB_PRI(ARB_PRIO_CPU_PRIORITY) = 0x12412D1;
		ARB_PRI(ARB_PRIO_COP_PRIORITY) = 0x0000000;
		ARB_PRI(ARB_PRIO_VCP_PRIORITY) = 0x220244A;
		ARB_PRI(ARB_PRIO_DMA_PRIORITY) = 0x320369B;
	}
}

// The uart is skipped for Copper, Hoag and Calcio. Used in Icosa, Iowa and Aula.
static void _config_gpios(bool nx_hoag)
{
	// Clamp inputs when tristated.
	APB_MISC(APB_MISC_PP_PINMUX_GLOBAL) = 0;

	if (!nx_hoag)
	{
		// Turn Joy-Con detect on. (GPIO mode and input logic for UARTB/C TX pins.)
		PINMUX_AUX(PINMUX_AUX_UART2_TX) = 0;
		PINMUX_AUX(PINMUX_AUX_UART3_TX) = 0;
		gpio_direction_input(GPIO_PORT_G, GPIO_PIN_0);
		gpio_direction_input(GPIO_PORT_D, GPIO_PIN_1);
	}

	// Set Joy-Con IsAttached pinmux. Shared with UARTB/UARTC TX.
	PINMUX_AUX(PINMUX_AUX_GPIO_PE6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_GPIO_PH6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;

	// Configure Joy-Con IsAttached pins. Shared with UARTB/UARTC TX.
	gpio_direction_input(GPIO_PORT_E, GPIO_PIN_6);
	gpio_direction_input(GPIO_PORT_H, GPIO_PIN_6);

	pinmux_config_i2c(I2C_1);
	pinmux_config_i2c(I2C_5);
	pinmux_config_uart(UART_A);

	// Configure volume up/down as inputs.
	gpio_direction_input(GPIO_PORT_X, GPIO_PIN_6 | GPIO_PIN_7);

	// Configure HOME as input. (Shared with UARTB RTS).
	PINMUX_AUX(PINMUX_AUX_BUTTON_HOME) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;
	gpio_direction_input(GPIO_PORT_Y, GPIO_PIN_1);

	// Power button can be configured for hoag here. Only SKU where it's connected.
}

static void _config_pmc_scratch()
{
	PMC(APBDEV_PMC_SCRATCH20)  &= 0xFFF3FFFF; // Unset Debug console from Customer Option.
	PMC(APBDEV_PMC_SCRATCH190) &= 0xFFFFFFFE; // Unset WDT_DURING_BR.
	PMC(APBDEV_PMC_SECURE_SCRATCH21) |= PMC_FUSE_PRIVATEKEYDISABLE_TZ_STICKY_BIT;
}

static void _mbist_workaround_bl()
{
	/*
	 * DFT MBIST HW Errata for T210.
	 * RAM Data corruption observed when MBIST_EN from DFT (Tegra X1 Si Errata)
	 *
	 * The MBIST_EN signals from the DFT logic can impact the functional logic of
	 * internal rams after power-on since they are driven by non-resetable flops.
	 * That can be worked around by enabling and then disabling the related clocks.
	 *
	 * The bootrom patches, already handle the LVL2 SLCG war by enabling all clocks
	 * and all LVL2 CLK overrides (power saving disable).
	 * The Bootloader then handles the IP block SLCG part and also restores the
	 * state for the clocks/lvl2 slcg.
	 * After these, per domain MBIST WAR is needed every time a domain gets
	 * unpowergated if it was previously powergated.
	 *
	 * Affected power domains: all except IRAM and CCPLEX.
	 */

	// Set mux output to SOR1 clock switch (for VI).
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) & ~BIT(14)) | BIT(15);
	// Enable PLLD and set csi to PLLD for test pattern generation (for VI).
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) |= PLL_BASE_ENABLE | BIT(23);

	// Make sure Audio clocks are enabled before accessing I2S.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET) = BIT(CLK_V_AHUB);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_Y_SET) = BIT(CLK_Y_APE);

	// Clear per-clock resets for APE/VIC/HOST1X/DISP1.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_CLR) = BIT(CLK_Y_APE);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_X_CLR) = BIT(CLK_X_VIC);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = BIT(CLK_L_HOST1X) | BIT(CLK_L_DISP1);
	usleep(2);

	// Set I2S to master to enable clocks and set SLCG overrides.
	for (u32 i2s_idx = 0; i2s_idx < 5; i2s_idx++)
	{
		I2S(I2S_CTRL + (i2s_idx << 8u)) |= I2S_CTRL_MASTER_EN;
		I2S(I2S_CG   + (i2s_idx << 8u))  = I2S_CG_SLCG_DISABLE;
	}
	// Set SLCG overrides for DISPA and VIC.
	DISPLAY_A(_DIREG(DC_COM_DSC_TOP_CTL)) |= BIT(2); // DSC_SLCG_OVERRIDE.
	VIC(VIC_THI_SLCG_OVERRIDE_LOW_A) = 0xFFFFFFFF;

	// Wait a bit for MBIST_EN to get unstuck (1 cycle min).
	usleep(2);

	// Reset SLCG to automatic mode.
	// for (u32 i2s_idx = 0; i2s_idx < 5; i2s_idx++)
	// 	I2S(I2S_CG   + (i2s_idx << 8u)) = I2S_CG_SLCG_ENABLE;
	// DISPLAY_A(_DIREG(DC_COM_DSC_TOP_CTL)) &= ~BIT(2); // DSC_SLCG_OVERRIDE.
	// VIC(VIC_THI_SLCG_OVERRIDE_LOW_A) = 0;

	// Set per-clock reset for APE/VIC/HOST1X/DISP1.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_SET) = BIT(CLK_Y_APE);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_HOST1X) | BIT(CLK_L_DISP1);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_X_SET) = BIT(CLK_X_VIC);

	// Disable all unneeded clocks that were enabled in bootrom.
	// CLK L Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_H) = BIT(CLK_H_PMC) |
											  BIT(CLK_H_FUSE);
	// CLK H Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) = BIT(CLK_L_RTC)  |
											  BIT(CLK_L_TMR)  |
											  BIT(CLK_L_GPIO) |
											  BIT(CLK_L_BPMP_CACHE_CTRL);
	// CLK U Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_U) = BIT(CLK_U_CSITE) |
											  BIT(CLK_U_IRAMA) |
											  BIT(CLK_U_IRAMB) |
											  BIT(CLK_U_IRAMC) |
											  BIT(CLK_U_IRAMD) |
											  BIT(CLK_U_BPMP_CACHE_RAM);
	// CLK V Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) = BIT(CLK_V_MSELECT)       |
											  BIT(CLK_V_APB2APE)       |
											  BIT(CLK_V_SPDIF_DOUBLER) |
											  BIT(CLK_V_SE);
	// CLK W Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_W) = BIT(CLK_W_PCIERX0) |
											  BIT(CLK_W_PCIERX1) |
											  BIT(CLK_W_PCIERX2) |
											  BIT(CLK_W_PCIERX3) |
											  BIT(CLK_W_PCIERX4) |
											  BIT(CLK_W_PCIERX5) |
											  BIT(CLK_W_ENTROPY) |
											  BIT(CLK_W_MC1);
	// CLK X Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X) = BIT(CLK_X_MC_CAPA)  |
											  BIT(CLK_X_MC_CBPA)  |
											  BIT(CLK_X_MC_CPU)   |
											  BIT(CLK_X_MC_BBC)   |
											  BIT(CLK_X_GPU)      |
											  BIT(CLK_X_DBGAPB)   |
											  BIT(CLK_X_PLLG_REF);
	// CLK Y Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) = BIT(CLK_Y_MC_CDPA) |
											  BIT(CLK_Y_MC_CCPA);

	// Reset clock gate overrides to automatic mode.
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE) = 0;

	// Set child clock sources.
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE)        &= 0x1F7FFFFF; // Disable PLLD and set reference clock and csi clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1)  &= ~(BIT(15) | BIT(14)); // Set SOR1 to automatic muxing of safe clock (24MHz) or SOR1 clk switch.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_VI)     = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_VI)     & 0x1FFFFFFF) | (4 << 29u); // Set clock source to PLLP_OUT.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X) & 0x1FFFFFFF) | (4 << 29u); // Set clock source to PLLP_OUT.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_NVENC)  = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_NVENC)  & 0x1FFFFFFF) | (4 << 29u); // Set clock source to PLLP_OUT.
}

static void _config_se_brom()
{
	// Enable Fuse visibility.
	clock_enable_fuse(true);

	// Try to set SBK from fuses. If patched, skip.
	fuse_set_sbk();

	// Make SBK unreadable.
	//FUSE(FUSE_PRIVATEKEYDISABLE) = FUSE_PRIVKEY_TZ_STICKY_BIT | FUSE_PRIVKEY_DISABLE;

	// Lock SSK (although it's not set and unused anyways).
	// se_key_acc_ctrl(15, SE_KEY_TBL_DIS_KEYREAD_FLAG);

	// This memset needs to happen here, else TZRAM will behave weirdly later on.
	memset((void *)TZRAM_BASE, 0, TZRAM_SIZE);
	PMC(APBDEV_PMC_CRYPTO_OP) = PMC_CRYPTO_OP_SE_ENABLE;

	// Clear SE interrupts.
	SE(SE_INT_STATUS_REG) = SE_INT_OP_DONE | SE_INT_OUT_DONE | SE_INT_OUT_LL_BUF_WR | SE_INT_IN_DONE | SE_INT_IN_LL_BUF_RD;

	// Save reset reason.
	hw_rst_status = PMC(APBDEV_PMC_SCRATCH200);
	hw_rst_reason = PMC(APBDEV_PMC_RST_STATUS) & PMC_RST_STATUS_MASK;

	// Clear the boot reason to avoid problems later.
	PMC(APBDEV_PMC_SCRATCH200) = 0;
	PMC(APBDEV_PMC_RST_STATUS) = PMC_RST_STATUS_POR;
	APB_MISC(APB_MISC_PP_STRAPPING_OPT_A) = (APB_MISC(APB_MISC_PP_STRAPPING_OPT_A) & 0xF0) | (7 << 10);
}

static void _config_regulators(bool tegra_t210, bool nx_hoag)
{
	// Set RTC/AO domain to POR voltage.
	if (tegra_t210)
		max7762x_regulator_set_voltage(REGULATOR_LDO4, 1000000);

	// Disable low battery shutdown monitor.
	max77620_low_battery_monitor_config(false);

	// Power on all relevant rails in case we came out of warmboot. Only keep MEM/MEM_COMP and SDMMC1 states.
	PMC(APBDEV_PMC_NO_IOPOWER) &= PMC_NO_IOPOWER_MEM_COMP | PMC_NO_IOPOWER_SDMMC1 | PMC_NO_IOPOWER_MEM;

	// Make sure SDMMC1 IO/Core are powered off.
	max7762x_regulator_enable(REGULATOR_LDO2, false);
	gpio_write(GPIO_PORT_E, GPIO_PIN_4, GPIO_LOW);
	PMC(APBDEV_PMC_NO_IOPOWER) |= PMC_NO_IOPOWER_SDMMC1;
	(void)PMC(APBDEV_PMC_NO_IOPOWER);
	sd_power_cycle_time_start = get_tmr_ms();

	// Disable backup battery charger.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CNFGBBC, MAX77620_CNFGBBC_RESISTOR_1K);

	// Set PWR delay for forced shutdown off to 6s.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_RSVD | (3 << MAX77620_ONOFFCNFG1_MRT_SHIFT));

	if (tegra_t210)
	{
		// Configure all Flexible Power Sequencers for MAX77620.
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG0, (7 << MAX77620_FPS_TIME_PERIOD_SHIFT) | (0 << MAX77620_FPS_EN_SRC_SHIFT));
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG1, (7 << MAX77620_FPS_TIME_PERIOD_SHIFT) | (1 << MAX77620_FPS_EN_SRC_SHIFT));
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG2, (7 << MAX77620_FPS_TIME_PERIOD_SHIFT));
		max77620_regulator_config_fps(REGULATOR_LDO4);
		max77620_regulator_config_fps(REGULATOR_LDO8);
		max77620_regulator_config_fps(REGULATOR_SD0);
		max77620_regulator_config_fps(REGULATOR_SD1);
		max77620_regulator_config_fps(REGULATOR_SD3);

		// Set GPIO3 to FPS0 for SYS 3V3 EN. Enabled when FPS0 is enabled.
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_GPIO3, (4 << MAX77620_FPS_PU_PERIOD_SHIFT) | (2 << MAX77620_FPS_PD_PERIOD_SHIFT));

		// Set vdd_core voltage to 1.125V.
		max7762x_regulator_set_voltage(REGULATOR_SD0, 1125000);

		// Power down CPU/GPU regulators after L4T warmboot.
		max77620_config_gpio(5, MAX77620_GPIO_OUTPUT_DISABLE);
		max77620_config_gpio(6, MAX77620_GPIO_OUTPUT_DISABLE);

		// Set POR configuration.
		max77621_config_default(REGULATOR_CPU0, MAX77621_CTRL_POR_CFG);
		max77621_config_default(REGULATOR_GPU0, MAX77621_CTRL_POR_CFG);
	}
	else
	{
		// Tegra X1+ set vdd_core voltage to 1.05V.
		max7762x_regulator_set_voltage(REGULATOR_SD0, 1050000);

		// Power on SD2 regulator for supplying LDO0/1/8.
		max7762x_regulator_set_voltage(REGULATOR_SD2, 1325000);

		// Set slew rate and enable SD2 regulator.
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_SD2_CFG, (1 << MAX77620_SD_SR_SHIFT)                                  |
																	  (MAX77620_POWER_MODE_NORMAL << MAX77620_SD_POWER_MODE_SHIFT) |
																	  MAX77620_SD_CFG1_FSRADE_SD_ENABLE);

		// Enable LDO8 on HOAG as it also powers I2C1 IO pads.
		if (nx_hoag)
		{
			max7762x_regulator_set_voltage(REGULATOR_LDO8, 2800000);
			max7762x_regulator_enable(REGULATOR_LDO8, true);
		}
	}
}

void hw_init()
{
	// Get Chip ID and SKU.
	bool tegra_t210 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;
	bool nx_hoag = fuse_read_hw_type() == FUSE_NX_HW_TYPE_HOAG;

	// Bootrom stuff we might skipped by going through rcm.
	_config_se_brom();

	// Unset APB2JTAG_OVERRIDE_EN and OBS_OVERRIDE_EN.
	SYSREG(AHB_AHB_SPARE_REG) &= 0xFFFFFF9F;
	PMC(APBDEV_PMC_SCRATCH49) &= 0xFFFFFFFC;

	// Perform the bootloader part of the Memory Built-In Self Test WAR if T210.
	if (tegra_t210)
		_mbist_workaround_bl();

	// Make sure PLLP_OUT3/4 is set to 408 MHz and enabled.
	CLOCK(CLK_RST_CONTROLLER_PLLP_OUTB) = 0x30003;

	// Enable Security Engine clock.
	clock_enable_se();

	// Enable Fuse visibility.
	clock_enable_fuse(true);

	// Disable Fuse programming.
	fuse_disable_program();

	// Enable clocks to Memory controllers and disable AHB redirect.
	mc_enable();

	// Initialize counters, CLKM, BPMP and other clocks based on 38.4MHz oscillator.
	_config_oscillators();

	// Initialize pin configuration.
	_config_gpios(nx_hoag);

	// Enable CL-DVFS clock unconditionally to avoid issues with I2C5 sharing.
	clock_enable_cl_dvfs();

	// Enable clocks to I2C1 and I2CPWR.
	clock_enable_i2c(I2C_1);
	clock_enable_i2c(I2C_5);

	// Enable clock to TZRAM.
	clock_enable_tzram();

	// Initialize I2C5, mandatory for PMIC.
	i2c_init(I2C_5);

	// Initialize various regulators based on Erista/Mariko platform.
	_config_regulators(tegra_t210, nx_hoag);

	// Initialize I2C1 for various power related devices.
	i2c_init(I2C_1);

	_config_pmc_scratch(); // Missing from 4.x+

	// Set BPMP/SCLK to PLLP_OUT (408MHz).
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003333;

	// Disable T210B01 TZRAM power-gating and lock the reg.
	if (!tegra_t210)
	{
		// This is not actually needed since it's done by bootrom. The read locks are extra.
		PMC(APBDEV_PMC_TZRAM_PWR_CNTRL)      &= ~PMC_TZRAM_PWR_CNTRL_SD;
		PMC(APBDEV_PMC_TZRAM_NON_SEC_DISABLE) = PMC_TZRAM_DISABLE_REG_WRITE | PMC_TZRAM_DISABLE_REG_READ;
		PMC(APBDEV_PMC_TZRAM_SEC_DISABLE)     = PMC_TZRAM_DISABLE_REG_WRITE | PMC_TZRAM_DISABLE_REG_READ;
	}

	// Set arbiter.
	hw_config_arbiter(false);

	// Initialize External memory controller and configure DRAM parameters.
	sdram_init();

	bpmp_mmu_enable();

	// Enable HOST1X used by every display module (DC, VIC, NVDEC, NVENC, TSEC, etc).
	clock_enable_host1x();

#ifdef DEBUG_UART_PORT
	// Setup debug uart port.
	#if   (DEBUG_UART_PORT == UART_B)
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
	#elif (DEBUG_UART_PORT == UART_C)
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);
	#endif
	pinmux_config_uart(DEBUG_UART_PORT);
	clock_enable_uart(DEBUG_UART_PORT);
	uart_init(DEBUG_UART_PORT, DEBUG_UART_BAUDRATE, UART_AO_TX_AO_RX);
	uart_invert(DEBUG_UART_PORT, DEBUG_UART_INVERT, UART_INVERT_TXD);
#endif
}

void hw_deinit(bool coreboot, u32 bl_magic)
{
	bool tegra_t210 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;

	// Scale down BPMP clock.
	bpmp_clk_rate_set(BPMP_CLK_NORMAL);

#ifdef BDK_HW_EXTRA_DEINIT
	// Disable temperature sensor, touchscreen, 5V regulators, Joy-Con and VIC.
	vic_end();
	tmp451_end();
	fan_set_duty(0);
	touch_power_off();
	jc_deinit();
	regulator_5v_disable(REGULATOR_5V_ALL);
#endif

	// set DRAM clock to 204MHz.
	minerva_change_freq(FREQ_204);
	nyx_str->mtc_cfg.init_done = 0;

	// Flush/disable MMU cache.
	bpmp_mmu_disable();

	// Reset arbiter.
	hw_config_arbiter(true);

	// Re-enable clocks to Audio Processing Engine as a workaround to rerunning mbist war.
	if (tegra_t210)
	{
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET) = BIT(CLK_V_AHUB);
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_Y_SET) = BIT(CLK_Y_APE);
	}

	// Do coreboot mitigations.
	if (coreboot)
	{
		msleep(10);

		clock_disable_cl_dvfs();

		// Disable Joy-con detect in order to restore UART TX.
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);

		// Reinstate SD controller power.
		PMC(APBDEV_PMC_NO_IOPOWER) &= ~PMC_NO_IOPOWER_SDMMC1;
	}

	// Seamless display or display power off.
	switch (bl_magic)
	{
	case BL_MAGIC_CRBOOT_SLD:;
		// Set pwm to 0%, switch to gpio mode and restore pwm duty.
		u32 brightness = display_get_backlight_brightness();
		display_backlight_brightness(0, 1000);
		gpio_config(GPIO_PORT_V, GPIO_PIN_0, GPIO_MODE_GPIO);
		display_backlight_brightness(brightness, 0);
		break;
	case BL_MAGIC_L4TLDR_SLD:
		// Do not disable display or backlight at all.
		break;
	default:
		display_end();
		clock_disable_host1x();
	}
}
