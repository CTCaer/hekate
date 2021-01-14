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

#ifndef __TMP451_H_
#define __TMP451_H_

#include <utils/types.h>

#define TMP451_I2C_ADDR 0x4C

#define TMP451_PCB_TEMP_REG    0x00
#define TMP451_SOC_TEMP_REG    0x01

#define TMP451_CONFIG_REG      0x09
#define TMP451_CNV_RATE_REG    0x0A

#define TMP451_SOC_TMP_DEC_REG 0x10
#define TMP451_PCB_TMP_DEC_REG 0x15

#define TMP451_SOC_TMP_OFH_REG 0x11
#define TMP451_SOC_TMP_OFL_REG 0x12

// If input is false, the return value is packed. MSByte is the integer in oC
// and the LSByte is the decimal point truncated to 2 decimal places.
// Otherwise it's an integer oC.
u16 tmp451_get_soc_temp(bool integer);
u16 tmp451_get_pcb_temp(bool integer);
void tmp451_init();
void tmp451_end();

#endif /* __TMP451_H_ */
