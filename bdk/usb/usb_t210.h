/*
 * USB driver for Tegra X1
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

#ifndef _USB_T210_H_
#define _USB_T210_H_

#include <utils/types.h>

/* General USB registers */
#define USB1_IF_USB_SUSP_CTRL         0x400
#define  SUSP_CTRL_USB_WAKE_ON_CNNT_EN_DEV   (1 << 3)
#define  SUSP_CTRL_USB_WAKE_ON_DISCON_EN_DEV (1 << 4)
#define  SUSP_CTRL_USB_PHY_CLK_VALID         (1 << 7)
#define  SUSP_CTRL_UTMIP_RESET               (1 << 11)
#define  SUSP_CTRL_UTMIP_PHY_ENB             (1 << 12)
#define  SUSP_CTRL_UTMIP_UTMIP_SUSPL1_SET    (1 << 25)
#define USB1_IF_USB_PHY_VBUS_SENSORS  0x404
#define USB1_UTMIP_XCVR_CFG0          0x808
#define USB1_UTMIP_BIAS_CFG0          0x80C
#define USB1_UTMIP_HSRX_CFG0          0x810
#define USB1_UTMIP_HSRX_CFG1          0x814
#define USB1_UTMIP_TX_CFG0            0x820
#define USB1_UTMIP_MISC_CFG1          0x828
#define USB1_UTMIP_DEBOUNCE_CFG0      0x82C
#define USB1_UTMIP_SPARE_CFG0         0x834
#define USB1_UTMIP_XCVR_CFG1          0x838
#define USB1_UTMIP_BIAS_CFG1          0x83C
#define USB1_UTMIP_BIAS_CFG2          0x850
#define USB1_UTMIP_XCVR_CFG2          0x854
#define USB1_UTMIP_XCVR_CFG3          0x858

/* USB Queue Head Descriptor */
#define USB2_QH_USB2D_QH_EP_BASE      (USB_BASE + 0x1000)
#define  USB_QHD_EP_CAP_IOS_ENABLE        (1 << 15)
#define  USB_QHD_EP_CAP_MAX_PKT_LEN_MASK  0x7FF
#define  USB_QHD_EP_CAP_ZERO_LEN_TERM_DIS (1 << 29)
#define  USB_QHD_EP_CAP_MULTI_NON_ISO     (0 << 30)
#define  USB_QHD_EP_CAP_MULTI_1           (1 << 30)
#define  USB_QHD_EP_CAP_MULTI_2           (2 << 30)
#define  USB_QHD_EP_CAP_MULTI_3           (3 << 30)

#define  USB_QHD_TOKEN_XFER_ERROR         (1 << 3)
#define  USB_QHD_TOKEN_BUFFER_ERROR       (1 << 5)
#define  USB_QHD_TOKEN_HALTED             (1 << 6)
#define  USB_QHD_TOKEN_ACTIVE             (1 << 7)
#define  USB_QHD_TOKEN_MULT_OVERR_MASK    (2 << 10)
#define  USB_QHD_TOKEN_IRQ_ON_COMPLETE    (1 << 15)
#define  USB_QHD_TOKEN_TOTAL_BYTES_SHIFT  16

/* USB_OTG/USB_1 controllers register bits */
#define USB2D_PORTSC1_SUSP (1 << 7)

#define USB2D_USBCMD_RUN      (1 << 0)
#define USB2D_USBCMD_RESET    (1 << 1)
#define USB2D_USBCMD_ITC_MASK (0xFF << 16)

#define USB2D_USBSTS_UI  (1 << 0)
#define USB2D_USBSTS_UEI (1 << 1)
#define USB2D_USBSTS_PCI (1 << 2)
#define USB2D_USBSTS_FRI (1 << 3)
#define USB2D_USBSTS_SEI (1 << 4)
#define USB2D_USBSTS_AAI (1 << 5)
#define USB2D_USBSTS_URI (1 << 6)
#define USB2D_USBSTS_SRI (1 << 7)
#define USB2D_USBSTS_SLI (1 << 8)

#define USB2D_USBMODE_CM_MASK   (3 << 0)
#define USB2D_USBMODE_CM_IDLE   0
#define USB2D_USBMODE_CM_RSVD   1
#define USB2D_USBMODE_CM_DEVICE 2
#define USB2D_USBMODE_CM_HOST   3

#define USB2D_ENDPT_STATUS_RX_OFFSET (1 << 0)
#define USB2D_ENDPT_STATUS_TX_OFFSET (1 << 16)

#define USB2D_ENDPTCTRL_RX_EP_STALL     (1 << 0)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_CTRL (0 << 2)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_ISO  (1 << 2)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_BULK (2 << 2)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_INTR (3 << 2)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_MASK (3 << 2)
#define USB2D_ENDPTCTRL_RX_EP_INHIBIT   (1 << 5)
#define USB2D_ENDPTCTRL_RX_EP_RESET     (1 << 6)
#define USB2D_ENDPTCTRL_RX_EP_ENABLE    (1 << 7)
#define USB2D_ENDPTCTRL_TX_EP_STALL     (1 << 16)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_CTRL (0 << 18)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_ISO  (1 << 18)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_BULK (2 << 18)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_INTR (3 << 18)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_MASK (3 << 18)
#define USB2D_ENDPTCTRL_TX_EP_INHIBIT   (1 << 21)
#define USB2D_ENDPTCTRL_TX_EP_RESET     (1 << 22)
#define USB2D_ENDPTCTRL_TX_EP_ENABLE    (1 << 23)

#define USB2D_HOSTPC1_DEVLC_ASUS      (1 << 17)
#define USB2D_HOSTPC1_DEVLC_PHCD      (1 << 22)
#define USB2D_HOSTPC1_DEVLC_PSPD_MASK (3 << 25)

#define USB2D_OTGSC_USB_ID_PULLUP     (1 << 5)
#define USB2D_OTGSC_USB_IRQ_STS_MASK  (0x7F << 16)

/* USB_OTG/USB_1 controllers registers */
typedef struct _t210_usb2d_t
{
	vu32 id;
	vu32 unk0;
	vu32 hw_host;
	vu32 hw_device;
	vu32 hw_txbuf;
	vu32 hw_rxbuf;
	vu32 unk1[26];
	vu32 gptimer0ld;
	vu32 gptimer0ctrl;
	vu32 gptimer1ld;
	vu32 gptimer1ctrl;
	vu32 unk2[28];
	vu16 caplength;
	vu16 hciversion;
	vu32 hcsparams;
	vu32 hccparams;
	vu32 unk3[5];
	vu32 dciversion;
	vu32 dccparams;
	vu32 extsts;
	vu32 usbextintr;
	vu32 usbcmd;
	vu32 usbsts;
	vu32 usbintr;
	vu32 frindex;
	vu32 unk4;
	vu32 periodiclistbase;
	vu32 asynclistaddr;
	vu32 asyncttsts;
	vu32 burstsize;
	vu32 txfilltuning;
	vu32 unk6;
	vu32 icusb_ctrl;
	vu32 ulpi_viewport;
	vu32 rsvd0[4];
	vu32 portsc1;
	vu32 rsvd1[15];
	vu32 hostpc1_devlc;
	vu32 rsvd2[15];
	vu32 otgsc;
	vu32 usbmode;
	vu32 unk10;
	vu32 endptnak;
	vu32 endptnak_enable;
	vu32 endptsetupstat;
	vu32 endptprime;
	vu32 endptflush;
	vu32 endptstatus;
	vu32 endptcomplete;
	vu32 endptctrl[16];
} t210_usb2d_t;

#endif
