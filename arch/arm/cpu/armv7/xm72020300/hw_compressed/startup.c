/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <config.h>
#include <linux/kconfig.h>
#include <asm/io.h>
#include <compiler.h>
#include <cpu_common.h>
#include <xmedia_timestamp.h>

const uintptr_t image_entry = (CONFIG_SYS_TEXT_BASE);

#ifndef CONFIG_XMEDIA_DISABLE_CONSOLE
#define info(_s)                uart_early_puts(_s)
#else
#define info(_s)
#endif
#define error(_s)               uart_early_puts(_s)
#define putstr(_s)              uart_early_puts(_s)

#include "hw_decompress.c"
#define GZIP_SIZE_OFFSET 0x4

#ifndef CONFIG_SYS_ICACHE_OFF
/* Invalidate entire I-cache and branch predictor array */
static void invalidate_icache_all(void)
{
	/*
	 * Invalidate all instruction caches to PoU.
	 * Also flushes branch target cache.
	 */
	asm volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0));

	/* Invalidate entire branch predictor array */
	asm volatile("mcr p15, 0, %0, c7, c5, 6" : : "r"(0));

	/* Full system DSB - make sure that the invalidation is complete */
	dsb();

	/* ISB - make sure the instruction stream sees it */
	isb();
}
#else
static void invalidate_icache_all(void)
{
}
#endif

u32 get_timestamp_count(void)
{
	return __raw_readl(TIMESTAMP_CNT_ADDR);
}

void update_timestamp_count(u32 cnt)
{
	__raw_writel(cnt, TIMESTAMP_CNT_ADDR);
}

void timestamp_timer_init(void)
{
	/* start timer for timestamp with mode below */
	unsigned int reg;

	reg = __raw_readl(SYS_CTRL_REG_BASE + REG_SC_CTRL);
	/* for timer,use sys BUS clk */
#define TIME_CLK_SEL  (1 << (18))
	reg |= TIME_CLK_SEL;
	__raw_writel(reg, SYS_CTRL_REG_BASE + REG_SC_CTRL);

	/* disable timer */
	__raw_writel(0, TIMESTAMP_TIMER_BASE + REG_TIMER_CONTROL);
	/* timer load */
	__raw_writel(~0, TIMESTAMP_TIMER_BASE + REG_TIMER_RELOAD);

	/*
	 * Timing mode: 32bits [bit 1 set as 1]
	 * ticking with 1/256 clock frequency [bit 3 set as 1, bit 2 set as 0]
	 * timing circulary [bit 6 set as 1]
	 * timer enabled [bit 7 set as 1]
	 */
	reg = (1 << 7) | (1 << 6) | (1 << 3) | (1 << 1);
	__raw_writel(reg, TIMESTAMP_TIMER_BASE + REG_TIMER_CONTROL);

	update_timestamp_count(0);
}

int timer_init(void)
{
	/*
	 * Under uboot, 0xffffffff is set to load register,
	 * timer_clk equals BUSCLK/2/256.
	 * e.g. BUSCLK equals 50M, it will roll back after 0xffffffff/timer_clk
	 * 43980s equals 12hours
	 */
	__raw_writel(0, CFG_TIMERBASE + REG_TIMER_CONTROL);
	__raw_writel(~0, CFG_TIMERBASE + REG_TIMER_RELOAD);

	/* 32 bit, periodic */
	__raw_writel(CFG_TIMER_CTRL, CFG_TIMERBASE + REG_TIMER_CONTROL);

	timestamp_timer_init();
	return 0;
}


void timestamp_mark(const char *func, u32 line, u32 type)
{
#define timestamp_get_time() readl(TIMESTAMP_TIMER_BASE + REG_TIMER_VALUE)
	u32 stamp = timestamp_get_time();
	u32 cnt = get_timestamp_count();

	if (cnt >= TIMESTAMP_MAX_ITEM_CNT) {
		uart_early_puts("No timestamp item!\n");
		return;
	}

	timestamp_item *item =
		(timestamp_item *)(uintptr_t)TIMESTAMP_ITEM_START_ADDR;
	item[cnt].stamp = ~stamp;
	item[cnt].func = (char *)func;
	item[cnt].line = line;
	item[cnt].type = type;
	cnt++;
	update_timestamp_count(cnt);
}

/******************************************************************************/
void start_armboot(void)
{
	unsigned char *pdst_l32 = NULL;
	unsigned int image_data_len;
	int pdst_len;
	int ret;
	int i;
	char *p = NULL;
	char *q = NULL;

	info("\r\nUncompress ");
	TIME_STAMP(0);

	/* use direct address mode */
	hw_dec_type = 0;
	/* init hw decompress IP */
	hw_dec_init();

	/* start decompress */
	pdst_l32 = (unsigned char *)image_entry;
	image_data_len = input_data_end - input_data;

	/* get dets length from compress image */
	p = (char *)&pdst_len;
	q = (char *)(input_data_end - GZIP_SIZE_OFFSET);
	for (i = 0; i < sizeof(int); i++)
		p[i] = q[i];

	ret = hw_dec_decompress(pdst_l32, &pdst_len, input_data,
						image_data_len, NULL);
	if (!ret) {
		info("Ok!");
	} else {
		error("Fail!");
		while (1);
	}

	info("\r\n");

	/* uinit hw decompress IP */
	hw_dec_uinit();
	void (*uboot)(void);
	uboot = (void (*))CONFIG_SYS_TEXT_BASE;
	invalidate_icache_all();
	TIME_STAMP(0);

	uboot();
}

void hang(void)
{
	error("### ERROR ### Please RESET the board ###\n");
	for (; ;) ;
}


void do_bad_sync(void)
{
	error("bad sync abort\r\n");
	error("Resetting CPU ...\r\n");
	reset_cpu(0);
}

void do_sync(void)
{
	error("sync abort\r\n");
	error("Resetting CPU ...\r\n");
	reset_cpu(0);
}

void do_bad_error(void)
{
	error("bad error\r\n");
	error("Resetting CPU ...\r\n");
	reset_cpu(0);
}

void do_error(void)
{
	error("error\r\n");
	error("Resetting CPU ...\r\n");
	reset_cpu(0);
}

void do_bad_fiq(void)
{
	error("bad fast interrupt request\r\n");
	error("Resetting CPU ...\r\n");
	reset_cpu(0);
}

void do_bad_irq(void)
{
	error("bad interrupt request\r\n");
	error("Resetting CPU ...\r\n");
	reset_cpu(0);
}

void do_fiq(void)
{
	error("fast interrupt request\r\n");
	error("Resetting CPU ...\r\n");
	reset_cpu(0);
}

void do_irq(void)
{
	error("interrupt request\r\n");
	error("Resetting CPU ...\r\n");
	reset_cpu(0);
}
