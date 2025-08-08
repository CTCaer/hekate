/*
 * PMIC Real Time Clock driver for Nintendo Switch's MAX77620-RTC
 *
 * Copyright (c) 2018-2025 CTCaer
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
#include <soc/pmc.h>
#include <soc/timer.h>
#include <soc/t210.h>

bool auto_dst     = false;
int  epoch_offset = 0;

void max77620_rtc_prep_read()
{
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_UPDATE0_REG, MAX77620_RTC_READ_UPDATE);
}

void max77620_rtc_get_time(rtc_time_t *time)
{
	u8 val = 0;

	// Update RTC regs from RTC clock.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_UPDATE0_REG, MAX77620_RTC_READ_UPDATE);

	// Get control reg config.
	val = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_CONTROL_REG);
	// TODO: Make also sure it's binary format?

	// Get time.
	time->sec  = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_SEC_REG)  & 0x7F;
	time->min  = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_MIN_REG)  & 0x7F;
	u8 hour    = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_HOUR_REG) & 0x1F;
	time->hour = hour & 0x1F;

	if (!(val & MAX77620_RTC_24H))
	{
		time->hour = hour & 0xF;
		if (hour & MAX77620_RTC_HOUR_PM_MASK)
			time->hour += 12;
	}

	// Get day of week. 1: Monday to 7: Sunday.
	time->weekday = 0;
	val = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_WEEKDAY_REG) & 0x7F;
	for (int i = 0; i < 8; i++)
	{
		time->weekday++;
		if (val & 1)
			break;
		val >>= 1;
	}

	// Get date.
	time->day   = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_DAY_REG) & 0x1F;
	time->month = (i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_MONTH_REG) & 0xF) - 1;
	time->month++; // Normally minus 1, but everything else expects 1 as January.
	time->year  = (i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_YEAR_REG) & 0x7F) + 2000;
}

void max77620_rtc_stop_alarm()
{
	u8 val = 0;

	// Update RTC regs from RTC clock.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_UPDATE0_REG, MAX77620_RTC_READ_UPDATE);
	msleep(16);

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
	time->sec  = epoch % 60;
	epoch /= 60;
	time->min  = epoch % 60;
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
	if (month < 14)
	{
		year -= 4716;
		month--;
	}
	else
	{
		year  -= 4715;
		month -= 13;
	}

	// Set date.
	time->year = year;
	time->month = month;
	time->day = day;

	// Set weekday.
	time->weekday = (day + 4) % 7;
}

u32 max77620_rtc_date_to_epoch(const rtc_time_t *time)
{
	u32 year, month, epoch;

	//Year
	year = time->year;
	//Month of year
	month = time->month;

	// Month/Year offset.
	if (month < 3)
	{
		month += 12;
		year--;
	}

	epoch  = (365 * year) + (year >> 2) - (year / 100) + (year / 400); // Years to days.

	epoch += (30 * month) + (3 * (month + 1) / 5) + time->day; // Months to days.
	epoch -= 719561; // Epoch time is 1/1/1970.

	epoch *= 86400; // Days to seconds.
	epoch += (3600 * time->hour) + (60 * time->min) + time->sec; // Add hours, minutes and seconds.

	return epoch;
}

void max77620_rtc_get_time_adjusted(rtc_time_t *time)
{
	max77620_rtc_get_time(time);
	if (epoch_offset)
	{
		u32 epoch = (u32)((s64)max77620_rtc_date_to_epoch(time) + epoch_offset);

		// Adjust for DST between 28 march and 28 october. Good enough to cover all years as week info is not valid.
		u16 md = (time->month << 8) | time->day;
		if (auto_dst && md >= 0x31C && md < 0xA1C)
			epoch += 3600;

		max77620_rtc_epoch_to_date(epoch, time);
	}
}

void max77620_rtc_set_auto_dst(bool enable)
{
	auto_dst = enable;
}

void max77620_rtc_set_epoch_offset(int offset)
{
	epoch_offset = offset;
}

void max77620_rtc_set_reboot_reason(rtc_reboot_reason_t *rr)
{
	max77620_rtc_stop_alarm();

	// Set reboot reason.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM1_YEAR_REG, rr->enc.val1);
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM2_YEAR_REG, rr->enc.val2);

	// Set reboot reason magic.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM1_WEEKDAY_REG, RTC_REBOOT_REASON_MAGIC);
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM2_WEEKDAY_REG, RTC_REBOOT_REASON_MAGIC);

	// Update RTC clock from RTC regs.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_UPDATE0_REG, MAX77620_RTC_WRITE_UPDATE);
	msleep(16);
}

bool max77620_rtc_get_reboot_reason(rtc_reboot_reason_t *rr)
{
	u8 magic[2];

	// Get reboot reason magic.
	magic[0] = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM1_WEEKDAY_REG);
	magic[1] = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM2_WEEKDAY_REG);

	// Magic must be correct and match on both registers.
	if (magic[0] != RTC_REBOOT_REASON_MAGIC || magic[0] != magic[1])
		return false;

	// Reboot reason setter is expected to have updated the actual regs already.
	rr->enc.val1 = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM1_YEAR_REG);
	rr->enc.val2 = i2c_recv_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM2_YEAR_REG);

	// Clear magic and update actual regs.
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM1_WEEKDAY_REG, 0);
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_ALARM2_WEEKDAY_REG, 0);
	i2c_send_byte(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_RTC_UPDATE0_REG, MAX77620_RTC_WRITE_UPDATE);

	// Return reboot reason. False if [config] was selected.
	return true;
}
