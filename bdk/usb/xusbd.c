/*
 * eXtensible USB Device driver (XDCI) for Tegra X1
 *
 * Copyright (c) 2020-2021 CTCaer
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
#include <usb/usb_descriptor_types.h>
#include <usb/usb_t210.h>

#include <gfx_utils.h>
#include <mem/mc.h>
#include <soc/bpmp.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <utils/btn.h>
#include <utils/util.h>

#include <memory_map.h>

#define XUSB_TRB_SLOTS 16 //! TODO: Consider upping it.
#define XUSB_LINK_TRB_IDX (XUSB_TRB_SLOTS - 1)
#define XUSB_LAST_TRB_IDX (XUSB_TRB_SLOTS - 1)

#define EP_DONT_RING     0
#define EP_RING_DOORBELL 1

typedef enum {
	XUSB_FULL_SPEED  = 1,
	XUSB_HIGH_SPEED  = 3,
	XUSB_SUPER_SPEED = 4
} xusb_speed_t;

typedef enum {
	EP_DISABLED = 0,
	EP_RUNNING  = 1,
	EP_HALTED   = 2,
	EP_STOPPED  = 3,
	EP_ERROR    = 4
} xusb_ep_status_t;

typedef enum {
	EP_TYPE_ISOC_OUT = 1,
	EP_TYPE_BULK_OUT = 2,
	EP_TYPE_INTR_OUT = 3,
	EP_TYPE_CNTRL    = 4,
	EP_TYPE_ISOC_IN  = 5,
	EP_TYPE_BULK_IN  = 6,
	EP_TYPE_INTR_IN  = 7
} xusb_ep_type_t;

typedef enum {
	XUSB_DEFAULT                 = 0,
	XUSB_ADDRESSED_STS_WAIT      = 1,
	XUSB_ADDRESSED               = 2,
	XUSB_CONFIGURED_STS_WAIT     = 3,
	XUSB_CONFIGURED              = 4,

	XUSB_LUN_CONFIGURED_STS_WAIT = 5,
	XUSB_LUN_CONFIGURED          = 6,
	XUSB_HID_CONFIGURED_STS_WAIT = 7,
	XUSB_HID_CONFIGURED          = 8,

	// XUSB_CONNECTED           = ,
	// XUSB_DISCONNECTED        = ,
	// XUSB_RESET               = ,
	// XUSB_SUSPENDED           = ,
} xusb_dev_state_t;

typedef enum {
	XUSB_TRB_NONE        = 0,
	XUSB_TRB_NORMAL      = 1,
	XUSB_TRB_DATA        = 3,
	XUSB_TRB_STATUS      = 4,
	XUSB_TRB_LINK        = 6,
	XUSB_TRB_TRANSFER    = 32,
	XUSB_TRB_PORT_CHANGE = 34,
	XUSB_TRB_SETUP       = 63,
} xusb_trb_type_t;

typedef enum {
	XUSB_COMP_INVALID                   = 0,
	XUSB_COMP_SUCCESS                   = 1,
	XUSB_COMP_DATA_BUFFER_ERROR         = 2,
	XUSB_COMP_BABBLE_DETECTED_ERROR     = 3,
	XUSB_COMP_USB_TRANSACTION_ERROR     = 4,
	XUSB_COMP_TRB_ERROR                 = 5,
	XUSB_COMP_STALL_ERROR               = 6,
	XUSB_COMP_RESOURCE_ERROR            = 7,
	XUSB_COMP_BANDWIDTH_ERROR           = 8,
	XUSB_COMP_NO_SLOTS_AVAILABLE_ERROR  = 9,
	XUSB_COMP_INVALID_STREAM_TYPE_ERROR = 10,
	XUSB_COMP_SLOT_NOT_ENABLED_ERROR    = 11,
	XUSB_COMP_EP_DISABLED_ERROR         = 12,
	XUSB_COMP_SHORT_PKT                 = 13,
	XUSB_COMP_RING_UNDERRUN             = 14,
	XUSB_COMP_RING_OVERRUN              = 15,
	XUSB_COMP_VF_EVENT_RING_FULL_ERROR  = 16,
	XUSB_COMP_PARAMETER_ERROR           = 17,
	XUSB_COMP_BANDWIDTH_OVERRUN_ERROR   = 18,
	XUSB_COMP_CONTEXT_STATE_ERROR       = 19,
	XUSB_COMP_NO_PING_RESPONSE_ERROR    = 20,
	XUSB_COMP_EVENT_RING_FULL_ERROR     = 21,
	XUSB_COMP_INCOMPATIBLE_DEVICE_ERROR = 22,
	XUSB_COMP_MISSED_SERVICE_ERROR      = 23,
	XUSB_COMP_COMMAND_RING_STOPPED      = 24,
	XUSB_COMP_COMMAND_ABORTED           = 25,
	XUSB_COMP_STOPPED                   = 26,
	XUSB_COMP_STOPPED_LENGTH_INVALID    = 27,
	XUSB_COMP_STOPPED_SHORT_PACKET      = 28,
	XUSB_COMP_EXIT_LATENCY_LARGE_ERROR  = 29,
	XUSB_COMP_ISOCH_BUFFER_OVERRUN      = 31,
	XUSB_COMP_EVENT_LOST_ERROR          = 32,
	XUSB_COMP_UNDEFINED_ERROR           = 33,
	XUSB_COMP_INVALID_STREAM_ID_ERROR   = 34,
	XUSB_COMP_SECONDARY_BANDWIDTH_ERROR = 35,
	XUSB_COMP_SPLIT_TRANSACTION_ERROR   = 36,

	XUSB_COMP_CODE_STREAM_NUMP_ERROR    = 219,
	XUSB_COMP_PRIME_PIPE_RECEIVED       = 220,
	XUSB_COMP_HOST_REJECTED             = 221,
	XUSB_COMP_CTRL_DIR_ERROR            = 222,
	XUSB_COMP_CTRL_SEQ_NUM_ERROR        = 223
} xusb_comp_code_t;

typedef struct _event_trb_t
{
	u32 rsvd0;
	u32 rsvd1;

	u32 rsvd2:24;
	u32 comp_code:8;

	u32 cycle:1;
	u32 rsvd3:9;
	u32 trb_type:6;
	u32 ep_id:5;
	u32 rsvd4:11;
} event_trb_t;

typedef struct _transfer_event_trb_t {
	u32 trb_pointer_lo;
	u32 trb_pointer_hi;

	u32 trb_tx_len:24;
	u32 comp_code:8;

	u32 cycle:1;
	u32 rsvddw3_0:1;
	u32 event_data:1;
	u32 rsvddw3_1:7;
	u32 trb_type:6;
	u32 ep_id:5;
	u32 rsvddw3_2:11;
} transfer_event_trb_t;

typedef struct _setup_event_trb_t
{
	usb_ctrl_setup_t ctrl_setup_data;

	u32 ctrl_seq_num:16;
	u32 rsvddw2_0:8;
	u32 comp_code:8;

	u32 cycle:1;
	u32 rsvddw3_0:9;
	u32 trb_type:6;
	u32 ep_id:5;
	u32 rsvddw3_1:11;
} setup_event_trb_t;

typedef struct _status_trb_t
{
	u32 rsvd0;
	u32 rsvd1;

	u32 rsvd2:22;
	u32 interrupt_target:10;

	u32 cycle:1;
	u32 ent:1;
	u32 rsvd3_0:2;
	u32 chain:1;
	u32 ioc:1;
	u32 rsvd3_1:4;
	u32 trb_type:6;
	u32 dir:1;
	u32 rsvd3_2:15;
} status_trb_t;

typedef struct _normal_trb_t
{
	u32 databufptr_lo;
	u32 databufptr_hi;

	u32 trb_tx_len:17;
	u32 td_size:5;
	u32 interrupt_target:10;

	u32 cycle:1;
	u32 ent:1;
	u32 isp:1;
	u32 no_snoop:1;
	u32 chain:1;
	u32 ioc:1;
	u32 idt:1;
	u32 rsvd0_0:2;
	u32 bei:1;
	u32 trb_type:6;
	u32 rsvd0_1:16;
} normal_trb_t;

typedef struct _data_trb_t
{
	u32 databufptr_lo;
	u32 databufptr_hi;

	u32 trb_tx_len:17;
	u32 td_size:5;
	u32 interrupt_target:10;

	u32 cycle:1;
	u32 ent:1;
	u32 isp:1;
	u32 no_snoop:1;
	u32 chain:1;
	u32 ioc:1;
	u32 rsvd0_0:4;
	u32 trb_type:6;
	u32 dir:1;
	u32 rsvd0_1:15;
} data_trb_t;

typedef struct _link_trb_t
{
	u32 rsvd0_0:4;
	u32 ring_seg_ptrlo:28;

	u32 ring_seg_ptrhi;

	u32 rsvd1_0:22;
	u32 interrupt_target:10;

	u32 cycle:1;
	u32 toggle_cycle:1;
	u32 rsvd3_0:2;
	u32 chain:1;
	u32 ioc:1;
	u32 rsvd3_1:4;
	u32 trb_type:6;
	u32 rsvd3_2:16;
} link_trb_t;

typedef struct _xusb_ep_ctx_t
{
	// Common context.
	u32 ep_state:3;
	u32 rsvddW0_0:5;
	u32 mult:2;
	u32 max_pstreams:5;
	u32 lsa:1;
	u32 interval:8;
	u32 rsvddW0_1:8;

	u32 rsvddw1_0:1;
	u32 cerr:2;
	u32 ep_type:3;
	u32 rsvddw1_1:1;
	u32 hid:1;
	u32 max_burst_size:8;
	u32 max_packet_size:16;

	u32 dcs:1;
	u32 rsvddw2_0:3;
	u32 trd_dequeueptr_lo:28;

	u32 trd_dequeueptr_hi;

	u32 avg_trb_len:16;
	u32 max_esit_payload:16;

	// Nvidia context.
	u32 event_data_txlen_acc;

	u32 cprog:8;
	u32 sbyte:7;
	u32 tp:2;
	u32 rec:1;
	u32 cec:2;
	u32 ced:1;
	u32 hsp1:1;
	u32 rty1:1;
	u32 std:1;
	u32 status:8;

	u32 data_offset;

	u32 scratch_pad0;

	u32 scratch_pad1;

	u32 cping:8;
	u32 sping:8;
	u32 toggle_cycle:2;
	u32 no_snoop:1;
	u32 ro:1;
	u32 tlm:1;
	u32 dlm:1;
	u32 hsp2:1;
	u32 rty2:1;
	u32 stop_rec_req:8;

	u32 device_addr:8;
	u32 hub_addr:8;
	u32 root_port_num:8;
	u32 slot_id:8;

	u32 routing_string:20;
	u32 speed:4;
	u32 lpu:1;
	u32 mtt:1;
	u32 hub:1;
	u32 dci:5;

	u32 tthub_slot_id:8;
	u32 ttport_num:8;
	u32 ssf:4;
	u32 sps:2;
	u32 interrupt_target:10;

	u32 frz:1;
	u32 end:1;
	u32 elm:1;
	u32 mrx:1;
	u32 ep_linklo:28;

	u32 ep_linkhi;
} xusb_ep_ctx_t;

typedef struct _xusbd_controller_t
{
	data_trb_t *cntrl_epenqueue_ptr;
	data_trb_t *cntrl_epdequeue_ptr;
	u32 cntrl_producer_cycle;
	data_trb_t *bulkout_epenqueue_ptr;
	data_trb_t *bulkout_epdequeue_ptr;
	u32 bulkout_producer_cycle;
	data_trb_t *bulkin_epenqueue_ptr;
	data_trb_t *bulkin_epdequeue_ptr;
	u32 bulkin_producer_cycle;
	event_trb_t *event_enqueue_ptr;
	event_trb_t *event_dequeue_ptr;
	u32 event_ccs;
	u32 device_state;
	u32 tx_bytes[2];
	u32 tx_count[2];
	u32 ctrl_seq_num;
	u32 config_num;
	u32 interface_num;
	u32 wait_for_event_trb;
	u32 port_speed;

	usb_desc_t *desc;
	usb_gadget_type gadget;

	u8 max_lun;
	bool max_lun_set;
	bool bulk_reset_req;
} xusbd_controller_t;

extern u32 hid_report_descriptor_jc_size;
extern u32 hid_report_descriptor_touch_size;
extern u8 hid_report_descriptor_jc[];
extern u8 hid_report_descriptor_touch[];
extern usb_desc_t usb_gadget_hid_jc_descriptors;
extern usb_desc_t usb_gadget_hid_touch_descriptors;
extern usb_desc_t usb_gadget_ums_descriptors;

// All rings and EP context must be aligned to 0x10.
typedef struct _xusbd_event_queues_t
{
	event_trb_t xusb_event_ring_seg0[XUSB_TRB_SLOTS];
	event_trb_t xusb_event_ring_seg1[XUSB_TRB_SLOTS];
	data_trb_t xusb_cntrl_event_queue[XUSB_TRB_SLOTS];
	data_trb_t xusb_bulkin_event_queue[XUSB_TRB_SLOTS];
	data_trb_t xusb_bulkout_event_queue[XUSB_TRB_SLOTS];
	volatile xusb_ep_ctx_t xusb_ep_ctxt[4];
} xusbd_event_queues_t;

// Set event queues context to a 0x10 aligned address.
xusbd_event_queues_t *xusb_evtq = (xusbd_event_queues_t *)XUSB_RING_ADDR;

xusbd_controller_t *usbd_xotg;
xusbd_controller_t usbd_xotg_controller_ctxt;

static int _xusb_xhci_mask_wait(u32 reg, u32 mask, u32 val, u32 retries)
{
	do
	{
		if ((XUSB_DEV_XHCI(reg) & mask) == val)
			return USB_RES_OK;
		usleep(1);
		--retries;
	}
	while (retries);

	return USB_ERROR_TIMEOUT;
}

// Event rings aligned to 0x10
static void _xusbd_ep_init_event_ring()
{
	memset(xusb_evtq->xusb_event_ring_seg0, 0, sizeof(xusb_evtq->xusb_event_ring_seg0));
	memset(xusb_evtq->xusb_event_ring_seg1, 0, sizeof(xusb_evtq->xusb_event_ring_seg1));

	//! TODO USB3: enable pcie regulators.

	// Set Event Ring Segment 0 Base Address.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERST0BALO) = (u32)xusb_evtq->xusb_event_ring_seg0;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERST0BAHI) = 0;

	// Set Event Ring Segment 1 Base Address.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERST1BALO) = (u32)xusb_evtq->xusb_event_ring_seg1;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERST1BAHI) = 0;

	// Set Event Ring Segment sizes.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERSTSZ) = (XUSB_TRB_SLOTS << 16) | XUSB_TRB_SLOTS;

	// Set Enqueue and Dequeue pointers.
	usbd_xotg->event_enqueue_ptr = xusb_evtq->xusb_event_ring_seg0;
	usbd_xotg->event_dequeue_ptr = xusb_evtq->xusb_event_ring_seg0;
	usbd_xotg->event_ccs = 1;

	// Event Ring Enqueue Pointer.
	u32 evt_ring_addr = (u32)xusb_evtq->xusb_event_ring_seg0 & 0xFFFFFFF0;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_EREPLO) = (XUSB_DEV_XHCI(XUSB_DEV_XHCI_EREPLO) & 0xE) | evt_ring_addr | XCHI_ECS;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_EREPHI) = 0;

	// Set Event Ring Dequeue Pointer.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERDPLO) = (XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERDPLO) & 0xF) | evt_ring_addr;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERDPHI) = 0;
}

static void _xusb_ep_set_type_and_metrics(u32 ep_idx, volatile xusb_ep_ctx_t *ep_ctxt)
{
	usb_ep_descr_t *ep_desc = NULL;
	usb_ep_descr_t *endpoints = usbd_xotg->desc->cfg->endpoint;

	switch (ep_idx)
	{
	case XUSB_EP_CTRL_IN:
		// Set EP type.
		ep_ctxt->ep_type = EP_TYPE_CNTRL;

		// Set max packet size based on port speed.
		ep_ctxt->avg_trb_len = 8;
		ep_ctxt->max_packet_size = 64; //! TODO USB3: max_packet_size = 512.
		break;

	case USB_EP_BULK_OUT:
		// Set default EP type.
		ep_ctxt->ep_type = EP_TYPE_BULK_OUT;

		// Check configuration descriptor.
		if (usbd_xotg->desc->cfg->interface.bInterfaceClass == 0x3) // HID Class.
			endpoints = (usb_ep_descr_t *)((void *)endpoints + sizeof(usb_hid_descr_t));

		for (u32 i = 0; i < usbd_xotg->desc->cfg->interface.bNumEndpoints; i++)
			if (endpoints[i].bEndpointAddress == USB_EP_ADDR_BULK_OUT)
			{
				ep_desc = &endpoints[i];
				break;
			}

		// Set actual EP type.
		if (ep_desc)
		{
			switch (ep_desc->bmAttributes)
			{
			case USB_EP_TYPE_ISO:
				ep_ctxt->ep_type = EP_TYPE_ISOC_OUT;
				break;
			case USB_EP_TYPE_BULK:
				ep_ctxt->ep_type = EP_TYPE_BULK_OUT;
				break;
			case USB_EP_TYPE_INTR:
				ep_ctxt->ep_type = EP_TYPE_INTR_OUT;
				break;
			}
		}

		// Set average TRB length.
		//TODO: Use ep type instead (we don't expect to calculate avg per gadget)?
		switch (usbd_xotg->gadget)
		{
		case USB_GADGET_UMS:
			ep_ctxt->avg_trb_len = 3072;
			break;
		case USB_GADGET_HID_GAMEPAD:
		case USB_GADGET_HID_TOUCHPAD:
			ep_ctxt->avg_trb_len = 1024;
			break;
		default:
			switch (usbd_xotg->port_speed)
			{
			case XUSB_SUPER_SPEED:
				ep_ctxt->avg_trb_len = 1024;
				break;
			case XUSB_HIGH_SPEED:
			case XUSB_FULL_SPEED:
				ep_ctxt->avg_trb_len = 512;
				break;
			}
			break;
		}

		// Set max burst rate.
		ep_ctxt->max_burst_size = (ep_desc->wMaxPacketSize >> 11) & 3;

		// Set max packet size based on port speed.
		if (usbd_xotg->port_speed == XUSB_SUPER_SPEED)
		{
			ep_ctxt->max_packet_size = 1024;

			//! TODO USB3:
			// If ISO or INTR EP, set Max Esit Payload size.
			//	ep_ctxt->max_burst_size = bMaxBurst;
			//if (ep_ctxt->ep_type == EP_TYPE_INTR_OUT || ep_ctxt->ep_type == EP_TYPE_ISOC_OUT)
			//	ep_ctxt->max_esit_payload = ep_ctxt->max_packet_size * (ep_ctxt->max_burst_size + 1);
		}
		else if (usbd_xotg->port_speed == XUSB_HIGH_SPEED)
		{
			ep_ctxt->max_packet_size = 512;

			// If ISO or INTR EP, set Max Esit Payload size.
			if (ep_ctxt->ep_type == EP_TYPE_INTR_OUT || ep_ctxt->ep_type == EP_TYPE_ISOC_OUT)
				ep_ctxt->max_esit_payload = ep_ctxt->max_packet_size * (ep_ctxt->max_burst_size + 1);
		}
		else
		{
			ep_ctxt->max_packet_size = 64;

			// If ISO or INTR EP, set Max Esit Payload size.
			if (ep_ctxt->ep_type == EP_TYPE_INTR_OUT || ep_ctxt->ep_type == EP_TYPE_ISOC_OUT)
				ep_ctxt->max_esit_payload = ep_ctxt->max_packet_size;
		}
		break;

	case USB_EP_BULK_IN:
		// Set default EP type.
		ep_ctxt->ep_type = EP_TYPE_BULK_IN;

		// Check configuration descriptor.
		if (usbd_xotg->desc->cfg->interface.bInterfaceClass == 0x3) // HID Class.
			endpoints = (usb_ep_descr_t *)((void *)endpoints + sizeof(usb_hid_descr_t));

		for (u32 i = 0; i < usbd_xotg->desc->cfg->interface.bNumEndpoints; i++)
			if (endpoints[i].bEndpointAddress == USB_EP_ADDR_BULK_IN)
			{
				ep_desc = &endpoints[i];
				break;
			}

		// Set actual EP type.
		if (ep_desc)
		{
			switch (ep_desc->bmAttributes)
			{
			case USB_EP_TYPE_ISO:
				ep_ctxt->ep_type = EP_TYPE_ISOC_IN;
				break;
			case USB_EP_TYPE_BULK:
				ep_ctxt->ep_type = EP_TYPE_BULK_IN;
				break;
			case USB_EP_TYPE_INTR:
				ep_ctxt->ep_type = EP_TYPE_INTR_IN;
				break;
			}
		}

		// Set average TRB length.
		//TODO: Use ep type instead (we don't expect to calculate avg per gadget)?
		switch (usbd_xotg->gadget)
		{
		case USB_GADGET_UMS:
			ep_ctxt->avg_trb_len = 3072;
			break;
		case USB_GADGET_HID_GAMEPAD:
		case USB_GADGET_HID_TOUCHPAD:
			ep_ctxt->avg_trb_len = 16; // Normal interrupt avg is 1024KB.
			break;
		default:
			switch (usbd_xotg->port_speed)
			{
			case XUSB_SUPER_SPEED:
				ep_ctxt->avg_trb_len = 1024;
				break;
			case XUSB_HIGH_SPEED:
			case XUSB_FULL_SPEED:
				ep_ctxt->avg_trb_len = 512;
				break;
			}
			break;
		}

		// Set max burst rate.
		ep_ctxt->max_burst_size = (ep_desc->wMaxPacketSize >> 11) & 3;

		// Set max packet size based on port speed.
		if (usbd_xotg->port_speed == XUSB_SUPER_SPEED)
		{
			ep_ctxt->max_packet_size = 1024;

			//! TODO USB3:
			// If ISO or INTR EP, set Max Esit Payload size.
			//	ep_ctxt->max_burst_size = bMaxBurst;
			//if (ep_ctxt->ep_type == EP_TYPE_INTR_IN || ep_ctxt->ep_type == EP_TYPE_ISOC_IN)
			//	ep_ctxt->max_esit_payload = ep_ctxt->max_packet_size * (ep_ctxt->max_burst_size + 1);
		}
		else if (usbd_xotg->port_speed == XUSB_HIGH_SPEED)
		{
			ep_ctxt->max_packet_size = 512;

			// If ISO or INTR EP, set Max Esit Payload size.
			if (ep_ctxt->ep_type == EP_TYPE_INTR_IN || ep_ctxt->ep_type == EP_TYPE_ISOC_IN)
				ep_ctxt->max_esit_payload = ep_ctxt->max_packet_size * (ep_ctxt->max_burst_size + 1);
		}
		else
		{
			ep_ctxt->max_packet_size = 64;

			// If ISO or INTR EP, set Max Esit Payload size.
			if (ep_ctxt->ep_type == EP_TYPE_INTR_IN || ep_ctxt->ep_type == EP_TYPE_ISOC_IN)
				ep_ctxt->max_esit_payload = ep_ctxt->max_packet_size;
		}
		break;
	}
}

static int _xusb_ep_init_context(u32 ep_idx)
{
	link_trb_t *link_trb;

	if (ep_idx > USB_EP_BULK_IN)
		return USB_ERROR_INIT;

	if (ep_idx == XUSB_EP_CTRL_OUT)
		ep_idx = XUSB_EP_CTRL_IN;

	volatile xusb_ep_ctx_t *ep_ctxt = &xusb_evtq->xusb_ep_ctxt[ep_idx];
	memset((void *)ep_ctxt, 0, sizeof(xusb_ep_ctx_t));

	ep_ctxt->ep_state = EP_RUNNING;
	ep_ctxt->dcs = 1;
	ep_ctxt->cec = 3;
	ep_ctxt->cerr = 3;
	ep_ctxt->max_burst_size = 0;

	switch (ep_idx)
	{
	case XUSB_EP_CTRL_IN:
		usbd_xotg->cntrl_producer_cycle = 1;
		usbd_xotg->cntrl_epenqueue_ptr = xusb_evtq->xusb_cntrl_event_queue;
		usbd_xotg->cntrl_epdequeue_ptr = xusb_evtq->xusb_cntrl_event_queue;

		_xusb_ep_set_type_and_metrics(ep_idx, ep_ctxt);

		ep_ctxt->trd_dequeueptr_lo = (u32)xusb_evtq->xusb_cntrl_event_queue >> 4;
		ep_ctxt->trd_dequeueptr_hi = 0;

		link_trb = (link_trb_t *)&xusb_evtq->xusb_cntrl_event_queue[XUSB_LINK_TRB_IDX];
		link_trb->toggle_cycle = 1;
		link_trb->ring_seg_ptrlo = (u32)xusb_evtq->xusb_cntrl_event_queue >> 4;
		link_trb->ring_seg_ptrhi = 0;
		link_trb->trb_type = XUSB_TRB_LINK;
		break;

	case USB_EP_BULK_OUT:
		usbd_xotg->bulkout_producer_cycle = 1;
		usbd_xotg->bulkout_epenqueue_ptr = xusb_evtq->xusb_bulkout_event_queue;
		usbd_xotg->bulkout_epdequeue_ptr = xusb_evtq->xusb_bulkout_event_queue;

		_xusb_ep_set_type_and_metrics(ep_idx, ep_ctxt);

		ep_ctxt->trd_dequeueptr_lo = (u32)xusb_evtq->xusb_bulkout_event_queue >> 4;
		ep_ctxt->trd_dequeueptr_hi = 0;

		link_trb = (link_trb_t *)&xusb_evtq->xusb_bulkout_event_queue[XUSB_LINK_TRB_IDX];
		link_trb->toggle_cycle = 1;
		link_trb->ring_seg_ptrlo = (u32)xusb_evtq->xusb_bulkout_event_queue >> 4;
		link_trb->ring_seg_ptrhi = 0;
		link_trb->trb_type = XUSB_TRB_LINK;
		break;

	case USB_EP_BULK_IN:
		usbd_xotg->bulkin_producer_cycle = 1;
		usbd_xotg->bulkin_epenqueue_ptr = xusb_evtq->xusb_bulkin_event_queue;
		usbd_xotg->bulkin_epdequeue_ptr = xusb_evtq->xusb_bulkin_event_queue;

		_xusb_ep_set_type_and_metrics(ep_idx, ep_ctxt);

		ep_ctxt->trd_dequeueptr_lo = (u32)xusb_evtq->xusb_bulkin_event_queue >> 4;
		ep_ctxt->trd_dequeueptr_hi = 0;

		link_trb = (link_trb_t *)&xusb_evtq->xusb_bulkin_event_queue[XUSB_LINK_TRB_IDX];
		link_trb->toggle_cycle = 1;
		link_trb->ring_seg_ptrlo = (u32)xusb_evtq->xusb_bulkin_event_queue >> 4;
		link_trb->ring_seg_ptrhi = 0;
		link_trb->trb_type = XUSB_TRB_LINK;
		break;
	}

	return USB_RES_OK;
}

static int _xusbd_ep_initialize(u32 ep_idx)
{
	switch (ep_idx)
	{
	case XUSB_EP_CTRL_IN:
	case XUSB_EP_CTRL_OUT:
		return _xusb_ep_init_context(XUSB_EP_CTRL_IN);
	case USB_EP_BULK_OUT:
	case USB_EP_BULK_IN:
		_xusb_ep_init_context(ep_idx);
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_RELOAD) = BIT(ep_idx);
		int res = _xusb_xhci_mask_wait(XUSB_DEV_XHCI_EP_RELOAD, BIT(ep_idx), 0, 1000);
		if (!res)
		{
			XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_PAUSE) &= ~BIT(ep_idx);
			XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_HALT) &= ~BIT(ep_idx);
		}
		return res;
	default:
		return USB_ERROR_INIT;
	}
}

static void _xusb_init_phy()
{
	// Configure and enable PLLU.
	clock_enable_pllu();

	// Enable IDDQ control by software and disable UTMIPLL IDDQ.
	CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) = (CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) & 0xFFFFFFFC) | 1;

	// Set UTMIPLL dividers and config based on OSC and enable it to 960 MHz.
	clock_enable_utmipll();

	// Set UTMIP misc config.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) = (CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) & 0xFEFFFFE8) | 0x2000008 | 0x20 | 2;
	usleep(2);

	// Set OTG PAD0 calibration.
	u32 fuse_usb_calib = FUSE(FUSE_USB_CALIB);
	// Set HS_CURR_LEVEL.
	XUSB_PADCTL(XUSB_PADCTL_USB2_OTG_PAD0_CTL_0) = (XUSB_PADCTL(XUSB_PADCTL_USB2_OTG_PAD0_CTL_0) & 0xFFFFFFC0) | (fuse_usb_calib & 0x3F);
	// Set TERM_RANGE_ADJ and RPD_CTRL.
	XUSB_PADCTL(XUSB_PADCTL_USB2_OTG_PAD0_CTL_1) = (XUSB_PADCTL(XUSB_PADCTL_USB2_OTG_PAD0_CTL_1) & 0x83FFFF87) | ((fuse_usb_calib & 0x780) >> 4) | ((u32)(FUSE(FUSE_USB_CALIB_EXT) << 27) >> 1);

	// Set VREG_LEV to 1.
	XUSB_PADCTL(XUSB_PADCTL_USB2_BATTERY_CHRG_OTGPAD0_CTL1) = (XUSB_PADCTL(XUSB_PADCTL_USB2_BATTERY_CHRG_OTGPAD0_CTL1) & 0xFFFFFE3F) | 0x80;

	// Disable power down on usb2 ports pads.
	XUSB_PADCTL(XUSB_PADCTL_USB2_OTG_PAD0_CTL_0) &= 0xDBFFFFFF; // Clear pad power down.
	XUSB_PADCTL(XUSB_PADCTL_USB2_OTG_PAD0_CTL_1) &= 0xFFFFFFFB; // Clear pad dr power down.
	XUSB_PADCTL(XUSB_PADCTL_USB2_BATTERY_CHRG_OTGPAD0_CTL0) &= 0xFFFFFFFE; // Clear charging power down.
	XUSB_PADCTL(XUSB_PADCTL_USB2_BIAS_PAD_CTL_0) &= 0xFFFFF7FF; // Clear bias power down.
	(void)XUSB_PADCTL(XUSB_PADCTL_USB2_OTG_PAD0_CTL_1);         // Commit write.

	// Enable USB2 tracking clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_Y_SET) = BIT(CLK_Y_USB2_TRK);
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_USB2_HSIC_TRK) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_USB2_HSIC_TRK) & 0xFFFFFF00) | 6; // Set trank divisor to 4.

	// Set tracking parameters and trigger it.
	XUSB_PADCTL(XUSB_PADCTL_USB2_BIAS_PAD_CTL_1) = 0x451E000;
	XUSB_PADCTL(XUSB_PADCTL_USB2_BIAS_PAD_CTL_1) =  0x51E000;
	usleep(100);

	// TRK cycle done. Force PDTRK input into power down.
	XUSB_PADCTL(XUSB_PADCTL_USB2_BIAS_PAD_CTL_1) = 0x451E000;
	usleep(3);

	// Re-trigger it.
	XUSB_PADCTL(XUSB_PADCTL_USB2_BIAS_PAD_CTL_1) = 0x51E000;
	usleep(100);

	// TRK cycle done. Force PDTRK input into power down.
	XUSB_PADCTL(XUSB_PADCTL_USB2_BIAS_PAD_CTL_1) |= 0x4000000;

	// Disable USB2 tracking clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_Y_CLR) = BIT(CLK_Y_USB2_TRK);

	// Wait for XUSB PHY to stabilize.
	usleep(30);
}

static void _xusbd_init_device_clocks()
{
	// Disable reset to PLLU_OUT1
	CLOCK(CLK_RST_CONTROLLER_PLLU_OUTA) |= 1;
	usleep(2);

	// Enable XUSB device clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_U_SET) = BIT(CLK_U_XUSB_DEV);

	// Set XUSB device core clock source to PLLP for a 102MHz result.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_CORE_DEV) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_CORE_DEV) & 0x1FFFFF00) | (1 << 29) | 6;
	usleep(2);

	// Set XUSB Full-Speed logic clock source to FO 48MHz.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_FS) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_FS) & 0x1FFFFFFF) | (2 << 29);

	// Enable XUSB Super-Speed logic clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_SET) = BIT(CLK_W_XUSB_SS);

	// Set XUSB Super-Speed logic clock source to HSIC 480MHz for 120MHz result and source FS logic clock from Super-Speed.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_SS) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_SS) & 0x1FFFFF00) | (3 << 29) | 6;

	// Clear reset to XUSB device and Super-Speed logic.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_CLR) = BIT(CLK_W_XUSB_SS);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_U_CLR) = BIT(CLK_U_XUSB_DEV);
	usleep(2);
}

int xusb_device_init()
{
	/////////////////////////////////////////////////
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_USBD);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = BIT(CLK_L_USBD);
	/////////////////////////////////////////////////


	// Enable XUSB clock and clear Reset to XUSB Pad Control.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_SET) = BIT(CLK_W_XUSB);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_SET) = BIT(CLK_W_XUSB);
	usleep(2);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_CLR) = BIT(CLK_W_XUSB);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_CLR) = BIT(CLK_W_XUSB_PADCTL);
	usleep(2);

	// USB2 Pads to XUSB.
	XUSB_PADCTL(XUSB_PADCTL_USB2_PAD_MUX) =
		(XUSB_PADCTL(XUSB_PADCTL_USB2_PAD_MUX) & ~(PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_MASK | PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_MASK)) |
		PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_XUSB | PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_XUSB;

	// Initialize XUSB controller PHY.
	_xusb_init_phy();

	// Set USB2.0 Port 0 to device mode.
	XUSB_PADCTL(XUSB_PADCTL_USB2_PORT_CAP) = (XUSB_PADCTL(XUSB_PADCTL_USB2_PORT_CAP) & ~PADCTL_USB2_PORT_CAP_PORT_0_CAP_MASK) | PADCTL_USB2_PORT_CAP_PORT_0_CAP_DEV;

	//! TODO USB3
	// // Set USB3.0 Port 0 cap to device.
	// XUSB_PADCTL(XUSB_PADCTL_SS_PORT_CAP) = (XUSB_PADCTL(XUSB_PADCTL_SS_PORT_CAP) & ~PADCTL_SS_PORT_CAP_0_PORT1_CAP_MASK) | PADCTL_SS_PORT_CAP_0_PORT1_CAP_DEVICE_ONLY;

	// Set Super Speed Port 0 to USB2 Port 0.
	XUSB_PADCTL(XUSB_PADCTL_SS_PORT_MAP) &= ~PADCTL_SS_PORT_MAP_PORT0_MASK; // 0: USB2_PORT0

	// Power Up ID Wake up and Vbus Wake Up for UTMIP
	PMC(APBDEV_PMC_USB_AO) &= 0xFFFFFFF3;
	usleep(1);

	// Initialize device clocks.
	_xusbd_init_device_clocks();

	// Enable AHB redirect for access to IRAM for Event/EP ring buffers.
	mc_enable_ahb_redirect(false); // Can be skipped if IRAM is not used.

	 // Enable XUSB device IPFS.
	XUSB_DEV_DEV(XUSB_DEV_CONFIGURATION) |= DEV_CONFIGURATION_EN_FPCI;

	// Configure PCI and BAR0 address space.
	XUSB_DEV_PCI(XUSB_CFG_1) |= CFG_1_BUS_MASTER | CFG_1_MEMORY_SPACE | CFG_1_IO_SPACE;
	usleep(1);
	XUSB_DEV_PCI(XUSB_CFG_4) = XUSB_DEV_BASE | CFG_4_ADDRESS_TYPE_32_BIT;

	// Mask SATA interrupt to MCORE.
	XUSB_DEV_DEV(XUSB_DEV_INTR_MASK) |= DEV_INTR_MASK_IP_INT_MASK;

	// AHB USB performance cfg.
	AHB_GIZMO(AHB_GIZMO_AHB_MEM) |= AHB_MEM_DONT_SPLIT_AHB_WR | AHB_MEM_ENB_FAST_REARBITRATE;
	AHB_GIZMO(AHB_GIZMO_USB3) |= AHB_GIZMO_IMMEDIATE;
	AHB_GIZMO(AHB_ARBITRATION_PRIORITY_CTRL) = PRIORITY_CTRL_WEIGHT(7) | PRIORITY_SELECT_USB3;
	AHB_GIZMO(AHB_AHB_MEM_PREFETCH_CFG1) =
		MEM_PREFETCH_ENABLE | MEM_PREFETCH_USB3_MST_ID | MEM_PREFETCH_ADDR_BNDRY(12) | 0x1000; // Addr boundary 64KB, Inactivity 4096 cycles.

	// Initialize context.
	usbd_xotg = &usbd_xotg_controller_ctxt;
	memset(usbd_xotg,  0, sizeof(xusbd_controller_t));

	// Initialize event and EP rings.
	_xusbd_ep_init_event_ring();
	memset(xusb_evtq->xusb_cntrl_event_queue, 0, sizeof(xusb_evtq->xusb_cntrl_event_queue));
	memset(xusb_evtq->xusb_bulkin_event_queue, 0, sizeof(xusb_evtq->xusb_bulkin_event_queue));
	memset(xusb_evtq->xusb_bulkout_event_queue, 0, sizeof(xusb_evtq->xusb_bulkout_event_queue));

	// Initialize Control EP.
	int res = _xusbd_ep_initialize(XUSB_EP_CTRL_IN);
	if (res)
		return USB_ERROR_INIT;

	// Enable events and interrupts.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_CTRL) |= XHCI_CTRL_IE | XHCI_CTRL_LSE;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ECPLO) = (u32)xusb_evtq->xusb_ep_ctxt & 0xFFFFFFF0;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ECPHI) = 0;

	//! TODO USB3:
	// XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTHALT) |= DEV_XHCI_PORTHALT_STCHG_INTR_EN;

	return USB_RES_OK;
}

static int _xusb_queue_trb(u32 ep_idx, void *trb, bool ring_doorbell)
{
	int res = USB_RES_OK;
	data_trb_t *next_trb;
	link_trb_t *link_trb;

	// Copy TRB and advance Enqueue list.
	switch (ep_idx)
	{
	case XUSB_EP_CTRL_IN:
		memcpy(usbd_xotg->cntrl_epenqueue_ptr, trb, sizeof(data_trb_t));

		// Advance queue and if Link TRB set index to 0 and toggle cycle bit.
		next_trb = &usbd_xotg->cntrl_epenqueue_ptr[1];
		if (next_trb->trb_type == XUSB_TRB_LINK)
		{
			link_trb = (link_trb_t *)next_trb;
			link_trb->cycle = usbd_xotg->cntrl_producer_cycle & 1;
			link_trb->toggle_cycle = 1;
			next_trb = (data_trb_t *)(link_trb->ring_seg_ptrlo << 4);
			usbd_xotg->cntrl_producer_cycle ^= 1;
		}
		usbd_xotg->cntrl_epenqueue_ptr = next_trb;
		break;

	case USB_EP_BULK_OUT:
		memcpy(usbd_xotg->bulkout_epenqueue_ptr, trb, sizeof(data_trb_t));

		// Advance queue and if Link TRB set index to 0 and toggle cycle bit.
		next_trb = &usbd_xotg->bulkout_epenqueue_ptr[1];
		if (next_trb->trb_type == XUSB_TRB_LINK)
		{
			link_trb = (link_trb_t *)next_trb;
			link_trb->cycle = usbd_xotg->bulkout_producer_cycle & 1;
			link_trb->toggle_cycle = 1;
			next_trb = (data_trb_t *)(link_trb->ring_seg_ptrlo << 4);
			usbd_xotg->bulkout_producer_cycle ^= 1;
		}
		usbd_xotg->bulkout_epenqueue_ptr = next_trb;
		break;

	case USB_EP_BULK_IN:
		memcpy(usbd_xotg->bulkin_epenqueue_ptr, trb, sizeof(data_trb_t));

		// Advance queue and if Link TRB set index to 0 and toggle cycle bit.
		next_trb = &usbd_xotg->bulkin_epenqueue_ptr[1];
		if (next_trb->trb_type == XUSB_TRB_LINK)
		{
			link_trb = (link_trb_t *)next_trb;
			link_trb->cycle = usbd_xotg->bulkin_producer_cycle & 1;
			link_trb->toggle_cycle = 1;
			next_trb = (data_trb_t *)(link_trb->ring_seg_ptrlo << 4);
			usbd_xotg->bulkin_producer_cycle ^= 1;
		}
		usbd_xotg->bulkin_epenqueue_ptr = next_trb;
		break;

	case XUSB_EP_CTRL_OUT:
	default:
		res = XUSB_ERROR_INVALID_EP;
		break;
	}

	// Ring doorbell.
	if (ring_doorbell)
	{
		bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);
		u32 target_id = (ep_idx << 8) & 0xFFFF;
		if (ep_idx == XUSB_EP_CTRL_IN)
			target_id |= usbd_xotg->ctrl_seq_num << 16;
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_DB) = target_id;
	}

	return res;
}

static void _xusb_create_status_trb(status_trb_t *trb, usb_dir_t direction)
{
	trb->cycle = usbd_xotg->cntrl_producer_cycle & 1;
	trb->ioc = 1; // Enable interrupt on completion.
	trb->trb_type = XUSB_TRB_STATUS;
	trb->dir = direction;
}

static void _xusb_create_normal_trb(normal_trb_t *trb, u8 *buf, u32 len, usb_dir_t direction)
{
	u8 producer_cycle;

	trb->databufptr_lo = (u32)buf;
	trb->databufptr_hi = 0;

	trb->trb_tx_len = len;

	// Single TRB transfer.
	trb->td_size = 0;
	trb->chain = 0;

	if (direction == USB_DIR_IN)
		producer_cycle = usbd_xotg->bulkin_producer_cycle & 1;
	else
		producer_cycle = usbd_xotg->bulkout_producer_cycle & 1;

	trb->cycle = producer_cycle;
	trb->isp = 1; // Enable interrupt on short packet.
	trb->ioc = 1; // Enable interrupt on completion.
	trb->trb_type = XUSB_TRB_NORMAL;
}

static void _xusb_create_data_trb(data_trb_t *trb, u8 *buf, u32 len, usb_dir_t direction)
{
	trb->databufptr_lo = (u32)buf;
	trb->databufptr_hi = 0;

	trb->trb_tx_len = len;

	// Single TRB transfer.
	trb->td_size = 0;
	trb->chain = 0;

	trb->cycle = usbd_xotg->cntrl_producer_cycle & 1;
	trb->isp = 1; // Enable interrupt on short packet.
	trb->ioc = 1; // Enable interrupt on completion.
	trb->trb_type = XUSB_TRB_DATA;
	trb->dir = direction;
}

static int _xusb_issue_status_trb(usb_dir_t direction)
{
	int res = USB_RES_OK;
	status_trb_t trb = {0};

	if (usbd_xotg->cntrl_epenqueue_ptr == usbd_xotg->cntrl_epdequeue_ptr || direction == USB_DIR_OUT)
	{
		_xusb_create_status_trb(&trb, direction);
		res = _xusb_queue_trb(XUSB_EP_CTRL_IN, &trb, EP_RING_DOORBELL);
		usbd_xotg->wait_for_event_trb = XUSB_TRB_STATUS;
	}

	return res;
}

static int _xusb_issue_normal_trb(u8 *buf, u32 len, usb_dir_t direction)
{
	normal_trb_t trb = {0};

	_xusb_create_normal_trb(&trb, buf, len, direction);
	u32 ep_idx = USB_EP_BULK_IN;
	if (direction == USB_DIR_OUT)
		ep_idx = USB_EP_BULK_OUT;
	int res = _xusb_queue_trb(ep_idx, &trb, EP_RING_DOORBELL);
	if (!res)
		usbd_xotg->wait_for_event_trb = XUSB_TRB_NORMAL;

	return res;
}

static int _xusb_issue_data_trb(u8 *buf, u32 len, usb_dir_t direction)
{
	data_trb_t trb = {0};

	int res = USB_RES_OK;
	if (usbd_xotg->cntrl_epenqueue_ptr == usbd_xotg->cntrl_epdequeue_ptr)
	{
		_xusb_create_data_trb(&trb, buf, len, direction);
		res = _xusb_queue_trb(XUSB_EP_CTRL_IN, &trb, EP_RING_DOORBELL);
		if (!res)
			usbd_xotg->wait_for_event_trb = XUSB_TRB_DATA;
	}
	return res;
}

int xusb_set_ep_stall(u32 endpoint, int ep_stall)
{
	u32 ep_mask = BIT(endpoint);
	if (ep_stall)
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_HALT) |= ep_mask;
	else
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_HALT) &= ~ep_mask;

	// Wait for EP status to change.
	int res = _xusb_xhci_mask_wait(XUSB_DEV_XHCI_EP_STCHG, ep_mask, ep_mask, 1000);
	if (res)
		return res;

	// Clear status change.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_STCHG) = ep_mask;

	return USB_RES_OK;
}

static int _xusb_wait_ep_stopped(u32 endpoint)
{
	u32 ep_mask = BIT(endpoint);

	// Wait for EP status to change.
	_xusb_xhci_mask_wait(XUSB_DEV_XHCI_EP_STOPPED, ep_mask, ep_mask, 1000);

	// Clear status change.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_STOPPED) = ep_mask;

	return USB_RES_OK;
}

static int _xusb_handle_transfer_event(transfer_event_trb_t *trb)
{
	// Advance dequeue list.
	data_trb_t *next_trb;
	switch (trb->ep_id)
	{
	case XUSB_EP_CTRL_IN:
		next_trb = &usbd_xotg->cntrl_epdequeue_ptr[1];
		if (next_trb->trb_type == XUSB_TRB_LINK)
			next_trb = (data_trb_t *)(next_trb->databufptr_lo & 0xFFFFFFF0);
		usbd_xotg->cntrl_epdequeue_ptr = next_trb;
		break;
	case USB_EP_BULK_OUT:
		next_trb = &usbd_xotg->bulkout_epdequeue_ptr[1];
		if (next_trb->trb_type == XUSB_TRB_LINK)
			next_trb = (data_trb_t *)(next_trb->databufptr_lo & 0xFFFFFFF0);
		usbd_xotg->bulkout_epdequeue_ptr = next_trb;
		break;
	case USB_EP_BULK_IN:
		next_trb = &usbd_xotg->bulkin_epdequeue_ptr[1];
		if (next_trb->trb_type == XUSB_TRB_LINK)
			next_trb = (data_trb_t *)(next_trb->databufptr_lo & 0xFFFFFFF0);
		usbd_xotg->bulkin_epdequeue_ptr = next_trb;
		break;
	default:
		// Should never happen.
		break;
	}

	// Handle completion code.
	switch (trb->comp_code)
	{
	case XUSB_COMP_SUCCESS:
	case XUSB_COMP_SHORT_PKT:
		switch (trb->ep_id)
		{
		case XUSB_EP_CTRL_IN:
			if (usbd_xotg->wait_for_event_trb == XUSB_TRB_DATA)
				return _xusb_issue_status_trb(USB_DIR_OUT);
			else if (usbd_xotg->wait_for_event_trb == XUSB_TRB_STATUS)
			{
				switch (usbd_xotg->device_state)
				{
				case XUSB_ADDRESSED_STS_WAIT:
					usbd_xotg->device_state = XUSB_ADDRESSED;
					break;
				case XUSB_CONFIGURED_STS_WAIT:
					usbd_xotg->device_state = XUSB_CONFIGURED;
					break;
				case XUSB_LUN_CONFIGURED_STS_WAIT:
					usbd_xotg->device_state = XUSB_LUN_CONFIGURED;
					break;
				case XUSB_HID_CONFIGURED_STS_WAIT:
					usbd_xotg->device_state = XUSB_HID_CONFIGURED;
					break;
				}
			}
			break;

		case USB_EP_BULK_IN:
			usbd_xotg->tx_bytes[USB_DIR_IN] -= trb->trb_tx_len;
			if (usbd_xotg->tx_count[USB_DIR_IN])
				usbd_xotg->tx_count[USB_DIR_IN]--;

			// If bytes remaining for a Bulk IN transfer, return error.
			if (trb->trb_tx_len)
				return XUSB_ERROR_XFER_BULK_IN_RESIDUE;
			break;

		case USB_EP_BULK_OUT:
			// If short packet and Bulk OUT, it's not an error because we prime EP for 4KB.
			usbd_xotg->tx_bytes[USB_DIR_OUT] -= trb->trb_tx_len;
			if (usbd_xotg->tx_count[USB_DIR_OUT])
				usbd_xotg->tx_count[USB_DIR_OUT]--;
			break;
		}
		return USB_RES_OK;
/*
	case XUSB_COMP_USB_TRANSACTION_ERROR:
	case XUSB_COMP_TRB_ERROR:
	case XUSB_COMP_RING_UNDERRUN:
	case XUSB_COMP_RING_OVERRUN:
	case XUSB_COMP_CTRL_DIR_ERROR: // Redefined.
		xusb_set_ep_stall(trb->ep_id, USB_EP_CFG_STALL);
		return USB_RES_OK;
*/
	case XUSB_COMP_BABBLE_DETECTED_ERROR:
		_xusb_wait_ep_stopped(trb->ep_id);
		xusb_set_ep_stall(trb->ep_id, USB_EP_CFG_STALL);
		return XUSB_ERROR_BABBLE_DETECTED;

	case XUSB_COMP_CTRL_DIR_ERROR:
		return XUSB_ERROR_XFER_DIR;

	case XUSB_COMP_CTRL_SEQ_NUM_ERROR:
		return XUSB_ERROR_SEQ_NUM; //! TODO: Can mean a new setup packet was received.

	default: // Every other completion code.
		return USB_ERROR_XFER_ERROR;
	}
}

/*
 * Other XUSB impl:
 * CBT: PR,  PRC,      WPR,       WRC, CSC, REQ, PLC, CEC.
 * LNX: REQ, PRC PR,   PRC & !PR, WRC, CSC, PLC, CEC.
 * BRO: CSC, PR | PRC, WPR | WRC, REQ, PLC, CEC.
 */

static int _xusb_handle_port_change()
{
	u32 status = XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC);
	u32 halt = XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTHALT);
	u32 clear_mask = XHCI_PORTSC_CEC | XHCI_PORTSC_PLC | XHCI_PORTSC_PRC | XHCI_PORTSC_WRC | XHCI_PORTSC_CSC;

	// Port reset (PR).
	if (status & XHCI_PORTSC_PR)
	{
		//! TODO:
		// XHCI_PORTSC_PR: device_state = XUSB_RESET

		//_disable_usb_wdt4();
	}

	// Port Reset Change (PRC).
	if (status & XHCI_PORTSC_PRC)
	{
		// Clear PRC bit.
		status &= ~clear_mask;
		status |= XHCI_PORTSC_PRC;
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC) = status;
	}

	// Warm Port Reset (WPR).
	if (status & XHCI_PORTSC_WPR)
	{
		//_disable_usb_wdt4();

		XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTHALT) &= ~XHCI_PORTHALT_HALT_LTSSM;
		(void)XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTHALT);

		//! TODO: XHCI_PORTSC_WPR: device_state = XUSB_RESET
	}

	// Warm Port Reset Change (WRC).
	if (status & XHCI_PORTSC_WRC)
	{
		// Clear WRC bit.
		status &= ~clear_mask;
		status |= XHCI_PORTSC_WRC;
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC) = status;
	}

	// Reread port status to handle more changes.
	status = XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC);

	// Connect Status Change (CSC).
	if (status & XHCI_PORTSC_CSC)
	{
		//! TODO: Check CCS.
		// CCS check seems to be
		// XHCI_PORTSC_CCS 1: device_state = XUSB_CONNECTED
		// XHCI_PORTSC_CCS 0: device_state = XUSB_DISCONNECTED
		// Always do XHCI_PORTSC_CSC bit clear.

		// Set port speed.
		usbd_xotg->port_speed = (status & XHCI_PORTSC_PS) >> 10;

		// In case host does not support Super Speed, revert the control EP packet size.
		if (usbd_xotg->port_speed != XUSB_SUPER_SPEED)
		{
			volatile xusb_ep_ctx_t *ep_ctxt = &xusb_evtq->xusb_ep_ctxt[XUSB_EP_CTRL_IN];
			ep_ctxt->avg_trb_len = 8;
			ep_ctxt->max_packet_size = 64;
			//! TODO: If super speed is supported, ep context reload, unpause and unhalt must happen.
		}

		// Clear CSC bit.
		status &= ~clear_mask;
		status |= XHCI_PORTSC_CSC;
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC) = status;
	}

	// Handle Config Request (STCHG_REQ).
	if (halt & XHCI_PORTHALT_STCHG_REQ)
	{
		// Clear Link Training Status and pending request/reject.
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTHALT) &= ~XHCI_PORTHALT_HALT_LTSSM;
		(void)XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTHALT);
	}

	// Reread port status to handle more changes.
	status = XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC);

	// Port link state change (PLC).
	if (status & XHCI_PORTSC_PLC)
	{
		// check XHCI_PORTSC_PLS_MASK
		// if XHCI_PORTSC_PLS_U3
		//  device_state = XUSB_SUSPENDED
		// else if XHCI_PORTSC_PLS_U0 and XUSB_SUSPENDED
		//  val = XUSB_DEV_XHCI_EP_PAUSE
		//  XUSB_DEV_XHCI_EP_PAUSE = 0
		//  XUSB_DEV_XHCI_EP_STCHG = val;

		// Clear PLC bit.
		status &= ~clear_mask;
		status |= XHCI_PORTSC_PLC;
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC) = status;
	}

	// Port configuration link error (CEC).
	if (status & XHCI_PORTSC_CEC)
	{
		status = XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC);
		status &= ~clear_mask;
		status |= XHCI_PORTSC_CEC;
		XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC) = status;

		return XUSB_ERROR_PORT_CFG;
	}

	return USB_RES_OK;
}

static int _xusb_handle_get_ep_status(u32 ep_idx)
{
	u32 ep_mask = BIT(ep_idx);
	static u8 xusb_ep_status_descriptor[2] = {0};

	xusb_ep_status_descriptor[0] =
		(XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_HALT) & ep_mask) ? USB_STATUS_EP_HALTED : USB_STATUS_EP_OK;
	return _xusb_issue_data_trb(xusb_ep_status_descriptor, 2, USB_DIR_IN);
}

static int _xusb_handle_get_class_request(usb_ctrl_setup_t *ctrl_setup)
{
	u8 _bRequest = ctrl_setup->bRequest;
	u16 _wIndex  = ctrl_setup->wIndex;
	u16 _wValue  = ctrl_setup->wValue;
	u16 _wLength = ctrl_setup->wLength;

	bool valid_interface = _wIndex == usbd_xotg->interface_num;
	bool valid_len = (_bRequest == USB_REQUEST_BULK_GET_MAX_LUN) ? 1 : 0;

	if (!valid_interface || _wValue != 0 || _wLength != valid_len)
		goto stall;

	switch (_bRequest)
	{
	case USB_REQUEST_BULK_RESET:
		usbd_xotg->bulk_reset_req = true;
		return _xusb_issue_status_trb(USB_DIR_IN); // DELAYED_STATUS;
	case USB_REQUEST_BULK_GET_MAX_LUN:
		if (!usbd_xotg->max_lun_set)
			goto stall;
		usbd_xotg->device_state = XUSB_LUN_CONFIGURED_STS_WAIT;
		return _xusb_issue_data_trb(&usbd_xotg->max_lun, 1, USB_DIR_IN);
	}

stall:
	xusb_set_ep_stall(XUSB_EP_CTRL_IN, USB_EP_CFG_STALL);
	return USB_RES_OK;
}

static int _xusb_handle_get_descriptor(usb_ctrl_setup_t *ctrl_setup)
{
	u32 size;
	void *descriptor;

	u32 wLength = ctrl_setup->wLength;

	u8 descriptor_type = ctrl_setup->wValue >> 8;
	u8 descriptor_subtype = ctrl_setup->wValue & 0xFF;

	switch (descriptor_type)
	{
	case USB_DESCRIPTOR_DEVICE:
		//! TODO USB3: Provide a super speed descriptor.
/*
		u32 soc_rev = APB_MISC(APB_MISC_GP_HIDREV);
		usb_device_descriptor.idProduct  = (soc_rev >> 8)  & 0xFF; // chip_id.
		usb_device_descriptor.idProduct |= ((soc_rev << 4) | (FUSE(FUSE_SKU_INFO) & 0xF)) << 8; // HIDFAM.
		usb_device_descriptor.bcdDevice  = (soc_rev >> 16) & 0xF; // MINORREV.
		usb_device_descriptor.bcdDevice |= ((soc_rev >> 4) & 0xF) << 8; // MAJORREV.
*/
		descriptor = usbd_xotg->desc->dev;
		size = usbd_xotg->desc->dev->bLength;
		break;
	case USB_DESCRIPTOR_CONFIGURATION:
		//! TODO USB3: Provide a super speed descriptor.
		if (usbd_xotg->gadget == USB_GADGET_UMS)
		{
			if (usbd_xotg->port_speed == XUSB_HIGH_SPEED) // High speed. 512 bytes.
			{
				usbd_xotg->desc->cfg->endpoint[0].wMaxPacketSize = 0x200; // No burst.
				usbd_xotg->desc->cfg->endpoint[1].wMaxPacketSize = 0x200; // No burst.
			}
			else // Full speed. 64 bytes.
			{
				usbd_xotg->desc->cfg->endpoint[0].wMaxPacketSize = 0x40;
				usbd_xotg->desc->cfg->endpoint[1].wMaxPacketSize = 0x40;
			}
		}
		else
		{
			usb_cfg_hid_descr_t *tmp = (usb_cfg_hid_descr_t *)usbd_xotg->desc->cfg;
			if (usbd_xotg->port_speed == XUSB_HIGH_SPEED) // High speed. 512 bytes.
			{
				tmp->endpoint[0].wMaxPacketSize = 0x200;
				tmp->endpoint[1].wMaxPacketSize = 0x200;
				tmp->endpoint[0].bInterval = usbd_xotg->gadget == USB_GADGET_HID_GAMEPAD ? 4 : 3; // 8ms : 4ms.
				tmp->endpoint[1].bInterval = usbd_xotg->gadget == USB_GADGET_HID_GAMEPAD ? 4 : 3; // 8ms : 4ms.
			}
			else // Full speed. 64 bytes.
			{
				tmp->endpoint[0].wMaxPacketSize = 0x40;
				tmp->endpoint[1].wMaxPacketSize = 0x40;
				tmp->endpoint[0].bInterval = usbd_xotg->gadget == USB_GADGET_HID_GAMEPAD ? 8 : 4; // 8ms : 4ms.
				tmp->endpoint[1].bInterval = usbd_xotg->gadget == USB_GADGET_HID_GAMEPAD ? 8 : 4; // 8ms : 4ms.
			}
		}
		descriptor = usbd_xotg->desc->cfg;
		size = usbd_xotg->desc->cfg->config.wTotalLength;
		break;
	case USB_DESCRIPTOR_STRING:
		switch (descriptor_subtype)
		{
		case 1:
			descriptor = usbd_xotg->desc->vendor;
			size = usbd_xotg->desc->vendor[0];
			break;
		case 2:
			descriptor = usbd_xotg->desc->product;
			size = usbd_xotg->desc->product[0];
			break;
		case 3:
			descriptor = usbd_xotg->desc->serial;
			size = usbd_xotg->desc->serial[0];
			break;
		case 0xEE:
			descriptor = usbd_xotg->desc->ms_os;
			size = usbd_xotg->desc->ms_os->bLength;
			break;
		default:
			descriptor = usbd_xotg->desc->lang_id;
			size = 4;
			break;
		}
		break;
	case USB_DESCRIPTOR_DEVICE_QUALIFIER:
		if (!usbd_xotg->desc->dev_qual)
		{
			xusb_set_ep_stall(XUSB_EP_CTRL_IN, USB_EP_CFG_STALL);
			return USB_RES_OK;
		}
		usbd_xotg->desc->dev_qual->bNumOtherConfigs = 0;
		descriptor = usbd_xotg->desc->dev_qual;
		size = usbd_xotg->desc->dev_qual->bLength;
		break;
	case USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION:
		if (!usbd_xotg->desc->cfg_other)
		{
			xusb_set_ep_stall(XUSB_EP_CTRL_IN, USB_EP_CFG_STALL);
			return USB_RES_OK;
		}
		if (usbd_xotg->port_speed == XUSB_HIGH_SPEED)
		{
			usbd_xotg->desc->cfg_other->endpoint[0].wMaxPacketSize = 0x40;
			usbd_xotg->desc->cfg_other->endpoint[1].wMaxPacketSize = 0x40;
		}
		else
		{
			usbd_xotg->desc->cfg_other->endpoint[0].wMaxPacketSize = 0x200;
			usbd_xotg->desc->cfg_other->endpoint[1].wMaxPacketSize = 0x200;
		}
		descriptor = usbd_xotg->desc->cfg_other;
		size = usbd_xotg->desc->cfg_other->config.wTotalLength;
		break;
	case USB_DESCRIPTOR_DEVICE_BINARY_OBJECT:
		descriptor = usbd_xotg->desc->dev_bot;
		size = usbd_xotg->desc->dev_bot->wTotalLength;
		break;
	default:
		xusb_set_ep_stall(XUSB_EP_CTRL_IN, USB_EP_CFG_STALL);
		return USB_RES_OK;
	}

	if (wLength < size)
		size = wLength;

	return _xusb_issue_data_trb(descriptor, size, USB_DIR_IN);
}

static void _xusb_handle_set_request_dev_address(usb_ctrl_setup_t *ctrl_setup)
{
	u32 addr = ctrl_setup->wValue & 0xFF;

	XUSB_DEV_XHCI(XUSB_DEV_XHCI_CTRL) = (XUSB_DEV_XHCI(XUSB_DEV_XHCI_CTRL) & 0x80FFFFFF) | (addr << 24);
	xusb_evtq->xusb_ep_ctxt[XUSB_EP_CTRL_IN].device_addr = addr;

	_xusb_issue_status_trb(USB_DIR_IN);

	usbd_xotg->device_state = XUSB_ADDRESSED_STS_WAIT;
}

static void _xusb_handle_set_request_configuration(usb_ctrl_setup_t *ctrl_setup)
{
	u32 config_num = ctrl_setup->wValue;
	if (!config_num) //TODO! we can change device_state here.
		return;

	// Initialize BULK EPs.
	_xusbd_ep_initialize(USB_EP_BULK_OUT);
	_xusbd_ep_initialize(USB_EP_BULK_IN);

	// Device mode start.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_CTRL) |= XHCI_CTRL_RUN;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ST)   |= XHCI_ST_RC;

	_xusb_issue_status_trb(USB_DIR_IN);

	usbd_xotg->config_num = config_num;
	usbd_xotg->device_state = XUSB_CONFIGURED_STS_WAIT;
}

static int _xusbd_handle_ep0_control_transfer(usb_ctrl_setup_t *ctrl_setup)
{
	u32 size;
	u8 *desc;
	bool ep_stall = false;
	bool transmit_data = false;

	u8  _bmRequestType = ctrl_setup->bmRequestType;
	u8  _bRequest      = ctrl_setup->bRequest;
	u16 _wValue        = ctrl_setup->wValue;
	u16 _wIndex        = ctrl_setup->wIndex;
	u16 _wLength       = ctrl_setup->wLength;

	static u8 xusb_dev_status_descriptor[2] = {USB_STATUS_DEV_SELF_POWERED, 0};
	static u8 xusb_interface_descriptor[4] = {0};
	static u8 xusb_configuration_descriptor[2] = {0};
	static u8 xusb_status_descriptor[2] = {0};

	//gfx_printf("ctrl: %02X %02X %04X %04X %04X\n", _bmRequestType, _bRequest, _wValue, _wIndex, _wLength);

	// Unhalt EP0 IN.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_EP_HALT) &= ~XHCI_EP_HALT_DCI_EP0_IN;
	u32 res = _xusb_xhci_mask_wait(XUSB_DEV_XHCI_EP_HALT, XHCI_EP_HALT_DCI_EP0_IN, 0, 1000);
	if (res)
		return res;

	switch (_bmRequestType)
	{
	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_DEVICE):
		if (_bRequest == USB_REQUEST_SET_ADDRESS)
			_xusb_handle_set_request_dev_address(ctrl_setup);
		else if (_bRequest == USB_REQUEST_SET_CONFIGURATION)
			_xusb_handle_set_request_configuration(ctrl_setup);
		return USB_RES_OK; // What about others.

	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE):
		usbd_xotg->interface_num = _wValue;
		return _xusb_issue_status_trb(USB_DIR_IN);

	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_ENDPOINT):
		if ((_wValue & 0xFF) == USB_FEATURE_ENDPOINT_HALT)
		{
			if (_bRequest == USB_REQUEST_CLEAR_FEATURE || _bRequest == USB_REQUEST_SET_FEATURE)
			{
				u32 ep = 0;
				switch (_wIndex) // endpoint
				{
				case USB_EP_ADDR_CTRL_OUT:
					ep = XUSB_EP_CTRL_OUT;
					break;
				case USB_EP_ADDR_CTRL_IN:
					ep = XUSB_EP_CTRL_IN;
					break;
				case USB_EP_ADDR_BULK_OUT:
					ep = USB_EP_BULK_OUT;
					break;
				case USB_EP_ADDR_BULK_IN:
					ep = USB_EP_BULK_IN;
					break;
				default:
					xusb_set_ep_stall(XUSB_EP_CTRL_IN, USB_EP_CFG_STALL);
					return USB_RES_OK;
				}

				if (_bRequest == USB_REQUEST_CLEAR_FEATURE)
					xusb_set_ep_stall(ep, USB_EP_CFG_CLEAR);
				else if (_bRequest == USB_REQUEST_SET_FEATURE)
					xusb_set_ep_stall(ep, USB_EP_CFG_STALL);

				return _xusb_issue_status_trb(USB_DIR_IN);
			}
		}
		ep_stall = true;
		break;

	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS    | USB_SETUP_RECIPIENT_INTERFACE):
		return _xusb_handle_get_class_request(ctrl_setup);

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_DEVICE):
		switch (_bRequest)
		{
		case USB_REQUEST_GET_STATUS:
			desc = xusb_dev_status_descriptor;
			size = sizeof(xusb_dev_status_descriptor);
			transmit_data = true;
			break;
		case USB_REQUEST_GET_DESCRIPTOR:
			return _xusb_handle_get_descriptor(ctrl_setup);
		case USB_REQUEST_GET_CONFIGURATION:
			xusb_configuration_descriptor[0] = usbd_xotg->config_num;
			desc = xusb_configuration_descriptor;
			size = sizeof(xusb_configuration_descriptor);
			transmit_data = true;
			break;
		default:
			ep_stall = true;
			break;
		}
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE):
		if (_bRequest == USB_REQUEST_GET_INTERFACE)
		{
			desc = xusb_interface_descriptor;
			size = sizeof(xusb_interface_descriptor);
			xusb_interface_descriptor[0] = usbd_xotg->interface_num;
			transmit_data = true;
		}
		else if (_bRequest == USB_REQUEST_GET_STATUS)
		{
			desc = xusb_status_descriptor;
			size = sizeof(xusb_status_descriptor);
			transmit_data = true;
		}
		else if (_bRequest == USB_REQUEST_GET_DESCRIPTOR && (_wValue >> 8) == USB_DESCRIPTOR_HID_REPORT && usbd_xotg->gadget > USB_GADGET_UMS)
		{
			if (usbd_xotg->gadget == USB_GADGET_HID_GAMEPAD)
			{
				desc = (u8 *)&hid_report_descriptor_jc;
				size = hid_report_descriptor_jc_size;
			}
			else // USB_GADGET_HID_TOUCHPAD
			{
				desc = (u8 *)&hid_report_descriptor_touch;
				size = hid_report_descriptor_touch_size;
			}
			transmit_data = true;
			usbd_xotg->device_state = XUSB_HID_CONFIGURED_STS_WAIT;
		}
		else
			ep_stall = true;
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_ENDPOINT):
		if (_bRequest == USB_REQUEST_GET_STATUS)
		{
			u32 ep = 0;
			switch (_wIndex) // endpoint
			{
			case USB_EP_ADDR_CTRL_OUT:
				ep = XUSB_EP_CTRL_OUT;
				break;
			case USB_EP_ADDR_CTRL_IN:
				ep = XUSB_EP_CTRL_IN;
				break;
			case USB_EP_ADDR_BULK_OUT:
				ep = USB_EP_BULK_OUT;
				break;
			case USB_EP_ADDR_BULK_IN:
				ep = USB_EP_BULK_IN;
				break;
			default:
				xusb_set_ep_stall(XUSB_EP_CTRL_IN, USB_EP_CFG_STALL);
				return USB_RES_OK;
			}
			return _xusb_handle_get_ep_status(ep);
		}

		ep_stall = true;
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_CLASS    | USB_SETUP_RECIPIENT_INTERFACE):
		return _xusb_handle_get_class_request(ctrl_setup);

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_VENDOR   | USB_SETUP_RECIPIENT_INTERFACE):
	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_VENDOR   | USB_SETUP_RECIPIENT_DEVICE):
		if (_bRequest == USB_REQUEST_GET_MS_DESCRIPTOR)
		{
			switch (_wIndex)
			{
			case USB_DESCRIPTOR_MS_COMPAT_ID:
				desc = (u8 *)usbd_xotg->desc->ms_cid;
				size = usbd_xotg->desc->ms_cid->dLength;
				transmit_data = true;
				break;
			case USB_DESCRIPTOR_MS_EXTENDED_PROPERTIES:
				desc = (u8 *)usbd_xotg->desc->mx_ext;
				size = usbd_xotg->desc->mx_ext->dLength;
				transmit_data = true;
				break;
			default:
				ep_stall = true;
				break;
			}
		}
		else
			ep_stall = true;
		break;

	default:
		ep_stall = true;
		break;
	}

	if (transmit_data)
	{
		memcpy((u8 *)USB_EP_CONTROL_BUF_ADDR, desc, size);
		if (_wLength < size)
				size = _wLength;
		return _xusb_issue_data_trb((u8 *)USB_EP_CONTROL_BUF_ADDR, size, USB_DIR_IN);
	}

	if (ep_stall)
		xusb_set_ep_stall(XUSB_EP_CTRL_IN, USB_EP_CFG_STALL);

	return USB_RES_OK;
}

static int _xusb_ep_operation(u32 tries)
{
	usb_ctrl_setup_t setup_event;
	volatile event_trb_t *event_trb;
	setup_event_trb_t *setup_event_trb;

	// Wait for an interrupt event.
	int res = _xusb_xhci_mask_wait(XUSB_DEV_XHCI_ST, XHCI_ST_IP, XHCI_ST_IP, tries);
	if (res)
		return res;

	// Clear interrupt status.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ST) |= XHCI_ST_IP;

	usbd_xotg->event_enqueue_ptr = (event_trb_t *)(XUSB_DEV_XHCI(XUSB_DEV_XHCI_EREPLO) & 0xFFFFFFF0);
	event_trb = usbd_xotg->event_dequeue_ptr;

	// Check if cycle matches.
	if ((event_trb->cycle & 1) != usbd_xotg->event_ccs)
		return XUSB_ERROR_INVALID_CYCLE;

	while ((event_trb->cycle & 1) == usbd_xotg->event_ccs)
	{
		switch (event_trb->trb_type)
		{
		case XUSB_TRB_TRANSFER:
			res = _xusb_handle_transfer_event((transfer_event_trb_t *)event_trb);
			break;
		case XUSB_TRB_PORT_CHANGE:
			res = _xusb_handle_port_change();
			break;
		case XUSB_TRB_SETUP:
			setup_event_trb = (setup_event_trb_t *)event_trb;
			memcpy(&setup_event, &setup_event_trb->ctrl_setup_data, sizeof(usb_ctrl_setup_t));
			usbd_xotg->ctrl_seq_num = setup_event_trb->ctrl_seq_num;
			res = _xusbd_handle_ep0_control_transfer(&setup_event);
			break;
		default:
			// TRB not supported.
			break;
		}

		// Check if last event TRB and reset to first one.
		if (usbd_xotg->event_dequeue_ptr == &xusb_evtq->xusb_event_ring_seg1[XUSB_LAST_TRB_IDX])
		{
			usbd_xotg->event_dequeue_ptr = xusb_evtq->xusb_event_ring_seg0;
			usbd_xotg->event_ccs ^= 1;
		}
		else // Advance dequeue to next event.
			usbd_xotg->event_dequeue_ptr = &usbd_xotg->event_dequeue_ptr[1];

		// Set next event.
		event_trb = usbd_xotg->event_dequeue_ptr;

		// If events exceed the interrupt time, handle them next interrupt.
		if (usbd_xotg->event_dequeue_ptr == usbd_xotg->event_enqueue_ptr)
			break;
	}

	// Clear Event Handler bit if enabled and set Dequeue pointer.
	u32 erdp = XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERDPLO) & 0xF;
	if (erdp & XHCI_ERDPLO_EHB)
		erdp |= XHCI_ERDPLO_EHB;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_ERDPLO) = ((u32)usbd_xotg->event_dequeue_ptr & 0xFFFFFFF0) | erdp;

	return res;
}

int xusb_device_enumerate(usb_gadget_type gadget)
{
	switch (gadget)
	{
	case USB_GADGET_UMS:
		usbd_xotg->desc = &usb_gadget_ums_descriptors;
		break;
	case USB_GADGET_HID_GAMEPAD:
		usbd_xotg->desc = &usb_gadget_hid_jc_descriptors;
		break;
	case USB_GADGET_HID_TOUCHPAD:
		usbd_xotg->desc = &usb_gadget_hid_touch_descriptors;
		break;
	}

	usbd_xotg->gadget = gadget;

	/*
	 * Set interrupt moderation to 0us.
	 * This is important because default value creates a 4.62ms latency.
	 * Effectively hurting transfers by having 15% to 96% performance loss.
	 */
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_RT_IMOD) = 0;

	// Disable Wake events.
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_0) = 0;
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) = 0;

	// Enable overrides for VBUS and ID.
	XUSB_PADCTL(XUSB_PADCTL_USB2_VBUS_ID) = (XUSB_PADCTL(XUSB_PADCTL_USB2_VBUS_ID) & ~(PADCTL_USB2_VBUS_ID_VBUS_OVR_MASK | PADCTL_USB2_VBUS_ID_SRC_MASK)) |
		PADCTL_USB2_VBUS_ID_VBUS_OVR_EN | PADCTL_USB2_VBUS_ID_SRC_ID_OVR_EN;

	// Clear halt for LTSSM.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTHALT) &= ~XHCI_PORTHALT_HALT_LTSSM;

	// Enable device mode.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_CTRL) |= XHCI_CTRL_ENABLE;

	// Override access to High/Full Speed.
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_CFG_DEV_FE) = (XUSB_DEV_XHCI(XUSB_DEV_XHCI_CFG_DEV_FE) & ~XHCI_CFG_DEV_FE_PORTREGSEL_MASK) | XHCI_CFG_DEV_FE_PORTREGSEL_HSFS;

	XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC) =
		(XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC) & ~XHCI_PORTSC_PLS_MASK) | XHCI_PORTSC_LWS | XHCI_PORTSC_PLS_RXDETECT;
	XUSB_DEV_XHCI(XUSB_DEV_XHCI_CFG_DEV_FE) &= ~XHCI_CFG_DEV_FE_PORTREGSEL_MASK;

	// Enable VBUS and set ID to Float.
	XUSB_PADCTL(XUSB_PADCTL_USB2_VBUS_ID) = (XUSB_PADCTL(XUSB_PADCTL_USB2_VBUS_ID) & ~PADCTL_USB2_VBUS_ID_OVR_MASK) |
		PADCTL_USB2_VBUS_ID_OVR_FLOAT | PADCTL_USB2_VBUS_ID_VBUS_ON;

	usbd_xotg->wait_for_event_trb = XUSB_TRB_SETUP;
	usbd_xotg->device_state = XUSB_DEFAULT;

	// Timeout if cable or communication isn't started in 1.5 minutes.
	u32 timer = get_tmr_ms() + 90000;
	while (true)
	{
		int res = _xusb_ep_operation(USB_XFER_SYNCED_ENUM); // 2s timeout.
		if (res && res != USB_ERROR_TIMEOUT)
			return res;

		if (usbd_xotg->device_state == XUSB_CONFIGURED)
			break;

		if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			return USB_ERROR_USER_ABORT;
	}

	return USB_RES_OK;
}

//! TODO: Do a full deinit.
void xusb_end(bool reset_ep, bool only_controller)
{
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_SET) = BIT(CLK_W_XUSB_SS);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_CLR) = BIT(CLK_W_XUSB_SS);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_U_SET) = BIT(CLK_U_XUSB_DEV);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_U_CLR) = BIT(CLK_U_XUSB_DEV);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_SET) = BIT(CLK_W_XUSB_PADCTL);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_CLR) = BIT(CLK_W_XUSB);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_SET) = BIT(CLK_W_XUSB);
	mc_disable_ahb_redirect(); // Can be skipped if IRAM is not used.
}

int xusb_handle_ep0_ctrl_setup()
{
	/*
	 * EP0 Control handling is done by normal ep operation in XUSB.
	 * Here we handle the bulk reset only.
	 */
	if (usbd_xotg->bulk_reset_req)
	{
		usbd_xotg->bulk_reset_req = false;
		return USB_RES_BULK_RESET;
	}

	return USB_RES_OK;
}

int xusb_device_ep1_out_read(u8 *buf, u32 len, u32 *bytes_read, u32 sync_tries)
{
	if (len > USB_EP_BUFFER_MAX_SIZE)
		len = USB_EP_BUFFER_MAX_SIZE;

	int res = USB_RES_OK;
	usbd_xotg->tx_count[USB_DIR_OUT] = 0;
	usbd_xotg->tx_bytes[USB_DIR_OUT] = len;
	_xusb_issue_normal_trb(buf, len, USB_DIR_OUT);
	usbd_xotg->tx_count[USB_DIR_OUT]++;

	if (sync_tries)
	{
		while (!res && usbd_xotg->tx_count[USB_DIR_OUT])
			res = _xusb_ep_operation(sync_tries);

		if (bytes_read)
			*bytes_read = res ? 0 : usbd_xotg->tx_bytes[USB_DIR_OUT];

		bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);
	}

	return res;
}

int xusb_device_ep1_out_read_big(u8 *buf, u32 len, u32 *bytes_read)
{
	if (len > USB_EP_BULK_OUT_MAX_XFER)
		len = USB_EP_BULK_OUT_MAX_XFER;

	u32 bytes = 0;
	*bytes_read = 0;
	u8 *buf_curr = buf;

	while (len)
	{
		u32 len_ep = MIN(len, USB_EP_BUFFER_MAX_SIZE);

		int res = xusb_device_ep1_out_read(buf_curr, len_ep, &bytes, USB_XFER_SYNCED_DATA);
		if (res)
			return res;

		len -= len_ep;
		buf_curr += len_ep;
		*bytes_read = *bytes_read + bytes;
	}

	return USB_RES_OK;
}

int xusb_device_ep1_out_reading_finish(u32 *pending_bytes, u32 sync_tries)
{
	int res = USB_RES_OK;
	while (!res && usbd_xotg->tx_count[USB_DIR_OUT])
		res = _xusb_ep_operation(sync_tries); // Infinite retries.

	if (pending_bytes)
		*pending_bytes = res ? 0 : usbd_xotg->tx_bytes[USB_DIR_OUT];

	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	return res;
}

int xusb_device_ep1_in_write(u8 *buf, u32 len, u32 *bytes_written, u32 sync_tries)
{
	if (len > USB_EP_BUFFER_MAX_SIZE)
		len = USB_EP_BUFFER_MAX_SIZE;

	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	int res = USB_RES_OK;
	usbd_xotg->tx_count[USB_DIR_IN] = 0;
	usbd_xotg->tx_bytes[USB_DIR_IN] = len;
	_xusb_issue_normal_trb(buf, len, USB_DIR_IN);
	usbd_xotg->tx_count[USB_DIR_IN]++;

	if (sync_tries)
	{
		while (!res && usbd_xotg->tx_count[USB_DIR_IN])
			res = _xusb_ep_operation(sync_tries);

		if (bytes_written)
			*bytes_written = res ? 0 : usbd_xotg->tx_bytes[USB_DIR_IN];
	}
	else
	{
		if ((usbd_xotg->port_speed == XUSB_FULL_SPEED  && len == 64)  ||
			(usbd_xotg->port_speed == XUSB_HIGH_SPEED  && len == 512) ||
			(usbd_xotg->port_speed == XUSB_SUPER_SPEED && len == 1024))
		{
			_xusb_issue_normal_trb(buf, 0, USB_DIR_IN);
			usbd_xotg->tx_count[USB_DIR_IN]++;
		}
	}

	return res;
}

int xusb_device_ep1_in_writing_finish(u32 *pending_bytes, u32 sync_tries)
{
	int res = USB_RES_OK;
	while (!res && usbd_xotg->tx_count[USB_DIR_IN])
		res = _xusb_ep_operation(sync_tries); // Infinite retries.

	if (pending_bytes)
		*pending_bytes = res ? 0 : usbd_xotg->tx_bytes[USB_DIR_IN];

	return res;
}

bool xusb_device_get_port_in_sleep()
{
	// Ejection heuristic.
	u32 link_mode = XUSB_DEV_XHCI(XUSB_DEV_XHCI_PORTSC) & XHCI_PORTSC_PLS_MASK;
	return (link_mode == XHCI_PORTSC_PLS_U3);
}

bool xusb_device_class_send_max_lun(u8 max_lun)
{
	// Timeout if get MAX_LUN request doesn't happen in 10s.
	u32 timer = get_tmr_ms() + 10000;

	usbd_xotg->max_lun = max_lun;
	usbd_xotg->max_lun_set = true;

	// Wait for request and transfer start.
	while (usbd_xotg->device_state != XUSB_LUN_CONFIGURED)
	{
		_xusb_ep_operation(USB_XFER_SYNCED_CLASS);
		if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			return true;
	}

	usbd_xotg->device_state = XUSB_CONFIGURED;

	return false;
}

bool xusb_device_class_send_hid_report()
{
	// Timeout if get GET_HID_REPORT request doesn't happen in 10s.
	u32 timer = get_tmr_ms() + 10000;

	// Wait for request and transfer start.
	while (usbd_xotg->device_state != XUSB_HID_CONFIGURED)
	{
		_xusb_ep_operation(USB_XFER_SYNCED_CLASS);
		if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			return true;
	}

	usbd_xotg->device_state = XUSB_CONFIGURED;

	return false;
}

void xusb_device_get_ops(usb_ops_t *ops)
{
	ops->usbd_flush_endpoint               = NULL;
	ops->usbd_set_ep_stall                 = xusb_set_ep_stall;
	ops->usbd_handle_ep0_ctrl_setup        = xusb_handle_ep0_ctrl_setup;
	ops->usbd_end                          = xusb_end;
	ops->usb_device_init                   = xusb_device_init;
	ops->usb_device_enumerate              = xusb_device_enumerate;
	ops->usb_device_class_send_max_lun     = xusb_device_class_send_max_lun;
	ops->usb_device_class_send_hid_report  = xusb_device_class_send_hid_report;
	ops->usb_device_get_suspended          = xusb_device_get_port_in_sleep;
	ops->usb_device_get_port_in_sleep      = xusb_device_get_port_in_sleep;

	ops->usb_device_ep1_out_read           = xusb_device_ep1_out_read;
	ops->usb_device_ep1_out_read_big       = xusb_device_ep1_out_read_big;
	ops->usb_device_ep1_out_reading_finish = xusb_device_ep1_out_reading_finish;
	ops->usb_device_ep1_in_write           = xusb_device_ep1_in_write;
	ops->usb_device_ep1_in_writing_finish  = xusb_device_ep1_in_writing_finish;
}
