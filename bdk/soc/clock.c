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

#include <soc/clock.h>
#include <soc/hw_init.h>
#include <soc/t210.h>
#include <storage/sdmmc.h>
#include <utils/util.h>

typedef struct _clock_osc_t
{
	u32 freq;
	u16 min;
	u16 max;
} clock_osc_t;

static const clock_osc_t _clock_osc_cnt[] = {
	{ 12000, 706,  757 },
	{ 13000, 766,  820 },
	{ 16800, 991,  1059 },
	{ 19200, 1133, 1210 },
	{ 26000, 1535, 1638 },
	{ 38400, 2268, 2418 },
	{ 48000, 2836, 3023 }
};

/* clock_t: reset, enable, source, index, clk_src, clk_div */

static const clock_t _clock_uart[] = {
	{ CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_UARTA,   CLK_L_UARTA,   0, 2 },
	{ CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_UARTB,   CLK_L_UARTB,   0, 2 },
	{ CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_CLK_SOURCE_UARTC,   CLK_H_UARTC,   0, 2 },
	{ CLK_RST_CONTROLLER_RST_DEVICES_U, CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_CONTROLLER_CLK_SOURCE_UARTD,   CLK_U_UARTD,   0, 2 },
	{ CLK_RST_CONTROLLER_RST_DEVICES_Y, CLK_RST_CONTROLLER_CLK_OUT_ENB_Y, CLK_RST_CONTROLLER_CLK_SOURCE_UARTAPE, CLK_Y_UARTAPE, 0, 2 }
};

//I2C default parameters - TLOW: 4, THIGH: 2, DEBOUNCE: 0, FM_DIV: 26.
static const clock_t _clock_i2c[] = {
	{ CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_I2C1, CLK_L_I2C1, 0, 19 }, //20.4MHz -> 100KHz
	{ CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_CLK_SOURCE_I2C2, CLK_H_I2C2, 0, 4  }, //81.6MHz -> 400KHz
	{ CLK_RST_CONTROLLER_RST_DEVICES_U, CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_CONTROLLER_CLK_SOURCE_I2C3, CLK_U_I2C3, 0, 4  }, //81.6MHz -> 400KHz
	{ CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_CONTROLLER_CLK_SOURCE_I2C4, CLK_V_I2C4, 0, 19 }, //20.4MHz -> 100KHz
	{ CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_CLK_SOURCE_I2C5, CLK_H_I2C5, 0, 4  }, //81.6MHz -> 400KHz
	{ CLK_RST_CONTROLLER_RST_DEVICES_X, CLK_RST_CONTROLLER_CLK_OUT_ENB_X, CLK_RST_CONTROLLER_CLK_SOURCE_I2C6, CLK_X_I2C6, 0, 19 }  //20.4MHz -> 100KHz
};

static clock_t _clock_se = {
	CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_CONTROLLER_CLK_SOURCE_SE,     CLK_V_SE,       0, 0 // 408MHz.
};

static clock_t _clock_tzram = {
	CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_NO_SOURCE,                        CLK_V_TZRAM,    0, 0
};

static clock_t _clock_host1x = {
	CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X, CLK_L_HOST1X,   4, 3 // 163.2MHz.
};
static clock_t _clock_tsec = {
	CLK_RST_CONTROLLER_RST_DEVICES_U, CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_CONTROLLER_CLK_SOURCE_TSEC,   CLK_U_TSEC,     0, 2 // 204MHz.
};
static clock_t _clock_sor_safe = {
	CLK_RST_CONTROLLER_RST_DEVICES_Y, CLK_RST_CONTROLLER_CLK_OUT_ENB_Y, CLK_NO_SOURCE,                        CLK_Y_SOR_SAFE, 0, 0
};
static clock_t _clock_sor0 = {
	CLK_RST_CONTROLLER_RST_DEVICES_X, CLK_RST_CONTROLLER_CLK_OUT_ENB_X, CLK_NOT_USED,                         CLK_X_SOR0,     0, 0
};
static clock_t _clock_sor1 = {
	CLK_RST_CONTROLLER_RST_DEVICES_X, CLK_RST_CONTROLLER_CLK_OUT_ENB_X, CLK_RST_CONTROLLER_CLK_SOURCE_SOR1,   CLK_X_SOR1,     0, 2 //204MHz.
};
static clock_t _clock_kfuse = {
	CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_NO_SOURCE,                        CLK_H_KFUSE,    0, 0
};

static clock_t _clock_cl_dvfs =	{
	CLK_RST_CONTROLLER_RST_DEVICES_W, CLK_RST_CONTROLLER_CLK_OUT_ENB_W, CLK_NO_SOURCE,                        CLK_W_DVFS,     0, 0
};
static clock_t _clock_coresight = {
	CLK_RST_CONTROLLER_RST_DEVICES_U, CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_CONTROLLER_CLK_SOURCE_CSITE,  CLK_U_CSITE,    0, 4 // 136MHz.
};

static clock_t _clock_pwm = {
	CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_PWM,    CLK_L_PWM,      6, 4 // Fref: 6.4MHz. HOS: PLLP / 54 = 7.55MHz.
};

static clock_t _clock_sdmmc_legacy_tm = {
	CLK_RST_CONTROLLER_RST_DEVICES_Y, CLK_RST_CONTROLLER_CLK_OUT_ENB_Y, CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC_LEGACY_TM, CLK_Y_SDMMC_LEGACY_TM, 4, 66
};

void clock_enable(const clock_t *clk)
{
	// Put clock into reset.
	CLOCK(clk->reset) = (CLOCK(clk->reset) & ~BIT(clk->index)) | BIT(clk->index);
	// Disable.
	CLOCK(clk->enable) &= ~BIT(clk->index);
	// Configure clock source if required.
	if (clk->source)
		CLOCK(clk->source) = clk->clk_div | (clk->clk_src << 29);
	// Enable.
	CLOCK(clk->enable) = (CLOCK(clk->enable) & ~BIT(clk->index)) | BIT(clk->index);
	usleep(2);

	// Take clock off reset.
	CLOCK(clk->reset) &= ~BIT(clk->index);
}

void clock_disable(const clock_t *clk)
{
	// Put clock into reset.
	CLOCK(clk->reset) = (CLOCK(clk->reset) & ~BIT(clk->index)) | BIT(clk->index);
	// Disable.
	CLOCK(clk->enable) &= ~BIT(clk->index);
}

void clock_enable_fuse(bool enable)
{
	// Enable Fuse registers visibility.
	CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) = (CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) & 0xEFFFFFFF) | ((enable & 1) << 28);
}

void clock_enable_uart(u32 idx)
{
	clock_enable(&_clock_uart[idx]);
}

void clock_disable_uart(u32 idx)
{
	clock_disable(&_clock_uart[idx]);
}

#define UART_SRC_CLK_DIV_EN BIT(24)

int clock_uart_use_src_div(u32 idx, u32 baud)
{
	u32 clk_src_div = CLOCK(_clock_uart[idx].source) & 0xE0000000;

	if (baud == 1000000)
		CLOCK(_clock_uart[idx].source) = clk_src_div | UART_SRC_CLK_DIV_EN | 49;
	else
	{
		CLOCK(_clock_uart[idx].source) = clk_src_div | 2;

		return 1;
	}

	return 0;
}

void clock_enable_i2c(u32 idx)
{
	clock_enable(&_clock_i2c[idx]);
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
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SE) |= 0x100;
}

void clock_enable_tzram()
{
	clock_enable(&_clock_tzram);
}

void clock_enable_host1x()
{
	clock_enable(&_clock_host1x);
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
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_SET) = BIT(CLK_H_KFUSE);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_CLR) = BIT(CLK_H_KFUSE);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = BIT(CLK_H_KFUSE);
	usleep(10); // Wait 10s to prevent glitching.

	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_CLR) = BIT(CLK_H_KFUSE);
	usleep(20); // Wait 20s fo kfuse hw to init.
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
	clock_enable(&_clock_pwm);
}

void clock_disable_pwm()
{
	clock_disable(&_clock_pwm);
}

void clock_enable_pllx()
{
	// Configure and enable PLLX if disabled.
	if (!(CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) & PLLX_BASE_ENABLE)) // PLLX_ENABLE.
	{
		CLOCK(CLK_RST_CONTROLLER_PLLX_MISC_3) &= ~PLLX_MISC3_IDDQ; // Disable IDDQ.
		usleep(2);

		// Set div configuration.
		const u32 pllx_div_cfg = (2 << 20) | (156 << 8) | 2; // P div: 2 (3), N div: 156, M div: 2. 998.4 MHz.

		// Bypass dividers.
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = PLLX_BASE_BYPASS | pllx_div_cfg;
		// Disable bypass
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = pllx_div_cfg;
		// Set PLLX_LOCK_ENABLE.
		CLOCK(CLK_RST_CONTROLLER_PLLX_MISC) |= PLLX_MISC_LOCK_EN;
		// Enable PLLX.
		CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) = PLLX_BASE_ENABLE | pllx_div_cfg;
	}

	// Wait for PLL to stabilize.
	while (!(CLOCK(CLK_RST_CONTROLLER_PLLX_BASE) & PLLX_BASE_LOCK))
		;
}

void clock_enable_pllc(u32 divn)
{
	u8 pll_divn_curr = (CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) >> 10) & 0xFF;

	// Check if already enabled and configured.
	if ((CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) & PLLCX_BASE_ENABLE) && (pll_divn_curr == divn))
		return;

	// Take PLLC out of reset and set basic misc parameters.
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC) =
		((CLOCK(CLK_RST_CONTROLLER_PLLC_MISC) & 0xFFF0000F) & ~PLLC_MISC_RESET) | (0x80000 << 4); // PLLC_EXT_FRU.
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC_2) |= 0xF0 << 8; // PLLC_FLL_LD_MEM.

	// Disable PLL and IDDQ in case they are on.
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE)   &= ~PLLCX_BASE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC_1) &= ~PLLC_MISC1_IDDQ;
	usleep(10);

	// Set PLLC dividers.
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) = (divn << 10) | 4; // DIVM: 4, DIVP: 1.

	// Enable PLLC and wait for Phase and Frequency lock.
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) |= PLLCX_BASE_ENABLE;
	while (!(CLOCK(CLK_RST_CONTROLLER_PLLC_BASE) & PLLCX_BASE_LOCK))
		;

	// Disable PLLC_OUT1, enable reset and set div to 1.5.
	CLOCK(CLK_RST_CONTROLLER_PLLC_OUT) = BIT(8);

	// Enable PLLC_OUT1 and bring it out of reset.
	CLOCK(CLK_RST_CONTROLLER_PLLC_OUT) |= (PLLC_OUT1_CLKEN | PLLC_OUT1_RSTN_CLR);
	msleep(1); // Wait a bit for PLL to stabilize.
}

void clock_disable_pllc()
{
	// Disable PLLC and PLLC_OUT1.
	CLOCK(CLK_RST_CONTROLLER_PLLC_OUT)    &= ~(PLLC_OUT1_CLKEN | PLLC_OUT1_RSTN_CLR);
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE)   &= ~PLLCX_BASE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLC_BASE)   |= PLLCX_BASE_REF_DIS;
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC_1) |= PLLC_MISC1_IDDQ;
	CLOCK(CLK_RST_CONTROLLER_PLLC_MISC)   |= PLLC_MISC_RESET;
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
	//CLOCK(CLK_RST_CONTROLLER_PLLC4_MISC) = PLLC4_MISC_EN_LCKDET;

	// Disable PLL and IDDQ in case they are on.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLLCX_BASE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLLC4_BASE_IDDQ;
	usleep(10);

	// Set PLLC4 dividers.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) = (104 << 8) | 4; // DIVM: 4, DIVP: 1.

	// Enable PLLC4 and wait for Phase and Frequency lock.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) |= PLLCX_BASE_ENABLE;
	while (!(CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) & PLLCX_BASE_LOCK))
		;

	msleep(1); // Wait a bit for PLL to stabilize.

	pllc4_enabled |= PLLC4_ENABLED;
}

static void _clock_disable_pllc4(u32 mask)
{
	pllc4_enabled &= ~mask;

	// Check if currently in use or disabled.
	if ((pllc4_enabled & PLLC4_IN_USE) || !(pllc4_enabled & PLLC4_ENABLED))
		return;

	// Disable PLLC4.
	msleep(1); // Wait at least 1ms to prevent glitching.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLLCX_BASE_ENABLE;
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
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) = pllu_cfg | PLLCX_BASE_ENABLE; // Enable.

	// Wait for PLL to stabilize.
	u32 timeout = (u32)TMR(TIMERUS_CNTR_1US) + 1300;
	while (!(CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) & PLLCX_BASE_LOCK)) // PLL_LOCK.
		if ((u32)TMR(TIMERUS_CNTR_1US) > timeout)
			break;
	usleep(10);

	// Enable PLLU USB/HSIC/ICUSB/48M.
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) |= 0x2E00000;
}

void clock_disable_pllu()
{
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) &= ~0x2E00000;  // Disable PLLU USB/HSIC/ICUSB/48M.
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) &= ~0x40000000; // Disable PLLU.
	CLOCK(CLK_RST_CONTROLLER_PLLU_MISC) &= ~0x20000000; // Enable reference clock.
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

static int _clock_sdmmc_is_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & BIT(CLK_L_SDMMC1);
	case SDMMC_2:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & BIT(CLK_L_SDMMC2);
	case SDMMC_3:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_U) & BIT(CLK_U_SDMMC3);
	case SDMMC_4:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & BIT(CLK_L_SDMMC4);
	}
	return 0;
}

static void _clock_sdmmc_set_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_SDMMC1);
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_SDMMC2);
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_U_SET) = BIT(CLK_U_SDMMC3);
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_SDMMC4);
		break;
	}
}

static void _clock_sdmmc_clear_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = BIT(CLK_L_SDMMC1);
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = BIT(CLK_L_SDMMC2);
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_U_CLR) = BIT(CLK_U_SDMMC3);
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = BIT(CLK_L_SDMMC4);
		break;
	}
}

static int _clock_sdmmc_is_enabled(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & BIT(CLK_L_SDMMC1);
	case SDMMC_2:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & BIT(CLK_L_SDMMC2);
	case SDMMC_3:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_U) & BIT(CLK_U_SDMMC3);
	case SDMMC_4:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & BIT(CLK_L_SDMMC4);
	}
	return 0;
}

static void _clock_sdmmc_set_enable(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = BIT(CLK_L_SDMMC1);
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = BIT(CLK_L_SDMMC2);
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_U_SET) = BIT(CLK_U_SDMMC3);
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = BIT(CLK_L_SDMMC4);
		break;
	}
}

static void _clock_sdmmc_clear_enable(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = BIT(CLK_L_SDMMC1);
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = BIT(CLK_L_SDMMC2);
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_U_CLR) = BIT(CLK_U_SDMMC3);
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = BIT(CLK_L_SDMMC4);
		break;
	}
}

static void _clock_sdmmc_config_legacy_tm()
{
	clock_t *clk = &_clock_sdmmc_legacy_tm;
	if (!(CLOCK(clk->enable) & BIT(clk->index)))
		clock_enable(clk);
}

typedef struct _clock_sdmmc_t
{
	u32 clock;
	u32 real_clock;
} clock_sdmmc_t;

static clock_sdmmc_t _clock_sdmmc_table[4] = { 0 };

#define SDMMC_CLOCK_SRC_PLLP_OUT0      0x0
#define SDMMC_CLOCK_SRC_PLLC4_OUT2     0x3
#define SDMMC4_CLOCK_SRC_PLLC4_OUT2_LJ 0x1

static int _clock_sdmmc_config_clock_host(u32 *pclock, u32 id, u32 val)
{
	u32 divisor = 0;
	u32 source = SDMMC_CLOCK_SRC_PLLP_OUT0;

	if (id > SDMMC_4)
		return 0;

	// Get IO clock divisor.
	switch (val)
	{
	case 25000:
		*pclock = 24728;
		divisor = 31; // 16.5 div.
		break;
	case 26000:
		*pclock = 25500;
		divisor = 30; // 16 div.
		break;
	case 40800:
		*pclock = 40800;
		divisor = 18; // 10 div.
		break;
	case 50000:
		*pclock = 48000;
		divisor = 15; // 8.5 div.
		break;
	case 52000:
		*pclock = 51000;
		divisor = 14; // 8 div.
		break;
	case 100000:
		source = SDMMC_CLOCK_SRC_PLLC4_OUT2;
		*pclock = 99840;
		divisor = 2;  // 2 div.
		break;
	case 164000:
		*pclock = 163200;
		divisor = 3;  // 2.5 div.
		break;
	case 200000: // 240MHz evo+.
		switch (id)
		{
		case SDMMC_1:
			source = SDMMC_CLOCK_SRC_PLLC4_OUT2;
			break;
		case SDMMC_2:
			source = SDMMC4_CLOCK_SRC_PLLC4_OUT2_LJ;
			break;
		case SDMMC_3:
			source = SDMMC_CLOCK_SRC_PLLC4_OUT2;
			break;
		case SDMMC_4:
			source = SDMMC4_CLOCK_SRC_PLLC4_OUT2_LJ;
			break;
		}
		*pclock = 199680;
		divisor = 0;  // 1 div.
		break;
	default:
		*pclock = 24728;
		divisor = 31; // 16.5 div.
	}

	_clock_sdmmc_table[id].clock = val;
	_clock_sdmmc_table[id].real_clock = *pclock;

	// Enable PLLC4 if in use by any SDMMC.
	if (source)
		_clock_enable_pllc4(BIT(id));

	// Set SDMMC legacy timeout clock.
	_clock_sdmmc_config_legacy_tm();

	// Set SDMMC clock.
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1) = (source << 29) | divisor;
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC2) = (source << 29) | divisor;
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3) = (source << 29) | divisor;
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4) = (source << 29) | divisor;
		break;
	}

	return 1;
}

void clock_sdmmc_config_clock_source(u32 *pclock, u32 id, u32 val)
{
	if (_clock_sdmmc_table[id].clock == val)
	{
		*pclock = _clock_sdmmc_table[id].real_clock;
	}
	else
	{
		int is_enabled = _clock_sdmmc_is_enabled(id);
		if (is_enabled)
			_clock_sdmmc_clear_enable(id);
		_clock_sdmmc_config_clock_host(pclock, id, val);
		if (is_enabled)
			_clock_sdmmc_set_enable(id);
		_clock_sdmmc_is_reset(id);
	}
}

void clock_sdmmc_get_card_clock_div(u32 *pclock, u16 *pdivisor, u32 type)
{
	// Get Card clock divisor.
	switch (type)
	{
	case SDHCI_TIMING_MMC_ID: // Actual IO Freq: 380.59 KHz.
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
	case SDHCI_TIMING_SD_ID: // Actual IO Freq: 380.43 KHz.
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
	case SDHCI_TIMING_UHS_DDR50:
		*pclock = 40800;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_MMC_HS102: // Actual IO Freq: 99.84 MHz.
		*pclock = 200000;
		*pdivisor = 2;
		break;
	}
}

int clock_sdmmc_is_not_reset_and_enabled(u32 id)
{
	return !_clock_sdmmc_is_reset(id) && _clock_sdmmc_is_enabled(id);
}

void clock_sdmmc_enable(u32 id, u32 val)
{
	u32 clock = 0;

	if (_clock_sdmmc_is_enabled(id))
		_clock_sdmmc_clear_enable(id);
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_config_clock_host(&clock, id, val);
	_clock_sdmmc_set_enable(id);
	_clock_sdmmc_is_reset(id);
	usleep((100000 + clock - 1) / clock);
	_clock_sdmmc_clear_reset(id);
	_clock_sdmmc_is_reset(id);
}

void clock_sdmmc_disable(u32 id)
{
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_clear_enable(id);
	_clock_sdmmc_is_reset(id);
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

