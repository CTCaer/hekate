/*
 * USB driver for Tegra X1
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

#include <usb/usb_descriptor_types.h>
#include <utils/types.h>

static usb_dev_descr_t usb_device_descriptor_ums =
{
	.bLength         = 18,
	.bDescriptorType = USB_DESCRIPTOR_DEVICE,
	.bcdUSB          = 0x210,
	.bDeviceClass    = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize  = 0x40,
	.idVendor        = 0x11EC, // Nintendo: 0x057E, Nvidia: 0x0955
	.idProduct       = 0xA7E0, // Switch:   0x2000, usbd:   0x3000
	.bcdDevice       = 0x0101,
	.iManufacturer   = 1,
	.iProduct        = 2,
	.iSerialNumber   = 3,
	.bNumConfigs     = 1
};

static usb_dev_qual_descr_t usb_device_qualifier_descriptor =
{
	.bLength          = 10,
	.bDescriptorType  = USB_DESCRIPTOR_DEVICE_QUALIFIER,
	.bcdUSB           = 0x210,
	.bDeviceClass     = 0x00,
	.bDeviceSubClass  = 0x00,
	.bDeviceProtocol  = 0x00,
	.bMaxPacketSize   = 0x40,
	.bNumOtherConfigs = 0x01,
	.bReserved        = 0x00
};

static usb_cfg_simple_descr_t usb_configuration_descriptor_ums =
{
	/* Configuration descriptor structure */
	.config.bLength               = 9,
	.config.bDescriptorType       = USB_DESCRIPTOR_CONFIGURATION,
	.config.wTotalLength          = 0x20,
	.config.bNumInterfaces        = 0x01,
	.config.bConfigurationValue   = 0x01,
	.config.iConfiguration        = 0x00,
	.config.bmAttributes          = USB_ATTR_SELF_POWERED | USB_ATTR_BUS_POWERED_RSVD,
	.config.bMaxPower             = 32 / 2,

	/* Interface descriptor structure */
	.interface.bLength            = 9,
	.interface.bDescriptorType    = USB_DESCRIPTOR_INTERFACE,
	.interface.bInterfaceNumber   = 0,
	.interface.bAlternateSetting  = 0,
	.interface.bNumEndpoints      = 2,
	.interface.bInterfaceClass    = 0x08, // Mass Storage Class.
	.interface.bInterfaceSubClass = 0x06, // SCSI Transparent Command Set.
	.interface.bInterfaceProtocol = 0x50, // Bulk-Only Transport.
	.interface.iInterface         = 0x00,

	/* Endpoint descriptor structure EP1 IN */
	.endpoint[0].bLength          = 7,
	.endpoint[0].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[0].bEndpointAddress = 0x81, // USB_EP_ADDR_BULK_IN.
	.endpoint[0].bmAttributes     = USB_EP_TYPE_BULK,
	.endpoint[0].wMaxPacketSize   = 0x200,
	.endpoint[0].bInterval        = 0x00,

	/* Endpoint descriptor structure EP1 OUT */
	.endpoint[1].bLength          = 7,
	.endpoint[1].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[1].bEndpointAddress = 0x01, // USB_EP_ADDR_BULK_OUT.
	.endpoint[1].bmAttributes     = USB_EP_TYPE_BULK,
	.endpoint[1].wMaxPacketSize   = 0x200,
	.endpoint[1].bInterval        = 0x00
};

static usb_cfg_simple_descr_t usb_other_speed_config_descriptor_ums =
{
	/* Other Speed Configuration descriptor structure */
	.config.bLength               = 9,
	.config.bDescriptorType       = USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION,
	.config.wTotalLength          = 0x20,
	.config.bNumInterfaces        = 0x01,
	.config.bConfigurationValue   = 0x01,
	.config.iConfiguration        = 0x00,
	.config.bmAttributes          = USB_ATTR_SELF_POWERED | USB_ATTR_BUS_POWERED_RSVD,
	.config.bMaxPower             = 32 / 2,

	/* Interface descriptor structure */
	.interface.bLength            = 9,
	.interface.bDescriptorType    = USB_DESCRIPTOR_INTERFACE,
	.interface.bInterfaceNumber   = 0x00,
	.interface.bAlternateSetting  = 0x00,
	.interface.bNumEndpoints      = 2,
	.interface.bInterfaceClass    = 0x08, // Mass Storage Class.
	.interface.bInterfaceSubClass = 0x06, // SCSI Transparent Command Set.
	.interface.bInterfaceProtocol = 0x50, // Bulk-Only Transport.
	.interface.iInterface         = 0x00,

	/* Endpoint descriptor structure EP1 IN */
	.endpoint[0].bLength          = 7,
	.endpoint[0].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[0].bEndpointAddress = 0x81, // USB_EP_ADDR_BULK_IN.
	.endpoint[0].bmAttributes     = USB_EP_TYPE_BULK,
	.endpoint[0].wMaxPacketSize   = 0x40,
	.endpoint[0].bInterval        = 0,

	/* Endpoint descriptor structure EP1 OUT */
	.endpoint[1].bLength          = 7,
	.endpoint[1].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[1].bEndpointAddress = 0x01, // USB_EP_ADDR_BULK_OUT.
	.endpoint[1].bmAttributes     = USB_EP_TYPE_BULK,
	.endpoint[1].wMaxPacketSize   = 0x40,
	.endpoint[1].bInterval        = 0
};

static usb_dev_bot_t usb_device_binary_object_descriptor =
{
	.bLength                = 5,
	.bDescriptorType        = USB_DESCRIPTOR_DEVICE_BINARY_OBJECT,
	.wTotalLength           = 22,
	.bNumDeviceCaps         = 2,

	/* Device Capability USB 2.0 Extension Descriptor */
	.bLengthCap0            = 7,
	.bDescriptorTypeCap0    = USB_DESCRIPTOR_DEVICE_BINARY_OBJECT_CAP,
	.bDevCapabilityTypeCap0 = 2, // USB2.
	.bmAttributesCap0       = 0,

	/* Device Capability SuperSpeed Descriptor */
	/* Needed for a USB2.10 device. */
	.bLengthCap1            = 10,
	.bDescriptorTypeCap1    = USB_DESCRIPTOR_DEVICE_BINARY_OBJECT_CAP,
	.bDevCapabilityTypeCap1 = 3,   // USB3.
	.bmAttributesCap1       = 0,
	.wSpeedsSupported       = 0x6, // FS | HS.
	.bFunctionalitySupport  = 1,   // FS and above.
	.bU1DevExitLat          = 0,
	.wU2DevExitLat          = 0
};

static u8 usb_lang_id_string_descriptor[4] =
{
	4, 3,
	0x09, 0x04
};

static u8 usb_serial_string_descriptor[26] =
{
	26, 0x03,
	'C', 0x00, '7', 0x00, 'C', 0x00, '0', 0x00,
	'9', 0x00, '2', 0x00, '4', 0x00, '2', 0x00, 'F', 0x00, '7', 0x00, '0', 0x00, '3', 0x00
};

static u8 usb_vendor_string_descriptor_ums[32] =
{
	26, 0x03,
	'N', 0, 'y', 0, 'x', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0,
	'D', 0, 'i', 0, 's', 0, 'k', 0
};

static u8 usb_product_string_descriptor_ums[22] =
{
	8, 0x03,
	'U', 0, 'M', 0, 'S', 0
};

static usb_ms_os_descr_t usb_ms_os_descriptor =
{
	.bLength         = 0x28,
	.bDescriptorType = 0x03,
	.wSignature[0]   = 'M',
	.wSignature[1]   = 'S',
	.wSignature[2]   = 'F',
	.wSignature[3]   = 'T',
	.wSignature[4]   = '1',
	.wSignature[5]   = '0',
	.wSignature[6]   = '0',
	.bVendorCode     = 0x99,
};

static usb_ms_cid_descr_t usb_ms_cid_descriptor =
{
	.dLength          = 0x28,
	.wVersion         = 0x100,
	.wCompatibilityId = USB_DESCRIPTOR_MS_COMPAT_ID,
	.bSections        = 1,
	.bInterfaceNumber = 0,
	.bReserved1       = 1,

	.bCompatibleId[0] = 'N',
	.bCompatibleId[1] = 'Y',
	.bCompatibleId[2] = 'X',
	.bCompatibleId[3] = 'U',
	.bCompatibleId[4] = 'S',
	.bCompatibleId[5] = 'B',
};

static usb_ms_ext_prop_descr_t usb_ms_ext_prop_descriptor_ums =
{
	.dLength             = 0x48,
	.wVersion            = 0x100,
	.wExtendedProperty   = USB_DESCRIPTOR_MS_EXTENDED_PROPERTIES,
	.wSections           = 1,

	.dPropertySize       = 0x3E,
	.dPropertyType       = 4, // DWORD

	.wPropertyNameLength = 0x2C,
	.wPropertyName[0]    = 'M', // MaximumTransferLength.
	.wPropertyName[1]    = 'a',
	.wPropertyName[2]    = 'x',
	.wPropertyName[3]    = 'i',
	.wPropertyName[4]    = 'm',
	.wPropertyName[5]    = 'u',
	.wPropertyName[6]    = 'm',
	.wPropertyName[7]    = 'T',
	.wPropertyName[8]    = 'r',
	.wPropertyName[9]    = 'a',
	.wPropertyName[10]   = 'n',
	.wPropertyName[11]   = 's',
	.wPropertyName[12]   = 'f',
	.wPropertyName[13]   = 'e',
	.wPropertyName[14]   = 'r',
	.wPropertyName[15]   = 'L',
	.wPropertyName[16]   = 'e',
	.wPropertyName[17]   = 'n',
	.wPropertyName[18]   = 'g',
	.wPropertyName[19]   = 't',
	.wPropertyName[20]   = 'h',
	.wPropertyName[21]   = 0,

	.dPropertyDataLength = 0x4,
	.wPropertyData[0]    = 0x00, // 1MB.
	.wPropertyData[1]    = 0x10,
};

static usb_ms_ext_prop_descr_t usb_ms_ext_prop_descriptor_hid =
{
	.dLength             = 7,
	.wVersion            = 0x100,
	.wExtendedProperty   = USB_DESCRIPTOR_MS_EXTENDED_PROPERTIES,
	.wSections           = 0,
};

static usb_dev_descr_t usb_device_descriptor_hid_jc =
{
	.bLength         = 18,
	.bDescriptorType = USB_DESCRIPTOR_DEVICE,
	.bcdUSB          = 0x210,
	.bDeviceClass    = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize  = 0x40,
	.idVendor        = 0x11EC, // Nintendo: 0x057E, Nvidia: 0x0955
	.idProduct       = 0xA7E1, // Switch:   0x2000, usbd:   0x3000
	.bcdDevice       = 0x0101,
	.iManufacturer   = 1,
	.iProduct        = 2,
	.iSerialNumber   = 3,
	.bNumConfigs     = 1
};

static usb_dev_descr_t usb_device_descriptor_hid_touch =
{
	.bLength         = 18,
	.bDescriptorType = USB_DESCRIPTOR_DEVICE,
	.bcdUSB          = 0x210,
	.bDeviceClass    = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize  = 0x40,
	.idVendor        = 0x11EC, // Nintendo: 0x057E, Nvidia: 0x0955
	.idProduct       = 0xA7E2, // Switch:   0x2000, usbd:   0x3000
	.bcdDevice       = 0x0101,
	.iManufacturer   = 1,
	.iProduct        = 2,
	.iSerialNumber   = 3,
	.bNumConfigs     = 1
};

u8 hid_report_descriptor_jc[] =
{
	0x05, 0x01,                // USAGE_PAGE (Generic Desktop),
	0x09, 0x04,                // USAGE (Joystick),
	0xa1, 0x01,                // COLLECTION (Application),
	0xa1, 0x02,                //   COLLECTION (Logical),
	0x75, 0x08,                //     REPORT_SIZE (8),
	0x95, 0x04,                //     REPORT_COUNT (4),
	0x15, 0x00,                //     LOGICAL_MINIMUM (0),
	0x26, 0xff, 0x00,          //     LOGICAL_MAXIMUM (255),
	0x35, 0x00,                //     PHYSICAL_MINIMUM (0),
	0x46, 0xff, 0x00,          //     PHYSICAL_MAXIMUM (255),
	0x09, 0x30,                //     USAGE (X_ID),
	0x09, 0x31,                //     USAGE (Y_ID),
	0x09, 0x32,                //     USAGE (Z_ID),
	0x09, 0x35,                //     USAGE (Rz_ID),
	0x81, 0x02,                //     INPUT (IOF_Variable),
	0x75, 0x04,                //     REPORT_SIZE (4),
	0x95, 0x01,                //     REPORT_COUNT (1),
	0x25, 0x07,                //     LOGICAL_MAXIMUM (7),
	0x46, 0x3b, 0x01,          //     PHYSICAL_MAXIMUM (315),
	0x65, 0x14,                //     UNIT (Eng_Rot_Angular_Pos),
	0x09, 0x39,                //     USAGE (Hat_Switch),
	0x81, 0x42,                //     INPUT (IOF_NullposVar),
	0x65, 0x00,                //     UNIT (Unit_None),
	0x75, 0x01,                //     REPORT_SIZE (1),
	0x95, 0x0c,                //     REPORT_COUNT (12),
	0x25, 0x01,                //     LOGICAL_MAXIMUM (1),
	0x45, 0x01,                //     PHYSICAL_MAXIMUM (1),
	0x05, 0x09,                //     USAGE_PAGE (Button_ID),
	0x19, 0x01,                //     USAGE_MINIMUM (1),
	0x29, 0x0c,                //     USAGE_MAXIMUM (12),
	0x81, 0x02,                //     INPUT (IOF_Variable),
	0xc0,                      //   END_COLLECTION(),
	0xc0                       // END_COLLECTION(),
};

u32 hid_report_descriptor_jc_size = sizeof(hid_report_descriptor_jc);

u8 hid_report_descriptor_touch[] =
{
	0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
	0x09, 0x05,                         // USAGE (Touch Pad)
	0xa1, 0x01,                         // COLLECTION (Application)
	0x85, 0x05,                         //   REPORT_ID (Touch pad)
	0x09, 0x22,                         //   USAGE (Finger)
	0xa1, 0x02,                         //   COLLECTION (Logical)
	0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
	0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
	0x09, 0x42,                         //     USAGE (Tip switch)
	0x95, 0x01,                         //     REPORT_COUNT (1)
	0x75, 0x01,                         //     REPORT_SIZE (1)
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)

	0x15, 0x00,                         //     LOGICAL_MINIMUM (1)
	0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
	0x75, 0x01,                         //     REPORT_SIZE (1)
	0x95, 0x07,                         //     REPORT_COUNT (7)
	0x09, 0x54,                         //     USAGE (Contact Count)
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)

	0x95, 0x01,                         //     REPORT_COUNT (1)
	0x75, 0x08,                         //     REPORT_SIZE (8)
	0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
	0x25, 0x0A,                         //     LOGICAL_MAXIMUM (10)
	0x09, 0x51,                         //     USAGE (Contact Identifier)
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)

	// 0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
	// 0x26, 0xF8, 0x2A,                   //     LOGICAL_MAXIMUM (11000)
	// 0x95, 0x01,                         //     REPORT_COUNT (1)
	// 0x75, 0x08,                         //     REPORT_SIZE (16)
	// 0x09, 0x30,                         //     USAGE (Pressure)
	// 0x81, 0x02,                         //     INPUT (Data,Var,Abs)

	0x05, 0x01,                         //     USAGE_PAGE (Generic Desk..
	0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x04,                   //     LOGICAL_MAXIMUM (1279)
	0x75, 0x10,                         //     REPORT_SIZE (16)
	0x55, 0x0e,                         //     UNIT_EXPONENT (-2)
	0x65, 0x13,                         //     UNIT(Inch,EngLinear)
	0x09, 0x30,                         //     USAGE (X)
	0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)
	0x46, 0xFF, 0x04,                   //     PHYSICAL_MAXIMUM (1279)
	0x95, 0x01,                         //     REPORT_COUNT (1)
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)
	0x26, 0xCF, 0x02,                   //     LOGICAL_MAXIMUM (719)
	0x46, 0xCF, 0x02,                   //     PHYSICAL_MAXIMUM (719)
	0x09, 0x31,                         //     USAGE (Y)
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)

	0x05, 0x0d,                         //     USAGE PAGE (Digitizers)
	0xc0,                               //    END_COLLECTION
	0xc0,                               // END_COLLECTION
};
u32 hid_report_descriptor_touch_size = sizeof(hid_report_descriptor_touch);

static usb_cfg_hid_descr_t usb_configuration_descriptor_hid_jc =
{
	/* Configuration descriptor structure */
	.config.bLength               = 9,
	.config.bDescriptorType       = USB_DESCRIPTOR_CONFIGURATION,
	.config.wTotalLength          = sizeof(usb_cfg_hid_descr_t),
	.config.bNumInterfaces        = 0x01,
	.config.bConfigurationValue   = 0x01,
	.config.iConfiguration        = 0x00,
	.config.bmAttributes          = USB_ATTR_SELF_POWERED | USB_ATTR_BUS_POWERED_RSVD,
	.config.bMaxPower             = 32 / 2,

	/* Interface descriptor structure */
	.interface.bLength            = 9,
	.interface.bDescriptorType    = USB_DESCRIPTOR_INTERFACE,
	.interface.bInterfaceNumber   = 0,
	.interface.bAlternateSetting  = 0,
	.interface.bNumEndpoints      = 2,
	.interface.bInterfaceClass    = 0x03, // Human Interface Device Class.
	.interface.bInterfaceSubClass = 0x00, // SCSI Transparent Command Set.
	.interface.bInterfaceProtocol = 0x00, // Bulk-Only Transport.
	.interface.iInterface         = 0x00,

	.hid.bLength                  = 9,
	.hid.bDescriptorType          = USB_DESCRIPTOR_HID,
	.hid.bcdHID                   = 0x110,
	.hid.bCountryCode             = 0,
	.hid.bNumDescriptors          = 1,
	.hid.bClassDescriptorType     = USB_DESCRIPTOR_HID_REPORT,
	.hid.bDescriptorLength        = sizeof(hid_report_descriptor_jc),

	/* Endpoint descriptor structure EP1 IN */
	.endpoint[0].bLength          = 7,
	.endpoint[0].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[0].bEndpointAddress = 0x81, // USB_EP_ADDR_BULK_IN.
	.endpoint[0].bmAttributes     = USB_EP_TYPE_INTR,
	.endpoint[0].wMaxPacketSize   = 0x200,
	.endpoint[0].bInterval        = 4,   // 8ms on HS.

	/* Endpoint descriptor structure EP1 OUT */
	.endpoint[1].bLength          = 7,
	.endpoint[1].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[1].bEndpointAddress = 0x01, // USB_EP_ADDR_BULK_OUT.
	.endpoint[1].bmAttributes     = USB_EP_TYPE_INTR,
	.endpoint[1].wMaxPacketSize   = 0x200,
	.endpoint[1].bInterval        = 4    // 8ms on HS.
};

static u8 usb_vendor_string_descriptor_hid[22] =
{
	16, 0x03,
	'N', 0, 'y', 0, 'x', 0, ' ', 0,
	'U', 0, 'S', 0, 'B', 0
};

static u8 usb_product_string_descriptor_hid_jc[24] =
{
	24, 0x03,
	'N', 0, 'y', 0, 'x', 0, ' ', 0,
	'J', 0, 'o', 0, 'y', 0, '-', 0, 'C', 0, 'o', 0, 'n', 0
};

static u8 usb_product_string_descriptor_hid_touch[26] =
{
	26, 0x03,
	'N', 0, 'y', 0, 'x', 0, ' ', 0,
	'T', 0, 'o', 0, 'u', 0, 'c', 0, 'h', 0, 'p', 0, 'a', 0, 'd', 0
};

static usb_cfg_hid_descr_t usb_configuration_descriptor_hid_touch =
{
	/* Configuration descriptor structure */
	.config.bLength               = 9,
	.config.bDescriptorType       = USB_DESCRIPTOR_CONFIGURATION,
	.config.wTotalLength          = sizeof(usb_cfg_hid_descr_t),
	.config.bNumInterfaces        = 0x01,
	.config.bConfigurationValue   = 0x01,
	.config.iConfiguration        = 0x00,
	.config.bmAttributes          = USB_ATTR_SELF_POWERED | USB_ATTR_BUS_POWERED_RSVD,
	.config.bMaxPower             = 32 / 2,

	/* Interface descriptor structure */
	.interface.bLength            = 9,
	.interface.bDescriptorType    = USB_DESCRIPTOR_INTERFACE,
	.interface.bInterfaceNumber   = 0,
	.interface.bAlternateSetting  = 0,
	.interface.bNumEndpoints      = 2,
	.interface.bInterfaceClass    = 0x03, // Human Interface Device Class.
	.interface.bInterfaceSubClass = 0x00, // SCSI Transparent Command Set.
	.interface.bInterfaceProtocol = 0x00, // Bulk-Only Transport.
	.interface.iInterface         = 0x00,

	.hid.bLength                  = 9,
	.hid.bDescriptorType          = USB_DESCRIPTOR_HID,
	.hid.bcdHID                   = 0x111,
	.hid.bCountryCode             = 0,
	.hid.bNumDescriptors          = 1,
	.hid.bClassDescriptorType     = USB_DESCRIPTOR_HID_REPORT,
	.hid.bDescriptorLength        = sizeof(hid_report_descriptor_touch),

	/* Endpoint descriptor structure EP1 IN */
	.endpoint[0].bLength          = 7,
	.endpoint[0].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[0].bEndpointAddress = 0x81, // USB_EP_ADDR_BULK_IN.
	.endpoint[0].bmAttributes     = USB_EP_TYPE_INTR,
	.endpoint[0].wMaxPacketSize   = 0x200,
	.endpoint[0].bInterval        = 3,   // 4ms on HS.

	/* Endpoint descriptor structure EP1 OUT */
	.endpoint[1].bLength          = 7,
	.endpoint[1].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[1].bEndpointAddress = 0x01, // USB_EP_ADDR_BULK_OUT.
	.endpoint[1].bmAttributes     = USB_EP_TYPE_INTR,
	.endpoint[1].wMaxPacketSize   = 0x200,
	.endpoint[1].bInterval        = 3    // 4ms on HS.
};

usb_desc_t usb_gadget_ums_descriptors =
{
	.dev       = &usb_device_descriptor_ums,
	.dev_qual  = &usb_device_qualifier_descriptor,
	.cfg       = &usb_configuration_descriptor_ums,
	.cfg_other = &usb_other_speed_config_descriptor_ums,
	.dev_bot   = &usb_device_binary_object_descriptor,
	.vendor    = usb_vendor_string_descriptor_ums,
	.product   = usb_product_string_descriptor_ums,
	.serial    = usb_serial_string_descriptor,
	.lang_id   = usb_lang_id_string_descriptor,
	.ms_os     = &usb_ms_os_descriptor,
	.ms_cid    = &usb_ms_cid_descriptor,
	.mx_ext    = &usb_ms_ext_prop_descriptor_ums
};

usb_desc_t usb_gadget_hid_jc_descriptors =
{
	.dev       = &usb_device_descriptor_hid_jc,
	.dev_qual  = &usb_device_qualifier_descriptor,
	.cfg       = (usb_cfg_simple_descr_t *)&usb_configuration_descriptor_hid_jc,
	.cfg_other = NULL,
	.dev_bot   = &usb_device_binary_object_descriptor,
	.vendor    = usb_vendor_string_descriptor_hid,
	.product   = usb_product_string_descriptor_hid_jc,
	.serial    = usb_serial_string_descriptor,
	.lang_id   = usb_lang_id_string_descriptor,
	.ms_os     = &usb_ms_os_descriptor,
	.ms_cid    = &usb_ms_cid_descriptor,
	.mx_ext    = &usb_ms_ext_prop_descriptor_hid
};

usb_desc_t usb_gadget_hid_touch_descriptors =
{
	.dev       = &usb_device_descriptor_hid_touch,
	.dev_qual  = &usb_device_qualifier_descriptor,
	.cfg       = (usb_cfg_simple_descr_t *)&usb_configuration_descriptor_hid_touch,
	.cfg_other = NULL,
	.dev_bot   = &usb_device_binary_object_descriptor,
	.vendor    = usb_vendor_string_descriptor_hid,
	.product   = usb_product_string_descriptor_hid_touch,
	.serial    = usb_serial_string_descriptor,
	.lang_id   = usb_lang_id_string_descriptor,
	.ms_os     = &usb_ms_os_descriptor,
	.ms_cid    = &usb_ms_cid_descriptor,
	.mx_ext    = &usb_ms_ext_prop_descriptor_hid
};
