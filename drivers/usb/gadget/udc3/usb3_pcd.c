/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include "usb3.h"
#include "usb3_hw.h"
#include "usb3_drv.h"
#include "usb3_prot.h"
#include "usb3_pcd.h"
#include <common.h>
#include <linux/ctype.h>
#include <linux/string.h>

#define BUFFER_SIZE 512
extern void usb3_bulk_out_transfer(void *dev);

const struct usb_interface_descriptor interface = {
	sizeof(struct usb_interface_descriptor), /* b_length */
	UDESC_INTERFACE,						/* b_descriptor_type */
	0,									/* b_interface_number */
	0,									/* b_alternate_setting */
	2,									/* b_num_endpoints */
	USB_CLASS_VENDOR_SPEC,				/* b_interface_class */
	USB_SC_VENDOR_SPEC,					/* b_interface_subclass */
	USB_PR_VENDOR_SPEC,					/* b_interface_protocol */
	0,									/* i_interface */
};

/* Two endpoint descriptors: bulk-in, bulk-out. */

const struct usb_ss_ep_comp_descriptor ep_comp = {
	sizeof(struct usb_ss_ep_comp_descriptor), /* b_length */
	UDESC_SS_USB_COMPANION,				/* b_descriptor_type */
	0,										/* b_max_burst */
	0,										/* bm_attributes */
	0,										/* w_bytes_per_interval */
};

const struct usb_endpoint_descriptor hs_bulk_in = {
	sizeof(struct usb_endpoint_descriptor), /* b_length */
	UDESC_ENDPOINT,						/* b_descriptor_type */
	UE_DIR_IN | USB3_BULK_IN_EP,			/* b_endpoint_address */
	USB_ENDPOINT_XFER_BULK,				/* bm_attributes */
	0x200,								/* w_max_packet_size: 512 of high-speed */
	0,									/* b_interval */
};

const struct usb_endpoint_descriptor hs_bulk_out = {
	sizeof(struct usb_endpoint_descriptor), /* b_length */
	UDESC_ENDPOINT,						/* b_descriptor_type */
	UE_DIR_OUT | USB3_BULK_OUT_EP,		/* b_endpoint_address */
	USB_ENDPOINT_XFER_BULK,				/* bm_attributes */
	0x200,								/* w_max_packet_size: 512 of high-speed */
	1,									/* b_interval */
};

const struct usb_endpoint_descriptor ss_bulk_in = {
	sizeof(struct usb_endpoint_descriptor), /* b_length */
	UDESC_ENDPOINT,						/* b_descriptor_type */
	UE_DIR_IN | USB3_BULK_IN_EP,			/* b_endpoint_address */
	USB_ENDPOINT_XFER_BULK,				/* bm_attributes */
	0x400,								/* w_max_packet_size: 1024 of super-speed */
	0,									/* b_interval */
};

const struct usb_endpoint_descriptor ss_bulk_out = {
	sizeof(struct usb_endpoint_descriptor), /* b_length */
	UDESC_ENDPOINT,						/* b_descriptor_type */
	UE_DIR_OUT | USB3_BULK_OUT_EP,		/* b_endpoint_address */
	USB_ENDPOINT_XFER_BULK,				/* bm_attributes */
	0x400,								/* w_max_packet_size: 1024 of super-speed */
	0,									/* b_interval */
};

/* * The BOS Descriptor */

const struct usb_dev_cap_20_ext_desc capability1 = {
	sizeof(struct usb_dev_cap_20_ext_desc), /* b_length */
	UDESC_DEVICE_CAPABILITY,				/* b_descriptor_type */
	USB_DEVICE_CAPABILITY_20_EXTENSION,	/* b_devcapability_type */
	0x2,									/* bm_attributes */
};

const struct usb_dev_cap_ss_usb capability2 = {
	sizeof(struct usb_dev_cap_ss_usb),									/* b_length */
	UDESC_DEVICE_CAPABILITY,											/* b_descriptor_type */
	USB_DEVICE_CAPABILITY_SS_USB,										/* b_devcapability_type */
	0x0,																/* bm_attributes */
	(USB_DC_SS_USB_SPEED_SUPPORT_SS | USB_DC_SS_USB_SPEED_SUPPORT_HIGH), /* w_speeds_supported */
	0x2,																/* b_functionality_support */
	/* @todo set these to correct value */
	0xa,   /* b_u1dev_exit_lat */
	0x100, /* w_u2dev_exit_lat */
};

const struct usb_dev_cap_container_id capability3 = {
	sizeof(struct usb_dev_cap_container_id), /* b_length */
	UDESC_DEVICE_CAPABILITY,				/* b_descriptor_type */
	USB_DEVICE_CAPABILITY_CONTAINER_ID,	/* b_devcapability_type */
	0,									/* b_reserved */
	/* @todo Create UUID */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* container_id */
};

const struct wusb_bos_desc usb_bos = {
	sizeof(struct wusb_bos_desc), /* b_length */
	UDESC_BOS,					/* b_descriptor_type */
	(sizeof(struct wusb_bos_desc) /* w_total_length */
	+ sizeof(capability1) + sizeof(capability2) + sizeof(capability3)),
	3, /* b_num_device_caps */
};

usb3_pcd_ep_t *usb3_get_out_ep(usb3_pcd_t *pcd, unsigned int ep_num)
{
	if (ep_num == 0) {
		return &pcd->ep0;
	} else {
		return &pcd->out_ep;
	}
}

usb3_pcd_ep_t *usb3_get_in_ep(usb3_pcd_t *pcd, unsigned int ep_num)
{
	if (ep_num == 0) {
		return &pcd->ep0;
	} else {
		return &pcd->in_ep;
	}
}

usb3_pcd_ep_t *usb3_get_ep_by_addr(usb3_pcd_t *pcd, unsigned short index)
{
	unsigned int ep_num = ue_get_addr(index);

	if (ue_get_dir(index) == UE_DIR_IN) {
		return usb3_get_in_ep(pcd, ep_num);
	} else {
		return usb3_get_out_ep(pcd, ep_num);
	}
}


void usb3_ep_clear_stall(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep)
{
	usb3_dev_ep_regs_t *ep_reg;

	if (ep->is_in) {
		ep_reg = ep->in_ep_reg;
	} else {
		ep_reg = ep->out_ep_reg;
	}

	usb3_dep_cstall(pcd, ep_reg);
}


unsigned int usb3_get_device_speed(usb3_pcd_t *pcd)
{
	unsigned int dsts;
	unsigned int speed = USB_SPEED_UNKNOWN;

	dsts = usb3_rd32(&pcd->dev_global_regs->dsts);

	switch ((dsts >> USB3_DSTS_CONNSPD_SHIFT) & (USB3_DSTS_CONNSPD_BITS >> USB3_DSTS_CONNSPD_SHIFT)) {
	case USB3_SPEED_HS_PHY_30MHZ_OR_60MHZ:
		speed = USB_SPEED_HIGH;
		break;

	case USB3_SPEED_FS_PHY_30MHZ_OR_60MHZ:
	case USB3_SPEED_FS_PHY_48MHZ:
		speed = USB_SPEED_FULL;
		break;

	case USB3_SPEED_LS_PHY_6MHZ:
		speed = USB_SPEED_LOW;
		break;

	case USB3_SPEED_SS_PHY_125MHZ_OR_250MHZ:
		speed = USB_SPEED_SUPER;
		break;
	default:
		break;
	}

	return speed;
}

void usb3_pcd_set_speed(usb3_pcd_t *pcd, int speed)
{
	/* Set the MPS of EP0 based on the connection speed */
	switch (speed) {
	case USB_SPEED_SUPER:
		pcd->ep0.maxpacket = 512; /* 512: 512bytes */
		pcd->in_ep.maxpacket = USB3_MAX_PACKET_SIZE;
		pcd->out_ep.maxpacket = USB3_MAX_PACKET_SIZE;
		break;

	case USB_SPEED_HIGH:
		pcd->ep0.maxpacket = 64; /* 64: 64bytes */
		pcd->in_ep.maxpacket = USB2_HS_MAX_PACKET_SIZE;
		pcd->out_ep.maxpacket = USB2_HS_MAX_PACKET_SIZE;
		break;

	case USB_SPEED_FULL:
		pcd->ep0.maxpacket = 64; /* 64: 64bytes */
		pcd->in_ep.maxpacket = USB2_FS_MAX_PACKET_SIZE;
		pcd->out_ep.maxpacket = USB2_FS_MAX_PACKET_SIZE;
		break;

	case USB_SPEED_LOW:
		pcd->ep0.maxpacket = 8; /* 8: 8bytes */
		pcd->in_ep.maxpacket = 0;
		pcd->out_ep.maxpacket = 0;
		break;
	default:
		break;
	}
}

void usb3_ep_set_stall(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep)
{
	usb3_dev_ep_regs_t *ep_reg;

	if (ep->is_in) {
		ep_reg = ep->in_ep_reg;
	} else {
		ep_reg = ep->out_ep_reg;
	}

	usb3_dep_sstall(pcd, ep_reg);
}

static void ep0_do_stall(usb3_pcd_t *pcd)
{
	usb3_pcd_ep_t *ep0 = &pcd->ep0;

	/* Stall EP0 IN & OUT simultanelusly */
	ep0->is_in = 1;
	usb3_ep_set_stall(pcd, ep0);
	ep0->is_in = 0;
	usb3_ep_set_stall(pcd, ep0);

	/* Prepare for the next setup transfer */
	ep0->stopped = 1;
	pcd->ep0state = EP0_IDLE;
	usb3_ep0_out_start(pcd);
}

static void do_clear_halt(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep)
{
	usb3_ep_clear_stall(pcd, ep);

	if (ep->stopped)
		ep->stopped = 0;
}

void usb3_pcd_ep_enable(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep)
{
	/*
	* Activate the EP
	*/
	ep->stopped = 0;
	ep->xfer_started = 0;

	/* Set initial data PID. */
	if (ep->type == USB3_EP_TYPE_BULK)
		ep->data_pid_start = 0;

	usb3_ep_activate(pcd, ep);
}

void usb3_do_status_ut_device(usb3_pcd_t *pcd)
{
	if (pcd == NULL)
		return;

	unsigned char *status = pcd->ep0_status_buf;

	if (pcd->speed != USB_SPEED_SUPER) {
		*(status + 1) = 0;
		return;
	}
	if (pcd->state == USB3_STATE_CONFIGURED) {
		if (usb3_u1_enabled(pcd))
			*status |= 1 << 2; /* left shift 2 bit */

		if (usb3_u2_enabled(pcd))
			*status |= 1 << 3; /* left shift 3 bit */

		*status |= pcd->ltm_enable << 4; /* left shift 4 bit */
	}

	*(status + 1) = 0;
}

static void usb3_do_get_status(usb3_pcd_t *pcd)
{
	usb_device_request_t ctrl = pcd->ep0_setup_pkt->req;
	unsigned char *status = pcd->ep0_status_buf;
	usb3_pcd_ep_t *ep;

	if (ctrl.w_length != 0x2) {
		ep0_do_stall(pcd);
		return;
	}

	switch (ut_get_recipient(ctrl.bm_request_type)) {
	case UT_DEVICE:
		usb3_do_status_ut_device(pcd);
		break;

	case UT_INTERFACE:
		*status = 0;
		*(status + 1) = 0;
		break;

	case UT_ENDPOINT:
		ep = usb3_get_ep_by_addr(pcd, ctrl.w_index);

		/* @todo check for EP stall */
		*status = ep->stopped;
		*(status + 1) = 0;
		break;

	default:
		ep0_do_stall(pcd);
		return;
	}

	pcd->ep0_req.bufdma = status;
	pcd->ep0_req.length = 0x2;
	pcd->ep0_req.actual = 0;
	usb3_ep0_start_transfer(pcd, &pcd->ep0_req);
}

static void usb3_do_clear_feature(usb3_pcd_t *pcd)
{
	usb_device_request_t ctrl = pcd->ep0_setup_pkt->req;
	usb3_pcd_ep_t *ep;

	switch (ut_get_recipient(ctrl.bm_request_type)) {
	case UT_DEVICE:
		switch (ctrl.w_value) {
		case UF_DEVICE_REMOTE_WAKEUP:
			break;

		case UF_TEST_MODE:
			/* @todo Add CLEAR_FEATURE for TEST modes. */
			break;

		case UF_U1_ENABLE:
			if (pcd->speed != USB_SPEED_SUPER || pcd->state != USB3_STATE_CONFIGURED) {
				ep0_do_stall(pcd);
				return;
			}
			usb3_disable_u1(pcd);
			break;

		case UF_U2_ENABLE:
			if (pcd->speed != USB_SPEED_SUPER || pcd->state != USB3_STATE_CONFIGURED) {
				ep0_do_stall(pcd);
				return;
			}
			usb3_disable_u2(pcd);
			break;

		case UF_LTM_ENABLE:
			if (pcd->speed != USB_SPEED_SUPER || pcd->state != USB3_STATE_CONFIGURED || ctrl.w_index != 0) {
				ep0_do_stall(pcd);
				return;
			}
			pcd->ltm_enable = 0;
			break;

		default:
			ep0_do_stall(pcd);
			return;
		}
		break;

	case UT_INTERFACE:
		/* if FUNCTION_SUSPEND ... */
		if (ctrl.w_value) {
			ep0_do_stall(pcd);
			return;
		}
		break;

	case UT_ENDPOINT:
		ep = usb3_get_ep_by_addr(pcd, ctrl.w_index);
		if (ctrl.w_value != UF_ENDPOINT_HALT) {
			ep0_do_stall(pcd);
			return;
		}
		do_clear_halt(pcd, ep);
		break;

	default:
		break;
	}

	pcd->ep0.is_in = 1;
	pcd->ep0state = EP0_IN_WAIT_NRDY;
}

static void usb3_do_set_feature(usb3_pcd_t *pcd)
{
	usb_device_request_t ctrl = pcd->ep0_setup_pkt->req;
	usb3_pcd_ep_t *ep;

	switch (ut_get_recipient(ctrl.bm_request_type)) {
	case UT_DEVICE:
		switch (ctrl.w_value) {
		case UF_DEVICE_REMOTE_WAKEUP:
			break;

		case UF_TEST_MODE:
			break;

		case UF_DEVICE_B_HNP_ENABLE:
			break;

		case UOTG_NTF_HOST_REL:
			break;

		case UOTG_B3_RSP_ENABLE:
			break;

		case UF_DEVICE_A_HNP_SUPPORT:
			/* RH port supports HNP */
			break;

		case UF_DEVICE_A_ALT_HNP_SUPPORT:
			/* other RH port does */
			break;

		case UF_U1_ENABLE:
			if (pcd->speed != USB_SPEED_SUPER || pcd->state != USB3_STATE_CONFIGURED) {
				ep0_do_stall(pcd);
				return;
			}
			usb3_enable_u1(pcd);
			break;

		case UF_U2_ENABLE:
			if (pcd->speed != USB_SPEED_SUPER || pcd->state != USB3_STATE_CONFIGURED) {
				ep0_do_stall(pcd);
				return;
			}
			usb3_enable_u2(pcd);
			break;

		case UF_LTM_ENABLE:
			if (pcd->speed != USB_SPEED_SUPER || pcd->state != USB3_STATE_CONFIGURED || ctrl.w_index != 0) {
				ep0_do_stall(pcd);
				return;
			}
			pcd->ltm_enable = 1;
			break;

		default:
			ep0_do_stall(pcd);
			return;
		}
		break;

	case UT_INTERFACE:
		/* if FUNCTION_SUSPEND ... */
		if (ctrl.w_value) {
			ep0_do_stall(pcd);
			return;
		}
		break;

	case UT_ENDPOINT:
		ep = usb3_get_ep_by_addr(pcd, ctrl.w_index);
		if (ctrl.w_value != UF_ENDPOINT_HALT) {
			ep0_do_stall(pcd);
			return;
		}
		ep->stopped = 1;
		usb3_ep_set_stall(pcd, ep);
		break;

	default:
		break;
	}

	pcd->ep0.is_in = 1;
	pcd->ep0state = EP0_IN_WAIT_NRDY;
}

static void usb3_do_set_address(usb3_pcd_t *pcd)
{
	usb_device_request_t ctrl = pcd->ep0_setup_pkt->req;

	if (ctrl.bm_request_type == UT_DEVICE) {
		usb3_set_address(pcd, ctrl.w_value);
		pcd->ep0.is_in = 1;
		pcd->ep0state = EP0_IN_WAIT_NRDY;
		if (ctrl.w_value) {
			pcd->state = USB3_STATE_ADDRESSED;
		} else {
			pcd->state = USB3_STATE_DEFAULT;
		}
	}
}

static void usb3_do_set_config(usb3_pcd_t *pcd)
{
	usb_device_request_t ctrl = pcd->ep0_setup_pkt->req;
	unsigned short wvalue = ctrl.w_value;
	usb3_pcd_ep_t *ep;

	if (ctrl.bm_request_type != (UT_WRITE | UT_STANDARD | UT_DEVICE)) {
		ep0_do_stall(pcd);
		return;
	}

	if (!wvalue || (wvalue == CONFIG_VALUE)) {
		pcd->new_config = wvalue;
		/* Set new configuration */
		if (wvalue) {
			/* Activate bulk in endpoint */
			ep = &pcd->in_ep;
			usb3_pcd_ep_enable(pcd, ep);

			/* Activate bulk out endpoint */
			ep = &pcd->out_ep;
			usb3_pcd_ep_enable(pcd, ep);

			/* Prepare for next bulk transfer */
			usb3_bulk_out_transfer((void *)pcd);
			usb3_bulk_in_transfer((void *)pcd, "start download process.");
			pcd->state = USB3_STATE_CONFIGURED;
		} else {
			pcd->state = USB3_STATE_ADDRESSED;
		}

		pcd->ep0.is_in = 1;
		pcd->ep0state = EP0_IN_WAIT_NRDY;
	} else {
		ep0_do_stall(pcd);
	}
}

static void usb3_do_get_config(usb3_pcd_t *pcd)
{
	usb_device_request_t ctrl = pcd->ep0_setup_pkt->req;
	unsigned char *status = pcd->ep0_status_buf;

	if (ctrl.bm_request_type != (UT_READ | UT_STANDARD | UT_DEVICE)) {
		ep0_do_stall(pcd);
		return;
	}

	/* Notify host the current config value */
	*status = pcd->new_config;

	pcd->ep0_req.bufdma = status;
	pcd->ep0_req.length = 1;
	pcd->ep0_req.actual = 0;
	usb3_ep0_start_transfer(pcd, &pcd->ep0_req);
}

static void usb3_do_get_descriptor(usb3_pcd_t *pcd)
{
	usb_device_request_t ctrl = pcd->ep0_setup_pkt->req;
	usb3_device_t *usb3_dev = pcd->usb3_dev;
	unsigned char dt = ctrl.w_value >> 8; /* 8bit */
	unsigned char index = (unsigned char)ctrl.w_value;
	unsigned short len = ctrl.w_length;
	unsigned char *buf = pcd->ep0_status_buf;
	unsigned short value = 0;

	if (ctrl.bm_request_type != (UT_READ | UT_STANDARD | UT_DEVICE)) {
		ep0_do_stall(pcd);
		return;
	}

	switch (dt) {
	case UDESC_DEVICE: {
		struct usb_device_descriptor *dev = (struct usb_device_descriptor *)usb3_dev->dev_desc;

		dev->b_length = sizeof(struct usb_device_descriptor);
		dev->b_descriptor_type = UDESC_DEVICE;

		dev->b_device_class = 0;
		dev->b_device_sub_class = 0;
		dev->b_device_protocol = 0;

		if (pcd->speed == USB_SPEED_SUPER) {
			dev->bcd_usb = 0x300;
			dev->b_max_packet_size0 = 9; // NOTE! 2 ^ 9 = 512 for USB3
		} else if (pcd->speed == USB_SPEED_HIGH) {
			dev->bcd_usb = 0x0200;
			dev->b_max_packet_size0 = pcd->ep0.maxpacket;
		} else {
			dev->bcd_usb = 0x0110;
			dev->b_max_packet_size0 = pcd->ep0.maxpacket;
		}

		dev->id_vendor = 0x1D6B;
		dev->id_product = 0xA001;

		dev->bcd_device = 0x0100;

		dev->i_manufacturer = STRING_MANUFACTURER;
		dev->i_product = STRING_PRODUCT;
		dev->i_serial_number = 0;

		dev->b_num_configurations = 1;

		value = sizeof(struct usb_device_descriptor);
		memcpy((void *)buf, (void *)dev, value);
	} break;

	case UDESC_DEVICE_QUALIFIER: {
		struct usb_qualifier_descriptor *qual = (struct usb_qualifier_descriptor *)buf;
		struct usb_device_descriptor *dev = (struct usb_device_descriptor *)usb3_dev->dev_desc;

		qual->b_length = sizeof(*qual);
		qual->b_descriptor_type = UDESC_DEVICE_QUALIFIER;
		qual->bcd_usb = dev->bcd_usb;
		qual->b_device_class = dev->b_device_class;
		qual->b_device_sub_class = dev->b_device_sub_class;
		qual->b_device_protocol = dev->b_device_protocol;
		qual->b_max_packet_size0 = dev->b_max_packet_size0;
		qual->b_num_configurations = 1;
		qual->b_reserved = 0;

		value = sizeof(usb_qualifier_descriptor_t);
	} break;

	case UDESC_CONFIG: {
		struct usb_config_descriptor *config = (struct usb_config_descriptor *)buf;

		config->b_length = sizeof(*config);
		config->b_descriptor_type = UDESC_CONFIG;
		config->b_num_interfaces = 1;
		config->b_configuration_value = CONFIG_VALUE;
		config->i_configuration = 0;
		config->bm_attributes = USB_CONFIG_ATT_ONE;

		if (pcd->speed == USB_SPEED_SUPER) {
			config->b_max_power = USB_CONFIG_VBUS_DRAW / 8; /* divided 8 */
		} else {
			config->b_max_power = USB_CONFIG_VBUS_DRAW / 2; /* divided 2 */
		}

		buf += sizeof(*config);
		memcpy((void *)buf, (void *)&interface, sizeof(interface));
		buf += sizeof(interface);

		switch (pcd->speed) {
		case USB_SPEED_SUPER:
			memcpy((void *)buf, (void *)&ss_bulk_in, sizeof(ss_bulk_in));
			buf += sizeof(ss_bulk_in);
			memcpy((void *)buf, (void *)&ep_comp, sizeof(ep_comp));
			buf += sizeof(ep_comp);
			memcpy((void *)buf, (void *)&ss_bulk_out, sizeof(ss_bulk_out));
			buf += sizeof(ss_bulk_out);
			memcpy((void *)buf, (void *)&ep_comp, sizeof(ep_comp));

			config->w_total_length = sizeof(*config) + sizeof(interface) + sizeof(ss_bulk_in) + sizeof(ep_comp) +
				sizeof(ss_bulk_out) + sizeof(ep_comp);
			break;

		default: /* HS/FS */
		{
			struct usb_endpoint_descriptor *endp = (struct usb_endpoint_descriptor *)buf;

			memcpy((void *)buf, (void *)&hs_bulk_in, sizeof(hs_bulk_in));
			(endp++)->w_max_packet_size = pcd->in_ep.maxpacket;
			buf += sizeof(hs_bulk_in);
			memcpy((void *)buf, (void *)&hs_bulk_out, sizeof(hs_bulk_out));
			(endp++)->w_max_packet_size = pcd->out_ep.maxpacket;
		}
			config->w_total_length = sizeof(*config) + sizeof(interface) + sizeof(hs_bulk_in) + sizeof(hs_bulk_out);
			break;
		}
		value = config->w_total_length;
	} break;

	case UDESC_STRING: {
		switch (index) {
		case STRING_LANGUAGE:
			buf[0] = 0x04;
			buf[1] = UDESC_STRING;
			buf[2] = 0x09; /* buf number 2 data */
			buf[3] = 0x04; /* buf number 3 data */

			value = 0x04;
			break;

		case STRING_MANUFACTURER:
		{
			int ix;
			unsigned char *ptr = &buf[1];

			*ptr++ = UDESC_STRING;

			memcpy((void *)ptr, (void *)usb3_dev->string_manu, usb3_dev->string_manu_len);
			ptr += usb3_dev->string_manu_len;

			for (ix = 0; ix < sizeof(usb3_dev->manu_suffix); ix++) {
				char chr = usb3_dev->manu_suffix[ix];
				if (chr == '\0')
					break;

				if (!isprint(chr))
					chr = '?';

				*ptr++ = chr;
				*ptr++ = '\0';
			}

			buf[0] = (unsigned char)(ptr - buf);
			value = buf[0];

			break;
		}

		case STRING_PRODUCT:
			buf[0] = usb3_dev->string_prod_len + 0x2;
			buf[1] = UDESC_STRING;

			memcpy((void *)(buf + 0x2), (void *)usb3_dev->string_prod, usb3_dev->string_prod_len);

			value = usb3_dev->string_prod_len + 0x2;
			break;

		default:
			ep0_do_stall(pcd);
			return;
		}
	} break;

	case UDESC_BOS:
		if (pcd->speed != USB_SPEED_SUPER) {
			/*
			* The USB compliance test (USB 2.0 Command Verifier)
			* issues this request. We should not run into the
			* default path here. But return for now until
			* the superspeed support is added.
			*/
		}

		value = usb_bos.w_total_length;

		memcpy((void *)buf, (void *)&usb_bos, sizeof(usb_bos));
		buf += sizeof(usb_bos);
		memcpy((void *)buf, (void *)&capability1, sizeof(capability1));
		buf += sizeof(capability1);
		memcpy((void *)buf, (void *)&capability2, sizeof(capability2));
		buf += sizeof(capability2);
		memcpy((void *)buf, (void *)&capability3, sizeof(capability3));

		break;

	default:
		ep0_do_stall(pcd);
		return;
	}

	pcd->ep0_req.bufdma = pcd->ep0_status_buf;
	pcd->ep0_req.length = value < len ? value : len;
	pcd->ep0_req.actual = 0;
	usb3_ep0_start_transfer(pcd, &pcd->ep0_req);
}

void usb3_do_setup(usb3_pcd_t *pcd)
{
	usb_device_request_t ctrl = pcd->ep0_setup_pkt->req;
	usb3_pcd_ep_t *ep0 = &pcd->ep0;
	unsigned short wlength;

	wlength = ctrl.w_length;

	ep0->stopped = 0;
	ep0->three_stage = 1;

	if (ctrl.bm_request_type & UE_DIR_IN) {
		ep0->is_in = 1;
		pcd->ep0state = EP0_IN_DATA_PHASE;
	} else {
		ep0->is_in = 0;
		pcd->ep0state = EP0_OUT_DATA_PHASE;
	}

	if (wlength == 0) {
		ep0->is_in = 1;
		pcd->ep0state = EP0_IN_WAIT_NRDY;
		ep0->three_stage = 0;
	}

	if ((ut_get_type(ctrl.bm_request_type)) != UT_STANDARD) {
		ep0_do_stall(pcd);
		return;
	}

	switch (ctrl.b_request) {
	case UR_GET_STATUS:
		usb3_do_get_status(pcd);
		break;

	case UR_CLEAR_FEATURE:
		usb3_do_clear_feature(pcd);
		break;

	case UR_SET_FEATURE:
		usb3_do_set_feature(pcd);
		break;

	case UR_SET_ADDRESS:
		usb3_do_set_address(pcd);
		break;

	case UR_SET_CONFIG:
		usb3_do_set_config(pcd);

		/* Must wait until SetConfig before accepting U1/U2 link
		* control, otherwise we have problems with VIA hubs
		*/
		usb3_accept_u1(pcd);
		usb3_accept_u2(pcd);

		usb_info("usb enum done\n");

		pcd->ltm_enable = 0;
		break;

	case UR_GET_CONFIG:
		usb3_do_get_config(pcd);
		break;

	case UR_GET_DESCRIPTOR:
		usb3_do_get_descriptor(pcd);
		break;

	case UR_SET_SEL:
		/* For now this is a no-op */
		pcd->ep0_req.bufdma = pcd->ep0_status_buf;
		pcd->ep0_req.length = USB3_STATUS_BUF_SIZE;
		pcd->ep0_req.actual = 0;
		ep0->send_zlp = 0;
		usb3_ep0_start_transfer(pcd, &pcd->ep0_req);
		break;

	case UR_SET_ISOC_DELAY:
		/* For now this is a no-op */
		pcd->ep0.is_in = 1;
		pcd->ep0state = EP0_IN_WAIT_NRDY;
		break;

	default:
		ep0_do_stall(pcd);
		break;
	}
}

void usb3_os_get_trb(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep, usb3_pcd_req_t *req)
{
	/* If EP0, fill request with EP0 IN/OUT data TRB */
	if (ep == &pcd->ep0) {
		if (ep->is_in) {
			req->trb = pcd->ep0_in_desc;
			req->trbdma = (phys_addr_t)pcd->ep0_in_desc;
		} else {
			req->trb = pcd->ep0_out_desc;
			req->trbdma = (phys_addr_t)pcd->ep0_out_desc;
		}
		/* Else fill request with TRB from the non-EP0 allocation */
	} else {
		req->trb = ep->ep_desc;
		req->trbdma = (phys_addr_t)ep->ep_desc;
	}
}

void usb3_ep0_start_transfer(usb3_pcd_t *pcd, usb3_pcd_req_t *req)
{
	usb3_pcd_ep_t *ep0 = &pcd->ep0;
	usb3_dev_ep_regs_t *ep_reg;
	usb3_dma_desc_t *desc;
	phys_addr_t desc_dma;
	unsigned int desc_type, len;
	unsigned char tri;

	/* Get the DMA Descriptor (TRB) for this request */
	usb3_os_get_trb(pcd, ep0, req);
	desc = req->trb;
	desc_dma = req->trbdma;

	if (ep0->is_in) {
		/*
		* Start DMA on EP0-IN
		*/
		ep_reg = ep0->in_ep_reg;

		/* DMA Descriptor (TRB) setup */
		len = req->length;

		if (pcd->ep0state == EP0_IN_STATUS_PHASE) {
			if (ep0->three_stage)
				desc_type = USB3_DSCCTL_TRBCTL_STATUS_3;
			else
				desc_type = USB3_DSCCTL_TRBCTL_STATUS_2;
		} else {
			desc_type = USB3_DSCCTL_TRBCTL_CTLDATA_1ST;
		}

		usb3_fill_desc(desc, (phys_addr_t)req->bufdma, len, 0, desc_type,
			USB3_DSCCTL_IOC_BIT | USB3_DSCCTL_ISP_BIT | USB3_DSCCTL_LST_BIT, 1);
		/* Issue "DEPSTRTXFER" command to EP0-IN */
		tri = usb3_dep_startxfer(pcd, ep_reg, desc_dma, 0);
		ep0->tri_in = tri;
	} else {
		/*
		* Start DMA on EP0-OUT
		*/
		ep_reg = ep0->out_ep_reg;

		/* DMA Descriptor (TRB) setup */
		len = (req->length + ep0->maxpacket - 1) & ~(ep0->maxpacket - 1);

		if (pcd->ep0state == EP0_OUT_STATUS_PHASE) {
			if (ep0->three_stage)
				desc_type = USB3_DSCCTL_TRBCTL_STATUS_3;
			else
				desc_type = USB3_DSCCTL_TRBCTL_STATUS_2;
		} else {
			desc_type = USB3_DSCCTL_TRBCTL_CTLDATA_1ST;
		}

		usb3_fill_desc(desc, (phys_addr_t)req->bufdma, len, 0, desc_type,
			USB3_DSCCTL_IOC_BIT | USB3_DSCCTL_ISP_BIT | USB3_DSCCTL_LST_BIT, 1);
		/* Issue "DEPSTRTXFER" command to EP0-OUT */
		tri = usb3_dep_startxfer(pcd, ep_reg, desc_dma, 0);
		ep0->tri_out = tri;
	}
}

static void ep0_continue_transfer(usb3_pcd_t *pcd, usb3_pcd_req_t *req)
{
	usb3_pcd_ep_t *ep0 = &pcd->ep0;
	usb3_dev_ep_regs_t *ep_reg;
	usb3_dma_desc_t *desc;
	phys_addr_t desc_dma;
	unsigned char tri;

	/* It can be called to send a 0-length packet after the end of a transfer, so the code here
	* only supports that case.
	*/

	if (ep0->is_in) {
		desc = pcd->ep0_in_desc;
		desc_dma = (phys_addr_t)(pcd->ep0_in_desc);
		ep_reg = ep0->in_ep_reg;

		/* DMA Descriptor Setup */
		usb3_fill_desc(desc, (phys_addr_t)req->bufdma, 0, 0, USB3_DSCCTL_TRBCTL_NORMAL,
			USB3_DSCCTL_IOC_BIT | USB3_DSCCTL_ISP_BIT | USB3_DSCCTL_LST_BIT, 1);

		tri = usb3_dep_startxfer(pcd, ep_reg, desc_dma, 0);
		ep0->tri_in = tri;
	}
}

void usb3_ep_start_transfer(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep)
{
	usb3_pcd_req_t *req = &ep->req;
	usb3_dev_ep_regs_t *ep_reg;
	usb3_dma_desc_t *desc;
	phys_addr_t desc_dma;
	unsigned int len;
	unsigned char tri;

	/* Get the TRB for this request */
	usb3_os_get_trb(pcd, ep, req);

	ep->send_zlp = 0;
	desc = req->trb;
	desc_dma = req->trbdma;

	if (ep->is_in) {
		/* For IN, TRB length is just xfer length */
		len = req->length;
	} else {
		/* For OUT, TRB length must be multiple of maxpacket */
		/* Must be power of 2, use cheap AND */
		len = (req->length + ep->maxpacket - 1) & ~(ep->maxpacket - 1);

		req->length = len;
	}

	/* DMA Descriptor Setup */
	usb3_fill_desc(desc, (phys_addr_t)req->bufdma, len, 0, USB3_DSCCTL_TRBCTL_NORMAL,
		USB3_DSCCTL_ISP_BIT | USB3_DSCCTL_IOC_BIT | USB3_DSCCTL_LST_BIT, 1);

	if (ep->is_in) {
		/*
		* Start DMA on EPn-IN
		*/
		ep_reg = ep->in_ep_reg;
		/* Issue "DEPSTRTXFER" command to EP */

		tri = usb3_dep_startxfer(pcd, ep_reg, desc_dma, 0);
		ep->tri_in = tri;
	} else {
		/*
		* Start DMA on EPn-OUT
		*/
		ep_reg = ep->out_ep_reg;
#if 1
		if (ep->xfer_started) {
			/* Issue "DEPUPDTXFER" command to EP */
			usb3_dep_updatexfer(pcd, ep_reg, ep->tri_out);
		} else {
#endif
			/* Issue "DEPSTRTXFER" command to EP */
			tri = usb3_dep_startxfer(pcd, ep_reg, desc_dma, 0);
			ep->tri_out = tri;
			ep->xfer_started = 1;
		}
	}
}

void usb3_bulk_out_transfer(void *dev)
{
	usb3_pcd_t *pcd = (usb3_pcd_t *)dev;
	usb3_pcd_ep_t *ep = &pcd->out_ep;
	usb3_pcd_req_t *req = &ep->req;

	req->length = ep->maxpacket;
	req->bufdma = pcd->ss_bulk_buf;
	req->complete = usb3_handle_protocol;

	usb3_ep_start_transfer(pcd, ep);

	return;
}

void usb3_bulk_out_continue_transfer(void *dev)
{
	usb3_pcd_t *pcd = (usb3_pcd_t *)dev;
	usb3_pcd_ep_t *ep = &pcd->out_ep;

	usb3_ep_start_transfer(pcd, ep);
}

void usb_tx_status_complete(void *dev) {}

void usb3_bulk_in_transfer(void *dev, const char *status)
{
	usb3_pcd_t *pcd = (usb3_pcd_t *)dev;
	usb3_pcd_ep_t *ep = &pcd->in_ep;
	usb3_pcd_req_t *req = &ep->req;
	int len;

	len = strlen(status) + 1;
	memset(req->bufdma, 0, BUFFER_SIZE);
	memcpy(req->bufdma, status, len);
	req->length = len;

	req->complete = usb_tx_status_complete;

	usb3_ep_start_transfer(pcd, ep);
}

static void ep0_complete_request(usb3_pcd_t *pcd, usb3_pcd_req_t *req, usb3_dma_desc_t *desc, int status)
{
	usb3_pcd_ep_t *ep = &pcd->ep0;

	if (!req)
		return;

	if (pcd->ep0state == EP0_OUT_DATA_PHASE || pcd->ep0state == EP0_IN_DATA_PHASE) {
		if (ep->is_in) {
			if (usb3_get_xfercnt(desc) == 0) {
				pcd->ep0.is_in = 0;
				pcd->ep0state = EP0_OUT_WAIT_NRDY;
			}
		} else {
			pcd->ep0.is_in = 1;
			pcd->ep0state = EP0_IN_WAIT_NRDY;
		}
	}
}

static void setup_in_status_phase(usb3_pcd_t *pcd, void *buf)
{
	usb3_pcd_ep_t *ep0 = &pcd->ep0;

	if (pcd->ep0state == EP0_STALL)
		return;

	ep0->is_in = 1;
	pcd->ep0state = EP0_IN_STATUS_PHASE;

	pcd->ep0_req.bufdma = buf;
	pcd->ep0_req.length = 0;
	pcd->ep0_req.actual = 0;
	usb3_ep0_start_transfer(pcd, &pcd->ep0_req);
}

static void setup_out_status_phase(usb3_pcd_t *pcd, void *buf)
{
	usb3_pcd_ep_t *ep0 = &pcd->ep0;

	if (pcd->ep0state == EP0_STALL)
		return;

	ep0->is_in = 0;
	pcd->ep0state = EP0_OUT_STATUS_PHASE;

	pcd->ep0_req.bufdma = buf;
	pcd->ep0_req.length = 0;
	pcd->ep0_req.actual = 0;
	usb3_ep0_start_transfer(pcd, &pcd->ep0_req);
}

void usb3_handle_ep0(usb3_pcd_t *pcd, usb3_pcd_req_t *req, unsigned int event)
{
	usb3_pcd_ep_t *ep0 = &pcd->ep0;
	usb3_dma_desc_t *desc;
	unsigned int byte_count;
	unsigned int len = 0;
	unsigned int status;

	switch (pcd->ep0state) {
	case EP0_IN_DATA_PHASE:
		if (!req)
			req = &pcd->ep0_req;
		desc = pcd->ep0_in_desc;

		if (dwc_usb3_is_hwo(desc))
			goto out;

		status = usb3_get_xfersts(desc);
		if (status & USB3_TRBRSP_SETUP_PEND)
			/* Start of a new Control transfer */
			desc->status = 0;

		byte_count = req->length - usb3_get_xfercnt(desc);
		req->actual += byte_count;
		req->bufdma += byte_count;

		if (req->actual < req->length) {
			/* IN CONTINUE, Stall EP0 */
			ep0_do_stall(pcd);
		} else if (ep0->send_zlp) {
			/* CONTINUE TRANSFER IN ZLP */
			ep0_continue_transfer(pcd, req);
			ep0->send_zlp = 0;
		} else {
			/* COMPLETE IN TRANSFER */
			ep0_complete_request(pcd, req, desc, 0);
		}

		break;

	case EP0_OUT_DATA_PHASE:
		if (!req)
			req = &pcd->ep0_req;
		desc = pcd->ep0_out_desc;

		if (dwc_usb3_is_hwo(desc))
			goto out;

		status = usb3_get_xfersts(desc);
		if (status & USB3_TRBRSP_SETUP_PEND)
			/* Start of a new Control transfer */

			len = (req->length + ep0->maxpacket - 1) & ~(ep0->maxpacket - 1);
		byte_count = len - usb3_get_xfercnt(desc);
		req->actual += byte_count;
		req->bufdma += byte_count;

		if (ep0->send_zlp) {
			/* CONTINUE TRANSFER OUT ZLP */
			ep0_continue_transfer(pcd, req);
			ep0->send_zlp = 0;
		} else {
			/* COMPLETE OUT TRANSFER */
			ep0_complete_request(pcd, req, desc, 0);
		}

		break;

	case EP0_IN_WAIT_NRDY:
	case EP0_OUT_WAIT_NRDY:
		if (ep0->is_in)
			setup_in_status_phase(pcd, pcd->ep0_setup_pkt);
		else
			setup_out_status_phase(pcd, pcd->ep0_setup_pkt);

		break;

	case EP0_IN_STATUS_PHASE:
	case EP0_OUT_STATUS_PHASE:
		if (ep0->is_in)
			desc = pcd->ep0_in_desc;
		else
			desc = pcd->ep0_out_desc;
		ep0_complete_request(pcd, req, desc, 0);

		pcd->ep0state = EP0_IDLE;
		ep0->stopped = 1;
		ep0->is_in = 0; /* OUT for next SETUP */

		/* Prepare for more SETUP Packets */
		usb3_ep0_out_start(pcd);
		break;

	case EP0_STALL:
		break;

	case EP0_IDLE:
		break;
	default:
		break;
	}
out:
	return;
}

void usb3_os_handle_ep0(usb3_pcd_t *pcd, unsigned int event)
{
	usb3_pcd_req_t *req = NULL;

	if (pcd->ep0state == EP0_IDLE) {
		usb3_do_setup(pcd);
	} else {
		usb3_handle_ep0(pcd, req, event);
	}
}

void usb3_request_done(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep, usb3_pcd_req_t *req, int status)
{
	if (ep != &pcd->ep0)
		req->trb = NULL;
	if (req->complete)
		req->complete(pcd);

	req->actual = 0;

	return;
}

int usb3_ep_complete_request(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep, unsigned int event)
{
	usb3_pcd_req_t *req = &ep->req;
	usb3_dma_desc_t *desc = req->trb;
	unsigned int byte_count;
	int error = USB_NO_ERR;

	ep->send_zlp = 0;

	if (!desc)
		return USB_PROCESS_ERR;

	if (dwc_usb3_is_hwo(desc))
		return USB_PROCESS_ERR;

	if (ep->is_in) {
		/* IN endpoint */
		if (usb3_get_xfercnt(desc) == 0)
			req->actual += req->length;
		/* Reset IN tri */
		ep->tri_in = 0;

		/* Complete the IN request */
		usb3_request_done(pcd, ep, req, 0);
	} else { /* OUT endpoint */
		byte_count = req->length - usb3_get_xfercnt(desc);
		req->actual += byte_count;
		req->bufdma += byte_count;
		pcd->file_received += byte_count;
		/* Reset OUT tri */
		ep->tri_out = 0;

		/* OUT transfer complete or not */
		if ((byte_count < ep->maxpacket) || (pcd->file_capacity <= pcd->file_received - FRAME_HEAD_LEN)) {
			/* Complete the OUT request */
			usb3_request_done(pcd, ep, req, 0);
		} else {
			usb3_bulk_out_continue_transfer((void *)pcd);
		}
	}

	return error;
}
