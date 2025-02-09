/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __USBTFTP_H__
#define __USBTFTP_H__

#include "../drivers/usb/gadget/udc3/usb3_drv.h"
#include "common.h"
#include "legacy-mtd-utils.h"

#include <common.h>
#include <malloc.h>
#include <command.h>

#include <spi.h>
#include <spi_flash.h>
#include <jffs2/jffs2.h>
#include <linux/mtd/mtd.h>

extern int get_eventbuf_count(usb3_device_t *dev);
extern void dcache_disable(void);
extern void usb3_common_init(usb3_device_t *dev, volatile uint8_t *base);
extern void phy_usb_init(int index);
extern void usb3_init(usb3_device_t *dev);
extern void dcache_enable(void);
extern int usb_stop(void);
extern void usb3_handle_event(usb3_device_t *dev);

extern char tx_state[200];
#if defined CONFIG_FMC_SPI_NOR
typedef int (*USB3_HANDLE_REQUEST)(uint8_t* const buff, unsigned int *bufflen);
extern void set_usb3_call_back_func(USB3_HANDLE_REQUEST func);
#endif

#endif /* __USBTFTP_H__ */
