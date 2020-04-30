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

#include <string.h>

#include "hw_init.h"
#include "bpmp.h"
#include "clock.h"
#include "fuse.h"
#include "gpio.h"
#include "i2c.h"
#include "pinmux.h"
#include "pmc.h"
#include "t210.h"
#include "uart.h"
#include "../gfx/di.h"
#include "../mem/mc.h"
#include "../mem/minerva.h"
#include "../mem/sdram.h"
#include "../power/max77620.h"
#include "../power/max7762x.h"
#include "../sec/se.h"
#include "../sec/se_t210.h"
#include "../storage/nx_sd.h"
#include "../storage/sdmmc.h"
#include "../utils/util.h"

extern boot_cfg_t b_cfg;
extern volatile nyx_storage_t *nyx_str;

/*
 * CLK_OSC - 38.4 MHz crystal.
 * CLK_M   - 19.2 MHz (osc/2).
 * CLK_S   - 32.768 KHz (from PMIC).
 * SCLK    - 204MHz init (-> 408MHz -> OC).
 * HCLK    - 204MHz init (-> 408MHz -> OC).
 * PCLK    - 68MHz  init (-> 136MHz -> OC/4).
 */

void _config_oscillators()
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

	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SYS) = 0;              // Set SCLK div to 1.
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20004444;  // Set clk source to Run and PLLP_OUT2 (204MHz).
	CLOCK(CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER) = 0x80000000; // Enable SUPER_SDIV to 1.
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 2;             // Set HCLK div to 1 and PCLK div to 3.
}

void _config_gpios()
{
	PINMUX_AUX(PINMUX_AUX_UART2_TX) = 0;
	PINMUX_AUX(PINMUX_AUX_UART3_TX) = 0;

	// Set Joy-Con IsAttached direction.
	PINMUX_AUX(PINMUX_AUX_GPIO_PE6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_GPIO_PH6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;

	// Set pin mode for Joy-Con IsAttached and UARTB/C TX pins.
#if !defined (DEBUG_UART_PORT) || DEBUG_UART_PORT != UART_B
	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_GPIO);
#endif
#if !defined (DEBUG_UART_PORT) || DEBUG_UART_PORT != UART_C
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);
#endif
	// Set Joy-Con IsAttached mode.
	gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_GPIO);

	// Enable input logic for Joy-Con IsAttached and UARTB/C TX pins.
	gpio_output_enable(GPIO_PORT_G, GPIO_PIN_0, GPIO_OUTPUT_DISABLE);
	gpio_output_enable(GPIO_PORT_D, GPIO_PIN_1, GPIO_OUTPUT_DISABLE);
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

void _config_pmc_scratch()
{
	PMC(APBDEV_PMC_SCRATCH20) &= 0xFFF3FFFF;  // Unset Debug console from Customer Option.
	PMC(APBDEV_PMC_SCRATCH190) &= 0xFFFFFFFE; // Unset DATA_DQ_E_IVREF EMC_PMACRO_DATA_PAD_TX_CTRL
	PMC(APBDEV_PMC_SECURE_SCRATCH21) |= PMC_FUSE_PRIVATEKEYDISABLE_TZ_STICKY_BIT;
}

void _mbist_workaround()
{
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) |= (1 << 10); // Enable AHUB clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) |= (1 <<  6); // Enable APE clock.

	// Set mux output to SOR1 clock switch.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) | 0x8000) & 0xFFFFBFFF;
	// Enabled PLLD and set csi to PLLD for test pattern generation.
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) |= 0x40800000;

	// Clear per-clock resets.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_CLR) = 0x40;       // Clear reset APE.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_X_CLR) = 0x40000;    // Clear reset VIC.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = 0x18000000; // Clear reset DISP1, HOST1X.
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

	// Set per-clock reset.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_SET) = 0x40;       // Set reset APE.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = 0x18000000; // Set reset DISP1, HOST1x.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_X_SET) = 0x40000;    // Set reset VIC.

	// Enable specific clocks and disable all others.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_H) = 0xC0;       // Enable clock PMC, FUSE.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) = 0x80000130; // Enable clock RTC, TMR, GPIO, BPMP_CACHE.
	//CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) = 0x80400130; // Keep USBD ON.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_U) = 0x1F00200;  // Enable clock CSITE, IRAMA, IRAMB, IRAMC, IRAMD, BPMP_CACHE_RAM.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) = 0x80400808; // Enable clock MSELECT, APB2APE, SPDIF_DOUBLER, SE.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_W) = 0x402000FC; // Enable clock PCIERX0, PCIERX1, PCIERX2, PCIERX3, PCIERX4, PCIERX5, ENTROPY, MC1.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X) = 0x23000780; // Enable clock MC_CAPA, MC_CAPB, MC_CPU, MC_BBC, DBGAPB, HPLL_ADSP, PLLG_REF.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) = 0x300;      // Enable clock MC_CDPA, MC_CCPA.

	// Disable clock gate overrides.
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD) = 0;
	CLOCK(CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE) = 0;

	// Set child clock sources.
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) &= 0x1F7FFFFF; // Disable PLLD and set reference clock and csi clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SOR1) &= 0xFFFF3FFF; // Set SOR1 to automatic muxing of safe clock (24MHz) or SOR1 clk switch.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_VI) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_VI) & 0x1FFFFFFF) | 0x80000000; // Set clock source to PLLP_OUT.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X) & 0x1FFFFFFF) | 0x80000000; // Set clock source to PLLP_OUT.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_NVENC) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_NVENC) & 0x1FFFFFFF) | 0x80000000; // Set clock source to PLLP_OUT.
}

void _config_se_brom()
{
	// Enable fuse clock.
	clock_enable_fuse(true);

	// Skip SBK/SSK if sept was run.
	if (!(b_cfg.boot_cfg & BOOT_CFG_SEPT_RUN))
	{
		// Bootrom part we skipped.
		u32 sbk[4] = {
			FUSE(FUSE_PRIVATE_KEY0),
			FUSE(FUSE_PRIVATE_KEY1),
			FUSE(FUSE_PRIVATE_KEY2),
			FUSE(FUSE_PRIVATE_KEY3)
		};
		// Set SBK to slot 14.
		se_aes_key_set(14, sbk, 0x10);

		// Lock SBK from being read.
		SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 14 * 4) = 0x7E;

		// Lock SSK (although it's not set and unused anyways).
		SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 15 * 4) = 0x7E;
	}

	// This memset needs to happen here, else TZRAM will behave weirdly later on.
	memset((void *)TZRAM_BASE, 0, 0x10000);
	PMC(APBDEV_PMC_CRYPTO_OP) = PMC_CRYPTO_OP_SE_ENABLE;
	SE(SE_INT_STATUS_REG_OFFSET) = 0x1F;

	// Clear the boot reason to avoid problems later
	PMC(APBDEV_PMC_SCRATCH200) = 0x0;
	PMC(APBDEV_PMC_RST_STATUS) = 0x0;
	APB_MISC(APB_MISC_PP_STRAPPING_OPT_A) = (APB_MISC(APB_MISC_PP_STRAPPING_OPT_A) & 0xF0) | (7 << 10);
}

void _config_regulators()
{
	// Disable low battery shutdown monitor.
	max77620_low_battery_monitor_config(false);

	// Disable SDMMC1 IO power.
	gpio_output_enable(GPIO_PORT_E, GPIO_PIN_4, GPIO_OUTPUT_DISABLE);
	max77620_regulator_enable(REGULATOR_LDO2, 0);
	sd_power_cycle_time_start = get_tmr_ms();

	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CNFGBBC, MAX77620_CNFGBBC_RESISTOR_1K);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1,
		(1 << 6) | (3 << MAX77620_ONOFFCNFG1_MRT_SHIFT)); // PWR delay for forced shutdown off.

	// Configure all Flexible Power Sequencers.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG0,
		(7 << MAX77620_FPS_TIME_PERIOD_SHIFT));
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG1,
		(7 << MAX77620_FPS_TIME_PERIOD_SHIFT) | (1 << MAX77620_FPS_EN_SRC_SHIFT));
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_CFG2,
		(7 << MAX77620_FPS_TIME_PERIOD_SHIFT));
	max77620_regulator_config_fps(REGULATOR_LDO4);
	max77620_regulator_config_fps(REGULATOR_LDO8);
	max77620_regulator_config_fps(REGULATOR_SD0);
	max77620_regulator_config_fps(REGULATOR_SD1);
	max77620_regulator_config_fps(REGULATOR_SD3);

	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_FPS_GPIO3,
		(4 << MAX77620_FPS_TIME_PERIOD_SHIFT) | (2 << MAX77620_FPS_PD_PERIOD_SHIFT)); // 3.x+

	// Set vdd_core voltage to 1.125V.
	max77620_regulator_set_voltage(REGULATOR_SD0, 1125000);

	// Fix CPU/GPU after a L4T warmboot.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_GPIO5, 2);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_GPIO6, 2);

	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_VOUT_REG, MAX77621_VOUT_0_95V); // Disable power.
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_VOUT_DVC_REG, MAX77621_VOUT_ENABLE | MAX77621_VOUT_1_09V); // Enable DVS power.
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_CONTROL1_REG, MAX77621_RAMP_50mV_PER_US);
	i2c_send_byte(I2C_5, MAX77621_CPU_I2C_ADDR, MAX77621_CONTROL2_REG,
		MAX77621_T_JUNCTION_120 | MAX77621_FT_ENABLE | MAX77621_CKKADV_TRIP_75mV_PER_US_HIST_DIS |
		MAX77621_CKKADV_TRIP_150mV_PER_US | MAX77621_INDUCTOR_NOMINAL);

	i2c_send_byte(I2C_5, MAX77621_GPU_I2C_ADDR, MAX77621_VOUT_REG, MAX77621_VOUT_0_95V); // Disable power.
	i2c_send_byte(I2C_5, MAX77621_GPU_I2C_ADDR, MAX77621_VOUT_DVC_REG, MAX77621_VOUT_ENABLE | MAX77621_VOUT_1_09V); // Enable DVS power.
	i2c_send_byte(I2C_5, MAX77621_GPU_I2C_ADDR, MAX77621_CONTROL1_REG, MAX77621_RAMP_50mV_PER_US);
	i2c_send_byte(I2C_5, MAX77621_GPU_I2C_ADDR, MAX77621_CONTROL2_REG,
		MAX77621_T_JUNCTION_120 | MAX77621_FT_ENABLE | MAX77621_CKKADV_TRIP_75mV_PER_US_HIST_DIS |
		MAX77621_CKKADV_TRIP_150mV_PER_US | MAX77621_INDUCTOR_NOMINAL);
}

void config_hw()
{
	// Bootrom stuff we skipped by going through rcm.
	_config_se_brom();
	//FUSE(FUSE_PRIVATEKEYDISABLE) = 0x11;
	SYSREG(AHB_AHB_SPARE_REG) &= 0xFFFFFF9F; // Unset APB2JTAG_OVERRIDE_EN and OBS_OVERRIDE_EN.
	PMC(APBDEV_PMC_SCRATCH49) = PMC(APBDEV_PMC_SCRATCH49) & 0xFFFFFFFC;

	_mbist_workaround();
	clock_enable_se();

	// Enable fuse clock.
	clock_enable_fuse(true);

	// Disable fuse programming.
	fuse_disable_program();

	mc_enable();

	_config_oscillators();
	APB_MISC(APB_MISC_PP_PINMUX_GLOBAL) = 0;
	_config_gpios();

#ifdef DEBUG_UART_PORT
	clock_enable_uart(DEBUG_UART_PORT);
	uart_init(DEBUG_UART_PORT, 115200);
#endif

	clock_enable_cl_dvfs();

	clock_enable_i2c(I2C_1);
	clock_enable_i2c(I2C_5);

	clock_enable_tzram();

	i2c_init(I2C_1);
	i2c_init(I2C_5);

	_config_regulators();

	_config_pmc_scratch(); // Missing from 4.x+

	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003333; // Set SCLK to PLLP_OUT (408MHz).

	sdram_init();

	bpmp_mmu_enable();

	// Clear flags from PMC_SCRATCH0
	PMC(APBDEV_PMC_SCRATCH0) &= ~PMC_SCRATCH0_MODE_PAYLOAD;
}

void reconfig_hw_workaround(bool extra_reconfig, u32 magic)
{
	// Flush and disable MMU.
	bpmp_mmu_disable();
	bpmp_clk_rate_set(BPMP_CLK_NORMAL);
	minerva_change_freq(FREQ_204);
	nyx_str->mtc_cfg.init_done = 0;

	// Re-enable clocks to Audio Processing Engine as a workaround to hanging.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) |= (1 << 10); // Enable AHUB clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) |= (1 <<  6); // Enable APE clock.

	if (extra_reconfig)
	{
		msleep(10);
		PMC(APBDEV_PMC_PWR_DET_VAL) |= PMC_PWR_DET_SDMMC1_IO_EN;

		clock_disable_cl_dvfs();

		// Disable Joy-con GPIOs.
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_SPIO);
	}

	// Power off display.
	display_end();

	// Enable clock to USBD and init SDMMC1 to avoid hangs with bad hw inits.
	if (magic == 0xBAADF00D)
	{
		CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) |= (1 << 22);
		sdmmc_init(&sd_sdmmc, SDMMC_1, SDMMC_POWER_3_3, SDMMC_BUS_WIDTH_1, SDHCI_TIMING_SD_ID, 0);
		clock_disable_cl_dvfs();

		msleep(200);
	}
}
