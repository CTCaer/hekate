/*
 * Battery charger driver for Nintendo Switch's TI BQ24193
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

#include "bq24193.h"
#include <soc/i2c.h>
#include <utils/util.h>

static u8 bq24193_get_reg(u8 reg)
{
	return i2c_recv_byte(I2C_1, BQ24193_I2C_ADDR, reg);
}

int bq24193_get_property(enum BQ24193_reg_prop prop, int *value)
{
	u8 data;

	switch (prop) {
		case BQ24193_InputVoltageLimit: // Input voltage limit (mV).
			data = bq24193_get_reg(BQ24193_InputSource);
			data = (data & BQ24193_INCONFIG_VINDPM_MASK) >> 3;
			*value = 0;
			*value += ((data >> 0) & 1) ? 80 : 0;
			*value += ((data >> 1) & 1) ? 160 : 0;
			*value += ((data >> 2) & 1) ? 320 : 0;
			*value += ((data >> 3) & 1) ? 640 : 0;
			*value += 3880;
			break;
		case BQ24193_InputCurrentLimit: // Input current limit (mA).
			data = bq24193_get_reg(BQ24193_InputSource);
			data &= BQ24193_INCONFIG_INLIMIT_MASK;
			switch (data)
			{
			case 0:
				*value = 100;
				break;
			case 1:
				*value = 150;
				break;
			case 2:
				*value = 500;
				break;
			case 3:
				*value = 900;
				break;
			case 4:
				*value = 1200;
				break;
			case 5:
				*value = 1500;
				break;
			case 6:
				*value = 2000;
				break;
			case 7:
				*value = 3000;
				break;
			}
			break;
		case BQ24193_SystemMinimumVoltage: // Minimum system voltage limit (mV).
			data = bq24193_get_reg(BQ24193_PORConfig);
			*value = (data & BQ24193_PORCONFIG_SYSMIN_MASK) >> 1;
			*value *= 100;
			*value += 3000;
			break;
		case BQ24193_FastChargeCurrentLimit: // Fast charge current limit (mA).
			data = bq24193_get_reg(BQ24193_ChrgCurr);
			data = (data & BQ24193_CHRGCURR_ICHG_MASK) >> 2;
			*value = 0;
			*value += ((data >> 0) & 1) ? 64 : 0;
			*value += ((data >> 1) & 1) ? 128 : 0;
			*value += ((data >> 2) & 1) ? 256 : 0;
			*value += ((data >> 3) & 1) ? 512 : 0;
			*value += ((data >> 4) & 1) ? 1024 : 0;
			*value += ((data >> 5) & 1) ? 2048 : 0;
			*value += 512;
			data = bq24193_get_reg(BQ24193_ChrgCurr);
			data &= BQ24193_CHRGCURR_20PCT_MASK;
			if (data)
				*value = *value * 20 / 100; // Fast charge current limit is 20%.
			break;
		case BQ24193_ChargeVoltageLimit: // Charge voltage limit (mV).
			data = bq24193_get_reg(BQ24193_ChrgVolt);
			data = (data & BQ24193_CHRGVOLT_VREG) >> 2;
			*value = 0;
			*value += ((data >> 0) & 1) ? 16 : 0;
			*value += ((data >> 1) & 1) ? 32 : 0;
			*value += ((data >> 2) & 1) ? 64 : 0;
			*value += ((data >> 3) & 1) ? 128 : 0;
			*value += ((data >> 4) & 1) ? 256 : 0;
			*value += ((data >> 5) & 1) ? 512 : 0;
			*value += 3504;
			break;
		case BQ24193_RechargeThreshold: // Recharge voltage threshold less than voltage limit (mV).
			data = bq24193_get_reg(BQ24193_ChrgVolt);
			data &= BQ24193_IRTHERMAL_THERM_MASK;
			if (data)
				*value = 300;
			else
				*value = 100;
			break;
		case BQ24193_ThermalRegulation: // Thermal regulation threshold (oC).
			data = bq24193_get_reg(BQ24193_IRCompThermal);
			data &= BQ24193_IRTHERMAL_THERM_MASK;
			switch (data)
			{
			case 0:
				*value = 60;
				break;
			case 1:
				*value = 80;
				break;
			case 2:
				*value = 100;
				break;
			case 3:
				*value = 120;
				break;
			}
			break;
		case BQ24193_ChargeStatus: // 0: Not charging, 1: Pre-charge, 2: Fast charging, 3: Charge termination done
			data = bq24193_get_reg(BQ24193_Status);
			*value = (data & BQ24193_STATUS_CHRG_MASK) >> 4;
			break;
		case BQ24193_TempStatus: // 0: Normal, 2: Warm, 3: Cool, 5: Cold, 6: Hot.
			data = bq24193_get_reg(BQ24193_FaultReg);
			*value = data & BQ24193_FAULT_THERM_MASK;
			break;
		case BQ24193_DevID: // Dev ID.
			data = bq24193_get_reg(BQ24193_VendorPart);
			*value = data & BQ24193_VENDORPART_DEV_MASK;
			break;
		case BQ24193_ProductNumber: // Product number.
			data = bq24193_get_reg(BQ24193_VendorPart);
			*value = (data & BQ24193_VENDORPART_PN_MASK) >> 3;
			break;
		default:
			return -1;
	}
	return 0;
}

void bq24193_enable_charger()
{
	u8 reg = bq24193_get_reg(BQ24193_PORConfig);

	reg &= ~BQ24193_PORCONFIG_CHGCONFIG_MASK;
	reg |= BQ24193_PORCONFIG_CHGCONFIG_CHARGER_EN;

	i2c_send_byte(I2C_1, BQ24193_I2C_ADDR, BQ24193_PORConfig, reg);
}

void bq24193_fake_battery_removal()
{
	// Disable watchdog to keep BATFET disabled.
	u8 value = bq24193_get_reg(BQ24193_ChrgTermTimer);
	value &= ~BQ24193_CHRGTERM_WATCHDOG_MASK;
	i2c_send_byte(I2C_1, BQ24193_I2C_ADDR, BQ24193_ChrgTermTimer, value);

	// Force BATFET to disabled state. This disconnects the battery from the system.
	value = bq24193_get_reg(BQ24193_Misc);
	value |= BQ24193_MISC_BATFET_DI_MASK;
	i2c_send_byte(I2C_1, BQ24193_I2C_ADDR, BQ24193_Misc, value);
}
