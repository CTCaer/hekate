/*
 * Enhanced USB Device (EDCI) driver for Tegra X1
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

#include <string.h>
#include <stdlib.h>

#include <usb/usbd.h>
#include <usb/usb_descriptor_types.h>
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
	USB_EP_STATUS_IDLE      = 0,
	USB_EP_STATUS_ACTIVE    = 1,
	USB_EP_STATUS_ERROR     = 2,
	USB_EP_STATUS_NO_CONFIG = 3,
	USB_EP_STATUS_STALLED   = 4,
	USB_EP_STATUS_DISABLED  = 5
} usb_ep_status_t;

typedef enum {
	USB_LOW_SPEED   = 0,
	USB_FULL_SPEED  = 1,
	USB_HIGH_SPEED  = 2,
	USB_SUPER_SPEED = 3,
} usb_speed_t;

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

typedef struct _usbd_controller_t
{
	u32 port_speed;
	t210_usb2d_t *regs;
	usb_ctrl_setup_t control_setup;
	usb_desc_t *desc;
	usb_gadget_type gadget;
	u8 config_num;
	u8 interface_num;
	u8 max_lun;
	bool usb_phy_ready;
	bool configuration_set;
	bool max_lun_set;
	bool bulk_reset_req;
	bool hid_report_sent;
	u32 charger_detect;
} usbd_controller_t;

extern u8  hid_report_descriptor_jc[];
extern u8  hid_report_descriptor_touch[];
extern u32 hid_report_descriptor_jc_size;
extern u32 hid_report_descriptor_touch_size;

extern usb_desc_t usb_gadget_hid_jc_descriptors;
extern usb_desc_t usb_gadget_hid_touch_descriptors;
extern usb_desc_t usb_gadget_ums_descriptors;

usbd_t *usbdaemon;

usbd_controller_t *usbd_otg;
usbd_controller_t usbd_usb_otg_controller_ctxt;

bool usb_init_done = false;

u8 *usb_ep0_ctrl_buf = (u8 *)USB_EP_CONTROL_BUF_ADDR;

static int _usbd_reset_usb_otg_phy_device_mode()
{
	usbd_otg->usb_phy_ready = false;

	// Clear UTMIP reset.
	USB(USB1_IF_USB_SUSP_CTRL) &= ~SUSP_CTRL_UTMIP_RESET;

	// Wait for PHY clock to get validated.
	u32 retries = 100000; // 200ms timeout.
	while (!(USB(USB1_IF_USB_SUSP_CTRL) & SUSP_CTRL_USB_PHY_CLK_VALID))
	{
		retries--;
		if (!retries)
			return USB_ERROR_INIT;
		usleep(1);
	}
	usbd_otg->usb_phy_ready = true;

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
			return USB_ERROR_INIT;
		usleep(1);
	}

	// Wait for PHY clock to get validated after reset.
	retries = 100000; // 200ms timeout.
	while (!(USB(USB1_IF_USB_SUSP_CTRL) & SUSP_CTRL_USB_PHY_CLK_VALID))
	{
		retries--;
		if (!retries)
			return USB_ERROR_INIT;
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
			return USB_ERROR_INIT;
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

	return USB_RES_OK;
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
		PINMUX_AUX(PINMUX_AUX_USB_VBUS_EN1) &=
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

static void _usb_init_phy()
{
	// Configure and enable PLLU.
	clock_enable_pllu();

	// Enable USBD clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = BIT(CLK_L_USBD);
	usleep(2);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_USBD);
	usleep(2);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = BIT(CLK_L_USBD);
	usleep(2);

	// Clear XUSB_PADCTL reset
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_CLR) = BIT(CLK_W_XUSB_PADCTL);

	// Enable USB PHY and reset for programming.
	u32 usb_susp_ctrl = USB(USB1_IF_USB_SUSP_CTRL);
	USB(USB1_IF_USB_SUSP_CTRL) = usb_susp_ctrl | SUSP_CTRL_UTMIP_RESET;
	USB(USB1_IF_USB_SUSP_CTRL) = usb_susp_ctrl | SUSP_CTRL_UTMIP_PHY_ENB | SUSP_CTRL_UTMIP_RESET;

	// Enable IDDQ control by software and disable UTMIPLL IDDQ.
	CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) = (CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) & 0xFFFFFFFC) | 1;
	usleep(10);

	// Disable crystal clock.
	USB(USB1_UTMIP_MISC_CFG1) &= 0xBFFFFFFF;
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) &= 0xBFFFFFFF;

	// Set B_SESS_VLD.
	USB(USB1_IF_USB_PHY_VBUS_SENSORS) |= 0x1000;
	USB(USB1_IF_USB_PHY_VBUS_SENSORS) |= 0x800;

	// Set UTMIPLL dividers and config based on OSC and enable it to 960 MHz.
	clock_enable_utmipll();

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

	// Enable USB2 tracking clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_Y_SET) = BIT(CLK_Y_USB2_TRK);
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

	// Disable USB2 tracking clock and configure UTMIP misc.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_Y_CLR) = BIT(CLK_Y_USB2_TRK);
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) = (CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) & 0xFEFFFFEA) | 0x2000000 | 0x28 | 2;
	usleep(1);

	USB(USB1_UTMIP_BIAS_CFG0) &= 0xFF3FF7FF;
	usleep(1);

	// Clear power downs on UTMIP ID and VBUS wake up, PD, PD2, PDZI, PDCHRP, PDDR.
	PMC(APBDEV_PMC_USB_AO) &= 0xFFFFFFF3;    // UTMIP ID and VBUS wake up.
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
}

int usb_device_init()
{
	if (usb_init_done)
		return USB_RES_OK;

	// Initialize USB2 controller PHY.
	_usb_init_phy();

	// AHB USB performance cfg.
	AHB_GIZMO(AHB_GIZMO_AHB_MEM) |= AHB_MEM_DONT_SPLIT_AHB_WR | AHB_MEM_ENB_FAST_REARBITRATE;
	AHB_GIZMO(AHB_GIZMO_USB) |= AHB_GIZMO_IMMEDIATE;
	AHB_GIZMO(AHB_ARBITRATION_PRIORITY_CTRL) = PRIORITY_CTRL_WEIGHT(7) | PRIORITY_SELECT_USB;
	AHB_GIZMO(AHB_AHB_MEM_PREFETCH_CFG1) =
		MEM_PREFETCH_ENABLE | MEM_PREFETCH_USB_MST_ID | MEM_PREFETCH_ADDR_BNDRY(12) | 0x1000; // Addr boundary 64KB, Inactivity 4096 cycles.

	// Set software and hardware context storage and clear it.
	usbdaemon = (usbd_t *)USBD_ADDR; // Depends on USB_TD_BUFFER_PAGE_SIZE aligned address.
	usbd_otg = &usbd_usb_otg_controller_ctxt;
	memset(usbd_otg,  0, sizeof(usbd_controller_t));
	memset(usbdaemon, 0, sizeof(usbd_t));

	usbd_otg->regs = (t210_usb2d_t *)USB_OTG_BASE;
	usbd_otg->usb_phy_ready = false;

	// Initialize USB PHY on the USB_OTG Controller (#1) in Device mode.
	int res = _usbd_reset_usb_otg_phy_device_mode();
	usbd_otg->configuration_set = false;

	_usb_charger_detect();

	if (!res)
		usb_init_done = true;

	return res;
}

static void _usb_device_power_down()
{
	// Enable PHY low power suspend.
	usbd_otg->regs->hostpc1_devlc |= USB2D_HOSTPC1_DEVLC_PHCD;
	// Do not use any controller regs after the above!
	// A reset or clear of the PHCD suspend bit must happen.

	// Power down OTG and Bias circuits.
	USB(USB1_UTMIP_BIAS_CFG0) |= BIT(11) | BIT(10); // UTMIP_OTGPD, UTMIP_BIASPD.

	// Power down ID detectors.
	USB(USB1_UTMIP_BIAS_CFG0) |= BIT(23) | BIT(22); // UTMIP_IDPD_SEL, UTMIP_IDPD_VAL.

	if (usbd_otg->charger_detect)
	{
		USB(USB1_UTMIP_BAT_CHRG_CFG0) = 1;  //UTMIP_PD_CHRG
		usbd_otg->charger_detect = 0;
	}

	// Power down the UTMIP transceivers.
	// UTMIP_FORCE_PDZI_POWERDOWN, UTMIP_FORCE_PD2_POWERDOWN, UTMIP_FORCE_PD_POWERDOWN.
	USB(USB1_UTMIP_XCVR_CFG0) |= BIT(18) | BIT(16) |BIT(14);
	// UTMIP_FORCE_PDDR_POWERDOWN, UTMIP_FORCE_PDCHRP_POWERDOWN, UTMIP_FORCE_PDDISC_POWERDOWN.
	USB(USB1_UTMIP_XCVR_CFG1) |= BIT(4) | BIT(2) | BIT(0);

	// Keep UTMIP in reset.
	USB(USB1_IF_USB_SUSP_CTRL) |= SUSP_CTRL_UTMIP_RESET;

	// Power down PD trunk.
	USB(USB1_UTMIP_BIAS_CFG1) |= BIT(0); //UTMIP_FORCE_PDTRK_POWERDOWN.

	// Force UTMIP_PLL power down.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) |= BIT(14);           // UTMIP_FORCE_PLL_ENABLE_POWERDOWN.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) |= BIT(12);           // UTMIP_FORCE_PLL_ACTIVE_POWERDOWN.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG2) |= BIT(4) | BIT(0); // UTMIP_FORCE_PD_SAMP_A/C_POWERDOWN.
	CLOCK(CLK_RST_CONTROLLER_UTMIP_PLL_CFG1) |= BIT(16);           // UTMIP_FORCE_PLLU_POWERDOWN.

	// Disable crystal clock.
	USB(USB1_UTMIP_MISC_CFG1) &= 0xBFFFFFFF;

	// Force enable UTMIPLL IDDQ.
	CLOCK(CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0) |= 3;

	// Set XUSB_PADCTL reset
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_W_SET) = BIT(CLK_W_XUSB_PADCTL);

	// Disable USBD clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = BIT(CLK_L_USBD);

	// Disable PLLU.
	clock_disable_pllu();

	usb_init_done = false;
}

static void _usbd_stall_reset_ep1(usb_dir_t direction, usb_ep_cfg_t stall)
{
	stall &= 1;
	if (direction == USB_DIR_IN)
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

void usb_device_stall_ep1_bulk_out()
{
	_usbd_stall_reset_ep1(USB_DIR_OUT, USB_EP_CFG_STALL);
}

void usb_device_stall_ep1_bulk_in()
{
	_usbd_stall_reset_ep1(USB_DIR_IN, USB_EP_CFG_STALL);
}

static int _usbd_get_max_pkt_length(int endpoint)
{
	switch (endpoint)
	{
	case USB_EP_CTRL_OUT:
	case USB_EP_CTRL_IN:
			return 64;
	case USB_EP_BULK_OUT:
	case USB_EP_BULK_IN:
		if (usbd_otg->port_speed == USB_HIGH_SPEED)
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
	usb_dir_t direction = endpoint & 1;

	memset((void *)&usbdaemon->qhs[endpoint], 0, sizeof(dQH_t));

	if (!endpoint)
		usbdaemon->qhs[endpoint].ep_capabilities = USB_QHD_EP_CAP_IOS_ENABLE;

	usbdaemon->qhs[endpoint].next_dTD_ptr = 1; // TERMINATE_SET

	u32 max_packet_len = _usbd_get_max_pkt_length(endpoint) & USB_QHD_EP_CAP_MAX_PKT_LEN_MASK;
	usbdaemon->qhs[endpoint].ep_capabilities |= max_packet_len << 16;

	if (direction == USB_DIR_IN)
	{
		u32 endpoint_type = usbd_otg->regs->endptctrl[actual_ep] & ~USB2D_ENDPTCTRL_TX_EP_TYPE_MASK;
		if (actual_ep)
			endpoint_type |= usbd_otg->gadget ? USB2D_ENDPTCTRL_TX_EP_TYPE_INTR : USB2D_ENDPTCTRL_TX_EP_TYPE_BULK;
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
			endpoint_type |= usbd_otg->gadget ? USB2D_ENDPTCTRL_RX_EP_TYPE_INTR : USB2D_ENDPTCTRL_RX_EP_TYPE_BULK;
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
			return USB_ERROR_TIMEOUT;
		usleep(1);
	}

	return USB_RES_OK;
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
	usb_dir_t direction = endpoint & 1;
	u32 reg_mask = endpoint;

	// Flash all endpoints or 1.
	if (endpoint != USB_EP_ALL)
	{
		if (direction == USB_DIR_IN)
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
			return USB_ERROR_TIMEOUT;
		usleep(1);
	}

	// Wait for the endpoint to finish all transactions (buffer not ready).
	retries = 100000; // 200ms timeout.
	while (usbd_otg->regs->endptstatus & reg_mask)
	{
		retries--;
		if (!retries)
			return USB_ERROR_TIMEOUT;
		usleep(1);
	}

	// Wait for the endpoint to clear the primed status.
	retries = 100000; // 200ms timeout.
	while (usbd_otg->regs->endptprime & reg_mask)
	{
		retries--;
		if (!retries)
			return USB_ERROR_TIMEOUT;
		usleep(1);
	}

	return USB_RES_OK;
}

void usbd_end(bool reset_ep, bool only_controller)
{
	if (reset_ep)
	{
		usbd_flush_endpoint(USB_EP_ALL);
		_usbd_stall_reset_ep1(USB_DIR_OUT, USB_EP_CFG_RESET); // EP1 Bulk OUT.
		_usbd_stall_reset_ep1(USB_DIR_IN, USB_EP_CFG_RESET);  // EP1 Bulk IN.

		usbd_otg->config_num = 0;
		usbd_otg->interface_num = 0;
		usbd_otg->configuration_set = false;
		usbd_otg->max_lun_set = false;
	}

	// Stop device controller.
	usbd_otg->regs->usbcmd &= ~USB2D_USBCMD_RUN;

	// Enable PHY auto low power suspend.
	usbd_otg->regs->hostpc1_devlc |= USB2D_HOSTPC1_DEVLC_ASUS;

	if (!only_controller)
		_usb_device_power_down();
}

static void _usbd_mark_ep_complete(u32 endpoint)
{
	u32 complete_bit;
	usb_hw_ep_t actual_ep = (endpoint & 2) >> 1;
	usb_dir_t direction = endpoint & 1;

	usbd_flush_endpoint(endpoint);
	memset((void *)&usbdaemon->dtds[endpoint * 4], 0, sizeof(dTD_t) * 4);
	memset((void *)&usbdaemon->qhs[endpoint], 0, sizeof(dQH_t));
	usbdaemon->ep_configured[endpoint] = 0;
	usbdaemon->ep_bytes_requested[endpoint] = 0;

	if (direction == USB_DIR_IN)
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
	usb_dir_t direction = endpoint & 1;

	if (direction == USB_DIR_IN)
		reg_mask = USB2D_ENDPT_STATUS_TX_OFFSET << actual_ep;
	else
		reg_mask = USB2D_ENDPT_STATUS_RX_OFFSET << actual_ep;

	if (actual_ep == USB_HW_EP1)
		reg_val = usbd_otg->regs->endptctrl[1];
	else
		reg_val = usbd_otg->regs->endptctrl[0];

	// Check stalled status.
	if (direction == USB_DIR_IN)
		status = reg_val & USB2D_ENDPTCTRL_TX_EP_STALL;
	else
		status = reg_val & USB2D_ENDPTCTRL_RX_EP_STALL;

	if (status)
		return USB_EP_STATUS_STALLED;

	// Check enabled status.
	if (direction == USB_DIR_IN)
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

static int _usbd_ep_operation(usb_ep_t endpoint, u8 *buf, u32 len, u32 sync_timeout)
{
	if (!buf)
		len = 0;

	u32 prime_bit;
	usb_hw_ep_t actual_ep = (endpoint & 2) >> 1;
	usb_dir_t direction = endpoint & 1;
	u32 length_left = len;
	u32 dtd_ep_idx = endpoint * 4;

	_usbd_mark_ep_complete(endpoint);

	if (endpoint == USB_EP_CTRL_OUT)
		usbdaemon->qhs[endpoint].ep_capabilities = USB_QHD_EP_CAP_IOS_ENABLE;

	u32 max_packet_len = _usbd_get_max_pkt_length(endpoint) & USB_QHD_EP_CAP_MAX_PKT_LEN_MASK;
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

	if (direction == USB_DIR_IN)
	{
		prime_bit = USB2D_ENDPT_STATUS_TX_OFFSET << actual_ep;
		bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);
	}
	else
		prime_bit = USB2D_ENDPT_STATUS_RX_OFFSET << actual_ep;

	// Prime endpoint.
	usbd_otg->regs->endptprime |= prime_bit; // USB2_CONTROLLER_USB2D_ENDPTPRIME.

	int res = USB_RES_OK;
	usb_ep_status_t ep_status;
	if (sync_timeout)
	{
		ep_status = _usbd_get_ep_status(endpoint);
		if (ep_status == USB_EP_STATUS_ACTIVE)
		{
			u32 retries = sync_timeout;
			while (retries)
			{
				ep_status = _usbd_get_ep_status(endpoint);
				if (ep_status != USB_EP_STATUS_ACTIVE)
				{
					if (ep_status == USB_EP_STATUS_DISABLED)
						res = USB2_ERROR_XFER_EP_DISABLED;
					goto out;
				}
				retries--;
				usleep(1);
			}
			res = USB_ERROR_TIMEOUT;
		}
		else if (ep_status == USB_EP_STATUS_DISABLED)
			res = USB2_ERROR_XFER_EP_DISABLED;
out:
		if (res)
			_usbd_mark_ep_complete(endpoint);
		else if (_usbd_get_ep_status(endpoint) != USB_EP_STATUS_IDLE)
			res = USB_ERROR_XFER_ERROR;

		if (direction == USB_DIR_OUT)
			bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);
	}

	return res;
}

static int _usbd_ep_ack(usb_ep_t ep)
{
	return _usbd_ep_operation(ep, NULL, 0, USB_XFER_SYNCED_ENUM);
}

static void _usbd_set_ep0_stall()
{
	// EP Control endpoints must be always stalled together.
	usbd_otg->regs->endptctrl[0] =
			USB2D_ENDPTCTRL_TX_EP_ENABLE | USB2D_ENDPTCTRL_TX_EP_STALL |
			USB2D_ENDPTCTRL_RX_EP_ENABLE | USB2D_ENDPTCTRL_RX_EP_STALL;
}

int usbd_set_ep_stall(u32 endpoint, int ep_stall)
{
	usb_hw_ep_t actual_ep = (endpoint & 2) >> 1;
	usb_dir_t direction = endpoint & 1;

	if (ep_stall)
	{
		if (direction == USB_DIR_IN)
			usbd_otg->regs->endptctrl[actual_ep] |= USB2D_ENDPTCTRL_TX_EP_STALL; // Stall EP Bulk IN.
		else
			usbd_otg->regs->endptctrl[actual_ep] |= USB2D_ENDPTCTRL_RX_EP_STALL; // Stall EP Bulk OUT.
	}
	else
	{
		if (direction == USB_DIR_IN)
			usbd_otg->regs->endptctrl[actual_ep] &= ~USB2D_ENDPTCTRL_TX_EP_STALL; // Clear stall EP Bulk IN.
		else
			usbd_otg->regs->endptctrl[actual_ep] &= ~USB2D_ENDPTCTRL_RX_EP_STALL; // Clear stall EP Bulk OUT.
	}

	return USB_RES_OK;
}

static void _usbd_handle_get_class_request(bool *transmit_data, u8 *descriptor, int *size, bool *ep_stall)
{
	u8 _bRequest = usbd_otg->control_setup.bRequest;
	u16 _wIndex  = usbd_otg->control_setup.wIndex;
	u16 _wValue  = usbd_otg->control_setup.wValue;
	u16 _wLength = usbd_otg->control_setup.wLength;

	bool valid_interface = _wIndex == usbd_otg->interface_num;
	bool valid_len = (_bRequest == USB_REQUEST_BULK_GET_MAX_LUN) ? 1 : 0;

	if (!valid_interface || _wValue != 0 || _wLength != valid_len)
	{
		*ep_stall = true;
		return;
	}

	switch (_bRequest)
	{
	case USB_REQUEST_BULK_RESET:
		_usbd_ep_ack(USB_EP_CTRL_IN);
		usbd_otg->bulk_reset_req = true;
		break; // DELAYED_STATUS;
	case USB_REQUEST_BULK_GET_MAX_LUN:
		*transmit_data = true;
		*size = 1;
		descriptor[0] = usbd_otg->max_lun; // Set 0 LUN for 1 drive supported.
		usbd_otg->max_lun_set = true;
		break;
	default:
		*ep_stall = true;
		break;
	}
}

static void _usbd_handle_get_descriptor(bool *transmit_data, void **descriptor, int *size, bool *ep_stall)
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
		*transmit_data = true;
		return;
		}
	case USB_DESCRIPTOR_CONFIGURATION:
		if (usbd_otg->gadget == USB_GADGET_UMS)
		{
			if (usbd_otg->port_speed == USB_HIGH_SPEED) // High speed. 512 bytes.
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
			if (usbd_otg->port_speed == USB_HIGH_SPEED) // High speed. 512 bytes.
			{
				tmp->endpoint[0].wMaxPacketSize = 0x200;
				tmp->endpoint[1].wMaxPacketSize = 0x200;
				tmp->endpoint[0].bInterval = usbd_otg->gadget == USB_GADGET_HID_GAMEPAD ? 4 : 3; // 8ms : 4ms.
				tmp->endpoint[1].bInterval = usbd_otg->gadget == USB_GADGET_HID_GAMEPAD ? 4 : 3; // 8ms : 4ms.
			}
			else // Full speed. 64 bytes.
			{
				tmp->endpoint[0].wMaxPacketSize = 0x40;
				tmp->endpoint[1].wMaxPacketSize = 0x40;
				tmp->endpoint[0].bInterval = usbd_otg->gadget == USB_GADGET_HID_GAMEPAD ? 8 : 4; // 8ms : 4ms.
				tmp->endpoint[1].bInterval = usbd_otg->gadget == USB_GADGET_HID_GAMEPAD ? 8 : 4; // 8ms : 4ms.
			}
		}
		*descriptor = usbd_otg->desc->cfg;
		*size = usbd_otg->desc->cfg->config.wTotalLength;
		*transmit_data = true;
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
			*descriptor = usbd_otg->desc->serial;
			*size = usbd_otg->desc->serial[0];
			break;
		case 0xEE:
			*descriptor = usbd_otg->desc->ms_os;
			*size = usbd_otg->desc->ms_os->bLength;
			break;
		default:
			*descriptor = usbd_otg->desc->lang_id;
			*size = 4;
			break;
		}
		*transmit_data = true;
		return;
	case USB_DESCRIPTOR_DEVICE_QUALIFIER:
		if (!usbd_otg->desc->dev_qual)
			goto exit;
		usbd_otg->desc->dev_qual->bNumOtherConfigs = 1;
		*descriptor = usbd_otg->desc->dev_qual;
		*size = usbd_otg->desc->dev_qual->bLength;
		*transmit_data = true;
		return;
	case USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION:
		if (!usbd_otg->desc->cfg_other)
			goto exit;
		if (usbd_otg->port_speed == USB_HIGH_SPEED)
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
		*transmit_data = true;
		return;
	case USB_DESCRIPTOR_DEVICE_BINARY_OBJECT:
		*descriptor = usbd_otg->desc->dev_bot;
		*size = usbd_otg->desc->dev_bot->wTotalLength;
		*transmit_data = true;
		return;
	default:
		*transmit_data = false;
		*ep_stall = true;
		return;
	}
exit:
	*transmit_data = false;
	*ep_stall = true;
	return;
}

static int _usbd_handle_set_request(bool *ep_stall)
{
	int res = USB_RES_OK;
	u8 bRequest = usbd_otg->control_setup.bRequest;
	if (bRequest == USB_REQUEST_SET_ADDRESS)
	{
		res = _usbd_ep_ack(USB_EP_CTRL_IN);

		// Set USB address for device mode.
		if (!res)
			usbd_otg->regs->periodiclistbase = (usbd_otg->regs->periodiclistbase & 0x1FFFFFF) | ((usbd_otg->control_setup.wValue & 0xFF) << 25);
	}
	else if (bRequest == USB_REQUEST_SET_CONFIGURATION)
	{
		res = _usbd_ep_ack(USB_EP_CTRL_IN);
		if (!res)
		{
			usbd_otg->config_num = usbd_otg->control_setup.wValue;
			_usbd_initialize_ep_ctrl(USB_EP_BULK_OUT);
			_usbd_initialize_ep_ctrl(USB_EP_BULK_IN);
			usbd_otg->configuration_set = true;
		}
	}
	else
		*ep_stall = true;

	return res;
}

static int _usbd_handle_ep0_control_transfer()
{
	int res = USB_RES_OK;
	bool ep_stall = false;
	bool transmit_data = false;

	u8 *descriptor = (u8 *)USB_DESCRIPTOR_ADDR;
	int size = 0;

	u8  _bmRequestType = usbd_otg->control_setup.bmRequestType;
	u8  _bRequest      = usbd_otg->control_setup.bRequest;
	u16 _wValue        = usbd_otg->control_setup.wValue;
	u16 _wIndex        = usbd_otg->control_setup.wIndex;
	u16 _wLength       = usbd_otg->control_setup.wLength;

	//gfx_printf("%02X %02X %04X %04X %04X\n", _bmRequestType, _bRequest, _wValue, _wIndex, _wLength);

	switch (_bmRequestType)
	{
	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_DEVICE):
		res = _usbd_handle_set_request(&ep_stall);
		break;

	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE):
		res = _usbd_ep_ack(USB_EP_CTRL_IN);
		if (!res)
			usbd_otg->interface_num = _wValue;
		break;

	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_ENDPOINT):
		switch (_bRequest)
		{
		case USB_REQUEST_CLEAR_FEATURE:
		case USB_REQUEST_SET_FEATURE:
			if ((_wValue & 0xFF) == USB_FEATURE_ENDPOINT_HALT)
			{
				int direction;
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

				res = _usbd_ep_ack(USB_EP_CTRL_IN);
			}
			else
				_usbd_stall_reset_ep1(3, USB_EP_CFG_STALL);

			break;
		default:
			ep_stall = true;
			break;
		}
		break;

	case (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS    | USB_SETUP_RECIPIENT_INTERFACE):
		memset(descriptor, 0, _wLength);
		_usbd_handle_get_class_request(&transmit_data, descriptor, &size, &ep_stall);
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_DEVICE):
		switch (_bRequest)
		{
		case USB_REQUEST_GET_STATUS:
			descriptor[0] = USB_STATUS_DEV_SELF_POWERED;
			descriptor[1] = 0; // No support for remove wake up.
			transmit_data = true;
			size = 2;
			break;
		case USB_REQUEST_GET_DESCRIPTOR:
			_usbd_handle_get_descriptor(&transmit_data, (void **)&descriptor, &size, &ep_stall);
			break;
		case USB_REQUEST_GET_CONFIGURATION:
			descriptor = (u8 *)&usbd_otg->config_num;
			size = _wLength;
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
			memset(descriptor, 0, _wLength);
			descriptor[0] = usbd_otg->interface_num;
			size = _wLength;
		}
		else if (_bRequest == USB_REQUEST_GET_STATUS)
		{
			memset(descriptor, 0, _wLength);
			size = _wLength;
		}
		else if (_bRequest == USB_REQUEST_GET_DESCRIPTOR && (_wValue >> 8) == USB_DESCRIPTOR_HID_REPORT && usbd_otg->gadget > USB_GADGET_UMS)
		{
			if (usbd_otg->gadget == USB_GADGET_HID_GAMEPAD)
			{
				descriptor = (u8 *)&hid_report_descriptor_jc;
				size = hid_report_descriptor_jc_size;
			}
			else // USB_GADGET_HID_TOUCHPAD
			{
				descriptor = (u8 *)&hid_report_descriptor_touch;
				size = hid_report_descriptor_touch_size;
			}

			usbd_otg->hid_report_sent = true;
		}
		else
		{
			ep_stall = true;
			break;
		}

		if (_wLength < size)
			size = _wLength;
		transmit_data = true;
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_ENDPOINT):
		if (_bRequest == USB_REQUEST_GET_STATUS)
		{
			int ep_req;
			switch (_wIndex)
			{
			case USB_EP_ADDR_CTRL_OUT:
				ep_req = USB_EP_CTRL_OUT;
				break;
			case USB_EP_ADDR_BULK_OUT:
				ep_req = USB_EP_BULK_OUT;
				break;
			case USB_EP_ADDR_CTRL_IN:
				ep_req = USB_EP_CTRL_IN;
				break;
			case USB_EP_ADDR_BULK_IN:
				ep_req = USB_EP_BULK_IN;
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

			transmit_data = true;
		}
		else
			_usbd_stall_reset_ep1(3, USB_EP_CFG_STALL);
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_CLASS    | USB_SETUP_RECIPIENT_INTERFACE):
		memset(descriptor, 0, _wLength);
		_usbd_handle_get_class_request(&transmit_data, descriptor, &size, &ep_stall);
		break;

	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_VENDOR   | USB_SETUP_RECIPIENT_INTERFACE):
	case (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_VENDOR   | USB_SETUP_RECIPIENT_DEVICE):
		if (_bRequest == USB_REQUEST_GET_MS_DESCRIPTOR)
		{
			switch (_wIndex)
			{
			case USB_DESCRIPTOR_MS_COMPAT_ID:
				descriptor = (u8 *)usbd_otg->desc->ms_cid;
				size = usbd_otg->desc->ms_cid->dLength;
				transmit_data = true;
				break;
			case USB_DESCRIPTOR_MS_EXTENDED_PROPERTIES:
				descriptor = (u8 *)usbd_otg->desc->mx_ext;
				size = usbd_otg->desc->mx_ext->dLength;
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

	// Transmit data to HOST if any.
	if (transmit_data)
	{
		memcpy(usb_ep0_ctrl_buf, descriptor, size);

		if (_wLength < size)
			size = _wLength;
		res = _usbd_ep_operation(USB_EP_CTRL_IN, usb_ep0_ctrl_buf, size, USB_XFER_SYNCED_ENUM);
		if (!res)
			res = _usbd_ep_ack(USB_EP_CTRL_OUT);
	}

out:
	if (ep_stall)
		_usbd_set_ep0_stall();

	return res;
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
		usbd_otg->configuration_set = false;
		usbd_otg->max_lun_set = false;

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
				return USB_RES_OK;

			if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
				return USB_ERROR_USER_ABORT;
		}
	}

	return USB_ERROR_TIMEOUT;
}

int usb_device_enumerate(usb_gadget_type gadget)
{
	switch (gadget)
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

	usbd_otg->gadget = gadget;

	return _usbd_ep0_initialize();
}

int usbd_handle_ep0_ctrl_setup()
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

	// Only return error if bulk reset was requested.
	if (usbd_otg->bulk_reset_req)
	{
		usbd_otg->bulk_reset_req = false;
		return USB_RES_BULK_RESET;
	}

	return USB_RES_OK;
}

static usb_ep_status_t _usbd_get_ep1_status(usb_dir_t dir)
{
	usb_ep_t ep;
	if (dir == USB_DIR_OUT)
		ep = USB_EP_BULK_OUT;
	else
		ep = USB_EP_BULK_IN;
	return _usbd_get_ep_status(ep);
}

int usb_device_ep1_out_read(u8 *buf, u32 len, u32 *bytes_read, u32 sync_timeout)
{
	if ((u32)buf % USB_EP_BUFFER_ALIGN)
		return USB2_ERROR_XFER_NOT_ALIGNED;

	if (len > USB_EP_BUFFER_MAX_SIZE)
		len = USB_EP_BUFFER_MAX_SIZE;

	int res = _usbd_ep_operation(USB_EP_BULK_OUT, buf, len, sync_timeout);

	if (sync_timeout && bytes_read)
		*bytes_read = res ? 0 : len;

	return res;
}

int usb_device_ep1_out_read_big(u8 *buf, u32 len, u32 *bytes_read)
{
	if ((u32)buf % USB_EP_BUFFER_ALIGN)
		return USB2_ERROR_XFER_NOT_ALIGNED;

	if (len > USB_EP_BULK_OUT_MAX_XFER)
		len = USB_EP_BULK_OUT_MAX_XFER;

	int res;
	u32 bytes = 0;
	*bytes_read = 0;
	u8 *buf_curr = buf;

	while (len)
	{
		u32 len_ep = MIN(len, USB_EP_BUFFER_MAX_SIZE);

		res = usb_device_ep1_out_read(buf_curr, len_ep, &bytes, USB_XFER_SYNCED_DATA);
		if (res)
			return res;

		len -= len_ep;
		buf_curr += len_ep;
		*bytes_read = *bytes_read + bytes;
	}

	return USB_RES_OK;
}

static int _usbd_get_ep1_out_bytes_read()
{
	if (_usbd_get_ep_status(USB_EP_BULK_OUT) != USB_EP_STATUS_IDLE)
		return 0;
	else
		return (usbdaemon->ep_bytes_requested[USB_EP_BULK_OUT] - (usbdaemon->qhs[USB_EP_BULK_OUT].token >> 16));
}

int usb_device_ep1_out_reading_finish(u32 *pending_bytes, u32 sync_timeout)
{
	usb_ep_status_t ep_status;
	do
	{
		ep_status = _usbd_get_ep1_status(USB_DIR_OUT);
		if ((ep_status == USB_EP_STATUS_IDLE) || (ep_status == USB_EP_STATUS_DISABLED))
			break;

		usbd_handle_ep0_ctrl_setup();
	}
	while ((ep_status == USB_EP_STATUS_ACTIVE) || (ep_status == USB_EP_STATUS_STALLED));

	*pending_bytes = _usbd_get_ep1_out_bytes_read();

	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	if (ep_status == USB_EP_STATUS_IDLE)
		return USB_RES_OK;
	else if (ep_status == USB_EP_STATUS_DISABLED)
		return USB2_ERROR_XFER_EP_DISABLED;
	else
		return USB_ERROR_XFER_ERROR;
}

int usb_device_ep1_in_write(u8 *buf, u32 len, u32 *bytes_written, u32 sync_timeout)
{
	if ((u32)buf % USB_EP_BUFFER_ALIGN)
		return USB2_ERROR_XFER_NOT_ALIGNED;

	if (len > USB_EP_BUFFER_MAX_SIZE)
		len = USB_EP_BUFFER_MAX_SIZE;

	int res = _usbd_ep_operation(USB_EP_BULK_IN, buf, len, sync_timeout);

	if (sync_timeout && bytes_written)
		*bytes_written = res ? 0 : len;

	return res;
}

static int _usbd_get_ep1_in_bytes_written()
{
	if (_usbd_get_ep_status(USB_EP_BULK_IN) != USB_EP_STATUS_IDLE)
		return 0;
	else
		return (usbdaemon->ep_bytes_requested[USB_EP_BULK_IN] - (usbdaemon->qhs[USB_EP_BULK_IN].token >> 16));
}

int usb_device_ep1_in_writing_finish(u32 *pending_bytes, u32 sync_timeout)
{
	usb_ep_status_t ep_status;
	do
	{
		ep_status = _usbd_get_ep1_status(USB_DIR_IN);
		if ((ep_status == USB_EP_STATUS_IDLE) || (ep_status == USB_EP_STATUS_DISABLED))
			break;

		usbd_handle_ep0_ctrl_setup();
	}
	while ((ep_status == USB_EP_STATUS_ACTIVE) || (ep_status == USB_EP_STATUS_STALLED));

	*pending_bytes = _usbd_get_ep1_in_bytes_written();

	if (ep_status == USB_EP_STATUS_IDLE)
		return USB_RES_OK;
	else if (ep_status == USB_EP_STATUS_DISABLED)
		return USB2_ERROR_XFER_EP_DISABLED;

	usb_device_stall_ep1_bulk_out();
	return USB_ERROR_XFER_ERROR;
}

bool usb_device_get_suspended()
{
	bool suspended = (usbd_otg->regs->portsc1 & USB2D_PORTSC1_SUSP) == USB2D_PORTSC1_SUSP;
	return suspended;
}

bool usb_device_get_port_in_sleep()
{
	// Windows heuristic: Forces port into suspend, sleep and J-State.
	return (usbd_otg->regs->portsc1) == 0x885;
}

int usb_device_class_send_max_lun(u8 max_lun)
{
	// Timeout if get MAX_LUN request doesn't happen in 10s.
	u32 timer = get_tmr_ms() + 10000;

	usbd_otg->max_lun = max_lun;

	while (!usbd_otg->max_lun_set)
	{
		usbd_handle_ep0_ctrl_setup();
		if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			return USB_ERROR_USER_ABORT;
	}

	return USB_RES_OK;
}

int usb_device_class_send_hid_report()
{
	// Timeout if get GET_HID_REPORT request doesn't happen in 10s.
	u32 timer = get_tmr_ms() + 10000;

	// Wait for request and transfer start.
	while (!usbd_otg->hid_report_sent)
	{
		usbd_handle_ep0_ctrl_setup();
		if (timer < get_tmr_ms() || btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
			return USB_ERROR_USER_ABORT;
	}

	return USB_RES_OK;
}

void usb_device_get_ops(usb_ops_t *ops)
{
	ops->usbd_flush_endpoint               = usbd_flush_endpoint;
	ops->usbd_set_ep_stall                 = usbd_set_ep_stall;
	ops->usbd_handle_ep0_ctrl_setup        = usbd_handle_ep0_ctrl_setup;
	ops->usbd_end                          = usbd_end;
	ops->usb_device_init                   = usb_device_init;
	ops->usb_device_enumerate              = usb_device_enumerate;
	ops->usb_device_class_send_max_lun     = usb_device_class_send_max_lun;
	ops->usb_device_class_send_hid_report  = usb_device_class_send_hid_report;
	ops->usb_device_get_suspended          = usb_device_get_suspended;
	ops->usb_device_get_port_in_sleep      = usb_device_get_port_in_sleep;

	ops->usb_device_ep1_out_read           = usb_device_ep1_out_read;
	ops->usb_device_ep1_out_read_big       = usb_device_ep1_out_read_big;
	ops->usb_device_ep1_out_reading_finish = usb_device_ep1_out_reading_finish;
	ops->usb_device_ep1_in_write           = usb_device_ep1_in_write;
	ops->usb_device_ep1_in_writing_finish  = usb_device_ep1_in_writing_finish;
}

