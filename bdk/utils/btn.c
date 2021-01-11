/*
 * Copyright (c) 2018 naehrwert
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

#include "btn.h"
#include <soc/i2c.h>
#include <soc/gpio.h>
#include <soc/t210.h>
#include <utils/util.h>
#include <power/max77620.h>

u8 btn_read()
{
	u8 res = 0;
	if (!gpio_read(GPIO_PORT_X, GPIO_PIN_7))
		res |= BTN_VOL_DOWN;
	if (!gpio_read(GPIO_PORT_X, GPIO_PIN_6))
		res |= BTN_VOL_UP;
	if (i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFSTAT) & MAX77620_ONOFFSTAT_EN0)
		res |= BTN_POWER;
	return res;
}

u8 btn_read_vol()
{
	u8 res = 0;
	if (!gpio_read(GPIO_PORT_X, GPIO_PIN_7))
		res |= BTN_VOL_DOWN;
	if (!gpio_read(GPIO_PORT_X, GPIO_PIN_6))
		res |= BTN_VOL_UP;
	return res;
}

u8 btn_wait()
{
	u8 res = 0, btn = btn_read();
	bool pwr = false;

	//Power button down, raise a filter.
	if (btn & BTN_POWER)
	{
		pwr = true;
		btn &= ~BTN_POWER;
	}

	do
	{
		res = btn_read();
		//Power button up, remove filter.
		if (!(res & BTN_POWER) && pwr)
			pwr = false;
		else if (pwr) //Power button still down.
			res &= ~BTN_POWER;
	} while (btn == res);

	return res;
}

u8 btn_wait_timeout(u32 time_ms, u8 mask)
{
	u32 timeout = get_tmr_ms() + time_ms;
	u8 res = btn_read() & mask;

	while (get_tmr_ms() < timeout)
	{
		if (res == mask)
			break;
		else
			res = btn_read() & mask;
	};

	return res;
}

u8 btn_wait_timeout_single(u32 time_ms, u8 mask)
{
	u8 single_button = mask & BTN_SINGLE;
	mask &= ~BTN_SINGLE;

	u32 timeout = get_tmr_ms() + time_ms;
	u8 res = btn_read();

	while (get_tmr_ms() < timeout)
	{
		if ((res & mask) == mask)
		{
			if (single_button && (res & ~mask)) // Undesired button detected.
				res = btn_read();
			else
				return (res & mask);
		}
		else
			res = btn_read();
	};

	// Timed out.
	if (!single_button || !time_ms)
		return (res & mask);
	else
		return 0; // Return no button press if single button requested.
}
