/*
 * Ambient light sensor driver for Nintendo Switch's Rohm BH1730
 *
 * Copyright (c) 2018 CTCaer
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

#include "als.h"
#include <power/max7762x.h>
#include <soc/clock.h>
#include <soc/i2c.h>
#include <soc/pinmux.h>
#include <utils/util.h>

#define BH1730_DEFAULT_GAIN        BH1730_GAIN_64X
#define BH1730_DEFAULT_ICYCLE       38

#define BH1730_INTERNAL_CLOCK_NS   2800
#define BH1730_ADC_CALC_DELAY_US   2000 /* BH1730_INTERNAL_CLOCK_MS * 714 */
#define BH1730_ITIME_CYCLE_TO_US   2700 /* BH1730_INTERNAL_CLOCK_MS * 964 */

#define BH1730_DEFAULT_ITIME_MS	   100

#define BH1730_LUX_MULTIPLIER      3600
#define BH1730_LUX_MULTIPLIER_AULA 1410

#define BH1730_LUX_MAX            100000

typedef struct _opt_win_cal_t
{
	u32 rc;
	u32 cv;
	u32 ci;
} opt_win_cal_t;

// Nintendo Switch Icosa/Iowa Optical Window calibration.
const opt_win_cal_t opt_win_cal_default[] = {
	{  500, 5002, 7502 },
	{  754, 2250, 2000 },
	{ 1029, 1999, 1667 },
	{ 1373,  884,  583 },
	{ 1879,  309,  165 }
};

// Nintendo Switch Aula Optical Window calibration.
const opt_win_cal_t opt_win_cal_aula[] = {
	{  231, 9697, 30300 },
	{  993, 3333,  2778 },
	{ 1478, 1621,  1053 },
	{ 7500,   81,    10 }
};

const u32 als_gain_idx_tbl[4] = { 1, 2, 64, 128 };

void set_als_cfg(als_ctxt_t *als_ctxt, u8 gain, u8 cycle)
{
	if (gain > BH1730_GAIN_128X)
		gain = BH1730_GAIN_128X;

	if (!cycle)
		cycle = 1;
	else if (cycle > 255)
		cycle = 255;

	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_GAIN_REG), gain);
	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_TIMING_REG), (256 - cycle));

	als_ctxt->gain = gain;
	als_ctxt->cycle = cycle;
}

void get_als_lux(als_ctxt_t *als_ctxt)
{
	u32 data[2];
	u32 visible_light;
	u32 ir_light;
	u64 lux = 0;
	u32 itime_us = BH1730_ITIME_CYCLE_TO_US * als_ctxt->cycle;

	// Get visible and ir light raw data. Mode is continuous so waiting for new values doesn't matter.
	data[0] = i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_DATA0LOW_REG)) +
		(i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_DATA0HIGH_REG)) << 8);
	data[1] = i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_DATA1LOW_REG)) +
		(i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_DATA1HIGH_REG)) << 8);

	visible_light = data[0];
	ir_light = data[1];

	als_ctxt->over_limit = visible_light > 65534 || ir_light > 65534;
	als_ctxt->vi_light = visible_light;
	als_ctxt->ir_light = ir_light;

	if (!visible_light)
	{
		als_ctxt->lux = 0;

		return;
	}

	// Set calibration parameters.
	u32 lux_multiplier = BH1730_LUX_MULTIPLIER;
	u32 opt_win_cal_count = ARRAY_SIZE(opt_win_cal_default);
	const opt_win_cal_t *opt_win_cal = opt_win_cal_default;

	// Apply optical window calibration coefficients.
	for (u32 i = 0; i < opt_win_cal_count; i++)
	{
		if (1000 * ir_light / visible_light < opt_win_cal[i].rc)
		{
			lux = ((u64)opt_win_cal[i].cv * data[0]) - (opt_win_cal[i].ci * data[1]);
			break;
		}
	}

	lux *= BH1730_DEFAULT_ITIME_MS * lux_multiplier;
	lux /= als_gain_idx_tbl[als_ctxt->gain] * itime_us;
	lux /= 1000;

	if (lux > BH1730_LUX_MAX)
		lux = BH1730_LUX_MAX;

	als_ctxt->lux = lux;
}

u8 als_power_on(als_ctxt_t *als_ctxt)
{
	// Enable power to ALS IC.
	max7762x_regulator_set_voltage(REGULATOR_LDO6, 2900000);
	max7762x_regulator_enable(REGULATOR_LDO6, true);

	// Init I2C2.
	pinmux_config_i2c(I2C_2);
	clock_enable_i2c(I2C_2);
	i2c_init(I2C_2);

	// Initialize ALS.
	u8 id = i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(0x12));
	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_SPEC(BH1730_SPECCMD_RESET), 0);

	set_als_cfg(als_ctxt, BH1730_DEFAULT_GAIN, BH1730_DEFAULT_ICYCLE);

	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_CONTROL_REG), BH1730_CTL_POWER_ON | BH1730_CTL_ADC_EN);

	return id;
}
