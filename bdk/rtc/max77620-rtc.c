/*
 * PMIC Real Time Clock driver for Nintendo Switch's MAX77620-RTC
 *
 * Copyright (c) 2018-2019 CTCaer
 * Copyright (c) 2019 shchmue
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

#include <rtc/max77620-rtc.h>
#include <soc/i2c.h>
#include <utils/util.h>

void max77620_rtc_get_time(rtc_time_t *time)
{
	u8 val = 0;

	// Update RTC regs from RTC clock.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_UPDATE0_REG, MAX77620_RTC_READ_UPDATE);

	// Get control reg config.
	val = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_CONTROL_REG);
	// TODO: Check for binary format also?

	// Get time.
	time->sec  = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_SEC_REG) & 0x7F;
	time->min  = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_MIN_REG) & 0x7F;
	u8 hour = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_HOUR_REG);
	time->hour = hour & 0x1F;

	if (!(val & MAX77620_RTC_24H) && (hour & MAX77620_RTC_HOUR_PM_MASK))
		time->hour = (time->hour & 0xF) + 12;

	// Get day of week. 1: Monday to 7: Sunday.
	time->weekday = 0;
	val = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_WEEKDAY_REG);
	for (int i = 0; i < 8; i++)
	{
		time->weekday++;
		if (val & 1)
			break;
		val >>= 1;
	}

	// Get date.
	time->day  = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_DATE_REG) & 0x1f;
	time->month = (i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_MONTH_REG) & 0xF) - 1;
	time->year  = (i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_YEAR_REG) & 0x7F) + 2000;
}

void max77620_rtc_stop_alarm()
{
	u8 val = 0;

	// Update RTC regs from RTC clock.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_UPDATE0_REG, MAX77620_RTC_READ_UPDATE);

	// Stop alarm for both ALARM1 and ALARM2. Horizon uses ALARM2.
	for (int i = 0; i < (MAX77620_RTC_NR_TIME_REGS * 2); i++)
	{
		val = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM1_SEC_REG + i);
		val &= ~MAX77620_RTC_ALARM_EN_MASK;
		i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM1_SEC_REG + i, val);
	}

	// Update RTC clock from RTC regs.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_UPDATE0_REG, MAX77620_RTC_WRITE_UPDATE);
}

void max77620_rtc_epoch_to_date(u32 epoch, rtc_time_t *time)
{
	u32 tmp, edays, year, month, day;

	// Set time.
	time->sec = epoch % 60;
	epoch /= 60;
	time->min = epoch % 60;
	epoch /= 60;
	time->hour = epoch % 24;
	epoch /= 24;

	// Calculate base date values.
	tmp = (u32)(((u64)4 * epoch + 102032) / 146097 + 15);
	tmp = (u32)((u64)epoch + 2442113 + tmp - (tmp >> 2));

	year = (20 * tmp - 2442) / 7305;
	edays = tmp - 365 * year - (year >> 2);
	month = edays * 1000 / 30601;
	day = edays - month * 30 - month * 601 / 1000;

	// Month/Year offset.
	if(month < 14)
	{
		year -= 4716;
		month--;
	}
	else
	{
		year -= 4715;
		month -= 13;
	}

	// Set date.
	time->year = year;
	time->month = month;
	time->day = day;

	// Set weekday.
	time->weekday = 0; //! TODO.
}

u32 max77620_rtc_date_to_epoch(const rtc_time_t *time)
{
	u32 year, month, epoch;

	//Year
	year = time->year;
	//Month of year
	month = time->month;

	// Month/Year offset.
	if(month < 3)
	{
		month += 12;
		year--;
	}

	epoch = (365 * year) + (year >> 2) - (year / 100) + (year / 400); // Years to days.

	epoch += (30 * month) + (3 * (month + 1) / 5) + time->day; // Months to days.
	epoch -= 719561; // Epoch time is 1/1/1970.

	epoch *= 86400; // Days to seconds.
	epoch += (3600 * time->hour) + (60 * time->min) + time->sec; // Add hours, minutes and seconds.

	return epoch;
}
