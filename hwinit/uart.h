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

#ifndef _UART_H_
#define _UART_H_

#include "types.h"

#define UART_A 0
#define UART_B 1
#define UART_C 2
//TODO: define clock inits for those.
/*#define UART_D 3
#define UART_E 4*/

#define BAUD_115200 115200

#define UART_TX_IDLE 0x00000001
#define UART_RX_IDLE 0x00000002
#define UART_TX_FIFO_FULL 0x100
#define UART_RX_FIFO_EMPTY 0x200

typedef struct _uart_t
{
	/* 0x00 */ vu32 UART_THR_DLAB;
	/* 0x04 */ vu32 UART_IER_DLAB;
	/* 0x08 */ vu32 UART_IIR_FCR;
	/* 0x0C */ vu32 UART_LCR;
	/* 0x10 */ vu32 UART_MCR;
	/* 0x14 */ vu32 UART_LSR;
	/* 0x18 */ vu32 UART_MSR;
	/* 0x1C */ vu32 UART_SPR;
	/* 0x20 */ vu32 UART_IRDA_CSR;
	/* 0x24 */ vu32 UART_RX_FIFO_CFG;
	/* 0x28 */ vu32 UART_MIE;
	/* 0x2C */ vu32 UART_VENDOR_STATUS;
	/* 0x30 */ u8 _pad_30[0xC];
	/* 0x3C */ vu32 UART_ASR;
} uart_t;

void uart_init(u32 idx, u32 baud);
void uart_wait_idle(u32 idx, u32 which);
void uart_send(u32 idx, u8 *buf, u32 len);
void uart_recv(u32 idx, u8 *buf, u32 len);

#endif
