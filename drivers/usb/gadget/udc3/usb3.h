/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __USB3_H__
#define __USB3_H__

#include "asm/types.h"
#include "types.h"


#define STRING_LANGUAGE 0
#define STRING_MANUFACTURER 1
#define STRING_PRODUCT 2

#define USB_SPEED_UNKNOWN 0x00
#define USB_SPEED_LOW 0x01
#define USB_SPEED_FULL 0x02
#define USB_SPEED_HIGH 0x03
#define USB_SPEED_VARIABLE 0x04
#define USB_SPEED_SUPER 0x05

#define UDESC_DEVICE 0x01
#define UDESC_CONFIG 0x02
#define UDESC_STRING 0x03
#define UDESC_INTERFACE 0x04
#define UDESC_ENDPOINT 0x05
#define UDESC_SS_USB_COMPANION 0x30
#define UDESC_DEVICE_QUALIFIER 0x06
#define UDESC_BOS 0x0f
#define UDESC_DEVICE_CAPABILITY 0x10

#define ut_get_dir(a) ((a)&0x80)
#define UT_WRITE 0x00
#define UT_READ 0x80

#define ut_get_type(a) ((a)&0x60)
#define UT_STANDARD 0x00
#define UT_CLASS 0x20
#define UT_VENDOR 0x40

#define ut_get_recipient(a) ((a)&0x1f)
#define UT_DEVICE 0x00
#define UT_INTERFACE 0x01
#define UT_ENDPOINT 0x02
#define UT_OTHER 0x03

#define UR_GET_STATUS 0x00
#define UR_CLEAR_FEATURE 0x01
#define UR_SET_FEATURE 0x03
#define UR_SET_ADDRESS 0x05
#define UR_GET_DESCRIPTOR 0x06
#define UR_SET_DESCRIPTOR 0x07
#define UR_GET_CONFIG 0x08
#define UR_SET_CONFIG 0x09
#define UR_GET_INTERFACE 0x0a
#define UR_SET_INTERFACE 0x0b
#define UR_SYNCH_FRAME 0x0c
#define UR_SET_SEL 0x30
#define UR_SET_ISOC_DELAY 0x31

/* Feature numbers */
#define UF_ENDPOINT_HALT 0
#define UF_DEVICE_REMOTE_WAKEUP 1
#define UF_TEST_MODE 2
#define UF_DEVICE_B_HNP_ENABLE 3
#define UF_DEVICE_A_HNP_SUPPORT 4
#define UF_DEVICE_A_ALT_HNP_SUPPORT 5
#define UF_FUNCTION_SUSPEND 0
#define UF_U1_ENABLE 48
#define UF_U2_ENABLE 49
#define UF_LTM_ENABLE 50

/* OTG feature selectors */
#define UOTG_B_HNP_ENABLE 3
#define UOTG_A_HNP_SUPPORT 4
#define UOTG_A_ALT_HNP_SUPPORT 5
#define UOTG_NTF_HOST_REL 51
#define UOTG_B3_RSP_ENABLE 52

#define ue_get_dir(a) ((a)&0x80)
#define ue_set_dir(a, d) ((a) | (((d)&1) << 7))
#define UE_DIR_IN 0x80
#define UE_DIR_OUT 0x00
#define UE_ADDR 0x0f
#define ue_get_addr(a) ((a)&UE_ADDR)

/* * Maxpacket size for EP0, defined by USB3 spec */
#define USB3_MAX_EP0_SIZE 512

/* * Maxpacket size for any EP, defined by USB3 spec */
#define USB3_MAX_PACKET_SIZE 1024
#define USB2_HS_MAX_PACKET_SIZE 512
#define USB2_FS_MAX_PACKET_SIZE 64

#define USB3_BULK_IN_EP 1
#define USB3_BULK_OUT_EP 1

typedef struct usb_device_request {
	unsigned char bm_request_type;
	unsigned char b_request;
	unsigned short w_value;
	unsigned short w_index;
	unsigned short w_length;
} usb_device_request_t;

#pragma pack(1)
/* * USB_DT_DEVICE: Device descriptor */
typedef struct usb_device_descriptor {
	unsigned char b_length;
	unsigned char b_descriptor_type;

	unsigned short bcd_usb;
#define USB_CLASS_COMM 0x02
#define USB_CLASS_VENDOR_SPEC 0xFF
#define USB_SC_VENDOR_SPEC 0xFF
#define USB_PR_VENDOR_SPEC 0xFF
	unsigned char b_device_class;
	unsigned char b_device_sub_class;
	unsigned char b_device_protocol;
	unsigned char b_max_packet_size0;
	unsigned short id_vendor;
	unsigned short id_product;
	unsigned short bcd_device;
	unsigned char i_manufacturer;
	unsigned char i_product;
	unsigned char i_serial_number;
	unsigned char b_num_configurations;
} usb_device_descriptor_t;

/* USB_DT_CONFIG: Config descriptor */
typedef struct usb_config_descriptor {
	unsigned char b_length;
	unsigned char b_descriptor_type;

	unsigned short w_total_length;
	unsigned char b_num_interfaces;
#define CONFIG_VALUE 1
	unsigned char b_configuration_value;
	unsigned char i_configuration;
#define USB_CONFIG_ATT_ONE (1 << 7)
	unsigned char bm_attributes;
#define USB_CONFIG_VBUS_DRAW (0xFA)
	unsigned char b_max_power;
} usb_config_descriptor_t;

/* USB_DT_DEVICE_QUALIFIER: Device Qualifier descriptor */
typedef struct usb_qualifier_descriptor {
	unsigned char b_length;
	unsigned char b_descriptor_type;

	unsigned short bcd_usb;
	unsigned char b_device_class;
	unsigned char b_device_sub_class;
	unsigned char b_device_protocol;
	unsigned char b_max_packet_size0;
	unsigned char b_num_configurations;
	unsigned char b_reserved;
} usb_qualifier_descriptor_t;

/* USB_DT_INTERFACE: Interface descriptor */
typedef struct usb_interface_descriptor {
	unsigned char b_length;
	unsigned char b_descriptor_type;

	unsigned char b_interface_number;
	unsigned char b_alternate_setting;
	unsigned char b_num_endpoints;
	unsigned char b_interface_class;
	unsigned char b_interface_subclass;
	unsigned char b_interface_protocol;
	unsigned char i_interface;
} usb_interface_descriptor_t;

/* USB_DT_ENDPOINT: Endpoint descriptor */
typedef struct usb_endpoint_descriptor {
	unsigned char b_length;
	unsigned char b_descriptor_type;

	unsigned char b_endpoint_address;
	unsigned char bm_attributes;
#define USB_ENDPOINT_XFER_CONTROL 0x00
#define USB_ENDPOINT_XFER_ISOC 0x01
#define USB_ENDPOINT_XFER_BULK 0x02
#define USB_ENDPOINT_XFER_INT 0x03
	unsigned short w_max_packet_size;
	unsigned char b_interval;
} usb_endpoint_descriptor_t;

/* USB_DT_SS_ENDPOINT_COMP: SuperSpeed Endpoint Companion descriptor */
typedef struct usb_ss_ep_comp_descriptor {
	unsigned char b_length;
	unsigned char b_descriptor_type;

	unsigned char b_max_burst;
	unsigned char bm_attributes;
	unsigned short w_bytes_per_interval;
} usb_ss_ep_comp_descriptor_t;

/* WUSB BOS Descriptor (Binary device Object Store) */
typedef struct wusb_bos_desc {
	unsigned char b_length;
	unsigned char b_descriptor_type;
	unsigned short w_total_length;
	unsigned char b_num_device_caps;
} wusb_bos_desc_t;

#define USB_DEVICE_CAPABILITY_20_EXTENSION 0x02
typedef struct usb_dev_cap_20_ext_desc {
	unsigned char b_length;
	unsigned char b_descriptor_type;
	unsigned char b_devcapability_type;
#define USB_20_EXT_LPM 0x02
	unsigned int bm_attributes;
} usb_dev_cap_20_ext_desc_t;

#define USB_DEVICE_CAPABILITY_SS_USB 0x03
typedef struct usb_dev_cap_ss_usb {
	unsigned char b_length;
	unsigned char b_descriptor_type;
	unsigned char b_devcapability_type;
#define USB_DC_SS_USB_LTM_CAPABLE 0x02
	unsigned char bm_attributes;
#define USB_DC_SS_USB_SPEED_SUPPORT_LOW 0x01
#define USB_DC_SS_USB_SPEED_SUPPORT_FULL 0x02
#define USB_DC_SS_USB_SPEED_SUPPORT_HIGH 0x04
#define USB_DC_SS_USB_SPEED_SUPPORT_SS 0x08
	unsigned int w_speeds_supported;
	unsigned char b_functionality_support;
	unsigned char b_u1dev_exit_lat;
	unsigned int w_u2dev_exit_lat;
} usb_dev_cap_ss_usb_t;

#define USB_DEVICE_CAPABILITY_CONTAINER_ID 0x04
typedef struct usb_dev_cap_container_id {
	unsigned char b_length;
	unsigned char b_descriptor_type;
	unsigned char b_devcapability_type;
	unsigned char b_reserved;
	unsigned char container_id[16];
} usb_dev_cap_container_id_t;
#pragma pack()

#endif /* __USB3_H__ */
