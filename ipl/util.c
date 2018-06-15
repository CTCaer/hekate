/*
* Copyright (c) 2018 naehrwert
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
#include "t210.h"

u32 get_tmr()
{
	return TMR(0x10);
}

void sleep(u32 ticks)
{
	u32 start = TMR(0x10);
	while (TMR(0x10) - start <= ticks)
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
