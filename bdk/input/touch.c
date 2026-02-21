/*
 * Touch driver for Nintendo Switch's STM FingerTip S (FTM4CD60DA1BE/FTM4CD50TA1BE) touch controller
 *
 * Copyright (c) 2018 langerhans
 * Copyright (c) 2018-2026 CTCaer
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
#include <soc/timer.h>
#include <soc/t210.h>
#include <utils/btn.h>
#include "touch.h"

static touch_panel_info_t _panels[] =
{
	{  0,  1, 1, 1,  "NISSHA NFT-K12D" },// 0.
	{  1,  0, 1, 1,  "GiS GGM6 B2X"    },// 1.
	{  2,  0, 0, 0,  "NISSHA NBF-K9A"  },// 3.
	{  3,  1, 0, 0,  "GiS 5.5\""       },// 4.
	{  4,  0, 0, 1,  "Samsung TSP"     },// 5?
	{ -1,  1, 0, 1,  "GiS VA 6.2\""    } // 2.
};

static touch_info_t _touch_info = { 0 };
static touch_panel_info_t _touch_panel_info = { 0 };

static int _touch_command(u8 cmd, u8 *buf, u8 size)
{
	return !i2c_send_buf_small(I2C_3, FTS4_I2C_ADDR, cmd, buf, size);
}

static int _touch_read_reg(u8 *cmd, u32 csize, u8 *buf, u32 size)
{
	return !i2c_xfer_packet(I2C_3, FTS4_I2C_ADDR, cmd, csize, buf, size);
}

int touch_get_event_count()
{
	u8 cmd[3] = { FTS4_CMD_HW_REG_READ, 0, FTS4_HW_REG_EVENT_COUNT };
	u8 buf[2];

	if (_touch_read_reg(cmd, sizeof(cmd), buf, sizeof(buf)))
		return 0;

	return (buf[1] >> 1);
}

static int _touch_wait_event(u8 event, u8 status, u32 timeout, u8 *buf)
{
	u32 timer = get_tmr_ms() + timeout;
	while (true)
	{
		if (!touch_get_event_count())
			goto retry;

		u8 tmp[FTS4_EVENT_SIZE];
		int res = i2c_recv_buf_big(tmp, FTS4_EVENT_SIZE, I2C_3, FTS4_I2C_ADDR, FTS4_CMD_READ_ONE_EVENT);

		// Check that event type and status match.
		if (res && tmp[0] == event && tmp[1] == status)
		{
			if (buf)
				memcpy(buf, &tmp[2], 6);
			return 0;
		}

retry:
		usleep(500);

		if (get_tmr_ms() > timer)
			return 1;
	}
}

#define X_REAL_MAX 1264
#define Y_REAL_MAX 704
#define EDGE_OFFSET 15

static void _touch_compensate_limits(touch_event_t *event, bool touching)
{
	event->x  = MAX(event->x, EDGE_OFFSET);
	event->x  = MIN(event->x, X_REAL_MAX);
	event->x -= EDGE_OFFSET;
	u32 x_adj = (1280 * 1000) / (X_REAL_MAX - EDGE_OFFSET);
	event->x  = ((u32)event->x * x_adj) / 1000;

	if (touching)
	{
		event->y  = MAX(event->y, EDGE_OFFSET);
		event->y  = MIN(event->y, Y_REAL_MAX);
		event->y -= EDGE_OFFSET;
		u32 y_adj = (720 * 1000) / (Y_REAL_MAX - EDGE_OFFSET);
		event->y  = ((u32)event->y * y_adj) / 1000;
	}
}

static void _touch_process_contact_event(touch_event_t *event, bool touching)
{
	event->x = (event->raw[1] << 4) | ((event->raw[3] & FTS4_MASK_Y_LSB) >> 4);

	// Normally, GUI elements have bigger horizontal estate.
	// Avoid parsing y axis when finger is removed to minimize touch noise.
	if (touching)
	{
		event->y = (event->raw[2] << 4) | (event->raw[3] & FTS4_MASK_X_MSB);

		event->z = event->raw[4] | (event->raw[5] << 8);
		event->z = event->z << 6;

		u16 tmp = 0x40;
		if ((event->raw[6] & 0x3F) != 1 && (event->raw[6] & 0x3F) != 0x3F)
			tmp = event->raw[6] & 0x3F;
		event->z /= tmp + 0x40;

		event->finger = ((event->raw[0] & FTS4_MASK_TOUCH_ID) >> 4) + 1;
	}
	else
		event->finger = 0;

	_touch_compensate_limits(event, touching);
}

static int _touch_parse_input_event(touch_event_t *event)
{
	switch (event->raw[0] & FTS4_MASK_EVENT_ID)
	{
	case FTS4_EV_MULTI_TOUCH_ENTER:
	case FTS4_EV_MULTI_TOUCH_MOTION:
		_touch_process_contact_event(event, true);
		if (event->z < 255) // Reject palm rest.
			event->touch = true;
		else
			event->touch = false;
		return 0;

	case FTS4_EV_MULTI_TOUCH_LEAVE:
		event->touch = false;
		_touch_process_contact_event(event, false);
		return 0;

	default:
		return 1; // No event.
	}
}

int touch_poll(touch_event_t *event)
{
	int res = !i2c_recv_buf_big(event->raw, FTS4_EVENT_SIZE, I2C_3, FTS4_I2C_ADDR, FTS4_CMD_LATEST_EVENT);

	if (!res)
		res = _touch_parse_input_event(event);

	return res;
}

touch_info_t *touch_get_chip_info()
{
	u8 buf[7] = { 0 };

	// Get chip info.
	u8 cmd[3] = { FTS4_CMD_HW_REG_READ, 0, FTS4_HW_REG_CHIP_ID_INFO };
	if (_touch_read_reg(cmd, sizeof(cmd), buf, sizeof(buf)))
	{
		memset(&_touch_info, 0, sizeof(touch_info_t));
		goto exit;
	}

	_touch_info.chip_id    = buf[1] << 8 | buf[2];
	_touch_info.fw_ver     = buf[3] << 8 | buf[4];
	_touch_info.config_id  = buf[5];
	_touch_info.config_ver = buf[6];

exit:
	return &_touch_info;
}

touch_panel_info_t *touch_get_panel_vendor()
{
	_touch_panel_info.idx = -2;

	u8 cmd = FTS4_VENDOR_GPIO_STATE;
	if (_touch_command(FTS4_CMD_VENDOR, &cmd, 1))
		return NULL;

	u8 buf[6] = { 0 };
	if (_touch_wait_event(FTS4_EV_VENDOR, FTS4_VENDOR_GPIO_STATE, 2000, buf))
		return NULL;

	for (u32 i = 0; i < ARRAY_SIZE(_panels); i++)
	{
		touch_panel_info_t *panel = &_panels[i];
		if (buf[0] == panel->gpio0 && buf[1] == panel->gpio1 && buf[2] == panel->gpio2)
			return panel;
	}

	// Touch panel not found, return current gpios.
	_touch_panel_info.gpio0 = buf[0];
	_touch_panel_info.gpio1 = buf[1];
	_touch_panel_info.gpio2 = buf[2];

	return &_touch_panel_info;
}

int touch_get_fw_info(touch_fw_info_t *fw)
{
	u8 buf[9] = { 0 };

	memset(fw, 0, sizeof(touch_fw_info_t));

	// Get fw address info.
	u8 cmd[3] = { FTS4_CMD_FB_REG_READ, 0, 0x60 };
	int res = _touch_read_reg(cmd, sizeof(cmd), buf, 3);
	if (!res)
	{
		// Get fw info.
		cmd[1] = buf[2]; cmd[2] = buf[1];
		res = _touch_read_reg(cmd, sizeof(cmd), buf, sizeof(buf));
		if (!res)
		{
			fw->fw_id   = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
			fw->ftb_ver = (buf[6] <<  8) |  buf[5];
			fw->fw_rev  = (buf[8] <<  8) |  buf[7];
		}
	}

	return res;
}

int touch_sys_reset()
{
	u8 cmd[3] = { 0, FTS4_HW_REG_SYS_RESET, 0x80 }; // System reset cmd.
	for (u8 retries = 0; retries < 3; retries++)
	{
		if (_touch_command(FTS4_CMD_HW_REG_WRITE, cmd, 3))
		{
			msleep(10);
			continue;
		}
		msleep(10);

		if (_touch_wait_event(FTS4_EV_CONTROLLER_READY, 0, 20, NULL))
			continue;
		else
			return 0;
	}

	return 1;
}

int touch_panel_ito_test(u8 *err)
{
	// Check that touch IC is supported.
	touch_info_t *info = touch_get_chip_info();
	if (info->chip_id != FTS4_I2C_CHIP_ID)
		return 1;

	// Reset touchscreen module.
	if (touch_sys_reset())
		return 1;

	// Do ITO Production test.
	if (_touch_command(FTS4_CMD_ITO_CHECK, NULL, 0))
		return 1;

	u8 buf[6] = { 0 };
	int res = _touch_wait_event(FTS4_EV_ERROR, FTS4_EV_ERROR_ITO_TEST, 2000, buf);
	if (!res && err)
	{
		err[0] = buf[0];
		err[1] = buf[1];
	}

	// Reset touchscreen module.
	touch_sys_reset();

	return res;
}

int touch_get_fb_info(u8 *buf)
{
	u8 cmd[3] = { FTS4_CMD_FB_REG_READ, 0, 0 };
	int res = 0;

	for (u32 i = 0; i < 0x10000; i += 4)
	{
		if (!res)
		{
			cmd[1] = (i >> 8) & 0xFF;
			cmd[2] = i & 0xFF;

			u8 tmp[5];
			memset(tmp, 0xCC, sizeof(tmp));
			res = _touch_read_reg(cmd, sizeof(cmd), tmp, sizeof(tmp));
			memcpy(&buf[i], tmp + 1, 4);
		}
	}

	return res;
}

int touch_switch_sense_mode(u8 mode, bool gis_6_2)
{
	// Set detection config.
	u8 cmd[3] = { 1, 0x64, 0 };

	switch (mode)
	{
	case FTS4_STYLUS_MODE:
		cmd[2] = !gis_6_2 ? 0xC8 : 0xAD;
		break;
	case FTS4_FINGER_MODE:
		cmd[2] = !gis_6_2 ? 0x8C : 0x79;
		break;
	}

	_touch_command(FTS4_CMD_DETECTION_CONFIG, cmd, 3);

	// Sense mode.
	cmd[0] = mode;

	return _touch_command(FTS4_CMD_SWITCH_SENSE_MODE, cmd, 1);
}

int touch_sense_enable()
{
	// Switch sense mode and enable multi-touch sensing.
	u8 cmd = FTS4_FINGER_MODE;
	if (_touch_command(FTS4_CMD_SWITCH_SENSE_MODE, &cmd, 1))
		return 0;

	if (_touch_command(FTS4_CMD_MS_MT_SENSE_ON, NULL, 0))
		return 0;

	if (_touch_command(FTS4_CMD_CLEAR_EVENT_STACK, NULL, 0))
		return 0;

	return 1;
}

int touch_execute_autotune()
{
	u8 buf[6] = { 0 };

	// Reset touchscreen module.
	if (touch_sys_reset())
		return 0;

	// Trim low power oscillator.
	if (_touch_command(FTS4_CMD_LP_TIMER_CALIB, NULL, 0))
		return 0;

	msleep(200);

	// Apply Mutual Sense Compensation tuning.
	if (_touch_command(FTS4_CMD_MS_CX_TUNING, NULL, 0))
		return 0;
	if (_touch_wait_event(FTS4_EV_STATUS, FTS4_EV_STATUS_MS_CX_TUNING_DONE, 2000, buf) || buf[0] || buf[1])
		return 0;

	// Apply Self Sense Compensation tuning.
	if (_touch_command(FTS4_CMD_SS_CX_TUNING, NULL, 0))
		return 0;
	if (_touch_wait_event(FTS4_EV_STATUS, FTS4_EV_STATUS_SS_CX_TUNING_DONE, 2000, buf) || buf[0] || buf[1])
		return 0;

	// Save Compensation data to EEPROM.
	if (_touch_command(FTS4_CMD_SAVE_CX_TUNING, NULL, 0))
		return 0;
	if (_touch_wait_event(FTS4_EV_STATUS, FTS4_EV_STATUS_WRITE_CX_TUNE_DONE, 2000, buf) || buf[0] || buf[1])
		return 0;

	return touch_sense_enable();
}

static int touch_init()
{
	// Check that touch IC is supported.
	touch_info_t *info = touch_get_chip_info();
	if (info->chip_id != FTS4_I2C_CHIP_ID)
		return 0;

	// Initialize touchscreen module.
	if (touch_sys_reset())
		return 0;

	return touch_sense_enable();
}

int touch_power_on()
{
	// Configure touchscreen Touch Reset pin.
	PINMUX_AUX(PINMUX_AUX_DAP4_SCLK) = PINMUX_PULL_DOWN | 1;
	gpio_direction_output(GPIO_PORT_J, GPIO_PIN_7, GPIO_LOW);
	usleep(20);

	// Enable LDO6 for touchscreen AVDD and DVDD supply.
	max7762x_regulator_set_voltage(REGULATOR_LDO6, 2900000);
	max7762x_regulator_enable(REGULATOR_LDO6, true);

	// Initialize I2C3.
	pinmux_config_i2c(I2C_3);
	clock_enable_i2c(I2C_3);
	i2c_init(I2C_3);
	usleep(1000);

	// Set Touch Reset pin.
	gpio_write(GPIO_PORT_J, GPIO_PIN_7, GPIO_HIGH);
	usleep(10000);

	// Wait for the touchscreen module to get ready.
	_touch_wait_event(FTS4_EV_CONTROLLER_READY, 0, 20, NULL);

	// Check for forced boot time calibration.
	if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
	{
		u8 err[2];
		if (!touch_panel_ito_test(err))
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
