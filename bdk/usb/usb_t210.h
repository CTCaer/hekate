/*
 * Enhanced & eXtensible USB device (EDCI & XDCI) driver for Tegra X1
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

#ifndef _USB_T210_H_
#define _USB_T210_H_

#include <utils/types.h>

/* EHCI USB */

/* General USB registers */
#define USB1_IF_USB_SUSP_CTRL         0x400
#define  SUSP_CTRL_USB_WAKE_ON_CNNT_EN_DEV   BIT(3)
#define  SUSP_CTRL_USB_WAKE_ON_DISCON_EN_DEV BIT(4)
#define  SUSP_CTRL_USB_PHY_CLK_VALID         BIT(7)
#define  SUSP_CTRL_UTMIP_RESET               BIT(11)
#define  SUSP_CTRL_UTMIP_PHY_ENB             BIT(12)
#define  SUSP_CTRL_UTMIP_UTMIP_SUSPL1_SET    BIT(25)
#define USB1_IF_USB_PHY_VBUS_SENSORS  0x404
#define USB1_UTMIP_XCVR_CFG0          0x808
#define USB1_UTMIP_BIAS_CFG0          0x80C
#define USB1_UTMIP_HSRX_CFG0          0x810
#define USB1_UTMIP_HSRX_CFG1          0x814
#define USB1_UTMIP_TX_CFG0            0x820
#define USB1_UTMIP_MISC_CFG1          0x828
#define USB1_UTMIP_DEBOUNCE_CFG0      0x82C
#define USB1_UTMIP_BAT_CHRG_CFG0      0x830
#define  BAT_CHRG_CFG0_PWRDOWN_CHRG          BIT(0)
#define  BAT_CHRG_CFG0_OP_SRC_EN             BIT(3)
#define USB1_UTMIP_SPARE_CFG0         0x834
#define USB1_UTMIP_XCVR_CFG1          0x838
#define USB1_UTMIP_BIAS_CFG1          0x83C
#define USB1_UTMIP_BIAS_CFG2          0x850
#define USB1_UTMIP_XCVR_CFG2          0x854
#define USB1_UTMIP_XCVR_CFG3          0x858

/* USB Queue Head Descriptor */
#define USB2_QH_USB2D_QH_EP_BASE      (USB_BASE + 0x1000)
#define  USB_QHD_EP_CAP_IOS_ENABLE        BIT(15)
#define  USB_QHD_EP_CAP_MAX_PKT_LEN_MASK  0x7FF
#define  USB_QHD_EP_CAP_ZERO_LEN_TERM_DIS BIT(29)
#define  USB_QHD_EP_CAP_MULTI_NON_ISO     (0 << 30)
#define  USB_QHD_EP_CAP_MULTI_1           (1 << 30)
#define  USB_QHD_EP_CAP_MULTI_2           (2 << 30)
#define  USB_QHD_EP_CAP_MULTI_3           (3 << 30)

#define  USB_QHD_TOKEN_XFER_ERROR         BIT(3)
#define  USB_QHD_TOKEN_BUFFER_ERROR       BIT(5)
#define  USB_QHD_TOKEN_HALTED             BIT(6)
#define  USB_QHD_TOKEN_ACTIVE             BIT(7)
#define  USB_QHD_TOKEN_MULT_OVERR_MASK    (2 << 10)
#define  USB_QHD_TOKEN_IRQ_ON_COMPLETE    BIT(15)
#define  USB_QHD_TOKEN_TOTAL_BYTES_SHIFT  16

/* USB_OTG/USB_1 controllers register bits */
#define USB2D_PORTSC1_SUSP BIT(7)

#define USB2D_USBCMD_RUN      BIT(0)
#define USB2D_USBCMD_RESET    BIT(1)
#define USB2D_USBCMD_ITC_MASK (0xFF << 16)

#define USB2D_USBSTS_UI  BIT(0)
#define USB2D_USBSTS_UEI BIT(1)
#define USB2D_USBSTS_PCI BIT(2)
#define USB2D_USBSTS_FRI BIT(3)
#define USB2D_USBSTS_SEI BIT(4)
#define USB2D_USBSTS_AAI BIT(5)
#define USB2D_USBSTS_URI BIT(6)
#define USB2D_USBSTS_SRI BIT(7)
#define USB2D_USBSTS_SLI BIT(8)

#define USB2D_USBMODE_CM_MASK   (3 << 0)
#define USB2D_USBMODE_CM_IDLE   0
#define USB2D_USBMODE_CM_RSVD   1
#define USB2D_USBMODE_CM_DEVICE 2
#define USB2D_USBMODE_CM_HOST   3

#define USB2D_ENDPT_STATUS_RX_OFFSET BIT(0)
#define USB2D_ENDPT_STATUS_TX_OFFSET BIT(16)

#define USB2D_ENDPTCTRL_RX_EP_STALL     BIT(0)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_CTRL (0 << 2)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_ISO  (1 << 2)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_BULK (2 << 2)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_INTR (3 << 2)
#define USB2D_ENDPTCTRL_RX_EP_TYPE_MASK (3 << 2)
#define USB2D_ENDPTCTRL_RX_EP_INHIBIT   BIT(5)
#define USB2D_ENDPTCTRL_RX_EP_RESET     BIT(6)
#define USB2D_ENDPTCTRL_RX_EP_ENABLE    BIT(7)
#define USB2D_ENDPTCTRL_TX_EP_STALL     BIT(16)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_CTRL (0 << 18)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_ISO  (1 << 18)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_BULK (2 << 18)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_INTR (3 << 18)
#define USB2D_ENDPTCTRL_TX_EP_TYPE_MASK (3 << 18)
#define USB2D_ENDPTCTRL_TX_EP_INHIBIT   BIT(21)
#define USB2D_ENDPTCTRL_TX_EP_RESET     BIT(22)
#define USB2D_ENDPTCTRL_TX_EP_ENABLE    BIT(23)

#define USB2D_HOSTPC1_DEVLC_ASUS      BIT(17)
#define USB2D_HOSTPC1_DEVLC_PHCD      BIT(22)
#define USB2D_HOSTPC1_DEVLC_PSPD_MASK (3 << 25)

#define USB2D_OTGSC_USB_ID_PULLUP     BIT(5)
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


/* XHCI USB */

/* XUSB DEV XHCI registers */
#define XUSB_DEV_XHCI_DB                 0x4
#define XUSB_DEV_XHCI_ERSTSZ             0x8
#define XUSB_DEV_XHCI_ERST0BALO          0x10
#define XUSB_DEV_XHCI_ERST0BAHI          0x14
#define XUSB_DEV_XHCI_ERST1BALO          0x18
#define XUSB_DEV_XHCI_ERST1BAHI          0x1C
#define XUSB_DEV_XHCI_ERDPLO             0x20
#define  XHCI_ERDPLO_EHB                 BIT(3)
#define XUSB_DEV_XHCI_ERDPHI             0x24
#define XUSB_DEV_XHCI_EREPLO             0x28
#define  XCHI_ECS                        BIT(0)
#define XUSB_DEV_XHCI_EREPHI             0x2C
#define XUSB_DEV_XHCI_CTRL               0x30
#define  XHCI_CTRL_RUN                   BIT(0)
#define  XHCI_CTRL_LSE                   BIT(1)
#define  XHCI_CTRL_IE                    BIT(4)
#define  XHCI_CTRL_ENABLE                BIT(31)
#define XUSB_DEV_XHCI_ST                 0x34
#define  XHCI_ST_RC                      BIT(0)
#define  XHCI_ST_IP                      BIT(4)
#define XUSB_DEV_XHCI_RT_IMOD            0x38
#define XUSB_DEV_XHCI_PORTSC             0x3C
#define  XHCI_PORTSC_CCS                 BIT(0)
#define  XHCI_PORTSC_PED                 BIT(1)
#define  XHCI_PORTSC_PR                  BIT(4)
#define  XHCI_PORTSC_PLS_MASK            (0xF << 5)
#define   XHCI_PORTSC_PLS_U0             (0 << 5)
#define   XHCI_PORTSC_PLS_U1             (1 << 5)
#define   XHCI_PORTSC_PLS_U2             (2 << 5)
#define   XHCI_PORTSC_PLS_U3             (3 << 5)
#define   XHCI_PORTSC_PLS_DISABLED       (4 << 5)
#define   XHCI_PORTSC_PLS_RXDETECT       (5 << 5)
#define   XHCI_PORTSC_PLS_INACTIVE       (6 << 5)
#define   XHCI_PORTSC_PLS_POLLING        (7 << 5)
#define   XHCI_PORTSC_PLS_RECOVERY       (8 << 5)
#define   XHCI_PORTSC_PLS_HOTRESET       (9 << 5)
#define   XHCI_PORTSC_PLS_COMPLIANCE     (10 << 5)
#define   XHCI_PORTSC_PLS_LOOPBACK       (11 << 5)
#define   XHCI_PORTSC_PLS_RESUME         (15 << 5)
#define  XHCI_PORTSC_PS                  (0xF << 10)
#define  XHCI_PORTSC_LWS                 BIT(16)
#define  XHCI_PORTSC_CSC                 BIT(17)
#define  XHCI_PORTSC_WRC                 BIT(19)
#define  XHCI_PORTSC_PRC                 BIT(21)
#define  XHCI_PORTSC_PLC                 BIT(22)
#define  XHCI_PORTSC_CEC                 BIT(23)
#define  XHCI_PORTSC_WPR                 BIT(30)
#define XUSB_DEV_XHCI_ECPLO              0x40
#define XUSB_DEV_XHCI_ECPHI              0x44
#define XUSB_DEV_XHCI_EP_HALT            0x50
#define  XHCI_EP_HALT_DCI_EP0_IN         BIT(0)
#define XUSB_DEV_XHCI_EP_PAUSE           0x54
#define XUSB_DEV_XHCI_EP_RELOAD          0x58
#define XUSB_DEV_XHCI_EP_STCHG           0x5C
#define XUSB_DEV_XHCI_PORTHALT           0x6C
#define XUSB_DEV_XHCI_EP_STOPPED         0x78
#define  XHCI_PORTHALT_HALT_LTSSM        BIT(0)
#define  XHCI_PORTHALT_STCHG_REQ         BIT(20)
#define XUSB_DEV_XHCI_CFG_DEV_FE         0x85C
#define  XHCI_CFG_DEV_FE_PORTREGSEL_MASK (3 << 0)
#define  XHCI_CFG_DEV_FE_PORTREGSEL_SS   (1 << 0)
#define  XHCI_CFG_DEV_FE_PORTREGSEL_HSFS (2 << 0)

/* XUSB DEV PCI registers */
#define XUSB_CFG_1                 0x4
#define  CFG_1_IO_SPACE            BIT(0)
#define  CFG_1_MEMORY_SPACE        BIT(1)
#define  CFG_1_BUS_MASTER          BIT(2)
#define XUSB_CFG_4                 0x10
#define  CFG_4_ADDRESS_TYPE_32_BIT (0 << 1)
#define  CFG_4_ADDRESS_TYPE_64_BIT (2 << 1)

/* XUSB DEV Device registers */
#define XUSB_DEV_CONFIGURATION     0x180
#define  DEV_CONFIGURATION_EN_FPCI BIT(0)
#define XUSB_DEV_INTR_MASK         0x188
#define  DEV_INTR_MASK_IP_INT_MASK BIT(16)

/* XUSB Pad Control registers */
#define XUSB_PADCTL_USB2_PAD_MUX 0x4
#define  PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_USB2 (0 << 0)
#define  PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_XUSB (1 << 0)
#define  PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_MASK (3 << 0)
#define  PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_USB2      (0 << 18)
#define  PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_XUSB      (1 << 18)
#define  PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_MASK      (3 << 18)
#define XUSB_PADCTL_USB2_PORT_CAP 0x8
#define  PADCTL_USB2_PORT_CAP_PORT_0_CAP_DIS  (0 << 0)
#define  PADCTL_USB2_PORT_CAP_PORT_0_CAP_HOST (1 << 0)
#define  PADCTL_USB2_PORT_CAP_PORT_0_CAP_DEV  (2 << 0)
#define  PADCTL_USB2_PORT_CAP_PORT_0_CAP_OTG  (3 << 0)
#define  PADCTL_USB2_PORT_CAP_PORT_0_CAP_MASK (3 << 0)
#define XUSB_PADCTL_SS_PORT_MAP 0x14
#define  PADCTL_SS_PORT_MAP_PORT0_MASK (0xF << 0)
#define XUSB_PADCTL_ELPG_PROGRAM_0 0x20
#define XUSB_PADCTL_ELPG_PROGRAM_1 0x24
#define XUSB_PADCTL_USB2_BATTERY_CHRG_OTGPAD0_CTL0 0x80
#define XUSB_PADCTL_USB2_BATTERY_CHRG_OTGPAD0_CTL1 0x84
#define XUSB_PADCTL_USB2_OTG_PAD0_CTL_0 0x88
#define XUSB_PADCTL_USB2_OTG_PAD0_CTL_1 0x8C
#define XUSB_PADCTL_USB2_BIAS_PAD_CTL_0 0x284
#define XUSB_PADCTL_USB2_BIAS_PAD_CTL_1 0x288
#define XUSB_PADCTL_USB2_VBUS_ID 0xC60
#define  PADCTL_USB2_VBUS_ID_VBUS_OVR_EN   (1 << 12)
#define  PADCTL_USB2_VBUS_ID_VBUS_OVR_MASK (3 << 12)
#define  PADCTL_USB2_VBUS_ID_VBUS_ON       BIT(14)
#define  PADCTL_USB2_VBUS_ID_SRC_ID_OVR_EN (1 << 16)
#define  PADCTL_USB2_VBUS_ID_SRC_MASK      (3 << 16)
#define  PADCTL_USB2_VBUS_ID_OVR_GND       (0 << 18)
#define  PADCTL_USB2_VBUS_ID_OVR_C         (1 << 18)
#define  PADCTL_USB2_VBUS_ID_OVR_B         (2 << 18)
#define  PADCTL_USB2_VBUS_ID_OVR_A         (4 << 18)
#define  PADCTL_USB2_VBUS_ID_OVR_FLOAT     (8 << 18)
#define  PADCTL_USB2_VBUS_ID_OVR_MASK      (0xF << 18)

#endif
