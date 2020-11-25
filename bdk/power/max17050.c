/*
 * Fuel gauge driver for Nintendo Switch's Maxim 17050
 *
 * Copyright (c) 2011 Samsung Electronics
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 * Copyright (c) 2018 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max17040_battery.c
 */

#include "max17050.h"
#include <soc/i2c.h>
#include <utils/util.h>

#define BASE_SNS_UOHM 5000

/* Status register bits */
#define STATUS_POR_BIT BIT(1)
#define STATUS_BST_BIT BIT(3)
#define STATUS_VMN_BIT BIT(8)
#define STATUS_TMN_BIT BIT(9)
#define STATUS_SMN_BIT BIT(10)
#define STATUS_BI_BIT  BIT(11)
#define STATUS_VMX_BIT BIT(12)
#define STATUS_TMX_BIT BIT(13)
#define STATUS_SMX_BIT BIT(14)
#define STATUS_BR_BIT  BIT(15)

#define VFSOC0_LOCK   0x0000
#define VFSOC0_UNLOCK 0x0080

#define MAX17050_VMAX_TOLERANCE 50 /* 50 mV */

static u32 battery_voltage = 0;
u32 max17050_get_cached_batt_volt()
{
	return battery_voltage;
}

static u16 max17050_get_reg(u8 reg)
{
	u16 data = 0;

	i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, reg);

	return data;
}

int max17050_get_property(enum MAX17050_reg reg, int *value)
{
	u16 data;

	switch (reg)
	{
	case MAX17050_Age: // Age (percent). Based on 100% x (FullCAP Register/DesignCap).
		data = max17050_get_reg(MAX17050_Age);
		*value = data >> 8; /* Show MSB. 1% increments */
		break;
	case MAX17050_Cycles: // Cycle count.
		*value = max17050_get_reg(MAX17050_Cycles);
		break;
	case MAX17050_MinVolt: // Voltage max/min
		data = max17050_get_reg(MAX17050_MinMaxVolt);
		*value = (data & 0xff) * 20; /* Voltage MIN. Units of 20mV */
		break;
	case MAX17050_MaxVolt: // Voltage max/min
		data = max17050_get_reg(MAX17050_MinMaxVolt);
		*value = (data >> 8) * 20; /* Voltage MAX. Units of LSB = 20mV */
		break;
	case MAX17050_V_empty: // Voltage min design.
		data = max17050_get_reg(MAX17050_V_empty);
		*value = (data >> 7) * 10; /* Units of LSB = 10mV */
		break;
	case MAX17050_VCELL: // Voltage now.
		data = max17050_get_reg(MAX17050_VCELL);
		*value = (data >> 3) * 625 / 1000; /* Units of LSB = 0.625mV */
		battery_voltage = *value;
		break;
	case MAX17050_AvgVCELL: // Voltage avg.
		data = max17050_get_reg(MAX17050_AvgVCELL);
		*value = (data >> 3) * 625 / 1000; /* Units of LSB = 0.625mV */
		break;
	case MAX17050_OCVInternal: // Voltage ocv.
		data = max17050_get_reg(MAX17050_OCVInternal);
		*value = (data >> 3) * 625 / 1000; /* Units of LSB = 0.625mV */
		break;
	case MAX17050_RepSOC: // Capacity %.
		*value = max17050_get_reg(MAX17050_RepSOC);
		break;
	case MAX17050_DesignCap: // Charge full design.
		data = max17050_get_reg(MAX17050_DesignCap);
		*value = data * (BASE_SNS_UOHM / MAX17050_BOARD_SNS_RESISTOR_UOHM) / MAX17050_BOARD_CGAIN;
		break;
	case MAX17050_FullCAP: // Charge full.
		data = max17050_get_reg(MAX17050_FullCAP);
		*value = data * (BASE_SNS_UOHM / MAX17050_BOARD_SNS_RESISTOR_UOHM) / MAX17050_BOARD_CGAIN;
		break;
	case MAX17050_RepCap: // Charge now.
		data = max17050_get_reg(MAX17050_RepCap);
		*value = data * (BASE_SNS_UOHM / MAX17050_BOARD_SNS_RESISTOR_UOHM) / MAX17050_BOARD_CGAIN;
		break;
	case MAX17050_TEMP: // Temp.
		data = max17050_get_reg(MAX17050_TEMP);
		*value = (s16)data;
		*value = *value * 10 / 256;
		break;
	case MAX17050_Current: // Current now.
		data = max17050_get_reg(MAX17050_Current);
		*value = (s16)data;
		*value *= 1562500 / (MAX17050_BOARD_SNS_RESISTOR_UOHM * MAX17050_BOARD_CGAIN);
		break;
	case MAX17050_AvgCurrent: // Current avg.
		data = max17050_get_reg(MAX17050_AvgCurrent);
		*value = (s16)data;
		*value *= 1562500 / (MAX17050_BOARD_SNS_RESISTOR_UOHM * MAX17050_BOARD_CGAIN);
		break;
	default:
		return -1;
	}
	return 0;
}

static int _max17050_write_verify_reg(u8 reg, u16 value)
{
	int retries = 8;
	int ret;
	u16 read_value;

	do
	{
		ret = i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, reg, (u8 *)&value, 2);
		read_value = max17050_get_reg(reg);
		if (read_value != value)
		{
			ret = -1;
			retries--;
		}
	} while (retries && read_value != value);

	return ret;
}

static void _max17050_override_por(u8 reg, u16 value)
{
	if (value)
		i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, reg, (u8 *)&value, 2);
}

static void _max17050_load_new_capacity_params()
{
	u16 fullcap, repSoc, dq_acc, dp_acc;

	fullcap = 0x2476; // 4667mAh design capacity.
	dq_acc = 0x10bc;  // From a healthy fuel gauge.
	dp_acc = 0x5e09;  //          =||=
	repSoc = 0x6400;  // 100%.

	_max17050_write_verify_reg(MAX17050_RemCap, fullcap);
	_max17050_write_verify_reg(MAX17050_RepCap, fullcap);

	_max17050_write_verify_reg(MAX17050_dQacc, dq_acc);
	_max17050_write_verify_reg(MAX17050_dPacc, dp_acc);

	_max17050_write_verify_reg(MAX17050_FullCAP, fullcap);
	//i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_DesignCap, (u8 *)&fullcap, 2);
	_max17050_write_verify_reg(MAX17050_FullCAPNom, fullcap);
	/* Update SOC register with new SOC */
	i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_RepSOC, (u8 *)&repSoc, 2);
}

static void _max17050_reset_vfsoc0_reg()
{
	u16 lockVal = 0;
	u16 vfSoc = 0x6440; // >100% for fully charged battery

	lockVal = VFSOC0_UNLOCK;
	i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_VFSOC0Enable, (u8 *)&lockVal, 2);

	_max17050_write_verify_reg(MAX17050_VFSOC0, vfSoc);

	lockVal = VFSOC0_LOCK;
	i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_VFSOC0Enable, (u8 *)&lockVal, 2);
}

static void _max17050_update_capacity_regs()
{
	u16 value = 0x2476; // Set to 4667mAh design capacity.
	_max17050_write_verify_reg(MAX17050_FullCAP, value);
	_max17050_write_verify_reg(MAX17050_FullCAPNom, value);
	//i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_DesignCap, config->design_cap, 2);
}

static void _max17050_write_config_regs()
{
	u16 value = 0;

	value = 0x7254;
	i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_CONFIG, (u8 *)&value, 2);
	value = 0x2473;
	i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_LearnCFG, (u8 *)&value, 2);
	//i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_FilterCFG, (u8 *)&value, 2)
	//i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_RelaxCFG, (u8 *)&value, 2)
	//i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, MAX17050_FullSOCThr, (u8 *)&value, 2)
}

/*
 * Block write all the override values coming from platform data.
 * This function MUST be called before the POR initialization proceedure
 * specified by maxim.
 */
static void _max17050_override_por_values()
{
	u16 dq_acc = 0x10bc; // From a healthy fuel gauge.
	u16 dp_acc = 0x5e09; //           =||=

	_max17050_override_por(MAX17050_dQacc, dq_acc);
	_max17050_override_por(MAX17050_dPacc, dp_acc);

	//_max17050_override_por(MAX17050_RCOMP0, config->rcomp0);  //0x58
	//_max17050_override_por(MAX17050_TempCo, config->tcompc0); //0x1b22

	//u16 k_empty0 = 0x439;
	//_max17050_override_por(map, MAX17050_K_empty0, k_empty0); // Unknown cell data
}

static void _max17050_set_por_bit(u16 value)
{
	_max17050_write_verify_reg(MAX17050_STATUS, value);
}

int max17050_fix_configuration()
{
	/* Init phase, set the POR bit */
	_max17050_set_por_bit(STATUS_POR_BIT);

	/* Override POR values */
	_max17050_override_por_values();
	/* After Power up, the MAX17050 requires 500ms in order
	 * to perform signal debouncing and initial SOC reporting
	 */
	msleep(500);

	/* Initialize configaration */
	_max17050_write_config_regs();

	/* update capacity params */
	_max17050_update_capacity_regs();

	/* delay must be atleast 350mS to allow VFSOC
	 * to be calculated from the new configuration
	 */
	msleep(350);

	/* reset vfsoc0 reg */
	_max17050_reset_vfsoc0_reg();

	/* load new capacity params */
	_max17050_load_new_capacity_params();

	/* Init complete, Clear the POR bit */
	//_max17050_set_por_bit(0); // Should we? Or let the switch to reconfigure POR?

	// Sets POR, BI, BR.
	_max17050_set_por_bit(0x8801);

	return 0;
}
