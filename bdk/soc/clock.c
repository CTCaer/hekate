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

#include <soc/bpmp.h>
#include <soc/clock.h>
#include <soc/hw_init.h>
#include <soc/pmc.h>
#include <soc/timer.h>
#include <soc/t210.h>
#include <storage/sdmmc.h>

#define RST_DEV_L_SET CLK_RST_CONTROLLER_RST_DEV_L_SET
#define RST_DEV_H_SET CLK_RST_CONTROLLER_RST_DEV_H_SET
#define RST_DEV_U_SET CLK_RST_CONTROLLER_RST_DEV_U_SET
#define RST_DEV_V_SET CLK_RST_CONTROLLER_RST_DEV_V_SET
#define RST_DEV_W_SET CLK_RST_CONTROLLER_RST_DEV_W_SET
#define RST_DEV_X_SET CLK_RST_CONTROLLER_RST_DEV_X_SET
#define RST_DEV_Y_SET CLK_RST_CONTROLLER_RST_DEV_Y_SET

#define CLK_ENB_L_SET CLK_RST_CONTROLLER_CLK_ENB_L_SET
#define CLK_ENB_H_SET CLK_RST_CONTROLLER_CLK_ENB_H_SET
#define CLK_ENB_U_SET CLK_RST_CONTROLLER_CLK_ENB_U_SET
#define CLK_ENB_V_SET CLK_RST_CONTROLLER_CLK_ENB_V_SET
#define CLK_ENB_W_SET CLK_RST_CONTROLLER_CLK_ENB_W_SET
#define CLK_ENB_X_SET CLK_RST_CONTROLLER_CLK_ENB_X_SET
#define CLK_ENB_Y_SET CLK_RST_CONTROLLER_CLK_ENB_Y_SET

#define RST_DEV_H_CLR CLK_RST_CONTROLLER_RST_DEV_H_CLR
#define CLK_ENB_H_CLR CLK_RST_CONTROLLER_CLK_ENB_H_CLR

typedef struct _clk_rst_mgd_t
{
	u16 reset;  // Reset  SET.
	u16 enable; // Enable SET.
	u16 source;
	u8  index;
} clk_rst_mgd_t;

typedef struct _clock_osc_t
{
	u32 freq;
	u16 min;
	u16 max;
} clock_osc_t;

static const clock_osc_t _clock_osc_cnt[] = {
	{ 12000, 706,  757  },
	{ 13000, 766,  820  },
	{ 16800, 991,  1059 },
	{ 19200, 1133, 1210 },
	{ 26000, 1535, 1638 },
	{ 38400, 2268, 2418 },
	{ 48000, 2836, 3023 }
};

/*
 * T210 rare HW Errata
 * A fraction of T210 silicon has an undocumented HW Errata on SDMMC clock state machine.
 * Specifically on enable when using the combo registers. Using SET/CLR variants is mandatory.
 */
static const clk_rst_mgd_t _clock_sdmmc[] = {
	{ RST_DEV_L_SET, CLK_ENB_L_SET, CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1, CLK_L_SDMMC1 },
	{ RST_DEV_L_SET, CLK_ENB_L_SET, CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC2, CLK_L_SDMMC2 },
	{ RST_DEV_U_SET, CLK_ENB_U_SET, CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3, CLK_U_SDMMC3 },
	{ RST_DEV_L_SET, CLK_ENB_L_SET, CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4, CLK_L_SDMMC4 },
};

/* clk_rst_t: reset, enable, source, index, clk_src, clk_div */

static const clk_rst_t _clock_uart[] = {
	{ RST_DEV_L_SET, CLK_ENB_L_SET, CLK_RST_CONTROLLER_CLK_SOURCE_UARTA,   CLK_L_UARTA,   0, CLK_SRC_DIV(2) },
	{ RST_DEV_L_SET, CLK_ENB_L_SET, CLK_RST_CONTROLLER_CLK_SOURCE_UARTB,   CLK_L_UARTB,   0, CLK_SRC_DIV(2) },
	{ RST_DEV_H_SET, CLK_ENB_H_SET, CLK_RST_CONTROLLER_CLK_SOURCE_UARTC,   CLK_H_UARTC,   0, CLK_SRC_DIV(2) },
	{ RST_DEV_U_SET, CLK_ENB_U_SET, CLK_RST_CONTROLLER_CLK_SOURCE_UARTD,   CLK_U_UARTD,   0, CLK_SRC_DIV(2) },
	{ RST_DEV_Y_SET, CLK_ENB_Y_SET, CLK_RST_CONTROLLER_CLK_SOURCE_UARTAPE, CLK_Y_UARTAPE, 0, CLK_SRC_DIV(2) }
};

//I2C default parameters - TLOW: 4, THIGH: 2, DEBOUNCE: 0, FM_DIV: 26.
static const clk_rst_t _clock_i2c[] = {
	{ RST_DEV_L_SET, CLK_ENB_L_SET, CLK_RST_CONTROLLER_CLK_SOURCE_I2C1, CLK_L_I2C1, 0, CLK_SRC_DIV(10.5) }, // 20.4 MHz -> 100 KHz
	{ RST_DEV_H_SET, CLK_ENB_H_SET, CLK_RST_CONTROLLER_CLK_SOURCE_I2C2, CLK_H_I2C2, 0, CLK_SRC_DIV(3)    }, // 81.6 MHz -> 400 KHz
	{ RST_DEV_U_SET, CLK_ENB_U_SET, CLK_RST_CONTROLLER_CLK_SOURCE_I2C3, CLK_U_I2C3, 0, CLK_SRC_DIV(3)    }, // 81.6 MHz -> 400 KHz
	{ RST_DEV_V_SET, CLK_ENB_V_SET, CLK_RST_CONTROLLER_CLK_SOURCE_I2C4, CLK_V_I2C4, 0, CLK_SRC_DIV(10.5) }, // 20.4 MHz -> 100 KHz
	{ RST_DEV_H_SET, CLK_ENB_H_SET, CLK_RST_CONTROLLER_CLK_SOURCE_I2C5, CLK_H_I2C5, 0, CLK_SRC_DIV(3)    }, // 81.6 MHz -> 400 KHz
	{ RST_DEV_X_SET, CLK_ENB_X_SET, CLK_RST_CONTROLLER_CLK_SOURCE_I2C6, CLK_X_I2C6, 0, CLK_SRC_DIV(10.5) }  // 20.4 MHz -> 100 KHz
};

static clk_rst_t _clock_se = {
	RST_DEV_V_SET, CLK_ENB_V_SET, CLK_RST_CONTROLLER_CLK_SOURCE_SE,          CLK_V_SE,          0, CLK_SRC_DIV(1)   // 408 MHz. Max: 627.2 MHz.
};
static clk_rst_t _clock_tzram = {
	RST_DEV_V_SET, CLK_ENB_V_SET, CLK_NO_SOURCE,                             CLK_V_TZRAM,       0, 0
};
static clk_rst_t _clock_host1x = { // Has idle divisor.
	RST_DEV_L_SET, CLK_ENB_L_SET, CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X,      CLK_L_HOST1X,      4, CLK_SRC_DIV(2.5) // 163.2MHz. Max: 408 MHz.
};
static clk_rst_t _clock_tsec = {
	RST_DEV_U_SET, CLK_ENB_U_SET, CLK_RST_CONTROLLER_CLK_SOURCE_TSEC,        CLK_U_TSEC,        0, CLK_SRC_DIV(2)   // 204 MHz. Max: 408 MHz.
};
static clk_rst_t _clock_nvdec = {
	RST_DEV_Y_SET, CLK_ENB_Y_SET, CLK_RST_CONTROLLER_CLK_SOURCE_NVDEC,       CLK_Y_NVDEC,       4, CLK_SRC_DIV(1)   // 408 MHz. Max: 716.8/979.2 MHz.
};
static clk_rst_t _clock_nvjpg = {
	RST_DEV_Y_SET, CLK_ENB_Y_SET, CLK_RST_CONTROLLER_CLK_SOURCE_NVJPG,       CLK_Y_NVJPG,       4, CLK_SRC_DIV(1)   // 408 MHz. Max: 627.2/652.8 MHz.
};
static clk_rst_t _clock_vic = { // Has idle divisor.
	RST_DEV_X_SET, CLK_ENB_X_SET, CLK_RST_CONTROLLER_CLK_SOURCE_VIC,         CLK_X_VIC,         2, CLK_SRC_DIV(1)   // 408 MHz. Max: 627.2/652.8 MHz.
};
static clk_rst_t _clock_sor_safe = {
	RST_DEV_Y_SET, CLK_ENB_Y_SET, CLK_NO_SOURCE,                             CLK_Y_SOR_SAFE,    0, 0                // 24 MHz.
};
static clk_rst_t _clock_sor0 = {
	RST_DEV_X_SET, CLK_ENB_X_SET, CLK_NOT_USED,                              CLK_X_SOR0,        0, 0                // 24 MHz (safe).
};
static clk_rst_t _clock_sor1 = {
	RST_DEV_X_SET, CLK_ENB_X_SET, CLK_RST_CONTROLLER_CLK_SOURCE_SOR1,        CLK_X_SOR1,        0, CLK_SRC_DIV(2)   // 204 MHz.
};
static clk_rst_t _clock_kfuse = {
	RST_DEV_H_SET, CLK_ENB_H_SET, CLK_NO_SOURCE,                             CLK_H_KFUSE,       0, 0
};
static clk_rst_t _clock_cl_dvfs =	{
	RST_DEV_W_SET, CLK_ENB_W_SET, CLK_NO_SOURCE,                             CLK_W_DVFS,        0, 0
};
static clk_rst_t _clock_coresight = {
	RST_DEV_U_SET, CLK_ENB_U_SET, CLK_RST_CONTROLLER_CLK_SOURCE_CSITE,       CLK_U_CSITE,       0, CLK_SRC_DIV(3)   // 136 MHz.
};
static clk_rst_t _clock_pwm = {
	RST_DEV_L_SET, CLK_ENB_L_SET, CLK_RST_CONTROLLER_CLK_SOURCE_PWM,         CLK_L_PWM,         6, CLK_SRC_DIV(3)   // Fref: 6.4MHz. HOS: PLLP / 54 = 7.55MHz.
};
static clk_rst_t _clock_sdmmc_legacy_tm = {
	RST_DEV_Y_SET, CLK_ENB_Y_SET, CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC_LEGACY_TM, CLK_Y_SDMMC_LEGACY_TM, 4, CLK_SRC_DIV(34) // 12MHz.
};
static clk_rst_t _clock_apbdma = {
	RST_DEV_H_SET, CLK_ENB_H_SET, CLK_NO_SOURCE,                             CLK_H_APBDMA,      0, 0                // Max: 204 MHz.
};
static clk_rst_t _clock_ahbdma = {
	RST_DEV_H_SET, CLK_ENB_H_SET, CLK_NO_SOURCE,                             CLK_H_AHBDMA,      0, 0                // Max: 408 MHz.
};
static clk_rst_t _clock_actmon = {
	RST_DEV_V_SET, CLK_ENB_V_SET, CLK_RST_CONTROLLER_CLK_SOURCE_ACTMON,      CLK_V_ACTMON,      6, CLK_SRC_DIV(1)   // 19.2 MHz.
};
static clk_rst_t _clock_extperiph1 = {
	RST_DEV_V_SET, CLK_ENB_V_SET, CLK_RST_CONTROLLER_CLK_SOURCE_EXTPERIPH1,  CLK_V_EXTPERIPH1,  0, CLK_SRC_DIV(1)
};
static clk_rst_t _clock_extperiph2 = {
	RST_DEV_V_SET, CLK_ENB_V_SET, CLK_RST_CONTROLLER_CLK_SOURCE_EXTPERIPH2,  CLK_V_EXTPERIPH2,  2, CLK_SRC_DIV(102) // 4.0 MHz
};

void clock_enable(const clk_rst_t *clk)
{
	const u32 module = BIT(clk->index);

	// Put clock into reset.
	CLOCK(clk->reset) = module;

	// Disable.
	CLOCK(clk->enable + CLK_CLR_OFFSET) = module;

	// Configure clock source if required.
	if (clk->source)
		CLOCK(clk->source) = (clk->clk_src << 29u) | clk->clk_div;

	// Enable.
	CLOCK(clk->enable) = module;
	usleep(2);

	// Take clock off reset.
	CLOCK(clk->reset + CLK_CLR_OFFSET) = module;

	// Commit changes.
	(void)CLOCK(clk->reset);
}

void clock_disable(const clk_rst_t *clk)
{
	const u32 module = BIT(clk->index);

	// Put clock into reset.
	CLOCK(clk->reset) = module;

	// Disable.
	CLOCK(clk->enable + CLK_CLR_OFFSET) = module;
}

void clock_enable_fuse(bool enable)
{
	// Enable Fuse registers visibility.
	CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) = (CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) & 0xEFFFFFFF) | ((enable & 1) << 28);
}

void clock_enable_uart(u32 idx)
{
	// Ease the stress to APB.
	bpmp_clk_rate_relaxed(true);

	clock_enable(&_clock_uart[idx]);

	// Restore sys clock.
	bpmp_clk_rate_relaxed(false);
}

void clock_disable_uart(u32 idx)
{
	clock_disable(&_clock_uart[idx]);
}

#define UART_SRC_CLK_DIV_EN BIT(24)

int clock_uart_use_src_div(u32 idx, u32 baud)
{
	u32 clk_src = CLOCK(_clock_uart[idx].source) & 0xE0000000;

	if (baud == 3000000)
		CLOCK(_clock_uart[idx].source) = clk_src | UART_SRC_CLK_DIV_EN | CLK_SRC_DIV(8.5);
	else if (baud == 1000000)
		CLOCK(_clock_uart[idx].source) = clk_src | UART_SRC_CLK_DIV_EN | CLK_SRC_DIV(25.5);
	else
	{
		CLOCK(_clock_uart[idx].source) = clk_src | CLK_SRC_DIV(2);

		return 1;
	}

	return 0;
}

void clock_enable_i2c(u32 idx)
{
	// Ease the stress to APB.
	bpmp_clk_rate_relaxed(true);

	clock_enable(&_clock_i2c[idx]);

	// Restore sys clock.
	bpmp_clk_rate_relaxed(false);
}

void clock_disable_i2c(u32 idx)
{
	clock_disable(&_clock_i2c[idx]);
}

void clock_enable_se()
{
	clock_enable(&_clock_se);

	// Lock clock to always enabled if T210B01.
	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210B01)
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SE) |= BIT(8);
}

void clock_enable_tzram()
{
	clock_enable(&_clock_tzram);
}

void clock_enable_host1x()
{
	clock_enable(&_clock_host1x);

	// Set idle frequency to 81.6 MHz.
	// CLOCK(_clock_host1x.clk_src) |= CLK_SRC_DIV(5) << 8;
}

void clock_disable_host1x()
{
	clock_disable(&_clock_host1x);
}

void clock_enable_tsec()
{
	clock_enable(&_clock_tsec);
}

void clock_disable_tsec()
{
	clock_disable(&_clock_tsec);
}

void clock_enable_nvdec()
{
	clock_enable(&_clock_nvdec);
}

void clock_disable_nvdec()
{
	clock_disable(&_clock_nvdec);
}

void clock_enable_nvjpg()
{
	clock_enable(&_clock_nvjpg);
}

void clock_disable_nvjpg()
{
	clock_disable(&_clock_nvjpg);
}

void clock_enable_vic()
{
	// Ease the stress to APB.
	bpmp_clk_rate_relaxed(true);

	clock_enable(&_clock_vic);

	// Set idle frequency to 136 MHz.
	// CLOCK(_clock_vic.clk_src) |= CLK_SRC_DIV(3) << 8;

	// Restore sys clock.
	bpmp_clk_rate_relaxed(false);
}

void clock_disable_vic()
{
	clock_disable(&_clock_vic);
}

void clock_enable_sor_safe()
{
	clock_enable(&_clock_sor_safe);
}

void clock_disable_sor_safe()
{
	clock_disable(&_clock_sor_safe);
}

void clock_enable_sor0()
{
	clock_enable(&_clock_sor0);
}

void clock_disable_sor0()
{
	clock_disable(&_clock_sor0);
}

void clock_enable_sor1()
{
	clock_enable(&_clock_sor1);
}

void clock_disable_sor1()
{
	clock_disable(&_clock_sor1);
}

void clock_enable_kfuse()
{
	CLOCK(RST_DEV_H_SET) = BIT(CLK_H_KFUSE);
	CLOCK(CLK_ENB_H_CLR) = BIT(CLK_H_KFUSE);
	CLOCK(CLK_ENB_H_SET) = BIT(CLK_H_KFUSE);
	usleep(10); // Wait 10us to prevent glitching.

	CLOCK(RST_DEV_H_CLR) = BIT(CLK_H_KFUSE);
	usleep(20); // Wait 20us for KFUSE HW to init.
}

void clock_disable_kfuse()
{
	clock_disable(&_clock_kfuse);
}

void clock_enable_cl_dvfs()
{
	clock_enable(&_clock_cl_dvfs);
}

void clock_disable_cl_dvfs()
{
	clock_disable(&_clock_cl_dvfs);
}

void clock_enable_coresight()
{
	clock_enable(&_clock_coresight);
}

void clock_disable_coresight()
{
	clock_disable(&_clock_coresight);
}

void clock_enable_pwm()
{
	// Ease the stress to APB.
	bpmp_clk_rate_relaxed(true);

	clock_enable(&_clock_pwm);

	// Restore sys clock.
	bpmp_clk_rate_relaxed(false);
}

void clock_disable_pwm()
{
	clock_disable(&_clock_pwm);
}

void clock_enable_apbdma()
{
	clock_enable(&_clock_apbdma);
}

void clock_disable_apbdma()
{
	clock_disable(&_clock_apbdma);
}

void clock_enable_ahbdma()
{
	clock_enable(&_clock_ahbdma);
}

void clock_disable_ahbdma()
{
	clock_disable(&_clock_ahbdma);
}

void clock_enable_actmon()
{
	clock_enable(&_clock_actmon);
}

void clock_disable_actmon()
{
	clock_disable(&_clock_actmon);
}

void clock_enable_extperiph1()
{
	clock_enable(&_clock_extperiph1);

	PMC(APBDEV_PMC_CLK_OUT_CNTRL) |= PMC_CLK_OUT_CNTRL_CLK1_SRC_SEL(OSC_CAR) | PMC_CLK_OUT_CNTRL_CLK1_FORCE_EN;
	usleep(5);
}

void clock_disable_extperiph1()
{
	PMC(APBDEV_PMC_CLK_OUT_CNTRL) &= ~((PMC_CLK_OUT_CNTRL_CLK1_SRC_SEL(OSC_CAR)) | PMC_CLK_OUT_CNTRL_CLK1_FORCE_EN);
	clock_disable(&_clock_extperiph1);
}

void clock_enable_extperiph2()
{
	clock_enable(&_clock_extperiph2);

	PMC(APBDEV_PMC_CLK_OUT_CNTRL) |= PMC_CLK_OUT_CNTRL_CLK2_SRC_SEL(OSC_CAR) | PMC_CLK_OUT_CNTRL_CLK2_FORCE_EN;
	usleep(5);
}

void clock_disable_extperiph2()
{
	PMC(APBDEV_PMC_CLK_OUT_CNTRL) &= ~((PMC_CLK_OUT_CNTRL_CLK2_SRC_SEL(OSC_CAR)) | PMC_CLK_OUT_CNTRL_CLK2_FORCE_EN);
	clock_disable(&_clock_extperiph2);
}

static void _clock_pll_wait_lock(u32 base, u32 max_delay)
{
	for (u32 i = 0; i < max_delay; i++)
	{
		if (CLOCK(base) & PLL_BASE_LOCK)
			break;
		usleep(1);
	}

	usleep(2);
}

void clock_enable_plld(u32 divp, u32 divn, bool lowpower, bool tegra_t210)
{
	u32 plld_div = (divp << 20) | (divn << 11) | 1;

	// N divider is fractional, so N = DIVN + 1/2 + PLLD_SDM_DIN/8192.
	u32 misc = BIT(21) | BIT(19) | BIT(18) | BIT(16) | 0xFC00; // Clock enable and PLLD_SDM_DIN: -1024 -> DIVN + 0.375.
	if (lowpower && tegra_t210)
		misc = BIT(21) | BIT(19) | BIT(18) | BIT(16) | 0x0AAA; // Clock enable and PLLD_SDM_DIN:  2730 -> DIVN + 0.833.

	// Set DISP1 clock source.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_DISP1) = 2 << 29u; // PLLD_OUT0.

	// Set dividers and enable PLLD.
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE)  = PLL_BASE_ENABLE | PLL_BASE_LOCK | plld_div;
	CLOCK(CLK_RST_CONTROLLER_PLLD_MISC1) = tegra_t210 ? 0x20 : 0; // Keep default PLLD_SETUP.

	// Set PLLD_SDM_DIN and enable (T210) PLLD to DSI pads.
	CLOCK(CLK_RST_CONTROLLER_PLLD_MISC) = misc;

	// Wait for PLL to stabilize.
	_clock_pll_wait_lock(CLK_RST_CONTROLLER_PLLD_BASE, 1000);
}

void clock_enable_pllx()
{
	// Configure and enable PLLX if disabled.
	if (!(CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) & PLL_BASE_ENABLE))  // PLLX_ENABLE.
	{
		CLOCK(CLK_RST_CONTROLLER_PLLX_MISC_3) &= ~PLLX_MISC3_IDDQ; // Disable IDDQ.
		usleep(2);

		// Set div configuration.
		const u32 pllx_div_cfg = (2 << 20) | (156 << 8) | 2; // P div: 2 (3), N div: 156, M div: 2. 998.4 MHz.

		// Bypass dividers.
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = PLL_BASE_BYPASS | pllx_div_cfg;
		// Disable bypass
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = pllx_div_cfg;
		// Set PLLX_LOCK_ENABLE.
		CLOCK(CLK_RST_CONTROLLER_PLLX_MISC) |= PLLX_MISC_LOCK_EN;
		// Enable PLLX.
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = PLL_BASE_ENABLE | pllx_div_cfg;
	}

	// Wait for PLL to stabilize.
	_clock_pll_wait_lock(CLK_RST_CONTROLLER_PLLX_BASE, 300);
}

void clock_enable_pllc(u32 divn)
{
	u32 enabled = CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) & PLL_BASE_ENABLE;
	u8 pll_divn_curr = (CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) >> 10) & 0xFF;

	// Check if already enabled and configured.
	if (enabled && (pll_divn_curr == divn))
		return;

	// WAR: Disable first to avoid HPLL overshoot.
	if (enabled)
		clock_disable_pllc();

	// Take PLLC out of reset (misc) and set misc2 parameters.
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC)    = (0x8000 << 4); // PLLC_EXT_FRU.
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC_2) |= 0xF << 8;      // PLLC_FLL_LD_MEM.

	// Disable PLL and IDDQ in case they are on.
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE)   &= ~PLL_BASE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC_1) &= ~PLLC_MISC1_IDDQ;
	usleep(10);

	// Set PLLC dividers.
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) = (divn << 10) | 6; // DIVM: 6, DIVP: 1.

	// Enable PLLC and wait for Phase and Frequency lock.
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) |= PLL_BASE_ENABLE;
	_clock_pll_wait_lock(CLK_RST_CONTROLLER_PLLC_BASE, 300);

	// Disable PLLC_OUT1, enable reset and set div to 1.
	CLOCK(CLK_RST_CONTROLLER_PLLC_OUT) = 0;

	// Enable PLLC_OUT1 and bring it out of reset.
	CLOCK(CLK_RST_CONTROLLER_PLLC_OUT) |= PLLC_OUT1_CLKEN | PLLC_OUT1_RSTN_CLR;
	msleep(1); // Wait a bit for PLL to stabilize.
}

void clock_disable_pllc()
{
	// Disable PLLC and PLLC_OUT1.
	CLOCK(CLK_RST_CONTROLLER_PLLC_OUT)    &= ~PLLC_OUT1_RSTN_CLR;
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC)    = PLLC_MISC_RESET;
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE)   &= ~PLL_BASE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC_1) |= PLLC_MISC1_IDDQ;
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC_2) &= ~(0xFF << 8); // PLLC_FLL_LD_MEM.
	usleep(10);
}

#define PLLC4_ENABLED BIT(31)
#define PLLC4_IN_USE  (~PLLC4_ENABLED)

u32 pllc4_enabled = 0;

static void _clock_enable_pllc4(u32 mask)
{
	pllc4_enabled |= mask;

	if (pllc4_enabled & PLLC4_ENABLED)
		return;

	// Enable Phase and Frequency lock detection.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_MISC) = PLLC4_MISC_EN_LCKDET;

	// Disable PLL and IDDQ in case they are on.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLL_BASE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLLC4_BASE_IDDQ;
	usleep(10);

	// Set PLLC4 dividers.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) = (0 << 19) | (104 << 8) | 4; // DIVP: 1, DIVN: 104, DIVM: 4. 998MHz OUT0, 199MHz OUT2.

	// Enable PLLC4 and wait for Phase and Frequency lock.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) |= PLL_BASE_ENABLE;
	_clock_pll_wait_lock(CLK_RST_CONTROLLER_PLLC4_BASE, 300 + 700);

	pllc4_enabled |= PLLC4_ENABLED;
}

static void _clock_disable_pllc4(u32 mask)
{
	pllc4_enabled &= ~mask;

	// Check if currently in use or disabled.
	if ((pllc4_enabled & PLLC4_IN_USE) || !(pllc4_enabled & PLLC4_ENABLED))
		return;

	// Disable PLLC4.
	usleep(100); // Wait at least 100us to prevent glitching.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLL_BASE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) |= PLLC4_BASE_IDDQ;
	usleep(10);

	pllc4_enabled = 0;
}

void clock_enable_pllu()
{
	// Configure PLLU.
	CLOCK(CLK_RST_CONTROLLER_PLLU_MISC) |= BIT(29); // Disable reference clock.
	u32 pllu_cfg = (CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) & 0xFFE00000) | BIT(24) | (1 << 16) | (0x19 << 8) | 2;
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) = pllu_cfg;
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) = pllu_cfg | PLL_BASE_ENABLE; // Enable.

	// Wait for PLL to stabilize.
	_clock_pll_wait_lock(CLK_RST_CONTROLLER_PLLU_BASE, 1000);
	usleep(8);

	// Enable PLLU USB/HSIC/ICUSB/48M.
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) |= 0x2E00000;
}

void clock_disable_pllu()
{
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) &= ~0x2E00000; // Disable PLLU USB/HSIC/ICUSB/48M.
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) &= ~BIT(30);   // Disable PLLU.
	CLOCK(CLK_RST_CONTROLLER_PLLU_MISC) &= ~BIT(29);   // Enable reference clock.
}

void clock_enable_utmipll()
{
	// Set UTMIPLL dividers and config based on OSC and enable it to 960 MHz.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG0) = (CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG0) & 0xFF0000FF) | (25 << 16) | (1 << 8); // 38.4Mhz * (25 / 1) = 960 MHz.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) = (CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) & 0xFF00003F) | (24 << 18); // Set delay count for 38.4Mhz osc crystal.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) = (CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) &  0x7FFA000) | (1 << 15) | 375;

	// Wait for UTMIPLL to stabilize.
	u32 retries = 10; // Wait 20us
	while (!(CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) & UTMIPLL_LOCK) && retries)
	{
		usleep(1);
		retries--;
	}
}

static int _clock_sdmmc_in_reset(u32 id)
{
	const clk_rst_mgd_t *clk = &_clock_sdmmc[id];

	return CLOCK(clk->reset) & BIT(clk->index);
}

static void _clock_sdmmc_set_reset(u32 id)
{
	const clk_rst_mgd_t *clk = &_clock_sdmmc[id];

	CLOCK(clk->reset) = BIT(clk->index);
}

static void _clock_sdmmc_clr_reset(u32 id)
{
	const clk_rst_mgd_t *clk = &_clock_sdmmc[id];

	CLOCK(clk->reset + CLK_CLR_OFFSET) = BIT(clk->index);
}

static int _clock_sdmmc_is_enabled(u32 id)
{
	const clk_rst_mgd_t *clk = &_clock_sdmmc[id];

	return CLOCK(clk->enable) & BIT(clk->index);
}

static void _clock_sdmmc_set_enable(u32 id)
{
	const clk_rst_mgd_t *clk = &_clock_sdmmc[id];

	CLOCK(clk->enable) = BIT(clk->index);
}

static void _clock_sdmmc_clr_enable(u32 id)
{
	const clk_rst_mgd_t *clk = &_clock_sdmmc[id];

	CLOCK(clk->enable + CLK_CLR_OFFSET) = BIT(clk->index);
}

static void _clock_sdmmc_config_legacy_tm()
{
	const clk_rst_t *clk = &_clock_sdmmc_legacy_tm;

	if (!(CLOCK(clk->enable) & BIT(clk->index)))
		clock_enable(clk);
}

typedef struct _clock_sdmmc_t
{
	u32 clock;
	u32 pclock;
} clock_sdmmc_t;

static clock_sdmmc_t _clock_sdmmc_table[4] = { 0 };

#define SDMMC_CLOCK_SRC_PLLP_OUT0      0x0
#define SDMMC_CLOCK_SRC_PLLC4_OUT2     0x3
#define SDMMC_CLOCK_SRC_PLLC4_OUT0     0x7
#define SDMMC4_CLOCK_SRC_PLLC4_OUT2_LJ 0x1

static int _clock_sdmmc_config_clock_host(u32 *pclock, u32 id, u32 clock)
{
	u32 divisor = 0;
	u32 source  = SDMMC_CLOCK_SRC_PLLP_OUT0;

	if (id > SDMMC_4)
		return 0;

	// Get IO clock divisor.
	switch (clock)
	{
	case 25000:
		*pclock = 24728;
		divisor = CLK_SRC_DIV(16.5);
		break;

	case 26000:
		*pclock = 25500;
		divisor = CLK_SRC_DIV(16);
		break;

	case 50000:
		*pclock = 48000;
		divisor = CLK_SRC_DIV(8.5);
		break;

	case 52000:
		*pclock = 51000;
		divisor = CLK_SRC_DIV(8);
		break;

	case 82000:
		*pclock = 81600;
		divisor = CLK_SRC_DIV(5);
		break;

	case 100000:
		source = SDMMC_CLOCK_SRC_PLLC4_OUT2;
		*pclock = 99840;
		divisor = CLK_SRC_DIV(2);
		break;

	case 164000:
		*pclock = 163200;
		divisor = CLK_SRC_DIV(2.5);
		break;

	case 200000:
		switch (id)
		{
		case SDMMC_1:
		case SDMMC_3:
			source = SDMMC_CLOCK_SRC_PLLC4_OUT2;
			break;
		case SDMMC_2:
		case SDMMC_4:
			source = SDMMC4_CLOCK_SRC_PLLC4_OUT2_LJ; // CLK RST divisor is ignored.
			break;
		}
		*pclock = 199680;
		divisor = CLK_SRC_DIV(1);
		break;

#ifdef BDK_SDMMC_UHS_DDR200_SUPPORT
	case 400000:
		source = SDMMC_CLOCK_SRC_PLLC4_OUT0;
		*pclock = 399360;
		divisor = CLK_SRC_DIV(2.5);
		break;
#endif
	}

	_clock_sdmmc_table[id].clock  = clock;
	_clock_sdmmc_table[id].pclock = *pclock;

	// Enable PLLC4 if in use by any SDMMC.
	if (source != SDMMC_CLOCK_SRC_PLLP_OUT0)
		_clock_enable_pllc4(BIT(id));

	// Set SDMMC legacy timeout clock.
	_clock_sdmmc_config_legacy_tm();

	// Set SDMMC clock.
	const clk_rst_mgd_t *clk = &_clock_sdmmc[id];
	CLOCK(clk->source) = (source << 29u) | divisor;

	return 1;
}

void clock_sdmmc_config_clock_source(u32 *pclock, u32 id, u32 clock)
{
	if (_clock_sdmmc_table[id].clock == clock)
	{
		*pclock = _clock_sdmmc_table[id].pclock;
	}
	else
	{
		// Ease the stress to APB.
		if (id == SDMMC_1)
			bpmp_clk_rate_relaxed(true);

		int is_enabled = _clock_sdmmc_is_enabled(id);
		if (is_enabled)
			_clock_sdmmc_clr_enable(id);

		_clock_sdmmc_config_clock_host(pclock, id, clock);

		if (is_enabled)
			_clock_sdmmc_set_enable(id);

		// Commit changes.
		_clock_sdmmc_in_reset(id);

		// Restore sys clock.
		if (id == SDMMC_1)
			bpmp_clk_rate_relaxed(false);
	}
}

void clock_sdmmc_get_card_clock_div(u32 *pclock, u16 *pdivisor, u32 type)
{
	// Get Card clock divisor.
	switch (type)
	{
	case SDHCI_TIMING_MMC_ID:     // Actual card clock: 386.36 KHz.
		*pclock = 26000;
		*pdivisor = 66;
		break;

	case SDHCI_TIMING_MMC_LS26:
		*pclock = 26000;
		*pdivisor = 1;
		break;

	case SDHCI_TIMING_MMC_HS52:
		*pclock = 52000;
		*pdivisor = 1;
		break;

	case SDHCI_TIMING_MMC_HS200:
	case SDHCI_TIMING_MMC_HS400:
	case SDHCI_TIMING_UHS_SDR104:
		*pclock = 200000;
		*pdivisor = 1;
		break;

	case SDHCI_TIMING_SD_ID:      // Actual card clock: 386.38 KHz.
		*pclock = 25000;
		*pdivisor = 64;
		break;

	case SDHCI_TIMING_SD_DS12:
	case SDHCI_TIMING_UHS_SDR12:
		*pclock = 25000;
		*pdivisor = 1;
		break;

	case SDHCI_TIMING_SD_HS25:
	case SDHCI_TIMING_UHS_SDR25:
		*pclock = 50000;
		*pdivisor = 1;
		break;

	case SDHCI_TIMING_UHS_SDR50:
		*pclock = 100000;
		*pdivisor = 1;
		break;

	case SDHCI_TIMING_UHS_SDR82:
		*pclock = 164000;
		*pdivisor = 1;
		break;

	case SDHCI_TIMING_UHS_DDR50:  // Actual card clock: 40.80 MHz.
		*pclock = 82000;
		*pdivisor = 2;
		break;

	case SDHCI_TIMING_MMC_HS100:  // Actual card clock: 99.84 MHz.
		*pclock = 200000;
		*pdivisor = 2;
		break;

#ifdef BDK_SDMMC_UHS_DDR200_SUPPORT
	case SDHCI_TIMING_UHS_DDR200: // Actual card clock: 199.68 KHz.
		*pclock = 400000;
		*pdivisor = 2;
		break;
#endif
	}
}

int clock_sdmmc_is_active(u32 id)
{
	return !_clock_sdmmc_in_reset(id) && _clock_sdmmc_is_enabled(id);
}

void clock_sdmmc_enable(u32 id, u32 clock)
{
	u32 pclock = 0;

	// Ease the stress to APB.
	if (id == SDMMC_1)
		bpmp_clk_rate_relaxed(true);

	_clock_sdmmc_clr_enable(id);
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_config_clock_host(&pclock, id, clock);
	_clock_sdmmc_set_enable(id);

	// Commit changes and wait 100 cycles for reset and for clocks to stabilize.
	_clock_sdmmc_in_reset(id);
	usleep((100 * 1000 + pclock - 1) / pclock);

	_clock_sdmmc_clr_reset(id);
	_clock_sdmmc_in_reset(id);

	// Restore sys clock.
	if (id == SDMMC_1)
		bpmp_clk_rate_relaxed(false);
}

void clock_sdmmc_disable(u32 id)
{
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_clr_enable(id);
	_clock_sdmmc_in_reset(id);
	_clock_disable_pllc4(BIT(id));
}

u32 clock_get_osc_freq()
{
	CLOCK(CLK_RST_CONTROLLER_OSC_FREQ_DET) = OSC_FREQ_DET_TRIG | (2 - 1); // 2 periods of 32.76KHz window.
	while (CLOCK(CLK_RST_CONTROLLER_OSC_FREQ_DET_STATUS) & OSC_FREQ_DET_BUSY)
		;
	u32 cnt = (CLOCK(CLK_RST_CONTROLLER_OSC_FREQ_DET_STATUS) & OSC_FREQ_DET_CNT);
	CLOCK(CLK_RST_CONTROLLER_OSC_FREQ_DET) = 0;

	// Return frequency in KHz.
	for (u32 i = 0; i < ARRAY_SIZE(_clock_osc_cnt); i++)
		if (cnt >= _clock_osc_cnt[i].min && cnt <= _clock_osc_cnt[i].max)
			return _clock_osc_cnt[i].freq;

	return 0;
}

u32 clock_get_dev_freq(clock_pto_id_t id)
{
	const u32 pto_win = 16;
	const u32 pto_osc = 32768;

	u32 val = ((id & PTO_SRC_SEL_MASK) << PTO_SRC_SEL_SHIFT) | PTO_DIV_SEL_DIV1 | PTO_CLK_ENABLE | (pto_win - 1);
	CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL) = val;
	(void)CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL);
	usleep(2);

	CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL) = val | PTO_CNT_RST;
	(void)CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL);
	usleep(2);

	CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL) = val;
	(void)CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL);
	usleep(2);

	CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL) = val | PTO_CNT_EN;
	(void)CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL);
	usleep((1000000 * pto_win / pto_osc) + 12 + 2);

	while (CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_STATUS) & PTO_CLK_CNT_BUSY)
		;

	u32 cnt = CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_STATUS) & PTO_CLK_CNT;

	CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL) = 0;
	(void)CLOCK(CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL);
	usleep(2);

	u32 freq_khz = (u64)cnt * pto_osc / pto_win / 1000;

	return freq_khz;
}

