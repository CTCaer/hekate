/*
 * Fuel gauge driver for Nintendo Switch's Maxim 17050
 *
 * Copyright (c) 2011 Samsung Electronics
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 * Copyright (c) 2018-2020 CTCaer
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
 */

#ifndef __MAX17050_H_
#define __MAX17050_H_

#include <utils/types.h>

/* Board default values */
#define MAX17050_BOARD_CGAIN 2 /* Actual: 1.99993 */
#define MAX17050_BOARD_SNS_RESISTOR_UOHM 5000 /* 0.005 Ohm */

#define MAX17050_STATUS_BattAbsent BIT(3)

/* Consider RepCap which is less then 10 units below FullCAP full */
#define MAX17050_FULL_THRESHOLD 10

#define MAX17050_CHARACTERIZATION_DATA_SIZE 48

#define MAXIM17050_I2C_ADDR 0x36

enum MAX17050_reg {
	MAX17050_STATUS		= 0x00,
	MAX17050_VALRT_Th	= 0x01,
	MAX17050_TALRT_Th	= 0x02,
	MAX17050_SALRT_Th	= 0x03,
	MAX17050_AtRate		= 0x04,
	MAX17050_RepCap		= 0x05,
	MAX17050_RepSOC		= 0x06,
	MAX17050_Age		= 0x07,
	MAX17050_TEMP		= 0x08,
	MAX17050_VCELL		= 0x09,
	MAX17050_Current	= 0x0A,
	MAX17050_AvgCurrent	= 0x0B,

	MAX17050_SOC		= 0x0D,
	MAX17050_AvSOC		= 0x0E,
	MAX17050_RemCap		= 0x0F,
	MAX17050_FullCAP	= 0x10,
	MAX17050_TTE		= 0x11,
	MAX17050_QRTbl00	= 0x12,
	MAX17050_FullSOCThr	= 0x13,
	MAX17050_RSLOW		= 0x14,

	MAX17050_AvgTA		= 0x16,
	MAX17050_Cycles		= 0x17,
	MAX17050_DesignCap	= 0x18,
	MAX17050_AvgVCELL	= 0x19,
	MAX17050_MinMaxTemp	= 0x1A,
	MAX17050_MinMaxVolt	= 0x1B,
	MAX17050_MinMaxCurr	= 0x1C,
	MAX17050_CONFIG		= 0x1D,
	MAX17050_ICHGTerm	= 0x1E,
	MAX17050_AvCap		= 0x1F,
	MAX17050_ManName	= 0x20,
	MAX17050_DevName	= 0x21,
	MAX17050_QRTbl10	= 0x22,
	MAX17050_FullCAPNom	= 0x23,
	MAX17050_TempNom	= 0x24,
	MAX17050_TempLim	= 0x25,
	MAX17050_TempHot	= 0x26,
	MAX17050_AIN		= 0x27,
	MAX17050_LearnCFG	= 0x28,
	MAX17050_FilterCFG	= 0x29,
	MAX17050_RelaxCFG	= 0x2A,
	MAX17050_MiscCFG	= 0x2B,
	MAX17050_TGAIN		= 0x2C,
	MAX17050_TOFF		= 0x2D,
	MAX17050_CGAIN		= 0x2E,
	MAX17050_COFF		= 0x2F,

	MAX17050_QRTbl20	= 0x32,
	MAX17050_SOC_empty	= 0x33,
	MAX17050_T_empty	= 0x34,
	MAX17050_FullCAP0	= 0x35,
	MAX17050_LAvg_empty	= 0x36,
	MAX17050_FCTC		= 0x37,
	MAX17050_RCOMP0		= 0x38,
	MAX17050_TempCo		= 0x39,
	MAX17050_V_empty	= 0x3A,
	MAX17050_K_empty0	= 0x3B,
	MAX17050_TaskPeriod	= 0x3C,
	MAX17050_FSTAT		= 0x3D,
	MAX17050_TIMER		= 0x3E,
	MAX17050_SHDNTIMER	= 0x3F,

	MAX17050_QRTbl30	= 0x42,

	MAX17050_dQacc		= 0x45,
	MAX17050_dPacc		= 0x46,

	MAX17050_VFSOC0		= 0x48,

	Max17050_QH0		= 0x4C,
	MAX17050_QH			= 0x4D,
	MAX17050_QL			= 0x4E,

	MAX17050_MinVolt	= 0x50, // Custom ID. Not to be sent to i2c.
	MAX17050_MaxVolt	= 0x51, // Custom ID. Not to be sent to i2c.

	MAX17050_VFSOC0Enable	= 0x60,
	MAX17050_MODELEnable1	= 0x62,
	MAX17050_MODELEnable2	= 0x63,

	MAX17050_MODELChrTbl	= 0x80,

	MAX17050_OCV			= 0xEE,

	MAX17050_OCVInternal	= 0xFB,

	MAX17050_VFSOC			= 0xFF,
};

int max17050_get_property(enum MAX17050_reg reg, int *value);
int max17050_fix_configuration();
u32 max17050_get_cached_batt_volt();

#endif /* __MAX17050_H_ */
