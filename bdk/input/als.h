/*
 * Ambient light sensor driver for Nintendo Switch's Rohm BH1730
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

#ifndef __ALS_H_
#define __ALS_H_

#include <utils/types.h>

#define BH1730_I2C_ADDR 0x29

#define BH1730_CMD_MAGIC   0x80
#define BH1730_CMD_SETADDR 0x00
#define BH1730_CMD_SPECCMD 0x60
#define  BH1730_SPECCMD_RESET 0x4

#define BH1730_CONTROL_REG   0x00
#define  BH1730_CTL_ADC_VALID 0x10
#define  BH1730_CTL_ONE_TIME  0x08
#define  BH1730_CTL_DAT0_ONLY 0x04
#define  BH1730_CTL_ADC_EN    0x02
#define  BH1730_CTL_POWER_ON  0x01
#define BH1730_TIMING_REG    0x01
#define BH1730_GAIN_REG      0x07
#define  BH1730_GAIN_1X      0x00
#define  BH1730_GAIN_2X      0x01
#define  BH1730_GAIN_64X     0x02
#define  BH1730_GAIN_128X    0x03
#define BH1730_DATA0LOW_REG  0x14
#define BH1730_DATA0HIGH_REG 0x15
#define BH1730_DATA1LOW_REG  0x16
#define BH1730_DATA1HIGH_REG 0x17

#define BH1730_ADDR(reg) (BH1730_CMD_MAGIC | BH1730_CMD_SETADDR | (reg))
#define BH1730_SPEC(cmd) (BH1730_CMD_MAGIC | BH1730_CMD_SPECCMD | (cmd))

typedef struct _als_ctxt_t
{
	u32  lux;
	bool over_limit;
	u32  vi_light;
	u32  ir_light;
	u8   gain;
	u8   cycle;
} als_ctxt_t;

void set_als_cfg(als_ctxt_t *als_ctxt, u8 gain, u8 cycle);
void get_als_lux(als_ctxt_t *als_ctxt);
u8   als_power_on(als_ctxt_t *als_ctxt);

#endif /* __ALS_H_ */
