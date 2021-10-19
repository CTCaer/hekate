/*
 * Touch driver for Nintendo Switch's STM FingerTip S (4CD60D) touch controller
 *
 * Copyright (c) 2018 langerhans
 * Copyright (c) 2018-2020 CTCaer
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

#include <soc/clock.h>
#include <soc/i2c.h>
#include <soc/pinmux.h>
#include <power/max7762x.h>
#include <soc/gpio.h>
#include <soc/t210.h>
#include <utils/btn.h>
#include <utils/util.h>
#include "touch.h"


#include <gfx_utils.h>
#define DPRINTF(...) gfx_printf(__VA_ARGS__)

static touch_panel_info_t _panels[] =
{
	{  0,  1, 1, 1,  "NISSHA NFT-K12D" },
	{  1,  0, 1, 1,  "GiS GGM6 B2X"    },
	{  2,  0, 0, 0,  "NISSHA NBF-K9A"  },
	{  3,  1, 0, 0,  "GiS 5.5\""       },
	{  4,  0, 0, 1,  "Samsung BH2109"  },
	{ -1,  1, 0, 1,  "GiS VA 6.2\""    }
};

static int touch_command(u8 cmd, u8 *buf, u8 size)
{
	int res = i2c_send_buf_small(I2C_3, STMFTS_I2C_ADDR, cmd, buf, size);
	if (!res)
		return 1;
	return 0;
}

static int touch_read_reg(u8 *cmd, u32 csize, u8 *buf, u32 size)
{
	int res = i2c_send_buf_small(I2C_3, STMFTS_I2C_ADDR, cmd[0], &cmd[1], csize - 1);
	if (res)
		res = i2c_recv_buf(buf, size, I2C_3, STMFTS_I2C_ADDR);
	if (!res)
		return 1;

	return 0;
}

static int touch_wait_event(u8 event, u8 status, u32 timeout, u8 *buf)
{
	u32 timer = get_tmr_ms() + timeout;
	while (true)
	{
		u8 tmp[8] = {0};
		i2c_recv_buf_small(tmp, 8, I2C_3, STMFTS_I2C_ADDR, STMFTS_READ_ONE_EVENT);
		if (tmp[1] == event && tmp[2] == status)
		{
			if (buf)
				memcpy(buf, &tmp[3], 5);
			return 0;
		}

		if (get_tmr_ms() > timer)
			return 1;
	}
}

#define X_REAL_MAX 1264
#define Y_REAL_MAX 704
#define EDGE_OFFSET 15

static void _touch_compensate_limits(touch_event *event, bool touching)
{
	event->x = MAX(event->x, EDGE_OFFSET);
	event->x = MIN(event->x, X_REAL_MAX);
	event->x -= EDGE_OFFSET;
	u32 x_adj = (1280 * 1000) / (X_REAL_MAX - EDGE_OFFSET);
	event->x = ((u32)event->x * x_adj) / 1000;

	if (touching)
	{
		event->y = MAX(event->y, EDGE_OFFSET);
		event->y = MIN(event->y, Y_REAL_MAX);
		event->y -= EDGE_OFFSET;
		u32 y_adj = (720 * 1000) / (Y_REAL_MAX - EDGE_OFFSET);
		event->y = ((u32)event->y * y_adj) / 1000;
	}
}

static void _touch_process_contact_event(touch_event *event, bool touching)
{
	event->x = (event->raw[2] << 4) | ((event->raw[4] & STMFTS_MASK_Y_LSB) >> 4);

	// Normally, GUI elements have bigger horizontal estate.
	// Avoid parsing y axis when finger is removed to minimize touch noise.
	if (touching)
	{
		event->y = (event->raw[3] << 4) | (event->raw[4] & STMFTS_MASK_X_MSB);

		event->z = event->raw[5] | (event->raw[6] << 8);
		event->z = event->z << 6;
		u16 tmp = 0x40;
		if ((event->raw[7] & 0x3F) != 1 && (event->raw[7] & 0x3F) != 0x3F)
			tmp = event->raw[7] & 0x3F;
		event->z /= tmp + 0x40;

		event->fingers = ((event->raw[1] & STMFTS_MASK_TOUCH_ID) >> 4) + 1;
	}
	else
		event->fingers = 0;

	_touch_compensate_limits(event, touching);
}

static void _touch_parse_event(touch_event *event)
{
	event->type = event->raw[1] & STMFTS_MASK_EVENT_ID;

	switch (event->type)
	{
	case STMFTS_EV_MULTI_TOUCH_ENTER:
	case STMFTS_EV_MULTI_TOUCH_MOTION:
		_touch_process_contact_event(event, true);
		if (event->z < 255) // Reject palm rest.
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

	// gfx_con_setpos(0, 300);
	// DPRINTF("x = %d    \ny = %d    \nz = %d  \n", event->x, event->y, event->z);
	// DPRINTF("0 = %02X\n1 = %02X\n2 = %02X\n3 = %02X\n", event->raw[0], event->raw[1], event->raw[2], event->raw[3]);
	// DPRINTF("4 = %02X\n5 = %02X\n6 = %02X\n7 = %02X\n", event->raw[4], event->raw[5], event->raw[6], event->raw[7]);
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

	//DPRINTF("ID: %04X, FW Ver: %d.%02d\nCfg ID: %02X, Cfg Ver: %d\n",
	//	info.chip_id, info.fw_ver >> 8, info.fw_ver & 0xFF, info.config_id, info.config_ver);

	return info;
}

touch_panel_info_t *touch_get_panel_vendor()
{
	u8 buf[5] = {0};
	u8 cmd = STMFTS_VENDOR_GPIO_STATE;
	static touch_panel_info_t panel_info = { -2,  0, 0, 0,  ""};

	if (touch_command(STMFTS_VENDOR, &cmd, 1))
		return NULL;

	if (touch_wait_event(STMFTS_EV_VENDOR, STMFTS_VENDOR_GPIO_STATE, 2000, buf))
		return NULL;

	for (u32 i = 0; i < ARRAY_SIZE(_panels); i++)
	{
		touch_panel_info_t *panel = &_panels[i];
		if (buf[0] == panel->gpio0 && buf[1] == panel->gpio1 && buf[2] == panel->gpio2)
			return panel;
	}

	// Touch panel not found, return current gpios.
	panel_info.gpio0 = buf[0];
	panel_info.gpio1 = buf[1];
	panel_info.gpio2 = buf[2];

	return &panel_info;
}

int touch_get_fw_info(touch_fw_info_t *fw)
{
	u8 buf[8] = {0};

	memset(fw, 0, sizeof(touch_fw_info_t));

	// Get fw address info.
	u8 cmd[3] = { STMFTS_RW_FRAMEBUFFER_REG, 0, 0x60 };
	int res = touch_read_reg(cmd, 3, buf, 3);
	if (!res)
	{
		// Get fw info.
		cmd[1] = buf[2]; cmd[2] = buf[1];
		res = touch_read_reg(cmd, 3, buf, 8);
		if (!res)
		{
			fw->fw_id = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
			fw->ftb_ver = (buf[6] << 8) | buf[5];
		}

		cmd[2]++;
		res = touch_read_reg(cmd, 3, buf, 8);
		if (!res)
			fw->fw_rev = (buf[7] << 8) | buf[6];
	}

	return res;
}

int touch_sys_reset()
{
	u8 cmd[3] = { 0, 0x28, 0x80 }; // System reset cmd.
	for (u8 retries = 0; retries < 3; retries++)
	{
		if (touch_command(STMFTS_WRITE_REG, cmd, 3))
		{
			msleep(10);
			continue;
		}
		msleep(10);
		if (touch_wait_event(STMFTS_EV_CONTROLLER_READY, 0, 20, NULL))
			continue;
		else
			return 0;
	}

	return 1;
}

int touch_panel_ito_test(u8 *err)
{
	int res = 0;

	// Reset touchscreen module.
	if (touch_sys_reset())
		return res;

	// Do ITO Production test.
	u8 cmd[2] = { 1, 0 };
	if (touch_command(STMFTS_ITO_CHECK, cmd, 2))
		return res;

	u32 timer = get_tmr_ms() + 2000;
	while (true)
	{
		u8 tmp[8] = {0};
		i2c_recv_buf_small(tmp, 8, I2C_3, STMFTS_I2C_ADDR, STMFTS_READ_ONE_EVENT);
		if (tmp[1] == 0xF && tmp[2] == 0x5)
		{
			if (err)
			{
				err[0] = tmp[3];
				err[1] = tmp[4];
			}

			res = 1;
			break;
		}

		if (get_tmr_ms() > timer)
			break;
	}

	// Reset touchscreen module.
	touch_sys_reset();

	return res;
}

int touch_get_fb_info(u8 *buf)
{
	u8 tmp[5];

	u8 cmd[3] = { STMFTS_RW_FRAMEBUFFER_REG, 0, 0 };
	int res = 0;


	for (u32 i = 0; i < 0x10000; i += 4)
	{
		if (!res)
		{
			cmd[1] = (i >> 8) & 0xFF;
			cmd[2] = i & 0xFF;
			memset(tmp, 0xCC, 5);
			res = touch_read_reg(cmd, 3, tmp, 5);
			memcpy(&buf[i], tmp + 1, 4);
		}
	}

	return res;
}

int touch_sense_enable()
{
	// Switch sense mode and enable multi-touch sensing.
	u8 cmd = STMFTS_FINGER_MODE;
	if (touch_command(STMFTS_SWITCH_SENSE_MODE, &cmd, 1))
		return 0;

	if (touch_command(STMFTS_MS_MT_SENSE_ON, NULL, 0))
		return 0;

	if (touch_command(STMFTS_CLEAR_EVENT_STACK, NULL, 0))
		return 0;

	return 1;
}

int touch_execute_autotune()
{
	// Reset touchscreen module.
	if (touch_sys_reset())
		return 0;

	// Trim low power oscillator.
	if (touch_command(STMFTS_LP_TIMER_CALIB, NULL, 0))
		return 0;
	msleep(200);

	// Apply Mutual Sense Compensation tuning.
	if (touch_command(STMFTS_MS_CX_TUNING, NULL, 0))
		return 0;
	if (touch_wait_event(STMFTS_EV_STATUS, STMFTS_EV_STATUS_MS_CX_TUNING_DONE, 2000, NULL))
		return 0;

	// Apply Self Sense Compensation tuning.
	if (touch_command(STMFTS_SS_CX_TUNING, NULL, 0))
		return 0;
	if (touch_wait_event(STMFTS_EV_STATUS, STMFTS_EV_STATUS_SS_CX_TUNING_DONE, 2000, NULL))
		return 0;

	// Save Compensation data to EEPROM.
	if (touch_command(STMFTS_SAVE_CX_TUNING, NULL, 0))
		return 0;
	if (touch_wait_event(STMFTS_EV_STATUS, STMFTS_EV_STATUS_WRITE_CX_TUNE_DONE, 2000, NULL))
		return 0;

	return touch_sense_enable();
}

static int touch_init()
{
	// Initialize touchscreen module.
	if (touch_sys_reset())
		return 0;

	return touch_sense_enable();
}

int touch_power_on()
{
	// Enable LDO6 for touchscreen AVDD supply.
	max7762x_regulator_set_voltage(REGULATOR_LDO6, 2900000);
	max7762x_regulator_enable(REGULATOR_LDO6, true);

	// Configure touchscreen VDD GPIO.
	PINMUX_AUX(PINMUX_AUX_DAP4_SCLK) = PINMUX_PULL_DOWN | 1;
	gpio_config(GPIO_PORT_J, GPIO_PIN_7, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_J, GPIO_PIN_7, GPIO_OUTPUT_ENABLE);
	gpio_write(GPIO_PORT_J, GPIO_PIN_7, GPIO_HIGH);

	// IRQ and more.
	// PINMUX_AUX(PINMUX_AUX_TOUCH_INT) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE | PINMUX_PULL_UP | 3;
	// gpio_config(GPIO_PORT_X, GPIO_PIN_1, GPIO_MODE_GPIO);
	// gpio_write(GPIO_PORT_X, GPIO_PIN_1, GPIO_LOW);

	// Configure Touscreen and GCAsic shared GPIO.
	PINMUX_AUX(PINMUX_AUX_CAM_I2C_SDA) = PINMUX_LPDR | PINMUX_INPUT_ENABLE | PINMUX_TRISTATE | PINMUX_PULL_UP | 2;
	PINMUX_AUX(PINMUX_AUX_CAM_I2C_SCL) = PINMUX_IO_HV | PINMUX_LPDR | PINMUX_TRISTATE | PINMUX_PULL_DOWN | 2;
	gpio_config(GPIO_PORT_S, GPIO_PIN_3, GPIO_MODE_GPIO); // GC detect.

	// Initialize I2C3.
	pinmux_config_i2c(I2C_3);
	clock_enable_i2c(I2C_3);
	i2c_init(I2C_3);

	// Wait for the touchscreen module to get ready.
	touch_wait_event(STMFTS_EV_CONTROLLER_READY, 0, 20, NULL);

	// Check for forced boot time calibration.
	if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
	{
		u8 err[2];
		if (touch_panel_ito_test(err))
			if (!err[0] && !err[1])
				return touch_execute_autotune();
	}

	// Initialize touchscreen.
	u32 retries = 3;
	while (retries)
	{
		if (touch_init())
			return 1;
		retries--;
	}

	return 0;
}

void touch_power_off()
{
	// Disable touchscreen power.
	gpio_write(GPIO_PORT_J, GPIO_PIN_7, GPIO_LOW);

	// Disables LDO6 for touchscreen VDD, AVDD supply
	max7762x_regulator_enable(REGULATOR_LDO6, false);

	clock_disable_i2c(I2C_3);
}