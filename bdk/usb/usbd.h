/*
 * Enhanced & eXtensible USB Device (EDCI & XDCI) driver for Tegra X1
 *
 * Copyright (c) 2019-2021 CTCaer
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

#ifndef _USB_H_
#define _USB_H_

#include <utils/types.h>

#define USB_TD_BUFFER_PAGE_SIZE 0x1000
#define USB_TD_BUFFER_MAX_SIZE  (USB_TD_BUFFER_PAGE_SIZE * 4)
//#define USB_HW_BUFFER_5_PAGES 0x5000
#define USB_EP_BUFFER_1_TD      (USB_TD_BUFFER_MAX_SIZE)
#define USB_EP_BUFFER_2_TD      (USB_TD_BUFFER_MAX_SIZE * 2)
#define USB_EP_BUFFER_4_TD      (USB_TD_BUFFER_MAX_SIZE * 4)
#define USB_EP_BUFFER_MAX_SIZE  (USB_EP_BUFFER_4_TD)
#define USB_EP_BUFFER_ALIGN     (USB_TD_BUFFER_PAGE_SIZE)

#define USB_XFER_START        0
#define USB_XFER_SYNCED_ENUM  1000000
#define USB_XFER_SYNCED_CMD   1000000
#define USB_XFER_SYNCED_DATA  2000000
#define USB_XFER_SYNCED_CLASS 5000000
#define USB_XFER_SYNCED       -1

typedef enum _usb_hid_type
{
	USB_HID_GAMEPAD,
	USB_HID_TOUCHPAD
} usb_hid_type;

typedef enum _usb_gadget_type
{
	USB_GADGET_UMS          = 0,
	USB_GADGET_HID_GAMEPAD  = 1,
	USB_GADGET_HID_TOUCHPAD = 2,
} usb_gadget_type;

typedef enum {
	USB_DIR_OUT = 0,
	USB_DIR_IN  = 1,
} usb_dir_t;

typedef enum
{
	XUSB_EP_CTRL_IN  = 0, // EP0.
	XUSB_EP_CTRL_OUT = 1, // EP0.

	USB_EP_CTRL_OUT = 0,  // EP0.
	USB_EP_CTRL_IN  = 1,  // EP0.

	USB_EP_BULK_OUT = 2,  // EP1.
	USB_EP_BULK_IN  = 3,  // EP1.
	USB_EP_ALL      = 0xFFFFFFFF
} usb_ep_t;

typedef enum
{
	USB_EP_ADDR_CTRL_OUT = 0x00,
	USB_EP_ADDR_CTRL_IN  = 0x80,
	USB_EP_ADDR_BULK_OUT = 0x01,
	USB_EP_ADDR_BULK_IN  = 0x81,
} usb_ep_addr_t;

typedef enum
{
	USB_EP_CFG_CLEAR = 0,
	USB_EP_CFG_RESET = 0,
	USB_EP_CFG_STALL = 1
} usb_ep_cfg_t;

typedef enum {
	USB_STATUS_EP_OK            = 0,
	USB_STATUS_EP_HALTED        = 1,

	USB_STATUS_DEV_SELF_POWERED = 1,
	USB_STATUS_DEV_REMOTE_WAKE  = 2,
} usb_set_clear_feature_req_t;

typedef enum {
	USB_SETUP_RECIPIENT_DEVICE    = 0,
	USB_SETUP_RECIPIENT_INTERFACE = 1,
	USB_SETUP_RECIPIENT_ENDPOINT  = 2,
	USB_SETUP_RECIPIENT_OTHER     = 3,

	USB_SETUP_TYPE_STANDARD       = 0x00,
	USB_SETUP_TYPE_CLASS          = 0x20,
	USB_SETUP_TYPE_VENDOR         = 0x40,
	USB_SETUP_TYPE_RESERVED       = 0x60,

	USB_SETUP_HOST_TO_DEVICE      = 0x00,
	USB_SETUP_DEVICE_TO_HOST      = 0x80,
} usb_setup_req_type_t;

typedef enum {
	USB_REQUEST_GET_STATUS        = 0,
	USB_REQUEST_CLEAR_FEATURE     = 1,
	USB_REQUEST_SET_FEATURE       = 3,
	USB_REQUEST_SET_ADDRESS       = 5,
	USB_REQUEST_GET_DESCRIPTOR    = 6,
	USB_REQUEST_SET_DESCRIPTOR    = 7,
	USB_REQUEST_GET_CONFIGURATION = 8,
	USB_REQUEST_SET_CONFIGURATION = 9,
	USB_REQUEST_GET_INTERFACE     = 10,
	USB_REQUEST_SET_INTERFACE     = 11,
	USB_REQUEST_SYNCH_FRAME       = 12,
	USB_REQUEST_SET_SEL           = 13,

	USB_REQUEST_GET_MS_DESCRIPTOR = 0x99,

	USB_REQUEST_BULK_GET_MAX_LUN  = 0xFE,
	USB_REQUEST_BULK_RESET        = 0xFF
} usb_standard_req_t;

typedef enum {
	USB_FEATURE_ENDPOINT_HALT        = 0,
	USB_FEATURE_DEVICE_REMOTE_WAKEUP = 1,
	USB_FEATURE_TEST_MODE            = 2,
} usb_get_status_req_t;

typedef enum _usb_error_t
{
	USB_RES_OK           = 0,
	USB_RES_BULK_RESET   = 1,

	USB_ERROR_USER_ABORT = 2,
	USB_ERROR_TIMEOUT    = 3,
	USB_ERROR_INIT       = 4,
	USB_ERROR_XFER_ERROR = 5,

	USB2_ERROR_XFER_EP_DISABLED     = 28,
	USB2_ERROR_XFER_NOT_ALIGNED     = 29,

	XUSB_ERROR_INVALID_EP           = USB_ERROR_XFER_ERROR,        // From 2.
	XUSB_ERROR_XFER_BULK_IN_RESIDUE = 7,
	XUSB_ERROR_INVALID_CYCLE        = USB2_ERROR_XFER_EP_DISABLED, // From 8.
	XUSB_ERROR_BABBLE_DETECTED      = 50,
	XUSB_ERROR_SEQ_NUM              = 51,
	XUSB_ERROR_XFER_DIR             = 52,
	XUSB_ERROR_PORT_CFG             = 54
} usb_error_t;

typedef struct _usb_ctrl_setup_t
{
	u8 bmRequestType;
	u8 bRequest;
	u16 wValue;
	u16 wIndex;
	u16 wLength;
} usb_ctrl_setup_t;

typedef struct _usb_ops_t
{
	int  (*usbd_flush_endpoint)(u32);
	int  (*usbd_set_ep_stall)(u32, int);
	int  (*usbd_handle_ep0_ctrl_setup)();
	void (*usbd_end)(bool, bool);
	int  (*usb_device_init)();
	int  (*usb_device_enumerate)(usb_gadget_type gadget);
	int  (*usb_device_class_send_max_lun)(u8);
	int  (*usb_device_class_send_hid_report)();

	int  (*usb_device_ep1_out_read)(u8 *, u32, u32 *, u32);
	int  (*usb_device_ep1_out_read_big)(u8 *, u32, u32 *);
	int  (*usb_device_ep1_out_reading_finish)(u32 *, u32);
	int  (*usb_device_ep1_in_write)(u8 *, u32, u32 *, u32);
	int  (*usb_device_ep1_in_writing_finish)(u32 *, u32);
	bool (*usb_device_get_suspended)();
	bool (*usb_device_get_port_in_sleep)();
} usb_ops_t;

typedef struct _usb_ctxt_t
{
	u32 type;
	u32 partition;
	u32 offset;
	u32 sectors;
	u32 ro;
	void (*system_maintenance)(bool);
	void *label;
	void (*set_text)(void *, const char *);
} usb_ctxt_t;

void usb_device_get_ops(usb_ops_t *ops);
void xusb_device_get_ops(usb_ops_t *ops);

int  usb_device_gadget_ums(usb_ctxt_t *usbs);
int  usb_device_gadget_hid(usb_ctxt_t *usbs);

#endif