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

#ifndef _TIMER_H_
#define _TIMER_H_

#include <utils/types.h>

// TMR registers.
#define TIMERUS_CNTR_1US   (0x10 + 0x0)
#define TIMERUS_USEC_CFG   (0x10 + 0x4)
#define TIMER_TMR8_TMR_PTV 0x78
#define TIMER_TMR9_TMR_PTV 0x80
#define  TIMER_PER_EN       BIT(30)
#define  TIMER_EN           BIT(31)
#define TIMER_TMR8_TMR_PCR 0x7C
#define TIMER_TMR9_TMR_PCR 0x8C
#define  TIMER_INTR_CLR     BIT(30)

// WDT registers.
#define TIMER_WDT4_CONFIG         (0x100 + 0x80)
#define  TIMER_SRC(TMR)    ((TMR) & 0xF)
#define  TIMER_PER(PER)    (((PER) & 0xFF) << 4)
#define  TIMER_IRQENABL_EN BIT(12)
#define  TIMER_FIQENABL_EN BIT(13)
#define  TIMER_SYSRESET_EN BIT(14)
#define  TIMER_PMCRESET_EN BIT(15)
#define TIMER_WDT4_COMMAND        (0x108 + 0x80)
#define  TIMER_START_CNT   BIT(0)
#define  TIMER_CNT_DISABLE BIT(1)
#define TIMER_WDT4_UNLOCK_PATTERN (0x10C + 0x80)
#define  TIMER_MAGIC_PTRN  0xC45A

u32  get_tmr_us();
u32  get_tmr_ms();
u32  get_tmr_s();
void usleep(u32 us);
void msleep(u32 ms);
#define ILOOP(is) ((is) / 3)
void isleep(u32 is);

void timer_usleep(u32 us);

void watchdog_start(u32 us, u32 mode);
void watchdog_end();
void watchdog_handle();
bool watchdog_fired();

#endif
