/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include "usb3_drv.h"
#include "usb3_pcd.h"
#include "usb3_prot.h"
#include "onchiprom.h"

#include <common.h>
#include <linux/string.h>

#define USTART 0xFA
#define UHEAD 0xFE
#define UDATA 0xDA
#define UTAIL 0xED
#define UCMD 0XAB
#define UREQ 0XFB

#define TX_STATE_BUFFER 500
#define TX_STATE_LIMIT 400
#define BUFFER_SIZE 512

char u_ack[2] = {0xAA, '\0'};
char u_nak[2] = {0x55, '\0'};
char tx_state[TX_STATE_BUFFER] = {0, 0x55, 0};
static phys_addr_t rx_addr;
static unsigned int rx_length;
int usb_connected;
int usb_out_open = 0;

void usb3_bulk_out_transfer_cmd(void *dev)
{
	usb3_pcd_t *pcd = (usb3_pcd_t *)dev;
	usb3_pcd_ep_t *ep = &pcd->out_ep;
	usb3_pcd_req_t *req = &ep->req;

	memset(pcd->ss_bulk_buf, 0x0, USB3_BULK_BUF_SIZE);

	req->length = ep->maxpacket;
	req->bufdma = pcd->ss_bulk_buf;
	req->complete = usb3_handle_protocol;
	usb3_ep_start_transfer(pcd, ep);

	return;
}

void usb3_handle_data(void *dev)
{
	usb3_pcd_t *pcd = NULL;
	usb3_pcd_ep_t *ep = NULL;
	usb3_pcd_req_t *req = NULL;
	if (dev == NULL)
		return;

	pcd = (usb3_pcd_t *)dev;
	ep = &pcd->out_ep;
	req = &ep->req;

	if (req->actual > rx_length)
		req->actual = rx_length;

	rx_addr += req->actual;
	rx_length -= req->actual;

	if (rx_length > 0) {
		usb3_bulk_out_transfer_data(dev);
	} else {
		usb3_bulk_out_transfer_cmd(dev);
	}
}

int usb3_bulk_out_transfer_data(void *dev)
{
	usb3_pcd_t *pcd = NULL;
	usb3_pcd_ep_t *ep = NULL;
	usb3_pcd_req_t *req = NULL;
	if (dev == NULL)
		return USB_PROCESS_ERR;

	pcd = (usb3_pcd_t *)dev;
	ep = &pcd->out_ep;
	req = &ep->req;

	/* For DWC3 controller,If CHN=0 and HWO=0 for the TRB, this field represents the total remaining Buffer
	 * Descriptor buffer size in bytes. Valid Range: 0 bytes to (16 MB - 1 byte).
	 * The hardware decrements this field to represent the remaining buffer size after data is
	 * transferred.
	 * Note: for bulk transfer, max length = 16MB - maxpacket;
	 *  */
	if (rx_length >= (USB3_DSCSTS_XFRCNT_MAX - ep->maxpacket)) {
		req->length = USB3_DSCSTS_XFRCNT_MAX - ep->maxpacket;
	} else {
		req->length = rx_length;
	}

	req->bufdma = (unsigned char *)rx_addr;
	req->complete = usb3_handle_data;

	usb3_ep_start_transfer(pcd, ep);

	return USB_NO_ERR;
}

void udc_puts(const char *s)
{
	if (s == NULL)
		return;

	if (strlen(tx_state) + strlen(s) > TX_STATE_LIMIT)
		(void)memset(tx_state, 0, TX_STATE_BUFFER);

	if (strlen(s) > TX_STATE_BUFFER) {
		return;
	} else {
		if (usb_out_open == 1) {
			strcat(tx_state, s);
		}
	}
}

typedef int (*USB3_HANDLE_REQUEST)(unsigned char * const buff, unsigned int *bufflen);
USB3_HANDLE_REQUEST handle_request = NULL;
void set_usb3_call_back_func(USB3_HANDLE_REQUEST func)
{
	handle_request = func;
}

void usb3_handle_protocol(void *dev)
{
	usb3_pcd_t *pcd = (usb3_pcd_t *)dev;
	int ret;
	char *buf = (char *)pcd->ss_bulk_buf;

	if (buf[0] == USTART) {
		usb3_bulk_out_transfer_cmd(pcd);
		usb3_bulk_in_transfer(dev, u_ack);
	} else if (buf[0] == UHEAD) {
		/*left shift 8/16/24 bit	[5][6][7][8]:buffer number */
		rx_addr = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | (buf[8]);
		/*left shift 8/16/24 bit	[1][2][3][4]:buffer number */
		rx_length = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | (buf[4]);
		if (rx_addr == rx_length) {
			usb3_bulk_out_transfer_cmd(pcd);
			usb_connected = 1;
			usb_out_open = 1;
		} else {
			usb3_bulk_out_transfer_data(pcd);
		}
		usb3_bulk_in_transfer(dev, u_ack);
		return;
	} else if(buf[0]==UCMD) {
		strcat(tx_state, " ");


		usb_out_open = 1;
		ret = run_command(buf + 0x3, 0);
		if (ret) {
			usb3_bulk_in_transfer(dev, "[EOT](ERROR)\r\n");
		} else {
			strcat(tx_state, "[EOT](OK)\r\n");
			usb3_bulk_out_transfer_cmd(pcd);
			usb3_bulk_in_transfer(dev, tx_state);
			memset(tx_state, 0, TX_STATE_BUFFER);
		}
	} else if (buf[0] == UTAIL) {
		if (rx_length > 0) {
			usb3_bulk_out_transfer_cmd(pcd);
			usb3_bulk_in_transfer(dev, u_nak);
		} else {
			usb3_bulk_in_transfer(dev, u_ack);
			usb3_bulk_out_transfer_cmd(pcd);
		}
	} else if (buf[0] == UREQ) {
		usb_out_open = 0;
		if (handle_request != NULL) {
			usb3_pcd_ep_t *ep = &pcd->in_ep;
			usb3_pcd_req_t *req = &ep->req;

			memset(req->bufdma, 0, BUFFER_SIZE);
			req->length = 0;
			ret = (*handle_request)(req->bufdma, &req->length);

			req->complete = usb_tx_status_complete;
			usb3_ep_start_transfer(pcd, ep);

			usb3_bulk_out_transfer_cmd(pcd);
		}
	}
}
