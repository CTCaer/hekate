/*
 * Copyright (c) 2019-2023 CTCaer
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

#include <soc/fuse.h>
#include <soc/gpio.h>
#include <soc/hw_init.h>
#include <soc/pinmux.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <utils/types.h>

static u8 reg_5v_dev = 0;
static bool usb_src = false;

void regulator_5v_enable(u8 dev)
{
	bool tegra_t210 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;

	// The power supply selection from battery or USB is automatic.
	if (!reg_5v_dev)
	{
		// Fan and Rail power from battery 5V regulator EN.
		PINMUX_AUX(PINMUX_AUX_SATA_LED_ACTIVE) = 1;
		gpio_direction_output(GPIO_PORT_A, GPIO_PIN_5, GPIO_HIGH);

		// Only Icosa has USB 5V VBUS rails.
		if (tegra_t210)
		{
			// Fan and Rail power from USB 5V VBUS EN.
			PINMUX_AUX(PINMUX_AUX_USB_VBUS_EN0) = PINMUX_LPDR | 1;
			gpio_direction_output(GPIO_PORT_CC, GPIO_PIN_4, GPIO_LOW);
		}

		// Make sure GPIO IO power is enabled.
		PMC(APBDEV_PMC_NO_IOPOWER) &= ~PMC_NO_IOPOWER_GPIO;
		(void)PMC(APBDEV_PMC_NO_IOPOWER); // Commit write.

		// Inform GPIO IO pads that we switched to 1.8V.
		PMC(APBDEV_PMC_PWR_DET_VAL) &= ~PMC_PWR_DET_33V_GPIO;
		(void)PMC(APBDEV_PMC_PWR_DET_VAL); // Commit write.

		usb_src = false;
	}
	reg_5v_dev |= dev;
}

void regulator_5v_disable(u8 dev)
{
	bool tegra_t210 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;

	reg_5v_dev &= ~dev;

	if (!reg_5v_dev)
	{
		// Rail power from battery 5V regulator.
		gpio_write(GPIO_PORT_A, GPIO_PIN_5, GPIO_LOW);

		// Only Icosa has USB 5V VBUS rails.
		if (tegra_t210)
		{
			// Rail power from USB 5V VBUS.
			gpio_write(GPIO_PORT_CC, GPIO_PIN_4, GPIO_LOW);
			usb_src = false;

		}
	}
}

bool regulator_5v_get_dev_enabled(u8 dev)
{
	return (reg_5v_dev & dev);
}

void regulator_5v_usb_src_enable(bool enable)
{
	// Only for Icosa.
	if (hw_get_chip_id() != GP_HIDREV_MAJOR_T210)
		return;

	if (enable && !usb_src)
	{
		gpio_write(GPIO_PORT_CC, GPIO_PIN_4, GPIO_HIGH);
		usb_src = true;
	}
	else if (!enable && usb_src)
	{
		gpio_write(GPIO_PORT_CC, GPIO_PIN_4, GPIO_LOW);
		usb_src = false;
	}
}
