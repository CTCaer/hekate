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

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#include <utils/types.h>

typedef enum {
	USB_DESCRIPTOR_DEVICE                    = 1,
	USB_DESCRIPTOR_CONFIGURATION             = 2,
	USB_DESCRIPTOR_STRING                    = 3,
	USB_DESCRIPTOR_INTERFACE                 = 4,
	USB_DESCRIPTOR_ENDPOINT                  = 5,
	USB_DESCRIPTOR_DEVICE_QUALIFIER          = 6,
	USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION = 7,
	USB_DESCRIPTOR_INTERFACE_POWER           = 8,
	USB_DESCRIPTOR_INTERFACE_OTG             = 9,
	USB_DESCRIPTOR_DEVICE_BINARY_OBJECT      = 15,
	USB_DESCRIPTOR_DEVICE_BINARY_OBJECT_CAP  = 16,
	USB_DESCRIPTOR_HID                       = 33,
	USB_DESCRIPTOR_HID_REPORT                = 34
} usb_desc_type_t;

typedef enum {
	USB_DESCRIPTOR_MS_COMPAT_ID           = 4,
	USB_DESCRIPTOR_MS_EXTENDED_PROPERTIES = 5
} usb_vendor_desc_type_t;

typedef enum {
	USB_ATTR_REMOTE_WAKE_UP   = 0x20,
	USB_ATTR_SELF_POWERED     = 0x40,
	USB_ATTR_BUS_POWERED_RSVD = 0x80
} usb_cfg_attr_type_t;

typedef enum
{
	USB_EP_TYPE_CTRL = 0,
	USB_EP_TYPE_ISO  = 1,
	USB_EP_TYPE_BULK = 2,
	USB_EP_TYPE_INTR = 3
} usb_cfg_ep_type_t;

/* Device descriptor structure */
typedef struct _usb_dev_descr_t
{
	u8  bLength;         // Size of this descriptor in bytes.
	u8  bDescriptorType; // Device Descriptor Type. (USB_DESCRIPTOR_DEVICE)
	u16 bcdUSB;          // USB Spec. Release number (2.1).
	u8  bDeviceClass;    // Class is specified in the interface descriptor.
	u8  bDeviceSubClass; // SubClass is specified in the interface descriptor.
	u8  bDeviceProtocol; // Protocol is specified in the interface descriptor.
	u8  bMaxPacketSize;  // Maximum packet size for EP0.
	u16 idVendor;        // Vendor ID assigned by USB forum.
	u16 idProduct;       // Product ID assigned by Organization.
	u16 bcdDevice;       // Device Release number in BCD.
	u8  iManufacturer;   // Index of String descriptor describing Manufacturer.
	u8  iProduct;        // Index of String descriptor describing Product.
	u8  iSerialNumber;   // Index of String descriptor describing Serial number.
	u8  bNumConfigs;     // Number of possible configuration.
} __attribute__((packed)) usb_dev_descr_t;

/* Device Qualigier descriptor structure */
typedef struct _usb_dev_qual_descr_t
{
	u8  bLength;          // Size of this descriptor in bytes.
	u8  bDescriptorType;  // Device Descriptor Type. (USB_DESCRIPTOR_DEVICE_QUALIFIER)
	u16 bcdUSB;           // USB Spec. Release number (2.1).
	u8  bDeviceClass;     // Class is specified in the interface descriptor.
	u8  bDeviceSubClass;  // SubClass is specified in the interface descriptor.
	u8  bDeviceProtocol;  // Protocol is specified in the interface descriptor.
	u8  bMaxPacketSize;   // Maximum packet size for EP0.
	u8  bNumOtherConfigs; // Number of possible other-speed configurations.
	u8  bReserved;        // Reserved for future use, must be zero
} __attribute__((packed)) usb_dev_qual_descr_t;

/* Configuration descriptor structure */
typedef struct _usb_cfg_descr_t
{
	u8  bLength;             // Length of this descriptor.
	u8  bDescriptorType;     // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
	u16 wTotalLength;        // Total length of all descriptors for this configuration.
	u8  bNumInterfaces;      // Number of interfaces in this configuration.
	u8  bConfigurationValue; // Value of this configuration (1 based).
	u8  iConfiguration;      // Index of String Descriptor describing the configuration.
	u8  bmAttributes;        // Configuration characteristics.
	u8  bMaxPower;           // Maximum power consumed by this configuration.
} __attribute__((packed)) usb_cfg_descr_t;

/* Interface descriptor structure */
typedef struct _usb_inter_descr_t
{
	u8 bLength;            // Length of this descriptor.
	u8 bDescriptorType;    // INTERFACE descriptor type (USB_DESCRIPTOR_INTERFACE).
	u8 bInterfaceNumber;   // Number of this interface (0 based).
	u8 bAlternateSetting;  // Value of this alternate interface setting.
	u8 bNumEndpoints;      // Number of endpoints in this interface.
	u8 bInterfaceClass;    // Class code (assigned by the USB-IF).
	u8 bInterfaceSubClass; // Subclass code (assigned by the USB-IF).
	u8 bInterfaceProtocol; // Protocol code (assigned by the USB-IF).
	u8 iInterface;         // Index of String Descriptor describing the interface.
} __attribute__((packed)) usb_inter_descr_t;

/* HID descriptor structure */
typedef struct _usb_hid_descr_t
{
	u8  bLength;              // Length of this descriptor.
	u8  bDescriptorType;      // INTERFACE descriptor type (USB_DESCRIPTOR_HID).
	u16 bcdHID;               // HID class specification release
	u8  bCountryCode;         // Country code.
	u8  bNumDescriptors;      // Number of descriptors.
	u8  bClassDescriptorType; // Type of class descriptor (USB_DESCRIPTOR_HID_REPORT).
	u16 bDescriptorLength;    // Report descriptor length.
} __attribute__((packed)) usb_hid_descr_t;

/* Endpoint descriptor structure */
typedef struct _usb_ep_descr_t
{
	u8  bLength;          // Length of this descriptor.
	u8  bDescriptorType;  // ENDPOINT descriptor type (USB_DESCRIPTOR_ENDPOINT).
	u8  bEndpointAddress; // Endpoint address. bit7 indicates direction (0=OUT, 1=IN).
	u8  bmAttributes;     // Endpoint transfer type.
	u16 wMaxPacketSize;   // Maximum packet size.
	u8  bInterval;        // Polling interval in frames. For Interrupt and Isochronous data transfer only.
} __attribute__((packed)) usb_ep_descr_t;

typedef struct _usb_cfg_simple_descr_t
{
	usb_cfg_descr_t   config;
	usb_inter_descr_t interface;
	usb_ep_descr_t    endpoint[2];
} __attribute__((packed)) usb_cfg_simple_descr_t;

typedef struct _usb_cfg_hid_descr_t
{
	usb_cfg_descr_t   config;
	usb_inter_descr_t interface;
	usb_hid_descr_t   hid;
	usb_ep_descr_t    endpoint[2];
} __attribute__((packed)) usb_cfg_hid_descr_t;

typedef struct _usb_dev_bot_t
{
	u8  bLength;                // Size of this descriptor in bytes.
	u8  bDescriptorType;        // Device Descriptor Type. (USB_DESCRIPTOR_DEVICE_BINARY_OBJECT)
	u16 wTotalLength;           // Size of this descriptor in bytes.
	u8  bNumDeviceCaps;         // Number of device capabilities in this descriptor.

	/* Device Capability USB 2.0 Extension Descriptor */
	/* Needed for a USB2.10 device. */
	u8  bLengthCap0;            // Size of this capability descriptor in bytes.
	u8  bDescriptorTypeCap0;    // Device Capability Descriptor Type. (USB_DESCRIPTOR_DEVICE_BINARY_OBJECT_CAP)
	u8  bDevCapabilityTypeCap0; // USB2: 2.
	u32 bmAttributesCap0;       // bit1: Link Power Management (LPM).

	u8  bLengthCap1;            // Size of this capability descriptor in bytes.
	u8  bDescriptorTypeCap1;    // Device Capability Descriptor Type. (USB_DESCRIPTOR_DEVICE_BINARY_OBJECT_CAP)
	u8  bDevCapabilityTypeCap1; // USB3: 3.
	u8  bmAttributesCap1;       // bit1: Latency Tolerance Messaging (LTM).
	u16 wSpeedsSupported;       // Supported bus speeds. 1: Low Speed, 2: Full Speed, 4: High Speed, 8: Super Speed.
	u8  bFunctionalitySupport;  // Lowest speed at which all the functionality is available. 1: Full speed and above.
	u8  bU1DevExitLat;          // USB3.0 U1 exit latency.
	u16 wU2DevExitLat;          // USB3.0 U2 exit latency.

} __attribute__((packed)) usb_dev_bot_t;

/* Microsoft OS String descriptor structure */
typedef struct _usb_ms_os_descr_t
{
	u8 bLength; // 0x12
	u8 bDescriptorType; // 3
	u16 wSignature[7]; // "MSFT100" UTF16 LE
	u8 bVendorCode; //
	u8 bPadding;
} __attribute__((packed)) usb_ms_os_descr_t;

/* Microsoft Compatible ID Feature descriptor structure */
typedef struct _usb_ms_cid_descr_t
{
	u32 dLength;
	u16 wVersion;
	u16 wCompatibilityId;
	u8 bSections;
	u8 bReserved0[7];
	u8 bInterfaceNumber;
	u8 bReserved1;
	u8 bCompatibleId[8];
	u8 bSubCompatibleId[8];
	u8 bReserved2[6];
} __attribute__((packed)) usb_ms_cid_descr_t;

/* Microsoft Extended Properties Feature descriptor structure */
typedef struct _usb_ms_ext_prop_descr_t
{
	u32 dLength;
	u16 wVersion;
	u16 wExtendedProperty;
	u16 wSections;
	u32 dPropertySize;
	u32 dPropertyType;
	u16 wPropertyNameLength;
	u16 wPropertyName[22];   // UTF16 LE
	u32 dPropertyDataLength;
	u16 wPropertyData[2];    // UTF16 LE
} __attribute__((packed)) usb_ms_ext_prop_descr_t;

typedef struct _usb_desc_t
{
	usb_dev_descr_t *dev;
	usb_dev_qual_descr_t *dev_qual;
	usb_cfg_simple_descr_t *cfg;
	usb_cfg_simple_descr_t *cfg_other;
	usb_dev_bot_t *dev_bot;
	u8 *vendor;
	u8 *product;
	usb_ms_os_descr_t *ms_os;
	usb_ms_cid_descr_t *ms_cid;
	usb_ms_ext_prop_descr_t *mx_ext;
} usb_desc_t;

usb_dev_descr_t usb_device_descriptor_ums =
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

usb_dev_qual_descr_t usb_device_qualifier_descriptor =
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

usb_cfg_simple_descr_t usb_configuration_descriptor_ums =
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

usb_cfg_simple_descr_t usb_other_speed_config_descriptor_ums =
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

usb_dev_bot_t usb_device_binary_object_descriptor =
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

u8 usb_vendor_string_descriptor_ums[32] = 
{
	26, 0x03,
	'N', 0, 'y', 0, 'x', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0,
	'D', 0, 'i', 0, 's', 0, 'k', 0
};

u8 usb_product_string_descriptor_ums[22] =
{
	8, 0x03,
	'U', 0, 'M', 0, 'S', 0
};

usb_ms_os_descr_t usb_ms_os_descriptor =
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

usb_ms_cid_descr_t usb_ms_cid_descriptor =
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

usb_ms_ext_prop_descr_t usb_ms_ext_prop_descriptor_ums =
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

usb_ms_ext_prop_descr_t usb_ms_ext_prop_descriptor_hid =
{
	.dLength             = 7,
	.wVersion            = 0x100,
	.wExtendedProperty   = USB_DESCRIPTOR_MS_EXTENDED_PROPERTIES,
	.wSections           = 0,
};

usb_dev_descr_t usb_device_descriptor_hid_jc =
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

usb_dev_descr_t usb_device_descriptor_hid_touch =
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

usb_cfg_hid_descr_t usb_configuration_descriptor_hid_jc =
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
	.hid.bDescriptorLength        = 0x43,

	/* Endpoint descriptor structure EP1 IN */
	.endpoint[0].bLength          = 7,
	.endpoint[0].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[0].bEndpointAddress = 0x81, // USB_EP_ADDR_BULK_IN.
	.endpoint[0].bmAttributes     = USB_EP_TYPE_INTR,
	.endpoint[0].wMaxPacketSize   = 0x200,
	.endpoint[0].bInterval        = 4, // 4ms on FS, 8ms on HS.

	/* Endpoint descriptor structure EP1 OUT */
	.endpoint[1].bLength          = 7,
	.endpoint[1].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[1].bEndpointAddress = 0x01, // USB_EP_ADDR_BULK_OUT.
	.endpoint[1].bmAttributes     = USB_EP_TYPE_INTR,
	.endpoint[1].wMaxPacketSize   = 0x200,
	.endpoint[1].bInterval        = 4 // 4ms on FS, 8ms on HS.
};

u8 usb_vendor_string_descriptor_hid[22] =
{
	16, 0x03,
	'N', 0, 'y', 0, 'x', 0, ' ', 0,
	'U', 0, 'S', 0, 'B', 0
};

u8 usb_product_string_descriptor_hid_jc[24] =
{
	24, 0x03,
	'N', 0, 'y', 0, 'x', 0, ' ', 0,
	'J', 0, 'o', 0, 'y', 0, '-', 0, 'C', 0, 'o', 0, 'n', 0
};

u8 usb_product_string_descriptor_hid_touch[26] =
{
	26, 0x03,
	'N', 0, 'y', 0, 'x', 0, ' ', 0,
	'T', 0, 'o', 0, 'u', 0, 'c', 0, 'h', 0, 'p', 0, 'a', 0, 'd', 0
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

usb_cfg_hid_descr_t usb_configuration_descriptor_hid_touch =
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
	.endpoint[0].bInterval        = 4, // 4ms on FS, 8ms on HS.

	/* Endpoint descriptor structure EP1 OUT */
	.endpoint[1].bLength          = 7,
	.endpoint[1].bDescriptorType  = USB_DESCRIPTOR_ENDPOINT,
	.endpoint[1].bEndpointAddress = 0x01, // USB_EP_ADDR_BULK_OUT.
	.endpoint[1].bmAttributes     = USB_EP_TYPE_INTR,
	.endpoint[1].wMaxPacketSize   = 0x200,
	.endpoint[1].bInterval        = 4 // 4ms on FS, 8ms on HS.
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
	.ms_os     = &usb_ms_os_descriptor,
	.ms_cid    = &usb_ms_cid_descriptor,
	.mx_ext    = &usb_ms_ext_prop_descriptor_hid
};

#endif
