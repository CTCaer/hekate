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
#include "../gfx/di.h"
#include "../power/max77620.h"
#include "../rtc/max77620-rtc.h"
#include "../soc/i2c.h"
#include "../soc/pmc.h"
#include "../soc/t210.h"

extern void sd_unmount();

u32 get_tmr_s()
{
	return RTC(APBDEV_RTC_SECONDS);
}

u32 get_tmr_ms()
{
	// The registers must be read with the following order:
	// RTC_MILLI_SECONDS (0x10) -> RTC_SHADOW_SECONDS (0xC)
	return (RTC(APBDEV_RTC_MILLI_SECONDS) | (RTC(APBDEV_RTC_SHADOW_SECONDS) << 10));
}

u32 get_tmr_us()
{
	return TMR(TIMERUS_CNTR_1US); //TIMERUS_CNTR_1US
}

void msleep(u32 milliseconds)
{
	u32 start = RTC(APBDEV_RTC_MILLI_SECONDS) | (RTC(APBDEV_RTC_SHADOW_SECONDS) << 10);
	while (((RTC(APBDEV_RTC_MILLI_SECONDS) | (RTC(APBDEV_RTC_SHADOW_SECONDS) << 10)) - start) <= milliseconds)
		;
}

void usleep(u32 microseconds)
{
	u32 start = TMR(TIMERUS_CNTR_1US);
	// Casting to u32 is important!
	while ((u32)(TMR(TIMERUS_CNTR_1US) - start) <= microseconds)
		;
}

void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops)
{
	for(u32 i = 0; i < num_ops; i++)
		base[ops[i].off] = ops[i].val;
}

void panic(u32 val)
{
	// Set panic code.
	PMC(APBDEV_PMC_SCRATCH200) = val;
	//PMC(APBDEV_PMC_CRYPTO_OP) = PMC_CRYPTO_OP_SE_DISABLE;
	TMR(TIMER_WDT4_UNLOCK_PATTERN) = TIMER_MAGIC_PTRN;
	TMR(TIMER_TMR9_TMR_PTV) = TIMER_EN | TIMER_PER_EN;
	TMR(TIMER_WDT4_CONFIG)  = TIMER_SRC(9) | TIMER_PER(1) | TIMER_PMCRESET_EN;
	TMR(TIMER_WDT4_COMMAND) = TIMER_START_CNT;
	while (1)
		;
}

void reboot_normal()
{
	sd_unmount();
	display_end();

	panic(0x21); // Bypass fuse programming in package1.
}

void reboot_rcm()
{
	sd_unmount();
	display_end();

	PMC(APBDEV_PMC_SCRATCH0) = 2; // Reboot into rcm.
	PMC(APBDEV_PMC_CNTRL) |= PMC_CNTRL_MAIN_RST;

	while (true)
		usleep(1);
}

void power_off()
{
	sd_unmount();

	// Stop the alarm, in case we injected and powered off too fast.
	max77620_rtc_stop_alarm();

	//TODO: we should probably make sure all regulators are powered off properly.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_PWR_OFF);
}
