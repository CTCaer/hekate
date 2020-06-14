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
#include <power/max77620.h>
#include <soc/clock.h>
#include <soc/i2c.h>
#include <soc/pinmux.h>
#include <utils/util.h>

#define HOS_GAIN  BH1730_GAIN_64X
#define HOS_ITIME 38

void set_als_cfg(als_table_t *als_val, u8 gain, u8 itime)
{
	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_GAIN_REG), gain);
	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_TIMING_REG), (256 - itime));

	als_val->gain = gain;
	als_val->itime = itime;
}

void get_als_lux(als_table_t *als_val)
{
	u32 data[2];
	float pre_gain_lux;
	float visible_light;
	float ir_light;
	float light_ratio;

	u8 adc_ready = 0;
	u8 retries = 100;

	const float als_gain_idx_tbl[4] = { 1.0, 2.0, 64.0, 128.0 };
	const float als_norm_res = 100.0;
	const float als_multiplier = 3.6;
	const float als_tint = 2.7;

	// Wait for ADC to prepare new data.
	while (!(adc_ready & BH1730_CTL_ADC_VALID) && retries)
	{
		retries--;
		adc_ready = i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_CONTROL_REG));
	}

	// Get visible and ir light raw data.
	data[0] = i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_DATA0LOW_REG)) +
		(i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_DATA0HIGH_REG)) << 8);
	data[1] = i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_DATA1LOW_REG)) +
		(i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_DATA1HIGH_REG)) << 8);

	als_val->over_limit = data[0] > 65534 || data[1] > 65534;
	als_val->vi_light = data[0];
	als_val->ir_light = data[1];

	if (!data[0] || !retries)
	{
		als_val->lux = 0.0;

		return;
	}

	visible_light = (float)data[0];
	ir_light = (float)data[1];
	light_ratio = (float)data[1] / (float)data[0];

	// The following are specific to the light filter Switch uses.
	if (light_ratio < 0.5)
		pre_gain_lux = visible_light * 5.002 - ir_light * 7.502;
	else if (light_ratio < 0.754)
		pre_gain_lux = visible_light * 2.250 - ir_light * 2.000;
	else if (light_ratio < 1.029)
		pre_gain_lux = visible_light * 1.999 - ir_light * 1.667;
	else if (light_ratio < 1.373)
		pre_gain_lux = visible_light * 0.884 - ir_light * 0.583;
	else if (light_ratio < 1.879)
		pre_gain_lux = visible_light * 0.309 - ir_light * 0.165;
	else pre_gain_lux = 0.0;

	als_val->lux = (pre_gain_lux / als_gain_idx_tbl[als_val->gain]) * (als_norm_res / ((float)als_val->itime * als_tint)) * als_multiplier;
}

u8 als_init(als_table_t *als_val)
{
	pinmux_config_i2c(I2C_2);
	clock_enable_i2c(I2C_2);
	i2c_init(I2C_2);

	max77620_regulator_set_volt_and_flags(REGULATOR_LDO6, 2900000, MAX77620_POWER_MODE_NORMAL);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_LDO6_CFG2, 0xD8 | MAX77620_LDO_CFG2_ADE_MASK);

	u8 id = i2c_recv_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(0x12));
	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_SPEC(BH1730_SPECCMD_RESET), 0);
	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_GAIN_REG), HOS_GAIN);
	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_TIMING_REG), (256 - HOS_ITIME));
	i2c_send_byte(I2C_2, BH1730_I2C_ADDR, BH1730_ADDR(BH1730_CONTROL_REG), BH1730_CTL_POWER_ON | BH1730_CTL_ADC_EN);

	als_val->gain = HOS_GAIN;
	als_val->itime = HOS_ITIME;

	return id;
}
