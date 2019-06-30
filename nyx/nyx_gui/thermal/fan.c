/*
 * Fan driver for Nintendo Switch
 *
 * Copyright (c) 2018 CTCaer
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

#include "fan.h"
#include "../gfx/gfx.h"
#include "../soc/gpio.h"
#include "../soc/pinmux.h"
#include "../soc/t210.h"
#include "../utils/util.h"

bool fan_init = false;

void set_fan_duty(u32 duty)
{
	if (!fan_init)
	{
		// Fan power from internal 5V regulator (battery).
		PINMUX_AUX(PINMUX_AUX_SATA_LED_ACTIVE) = 3;
		gpio_config(GPIO_PORT_A, GPIO_PIN_5, GPIO_MODE_GPIO);
		gpio_output_enable(GPIO_PORT_A, GPIO_PIN_5, GPIO_OUTPUT_ENABLE);
		gpio_write(GPIO_PORT_A, GPIO_PIN_5, GPIO_HIGH);
		
		// Fan power from USB 5V vdd.
		PINMUX_AUX(PINMUX_AUX_USB_VBUS_EN0) = 3;
		gpio_config(GPIO_PORT_CC, GPIO_PIN_4, GPIO_MODE_GPIO);
		gpio_output_enable(GPIO_PORT_CC, GPIO_PIN_4, GPIO_OUTPUT_ENABLE);
		gpio_write(GPIO_PORT_CC, GPIO_PIN_4, GPIO_HIGH);

		// Fan tachometer.
		PINMUX_AUX(PINMUX_AUX_CAM1_PWDN) = PINMUX_PULL_UP | PINMUX_TRISTATE | PINMUX_INPUT_ENABLE | 3;
		gpio_output_enable(GPIO_PORT_S, GPIO_PIN_7, GPIO_OUTPUT_DISABLE);
		gpio_config(GPIO_PORT_S, GPIO_PIN_7, GPIO_MODE_GPIO);
		gpio_write(GPIO_PORT_S, GPIO_PIN_7, GPIO_LOW);

		PWM(PWM_CONTROLLER_PWM_CSR_1) = (1 << 31) | (255 << 16); // Max PWM to disable fan.

		PINMUX_AUX(PINMUX_AUX_LCD_GPIO2) = 1; // Set source to PWM1.
		gpio_config(GPIO_PORT_V, GPIO_PIN_4, GPIO_MODE_SPIO); // Fan power mode.

		fan_init = true;
	}

	if (duty > 236)
		duty = 236;

	// Inverted polarity.
	u32 inv_duty = 236 - duty;
	if (inv_duty == 236)
		inv_duty = 255;

	// Set PWM duty.
	if (inv_duty)
		PWM(PWM_CONTROLLER_PWM_CSR_1) = (1 << 31) | (inv_duty << 16);
	else
		PWM(PWM_CONTROLLER_PWM_CSR_1) = 0;
}

void get_fan_speed(u32 *duty, u32 *rpm)
{
	if (rpm)
	{
		u32  irq_count = 0;
		bool should_read = true;
		bool irq_val = 0;

		// Poll irqs for 2 seconds.
		int timer = get_tmr_us() + 1000000;
		while (timer - get_tmr_us())
		{
			irq_val = gpio_read(GPIO_PORT_S, GPIO_PIN_7);
			if (irq_val && should_read)
			{
				irq_count++;
				should_read = false;
			}
			else if (!irq_val)
				should_read = true;
		}

		// Calculate rpm based on triggered interrupts.
		*rpm = 60000000 / ((1000000 * 2) / irq_count);
	}

	if (duty)
		*duty = 236 - ((PWM(PWM_CONTROLLER_PWM_CSR_1) >> 16) & 0xFF);
}
