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

#include "../soc/clock.h"
#include "../soc/t210.h"
#include "../utils/util.h"
#include "../storage/sdmmc.h"

static const clk_desc_t _clock_uart[] = {
	/* UART A */ { CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_UARTA, 6, 0, 0 },
	/* UART B */ { CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_UARTB, 7, 0, 0 },
	/* UART C */ { CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_CLK_SOURCE_UARTC, 0x17, 0, 0 },
	/* UART D */ { 0 },
	/* UART E */ { 0 }
};

static const clk_desc_t _clock_i2c[] = {
	/* I2C1 */ { CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_I2C1, 0xC, 6, 0 },
	/* I2C2 */ { 0 },
	/* I2C3 */ { 0 },
	/* I2C4 */ { 0 },
	/* I2C5 */ { CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_CLK_SOURCE_I2C5, 0xF, 6, 0 },
	/* I2C6 */ { 0 }
};

static clk_desc_t _clock_se = { CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_CONTROLLER_CLK_SOURCE_SE, 0x1F, 0, 0 };
static clk_desc_t _clock_unk2 = { CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_CONTROLLER_RST_SOURCE, 0x1E, 0, 0 };

static clk_desc_t _clock_host1x = { CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X, 0x1C, 4, 3 };
static clk_desc_t _clk_desc_tsec = { CLK_RST_CONTROLLER_RST_DEVICES_U, CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_CONTROLLER_CLK_SOURCE_TSEC, 0x13, 0, 2 };
static clk_desc_t _clock_sor_safe = { CLK_RST_CONTROLLER_RST_DEVICES_Y, CLK_RST_CONTROLLER_CLK_OUT_ENB_Y, CLK_RST_CONTROLLER_RST_SOURCE, 0x1E, 0, 0 };
static clk_desc_t _clock_sor0 = { CLK_RST_CONTROLLER_RST_DEVICES_X, CLK_RST_CONTROLLER_CLK_OUT_ENB_X, CLK_RST_CONTROLLER_RST_SOURCE, 0x16, 0, 0 };
static clk_desc_t _clock_sor1 = { CLK_RST_CONTROLLER_RST_DEVICES_X, CLK_RST_CONTROLLER_CLK_OUT_ENB_X, CLK_RST_CONTROLLER_CLK_SOURCE_SOR1, 0x17, 0, 2 };
static clk_desc_t _clock_kfuse = { CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_RST_SOURCE, 8, 0, 0 };

static clk_desc_t _clock_cl_dvfs = { CLK_RST_CONTROLLER_RST_DEVICES_W, CLK_RST_CONTROLLER_CLK_OUT_ENB_W, CLK_RST_CONTROLLER_RST_SOURCE, 0x1B, 0, 0 };
static clk_desc_t _clock_coresight = { CLK_RST_CONTROLLER_RST_DEVICES_U, CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_CONTROLLER_CLK_SOURCE_CSITE, 9, 0, 4};

static clk_desc_t _clock_pwm = { CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_CONTROLLER_CLK_SOURCE_PWM, 0x11, 6, 4};

void clock_enable(const clk_desc_t *clk)
{
	// Put clock into reset.
	CLOCK(clk->reset) = (CLOCK(clk->reset) & ~(1 << clk->index)) | (1 << clk->index);
	// Disable.
	CLOCK(clk->enable) &= ~(1 << clk->index);
	// Configure clock source if required.
	if (clk->source)
		CLOCK(clk->source) = clk->clk_div | (clk->clk_src << 29);
	// Enable.
	CLOCK(clk->enable) = (CLOCK(clk->enable) & ~(1 << clk->index)) | (1 << clk->index);
	// Take clock off reset.
	CLOCK(clk->reset) &= ~(1 << clk->index);
}

void clock_disable(const clk_desc_t *clk)
{
	// Put clock into reset.
	CLOCK(clk->reset) = (CLOCK(clk->reset) & ~(1 << clk->index)) | (1 << clk->index);
	// Disable.
	CLOCK(clk->enable) &= ~(1 << clk->index);
}

void clock_enable_fuse(bool enable)
{
	CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) = (CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) & 0xEFFFFFFF) | ((enable & 1) << 28);
}

void clock_enable_uart(u32 idx)
{
	clock_enable(&_clock_uart[idx]);
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
}

void clock_enable_unk2()
{
	clock_enable(&_clock_unk2);
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
	clock_enable(&_clk_desc_tsec);
}

void clock_disable_tsec()
{
	clock_disable(&_clk_desc_tsec);
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
	//clock_enable(&_clock_kfuse);
	CLOCK(0x8) = (CLOCK(0x8) & 0xFFFFFEFF) | 0x100;
	CLOCK(0x14) &= 0xFFFFFEFF;
	CLOCK(0x14) = (CLOCK(0x14) & 0xFFFFFEFF) | 0x100;
	usleep(10);
	CLOCK(0x8) &= 0xFFFFFEFF;
	usleep(20);
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

#define L_SWR_SDMMC1_RST (1 << 14)
#define L_SWR_SDMMC2_RST (1 << 9)
#define L_SWR_SDMMC4_RST (1 << 15)
#define U_SWR_SDMMC3_RST (1 << 5)

#define L_CLK_ENB_SDMMC1 (1 << 14)
#define L_CLK_ENB_SDMMC2 (1 << 9)
#define L_CLK_ENB_SDMMC4 (1 << 15)
#define U_CLK_ENB_SDMMC3 (1 << 5)

#define L_SET_SDMMC1_RST (1 << 14)
#define L_SET_SDMMC2_RST (1 << 9)
#define L_SET_SDMMC4_RST (1 << 15)
#define U_SET_SDMMC3_RST (1 << 5)

#define L_CLR_SDMMC1_RST (1 << 14)
#define L_CLR_SDMMC2_RST (1 << 9)
#define L_CLR_SDMMC4_RST (1 << 15)
#define U_CLR_SDMMC3_RST (1 << 5)

#define L_SET_CLK_ENB_SDMMC1 (1 << 14)
#define L_SET_CLK_ENB_SDMMC2 (1 << 9)
#define L_SET_CLK_ENB_SDMMC4 (1 << 15)
#define U_SET_CLK_ENB_SDMMC3 (1 << 5)

#define L_CLR_CLK_ENB_SDMMC1 (1 << 14)
#define L_CLR_CLK_ENB_SDMMC2 (1 << 9)
#define L_CLR_CLK_ENB_SDMMC4 (1 << 15)
#define U_CLR_CLK_ENB_SDMMC3 (1 << 5)

static int _clock_sdmmc_is_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & L_SWR_SDMMC1_RST;
	case SDMMC_2:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & L_SWR_SDMMC2_RST;
	case SDMMC_3:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_U) & U_SWR_SDMMC3_RST;
	case SDMMC_4:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & L_SWR_SDMMC4_RST;
	}
	return 0;
}

static void _clock_sdmmc_set_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = L_SET_SDMMC1_RST;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = L_SET_SDMMC2_RST;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_U_SET) = U_SET_SDMMC3_RST;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = L_SET_SDMMC4_RST;
	}
}

static void _clock_sdmmc_clear_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = L_CLR_SDMMC1_RST;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = L_CLR_SDMMC2_RST;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_U_CLR) = U_CLR_SDMMC3_RST;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = L_CLR_SDMMC4_RST;
	}
}

static int _clock_sdmmc_is_enabled(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & L_CLK_ENB_SDMMC1;
	case SDMMC_2:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & L_CLK_ENB_SDMMC2;
	case SDMMC_3:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_U) & U_CLK_ENB_SDMMC3;
	case SDMMC_4:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & L_CLK_ENB_SDMMC4;
	}
	return 0;
}

static void _clock_sdmmc_set_enable(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = L_SET_CLK_ENB_SDMMC1;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = L_SET_CLK_ENB_SDMMC2;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_U_SET) = U_SET_CLK_ENB_SDMMC3;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = L_SET_CLK_ENB_SDMMC4;
	}
}

static void _clock_sdmmc_clear_enable(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = L_CLR_CLK_ENB_SDMMC1;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = L_CLR_CLK_ENB_SDMMC2;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_U_CLR) = U_CLR_CLK_ENB_SDMMC3;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = L_CLR_CLK_ENB_SDMMC4;
	}
}

static u32 _clock_sdmmc_table[8] = { 0 };

static int _clock_sdmmc_config_clock_source_inner(u32 *pout, u32 id, u32 val)
{
	u32 divisor = 0;
	u32 source = 0;

	switch (val)
	{
	case 25000:
		*pout = 24728;
		divisor = 31;
		break;
	case 26000:
		*pout = 25500;
		divisor = 30;
		break;
	case 40800:
		*pout = 40800;
		divisor = 18;
		break;
	case 50000:
		*pout = 48000;
		divisor = 15;
		break;
	case 52000:
		*pout = 51000;
		divisor = 14;
		break;
	case 100000:
		*pout = 90667;
		divisor = 7;
		break;
	case 200000:
		*pout = 163200;
		divisor = 3;
		break;
	case 208000:
		*pout = 204000;
		divisor = 2;
		break;
	default:
		*pout = 24728;
		divisor = 31;
	}

	_clock_sdmmc_table[2 * id] = val;
	_clock_sdmmc_table[2 * id + 1] = *pout;

	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1) = source | divisor;
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC2) = source | divisor;
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3) = source | divisor;
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4) = source | divisor;
		break;
	}

	return 1;
}

void clock_sdmmc_config_clock_source(u32 *pout, u32 id, u32 val)
{
	if (_clock_sdmmc_table[2 * id] == val)
	{
		*pout = _clock_sdmmc_table[2 * id + 1];
	}
	else
	{
		int is_enabled = _clock_sdmmc_is_enabled(id);
		if (is_enabled)
			_clock_sdmmc_clear_enable(id);
		_clock_sdmmc_config_clock_source_inner(pout, id, val);
		if (is_enabled)
			_clock_sdmmc_set_enable(id);
		_clock_sdmmc_is_reset(id);
	}
}

void clock_sdmmc_get_params(u32 *pout, u16 *pdivisor, u32 type)
{
	switch (type)
	{
	case 0:
		*pout = 26000;
		*pdivisor = 66;
		break;
	case 1:
		*pout = 26000;
		*pdivisor = 1;
		break;
	case 2:
		*pout = 52000;
		*pdivisor = 1;
		break;
	case 3:
	case 4:
	case 11:
		*pout = 200000;
		*pdivisor = 1;
		break;
	case 5:
		*pout = 25000;
		*pdivisor = 64;
	case 6:
	case 8:
		*pout = 25000;
		*pdivisor = 1;
		break;
	case 7:
		*pout = 50000;
		*pdivisor = 1;
	case 10:
		*pout = 100000;
		*pdivisor = 1;
	case 13:
		*pout = 40800;
		*pdivisor = 1;
		break;
	case 14:
		*pout = 200000;
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
	u32 div = 0;

	if (_clock_sdmmc_is_enabled(id))
		_clock_sdmmc_clear_enable(id);
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_config_clock_source_inner(&div, id, val);
	_clock_sdmmc_set_enable(id);
	_clock_sdmmc_is_reset(id);
	usleep((100000 + div - 1) / div);
	_clock_sdmmc_clear_reset(id);
	_clock_sdmmc_is_reset(id);
}

void clock_sdmmc_disable(u32 id)
{
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_clear_enable(id);
	_clock_sdmmc_is_reset(id);
}
