/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __USB3_DRIVER_H__
#define __USB3_DRIVER_H__

#include "sys.h"
#include "usb3.h"
#include "usb3_hw.h"
#include "debug.h"

/* time limit of waiting for key enents */
#define USB_WAIT_FOR_ENUM_DONE_TIME_MS (300 * 1000)
#define USB_WAIT_FOR_RX_NEXT_TIME_MS (60 * 1000)

enum usb_error_type {
	USB_RX_NEXT_TIMEOUT = -5,
	USB_ENUM_DONE_TIMEOUT = -3,
	USB_PROCESS_ERR = -1,
	USB_NO_ERR = 0,
};

struct usb_data_handle {
	unsigned int u_file_type;
	unsigned int u_file_address;
	unsigned int u_file_length;

	unsigned int is_rx_start;

	unsigned int init_time_stamp;
	unsigned int rx_time_stamp;
};

#define usb_info(format, arg...) debug_printf("[USBINFO]" format, ##arg);
#define usb_err(format, arg...) debug_printf("[USBERR]" format, ##arg);

typedef enum pcd_state {
	USB3_STATE_UNCONNECTED, /* no host */
	USB3_STATE_DEFAULT,
	USB3_STATE_ADDRESSED,
	USB3_STATE_CONFIGURED,
} pcdstate_e;

typedef enum ep0_state {
	EP0_IDLE,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_IN_WAIT_NRDY,
	EP0_OUT_WAIT_NRDY,
	EP0_IN_STATUS_PHASE,
	EP0_OUT_STATUS_PHASE,
	EP0_STALL,
} ep0state_e;

typedef union usb_setup_pkt {
	usb_device_request_t req;
	unsigned int d32[2];
	unsigned char d8[8];
} usb_setup_pkt_t;

typedef struct usb3_pcd_req {
	usb3_dma_desc_t *trb;
	phys_addr_t trbdma;

	unsigned int length;
	unsigned int actual;

	unsigned char *bufdma;
	void (*complete)(void *);
} usb3_pcd_req_t;

typedef struct usb3_pcd_ep {
	struct usb3_pcd *pcd;

	usb3_dev_ep_regs_t *out_ep_reg;
	usb3_dev_ep_regs_t *in_ep_reg;

	unsigned char phys;
	unsigned char num;
	unsigned char type;
	unsigned char maxburst;
	unsigned short maxpacket;
	/* Tx FIFO # for IN EPs */
	unsigned char tx_fifo_num;

	/* The Transfer Resource Index from the Start Transfer command */
	unsigned char tri_out;
	unsigned char tri_in;

	unsigned char stopped;
	/* Send ZLP */
	unsigned char send_zlp;
	/* True if 3-stage control transfer */
	unsigned char three_stage;
	/* True if transfer has been started on EP */
	unsigned char xfer_started;
	/* EP direction 0 = OUT */
	unsigned char is_in;
	/* True if endpoint is active */
	unsigned char active;
	/* Initial data pid of bulk endpoint */
	unsigned char data_pid_start;

	/* ep_desc (excluding ep0) */
	usb3_dma_desc_t *ep_desc;

	/* TRB descriptor must be aligned to 16 bytes */
	unsigned char epx_desc[32];

	/* request (excluding ep0) */
	usb3_pcd_req_t req;
} usb3_pcd_ep_t;

typedef struct usb3_pcd {
	struct usb3_device *usb3_dev;

	signed int link_state;
	pcdstate_e state;
	unsigned char new_config;
	ep0state_e ep0state;

	unsigned int eps_enabled;
	unsigned int ltm_enable;

	usb3_pcd_ep_t ep0;
	usb3_pcd_ep_t out_ep;
	usb3_pcd_ep_t in_ep;

	usb3_dev_global_regs_t *dev_global_regs;
	usb3_dev_ep_regs_t *out_ep_regs;
	usb3_dev_ep_regs_t *in_ep_regs;

	usb3_pcd_req_t ep0_req;

	unsigned char speed;

	usb3_dma_desc_t *ep0_setup_desc;
	usb3_dma_desc_t *ep0_in_desc;
	usb3_dma_desc_t *ep0_out_desc;

	/* TRB descriptor must be aligned to 16 bytes */
	unsigned char ep0_setup[32];
	unsigned char ep0_in[32];
	unsigned char ep0_out[32];

	usb_setup_pkt_t ep0_setup_pkt[5];

#define USB3_STATUS_BUF_SIZE 512
	unsigned char ep0_status_buf[USB3_STATUS_BUF_SIZE];

#define USB3_BULK_BUF_SIZE 512
	unsigned char ss_bulk_buf[USB3_BULK_BUF_SIZE];

	unsigned int file_type;
	unsigned int file_address;
	unsigned int file_capacity;
	unsigned int file_total_frame;
	unsigned int file_curr_frame;
	unsigned int file_next_frame;
	unsigned int file_received;
	unsigned int file_complete;
} usb3_pcd_t;

#define usb3_pcd_ep_to_pcd(pcd_ep) ((pcd_ep)->usb3_pcd_ep_t.pcd)
#define usb3_pcd_ep_num(pcd_ep) ((pcd_ep)->usb3_pcd_ep_t.num)
#define usb3_pcd_ep_type(pcd_ep) ((pcd_ep)->usb3_pcd_ep_t.type)
#define usb3_pcd_ep_is_in(pcd_ep) ((pcd_ep)->usb3_pcd_ep_t.is_in)

#define dwc_usb3_is_hwo(desc) ((desc)->control & USB3_DSCCTL_HWO_BIT)
#define dwc_usb3_is_ioc(desc) ((desc)->control & USB3_DSCCTL_IOC_BIT)

#define usb3_get_xfercnt(desc) \
	(((desc)->status >> USB3_DSCSTS_XFRCNT_SHIFT) & (USB3_DSCSTS_XFRCNT_BITS >> USB3_DSCSTS_XFRCNT_SHIFT))
#define usb3_get_xfersts(desc) \
	(((desc)->status >> USB3_DSCSTS_TRBRSP_SHIFT) & (USB3_DSCSTS_TRBRSP_BITS >> USB3_DSCSTS_TRBRSP_SHIFT))

#define map_to_dma_addr(addr) ((addr))
// 	((addr) + (REG_BASE_LP_RAM_ACORE - REG_BASE_LP_RAM))

typedef struct usb3_device {
	volatile unsigned char *base;
	void *dev_desc;
	unsigned char string_manu[32];
	unsigned char string_prod[32];
	unsigned int string_manu_len;
	unsigned int string_prod_len;

	char manu_suffix[16];

	usb3_pcd_t pcd;
	unsigned int snpsid;
	usb3_core_global_regs_t *core_global_regs;

#define USB3_EVENT_BUF_SIZE 256
	unsigned int event_buf[USB3_EVENT_BUF_SIZE];
	unsigned int *event_ptr;
	struct usb_data_handle *p_handle;
} usb3_device_t;

unsigned int usb3_rd32(volatile unsigned int *addr);
void usb3_wr32(volatile unsigned int *addr, unsigned int val);
void usb3_set_address(usb3_pcd_t *pcd, unsigned int addr);
void usb3_accept_u1(usb3_pcd_t *pcd);
void usb3_accept_u2(usb3_pcd_t *pcd);
void usb3_enable_u1(usb3_pcd_t *pcd);
void usb3_enable_u2(usb3_pcd_t *pcd);
void usb3_disable_u1(usb3_pcd_t *pcd);
void usb3_disable_u2(usb3_pcd_t *pcd);
unsigned int usb3_u1_enabled(usb3_pcd_t *pcd);
unsigned int usb3_u2_enabled(usb3_pcd_t *pcd);
void usb3_dep_cstall(usb3_pcd_t *pcd, usb3_dev_ep_regs_t *ep_reg);
void usb3_dep_sstall(usb3_pcd_t *pcd, usb3_dev_ep_regs_t *ep_reg);
unsigned int handshake(usb3_device_t *dev, volatile unsigned int *ptr, unsigned int mask, unsigned int done);
void usb3_fill_desc(usb3_dma_desc_t *desc, phys_addr_t dma_addr, unsigned int dma_len, unsigned int stream, unsigned int type,
	unsigned int ctrlbits, int own);
void usb3_dep_startnewcfg(usb3_pcd_t *pcd, usb3_dev_ep_regs_t *ep_reg, unsigned int rsrcidx);
void usb3_dep_cfg(usb3_pcd_t *pcd, usb3_dev_ep_regs_t *ep_reg, unsigned int depcfg0, unsigned int depcfg1, unsigned int depcfg2);
void usb3_dep_xfercfg(usb3_pcd_t *pcd, usb3_dev_ep_regs_t *ep_reg, unsigned int depstrmcfg);
unsigned char usb3_dep_startxfer(usb3_pcd_t *pcd, usb3_dev_ep_regs_t *ep_reg, phys_addr_t dma_addr, unsigned int stream_or_uf);
void usb3_dep_updatexfer(usb3_pcd_t *pcd, usb3_dev_ep_regs_t *ep_reg, unsigned int tri);
void usb3_ep_activate(usb3_pcd_t *pcd, usb3_pcd_ep_t *ep);
void usb3_ep0_out_start(usb3_pcd_t *pcd);
void usb3_ep0_start_transfer(usb3_pcd_t *pcd, usb3_pcd_req_t *req);
void usb3_resume_usb2_phy(usb3_pcd_t *pcd);
void usb3_handle_dev_intr(usb3_pcd_t *pcd, unsigned int event);
int usb3_handle_ep_intr(usb3_pcd_t *pcd, unsigned int physep, unsigned int event);


#endif /* __USB3_DRIVER_H */
