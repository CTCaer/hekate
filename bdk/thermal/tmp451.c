/*
 * SOC/PCB Temperature driver for Nintendo Switch's TI TMP451
 *
 * Copyright (c) 2018-2020 CTCaer
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

#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/t210.h>
#include <thermal/tmp451.h>

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

void tmp451_init()
{
	// Disable ALARM and Range to 0 - 127 oC.
	i2c_send_byte(I2C_1, TMP451_I2C_ADDR, TMP451_CONFIG_REG, 0x80);

	// Set remote sensor offsets based on SoC.
	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210)
	{
		// Set offset to 0 oC for Erista.
		i2c_send_byte(I2C_1, TMP451_I2C_ADDR, TMP451_SOC_TMP_OFH_REG, 0);
		i2c_send_byte(I2C_1, TMP451_I2C_ADDR, TMP451_SOC_TMP_OFL_REG, 0);
	}
	else
	{
		// Set offset to -12.5 oC for Mariko.
		i2c_send_byte(I2C_1, TMP451_I2C_ADDR, TMP451_SOC_TMP_OFH_REG, 0xF3); // - 13  oC.
		i2c_send_byte(I2C_1, TMP451_I2C_ADDR, TMP451_SOC_TMP_OFL_REG, 0x80); // + 0.5 oC.
	}

	// Set conversion rate to 32/s and make a read to update the reg.
	i2c_send_byte(I2C_1, TMP451_I2C_ADDR, TMP451_CNV_RATE_REG, 9);
	tmp451_get_soc_temp(false);

	// Set rate to every 4 seconds.
	i2c_send_byte(I2C_1, TMP451_I2C_ADDR, TMP451_CNV_RATE_REG, 2);
}

void tmp451_end()
{
	// Place into shutdown mode to conserve power.
	i2c_send_byte(I2C_1, TMP451_I2C_ADDR, TMP451_CONFIG_REG, 0xC0);
}
