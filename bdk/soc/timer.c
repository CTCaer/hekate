/*
 * Timer/Watchdog driver for Tegra X1
 *
 * Copyright (c) 2019 CTCaer
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

#include <soc/bpmp.h>
#include <soc/irq.h>
#include <soc/timer.h>
#include <soc/t210.h>
#include <utils/types.h>

#define EXCP_TYPE_ADDR 0x4003FFF8
#define  EXCP_TYPE_WDT 0x544457 // "WDT".

#define USE_RTC_TIMER

u32 get_tmr_s()
{
	(void)RTC(APBDEV_RTC_MILLI_SECONDS);
	return (u32)RTC(APBDEV_RTC_SECONDS);
}

u32 get_tmr_ms()
{
	// The registers must be read with the following order:
	// RTC_MILLI_SECONDS (0x10) -> RTC_SHADOW_SECONDS (0xC)
	return (u32)(RTC(APBDEV_RTC_MILLI_SECONDS) + (RTC(APBDEV_RTC_SHADOW_SECONDS) * 1000));
}

u32 get_tmr_us()
{
	return (u32)TMR(TIMERUS_CNTR_1US);
}

void msleep(u32 ms)
{
#ifdef USE_RTC_TIMER
	u32 start = (u32)RTC(APBDEV_RTC_MILLI_SECONDS) + (RTC(APBDEV_RTC_SHADOW_SECONDS) * 1000);
	// Casting to u32 is important!
	while (((u32)(RTC(APBDEV_RTC_MILLI_SECONDS) + (RTC(APBDEV_RTC_SHADOW_SECONDS) * 1000)) - start) <= ms)
		;
#else
	bpmp_msleep(ms);
#endif
}

void usleep(u32 us)
{
#ifdef USE_RTC_TIMER
	u32 start = (u32)TMR(TIMERUS_CNTR_1US);

	// Check if timer is at upper limits and use BPMP sleep so it doesn't wake up immediately.
	if ((start + us) < start)
		bpmp_usleep(us);
	else
		while ((u32)(TMR(TIMERUS_CNTR_1US) - start) <= us) // Casting to u32 is important!
			;
#else
	bpmp_usleep(us);
#endif
}

// Instruction wait loop. Each loop is 3 cycles (SUBS+BGT). Usage: isleep(ILOOP(instr)). Base 408MHz: 7.35ns.
void __attribute__((target("arm"))) isleep(u32 is)
{
	asm volatile( "0:" "SUBS %[is_cnt], #1;" "BGT 0b;" : [is_cnt] "+r" (is));
}

void timer_usleep(u32 us)
{
	TMR(TIMER_TMR8_TMR_PTV) = TIMER_EN | us;

	irq_wait_event(IRQ_TMR8);

	TMR(TIMER_TMR8_TMR_PCR) = TIMER_INTR_CLR;
}

void watchdog_start(u32 us, u32 mode)
{
	// WDT4 is for BPMP.
	TMR(TIMER_WDT4_UNLOCK_PATTERN) = TIMER_MAGIC_PTRN;
	TMR(TIMER_TMR9_TMR_PTV) = TIMER_EN | TIMER_PER_EN | us;
	TMR(TIMER_WDT4_CONFIG)  = TIMER_SRC(9) | TIMER_PER(1) | mode;
	TMR(TIMER_WDT4_COMMAND) = TIMER_START_CNT;
}

void watchdog_end()
{
	// WDT4 is for BPMP.
	TMR(TIMER_TMR9_TMR_PTV) = 0;
	TMR(TIMER_WDT4_UNLOCK_PATTERN) = TIMER_MAGIC_PTRN;
	TMR(TIMER_WDT4_COMMAND) = TIMER_START_CNT; // Re-arm to clear any interrupts.
	TMR(TIMER_WDT4_COMMAND) = TIMER_CNT_DISABLE;
	TMR(TIMER_TMR9_TMR_PCR) = TIMER_INTR_CLR;
}

void watchdog_handle()
{
	// Disable watchdog and clear its interrupts.
	watchdog_end();

	// Set watchdog magic.
	*(u32 *)EXCP_TYPE_ADDR = EXCP_TYPE_WDT;
}

bool watchdog_fired()
{
	// Return if watchdog got fired. User handles clearing.
	return (*(u32 *)EXCP_TYPE_ADDR == EXCP_TYPE_WDT);
}
