/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <command.h>
#include "ddr_training_custom.h"

#define UART_PL01X_DR          0x00 /* Data read or written from the interface. */
#define UART_PL01X_FR          0x18 /* Flag register (Read only). */
#define UART_PL01X_FR_TXFF     0x20

#define io_write(addr, val) (*(volatile unsigned int *)(addr) = (val))
#define io_read(addr) (*(volatile unsigned int *)(addr))

void uart_early_putc(const char c)
{
	/* Wait until there is space in the FIFO */
	while (io_read(DDR_REG_BASE_UART0 + UART_PL01X_FR) & UART_PL01X_FR_TXFF);

	/* Send the character */
	io_write(DDR_REG_BASE_UART0 + UART_PL01X_DR, c);
}

void uart_early_puts(const char *s)
{
	if (s == NULL)
		return;
	while (*s)
		uart_early_putc (*s++);
}

void uart_early_put_hex(const unsigned int hex)
{
	int i;
	char c;

	for (i = 28; i >= 0; i -= 4) { /* start from bit28, 4 bit per char */
		c = (hex >> (unsigned int)i) & 0x0F;
		if (c < 10) /* Decimal 10 */
			c += '0';
		else
			c += 'A' - 10; /* Decimal 10 */
		uart_early_putc(c);
	}
}
