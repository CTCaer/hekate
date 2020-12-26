/*
 * BPMP-Lite IRQ driver for Tegra X1
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

#include <string.h>

#include "irq.h"
#include <soc/t210.h>
#include <gfx_utils.h>
#include <mem/heap.h>

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

extern void irq_disable();
extern void irq_enable_cpu_irq_exceptions();
extern void irq_disable_cpu_irq_exceptions();

typedef struct _irq_ctxt_t
{
	u32  irq;
	int  (*handler)(u32 irq, void *data);
	void *data;
	u32  flags;
} irq_ctxt_t;

bool irq_init_done = false;
irq_ctxt_t irqs[IRQ_MAX_HANDLERS];

static void _irq_enable_source(u32 irq)
{
	u32 ctrl_idx = irq >> 5;
	u32 bit = irq % 32;

	// Set as normal IRQ.
	ICTLR(ctrl_idx, PRI_ICTLR_COP_IEP_CLASS) &= ~BIT(bit);

	// Enable IRQ source.
	ICTLR(ctrl_idx, PRI_ICTLR_COP_IER_SET) = BIT(bit);
}

static void _irq_disable_source(u32 irq)
{
	u32 ctrl_idx = irq >> 5;
	u32 bit = irq % 32;

	// Disable IRQ source.
	ICTLR(ctrl_idx, PRI_ICTLR_COP_IER_CLR) = BIT(bit);
}

static void _irq_disable_and_ack_all()
{
	// Disable and ack all IRQ sources.
	for (u32 ctrl_idx = 0; ctrl_idx < 6; ctrl_idx++)
	{
		u32 enabled_irqs = ICTLR(ctrl_idx, PRI_ICTLR_COP_IER);
		ICTLR(ctrl_idx, PRI_ICTLR_COP_IER_CLR) = enabled_irqs;
		ICTLR(ctrl_idx, PRI_ICTLR_FIR_CLR) = enabled_irqs;
	}
}

static void _irq_ack_source(u32 irq)
{
	u32 ctrl_idx = irq >> 5;
	u32 bit = irq % 32;

	// Force stop the interrupt as it's serviced here.
	ICTLR(ctrl_idx, PRI_ICTLR_FIR_CLR) = BIT(bit);
}

void irq_free(u32 irq)
{
	for (u32 idx = 0; idx < IRQ_MAX_HANDLERS; idx++)
	{
		if (irqs[idx].irq == irq && irqs[idx].handler)
		{
			irqs[idx].irq = 0;
			irqs[idx].handler = NULL;
			irqs[idx].data = NULL;
			irqs[idx].flags = 0;

			_irq_disable_source(irq);
		}
	}
}

static void _irq_free_all()
{
	for (u32 idx = 0; idx < IRQ_MAX_HANDLERS; idx++)
	{
		if (irqs[idx].handler)
		{
			_irq_disable_source(irqs[idx].irq);

			irqs[idx].irq = 0;
			irqs[idx].handler = NULL;
			irqs[idx].data = NULL;
			irqs[idx].flags = 0;
		}
	}
}

static irq_status_t _irq_handle_source(u32 irq)
{
	int status = IRQ_NONE;

	_irq_disable_source(irq);
	_irq_ack_source(irq);

	u32 idx;
	for (idx = 0; idx < IRQ_MAX_HANDLERS; idx++)
	{
		if (irqs[idx].irq == irq)
		{
			status = irqs[idx].handler(irqs[idx].irq, irqs[idx].data);

			if (status == IRQ_HANDLED)
				break;
		}
	}

	// Do not re-enable if not handled.
	if (status == IRQ_NONE)
		return status;

	if (irqs[idx].flags & IRQ_FLAG_ONE_OFF)
		irq_free(irq);
	else
		_irq_enable_source(irq);

	return status;
}

void irq_handler()
{
	// Get IRQ source.
	u32 irq = EXCP_VEC(EVP_COP_IRQ_STS) & 0xFF;

	if (!irq_init_done)
	{
		_irq_disable_source(irq);
		_irq_ack_source(irq);

		return;
	}

	DPRINTF("IRQ: %d\n", irq);

	int err = _irq_handle_source(irq);

	if (err == IRQ_NONE)
	{
		DPRINTF("Unhandled IRQ got disabled: %d!\n", irq);
	}
}

static void _irq_init()
{
	_irq_disable_and_ack_all();
	memset(irqs, 0, sizeof(irq_ctxt_t) * IRQ_MAX_HANDLERS);
	irq_init_done = true;
}

void irq_end()
{
	if (!irq_init_done)
		return;

	_irq_free_all();
	irq_disable_cpu_irq_exceptions();
	irq_init_done = false;
}

void irq_wait_event(u32 irq)
{
	irq_disable_cpu_irq_exceptions();

	_irq_enable_source(irq);

	// Halt BPMP and wait for the IRQ. No need to use WAIT_EVENT + LIC_IRQ when BPMP serves the IRQ.
	FLOW_CTLR(FLOW_CTLR_HALT_COP_EVENTS) = HALT_COP_STOP_UNTIL_IRQ;

	_irq_disable_source(irq);
	_irq_ack_source(irq);

	irq_enable_cpu_irq_exceptions();
}

void irq_disable_wait_event()
{
	irq_enable_cpu_irq_exceptions();
}

irq_status_t irq_request(u32 irq, irq_handler_t handler, void *data, irq_flags_t flags)
{
	if (!irq_init_done)
		_irq_init();

	for (u32 idx = 0; idx < IRQ_MAX_HANDLERS; idx++)
	{
		if (irqs[idx].handler == NULL ||
			(irqs[idx].irq == irq && irqs[idx].flags & IRQ_FLAG_REPLACEABLE))
		{
			DPRINTF("Registered handler, IRQ: %d, Slot: %d\n", irq, idx);
			DPRINTF("Handler: %08p, Flags: %x\n", (u32)handler, flags);

			irqs[idx].irq = irq;
			irqs[idx].handler = handler;
			irqs[idx].data = data;
			irqs[idx].flags = flags;

			_irq_enable_source(irq);

			return IRQ_ENABLED;
		}
		else if (irqs[idx].irq == irq)
			return IRQ_ALREADY_REGISTERED;
	}

	return IRQ_NO_SLOTS_AVAILABLE;
}

void __attribute__ ((target("arm"))) fiq_setup()
{
/*
	asm volatile("mrs r12, cpsr\n\t"
		"bic r12, r12, #0x1F\n\t"
		"orr r12, r12, #0x11\n\t"
		"msr cpsr_c, r12\n\t");

	register volatile char *text asm ("r8");
	register volatile char *uart_tx asm ("r9");
	register int len asm ("r10");

	len = 5;
	uart_tx = (char *)0x70006040;
	memcpy((char *)text, "FIQ\r\n", len);
	*uart_tx = 0;

	asm volatile("mrs r12, cpsr\n"
		"orr r12, r12, #0x1F\n"
		"msr cpsr_c, r12");
*/
}

void  __attribute__ ((target("arm"), interrupt ("FIQ"))) fiq_handler()
{
/*
	register volatile char *text asm ("r8");
	register volatile char *uart_tx asm ("r9");
	register int len asm ("r10");

	while (len)
	{
		*uart_tx = *text++;
		len--;
	}
*/
}
