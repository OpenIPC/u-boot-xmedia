/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __SYS_H__
#define __SYS_H__
#include "types.h"
static inline void writel(unsigned int val, unsigned long int addr)
{
	(*(volatile unsigned int *)(addr)) = (val);
}

static inline void writew(unsigned short val, unsigned long int addr)
{
	(*(volatile unsigned short *)(addr)) = (val);
}

static inline void writeb(unsigned char val, unsigned long int addr)
{
	(*(volatile unsigned char *)(addr)) = (val);
}

static inline unsigned int readl(unsigned long int addr)
{
	return (*(volatile unsigned int *)(addr));
}

static inline unsigned short readw(unsigned long int addr)
{
	return (*(volatile unsigned short *)(addr));
}

static inline unsigned char readb(unsigned long int addr)
{
	return (*(volatile unsigned char *)(addr));
}

/* Set the bits of addr to 1, which in mask is 1 */
static inline void set_bits(unsigned int mask, unsigned long int addr)
{
	(*(volatile unsigned int *)(addr)) |= mask;
}

/* Clear the bits of addr to 0, which in mask is 1 */
static inline void clr_bits(unsigned int mask, unsigned long int addr)
{
	(*(volatile unsigned int *)(addr)) &= ~(mask);
}

/* Return True if all the bits of addr are 1, which is 1 in mask.
   Else, return False. */
static inline unsigned int is_bits_set(unsigned int mask, unsigned long int addr)
{
	return (((*(volatile unsigned int *)(addr)) & (mask)) == (mask));
}
#endif /* end of __SYS_H__ */
