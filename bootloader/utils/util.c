/*
* Copyright (c) 2018 naehrwert
* Copyright (C) 2018 CTCaer
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

#include "util.h"
#include "../soc/t210.h"

u32 get_tmr_s()
{
	return RTC(0x8); //RTC_SECONDS
}

u32 get_tmr_ms()
{
	// The registers must be read with the following order:
	// -> RTC_MILLI_SECONDS (0x10) -> RTC_SHADOW_SECONDS (0xC)
	return (RTC(0x10) | (RTC(0xC)<< 10));
}

u32 get_tmr_us()
{
	return TMR(0x10); //TIMERUS_CNTR_1US
}

void msleep(u32 milliseconds)
{
	u32 start = RTC(0x10) | (RTC(0xC)<< 10);
	while (((RTC(0x10) | (RTC(0xC)<< 10)) - start) <= milliseconds)
		;
}

void usleep(u32 microseconds)
{
	u32 start = TMR(0x10);
	while ((TMR(0x10) - start) <= microseconds)
		;
}

void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops)
{
	for(u32 i = 0; i < num_ops; i++)
		base[ops[i].off] = ops[i].val;
}

#define CRC32C_POLY 0x82F63B78
u32 crc32c(const void *buf, u32 len)
{
	const u8 *cbuf = (const u8 *)buf;
	u32 crc = 0xFFFFFFFF;
	while (len--)
	{
		crc ^= *cbuf++;
		for (int i = 0; i < 8; i++)
			crc = crc & 1 ? (crc >> 1) ^ CRC32C_POLY : crc >> 1;
	}
	return ~crc;
}

u32 memcmp32sparse(const u32 *buf1, const u32 *buf2, u32 len)
{
	u32 len32 = len / 4;

	if (!(len32 % 32))
	{
		while (len32)
		{
			len32 -= 32;
			if(buf1[len32] != buf2[len32])
				return 1;
		}
	}
	else
	{
		while (len32)
		{
			len32 -= 32;
			if(buf1[len32] != buf2[len32])
				return 1;
			if (len32 < 32)
				return 0;
		}
	}

	return 0;
}

char *itoa(int value, char *str, int base)
{
	char *p = str;
	const char lut[] = "0123456789abcdef";
	bool negative = false;

	if (base < 2 || base > 16)
	{
		*p = '\0';
		return str;
	}

	if (value < 0)
	{
		value = -value;
		negative = true;
	}

	if (negative)
		p++;
	for (int v = value; v; v /= base)
		p++;
	*p-- = '\0';
	for (int v = value; v; v /= base)
		*p-- = lut[v % base];
	if (negative)
		*p-- = '-';

	return str;
}
