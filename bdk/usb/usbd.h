/*
 * USB Device driver for Tegra X1
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

typedef enum _usb_hid_type
{
	USB_HID_GAMEPAD,
	USB_HID_TOUCHPAD
} usb_hid_type;

typedef enum _usb_gadget_type
{
	USB_GADGET_UMS,
	USB_GADGET_HID_GAMEPAD,
	USB_GADGET_HID_TOUCHPAD
} usb_gadget_type;

typedef enum
{
    USB_EP_CTRL_OUT = 0, // EP0.
    USB_EP_CTRL_IN  = 1, // EP0.
    USB_EP_BULK_OUT = 2, // EP1.
    USB_EP_BULK_IN  = 3, // EP1.
    USB_EP_ALL      = 0xFFFFFFFF
} usb_ep_t;

int  usbd_flush_endpoint(u32 ep);
void usbd_set_ep_stall(u32 endpoint, int ep_stall);
int  usbd_get_max_pkt_length(int endpoint);
int usbd_handle_ep0_pending_control_transfer();
void usbd_end(bool reset_ep, bool only_controller);
int  usb_device_init();
int  usb_device_ep0_initialize(usb_gadget_type type);
int  usb_device_read_ep1_out(u8 *buf, u32 len, u32 *bytes_read, bool sync);
int  usb_device_read_ep1_out_big_reads(u8 *buf, u32 len, u32 *bytes_read);
int  usb_device_ep1_out_reading_finish(u32 *pending_bytes);
int  usb_device_write_ep1_in(u8 *buf, u32 len, u32 *bytes_written, bool sync);
int  usb_device_ep1_in_writing_finish(u32 *pending_bytes);
bool usb_device_get_suspended();

int  usb_device_gadget_ums(usb_ctxt_t *usbs);
int  usb_device_gadget_hid(usb_ctxt_t *usbs);
bool usb_device_get_max_lun(u8 max_lun);
bool usb_device_get_hid_report();
u32  usb_device_get_port_status();

#endif