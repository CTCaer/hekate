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

#ifndef _USB_DESCRIPTORS_TYPES_H_
#define _USB_DESCRIPTORS_TYPES_H_

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

/* Device Qualifier descriptor structure */
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
	u8 *serial;
	u8 *lang_id;
	usb_ms_os_descr_t *ms_os;
	usb_ms_cid_descr_t *ms_cid;
	usb_ms_ext_prop_descr_t *mx_ext;
} usb_desc_t;

#endif
