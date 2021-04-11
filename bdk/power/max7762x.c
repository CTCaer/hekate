/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2019-2020 CTCaer
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

#include <power/max7762x.h>
#include <power/max77620.h>
#include <power/max77812.h>
#include <soc/fuse.h>
#include <soc/i2c.h>
#include <soc/t210.h>
#include <utils/util.h>

#define REGULATOR_SD  0
#define REGULATOR_LDO 1
#define REGULATOR_BC0 2
#define REGULATOR_BC1 3

typedef struct _max77620_fps_t
{
	u8 fps_addr;
	u8 fps_src;
	u8 pd_period;
	u8 pu_period;
} max77620_fps_t;

typedef struct _max77621_ctrl_t
{
	u8 ctrl1_por;
	u8 ctrl1_hos;
	u8 ctrl2_por;
	u8 ctrl2_hos;
} max77621_ctrl_t;

typedef struct _max77812_ctrl_t
{
	u8 mask;
	u8 shift;
	u8 rsvd0;
	u8 rsvd1;
} max77812_en_t;

typedef struct _max77620_regulator_t
{
	const char *name;

	u32 uv_step;
	u32 uv_min;
	u32 uv_default;
	u32 uv_max;

	u8 type;
	u8 volt_addr;
	u8 cfg_addr;
	u8 volt_mask;

	union {
		max77620_fps_t fps;
		max77621_ctrl_t ctrl;
		max77812_en_t enable;
	};
} max77620_regulator_t;

static const max77620_regulator_t _pmic_regulators[] = {
	{  "sd0",  12500, 600000,  625000, 1400000,  REGULATOR_SD,  MAX77620_REG_SD0,      MAX77620_REG_SD0_CFG,    MAX77620_SD0_VOLT_MASK,  {{ MAX77620_REG_FPS_SD0,  1, 7, 1 }} },
	{  "sd1",  12500, 600000, 1125000, 1250000,  REGULATOR_SD,  MAX77620_REG_SD1,      MAX77620_REG_SD1_CFG,    MAX77620_SD1_VOLT_MASK,  {{ MAX77620_REG_FPS_SD1,  0, 1, 5 }} },
	{  "sd2",  12500, 600000, 1325000, 1350000,  REGULATOR_SD,  MAX77620_REG_SD2,      MAX77620_REG_SD2_CFG,    MAX77620_SDX_VOLT_MASK,  {{ MAX77620_REG_FPS_SD2,  1, 5, 2 }} },
	{  "sd3",  12500, 600000, 1800000, 1800000,  REGULATOR_SD,  MAX77620_REG_SD3,      MAX77620_REG_SD3_CFG,    MAX77620_SDX_VOLT_MASK,  {{ MAX77620_REG_FPS_SD3,  0, 3, 3 }} },
	{ "ldo0",  25000, 800000, 1200000, 1200000,  REGULATOR_LDO, MAX77620_REG_LDO0_CFG, MAX77620_REG_LDO0_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO0, 3, 7, 0 }} },
	{ "ldo1",  25000, 800000, 1050000, 1050000,  REGULATOR_LDO, MAX77620_REG_LDO1_CFG, MAX77620_REG_LDO1_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO1, 3, 7, 0 }} },
	{ "ldo2",  50000, 800000, 1800000, 3300000,  REGULATOR_LDO, MAX77620_REG_LDO2_CFG, MAX77620_REG_LDO2_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO2, 3, 7, 0 }} },
	{ "ldo3",  50000, 800000, 3100000, 3100000,  REGULATOR_LDO, MAX77620_REG_LDO3_CFG, MAX77620_REG_LDO3_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO3, 3, 7, 0 }} },
	{ "ldo4",  12500, 800000,  850000, 1000000,  REGULATOR_LDO, MAX77620_REG_LDO4_CFG, MAX77620_REG_LDO4_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO4, 0, 7, 1 }} },
	{ "ldo5",  50000, 800000, 1800000, 1800000,  REGULATOR_LDO, MAX77620_REG_LDO5_CFG, MAX77620_REG_LDO5_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO5, 3, 7, 0 }} },
	{ "ldo6",  50000, 800000, 2900000, 2900000,  REGULATOR_LDO, MAX77620_REG_LDO6_CFG, MAX77620_REG_LDO6_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO6, 3, 7, 0 }} },
	{ "ldo7",  50000, 800000, 1050000, 1050000,  REGULATOR_LDO, MAX77620_REG_LDO7_CFG, MAX77620_REG_LDO7_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO7, 1, 4, 3 }} },
	{ "ldo8",  50000, 800000, 1050000, 2800000,  REGULATOR_LDO, MAX77620_REG_LDO8_CFG, MAX77620_REG_LDO8_CFG2,  MAX77620_LDO_VOLT_MASK,  {{ MAX77620_REG_FPS_LDO8, 3, 7, 0 }} },

	{ "max77621_CPU", 6250, 606250, 1000000, 1400000,  REGULATOR_BC0, MAX77621_VOUT_REG,    MAX77621_VOUT_DVS_REG,  MAX77621_DVC_DVS_VOLT_MASK, {{ MAX77621_CPU_CTRL1_POR_DEFAULT, MAX77621_CPU_CTRL1_HOS_DEFAULT, MAX77621_CPU_CTRL2_POR_DEFAULT, MAX77621_CPU_CTRL2_HOS_DEFAULT }} },
	{ "max77621_GPU", 6250, 606250, 1200000, 1400000,  REGULATOR_BC0, MAX77621_VOUT_REG,    MAX77621_VOUT_DVS_REG,  MAX77621_DVC_DVS_VOLT_MASK, {{ MAX77621_CPU_CTRL1_POR_DEFAULT, MAX77621_CPU_CTRL1_HOS_DEFAULT, MAX77621_CPU_CTRL2_POR_DEFAULT, MAX77621_CPU_CTRL2_HOS_DEFAULT }} },
	{ "max77812_CPU", 5000, 250000,  600000, 1525000,  REGULATOR_BC1, MAX77812_REG_M4_VOUT, MAX77812_REG_EN_CTRL,   MAX77812_BUCK_VOLT_MASK,    {{ MAX77812_EN_CTRL_EN_M4_MASK, MAX77812_EN_CTRL_EN_M4_SHIFT, 0, 0 }} },
	//{ "max77812_GPU", 5000, 250000,  600000, 1525000,  REGULATOR_BC1, MAX77812_REG_M1_VOUT, MAX77812_REG_EN_CTRL,   MAX77812_BUCK_VOLT_MASK,    {{ MAX77812_EN_CTRL_EN_M1_MASK, MAX77812_EN_CTRL_EN_M1_SHIFT, 0, 0 }} },
	//{ "max77812_RAM", 5000, 250000,  600000, 1525000,  REGULATOR_BC1, MAX77812_REG_M3_VOUT, MAX77812_REG_EN_CTRL,   MAX77812_BUCK_VOLT_MASK,    {{ MAX77812_EN_CTRL_EN_M3_MASK, MAX77812_EN_CTRL_EN_M3_SHIFT, 0, 0 }} } // Only on PHASE211 configuration.
};

static u8 _max77812_get_address()
{
	static u8 max77812_i2c_addr = 0;

	if (max77812_i2c_addr)
		return max77812_i2c_addr;

	max77812_i2c_addr =
		!(FUSE(FUSE_RESERVED_ODM28_T210B01) & 1) ? MAX77812_PHASE31_CPU_I2C_ADDR : MAX77812_PHASE211_CPU_I2C_ADDR;

	return max77812_i2c_addr;
}

static u8 _max7762x_get_i2c_address(u32 id)
{
	const max77620_regulator_t *reg = &_pmic_regulators[id];

	// Choose the correct i2c address.
	switch (reg->type)
	{
	case REGULATOR_SD:
	case REGULATOR_LDO:
		return MAX77620_I2C_ADDR;
	case REGULATOR_BC0:
		return (id == REGULATOR_CPU0 ? MAX77621_CPU_I2C_ADDR : MAX77621_GPU_I2C_ADDR);
	case REGULATOR_BC1:
		return _max77812_get_address();
	default:
		return 0;
	}
}

static void _max7762x_set_reg(u8 addr, u8 reg, u8 val)
{
	u32 retries = 100;
	while (retries)
	{
		if (i2c_send_byte(I2C_5, addr, reg, val))
			break;

		usleep(50);
		retries--;
	}
}

int max77620_regulator_get_status(u32 id)
{
	if (id > REGULATOR_LDO8)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	// SD power OK status.
	if (reg->type == REGULATOR_SD)
	{
		u8 mask = 1u << (7 - id);
		return (i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_STATSD) & mask) ? 0 : 1;
	}

	// LDO power OK status.
	return (i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, reg->cfg_addr) & MAX77620_LDO_CFG2_POK_MASK) ? 1 : 0;
}

int max77620_regulator_config_fps(u32 id)
{
	if (id > REGULATOR_LDO8)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	// Set FPS configuration.
	_max7762x_set_reg(MAX77620_I2C_ADDR,
		reg->fps.fps_addr,
		(reg->fps.fps_src   << MAX77620_FPS_SRC_SHIFT)       |
		(reg->fps.pu_period << MAX77620_FPS_PU_PERIOD_SHIFT) |
		(reg->fps.pd_period << MAX77620_FPS_PD_PERIOD_SHIFT));

	return 1;
}

int max7762x_regulator_set_voltage(u32 id, u32 mv)
{
	if (id > REGULATOR_MAX)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	if (mv < reg->uv_min || mv > reg->uv_max)
		return 0;

	u8 addr = _max7762x_get_i2c_address(id);

	// Calculate voltage multiplier.
	u32 mult = (mv + reg->uv_step - 1 - reg->uv_min) / reg->uv_step;
	u8 val = i2c_recv_byte(I2C_5, addr, reg->volt_addr);
	val = (val & ~reg->volt_mask) | (mult & reg->volt_mask);

	// Set voltage.
	_max7762x_set_reg(addr, reg->volt_addr, val);

	// If max77621 set DVS voltage also.
	if (reg->type == REGULATOR_BC0)
		_max7762x_set_reg(addr, reg->cfg_addr, MAX77621_VOUT_ENABLE_MASK | val);

	// Wait for ramp up/down delay.
	usleep(1000);

	return 1;
}

int max7762x_regulator_enable(u32 id, bool enable)
{
	u8 reg_addr;
	u8 enable_val;
	u8 enable_mask;
	u8 enable_shift;

	if (id > REGULATOR_MAX)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	// Choose the correct i2c and register addresses and mask/shift for each type.
	switch (reg->type)
	{
	case REGULATOR_SD:
		reg_addr     = reg->cfg_addr;
		enable_val   = MAX77620_POWER_MODE_NORMAL;
		enable_mask  = MAX77620_SD_POWER_MODE_MASK;
		enable_shift = MAX77620_SD_POWER_MODE_SHIFT;
		break;
	case REGULATOR_LDO:
		reg_addr     = reg->volt_addr;
		enable_val   = MAX77620_POWER_MODE_NORMAL;
		enable_mask  = MAX77620_LDO_POWER_MODE_MASK;
		enable_shift = MAX77620_LDO_POWER_MODE_SHIFT;
		break;
	case REGULATOR_BC0:
		reg_addr     = reg->volt_addr;
		enable_val   = MAX77621_VOUT_ENABLE;
		enable_mask  = MAX77621_DVC_DVS_ENABLE_MASK;
		enable_shift = MAX77621_DVC_DVS_ENABLE_SHIFT;
		break;
	case REGULATOR_BC1:
		reg_addr     = reg->cfg_addr;
		enable_val   = MAX77812_EN_CTRL_ENABLE;
		enable_mask  = reg->enable.mask;
		enable_shift = reg->enable.shift;
		break;
	default:
		return 0;
	}

	u8 addr = _max7762x_get_i2c_address(id);

	// Read and enable/disable.
	u8 val = i2c_recv_byte(I2C_5, addr, reg_addr);
	val &= ~enable_mask;

	if (enable)
		val |= (enable_val << enable_shift);

	// Set enable.
	_max7762x_set_reg(addr, reg_addr, val);

	// Wait for enable/disable ramp delay.
	usleep(1000);

	return 1;
}

void max77620_config_gpio(u32 gpio_id, bool enable)
{
	if (gpio_id > 7)
		return;

	// Configure as standard GPIO.
	u8 val = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_AME_GPIO);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_AME_GPIO, val & ~BIT(gpio_id));

	// Set GPIO configuration.
	if (enable)
		val = MAX77620_CNFG_GPIO_OUTPUT_VAL_HIGH | MAX77620_CNFG_GPIO_DIR_OUTPUT | MAX77620_CNFG_GPIO_DRV_PUSHPULL;
	else
		val = MAX77620_CNFG_GPIO_DIR_INPUT | MAX77620_CNFG_GPIO_DRV_OPENDRAIN;
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_GPIO0 + gpio_id, val);
}

void max77621_config_default(u32 id, bool por)
{
	const max77620_regulator_t *reg = &_pmic_regulators[id];

	if (reg->type != REGULATOR_BC0)
		return;

	u8 addr = _max7762x_get_i2c_address(id);

	if (por)
	{
		// Set voltage and disable power before changing the inductor.
		max7762x_regulator_set_voltage(id, 1000000);
		max7762x_regulator_enable(id, false);

		// Configure to default.
		i2c_send_byte(I2C_5, addr, MAX77621_CONTROL1_REG, reg->ctrl.ctrl1_por);
		i2c_send_byte(I2C_5, addr, MAX77621_CONTROL2_REG, reg->ctrl.ctrl2_por);
	}
	else
	{
		i2c_send_byte(I2C_5, addr, MAX77621_CONTROL1_REG, reg->ctrl.ctrl1_hos);
		i2c_send_byte(I2C_5, addr, MAX77621_CONTROL2_REG, reg->ctrl.ctrl2_hos);
	}
}

void max77620_config_default()
{
	// Check if Erista OTP.
	if (i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CID4) != 0x35)
		return;

	// Set default voltages and enable regulators.
	for (u32 i = 1; i <= REGULATOR_LDO8; i++)
	{
		max77620_regulator_config_fps(i);
		max7762x_regulator_set_voltage(i, _pmic_regulators[i].uv_default);
		if (_pmic_regulators[i].fps.fps_src != MAX77620_FPS_SRC_NONE)
			max7762x_regulator_enable(i, true);
	}

	// Enable SD0 output voltage sense and disable for SD1. Additionally disable the reserved bit.
	_max7762x_set_reg(MAX77620_I2C_ADDR, MAX77620_REG_SD_CFG2, MAX77620_SD_CNF2_ROVS_EN_SD0);
}

// Stock HOS: disabled.
void max77620_low_battery_monitor_config(bool enable)
{
	_max7762x_set_reg(MAX77620_I2C_ADDR, MAX77620_REG_CNFGGLBL1,
		MAX77620_CNFGGLBL1_LBDAC_EN             |
		(enable ? MAX77620_CNFGGLBL1_MPPLD : 0) |
		MAX77620_CNFGGLBL1_LBHYST_200           |
		MAX77620_CNFGGLBL1_LBDAC_2800);
}
