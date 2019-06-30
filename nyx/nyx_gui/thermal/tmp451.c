/*
 * SOC/PCB Temperature driver for Nintendo Switch's TI TMP451
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

#include "tmp451.h"
#include "../soc/i2c.h"

u16 tmp451_get_soc_temp(bool intenger)
{
	u8 val;
	u16 temp = 0;

	val = i2c_recv_byte(I2C_1, TMP451_I2C_ADDR, TMP451_SOC_TEMP_REG);
	if (intenger)
		return val;

	temp = val << 8;
	val = i2c_recv_byte(I2C_1, TMP451_I2C_ADDR, TMP451_SOC_TMP_DEC_REG);
	temp |= ((val >> 4) * 625) / 100;

	return temp;
}

u16 tmp451_get_pcb_temp(bool intenger)
{
	u8 val;
	u16 temp = 0;

	val = i2c_recv_byte(I2C_1, TMP451_I2C_ADDR, TMP451_PCB_TEMP_REG);
	if (intenger)
		return val;

	temp = val << 8;
	val = i2c_recv_byte(I2C_1, TMP451_I2C_ADDR, TMP451_PCB_TMP_DEC_REG);
	temp |= ((val >> 4) * 625) / 100;

	return temp;
}
