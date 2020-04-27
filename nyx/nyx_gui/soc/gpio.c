/*
 * Copyright (c) 2018 naehrwert
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

#include "gpio.h"
#include "irq.h"
#include "t210.h"

#define GPIO_BANK_IDX(port)      (port >> 2)

#define GPIO_CNF_OFFSET(port)     (0x00 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_OE_OFFSET(port)      (0x10 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_OUT_OFFSET(port)     (0x20 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_IN_OFFSET(port)      (0x30 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_INT_STA_OFFSET(port) (0x40 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_INT_ENB_OFFSET(port) (0x50 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_INT_LVL_OFFSET(port) (0x60 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_INT_CLR_OFFSET(port) (0x70 + ((port >> 2) << 8) + ((port % 4) << 2))

#define GPIO_CNF_MASKED_OFFSET(port)     (0x80 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_OE_MASKED_OFFSET(port)      (0x90 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_OUT_MASKED_OFFSET(port)     (0xA0 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_INT_STA_MASKED_OFFSET(port) (0xC0 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_INT_ENB_MASKED_OFFSET(port) (0xD0 + ((port >> 2) << 8) + ((port % 4) << 2))
#define GPIO_INT_LVL_MASKED_OFFSET(port) (0xE0 + ((port >> 2) << 8) + ((port % 4) << 2))

static u8 gpio_bank_irq_ids[8] = {
	IRQ_GPIO1, IRQ_GPIO2, IRQ_GPIO3, IRQ_GPIO4,
	IRQ_GPIO5, IRQ_GPIO6, IRQ_GPIO7, IRQ_GPIO8
};

void gpio_config(u32 port, u32 pins, int mode)
{
	u32 offset = GPIO_CNF_OFFSET(port);

	if (mode)
		GPIO(offset) |= pins;
	else
		GPIO(offset) &= ~pins;

	(void)GPIO(offset); // Commit the write.
}

void gpio_output_enable(u32 port, u32 pins, int enable)
{
	u32 port_offset = GPIO_OE_OFFSET(port);

	if (enable)
		GPIO(port_offset) |= pins;
	else
		GPIO(port_offset) &= ~pins;

	(void)GPIO(port_offset); // Commit the write.
}

void gpio_write(u32 port, u32 pins, int high)
{
	u32 port_offset = GPIO_OUT_OFFSET(port);

	if (high)
		GPIO(port_offset) |= pins;
	else
		GPIO(port_offset) &= ~pins;

	(void)GPIO(port_offset); // Commit the write.
}

int gpio_read(u32 port, u32 pins)
{
	u32 port_offset = GPIO_IN_OFFSET(port);

	return (GPIO(port_offset) & pins) ? 1 : 0;
}

static void _gpio_interrupt_clear(u32 port, u32 pins)
{
	u32 port_offset = GPIO_INT_CLR_OFFSET(port);

	GPIO(port_offset) |= pins;

	(void)GPIO(port_offset); // Commit the write.
}

int gpio_interrupt_status(u32 port, u32 pins)
{
	u32 port_offset = GPIO_INT_STA_OFFSET(port);
	u32 enabled = GPIO(GPIO_INT_ENB_OFFSET(port)) & pins;

	int status = ((GPIO(port_offset) & pins) && enabled) ? 1 : 0;

	// Clear the interrupt status.
	if (status)
		_gpio_interrupt_clear(port, pins);

	return status;
}

void gpio_interrupt_enable(u32 port, u32 pins, int enable)
{
	u32 port_offset = GPIO_INT_ENB_OFFSET(port);

	// Clear any possible stray interrupt.
	_gpio_interrupt_clear(port, pins);

	if (enable)
		GPIO(port_offset) |= pins;
	else
		GPIO(port_offset) &= ~pins;

	(void)GPIO(port_offset); // Commit the write.
}

void gpio_interrupt_level(u32 port, u32 pins, int high, int edge, int delta)
{
	u32 port_offset = GPIO_INT_LVL_OFFSET(port);

	u32 val = GPIO(port_offset);

	if (high)
		val |= pins;
	else
		val &= ~pins;

	if (edge)
		val |= pins << 8;
	else
		val &= ~(pins << 8);

	if (delta)
		val |= pins << 16;
	else
		val &= ~(pins << 16);

	GPIO(port_offset) = val;

	(void)GPIO(port_offset); // Commit the write.

	// Clear any possible stray interrupt.
	_gpio_interrupt_clear(port, pins);
}

u32 gpio_get_bank_irq_id(u32 port)
{
	u32 bank_idx = GPIO_BANK_IDX(port);

	return gpio_bank_irq_ids[bank_idx];
}