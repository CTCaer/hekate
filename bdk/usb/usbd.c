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

#include <string.h>
#include <stdlib.h>

#include <usb/usbd.h>
#include <usb/usb_descriptors.h>
#include <usb/usb_t210.h>

#include <gfx_utils.h>
#include <soc/bpmp.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/gpio.h>
#include <soc/pinmux.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <utils/btn.h>
#include <utils/util.h>

#include <memory_map.h>

typedef enum
{
    USB_HW_EP0 = 0,
    USB_HW_EP1 = 1
} usb_hw_ep_t;

typedef enum
{
    USB_EP_ADDR_CTRL_OUT = 0x00,
    USB_EP_ADDR_CTRL_IN  = 0x80,
    USB_EP_ADDR_BULK_OUT = 0x01,
    USB_EP_ADDR_BULK_IN  = 0x81,
} usb_ep_addr_t;

typedef enum
{
    USB_EP_CFG_RESET = 0,
    USB_EP_CFG_STALL = 1
} usb_ep_cfg_t;

typedef enum
{
	USB_EP_STATUS_IDLE      = 0,
	USB_EP_STATUS_ACTIVE    = 1,
	USB_EP_STATUS_ERROR     = 2,
	USB_EP_STATUS_NO_CONFIG = 3,
	USB_EP_STATUS_STALLED   = 4,
	USB_EP_STATUS_DISABLED  = 5
} usb_ep_status_t;

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

	USB_REQUEST_GET_MS_DESCRIPTOR = 0x99,

	USB_REQUEST_BULK_GET_MAX_LUN  = 0xFE,
	USB_REQUEST_BULK_RESET        = 0xFF
} usb_standard_req_t;

typedef enum {
	USB_FEATURE_ENDPOINT_HALT        = 0,
	USB_FEATURE_DEVICE_REMOTE_WAKEUP = 1,
	USB_FEATURE_TEST_MODE            = 2,
} usb_get_status_req_t;

typedef enum {
	USB_STATUS_EP_OK            = 0,
	USB_STATUS_EP_HALTED        = 1,

	USB_STATUS_DEV_SELF_POWERED = 1,
	USB_STATUS_DEV_REMOTE_WAKE  = 2,
} usb_set_clear_feature_req_t;

typedef enum {
	USB_XFER_DIR_OUT = 0,
	USB_XFER_DIR_IN  = 1,
} usb_xfer_dir_t;

typedef enum {
	USB_SPEED_LOW   = 0,
	USB_SPEED_FULL  = 1,
	USB_SPEED_HIGH  = 2,
	USB_SPEED_SUPER = 3,
} usb_speed_t;

typedef enum {
	USB_XFER_TYPE_CONTROL = 0,
	USB_XFER_TYPE_ISOCHRONOUS = 1,
	USB_XFER_TYPE_BULK = 2,
	USB_XFER_TYPE_INTERRUPT = 3,
} usb_xfer_type_t;

typedef struct _dTD_t
{
	vu32 next_dTD;
	vu32 info;
	vu32 pages[5];
	vu32 reserved;
} dTD_t;

typedef struct _dQH_t
{
	vu32 ep_capabilities;
	vu32 curr_dTD_ptr;
	vu32 next_dTD_ptr;
	vu32 token;
	vu32 buffers[5]; // hmmm.
	vu32 reserved;
	vu32 setup[2];
	vu32 gap[4];
} dQH_t;

typedef struct _usbd_t
{
	volatile dTD_t dtds[4 * 4]; // 4 dTD per endpoint.
	volatile dQH_t *qhs;
	int ep_configured[4];
	int ep_bytes_requested[4];
} usbd_t;

typedef struct _usb_ctrl_setup_t
{
	u8 bmRequestType;
	u8 bRequest;
	u16 wValue;
	u16 wIndex;
	u16 wLength;
} usb_ctrl_setup_t;

typedef struct _usbd_controller_t
{
	u32 port_speed;
	t210_usb2d_t *regs;
	usb_ctrl_setup_t control_setup;
	usb_desc_t *desc;
	usb_gadget_type type;
	u8 configuration_set;
	u8 usb_phy_ready;
	u8 configuration;
	u8 interface;
	u8 max_lun;
	u8 max_lun_set;
	u8 bulk_reset_req;
	u8 hid_report_sent;
	bool charger_detect;
} usbd_controller_t;

u8 usb_serial_string_descriptor[26] =
{
	26, 0x03,
	'C', 0x00, '7', 0x00, 'C', 0x00, '0', 0x00,
	'9', 0x00, '2', 0x00, '4', 0x00, '2', 0x00, 'F', 0x00, '7', 0x00, '0', 0x00, '3', 0x00
};

u8 usb_lang_id_string_descriptor[] =
{
	4, 3,
	0x09, 0x04
};

usbd_t *usbdaemon;

usbd_controller_t *usbd_otg;
usbd_controller_t usbd_usb_otg_controller_ctxt;

bool usb_init_done = false;

u8 *usb_ep0_ctrl_buf = (u8 *)USB_EP_CONTROL_BUF_ADDR;

static int _usbd_reset_usb_otg_phy_device_mode()
{
	usbd_otg->usb_phy_ready = 0;

	// Clear UTMIP reset.
	USB(USB1_IF_USB_SUSP_CTRL) &= ~SUSP_CTRL_UTMIP_RESET;

	// Wait for PHY clock to get validated.
	u32 retries = 100000; // 200ms timeout.
	while (!(USB(USB1_IF_USB_SUSP_CTRL) & SUSP_CTRL_USB_PHY_CLK_VALID))
	{
		retries--;
		if (!retries)
			return 1;
		usleep(1);
	}
	usbd_otg->usb_phy_ready = 1;

	// Clear all device addresses, enabled setup requests and transmit events.
	usbd_otg->regs->periodiclistbase = 0;
	usbd_otg->regs->endptsetupstat = usbd_otg->regs->endptsetupstat;
	usbd_otg->regs->endptcomplete = usbd_otg->regs->endptcomplete;

	// Stop device controller.
	usbd_otg->regs->usbcmd &= ~USB2D_USBCMD_RUN;

	// Set controller mode to idle.
	usbd_otg->regs->usbmode &= ~USB2D_USBMODE_CM_MASK;

	// Reset the controller.
	usbd_otg->regs->usbcmd |= USB2D_USBCMD_RESET;

	// Wait for the reset to complete.
	retries = 100000; // 200ms timeout.
	while (usbd_otg->regs->usbcmd & USB2D_USBCMD_RESET)
	{
		retries--;
		if (!retries)
			return 2;
		usleep(1);
	}

	// Wait for PHY clock to get validated after reset.
	retries = 100000; // 200ms timeout.
	while (!(USB(USB1_IF_USB_SUSP_CTRL) & SUSP_CTRL_USB_PHY_CLK_VALID))
	{
		retries--;
		if (!retries)
			return 3;
		usleep(1);
	}

	// Set controller to Device mode.
	usbd_otg->regs->usbmode = (usbd_otg->regs->usbmode & ~USB2D_USBMODE_CM_MASK) | USB2D_USBMODE_CM_DEVICE;

	// Wait for the selected mode to be enabled.
	retries = 100000; // 200ms timeout.
	while ((usbd_otg->regs->usbmode & USB2D_USBMODE_CM_MASK) != USB2D_USBMODE_CM_DEVICE)
	{
		retries--;
		if (!retries)
			return 4;
		usleep(1);
	}

	// Disable all interrupts.
	usbd_otg->regs->usbintr = 0;

	// Set the ID pullup and disable all OTGSC interrupts.
	usbd_otg->regs->otgsc = USB2D_OTGSC_USB_ID_PULLUP;

	// Clear all relevant interrupt statuses.
	usbd_otg->regs->usbsts =
		USB2D_USBSTS_UI | USB2D_USBSTS_UEI | USB2D_USBSTS_PCI |
		USB2D_USBSTS_FRI | USB2D_USBSTS_SEI | USB2D_USBSTS_AAI |
		USB2D_USBSTS_URI | USB2D_USBSTS_SRI | USB2D_USBSTS_SLI;

	// Disable and clear all OTGSC interrupts.
	usbd_otg->regs->otgsc = USB2D_OTGSC_USB_IRQ_STS_MASK;

	// Clear EP0, EP1, EP2 setup requests.
	usbd_otg->regs->endptsetupstat = 7; //TODO: Shouldn't this be endptsetupstat = endptsetupstat?

	// Set all interrupts to immediate.
	usbd_otg->regs->usbcmd &= ~USB2D_USBCMD_ITC_MASK;

	return 0;
}

static void _usb_charger_detect()
{
	// Charger detect init.
	usbd_otg->charger_detect = 0;
	bool charger_detect_enable = FUSE(FUSE_RESERVED_SW) & 0x10; // Disabled on Switch production.
	if (charger_detect_enable)
	{
		usbd_otg->charger_detect |= 1;
		// Configure detect pin.
		PINMUX_AUX(PINMUX_AUX_LCD_GPIO1) &= ~(PINMUX_PARKED | PINMUX_TRISTATE | PINMUX_PULL_MASK);
		gpio_config(GPIO_PORT_V, GPIO_PIN_3, GPIO_MODE_GPIO);

		// Configure charger pin.
		PINMUX_AUX(PINMUX_AUX_USB_VBUS_EN0) &=
			~(PINMUX_INPUT_ENABLE | PINMUX_PARKED | PINMUX_TRISTATE | PINMUX_PULL_MASK);
		gpio_config(GPIO_PORT_CC, GPIO_PIN_5, GPIO_MODE_GPIO);
		gpio_output_enable(GPIO_PORT_CC, GPIO_PIN_5, GPIO_OUTPUT_ENABLE);

		// Enable charger.
		if (gpio_read(GPIO_PORT_V, GPIO_PIN_3))
		{
			usbd_otg->charger_detect |= 2;
			gpio_write(GPIO_PORT_CC, GPIO_PIN_5, GPIO_HIGH);
			usbd_otg->charger_detect |= 0x100;
			USB(USB1_UTMIP_BAT_CHRG_CFG0) = BAT_CHRG_CFG0_OP_SRC_EN; // Clears UTMIP_PD_CHRG and enables charger detect.
			usleep(5000);
		}
	}
}

int usb_device_init()
{
	if (usb_init_done)
		return 0;

	// Configure PLLU.
	CLOCK(CLK_RST_CONTROLLER_PLLU_MISC) = CLOCK(CLK_RST_CONTROLLER_PLLU_MISC) | 0x20000000; // Disable reference clock.
	u32 pllu_cfg = (((((CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) >> 8 << 8) | 2) & 0xFFFF00FF) | ((0x19 << 8) & 0xFFFF)) & 0xFFE0FFFF) | (1<< 16) | 0x1000000;
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) = pllu_cfg;
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) = pllu_cfg | 0x40000000; // Enable.

	// Wait for PLL to stabilize.
	u32 timeout = (u32)TMR(TIMERUS_CNTR_1US) + 1300;
	while (!(CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) & (1 << 27))) // PLL_LOCK.
		if ((u32)TMR(TIMERUS_CNTR_1US) > timeout)
			break;
	usleep(10);

	// Enable PLLU USB/HSIC/ICUSB/48M.
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) = CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) | 0x2600000 | 0x800000;

	// Enable USBD clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = (1 << 22);
	usleep(2);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = (1 << 22);
	usleep(2);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = (1 << 22);
	usleep(2);

	// Clear XUSB_PADCTL reset
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_CLR) = (1 << 14);

	// Enable USB PHY and reset for programming.
	u32 usb_susp_ctrl = USB(USB1_IF_USB_SUSP_CTRL);
	USB(USB1_IF_USB_SUSP_CTRL) = usb_susp_ctrl | SUSP_CTRL_UTMIP_RESET;
	USB(USB1_IF_USB_SUSP_CTRL) = usb_susp_ctrl | SUSP_CTRL_UTMIP_PHY_ENB | SUSP_CTRL_UTMIP_RESET;

	// Disable UTMIPLL IDDQ.
	CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) &= 0xFFFFFFFD;
	usleep(10);

	// Disable crystal clock.
	USB(USB1_UTMIP_MISC_CFG1) &= 0xBFFFFFFF;
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) &= 0xBFFFFFFF;

	// Set B_SESS_VLD.
	USB(USB1_IF_USB_PHY_VBUS_SENSORS) |= 0x1000;
	USB(USB1_IF_USB_PHY_VBUS_SENSORS) |= 0x800;

	// Set UTMIPLL dividers and enable it.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG0) = (CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG0) & 0xFF0000FF) | 0x190000 | 0x100;
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) = (CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) & 0xFF00003F) | 0x600000; // Set delay count for 38.4Mhz osc crystal.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) = ((CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) & 0x7FFF000) | 0x8000 | 0x177) & 0xFFFFAFFF;

	// Wait for UTMIPLL to stabilize.
	u32 retries = 10; // Wait 20us
	while (!(CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) & 0x80000000) && retries)
	{
		usleep(1);
		retries--;
	}

	// Configure UTMIP Transceiver Cells.
	u32 fuse_usb_calib = FUSE(FUSE_USB_CALIB);
	USB(USB1_UTMIP_XCVR_CFG0) = (((USB(USB1_UTMIP_XCVR_CFG0) & 0xFFFFFFF0) | (fuse_usb_calib & 0xF)) & 0xFE3FFFFF) | ((fuse_usb_calib & 0x3F) << 25 >> 29 << 22);
	USB(USB1_UTMIP_XCVR_CFG1) = (USB(USB1_UTMIP_XCVR_CFG1) & 0xFFC3FFFF) | ((fuse_usb_calib << 21) >> 28 << 18);
	USB(USB1_UTMIP_XCVR_CFG3) = (USB(USB1_UTMIP_XCVR_CFG3) & 0xFFFFC1FF) | ((FUSE(FUSE_USB_CALIB_EXT) & 0x1F) << 9);
	USB(USB1_UTMIP_XCVR_CFG0) &= 0xFFDFFFFF;
	USB(USB1_UTMIP_XCVR_CFG2) = (USB(USB1_UTMIP_XCVR_CFG2) & 0xFFFFF1FF) | 0x400;
	usleep(1);

	// Configure misc UTMIP.
	USB(USB1_UTMIP_DEBOUNCE_CFG0) = (USB(USB1_UTMIP_DEBOUNCE_CFG0) & 0xFFFF0000) | 0xBB80;
	USB(USB1_UTMIP_BIAS_CFG1) = (USB(USB1_UTMIP_BIAS_CFG1) & 0xFFFFC0FF) | 0x100; // when osc is 38.4KHz

	//USB(USB1_UTMIP_SPARE_CFG0) &= 0xFFFFFEE7; unpatched0
	USB(USB1_UTMIP_BIAS_CFG2) |= 2; //patched0 - UTMIP_HSSQUELCH_LEVEL_NEW: 2.
	USB(USB1_UTMIP_SPARE_CFG0) &= 0xFFFFFE67; //patched0 - FUSE_HS_IREF_CAP_CFG
	USB(USB1_UTMIP_TX_CFG0) |= 0x80000;

	//USB(USB1_UTMIP_HSRX_CFG0) = (USB(USB1_UTMIP_HSRX_CFG0) & 0xFFF003FF) | 0x88000 | 0x4000; unpatched1
	USB(USB1_UTMIP_HSRX_CFG0) = (USB(USB1_UTMIP_HSRX_CFG0) & 0xF0F003FF) | 0x88000 | 0x4000; //patched1 - reset UTMIP_PCOUNT_UPDN_DIV: From 1 to 0.
	USB(USB1_UTMIP_BIAS_CFG2) &= 0xFFFFFFF8; //patched1 - UTMIP_HSSQUELCH_LEVEL_NEW: 0

	USB(USB1_UTMIP_HSRX_CFG1) = (USB(USB1_UTMIP_HSRX_CFG1) & 0xFFFFFFC1) | 0x12;
	USB(USB1_UTMIP_MISC_CFG1) |= 0x40000000;

	// Enable crystal clock.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) |= 0x40000000;
	// Enable USB2 tracking.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) |= 0x40000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_USB2_HSIC_TRK) = (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_USB2_HSIC_TRK) & 0xFFFFFF00) | 6; // Set trank divisor to 4.

	USB(USB1_UTMIP_BIAS_CFG1) = (USB(USB1_UTMIP_BIAS_CFG1) & 0xFFC03F07) | 0x78000 | 0x50; // Set delays.
	USB(USB1_UTMIP_BIAS_CFG0) &= 0xFFFFFBFF; // Disable Power down bias circuit.
	usleep(1);

	// Force PDTRK input into power up.
	USB(USB1_UTMIP_BIAS_CFG1) = (USB(USB1_UTMIP_BIAS_CFG1) & 0xFFFFFFFE) | 2;
	usleep(100);

	// TRK cycle done. Force PDTRK input into power down.
	USB(USB1_UTMIP_BIAS_CFG1) = (USB(USB1_UTMIP_BIAS_CFG1) & 0xFF7FFFFF) | 1;
	usleep(3);

	// Force PDTRK input into power up.
	USB(USB1_UTMIP_BIAS_CFG1) = USB(USB1_UTMIP_BIAS_CFG1) & 0xFFFFFFFE;
	usleep(100);

	// TRK cycle done. Force PDTRK input into power down.
	USB(USB1_UTMIP_BIAS_CFG1) = (USB(USB1_UTMIP_BIAS_CFG1) & 0xFF7FFFFF) | 1;

	// Disable USB2_TRK clock and configure UTMIP misc.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) &= 0xFFFBFFFF;
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) = (CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) & 0xFEFFFFEA) | 0x2000000 | 0x28 | 2;
	usleep(1);

	USB(USB1_UTMIP_BIAS_CFG0) &= 0xFF3FF7FF;
	usleep(1);

	// Clear power downs on UTMIP ID and VBUS wake up, PD, PD2, PDZI, PDCHRP, PDDR.
	PMC(APBDEV_PMC_USB_AO) &= 0xFFFFFFF3; // UTMIP ID and VBUS wake up.
	usleep(1);
	USB(USB1_UTMIP_XCVR_CFG0) &= 0xFFFFBFFF; // UTMIP_FORCE_PD_POWERDOWN.
	usleep(1);
	USB(USB1_UTMIP_XCVR_CFG0) &= 0xFFFEFFFF; // UTMIP_FORCE_PD2_POWERDOWN.
	usleep(1);
	USB(USB1_UTMIP_XCVR_CFG0) &= 0xFFFBFFFF; // UTMIP_FORCE_PDZI_POWERDOWN.
	usleep(1);
	USB(USB1_UTMIP_XCVR_CFG1) &= 0xFFFFFFFB; // UTMIP_FORCE_PDCHRP_POWERDOWN.
	usleep(1);
	USB(USB1_UTMIP_XCVR_CFG1) &= 0xFFFFFFEF; // UTMIP_FORCE_PDDR_POWERDOWN.
	usleep(1);

	// AHB USB performance cfg.
	AHB_GIZMO(AHB_GIZMO_AHB_MEM) |= AHB_MEM_ENB_FAST_REARBITRATE;
	AHB_GIZMO(AHB_GIZMO_USB) |= AHB_GIZMO_USB_IMMEDIATE;
	AHB_GIZMO(AHB_ARBITRATION_PRIORITY_CTRL) |= ARBITRATION_PRIORITY_CTRL_ENB_FAST_REARBITRATE;
	AHB_GIZMO(AHB_AHB_MEM_PREFETCH_CFG1) =
		MEM_PREFETCH_ENABLE | (MEM_PREFETCH_AHB_MST_USB << 26) | (12 << 21) | 0x1000; // addr boundary 64KB

	// Set software and hardware context storage and clear it.
	usbdaemon = (usbd_t *)USBD_ADDR; // Depends on USB_TD_BUFFER_PAGE_SIZE aligned address.
	usbd_otg = &usbd_usb_otg_controller_ctxt;
	memset(usbd_otg,  0, sizeof(usbd_controller_t));
	memset(usbdaemon, 0, sizeof(usbd_t));

	usbd_otg->regs = (t210_usb2d_t *)USB_OTG_BASE;
	usbd_otg->usb_phy_ready = 0;

	// Initialize USB PHY on the USB_OTG Controller (#1) in Device mode.
	int result = _usbd_reset_usb_otg_phy_device_mode();
	usbd_otg->configuration_set = 0;

	_usb_charger_detect();

	if (!result)
		usb_init_done = true;

	return result;
}

static void _usb_device_power_down()
{
	// Enable PHY low power suspend.
	usbd_otg->regs->hostpc1_devlc |= USB2D_HOSTPC1_DEVLC_PHCD;
	// Do not use any controller regs after the above!
	// A reset or clear of the PHCD suspend bit must happen.

	// Power down OTG and Bias circuits.
	USB(USB1_UTMIP_BIAS_CFG0) |= (1 << 11) | (1 << 10); // UTMIP_OTGPD, UTMIP_BIASPD.

	// Power down ID detectors.
	USB(USB1_UTMIP_BIAS_CFG0) |= (1 << 23) | (1 << 22); //UTMIP_IDPD_SEL, UTMIP_IDPD_VAL.

	if (usbd_otg->charger_detect)
	{
		USB(USB1_UTMIP_BAT_CHRG_CFG0) = 1;  //UTMIP_PD_CHRG
		usbd_otg->charger_detect = 0;
	}

	// Power down the UTMIP transceivers.
	// UTMIP_FORCE_PDZI_POWERDOWN, UTMIP_FORCE_PD2_POWERDOWN, UTMIP_FORCE_PD_POWERDOWN.
	USB(USB1_UTMIP_XCVR_CFG0) |= (1 << 18) | (1 << 16) |(1 << 14);
	// UTMIP_FORCE_PDDR_POWERDOWN, UTMIP_FORCE_PDCHRP_POWERDOWN, UTMIP_FORCE_PDDISC_POWERDOWN.
	USB(USB1_UTMIP_XCVR_CFG1) |= (1 << 4) | (1 << 2) | (1 << 0);

	// Keep UTMIP in reset.
	USB(USB1_IF_USB_SUSP_CTRL) |= SUSP_CTRL_UTMIP_RESET;

	// Power down PD trunk.
	USB(USB1_UTMIP_BIAS_CFG1) |= (1 << 0); //UTMIP_FORCE_PDTRK_POWERDOWN.

	// Force UTMIP_PLL power down.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) |= (1 << 14);           // UTMIP_FORCE_PLL_ENABLE_POWERDOWN.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) |= (1 << 12);           // UTMIP_FORCE_PLL_ACTIVE_POWERDOWN.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) |= (1 << 4) | (1 << 0); // UTMIP_FORCE_PD_SAMP_A/C_POWERDOWN.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) |= (1 << 16);           // UTMIP_FORCE_PLLU_POWERDOWN.

	// Disable crystal clock.
	USB(USB1_UTMIP_MISC_CFG1) &= 0xBFFFFFFF;

	// Enable UTMIPLL IDDQ.
	CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) |= 2;

	// Set XUSB_PADCTL reset
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_SET) = (1 << 14);

	// Disable USBD clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = (1 << 22);

	// Completely disable PLLU.
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) &= ~0x2E00000;  // Disable PLLU USB/HSIC/ICUSB/48M.
	CLOCK(CLK_RST_CONTROLLER_PLLU_BASE) &= ~0x40000000; // Disable PLLU.
	CLOCK(CLK_RST_CONTROLLER_PLLU_MISC) &= ~0x20000000; // Enable reference clock.

	usb_init_done = false;
}

static void _usbd_stall_reset_ep1(usb_xfer_dir_t direction, usb_ep_cfg_t stall)
{
	stall &= 1;
	if (direction == USB_XFER_DIR_IN)
	{
		usbd_otg->regs->endptctrl[1] = (usbd_otg->regs->endptctrl[1] & ~USB2D_ENDPTCTRL_TX_EP_STALL) | ((u32)stall << 16);
		if (!stall)
			usbd_otg->regs->endptctrl[1] |= USB2D_ENDPTCTRL_TX_EP_RESET;
	}
	else
	{
		usbd_otg->regs->endptctrl[1] = (usbd_otg->regs->endptctrl[1] & ~USB2D_ENDPTCTRL_RX_EP_STALL) | stall;
		if (!stall)
			usbd_otg->regs->endptctrl[1] |= USB2D_ENDPTCTRL_RX_EP_RESET;
	}
}

void usbd_end(bool reset_ep, bool only_controller)
{
	if (reset_ep)
	{
		usbd_flush_endpoint(USB_EP_ALL);
		_usbd_stall_reset_ep1(0, USB_EP_CFG_RESET); // EP1 Bulk IN.
		_usbd_stall_reset_ep1(1, USB_EP_CFG_RESET); // EP1 Bulk OUT.
		//TODO: what about EP0 simultaneous in/out reset.

		usbd_otg->configuration = 0;
		usbd_otg->interface = 0;
		usbd_otg->configuration_set = 0;
		usbd_otg->max_lun_set = 0;
	}

	// Stop device controller.
	usbd_otg->regs->usbcmd &= ~USB2D_USBCMD_RUN;

	// Enable PHY auto low power suspend.
	usbd_otg->regs->hostpc1_devlc |= USB2D_HOSTPC1_DEVLC_ASUS;

	if (!only_controller)
		_usb_device_power_down();
}

void usb_device_stall_ep1_bulk_out()
{
	_usbd_stall_reset_ep1(USB_XFER_DIR_OUT, USB_EP_CFG_STALL);
}

void usb_device_stall_ep1_bulk_in()
{
	_usbd_stall_reset_ep1(USB_XFER_DIR_IN, USB_EP_CFG_STALL);
}

int usbd_get_max_pkt_length(int endpoint)
{
	switch (endpoint)
	{
	case USB_EP_CTRL_OUT:
	case USB_EP_CTRL_IN:
			return 64;
	case USB_EP_BULK_OUT:
	case USB_EP_BULK_IN:
		if (usbd_otg->port_speed == 2)
			return 512;
		else
			return 64;
	default:
		return 64;
	}
}

static void _usbd_initialize_ep_ctrl(u32 endpoint)
{
	usb_hw_ep_t actual_ep = (endpoint & 2) >> 1;
	usb_xfer_dir_t direction = endpoint & 1;

	memset((void *)&usbdaemon->qhs[endpoint], 0, sizeof(dQH_t));

	if (!endpoint)
		usbdaemon->qhs[endpoint].ep_capabilities = USB_QHD_EP_CAP_IOS_ENABLE;

	usbdaemon->qhs[endpoint].next_dTD_ptr = 1; // TERMINATE_SET

	u32 max_packet_len = usbd_get_max_pkt_length(endpoint) & USB_QHD_EP_CAP_MAX_PKT_LEN_MASK;
	usbdaemon->qhs[endpoint].ep_capabilities |= max_packet_len << 16;

	if (direction == USB_XFER_DIR_IN)
	{
		u32 endpoint_type = usbd_otg->regs->endptctrl[actual_ep] & ~USB2D_ENDPTCTRL_TX_EP_TYPE_MASK;
		if (actual_ep)
			endpoint_type |= usbd_otg->type ? USB2D_ENDPTCTRL_TX_EP_TYPE_INTR : USB2D_ENDPTCTRL_TX_EP_TYPE_BULK;
		else
			endpoint_type |= USB2D_ENDPTCTRL_TX_EP_TYPE_CTRL;

		usbd_otg->regs->endptctrl[actual_ep] = endpoint_type;

		usbd_otg->regs->endptctrl[actual_ep] &= ~USB2D_ENDPTCTRL_TX_EP_STALL;

		if (actual_ep == USB_HW_EP1)
			usbd_otg->regs->endptctrl[1] |= USB2D_ENDPTCTRL_TX_EP_RESET;

		usbd_otg->regs->endptctrl[actual_ep] |= USB2D_ENDPTCTRL_TX_EP_ENABLE;
	}
	else // EP Bulk OUT.
	{
		u32 endpoint_type = usbd_otg->regs->endptctrl[actual_ep] & ~USB2D_ENDPTCTRL_RX_EP_TYPE_MASK;
		if (actual_ep)
		{
			endpoint_type |= usbd_otg->type ? USB2D_ENDPTCTRL_RX_EP_TYPE_INTR : USB2D_ENDPTCTRL_RX_EP_TYPE_BULK;
		}
		else
			endpoint_type |= USB2D_ENDPTCTRL_RX_EP_TYPE_CTRL;

		usbd_otg->regs->endptctrl[actual_ep] = endpoint_type;
		usbd_otg->regs->endptctrl[actual_ep] &= ~USB2D_ENDPTCTRL_RX_EP_STALL;

		if (actual_ep == USB_HW_EP1)
			usbd_otg->regs->endptctrl[1] |= USB2D_ENDPTCTRL_RX_EP_RESET;

		usbd_otg->regs->endptctrl[actual_ep] |= USB2D_ENDPTCTRL_RX_EP_ENABLE;
	}
}

static int _usbd_initialize_ep0()
{
	memset((void *)usbdaemon->qhs, 0, sizeof(dQH_t) * 4);  // Clear all used EP queue heads.
	memset((void *)usbdaemon->dtds, 0, sizeof(dTD_t) * 4); // Clear all used EP0 token heads.

	usbd_otg->regs->asynclistaddr = (u32)usbdaemon->qhs;

	_usbd_initialize_ep_ctrl(USB_EP_CTRL_OUT);
	_usbd_initialize_ep_ctrl(USB_EP_CTRL_IN);

	// Disable Auto Low Power.
	usbd_otg->regs->hostpc1_devlc &= ~USB2D_HOSTPC1_DEVLC_ASUS;

	// Initiate an attach event.
	usbd_otg->regs->usbcmd |= USB2D_USBCMD_RUN;

	u32 retries = 100000; // 200ms timeout.
	while (!(usbd_otg->regs->usbcmd & USB2D_USBCMD_RUN))
	{
		retries--;
		if (!retries)
			return 3;
		usleep(1);
	}

	return 0;
}

// static void _disable_usb_wdt4()
// {
// 	if (TIMER_WDT4_STATUS & 1)// active
// 	{
// 		TIMER_TMR0_TMR_PTV &= 0x7FFFFFFF; // Disable timer
// 		TIMER_WDT4_UNLOCK_PATTERN = 0xC45A; // Alow writes to disable counter bit.
// 		TIMER_WDT4_COMMAND |= 2; // Disable counter
// 		TIMER_TMR0_TMR_PCR |= 0x40000000;// INTR_CLR
// 	}
// }

int usbd_flush_endpoint(u32 endpoint)
{

	usb_hw_ep_t actual_ep = (endpoint & 2) >> 1;
	usb_xfer_dir_t direction = endpoint & 1;
	u32 reg_mask = endpoint;

	// Flash all endpoints or 1.
	if (endpoint != USB_EP_ALL)
	{
		if (direction == USB_XFER_DIR_IN)
			reg_mask = USB2D_ENDPT_STATUS_TX_OFFSET << actual_ep;
		else
			reg_mask = USB2D_ENDPT_STATUS_RX_OFFSET << actual_ep;
	}
	usbd_otg->regs->endptflush = reg_mask;

	u32 retries = 100000; // 200ms timeout.
	while (usbd_otg->regs->endptflush & reg_mask)
	{
		retries--;
		if (!retries)
			return 3;
		usleep(1);
	}

	// Wait for the endpoint to finish all transactions (buffer not ready).
	retries = 100000; // 200ms timeout.
	while (usbd_otg->regs->endptstatus & reg_mask)
	{
		retries--;
		if (!retries)
			return 3;
		usleep(1);
	}

	// Wait for the endpoint to clear the primed status.
	retries = 100000; // 200ms timeout.
	while (usbd_otg->regs->endptprime & reg_mask)
	{
		retries--;
		if (!retries)
			return 3;
		usleep(1);
	}

	return 0;
}

static void _usbd_mark_ep_complete(u32 endpoint)
{
	u32 complete_bit;
	usb_hw_ep_t actual_ep = (endpoint & 2) >> 1;
	usb_xfer_dir_t direction = endpoint & 1;

	usbd_flush_endpoint(endpoint);
	memset((void *)&usbdaemon->dtds[endpoint * 4], 0, sizeof(dTD_t) * 4);
	memset((void *)&usbdaemon->qhs[endpoint], 0, sizeof(dQH_t));
	usbdaemon->ep_configured[endpoint] = 0;
	usbdaemon->ep_bytes_requested[endpoint] = 0;

	if (direction == USB_XFER_DIR_IN)
		complete_bit = USB2D_ENDPT_STATUS_TX_OFFSET << actual_ep;
	else
		complete_bit = USB2D_ENDPT_STATUS_RX_OFFSET << actual_ep;

	usbd_otg->regs->endptcomplete |= complete_bit;
}

static usb_ep_status_t _usbd_get_ep_status(usb_ep_t endpoint)
{
	bool status;
	u32 reg_val;
	u32 reg_mask;
	u32 actual_ep = (endpoint & 2) >> 1;
	usb_xfer_dir_t direction = endpoint & 1;

	if (direction == USB_XFER_DIR_IN)
		reg_mask = USB2D_ENDPT_STATUS_TX_OFFSET << actual_ep;
	else
		reg_mask = USB2D_ENDPT_STATUS_RX_OFFSET << actual_ep;

	if (actual_ep == USB_HW_EP1)
		reg_val = usbd_otg->regs->endptctrl[1];
	else
		reg_val = usbd_otg->regs->endptctrl[0];

	// Check stalled status.
	if (direction == USB_XFER_DIR_IN)
		status = reg_val & USB2D_ENDPTCTRL_TX_EP_STALL;
	else
		status = reg_val & USB2D_ENDPTCTRL_RX_EP_STALL;

	if (status)
		return USB_EP_STATUS_STALLED;

	// Check enabled status.
	if (direction == USB_XFER_DIR_IN)
		status = reg_val & USB2D_ENDPTCTRL_TX_EP_ENABLE;
	else
		status = reg_val & USB2D_ENDPTCTRL_RX_EP_ENABLE;

	if (!status)
		return USB_EP_STATUS_DISABLED;

	// CHeck qHD error status.
	u32 token_error_mask = USB_QHD_TOKEN_HALTED | USB_QHD_TOKEN_BUFFER_ERROR | USB_QHD_TOKEN_XFER_ERROR;
	if (usbdaemon->qhs[endpoint].token & token_error_mask)
		return USB_EP_STATUS_ERROR;

	// Check if endpoint has a request or a ready buffer.
	if ((usbd_otg->regs->endptprime & reg_mask) || (usbd_otg->regs->endptstatus & reg_mask))
		return USB_EP_STATUS_ACTIVE; // RX/TX active.

	// Return idle or not configured status.
	if (!usbdaemon->ep_configured[endpoint])
		return USB_EP_STATUS_NO_CONFIG;

	return USB_EP_STATUS_IDLE;
}

static int _usbd_ep_operation(usb_ep_t endpoint, u8 *buf, u32 len, bool sync)
{
	if (!buf)
		len = 0;

	u32 prime_bit;
	usb_hw_ep_t actual_ep = (endpoint & 2) >> 1;
	usb_xfer_dir_t direction = endpoint & 1;
	u32 length_left = len;
	u32 dtd_ep_idx = endpoint * 4;

	_usbd_mark_ep_complete(endpoint);

	if (endpoint == USB_EP_CTRL_OUT)
		usbdaemon->qhs[endpoint].ep_capabilities = USB_QHD_EP_CAP_IOS_ENABLE;

	u32 max_packet_len = usbd_get_max_pkt_length(endpoint) & USB_QHD_EP_CAP_MAX_PKT_LEN_MASK;
	usbdaemon->qhs[endpoint].ep_capabilities |= (max_packet_len << 16) | USB_QHD_EP_CAP_ZERO_LEN_TERM_DIS;
	usbdaemon->qhs[endpoint].next_dTD_ptr = 0; // Clear terminate bit.
	//usbdaemon->qhs[endpoint].ep_capabilities |= USB_QHD_TOKEN_IRQ_ON_COMPLETE;

	usbdaemon->ep_configured[endpoint] = 1;
	usbdaemon->ep_bytes_requested[endpoint] = len;

	// Configure dTD.
	u32 dtd_idx = 0;
	do
	{
		if (dtd_idx)
			usbdaemon->dtds[dtd_ep_idx + dtd_idx - 1].next_dTD = (u32)&usbdaemon->dtds[dtd_ep_idx + dtd_idx];

		u32 dtd_size = MIN(length_left, USB_TD_BUFFER_MAX_SIZE); // 16KB max per dTD.
		usbdaemon->dtds[dtd_ep_idx + dtd_idx].info = (dtd_size << 16) | USB_QHD_TOKEN_ACTIVE;
		// usbdaemon->dtds[dtd_ep_idx + dtd_idx].info |= USB_QHD_TOKEN_IRQ_ON_COMPLETE;

		// Set buffers addresses to all page pointers.
		u32 dt_buffer_offset = dtd_idx * USB_TD_BUFFER_MAX_SIZE;
		for (u32 i = 0; i < 4; i++)
			usbdaemon->dtds[dtd_ep_idx + dtd_idx].pages[i] =
				(u32)&buf[dt_buffer_offset + (USB_TD_BUFFER_PAGE_SIZE * i)];

		//usbdaemon->dtds[dtd_ep_idx + dtd_idx].pages[5] =
		//	(u32)&buf[dt_buffer_offset + (USB_TD_BUFFER_PAGE_SIZE * 4)]; // Last buffer. Unused.

		length_left -= dtd_size;
		if (length_left)
			dtd_idx++;
	}
	while (length_left);

	// Last dTD, terminate it.
	usbdaemon->dtds[dtd_ep_idx + dtd_idx].next_dTD = 1;

	// Set first dTD address to queue head next dTD.
	usbdaemon->qhs[endpoint].next_dTD_ptr |= (u32)&usbdaemon->dtds[dtd_ep_idx] & 0xFFFFFFE0;

	// Flush AHB prefetcher.
	AHB_GIZMO(AHB_AHB_MEM_PREFETCH_CFG1) &= ~MEM_PREFETCH_ENABLE;
	AHB_GIZMO(AHB_AHB_MEM_PREFETCH_CFG1) |=  MEM_PREFETCH_ENABLE;

	if (direction == USB_XFER_DIR_IN)
	{
		prime_bit = USB2D_ENDPT_STATUS_TX_OFFSET << actual_ep;
		bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);
	}
	else
		prime_bit = USB2D_ENDPT_STATUS_RX_OFFSET << actual_ep;

	// Prime endpoint.
	usbd_otg->regs->endptprime |= prime_bit; // USB2_CONTROLLER_USB2D_ENDPTPRIME.

	int res = 0;
	usb_ep_status_t ep_status;
	if (sync)
	{
		ep_status = _usbd_get_ep_status(endpoint);
		if (ep_status == USB_EP_STATUS_ACTIVE)
		{
			u32 retries = 1000000; // Timeout 2s.
			while (retries)
			{
				ep_status = _usbd_get_ep_status(endpoint);
				if (ep_status != USB_EP_STATUS_ACTIVE)
				{
					if (ep_status == USB_EP_STATUS_DISABLED)
						res = 28;
					goto out;
				}
				retries--;
				usleep(1);
			}
			res = 3;
		}
		else if (ep_status == USB_EP_STATUS_DISABLED)
			res = 28;
out:
		if (res)
			_usbd_mark_ep_complete(endpoint);
		else if (_usbd_get_ep_status(endpoint) != USB_EP_STATUS_IDLE)
			res = 26;
	}

	if (direction == USB_XFER_DIR_OUT)
		bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	return res;
}

static int _usbd_ep_ack(usb_ep_t ep)
{
	return _usbd_ep_operation(ep, NULL, 0, true);
}

static void _usbd_set_ep0_stall()
{
	// EP Control endpoints must be always stalled together.
	usbd_otg->regs->endptctrl[0] =
			USB2D_ENDPTCTRL_TX_EP_ENABLE | USB2D_ENDPTCTRL_TX_EP_STALL |
			USB2D_ENDPTCTRL_RX_EP_ENABLE | USB2D_ENDPTCTRL_RX_EP_STALL;
}

void usbd_set_ep_stall(u32 endpoint, int ep_stall)
{
	usb_hw_ep_t actual_ep = (endpoint & 2) >> 1;
	usb_xfer_dir_t direction = endpoint & 1;

	if (ep_stall)
	{
		if (direction == USB_XFER_DIR_IN)
			usbd_otg->regs->endptctrl[actual_ep] |= USB2D_ENDPTCTRL_TX_EP_STALL; // Stall EP Bulk IN.
		else
			usbd_otg->regs->endptctrl[actual_ep] |= USB2D_ENDPTCTRL_RX_EP_STALL; // Stall EP Bulk OUT.
	}
	else
	{
		if (direction == USB_XFER_DIR_IN)
			usbd_otg->regs->endptctrl[actual_ep] &= ~USB2D_ENDPTCTRL_TX_EP_STALL; // Clear stall EP Bulk IN.
		else
			usbd_otg->regs->endptctrl[actual_ep] &= ~USB2D_ENDPTCTRL_RX_EP_STALL; // Clear stall EP Bulk OUT.
	}
}

static void _usbd_handle_get_class_request(bool *transmit_data, u8 *descriptor, int *size, int *ep_stall)
{
	u8 _bRequest = usbd_otg->control_setup.bRequest;
	u16 _wIndex  = usbd_otg->control_setup.wIndex;
	u16 _wValue  = usbd_otg->control_setup.wValue;
	u16 _wLength = usbd_otg->control_setup.wLength;

	bool valid_interface = _wIndex == usbd_otg->interface;
	bool valid_len = _bRequest == USB_REQUEST_BULK_GET_MAX_LUN ? 1 : 0;

	if (!valid_interface || _wValue != 0 || _wLength != valid_len)
	{
		*ep_stall = 1;
		return;
	}

	switch (_bRequest)
	{
	case USB_REQUEST_BULK_RESET:
		_usbd_ep_ack(USB_EP_CTRL_IN);
		usbd_otg->bulk_reset_req = 1;
		break; // DELAYED_STATUS;
	case USB_REQUEST_BULK_GET_MAX_LUN:
		*transmit_data = 1;
		descriptor[0] = usbd_otg->max_lun; // Set 0 LUN for 1 drive supported.
		usbd_otg->max_lun_set = 1;
		break;
	default:
		*ep_stall = 1;
		break;
	}
}

static void _usbd_handle_get_descriptor(bool *transmit_data, void **descriptor, int *size, int *ep_stall)
{
	u8 descriptor_type = usbd_otg->control_setup.wValue >> 8;
	u8 descriptor_subtype = usbd_otg->control_setup.wValue & 0xFF;

	switch (descriptor_type)
	{
	case USB_DESCRIPTOR_DEVICE:
		{
/*
		u32 soc_rev = APB_MISC(APB_MISC_GP_HIDREV);
		usb_device_descriptor.idProduct  = (soc_rev >> 8)  & 0xFF; // chip_id.
		usb_device_descriptor.idProduct |= ((soc_rev << 4) | (FUSE(FUSE_SKU_INFO) & 0xF)) << 8; // HIDFAM.
		usb_device_descriptor.bcdDevice  = (soc_rev >> 16) & 0xF; // MINORREV.
		usb_device_descriptor.bcdDevice |= ((soc_rev >> 4) & 0xF) << 8; // MAJORREV.
*/
		*descriptor = usbd_otg->desc->dev;
		*size = usbd_otg->desc->dev->bLength;
		*transmit_data = 1;
		return;
		}
	case USB_DESCRIPTOR_CONFIGURATION:
		if (usbd_otg->type == USB_GADGET_UMS)
		{
			if (usbd_otg->port_speed == 2) // High speed. 512 bytes.
			{
				usbd_otg->desc->cfg->endpoint[0].wMaxPacketSize = 0x200;
				usbd_otg->desc->cfg->endpoint[1].wMaxPacketSize = 0x200;
			}
			else // Full speed. 64 bytes.
			{
				usbd_otg->desc->cfg->endpoint[0].wMaxPacketSize = 0x40;
				usbd_otg->desc->cfg->endpoint[1].wMaxPacketSize = 0x40;
			}
		}
		else
		{
			usb_cfg_hid_descr_t *tmp = (usb_cfg_hid_descr_t *)usbd_otg->desc->cfg;
			if (usbd_otg->port_speed == 2) // High speed. 512 bytes.
			{
				tmp->endpoint[0].wMaxPacketSize = 0x200;
				tmp->endpoint[1].wMaxPacketSize = 0x200;
			}
			else // Full speed. 64 bytes.
			{
				tmp->endpoint[0].wMaxPacketSize = 0x40;
				tmp->endpoint[1].wMaxPacketSize = 0x40;
			}
		}
		*descriptor = usbd_otg->desc->cfg;
		*size = usbd_otg->desc->cfg->config.wTotalLength;
		*transmit_data = 1;
		return;
	case USB_DESCRIPTOR_STRING:
		switch (descriptor_subtype)
		{
		case 1:
			*descriptor = usbd_otg->desc->vendor;
			*size = usbd_otg->desc->vendor[0];
			break;
		case 2:
			*descriptor = usbd_otg->desc->product;
			*size = usbd_otg->desc->product[0];
			break;
		case 3:
			*descriptor = usb_serial_string_descriptor;
			*size = usb_serial_string_descriptor[0];
			break;
		case 0xEE:
			*descriptor = usbd_otg->desc->ms_os;
			*size = usbd_otg->desc->ms_os->bLength;
			break;
		default:
			*descriptor = usb_lang_id_string_descriptor;
			*size = 4;
			break;
		}
		*transmit_data = 1;
		return;
	case USB_DESCRIPTOR_DEVICE_QUALIFIER:
		if (!usbd_otg->desc->dev_qual)
			goto exit;
		*descriptor = usbd_otg->desc->dev_qual;
		*size = usbd_otg->desc->dev_qual->bLength;
		*transmit_data = 1;
		return;
	case USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION:
		if (!usbd_otg->desc->cfg_other)
			goto exit;
		if (usbd_otg->port_speed == 2)
		{
			usbd_otg->desc->cfg_other->endpoint[0].wMaxPacketSize = 0x40;
			usbd_otg->desc->cfg_other->endpoint[1].wMaxPacketSize = 0x40;
		}
		else
		{
			usbd_otg->desc->cfg_other->endpoint[0].wMaxPacketSize = 0x200;
			usbd_otg->desc->cfg_other->endpoint[1].wMaxPacketSize = 0x200;
		}
		if ((usbd_otg->charger_detect & 1) && (usbd_otg->charger_detect & 2))
			usbd_otg->desc->cfg_other->config.bMaxPower = 500 / 2;
		*descriptor = usbd_otg->desc->cfg_other;
		*size = usbd_otg->desc->cfg_other->config.wTotalLength;
		*transmit_data = 1;
		return;
	case USB_DESCRIPTOR_DEVICE_BINARY_OBJECT:
		*descriptor = usbd_otg->desc->dev_bot;
		*size = usbd_otg->desc->dev_bot->wTotalLength;
		*transmit_data = 1;
		return;
	default:
		*transmit_data = 0;
		*ep_stall = 1;
		return;
	}
exit:
	*transmit_data = 0;
	*ep_stall = 1;
	return;
}

static int _usbd_handle_set_request(int *ep_stall)
{
	int ret = 0;
	u8 bRequest = usbd_otg->control_setup.bRequest;
	if (bRequest == USB_REQUEST_SET_ADDRESS)
	{
		ret = _usbd_ep_ack(USB_EP_CTRL_IN);

		// Set USB address for device mode.
		if (!ret)
			usbd_otg->regs->periodiclistbase = (usbd_otg->regs->periodiclistbase & 0x1FFFFFF) | ((usbd_otg->control_setup.wValue & 0xFF) << 25);
	}
	else if (bRequest == USB_REQUEST_SET_CONFIGURATION)
	{
		ret = _usbd_ep_ack(USB_EP_CTRL_IN);
		if (!ret)
		{
			usbd_otg->configuration = usbd_otg->control_setup.wValue;
			_usbd_initialize_ep_ctrl(USB_EP_BULK_OUT);
			_usbd_initialize_ep_ctrl(USB_EP_BULK_IN);
			usbd_otg->configuration_set = 1;
		}
	}
	else
		*ep_stall = 1;

	return ret;
}

static int _usbd_handle_ep0_control_transfer()
{
	int direction;

	int ret = 0;
	bool transmit_data = 0;
	u8 *descriptor = (u8 *)USB_DESCRIPTOR_ADDR;
	int size = 0;
	int ep_stall = 0;

	u8  _bmRequestType = usbd_otg->control_setup.bmRequestType;
	u8  _bRequest      = usbd_otg->control_setup.bRequest;
	u16 _wValue        = usbd_otg->control_setup.wValue;
	u16 _wIndex        = usbd_otg->control_setup.wIndex;
	u16 _wLength       = usbd_otg->control_setup.wLength;

	//gfx_printf("%02X %02X %04X %04X %04X\n", _bmRequestType, _bRequest, _wValue, _wIndex, _wLength);

	switch (_bmRequestType)
	{
	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_DEVICE    | USB_SETUP_TYPE_STANDARD):
		ret = _usbd_handle_set_request(&ep_stall);
		break;

	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_STANDARD):
		ret = _usbd_ep_ack(USB_EP_CTRL_IN);
		if (!ret)
			usbd_otg->interface = _wValue;
		break;

	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_ENDPOINT  | USB_SETUP_TYPE_STANDARD):
		switch (_bRequest)
		{
		case USB_REQUEST_CLEAR_FEATURE:
		case USB_REQUEST_SET_FEATURE:
			if ((_wValue & 0xFF) == USB_FEATURE_ENDPOINT_HALT)
			{
				switch (_wIndex) // endpoint
				{
				case USB_EP_ADDR_CTRL_OUT:
					direction = 2;
					break;
				case USB_EP_ADDR_CTRL_IN:
					direction = 3;
					break;
				case USB_EP_ADDR_BULK_OUT:
					direction = 0;
					break;
				case USB_EP_ADDR_BULK_IN:
					direction = 1;
					break;
				default:
					_usbd_stall_reset_ep1(3, USB_EP_CFG_STALL);
					goto out;
				}

				if (_bRequest == USB_REQUEST_CLEAR_FEATURE)
					_usbd_stall_reset_ep1(direction, USB_EP_CFG_RESET);
				else
					_usbd_stall_reset_ep1(direction, USB_EP_CFG_STALL);

				ret = _usbd_ep_ack(USB_EP_CTRL_IN);
			}
			else
				_usbd_stall_reset_ep1(3, USB_EP_CFG_STALL);

			break;
		default:
			ep_stall = 1;
			break;
		}
		break;
	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS):
		_usbd_handle_get_class_request(&transmit_data, descriptor, &size, &ep_stall);
		break;
	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE    | USB_SETUP_TYPE_STANDARD):
		switch (_bRequest)
		{
		case USB_REQUEST_GET_STATUS:
			descriptor[0] = USB_STATUS_DEV_SELF_POWERED;
			descriptor[1] = 0; // No support for remove wake up.
			transmit_data = 1;
			size = 2;
			break;
		case USB_REQUEST_GET_DESCRIPTOR:
			_usbd_handle_get_descriptor(&transmit_data, (void **)&descriptor, &size, &ep_stall);
			break;
		case USB_REQUEST_GET_CONFIGURATION:
			descriptor = (u8 *)&usbd_otg->configuration;
			size = _wLength;
			transmit_data = 1;
			break;
		default:
			ep_stall = 1;
			break;
		}
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_STANDARD):
		if (_bRequest == USB_REQUEST_GET_INTERFACE)
		{
			descriptor = (void *)&usbd_otg->interface;
		}
		else if (_bRequest == USB_REQUEST_GET_STATUS)
		{
			memset(descriptor, 0, _wLength);
		}
		else if (_bRequest == USB_REQUEST_GET_DESCRIPTOR && (_wValue >> 8) == USB_DESCRIPTOR_HID_REPORT && usbd_otg->type > USB_GADGET_UMS)
		{
			if (usbd_otg->type == USB_GADGET_HID_GAMEPAD)
			{
				descriptor = (u8 *)&hid_report_descriptor_jc;
				_wLength = sizeof(hid_report_descriptor_jc);
			}
			else // USB_GADGET_HID_TOUCHPAD
			{
				descriptor = (u8 *)&hid_report_descriptor_touch;
				_wLength = sizeof(hid_report_descriptor_touch);
			}

			usbd_otg->hid_report_sent = 1;
		}
		else
		{
			ep_stall = 1;
			break;
		}

		size = _wLength;
		transmit_data = 1;
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_ENDPOINT  | USB_SETUP_TYPE_STANDARD):
		if (_bRequest == USB_REQUEST_GET_STATUS)
		{
			int ep_req;
			switch (_wIndex)
			{
			case 0:
				ep_req = 0;
				break;
			case 1:
				ep_req = 2;
				break;
			case 0x80:
				ep_req = 1;
				break;
			case 0x81:
				ep_req = 3;
				break;
			default:
				_usbd_stall_reset_ep1(3, USB_EP_CFG_STALL);
				goto out;
			}

			size = _wLength;
			memset(descriptor, 0, size);

			if (_usbd_get_ep_status(ep_req) == USB_EP_STATUS_STALLED)
				descriptor[0] = USB_STATUS_EP_HALTED;
			else
				descriptor[0] = USB_STATUS_EP_OK;

			transmit_data = 1;
		}
		else
			_usbd_stall_reset_ep1(3, USB_EP_CFG_STALL);
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS):
		memset(descriptor, 0, _wLength);

		_usbd_handle_get_class_request(&transmit_data, descriptor, &size, &ep_stall);
		size = _wLength;
		break;
	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_VENDOR):
	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_DEVICE    | USB_SETUP_TYPE_VENDOR):
		if (_bRequest == USB_REQUEST_GET_MS_DESCRIPTOR)
		{
			switch (_wIndex)
			{
			case USB_DESCRIPTOR_MS_COMPAT_ID:
				descriptor = (u8 *)usbd_otg->desc->ms_cid;
				size = usbd_otg->desc->ms_cid->dLength;
				transmit_data = 1;
				break;
			case USB_DESCRIPTOR_MS_EXTENDED_PROPERTIES:
				descriptor = (u8 *)usbd_otg->desc->mx_ext;
				size = usbd_otg->desc->mx_ext->dLength;
				transmit_data = 1;
				break;
			default:
				ep_stall = 1;
				break;
			}
		}
		else
			ep_stall = 1;
		break;

	default:
		ep_stall = 1;
		break;
	}

	// Transmit data to HOST if any.
	if (transmit_data)
	{
		memcpy(usb_ep0_ctrl_buf, descriptor, size);

		if (_wLength < size)
			size = _wLength;
		ret = _usbd_ep_operation(USB_EP_CTRL_IN, usb_ep0_ctrl_buf, size, true);
		if (!ret)
			ret = _usbd_ep_ack(USB_EP_CTRL_OUT);
	}

out:
	if (ep_stall)
		_usbd_set_ep0_stall();

	return ret;
}

static int _usbd_ep0_initialize()
{
	bool enter = false;
	if (usbd_otg->configuration_set)
		enter = true;
	else
	{
		usbdaemon->qhs = (volatile dQH_t *)USB2_QH_USB2D_QH_EP_BASE;

		if (!_usbd_initialize_ep0())
			enter = true;
	}

	if (enter)
	{
		usbd_otg->configuration_set = 0;
		usbd_otg->max_lun_set = 0;

		// Timeout if cable or communication isn't started in 1.5 minutes.
		u32 timer = get_tmr_ms() + 90000;
		while (true)
		{
			u32 usb_status_irqs = usbd_otg->regs->usbsts;

			// Clear all interrupt statuses.
			usbd_otg->regs->usbsts = usb_status_irqs;

			// Check if a reset was received.
			if (usb_status_irqs & USB2D_USBSTS_URI)
			{
				//_disable_usb_wdt4();

				// Clear all device addresses, enabled setup requests, transmit events and flush all endpoints.
				usbd_otg->regs->periodiclistbase = 0;
				usbd_otg->regs->endptsetupstat = usbd_otg->regs->endptsetupstat;
				usbd_otg->regs->endptcomplete = usbd_otg->regs->endptcomplete;
				usbd_flush_endpoint(USB_EP_ALL);
			}

			// Check if port change happened.
			if (usb_status_irqs & USB2D_USBSTS_PCI)
				usbd_otg->port_speed = (usbd_otg->regs->hostpc1_devlc & USB2D_HOSTPC1_DEVLC_PSPD_MASK) >> 25;

			// Acknowledge setup request for EP0 and copy its configuration.
			u32 ep0_setup_req = usbd_otg->regs->endptsetupstat;
			if (ep0_setup_req & 1)
			{
				usbd_otg->regs->endptsetupstat = ep0_setup_req;
				memcpy(&usbd_otg->control_setup, (void *)usbdaemon->qhs->setup, 8);
				if (_usbd_handle_ep0_control_transfer())
					break;
			}
			if (usbd_otg->configuration_set)
				return 0;

			if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
				return 2;
		}
	}

	return 3;
}

int usb_device_ep0_initialize(usb_gadget_type type)
{
	switch (type)
	{
	case USB_GADGET_UMS:
		usbd_otg->desc = &usb_gadget_ums_descriptors;
		break;
	case USB_GADGET_HID_GAMEPAD:
		usbd_otg->desc = &usb_gadget_hid_jc_descriptors;
		break;
	case USB_GADGET_HID_TOUCHPAD:
		usbd_otg->desc = &usb_gadget_hid_touch_descriptors;
		break;
	}

	usbd_otg->type = type;

	int result = _usbd_ep0_initialize();
	if (result)
		result = 8;
	return result;
}

int usbd_handle_ep0_pending_control_transfer()
{
	// Acknowledge setup request for EP0 and copy its configuration.
	u32 ep0_setup_req = usbd_otg->regs->endptsetupstat;
	if (ep0_setup_req & 1)
	{
		usbd_otg->regs->endptsetupstat = ep0_setup_req;
		memcpy(&usbd_otg->control_setup, (void *)usbdaemon->qhs->setup, 8);
		_usbd_handle_ep0_control_transfer();
		memset(usb_ep0_ctrl_buf, 0, USB_TD_BUFFER_PAGE_SIZE);
	}

	if (usbd_otg->bulk_reset_req)
	{
		usbd_otg->bulk_reset_req = 0;
		return 1;
	}

	return 0;
}

static usb_ep_status_t _usbd_get_ep1_status(usb_xfer_dir_t dir)
{
	usb_ep_t ep;
	if (dir == USB_XFER_DIR_OUT)
		ep = USB_EP_BULK_OUT;
	else
		ep = USB_EP_BULK_IN;
	return _usbd_get_ep_status(ep);
}

int usb_device_read_ep1_out(u8 *buf, u32 len, u32 *bytes_read, bool sync)
{
	if (len > USB_EP_BUFFER_MAX_SIZE)
		len = USB_EP_BUFFER_MAX_SIZE;

	int result = _usbd_ep_operation(USB_EP_BULK_OUT, buf, len, sync);

	if (sync && bytes_read)
	{
		if (result)
			*bytes_read = 0;
		else
			*bytes_read = len;
	}

	return result;
}

int usb_device_read_ep1_out_big_reads(u8 *buf, u32 len, u32 *bytes_read)
{
	if (len > USB_EP_BULK_OUT_MAX_XFER)
		len = USB_EP_BULK_OUT_MAX_XFER;

	int result;
	u32 bytes = 0;
	*bytes_read = 0;
	u8 *buf_curr = buf;

	while (len)
	{
		u32 len_ep = MIN(len, USB_EP_BUFFER_MAX_SIZE);

		result = usb_device_read_ep1_out(buf_curr, len_ep, &bytes, true);
		if (!result)
		{
			len -= len_ep;
			buf_curr += len_ep;
			*bytes_read = *bytes_read + bytes;
		}
		else
			break;
	}

	return result;
}

static int _usbd_get_ep1_out_bytes_read()
{
	if (_usbd_get_ep_status(2) != USB_EP_STATUS_IDLE)
		return 0;
	else
		return (usbdaemon->ep_bytes_requested[2] - (usbdaemon->qhs[2].token >> 16));
}

int usb_device_ep1_out_reading_finish(u32 *pending_bytes)
{
	usb_ep_status_t ep_status;
	do
	{
		ep_status = _usbd_get_ep1_status(USB_XFER_DIR_OUT);
		if ((ep_status == USB_EP_STATUS_IDLE) || (ep_status == USB_EP_STATUS_DISABLED))
			break;

		usbd_handle_ep0_pending_control_transfer();
	}
	while ((ep_status == USB_EP_STATUS_ACTIVE) || (ep_status == USB_EP_STATUS_STALLED));

	*pending_bytes = _usbd_get_ep1_out_bytes_read();

	if (ep_status == USB_EP_STATUS_IDLE)
		return 0;
	else if (ep_status == USB_EP_STATUS_DISABLED)
		return 28;
	else
		return 26;
}

int usb_device_write_ep1_in(u8 *buf, u32 len, u32 *bytes_written, bool sync)
{
	if (len > USB_EP_BUFFER_MAX_SIZE)
		len = USB_EP_BUFFER_MAX_SIZE;

	int result = _usbd_ep_operation(USB_EP_BULK_IN, buf, len, sync);

	if (sync && bytes_written)
	{
		if (result)
			*bytes_written = 0;
		else
			*bytes_written = len;
	}

	return result;
}

static int _usbd_get_ep1_in_bytes_written()
{
	if (_usbd_get_ep_status(3) != USB_EP_STATUS_IDLE)
		return 0;
	else
		return (usbdaemon->ep_bytes_requested[3] - (usbdaemon->qhs[3].token >> 16));
}

int usb_device_ep1_in_writing_finish(u32 *pending_bytes)
{
	usb_ep_status_t ep_status;
	do
	{
		ep_status = _usbd_get_ep1_status(USB_XFER_DIR_IN);
		if ((ep_status == USB_EP_STATUS_IDLE) || (ep_status == USB_EP_STATUS_DISABLED))
			break;

		usbd_handle_ep0_pending_control_transfer();
	}
	while ((ep_status == USB_EP_STATUS_ACTIVE) || (ep_status == USB_EP_STATUS_STALLED));

	*pending_bytes = _usbd_get_ep1_in_bytes_written();

	if (ep_status == USB_EP_STATUS_IDLE)
		return 0;
	else if (ep_status == USB_EP_STATUS_DISABLED)
		return 28;

	usb_device_stall_ep1_bulk_out();
	return 26;
}

bool usb_device_get_suspended()
{
	u32 suspended = usbd_otg->regs->portsc1 & USB2D_PORTSC1_SUSP;
	return (suspended ? true : false);
}

u32 usb_device_get_port_status()
{
	return (usbd_otg->regs->portsc1);
}

bool usb_device_get_max_lun(u8 max_lun)
{
	// Timeout if get MAX_LUN request doesn't happen in 10s.
	u32 timer = get_tmr_ms() + 10000;

	usbd_otg->max_lun = max_lun;

	while (!usbd_otg->max_lun_set)
	{
		usbd_handle_ep0_pending_control_transfer();
		if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			return true;
	}

	return false;
}

bool usb_device_get_hid_report()
{
	// Timeout if get GET_HID_REPORT request doesn't happen in 10s.
	u32 timer = get_tmr_ms() + 10000;

	while (!usbd_otg->hid_report_sent)
	{
		usbd_handle_ep0_pending_control_transfer();
		if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			return true;
	}

	return false;
}
