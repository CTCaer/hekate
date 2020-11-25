/*
 * PMIC Real Time Clock driver for Nintendo Switch's MAX77620-RTC
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

#ifndef _MFD_MAX77620_RTC_H_
#define _MFD_MAX77620_RTC_H_

#include <utils/types.h>

#define MAX77620_RTC_I2C_ADDR       0x68

#define MAX77620_RTC_NR_TIME_REGS   7

#define MAX77620_RTC_CONTROLM_REG   0x02
#define MAX77620_RTC_CONTROL_REG    0x03
#define  MAX77620_RTC_BIN_FORMAT    BIT(0)
#define  MAX77620_RTC_24H           BIT(1)

#define MAX77620_RTC_UPDATE0_REG    0x04
#define  MAX77620_RTC_WRITE_UPDATE  BIT(0)
#define  MAX77620_RTC_READ_UPDATE   BIT(4)

#define MAX77620_RTC_SEC_REG        0x07
#define MAX77620_RTC_MIN_REG        0x08
#define MAX77620_RTC_HOUR_REG       0x09
#define  MAX77620_RTC_HOUR_PM_MASK  BIT(6)
#define MAX77620_RTC_WEEKDAY_REG    0x0A
#define MAX77620_RTC_MONTH_REG      0x0B
#define MAX77620_RTC_YEAR_REG       0x0C
#define MAX77620_RTC_DATE_REG       0x0D

#define MAX77620_ALARM1_SEC_REG     0x0E
#define MAX77620_ALARM1_MIN_REG     0x0F
#define MAX77620_ALARM1_HOUR_REG    0x10
#define MAX77620_ALARM1_WEEKDAY_REG 0x11
#define MAX77620_ALARM1_MONTH_REG   0x12
#define MAX77620_ALARM1_YEAR_REG    0x13
#define MAX77620_ALARM1_DATE_REG    0x14
#define MAX77620_ALARM2_SEC_REG     0x15
#define MAX77620_ALARM2_MIN_REG     0x16
#define MAX77620_ALARM2_HOUR_REG    0x17
#define MAX77620_ALARM2_WEEKDAY_REG 0x18
#define MAX77620_ALARM2_MONTH_REG   0x19
#define MAX77620_ALARM2_YEAR_REG    0x1A
#define MAX77620_ALARM2_DATE_REG    0x1B
#define  MAX77620_RTC_ALARM_EN_MASK	BIT(7)

typedef struct _rtc_time_t {
	u8  weekday;
	u8  sec;
	u8  min;
	u8  hour;
	u8  day;
	u8  month;
	u16 year;
} rtc_time_t;

void max77620_rtc_get_time(rtc_time_t *time);
void max77620_rtc_stop_alarm();
void max77620_rtc_epoch_to_date(u32 epoch, rtc_time_t *time);
u32 max77620_rtc_date_to_epoch(const rtc_time_t *time);

#endif /* _MFD_MAX77620_RTC_H_ */
