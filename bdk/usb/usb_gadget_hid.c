/*
 * USB Gadget HID driver for Tegra X1
 *
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

#include <string.h>

#include <usb/usbd.h>
#include <gfx_utils.h>
#include <input/joycon.h>
#include <input/touch.h>
#include <soc/hw_init.h>
#include <soc/t210.h>
#include <utils/util.h>

#include <memory_map.h>

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

typedef struct _gamepad_report_t
{
	u8 x;
	u8 y;
	u8 z;
	u8 rz;

	u8 hat:4;
	u8 btn1:1;
	u8 btn2:1;
	u8 btn3:1;
	u8 btn4:1;

	u8 btn5:1;
	u8 btn6:1;
	u8 btn7:1;
	u8 btn8:1;
	u8 btn9:1;
	u8 btn10:1;
	u8 btn11:1;
	u8 btn12:1;
} __attribute__((packed)) gamepad_report_t;

typedef struct _jc_cal_t
{
	bool cl_done;
	bool cr_done;
	u16  clx_max;
	u16  clx_min;
	u16  cly_max;
	u16  cly_min;
	u16  crx_max;
	u16  crx_min;
	u16  cry_max;
	u16  cry_min;
} jc_cal_t;

static jc_cal_t jc_cal_ctx;
static usb_ops_t usb_ops;

static bool _jc_calibration(jc_gamepad_rpt_t *jc_pad)
{
	// Calibrate left stick.
	if (!jc_cal_ctx.cl_done)
	{
		if (jc_pad->conn_l
			&& jc_pad->lstick_x > 0x400 && jc_pad->lstick_y > 0x400
			&& jc_pad->lstick_x < 0xC00 && jc_pad->lstick_y < 0xC00)
		{
			jc_cal_ctx.clx_max = jc_pad->lstick_x + 0x72;
			jc_cal_ctx.clx_min = jc_pad->lstick_x - 0x72;
			jc_cal_ctx.cly_max = jc_pad->lstick_y + 0x72;
			jc_cal_ctx.cly_min = jc_pad->lstick_y - 0x72;
			jc_cal_ctx.cl_done = true;
		}
		else
			return false;
	}

	// Calibrate right stick.
	if (!jc_cal_ctx.cr_done)
	{
		if (jc_pad->conn_r
			&& jc_pad->rstick_x > 0x400 && jc_pad->rstick_y > 0x400
			&& jc_pad->rstick_x < 0xC00 && jc_pad->rstick_y < 0xC00)
		{
			jc_cal_ctx.crx_max = jc_pad->rstick_x + 0x72;
			jc_cal_ctx.crx_min = jc_pad->rstick_x - 0x72;
			jc_cal_ctx.cry_max = jc_pad->rstick_y + 0x72;
			jc_cal_ctx.cry_min = jc_pad->rstick_y - 0x72;
			jc_cal_ctx.cr_done = true;
		}
		else
			return false;
	}

	return true;
}

static bool _jc_poll(gamepad_report_t *rpt)
{
	// Poll Joy-Con.
	jc_gamepad_rpt_t *jc_pad = joycon_poll();

	if (!jc_pad)
		return false;

	// Exit emulation if Left stick and Home are pressed.
	if (jc_pad->l3 && jc_pad->home)
		return true;

	if (!jc_cal_ctx.cl_done || !jc_cal_ctx.cr_done)
	{
		if (!_jc_calibration(jc_pad))
			return false;
	}

	// Re-calibrate on disconnection.
	if (!jc_pad->conn_l)
		jc_cal_ctx.cl_done = false;
	if (!jc_pad->conn_r)
		jc_cal_ctx.cr_done = false;

	// Calculate left analog stick.
	if (jc_pad->lstick_x <= jc_cal_ctx.clx_max && jc_pad->lstick_x >= jc_cal_ctx.clx_min)
		rpt->x = 0x7F;
	else if (jc_pad->lstick_x > jc_cal_ctx.clx_max)
	{
		u16 x_raw = (jc_pad->lstick_x - jc_cal_ctx.clx_max) / 7;
		if (x_raw > 0x7F)
			x_raw = 0x7F;
		rpt->x = 0x7F + x_raw;
	}
	else
	{
		u16 x_raw = (jc_cal_ctx.clx_min - jc_pad->lstick_x) / 7;
		if (x_raw > 0x7F)
			x_raw = 0x7F;
		rpt->x = 0x7F - x_raw;
	}

	if (jc_pad->lstick_y <= jc_cal_ctx.cly_max && jc_pad->lstick_y >= jc_cal_ctx.cly_min)
		rpt->y = 0x7F;
	else if (jc_pad->lstick_y > jc_cal_ctx.cly_max)
	{
		u16 y_raw = (jc_pad->lstick_y - jc_cal_ctx.cly_max) / 7;
		if (y_raw > 0x7F)
			y_raw = 0x7F;
		rpt->y = 0x7F - y_raw;
	}
	else
	{
		u16 y_raw = (jc_cal_ctx.cly_min - jc_pad->lstick_y) / 7;
		if (y_raw > 0x7F)
			y_raw = 0x7F;
		rpt->y = 0x7F + y_raw;
	}

	// Calculate right analog stick.
	if (jc_pad->rstick_x <= jc_cal_ctx.crx_max && jc_pad->rstick_x >= jc_cal_ctx.crx_min)
		rpt->z = 0x7F;
	else if (jc_pad->rstick_x > jc_cal_ctx.crx_max)
	{
		u16 x_raw = (jc_pad->rstick_x - jc_cal_ctx.crx_max) / 7;
		if (x_raw > 0x7F)
			x_raw = 0x7F;
		rpt->z = 0x7F + x_raw;
	}
	else
	{
		u16 x_raw = (jc_cal_ctx.crx_min - jc_pad->rstick_x) / 7;
		if (x_raw > 0x7F)
			x_raw = 0x7F;
		rpt->z = 0x7F - x_raw;
	}

	if (jc_pad->rstick_y <= jc_cal_ctx.cry_max && jc_pad->rstick_y >= jc_cal_ctx.cry_min)
		rpt->rz = 0x7F;
	else if (jc_pad->rstick_y > jc_cal_ctx.cry_max)
	{
		u16 y_raw = (jc_pad->rstick_y - jc_cal_ctx.cry_max) / 7;
		if (y_raw > 0x7F)
			y_raw = 0x7F;
		rpt->rz = 0x7F - y_raw;
	}
	else
	{
		u16 y_raw = (jc_cal_ctx.cry_min - jc_pad->rstick_y) / 7;
		if (y_raw > 0x7F)
			y_raw = 0x7F;
		rpt->rz = 0x7F + y_raw;
	}

	// Set D-pad.
	switch ((jc_pad->buttons >> 16) & 0xF)
	{
	case 0: // none
		rpt->hat = 0xF;
		break;
	case 1: // down
		rpt->hat = 4;
		break;
	case 2: // up
		rpt->hat = 0;
		break;
	case 4: // right
		rpt->hat = 2;
		break;
	case 5: // down + right
		rpt->hat = 3;
		break;
	case 6: // up + right
		rpt->hat = 1;
		break;
	case 8: // left
		rpt->hat = 6;
		break;
	case 9: // down + left
		rpt->hat = 5;
		break;
	case 10: // up + left
		rpt->hat = 7;
		break;
	default:
		rpt->hat = 0xF;
		break;
	}

	// Set buttons.
	rpt->btn1 = jc_pad->b; // x.
	rpt->btn2 = jc_pad->a; // a.
	rpt->btn3 = jc_pad->y; // b.
	rpt->btn4 = jc_pad->x; // y.

	rpt->btn5  = jc_pad->l;
	rpt->btn6  = jc_pad->r;
	rpt->btn7  = jc_pad->zl;
	rpt->btn8  = jc_pad->zr;
	rpt->btn9  = jc_pad->minus;
	rpt->btn10 = jc_pad->plus;
	rpt->btn11 = jc_pad->l3;
	rpt->btn12 = jc_pad->r3;

	//rpt->btn13 = jc_pad->cap;
	//rpt->btn14 = jc_pad->home;

	return false;
}

typedef struct _touchpad_report_t
{
	u8  rpt_id;
	u8  tip_switch:1;
	u8  count:7;

	u8  id;

	//u16 z;
	u16 x;
	u16 y;
} __attribute__((packed)) touchpad_report_t;

static bool _fts_touch_read(touchpad_report_t *rpt)
{
	static touch_event touchpad;

	touch_poll(&touchpad);

	rpt->rpt_id = 5;
	rpt->count = 1;

	// Decide touch enable.
	switch (touchpad.type & STMFTS_MASK_EVENT_ID)
	{
	//case STMFTS_EV_MULTI_TOUCH_ENTER:
	case STMFTS_EV_MULTI_TOUCH_MOTION:
		rpt->x = touchpad.x;
		rpt->y = touchpad.y;
		//rpt->z = touchpad.z;
		rpt->id = touchpad.fingers ? touchpad.fingers - 1 : 0;
		rpt->tip_switch = 1;
		break;
	case STMFTS_EV_MULTI_TOUCH_LEAVE:
		rpt->x = touchpad.x;
		rpt->y = touchpad.y;
		//rpt->z = touchpad.z;
		rpt->id = touchpad.fingers ? touchpad.fingers - 1 : 0;
		rpt->tip_switch = 0;
		break;
	case STMFTS_EV_NO_EVENT:
		return false;
	}

	return true;
}

static u8 _hid_transfer_start(usb_ctxt_t *usbs, u32 len)
{
	u8 status = usb_ops.usb_device_ep1_in_write((u8 *)USB_EP_BULK_IN_BUF_ADDR, len, NULL, USB_XFER_SYNCED_CMD);
	if (status == USB_ERROR_XFER_ERROR)
	{
		usbs->set_text(usbs->label, "#FFDD00 Error:# EP IN transfer!");
		if (usb_ops.usbd_flush_endpoint)
			usb_ops.usbd_flush_endpoint(USB_EP_BULK_IN);
	}

	// Linux mitigation: If timed out, clear status.
	if (status == USB_ERROR_TIMEOUT)
		return 0;

	return status;
}

static bool _hid_poll_jc(usb_ctxt_t *usbs)
{
	if (_jc_poll((gamepad_report_t *)USB_EP_BULK_IN_BUF_ADDR))
		return true;

	// Send HID report.
	if (_hid_transfer_start(usbs, sizeof(gamepad_report_t)))
		return true; // EP Error.

	return false;
}

static bool _hid_poll_touch(usb_ctxt_t *usbs)
{
	_fts_touch_read((touchpad_report_t *)USB_EP_BULK_IN_BUF_ADDR);

	// Send HID report.
	if (_hid_transfer_start(usbs, sizeof(touchpad_report_t)))
		return true; // EP Error.

	return false;
}

int usb_device_gadget_hid(usb_ctxt_t *usbs)
{
	int res = 0;
	u32 gadget_type;
	u32 polling_time;

	// Get USB Controller ops.
	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210)
		usb_device_get_ops(&usb_ops);
	else
		xusb_device_get_ops(&usb_ops);

	if (usbs->type == USB_HID_GAMEPAD)
	{
		polling_time = 8000;
		gadget_type = USB_GADGET_HID_GAMEPAD;
	}
	else
	{
		polling_time = 4000;
		gadget_type = USB_GADGET_HID_TOUCHPAD;
	}

	usbs->set_text(usbs->label, "#C7EA46 Status:# Started USB");

	if (usb_ops.usb_device_init())
	{
		usb_ops.usbd_end(false, true);
		return 1;
	}

	usbs->set_text(usbs->label, "#C7EA46 Status:# Waiting for connection");

	// Initialize Control Endpoint.
	if (usb_ops.usb_device_enumerate(gadget_type))
		goto error;

	usbs->set_text(usbs->label, "#C7EA46 Status:# Waiting for HID report request");

	if (usb_ops.usb_device_class_send_hid_report())
		goto error;

	usbs->set_text(usbs->label, "#C7EA46 Status:# Started HID emulation");

	u32 timer_sys = get_tmr_ms() + 5000;
	while (true)
	{
		u32 timer = get_tmr_us();

		// Parse input device.
		if (usbs->type == USB_HID_GAMEPAD)
		{
			if (_hid_poll_jc(usbs))
				break;
		}
		else
		{
			if (_hid_poll_touch(usbs))
				break;
		}

		// Check for suspended USB in case the cable was pulled.
		if (usb_ops.usb_device_get_suspended())
			break; // Disconnected.

		// Handle control endpoint.
		usb_ops.usbd_handle_ep0_ctrl_setup();

		// Wait max gadget timing.
		timer = get_tmr_us() - timer;
		if (timer < polling_time)
			usleep(polling_time - timer);

		if (timer_sys < get_tmr_ms())
		{
			usbs->system_maintenance(true);
			timer_sys = get_tmr_ms() + 5000;
		}
	}

	usbs->set_text(usbs->label, "#C7EA46 Status:# HID ended");
	goto exit;

error:
	usbs->set_text(usbs->label, "#FFDD00 Error:# Timed out or canceled");
	res = 1;

exit:
	usb_ops.usbd_end(true, false);

	return res;
}
