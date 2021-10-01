/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2021 CTCaer
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
#include <soc/t210.h>
#include <mem/mc.h>
#include <mem/minerva.h>
#include <mem/sdram.h>
#include <power/bq24193.h>
#include <power/max77620.h>
#include <power/max7762x.h>
#include <power/regulator_5v.h>
#include <storage/nx_sd.h>
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
	SYSCTR0(SYSCTR0_CNTFID0) = 19200000;             // Set counter frequency.
	TMR(TIMERUS_USEC_CFG) = 0x45F;                   // For 19.2MHz clk_m.
	CLOCK(CLK_RST_CONTROLLER_OSC_CTRL) = 0x50000071; // Set OSC to 38.4MHz and drive strength.

	PMC(APBDEV_PMC_OSC_EDPD_OVER) = (PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFFFFF81) | 0xE; // Set LP0 OSC drive strength.
	PMC(APBDEV_PMC_OSC_EDPD_OVER) = (PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFBFFFFF) | PMC_OSC_EDPD_OVER_OSC_CTRL_OVER;
	PMC(APBDEV_PMC_CNTRL2) = (PMC(APBDEV_PMC_CNTRL2) & 0xFFFFEFFF) | PMC_CNTRL2_HOLD_CKE_LOW_EN;
	PMC(APBDEV_PMC_SCRATCH188) = (PMC(APBDEV_PMC_SCRATCH188) & 0xFCFFFFFF) | (4 << 23); // LP0 EMC2TMC_CFG_XM2COMP_PU_VREF_SEL_RANGE.

	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 0x10;   // Set HCLK div to 2 and PCLK div to 1.
	CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE) &= 0xBFFFFFFF; // PLLMB disable.

	PMC(APBDEV_PMC_TSC_MULT) = (PMC(APBDEV_PMC_TSC_MULT) & 0xFFFF0000) | 0x249F; //0x249F = 19200000 * (16 / 32.768 kHz)

	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SYS) = 0;              // Set BPMP/SCLK div to 1.
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20004444;  // Set BPMP/SCLK source to Run and PLLP_OUT2 (204MHz).
	CLOCK(CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER) = 0x80000000; // Enable SUPER_SDIV to 1.
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 2;             // Set HCLK div to 1 and PCLK div to 3.
}

// The uart is skipped for Copper, Hoag and Calcio. Used in Icosa, Iowa and Aula.
static void _config_gpios(bool nx_hoag)
{
	// Clamp inputs when tristated.
	APB_MISC(APB_MISC_PP_PINMUX_GLOBAL) = 0;

	if (!nx_hoag)
	{
		PINMUX_AUX(PINMUX_AUX_UART2_TX) = 0;
		PINMUX_AUX(PINMUX_AUX_UART3_TX) = 0;

		// Set pin mode for UARTB/C TX pins.
#if !defined (DEBUG_UART_PORT) || DEBUG_UART_PORT != UART_B
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_GPIO);
#endif
#if !defined (DEBUG_UART_PORT) || DEBUG_UART_PORT != UART_C
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);
#endif

		// Enable input logic for UARTB/C TX pins.
		gpio_output_enable(GPIO_PORT_G, GPIO_PIN_0, GPIO_OUTPUT_DISABLE);
		gpio_output_enable(GPIO_PORT_D, GPIO_PIN_1, GPIO_OUTPUT_DISABLE);
	}

	// Set Joy-Con IsAttached direction.
	PINMUX_AUX(PINMUX_AUX_GPIO_PE6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_GPIO_PH6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;

	// Set Joy-Con IsAttached mode.
	gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_GPIO);

	// Enable input logic for Joy-Con IsAttached pins.
	gpio_output_enable(GPIO_PORT_E, GPIO_PIN_6, GPIO_OUTPUT_DISABLE);
	gpio_output_enable(GPIO_PORT_H, GPIO_PIN_6, GPIO_OUTPUT_DISABLE);

	pinmux_config_i2c(I2C_1);
	pinmux_config_i2c(I2C_5);
	pinmux_config_uart(UART_A);

	// Configure volume up/down as inputs.
	gpio_config(GPIO_PORT_X, GPIO_PIN_6, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_X, GPIO_PIN_7, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_X, GPIO_PIN_6, GPIO_OUTPUT_DISABLE);
	gpio_output_enable(GPIO_PORT_X, GPIO_PIN_7, GPIO_OUTPUT_DISABLE);

	// Configure HOME as inputs.
	// PINMUX_AUX(PINMUX_AUX_BUTTON_HOME) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;
	// gpio_config(GPIO_PORT_Y, GPIO_PIN_1, GPIO_MODE_GPIO);
}

static void _config_pmc_scratch()
{
	PMC(APBDEV_PMC_SCRATCH20)  &= 0xFFF3FFFF; // Unset Debug console from Customer Option.
	PMC(APBDEV_PMC_SCRATCH190) &= 0xFFFFFFFE; // Unset DATA_DQ_E_IVREF EMC_PMACRO_DATA_PAD_TX_CTRL
	PMC(APBDEV_PMC_SECURE_SCRATCH21) |= PMC_FUSE_PRIVATEKEYDISABLE_TZ_STICKY_BIT;
}

static void _mbist_workaround()
{
	// Make sure Audio clocks are enabled before accessing I2S.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) |= BIT(CLK_V_AHUB);
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) |= BIT(CLK_Y_APE);

	// Set mux output to SOR1 clock switch.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) | 0x8000) & 0xFFFFBFFF;
	// Enabled PLLD and set csi to PLLD for test pattern generation.
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) |= 0x40800000;

	// Clear per-clock resets for APE/VIC/HOST1X/DISP1.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_CLR) = BIT(CLK_Y_APE);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_X_CLR) = BIT(CLK_X_VIC);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = BIT(CLK_L_HOST1X) | BIT(CLK_L_DISP1);
	usleep(2);

	// I2S channels to master and disable SLCG.
	I2S(I2S1_CTRL) |= I2S_CTRL_MASTER_EN;
	I2S(I2S1_CG)   &= ~I2S_CG_SLCG_ENABLE;
	I2S(I2S2_CTRL) |= I2S_CTRL_MASTER_EN;
	I2S(I2S2_CG)   &= ~I2S_CG_SLCG_ENABLE;
	I2S(I2S3_CTRL) |= I2S_CTRL_MASTER_EN;
	I2S(I2S3_CG)   &= ~I2S_CG_SLCG_ENABLE;
	I2S(I2S4_CTRL) |= I2S_CTRL_MASTER_EN;
	I2S(I2S4_CG)   &= ~I2S_CG_SLCG_ENABLE;
	I2S(I2S5_CTRL) |= I2S_CTRL_MASTER_EN;
	I2S(I2S5_CG)   &= ~I2S_CG_SLCG_ENABLE;

	DISPLAY_A(_DIREG(DC_COM_DSC_TOP_CTL)) |= 4; // DSC_SLCG_OVERRIDE.
	VIC(0x8C) = 0xFFFFFFFF;
	usleep(2);

	// Set per-clock reset for APE/VIC/HOST1X/DISP1.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_SET) = BIT(CLK_Y_APE);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_HOST1X) | BIT(CLK_L_DISP1);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_X_SET) = BIT(CLK_X_VIC);

	// Enable specific clocks and disable all others.
	// CLK L Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_H) =
		BIT(CLK_H_PMC) |
		BIT(CLK_H_FUSE);
	// CLK H Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) =
		BIT(CLK_L_RTC)  |
		BIT(CLK_L_TMR)  |
		BIT(CLK_L_GPIO) |
		BIT(CLK_L_BPMP_CACHE_CTRL);
	// CLK U Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_U) =
		BIT(CLK_U_CSITE) |
		BIT(CLK_U_IRAMA) |
		BIT(CLK_U_IRAMB) |
		BIT(CLK_U_IRAMC) |
		BIT(CLK_U_IRAMD) |
		BIT(CLK_U_BPMP_CACHE_RAM);
	// CLK V Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) =
		BIT(CLK_V_MSELECT)       |
		BIT(CLK_V_APB2APE)       |
		BIT(CLK_V_SPDIF_DOUBLER) |
		BIT(CLK_V_SE);
	// CLK W Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_W) =
		BIT(CLK_W_PCIERX0) |
		BIT(CLK_W_PCIERX1) |
		BIT(CLK_W_PCIERX2) |
		BIT(CLK_W_PCIERX3) |
		BIT(CLK_W_PCIERX4) |
		BIT(CLK_W_PCIERX5) |
		BIT(CLK_W_ENTROPY) |
		BIT(CLK_W_MC1);
	// CLK X Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X) =
		BIT(CLK_X_MC_CAPA)  |
		BIT(CLK_X_MC_CBPA)  |
		BIT(CLK_X_MC_CPU)   |
		BIT(CLK_X_MC_BBC)   |
		BIT(CLK_X_GPU)      |
		BIT(CLK_X_DBGAPB)   |
		BIT(CLK_X_PLLG_REF);
	// CLK Y Devices.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) =
		BIT(CLK_Y_MC_CDPA) |
		BIT(CLK_Y_MC_CCPA);

	// Disable clock gate overrides.
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE) = 0;

	// Set child clock sources.
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE)        &= 0x1F7FFFFF; // Disable PLLD and set reference clock and csi clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1)  &= 0xFFFF3FFF; // Set SOR1 to automatic muxing of safe clock (24MHz) or SOR1 clk switch.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_VI)     = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_VI) & 0x1FFFFFFF) | 0x80000000;     // Set clock source to PLLP_OUT.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X) & 0x1FFFFFFF) | 0x80000000; // Set clock source to PLLP_OUT.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_NVENC)  = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_NVENC) & 0x1FFFFFFF) | 0x80000000;  // Set clock source to PLLP_OUT.
}

static void _config_se_brom()
{
	// Enable Fuse visibility.
	clock_enable_fuse(true);

	// Try to set SBK from fuses. If patched, skip.
	fuse_set_sbk();

	// Lock SSK (although it's not set and unused anyways).
	// se_key_acc_ctrl(15, SE_KEY_TBL_DIS_KEYREAD_FLAG);

	// This memset needs to happen here, else TZRAM will behave weirdly later on.
	memset((void *)TZRAM_BASE, 0, SZ_64K);
	PMC(APBDEV_PMC_CRYPTO_OP) = PMC_CRYPTO_OP_SE_ENABLE;
	SE(SE_INT_STATUS_REG) = 0x1F; // Clear all SE interrupts.

	// Save reset reason.
	hw_rst_status = PMC(APBDEV_PMC_SCRATCH200);
	hw_rst_reason = PMC(APBDEV_PMC_RST_STATUS) & PMC_RST_STATUS_MASK;

	// Clear the boot reason to avoid problems later.
	PMC(APBDEV_PMC_SCRATCH200) = 0x0;
	PMC(APBDEV_PMC_RST_STATUS) = 0x0;
	APB_MISC(APB_MISC_PP_STRAPPING_OPT_A) = (APB_MISC(APB_MISC_PP_STRAPPING_OPT_A) & 0xF0) | (7 << 10);
}

static void _config_regulators(bool tegra_t210)
{
	// Set RTC/AO domain to POR voltage.
	if (tegra_t210)
		max7762x_regulator_set_voltage(REGULATOR_LDO4, 1000000);

	// Disable low battery shutdown monitor.
	max77620_low_battery_monitor_config(false);

	// Disable SDMMC1 IO power.
	gpio_write(GPIO_PORT_E, GPIO_PIN_4, GPIO_LOW);
	max7762x_regulator_enable(REGULATOR_LDO2, false);
	sd_power_cycle_time_start = get_tmr_ms();

	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CNFGBBC, MAX77620_CNFGBBC_RESISTOR_1K);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1,
		MAX77620_ONOFFCNFG1_RSVD | (3 << MAX77620_ONOFFCNFG1_MRT_SHIFT)); // PWR delay for forced shutdown off.

	if (tegra_t210)
	{
		// Configure all Flexible Power Sequencers for MAX77620.
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG0, (7 << MAX77620_FPS_TIME_PERIOD_SHIFT));
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG1, (7 << MAX77620_FPS_TIME_PERIOD_SHIFT) | (1 << MAX77620_FPS_EN_SRC_SHIFT));
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG2, (7 << MAX77620_FPS_TIME_PERIOD_SHIFT));
		max77620_regulator_config_fps(REGULATOR_LDO4);
		max77620_regulator_config_fps(REGULATOR_LDO8);
		max77620_regulator_config_fps(REGULATOR_SD0);
		max77620_regulator_config_fps(REGULATOR_SD1);
		max77620_regulator_config_fps(REGULATOR_SD3);

		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_GPIO3,
			(4 << MAX77620_FPS_TIME_PERIOD_SHIFT) | (2 << MAX77620_FPS_PD_PERIOD_SHIFT)); // 3.x+

		// Set vdd_core voltage to 1.125V.
		max7762x_regulator_set_voltage(REGULATOR_SD0, 1125000);

		// Fix CPU/GPU after L4T warmboot.
		max77620_config_gpio(5, MAX77620_GPIO_OUTPUT_DISABLE);
		max77620_config_gpio(6, MAX77620_GPIO_OUTPUT_DISABLE);

		// Set POR configuration.
		max77621_config_default(REGULATOR_CPU0, MAX77621_CTRL_POR_CFG);
		max77621_config_default(REGULATOR_GPU0, MAX77621_CTRL_POR_CFG);
	}
	else // Tegra X1+ set vdd_core voltage to 1.05V.
		max7762x_regulator_set_voltage(REGULATOR_SD0, 1050000);
}

void hw_init()
{
	// Get Chip ID.
	bool tegra_t210 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;
	bool nx_hoag = fuse_read_hw_type() == FUSE_NX_HW_TYPE_HOAG;

	// Bootrom stuff we skipped by going through rcm.
	_config_se_brom();
	//FUSE(FUSE_PRIVATEKEYDISABLE) = 0x11;
	SYSREG(AHB_AHB_SPARE_REG) &= 0xFFFFFF9F; // Unset APB2JTAG_OVERRIDE_EN and OBS_OVERRIDE_EN.
	PMC(APBDEV_PMC_SCRATCH49) = PMC(APBDEV_PMC_SCRATCH49) & 0xFFFFFFFC;

	// Perform Memory Built-In Self Test WAR if T210.
	if (tegra_t210)
		_mbist_workaround();

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

#ifdef DEBUG_UART_PORT
	clock_enable_uart(DEBUG_UART_PORT);
	uart_init(DEBUG_UART_PORT, DEBUG_UART_BAUDRATE);
	uart_invert(DEBUG_UART_PORT, DEBUG_UART_INVERT, UART_INVERT_TXD);
#endif

	// Enable Dynamic Voltage and Frequency Scaling device clock.
	clock_enable_cl_dvfs();

	// Enable clocks to I2C1 and I2CPWR.
	clock_enable_i2c(I2C_1);
	clock_enable_i2c(I2C_5);

	// Enable clock to TZRAM.
	clock_enable_tzram();

	// Initialize I2C5, mandatory for PMIC.
	i2c_init(I2C_5);

	//! TODO: Why? Device is NFC MCU on Lite.
	if (nx_hoag)
	{
		max7762x_regulator_set_voltage(REGULATOR_LDO8, 2800000);
		max7762x_regulator_enable(REGULATOR_LDO8, true);
	}

	// Initialize I2C1 for various power related devices.
	i2c_init(I2C_1);

	// Initialize various regulators based on Erista/Mariko platform.
	_config_regulators(tegra_t210);

	// Enable charger in case it's disabled.
	bq24193_enable_charger();

	_config_pmc_scratch(); // Missing from 4.x+

	// Set BPMP/SCLK to PLLP_OUT (408MHz).
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003333;

	// Disable TZRAM shutdown control and lock the regs.
	if (!tegra_t210)
	{
		PMC(APBDEV_PMC_TZRAM_PWR_CNTRL) &= 0xFFFFFFFE;
		PMC(APBDEV_PMC_TZRAM_NON_SEC_DISABLE) = 3;
		PMC(APBDEV_PMC_TZRAM_SEC_DISABLE) = 3;
	}

	// Initialize External memory controller and configure DRAM parameters.
	sdram_init();

	bpmp_mmu_enable();
}

void hw_reinit_workaround(bool coreboot, u32 bl_magic)
{
	// Disable BPMP max clock.
	bpmp_clk_rate_set(BPMP_CLK_NORMAL);

#ifdef NYX
	// Disable temperature sensor, touchscreen, 5V regulators and Joy-Con.
	tmp451_end();
	set_fan_duty(0);
	touch_power_off();
	jc_deinit();
	regulator_5v_disable(REGULATOR_5V_ALL);
#endif

	// Flush/disable MMU cache and set DRAM clock to 204MHz.
	bpmp_mmu_disable();
	minerva_change_freq(FREQ_204);
	nyx_str->mtc_cfg.init_done = 0;

	// Re-enable clocks to Audio Processing Engine as a workaround to hanging.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) |= BIT(CLK_V_AHUB);
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) |= BIT(CLK_Y_APE);

	// Do coreboot mitigations.
	if (coreboot)
	{
		msleep(10);

		clock_disable_cl_dvfs();

		// Disable Joy-con GPIOs.
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_SPIO);

		// Reinstate SD controller power.
		PMC(APBDEV_PMC_NO_IOPOWER) &= ~(PMC_NO_IOPOWER_SDMMC1_IO_EN);
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
	default:
		display_end();
	}

	// Enable clock to USBD and init SDMMC1 to avoid hangs with bad hw inits.
	if (bl_magic == BL_MAGIC_BROKEN_HWI)
	{
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = BIT(CLK_L_USBD);
		sdmmc_init(&sd_sdmmc, SDMMC_1, SDMMC_POWER_3_3, SDMMC_BUS_WIDTH_1, SDHCI_TIMING_SD_ID, 0);
		clock_disable_cl_dvfs();

		msleep(200);
	}
}
