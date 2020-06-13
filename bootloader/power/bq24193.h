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

#ifndef __BQ24193_H_
#define __BQ24193_H_

#define BQ24193_I2C_ADDR 0x6B

// REG 0 masks.
#define BQ24193_INCONFIG_INLIMIT_MASK (7<<0)
#define BQ24193_INCONFIG_VINDPM_MASK   0x78
#define BQ24193_INCONFIG_HIZ_EN_MASK  (1<<7)

// REG 1 masks.
#define BQ24193_PORCONFIG_BOOST_MASK       (1<<0)
#define BQ24193_PORCONFIG_SYSMIN_MASK      (7<<1)
#define BQ24193_PORCONFIG_CHGCONFIG_MASK   (3<<4)
#define BQ24193_PORCONFIG_CHGCONFIG_CHARGER_EN (1<<4)
#define BQ24193_PORCONFIG_I2CWATCHDOG_MASK (1<<6)
#define BQ24193_PORCONFIG_RESET_MASK       (1<<7)

// REG 2 masks.
#define BQ24193_CHRGCURR_20PCT_MASK (1<<0)
#define BQ24193_CHRGCURR_ICHG_MASK   0xFC

// REG 3 masks.
#define BQ24193_PRECHRG_ITERM   0x0F
#define BQ24193_PRECHRG_IPRECHG 0xF0

// REG 4 masks.
#define BQ24193_CHRGVOLT_VTHRES  (1<<0)
#define BQ24193_CHRGVOLT_BATTLOW (1<<1)
#define BQ24193_CHRGVOLT_VREG     0xFC

// REG 5 masks.
#define BQ24193_CHRGTERM_ISET_MASK     (1<<0)
#define BQ24193_CHRGTERM_CHGTIMER_MASK (3<<1)
#define BQ24193_CHRGTERM_ENTIMER_MASK  (1<<3)
#define BQ24193_CHRGTERM_WATCHDOG_MASK (3<<4)
#define BQ24193_CHRGTERM_TERM_ST_MASK  (1<<6)
#define BQ24193_CHRGTERM_TERM_EN_MASK  (1<<7)

// REG 6 masks.
#define BQ24193_IRTHERMAL_THERM_MASK    (3<<0)
#define BQ24193_IRTHERMAL_VCLAMP_MASK   (7<<2)
#define BQ24193_IRTHERMAL_BATTCOMP_MASK (7<<5)

// REG 7 masks.
#define BQ24193_MISC_INT_MASK       (3<<0)
#define BQ24193_MISC_VSET_MASK      (1<<4)
#define BQ24193_MISC_BATFET_DI_MASK (1<<5)
#define BQ24193_MISC_TMR2X_EN_MASK  (1<<6)
#define BQ24193_MISC_DPDM_EN_MASK   (1<<7)

// REG 8 masks.
#define BQ24193_STATUS_VSYS_MASK  (1<<0)
#define BQ24193_STATUS_THERM_MASK (1<<1)
#define BQ24193_STATUS_PG_MASK    (1<<2)
#define BQ24193_STATUS_DPM_MASK   (1<<3)
#define BQ24193_STATUS_CHRG_MASK  (3<<4)
#define BQ24193_STATUS_VBUS_MASK  (3<<6)

// REG 9 masks.
#define BQ24193_FAULT_THERM_MASK    (7<<0)
#define BQ24193_FAULT_BATT_OVP_MASK (1<<3)
#define BQ24193_FAULT_CHARGE_MASK   (3<<4)
#define BQ24193_FAULT_BOOST_MASK    (1<<6)
#define BQ24193_FAULT_WATCHDOG_MASK (1<<7)

// REG A masks.
#define BQ24193_VENDORPART_DEV_MASK (3<<0)
#define BQ24193_VENDORPART_PN_MASK  (7<<3)

enum BQ24193_reg {
	BQ24193_InputSource     = 0x00,
	BQ24193_PORConfig       = 0x01,
	BQ24193_ChrgCurr        = 0x02,
	BQ24193_PreChrgTerm     = 0x03,
	BQ24193_ChrgVolt        = 0x04,
	BQ24193_ChrgTermTimer   = 0x05,
	BQ24193_IRCompThermal   = 0x06,
	BQ24193_Misc            = 0x07,
	BQ24193_Status          = 0x08,
	BQ24193_FaultReg        = 0x09,
	BQ24193_VendorPart      = 0x0A,
};

enum BQ24193_reg_prop {
	BQ24193_InputVoltageLimit,      // REG 0.
	BQ24193_InputCurrentLimit,      // REG 0.
	BQ24193_SystemMinimumVoltage,   // REG 1.
	BQ24193_FastChargeCurrentLimit, // REG 2.
	BQ24193_ChargeVoltageLimit,     // REG 4.
	BQ24193_RechargeThreshold,      // REG 4.
	BQ24193_ThermalRegulation,      // REG 6.
	BQ24193_ChargeStatus,           // REG 8.
	BQ24193_TempStatus,             // REG 9.
	BQ24193_DevID,                  // REG A.
	BQ24193_ProductNumber,          // REG A.
};

int bq24193_get_property(enum BQ24193_reg_prop prop, int *value);
void bq24193_enable_charger();
void bq24193_fake_battery_removal();

#endif /* __BQ24193_H_ */
