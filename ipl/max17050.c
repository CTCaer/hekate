/*
 * Fuel gauge driver for Nintendo Switch's Maxim 17050
 *
 * Copyright (C) 2011 Samsung Electronics
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 * Copyright (C) 2018 CTCaer
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
#include "i2c.h"
#include "util.h"

/* Status register bits */
#define STATUS_POR_BIT (1 << 1)
#define STATUS_BST_BIT (1 << 3)
#define STATUS_VMN_BIT (1 << 8)
#define STATUS_TMN_BIT (1 << 9)
#define STATUS_SMN_BIT (1 << 10)
#define STATUS_BI_BIT  (1 << 11)
#define STATUS_VMX_BIT (1 << 12)
#define STATUS_TMX_BIT (1 << 13)
#define STATUS_SMX_BIT (1 << 14)
#define STATUS_BR_BIT  (1 << 15)

/* Interrupt mask bits */
#define CONFIG_ALRT_BIT_ENBL   (1 << 2)
#define STATUS_INTR_SOCMIN_BIT (1 << 10)
#define STATUS_INTR_SOCMAX_BIT (1 << 14)

#define VFSOC0_LOCK   0x0000
#define VFSOC0_UNLOCK 0x0080

#define dP_ACC_100 0x1900
#define dP_ACC_200 0x3200

#define MAX17050_VMAX_TOLERANCE 50 /* 50 mV */

static int _max17050_get_temperature(int *temp)
{
	u16 data;

	i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_TEMP);

	*temp = (s16)data;
	/* The value is converted into deci-centigrade scale */
	/* Units of LSB = 1 / 256 degree Celsius */
	*temp = *temp * 10 / 256;
	return 0;
}

int _max17050_get_status(int *status)
{
	int charge_full, charge_now;
	int avg_current;
	u16 data;

	/*
	 * The MAX170xx has builtin end-of-charge detection and will update
	 * FullCAP to match RepCap when it detects end of charging.
	 *
	 * When this cycle the battery gets charged to a higher (calculated)
	 * capacity then the previous cycle then FullCAP will get updated
	 * contineously once end-of-charge detection kicks in, so allow the
	 * 2 to differ a bit.
	 */

	i2c_recv_buf_small((u8 *)&charge_full, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_FullCAP);

	i2c_recv_buf_small((u8 *)&charge_now, 2, I2C_1, MAXIM17050_I2C_ADDR,  MAX17050_RepCap);

	if ((charge_full - charge_now) <= MAX17050_FULL_THRESHOLD) {
		*status = 0xFF; //FULL
		return 0;
	}

	/*
	 * Even though we are supplied, we may still be discharging if the
	 * supply is e.g. only delivering 5V 0.5A. Check current if available.
	 */
	i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_AvgCurrent);

	avg_current = (s16)data;
	avg_current *= 1562500 / MAX17050_DEFAULT_SNS_RESISTOR;

	if (avg_current > 0)
		*status = 0x1; //Charging
	else
		*status = 0x0; //Discharging

	return 0;
}

int _max17050_get_battery_health(int *health)
{
	int temp, vavg, vbatt;
	u16 val;

	i2c_recv_buf_small((u8 *)&val, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_AvgVCELL);
	/* bits [0-3] unused */
	vavg = val * 625 / 8;
	/* Convert to millivolts */
	vavg /= 1000;

	i2c_recv_buf_small((u8 *)&val, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_VCELL);
	/* bits [0-3] unused */
	vbatt = val * 625 / 8;
	/* Convert to millivolts */
	vbatt /= 1000;

	if (vavg < MAX17050_DEFAULT_VMIN) {
		*health = HEALTH_DEAD;
		goto out;
	}

	if (vbatt > MAX17050_DEFAULT_VMAX + MAX17050_VMAX_TOLERANCE) {
		*health = HEALTH_OVERVOLTAGE;
		goto out;
	}

	_max17050_get_temperature(&temp);

	if (temp < MAX17050_DEFAULT_TEMP_MIN) {
		*health = HEALTH_COLD;
		goto out;
	}

	if (temp > MAX17050_DEFAULT_TEMP_MAX) {
		*health = HEALTH_OVERHEAT;
		goto out;
	}

	*health = HEALTH_GOOD; // 1

out:
	return 0;
}

int max17050_get_property(enum MAX17050_reg reg, int *value)
{
	u16 data;

	switch (reg) {
		//case 0x101://///////////////////////////FIX
		//	_max17050_get_status(value);
		//	break;
		case MAX17050_Cycles: //Cycle count.
			i2c_recv_buf_small((u8 *)value, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_Cycles);
			break;
		case MAX17050_MinMaxVolt: //Voltage max/min
			i2c_recv_buf_small((u8 *)value, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_MinMaxVolt);
			//value = (data >> 8) * 20; /* Voltage MAX. Units of LSB = 20mV */
			//value = (data & 0xff) * 20; /* Voltage MIN. Units of 20mV */
			*value = data;
			break;
		case MAX17050_V_empty: //Voltage min design.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_V_empty);
			*value = (data >> 7) * 10; /* Units of LSB = 10mV */
			break;
		case MAX17050_VCELL: //Voltage now.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_VCELL);
			*value = data * 625 / 8 / 1000;
			break;
		case MAX17050_AvgVCELL: //Voltage avg.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_AvgVCELL);
			*value = data * 625 / 8 / 1000;
			break;
		case MAX17050_OCVInternal: //Voltage ocv.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_OCVInternal);
			*value = data * 625 / 8 / 1000;
			break;
		case MAX17050_RepSOC: //Capacity %.
			i2c_recv_buf_small((u8 *)value, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_RepSOC);
			break;
		case MAX17050_DesignCap: //Charge full design.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_DesignCap);
			data = data * 5 / 10;
			*value = data;
			break;
		case MAX17050_FullCAP: //Charge full.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_FullCAP);
			data = data * 5 / 10;
			*value = data;
			break;
		case MAX17050_RepCap: //Charge now.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_RepCap);
			data = data * 5 / 10;
			*value = data;
			break;
		case MAX17050_TEMP: //Temp.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_TEMP);
			*value = (s16)(data);
			*value = *value * 10 / 256;
			break;
		//case 0x100: //FIX me
		//	_max17050_get_battery_health(value);
		//	break;
		case MAX17050_Current: //Current now.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_Current);
			*value = (s16)data;
			*value *= 1562500 / MAX17050_DEFAULT_SNS_RESISTOR;
			break;
		case MAX17050_AvgCurrent: //Current avg.
			i2c_recv_buf_small((u8 *)&data, 2, I2C_1, MAXIM17050_I2C_ADDR, MAX17050_AvgCurrent);
			*value = (s16)data;
			*value *= 1562500 / MAX17050_DEFAULT_SNS_RESISTOR;
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

	do {
		ret = i2c_send_buf_small(I2C_1, MAXIM17050_I2C_ADDR, reg, (u8 *)&value, 2);
		i2c_recv_buf_small((u8 *)&read_value, 2, I2C_1, MAXIM17050_I2C_ADDR, reg);
		if (read_value != value) {
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
	u16 value = 0x2476; //Set to 4667mAh design capacity.
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
	//_max17050_override_por(map, MAX17050_K_empty0, k_empty0); // unknown cell data
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
	sleep(500000);

	/* Initialize configaration */
	_max17050_write_config_regs();


	/* update capacity params */
	_max17050_update_capacity_regs();

	/* delay must be atleast 350mS to allow VFSOC
	 * to be calculated from the new configuration
	 */
	sleep(350000);

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
