/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <serial.h>
#include <usb.h>
#include <crc16.h>

#ifdef DUMP_SWITCH
static void dump_buf(char *buf, int len)
{
	int i;
	char *p = buf;
	printf("%s->%d, buf=0x%.8x, len=%d\n",
			__FUNCTION__, __LINE__,
			(int)(buf), (int)(len));
	for (i = 0; i < (len); i++) {
		printf("0x%.2x ", *(p + i));
		if (!((i + 1) & 0x07))
			printf("\n");
	}
	printf("\n");
}
#endif

#define	XHEAD	0xAB
#define	XCMD	0xCD
#define	ACK     0xAA            /* ACK VALUE */
#define	NAK     0x55            /* NAK VALUE */

#define START_FRAME_LEN	5
#define MAX_BUFF_SIZE		1024
#define MAX_SEND_SIZE    20

static char recv_buf[MAX_BUFF_SIZE];

static unsigned short calc_crc16(const unsigned char *packet, unsigned long length)
{
	unsigned short crc16 = 0;
	unsigned long i;

	const unsigned short *crc16_table = get_crc16_table();
	if (crc16_table == NULL) {
		return 0;
	}

	for (i = 0; i < length; i++)
		crc16 = ((crc16 << 8) |     /* fetches High or low 8 bit */
			packet[i]) ^ crc16_table[(crc16 >> 8) & 0xFF];

	for (i = 0; i < 2; i++)  /* 2: Cycle and fetches High or low 8 bit */
		crc16 = ((crc16 << 8) | 0) ^ crc16_table[(crc16 >> 8) & 0xFF];

	return crc16;
}

static int recv_byte(void)
{
	if (serial_tstc())
		return serial_getc();

	return -1;
}

static char recv_data(void)
{
	int ret = -1;

	while (ret == -1)
		ret = recv_byte();

	return (char)ret;
}

void download_process(void)
{
	int i;
	int cr;
	int ret;
	unsigned int head_frame_len;
	unsigned int cmd_len = 0;
	unsigned char send_buf[MAX_SEND_SIZE] = {0};
	unsigned short cksum;

	while (1) {
retry:
		cr = recv_byte();
		if (cr == -1)
			goto retry;

		if (cr == XHEAD) {
			head_frame_len = START_FRAME_LEN;
			recv_buf[0] = (char)cr;

			/* RECV: head frame */
			for (i = 0; i < (head_frame_len - 1); i++)
				recv_buf[i + 1] = recv_data();

			/* crc check 3 length */
			cksum = calc_crc16((unsigned char *)recv_buf, 3);
			/* The most significant 8 bits of array 3 and array 4 are used. */
			if (cksum == (((unsigned char)recv_buf[3] << 8) | (unsigned char)recv_buf[4])) {
				/* init, The most significant 8 bits of array 1 and array 2 Plus 3. */
				cmd_len = (((unsigned char)recv_buf[1] << 8) | (unsigned char)recv_buf[2]) + 3;

				/* SEND: ack */
				send_buf[0] = ACK;
			} else {
				/* init */
				cmd_len = 0;

				/* SEND: nak */
				send_buf[0] = NAK;
			}
			serial_putc(send_buf[0]);
		} else if (cr == XCMD) {
			recv_buf[0] = (char)cr;

			if (cmd_len > MAX_BUFF_SIZE)
				goto retry;

			/* RECV: cmd data */
			for (i = 0; i < (cmd_len - 1); i++)
				recv_buf[i + 1] = recv_data();

			/* crc check cmd_len subtract 2 length */
			cksum = calc_crc16((unsigned char *)recv_buf, cmd_len - 2);
			/* cmd_len subtract 2 length significant 8 bits */
			if (cksum == (((unsigned char)(recv_buf[cmd_len - 2]) << 8) |
						  (unsigned char)recv_buf[cmd_len - 1])) {
				/* SEND: ack wait result */
				send_buf[0] = ACK;
				serial_putc(send_buf[0]);
			} else {
				memset(recv_buf, 0, sizeof(recv_buf));
				/* SEND: nak */
				send_buf[0] = NAK;
				serial_putc(send_buf[0]);

				goto retry;
			}

			/* clean crc */
			recv_buf[cmd_len - 1] = 0;
			/* cmd_len subtract 2 length */
			recv_buf[cmd_len - 2] = 0;

			/* cmd process */
			ret = run_command((recv_buf + 1), 0);
			if (ret)
				/* SEND: end flag */
				serial_puts("[EOT](ERROR)\n");
			else
				/* SEND: end flag */
				serial_puts("[EOT](OK)\n");
		} else {
			/* flush fifo */
		}
	}
}

__attribute__((weak)) int check_update_button(void)
{
	return 0;
}

__attribute__((weak)) void download_boot(const int (*handle)(void))
{
#if !(defined(CONFIG_TARGET_XM72050500) || defined(CONFIG_TARGET_XM720XXX))
	/* bootrom is already configs here, it doesnot be need in 72050500 set */
	if (check_update_button()) {
		writel(START_MAGIC, REG_START_FLAG);
		writel(SELF_BOOT_TYPE_USBDEV, SYS_CTRL_REG_BASE + REG_SC_GEN9);
	}
#endif

	if ((*(volatile unsigned int *)(REG_START_FLAG)) == START_MAGIC) {
		/* note: uart output is enable again for download */
		serial_enable_output(true);

		/* clear flag */
		*(volatile unsigned int *)(REG_START_FLAG) = 0;

//TODO: XXX
#if  defined(FPGA_SUPPORT) && (defined(CONFIG_TARGET_XM72050500) || defined(CONFIG_TARGET_XM720XXX))
/**************************************************************************************************/
		serial_puts("******************download_boot should not exe this function now*****************\n");
		return;
/**************************************************************************************************/
#endif
		serial_puts("start download process.\n");

		/* wait cmd from pc */
		for (;;) {
			if ((*(volatile unsigned int *)(SYS_CTRL_REG_BASE + REG_SC_GEN9)) ==
								SELF_BOOT_TYPE_USBDEV) {
#ifdef CONFIG_USB_GADGET
				udc_connect();
#else
				printf("[error] download_boot by usb is not supported.\n");
#endif
			} else {
				download_process();
			}
		}

		/* note: uart output is disable again */
		serial_enable_output(false);
	}

	if (handle)
		handle();
}
