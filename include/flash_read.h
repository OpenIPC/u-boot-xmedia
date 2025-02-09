/*
 * Copyright (c) XMEDIA. All rights reserved.
 */


#ifndef __FLASH_READ_H
#define __FLASH_READ_H

#define EMMC_BLOCK_SHIFT	9 /* change it if MMC_MAX_BLOCK_LEN changed */
#define FLASH_ALIGNED_SIZE (1 << EMMC_BLOCK_SHIFT) /* 512 Byte */

/*
 * read flash into out_addr from offset.
 * note:
 *      1. offset should aligned with FLASH_ALIGNED_SIZE
 *      2. if size is not aligned with FLASH_ALIGNED_SIZE,
 *         system will read (((size / FLASH_ALIGNED_SIZE) + 1) * FLASH_ALIGNED_SIZE) content to out_addr
 */
int flash_read(unsigned long offset, unsigned int size, unsigned char *out_addr);

/*
 * read flash into out_addr from offset.
 * note:
 *      1. offset and size should aligned with FLASH_ALIGNED_SIZE
 */
int flash_read_aligned(unsigned long offset, unsigned int size, unsigned char *out_addr);

#endif