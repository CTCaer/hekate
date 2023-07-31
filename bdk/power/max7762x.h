/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2019-2020 CTCaer
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

#ifndef _MAX7762X_H_
#define _MAX7762X_H_

#include <utils/types.h>

/*
 * SDx actual min is 625 mV. Multipliers 0/1 reserved.
 * SD0 max is 1400 mV
 * SD1 max is 1550 mV
 * SD2 max is 3787.5 mV
 * SD3 max is 3787.5 mV
 */

/*
* Switch Power domains (max77620):
* Name  | Usage         | uV step | uV min | uV default | uV max  | Init
*-------+---------------+---------+--------+------------+---------+------------------
*  sd0  | SoC           | 12500   | 600000 |  625000    | 1400000 | 1.125V (pkg1.1)
*  sd1  | SDRAM         | 12500   | 600000 | 1125000    | 1125000 | 1.1V   (pkg1.1)
*  sd2  | ldo{0-1, 7-8} | 12500   | 600000 | 1325000    | 1350000 | 1.325V (pcv)
*  sd3  | 1.8V general  | 12500   | 600000 | 1800000    | 1800000 |
*  ldo0 | Display Panel | 25000   | 800000 | 1200000    | 1200000 | 1.2V   (pkg1.1)
*  ldo1 | XUSB, PCIE    | 25000   | 800000 | 1050000    | 1050000 | 1.05V  (pcv)
*  ldo2 | SDMMC1        | 50000   | 800000 | 1800000    | 3300000 |
*  ldo3 | GC ASIC       | 50000   | 800000 | 3100000    | 3100000 | 3.1V   (pcv)
*  ldo4 | RTC           | 12500   | 800000 |  850000    |  850000 | 0.85V  (AO, pcv)
*  ldo5 | GC Card       | 50000   | 800000 | 1800000    | 1800000 | 1.8V   (pcv)
*  ldo6 | Touch, ALS    | 50000   | 800000 | 2900000    | 2900000 | 2.9V   (pcv)
*  ldo7 | XUSB          | 50000   | 800000 | 1050000    | 1050000 | 1.05V  (pcv)
*  ldo8 | XUSB, DP, MCU | 50000   | 800000 | 1050000    | 2800000 | 1.05V/2.8V (pcv)
*/


// GPIOs T210: 3: 3.3V, 5: CPU PMIC, 6: GPU PMIC, 7: DSI/VI 1.2V powered by ldo0.

/*
 * OTP:  T210 - T210B01:
 * SD0:  1.0V   1.05V - SoC. EN Based on FPSSRC.
 * SD1:  1.15V  1.1V  - DRAM for T210. EN Based on FPSSRC.
 * SD2:  1.35V  1.35V
 * SD3:  1.8V   1.8V
 * All powered off?
 * LDO0:   --   --    - Display
 * LDO1: 1.05V  1.05V
 * LDO2:   --   --    - SD
 * LDO3: 3.1V   3.1V  - GC ASIC
 * LDO4: 1.0V   0.8V  - Needed for RTC domain on T210.
 * LDO5: 3.1V   3.1V
 * LDO6: 2.8V   2.9V  - Touch.
 * LDO7: 1.05V  1.0V
 * LDO8: 1.05V  1.0V
 */

/*
* MAX77620_AME_GPIO: control GPIO modes (bits 0 - 7 correspond to GPIO0 - GPIO7); 0 -> GPIO, 1 -> alt-mode
* MAX77620_REG_GPIOx: 0x9 sets output and enable
*/

/*! MAX77620 partitions. */
#define REGULATOR_SD0  0
#define REGULATOR_SD1  1
#define REGULATOR_SD2  2
#define REGULATOR_SD3  3
#define REGULATOR_LDO0 4
#define REGULATOR_LDO1 5
#define REGULATOR_LDO2 6
#define REGULATOR_LDO3 7
#define REGULATOR_LDO4 8
#define REGULATOR_LDO5 9
#define REGULATOR_LDO6 10
#define REGULATOR_LDO7 11
#define REGULATOR_LDO8 12
#define REGULATOR_CPU0 13 // T210 CPU.
#define REGULATOR_GPU0 14 // T210 CPU.
#define REGULATOR_CPU1 15 // T210B01 CPU.
#define REGULATOR_RAM0 16 // T210B01 RAM for PHASE211.
//#define REGULATOR_GPU1 17 // T210B01 CPU.
#define REGULATOR_MAX  REGULATOR_RAM0

#define MAX77621_CPU_I2C_ADDR 0x1B
#define MAX77621_GPU_I2C_ADDR 0x1C

#define MAX77621_REG_VOUT     0x00
#define MAX77621_REG_VOUT_DVS 0x01
#define MAX77621_REG_CONTROL1 0x02
#define MAX77621_REG_CONTROL2 0x03
#define MAX77621_REG_CHIPID1  0x04
#define MAX77621_REG_CHIPID2  0x05

/* MAX77621_VOUT_DVC_DVS */
#define MAX77621_DVC_DVS_VOLT_MASK    0x7F
#define MAX77621_DVC_DVS_ENABLE_SHIFT 7
#define MAX77621_DVC_DVS_ENABLE_MASK  (1 << MAX77621_DVC_DVS_ENABLE_SHIFT)

/* MAX77621_VOUT */
#define MAX77621_VOUT_DISABLE     0
#define MAX77621_VOUT_ENABLE      1
#define MAX77621_VOUT_ENABLE_MASK (MAX77621_VOUT_ENABLE << MAX77621_DVC_DVS_ENABLE_SHIFT)

/* MAX77621_CONTROL1 */
#define MAX77621_RAMP_12mV_PER_US  0x0
#define MAX77621_RAMP_25mV_PER_US  0x1
#define MAX77621_RAMP_50mV_PER_US  0x2
#define MAX77621_RAMP_200mV_PER_US 0x3
#define MAX77621_RAMP_MASK         0x3

#define MAX77621_FREQSHIFT_9PER    BIT(2)
#define MAX77621_BIAS_ENABLE       BIT(3)
#define MAX77621_AD_ENABLE         BIT(4)
#define MAX77621_NFSR_ENABLE       BIT(5)
#define MAX77621_FPWM_EN_M         BIT(6)
#define MAX77621_SNS_ENABLE        BIT(7)

/* MAX77621_CONTROL2 */
#define MAX77621_INDUCTOR_MIN_30_PER  0
#define MAX77621_INDUCTOR_NOMINAL     1
#define MAX77621_INDUCTOR_PLUS_30_PER 2
#define MAX77621_INDUCTOR_PLUS_60_PER 3
#define MAX77621_INDUCTOR_MASK        3

#define MAX77621_CKKADV_TRIP_75mV_PER_US          (0  << 2)
#define MAX77621_CKKADV_TRIP_150mV_PER_US         (1u << 2)
#define MAX77621_CKKADV_TRIP_DISABLE              (3u << 2)
#define MAX77621_CKKADV_TRIP_MASK                 (3u << 2)

#define MAX77621_FT_ENABLE       BIT(4)
#define MAX77621_DISCH_ENABLE    BIT(5)
#define MAX77621_WDTMR_ENABLE    BIT(6)
#define MAX77621_T_JUNCTION_120  BIT(7)

#define MAX77621_CPU_CTRL1_POR_DEFAULT  (MAX77621_RAMP_50mV_PER_US)
#define MAX77621_CPU_CTRL1_HOS_DEFAULT  (MAX77621_AD_ENABLE               | \
										 MAX77621_NFSR_ENABLE             | \
										 MAX77621_SNS_ENABLE              | \
										 MAX77621_RAMP_12mV_PER_US)
#define MAX77621_CPU_CTRL2_POR_DEFAULT  (MAX77621_T_JUNCTION_120          | \
										 MAX77621_FT_ENABLE               | \
										 MAX77621_CKKADV_TRIP_DISABLE     | \
										 MAX77621_INDUCTOR_NOMINAL)
#define MAX77621_CPU_CTRL2_HOS_DEFAULT  (MAX77621_T_JUNCTION_120          | \
										 MAX77621_WDTMR_ENABLE            | \
										 MAX77621_CKKADV_TRIP_75mV_PER_US | \
										 MAX77621_INDUCTOR_NOMINAL)

#define MAX77621_CTRL_HOS_CFG 0
#define MAX77621_CTRL_POR_CFG 1

int  max77620_regulator_get_status(u32 id);
int  max77620_regulator_config_fps(u32 id);
int  max7762x_regulator_set_voltage(u32 id, u32 uv);
int  max7762x_regulator_enable(u32 id, bool enable);
void max77620_config_gpio(u32 id, bool enable);
void max77620_config_default();
void max77620_low_battery_monitor_config(bool enable);

void max77621_config_default(u32 id, bool por);

#endif
