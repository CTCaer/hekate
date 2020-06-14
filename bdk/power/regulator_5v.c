/*
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

#include <soc/gpio.h>
#include <soc/pinmux.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <utils/types.h>

static u8 reg_5v_dev = 0;

void regulator_enable_5v(u8 dev)
{
	// The power supply selection from battery or USB is automatic.
	if (!reg_5v_dev)
	{
		// Fan and Rail power from internal 5V regulator (battery).
		PINMUX_AUX(PINMUX_AUX_SATA_LED_ACTIVE) = 1;
		gpio_config(GPIO_PORT_A, GPIO_PIN_5, GPIO_MODE_GPIO);
		gpio_output_enable(GPIO_PORT_A, GPIO_PIN_5, GPIO_OUTPUT_ENABLE);
		gpio_write(GPIO_PORT_A, GPIO_PIN_5, GPIO_HIGH);

		// Fan and Rail power from USB 5V VDD.
		PINMUX_AUX(PINMUX_AUX_USB_VBUS_EN0) = PINMUX_LPDR | 1;
		gpio_config(GPIO_PORT_CC, GPIO_PIN_4, GPIO_MODE_GPIO);
		gpio_output_enable(GPIO_PORT_CC, GPIO_PIN_4, GPIO_OUTPUT_ENABLE);
		gpio_write(GPIO_PORT_CC, GPIO_PIN_4, GPIO_HIGH);

		// Make sure GPIO power is enabled.
		PMC(APBDEV_PMC_NO_IOPOWER) &= ~PMC_NO_IOPOWER_GPIO_IO_EN;
		// Override power detect for GPIO AO IO rails.
		PMC(APBDEV_PMC_PWR_DET_VAL) &= ~PMC_PWR_DET_GPIO_IO_EN;
	}
	reg_5v_dev |= dev;
}

void regulator_disable_5v(u8 dev)
{
	reg_5v_dev &= ~dev;

	if (!reg_5v_dev)
	{
		// Rail power from internal 5V regulator (battery).
		gpio_write(GPIO_PORT_A, GPIO_PIN_5, GPIO_LOW);
		gpio_output_enable(GPIO_PORT_A, GPIO_PIN_5, GPIO_OUTPUT_DISABLE);
		gpio_config(GPIO_PORT_A, GPIO_PIN_5, GPIO_MODE_SPIO);
		PINMUX_AUX(PINMUX_AUX_SATA_LED_ACTIVE) = PINMUX_PARKED | PINMUX_INPUT_ENABLE;

		// Rail power from USB 5V VDD.
		gpio_write(GPIO_PORT_CC, GPIO_PIN_4, GPIO_LOW);
		gpio_output_enable(GPIO_PORT_CC, GPIO_PIN_4, GPIO_OUTPUT_DISABLE);
		gpio_config(GPIO_PORT_CC, GPIO_PIN_4, GPIO_MODE_SPIO);
		PINMUX_AUX(PINMUX_AUX_USB_VBUS_EN0) = PINMUX_IO_HV | PINMUX_LPDR | PINMUX_PARKED | PINMUX_INPUT_ENABLE;

		// GPIO AO IO rails.
		PMC(APBDEV_PMC_PWR_DET_VAL) |= PMC_PWR_DET_GPIO_IO_EN;
	}
}

bool regulator_get_5v_dev_enabled(u8 dev)
{
	return (reg_5v_dev & dev);
}
