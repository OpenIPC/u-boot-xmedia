/*
 * Copyright (c) XMEDIA. All rights reserved.
 */
#ifndef HW_DECOMPRESS_H
#define HW_DECOMPRESS_H

#define GZIP_MAX_LEN    0xffffff /* (16MB - 1Byte) */

typedef enum {
	HW_DECOMPRESS_OP_START = 0, /* decompress operation start */
	HW_DECOMPRESS_OP_CONTINUE,  /* decompress operation continue */
	HW_DECOMPRESS_OP_END,       /* decompress operation end */
	HW_DECOMPRESS_OP_ONCE,      /* decompress operation just once */
} hw_decompress_op_type;

extern unsigned int hw_dec_type;
void hw_dec_init(void);
void hw_dec_uinit(void);

int hw_dec_decompress(const unsigned char *dst, int *dstlen,
	const unsigned char *src, int srclen, const void *unused);

/*
 * Support decompressing gzip file by sections
 * op_type: operation type
 * dst: addr of uncompressed file
 * dstlen: len of uncompressed file
 * src: addr of compressed file
 * srclen: len of compressed file
 *
 * note: only support v1 version now for sections feature
 */
int hw_dec_decompress_ex(hw_decompress_op_type op_type, const unsigned char *dst,
	int *dstlen, const unsigned char *src, int srclen);
#endif
