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

#ifndef _UART_H_
#define _UART_H_

#include <utils/types.h>

#define UART_A 0
#define UART_B 1
#define UART_C 2
#define UART_D 3
#define UART_E 4

#define BAUD_115200 115200

#define UART_TX_IDLE BIT(0)
#define UART_RX_RDYR BIT(1)

#define UART_TX_FIFO_FULL  BIT(8)
#define UART_RX_FIFO_EMPTY BIT(9)

#define UART_INVERT_RXD BIT(0)
#define UART_INVERT_TXD BIT(1)
#define UART_INVERT_CTS BIT(2)
#define UART_INVERT_RTS BIT(3)

#define UART_IER_DLAB_IE_EORD BIT(5)

#define UART_LCR_WORD_LENGTH_8 0x3
#define UART_LCR_STOP BIT(2)
#define UART_LCR_DLAB BIT(7)

#define UART_LSR_RDR   BIT(0)
#define UART_LSR_THRE  BIT(5)
#define UART_LSR_TMTY  BIT(6)
#define UART_LSR_FIFOE BIT(7)

#define UART_IIR_FCR_EN_FIFO BIT(0)
#define UART_IIR_FCR_RX_CLR  BIT(1)
#define UART_IIR_FCR_TX_CLR  BIT(2)

#define UART_IIR_NO_INT   BIT(0)
#define UART_IIR_INT_MASK 0xF
/* Custom returned interrupt results. Actual interrupts are -1 */
#define UART_IIR_NOI   0 // No interrupt.
#define UART_IIR_MSI   1 // Modem status interrupt.
#define UART_IIR_THRI  2 // Transmitter holding register empty.
#define UART_IIR_RDI   3 // Receiver data interrupt.
#define UART_IIR_ERROR 4 // Overrun Error, Parity Error, Framing Error, Break.
#define UART_IIR_REDI  5 // Receiver end of data interrupt.
#define UART_IIR_RDTI  7 // Receiver data timeout interrupt.

#define UART_MCR_DTR    BIT(0)
#define UART_MCR_RTS    BIT(1)
#define UART_MCR_CTS_EN BIT(5)
#define UART_MCR_RTS_EN BIT(6)

#define UART_FIFO_SIZE  36

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

//! TODO: Commented out modes are not supported yet.
typedef enum _uart_mode_t
{
	UART_AO_TX_AO_RX = 0,
	//UART_MN_TX_AO_RX = UART_MCR_RTS | UART_MCR_DTR,
	UART_AO_TX_MN_RX = UART_MCR_RTS, // Up to 36 bytes read.
	//UART_MN_TX_AO_RX = UART_MCR_DTR,
	//UART_HW_TX_HW_RX = UART_MCR_RTS_EN | UART_MCR_CTS_EN,
	UART_AO_TX_HW_RX = UART_MCR_RTS_EN,
	//UART_HW_TX_AO_RX = UART_MCR_CTS_EN,
} uart_mode_t;

void uart_init(u32 idx, u32 baud, u32 mode);
void uart_wait_xfer(u32 idx, u32 which);
void uart_send(u32 idx, const u8 *buf, u32 len);
u32  uart_recv(u32 idx, u8 *buf, u32 len);
void uart_invert(u32 idx, bool enable, u32 invert_mask);
void uart_set_mode(u32 idx, u32 mode);
u32  uart_get_IIR(u32 idx);
void uart_set_IIR(u32 idx);
void uart_empty_fifo(u32 idx, u32 which);
#ifdef DEBUG_UART_PORT
void uart_printf(const char *fmt, ...);
#endif

#endif
