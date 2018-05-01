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

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "types.h"

/*! Clock registers. */
#define CLK_RST_CONTROLLER_RST_DEVICES_L 0x4
#define CLK_RST_CONTROLLER_RST_DEVICES_U 0xC
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_L 0x10
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_H 0x14
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_U 0x18
#define CLK_RST_CONTROLLER_CCLK_BURST_POLICY 0x20
#define CLK_RST_CONTROLLER_SUPER_CCLK_DIVIDER 0x24
#define CLK_RST_CONTROLLER_SCLK_BURST_POLICY 0x28
#define CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER 0x2C
#define CLK_RST_CONTROLLER_CLK_SYSTEM_RATE 0x30
#define CLK_RST_CONTROLLER_MISC_CLK_ENB 0x48
#define CLK_RST_CONTROLLER_OSC_CTRL 0x50
#define CLK_RST_CONTROLLER_PLLX_BASE 0xE0
#define CLK_RST_CONTROLLER_PLLX_MISC 0xE4
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1 0x150
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC2 0x154
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4 0x164
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3 0x1BC
#define CLK_RST_CONTROLLER_CLK_SOURCE_EMC 0x19C
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_X 0x280
#define CLK_RST_CONTROLLER_CLK_ENB_X_SET 0x284
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_Y 0x298
#define CLK_RST_CONTROLLER_CLK_ENB_Y_SET 0x29C
#define CLK_RST_CONTROLLER_RST_DEV_L_SET 0x300
#define CLK_RST_CONTROLLER_RST_DEV_L_CLR 0x304
#define CLK_RST_CONTROLLER_RST_DEV_H_SET 0x308
#define CLK_RST_CONTROLLER_RST_DEV_U_SET 0x310
#define CLK_RST_CONTROLLER_RST_DEV_U_CLR 0x314
#define CLK_RST_CONTROLLER_CLK_ENB_L_SET 0x320
#define CLK_RST_CONTROLLER_CLK_ENB_L_CLR 0x324
#define CLK_RST_CONTROLLER_CLK_ENB_H_SET 0x328
#define CLK_RST_CONTROLLER_CLK_ENB_U_SET 0x330
#define CLK_RST_CONTROLLER_CLK_ENB_U_CLR 0x334
#define CLK_RST_CONTROLLER_RST_DEVICES_V 0x358
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_V 0x360
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_W 0x364
#define CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2 0x388
#define CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT 0x3B4
#define CLK_RST_CONTROLLER_CLK_ENB_V_SET 0x440
#define CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR 0x454
#define CLK_RST_CONTROLLER_PLLX_MISC_3 0x518
#define CLK_RST_CONTROLLER_SPARE_REG0 0x55C
#define CLK_RST_CONTROLLER_PLLMB_BASE 0x5E8
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC_LEGACY_TM 0x694

/*! Generic clock descriptor. */
typedef struct _clock_t
{
	u32 reset;
	u32 enable;
	u32 source;
	u8 index;
	u8 clk_src;
	u8 clk_div;
} clock_t;

/*! Generic clock enable/disable. */
void clock_enable(const clock_t *clk);
void clock_disable(const clock_t *clk);

/*! Clock control for specific hardware portions. */
void clock_enable_fuse(u32 enable);
void clock_enable_uart(u32 idx);
void clock_enable_i2c(u32 idx);
void clock_enable_se();
void clock_enable_host1x();
void clock_disable_host1x();
void clock_enable_tsec();
void clock_disable_tsec();
void clock_enable_sor_safe();
void clock_disable_sor_safe();
void clock_enable_sor0();
void clock_disable_sor0();
void clock_enable_sor1();
void clock_disable_sor1();
void clock_enable_kfuse();
void clock_disable_kfuse();
void clock_enable_cl_dvfs();
void clock_enable_coresight();
void clock_sdmmc_config_clock_source(u32 *pout, u32 id, u32 val);
void clock_sdmmc_get_params(u32 *pout, u16 *pdivisor, u32 type);
int clock_sdmmc_is_not_reset_and_enabled(u32 id);
void clock_sdmmc_enable(u32 id, u32 val);
void clock_sdmmc_disable(u32 id);

#endif
