/*
 * Touch driver for Nintendo Switch's STMicroelectronics FingerTip touch controller
 *
 * Copyright (c) 2018 langerhans
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

#include <string.h>

#include "../soc/clock.h"
#include "../soc/i2c.h"
#include "../soc/pinmux.h"
#include "../power/max7762x.h"
#include "../power/max77620.h"
#include "../soc/gpio.h"
#include "../soc/t210.h"
#include "../utils/util.h"
#include "touch.h"


#include "../gfx/gfx.h"
#define DPRINTF(...) gfx_printf(__VA_ARGS__)

static int touch_command(u8 cmd)
{
	int err = i2c_send_byte(I2C_3, STMFTS_I2C_ADDR, cmd, 0);
	if (!err)
		return 1;
		
	// TODO: Check for completion in event loop
	msleep(1);
	return 0;
}

static void _touch_process_contact_event(touch_event *event, bool touching)
{
	event->x = (event->raw[2] << 4) | ((event->raw[4] & STMFTS_MASK_Y_LSB) >> 4);

	// Normally, GUI elements have bigger horizontal estate.
	// Avoid parsing y axis when finger is removed to minimize touch noise.
	if (touching)
	{
		event->y = (event->raw[3] << 4) | (event->raw[4] & STMFTS_MASK_X_MSB);
		event->z = event->raw[5];
		event->fingers = ((event->raw[1] & STMFTS_MASK_TOUCH_ID) >> 4) + 1;
	}
	else
		event->fingers = 0;
}

static void _touch_parse_event(touch_event *event) 
{
	event->type = event->raw[1] & STMFTS_MASK_EVENT_ID;

	switch (event->type) 
	{
	case STMFTS_EV_MULTI_TOUCH_ENTER:
	case STMFTS_EV_MULTI_TOUCH_MOTION:
		_touch_process_contact_event(event, true);
		if (event->z > 52) // Discard noisy hover touch.
			event->touch = true;
		else
		{
			event->touch = false;
			event->type = STMFTS_EV_MULTI_TOUCH_LEAVE;
		}
		break;
	case STMFTS_EV_MULTI_TOUCH_LEAVE:
		event->touch = false;
		_touch_process_contact_event(event, false);
		break;
	case STMFTS_EV_NO_EVENT:
		if (event->touch)
			event->type = STMFTS_EV_MULTI_TOUCH_MOTION;
		break;
	default:
		if (event->touch && event->raw[0] == STMFTS_EV_MULTI_TOUCH_MOTION)
			event->type = STMFTS_EV_MULTI_TOUCH_MOTION;
		else
			event->type = STMFTS_EV_MULTI_TOUCH_LEAVE;
	}

	// gfx_con_setpos(&gfx_con, 0, 300);
	// DPRINTF("x = %d    \ny = %d    \nz: %d  \n", event->x, event->y, event->z);
	// DPRINTF("0 = %02X\n1 = %02x\n2 = %02x\n3 = %02x\n", event->raw[0], event->raw[1], event->raw[2], event->raw[3]);
	// DPRINTF("4 = %02X\n5 = %02x\n6 = %02x\n7 = %02x\n", event->raw[4], event->raw[5], event->raw[6], event->raw[7]);
}

void touch_poll(touch_event *event)
{
	i2c_recv_buf_small(event->raw, 8, I2C_3, STMFTS_I2C_ADDR, STMFTS_LATEST_EVENT);

	_touch_parse_event(event);
}

touch_event touch_poll_wait()
{
	touch_event event;
	do 
	{
		touch_poll(&event);
	} while (event.type != STMFTS_EV_MULTI_TOUCH_LEAVE);

	return event;
}

touch_info touch_get_info()
{
	touch_info info;
	u8 buf[8];
	memset(&buf, 0, 8);
	i2c_recv_buf_small(buf, 8, I2C_3, STMFTS_I2C_ADDR, STMFTS_READ_INFO);

	info.chip_id = buf[0] << 8 | buf[1];
	info.fw_ver = buf[2] << 8 | buf[3];
	info.config_id = buf[4];
	info.config_ver = buf[5];

	//DPRINTF("ID: %04X, FW Ver: %d.%02d\nCfg ID: %02x, Cfg Ver: %d\n",
	//	info.chip_id, info.fw_ver >> 8, info.fw_ver & 0xFF, info.config_id, info.config_ver);

	return info;
}

int touch_power_on()
{
	// Configure touchscreen GPIO.
	PINMUX_AUX(PINMUX_AUX_DAP4_SCLK) = PINMUX_PULL_DOWN | 3;
	gpio_config(GPIO_PORT_J, GPIO_PIN_7, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_J, GPIO_PIN_7, GPIO_OUTPUT_ENABLE);
	gpio_write(GPIO_PORT_J, GPIO_PIN_7, GPIO_HIGH);

	// IRQ and more.
	// PINMUX_AUX(PINMUX_AUX_TOUCH_INT) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE | PINMUX_PULL_UP | 3;
	// gpio_config(GPIO_PORT_X, GPIO_PIN_1, GPIO_MODE_GPIO);
	// gpio_write(GPIO_PORT_X, GPIO_PIN_1, GPIO_LOW);

	// Configure Touscreen and GCAsic shared GPIO.
	PINMUX_AUX(PINMUX_AUX_CAM_I2C_SDA) = PINMUX_TRISTATE | PINMUX_INPUT_ENABLE | PINMUX_PULL_UP | 3;
	PINMUX_AUX(PINMUX_AUX_CAM_I2C_SCL) = PINMUX_INPUT_ENABLE | 1;
	gpio_config(GPIO_PORT_S, GPIO_PIN_3, GPIO_MODE_GPIO);

	// Initialize I2C3.
	pinmux_config_i2c(I2C_3);
	clock_enable_i2c(I2C_3);
	i2c_init(I2C_3);

	// Enables LDO6 for touchscreen VDD/AVDD supply
	max77620_regulator_set_volt_and_flags(REGULATOR_LDO6, 2900000, MAX77620_POWER_MODE_NORMAL);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_LDO6_CFG2,
		MAX77620_LDO_CFG2_ADE_ENABLE | (3 << 3) | (MAX77620_POWER_MODE_NORMAL << MAX77620_LDO_POWER_MODE_SHIFT)); 

	msleep(20);

	// Initialize touchscreen module.
	if (touch_command(STMFTS_SYSTEM_RESET))
		return 0;

	if (touch_command(STMFTS_SLEEP_OUT))
		return 0;

	if (touch_command(STMFTS_MS_CX_TUNING))
		return 0;

	if (touch_command(STMFTS_SS_CX_TUNING))
		return 0;

	if (touch_command(STMFTS_FULL_FORCE_CALIBRATION))
		return 0;

	if (touch_command(STMFTS_MS_MT_SENSE_ON))
		return 0;

	if (touch_command(STMFTS_SS_HOVER_SENSE_OFF))
		return 0;

	return 1;
}

void touch_power_off() 
{
	touch_command(STMFTS_SLEEP_IN);

	// Disable touchscreen power.
	gpio_write(GPIO_PORT_J, GPIO_PIN_7, GPIO_LOW);

	// Disables LDO6 for touchscreen VDD, AVDD supply
	max77620_regulator_enable(REGULATOR_LDO6, 0);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_LDO6_CFG2,
		MAX77620_LDO_CFG2_ADE_ENABLE | (2 << 3) | (MAX77620_POWER_MODE_NORMAL << MAX77620_LDO_POWER_MODE_SHIFT));

	clock_disable_i2c(I2C_3);
}