/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include "hw_decompress_v1.h"
#include <hw_decompress.h>
#include <asm/io.h>
#ifndef error
#define error(fmt, args...) do { \
	printf("ERROR: " pr_fmt(fmt) "\nat %s:%d/%s()\n", \
		##args, __FILE__, __LINE__, __func__);	\
} while (0)

#include <common.h>
#endif
#include <compiler.h>


#if (defined CONFIG_TARGET_XM72050200)
#include "hw_decompress_xm72050200.c"
#endif

#if (defined CONFIG_TARGET_XM72050300)
#include "hw_decompress_xm72050300.c"
#endif

#if (defined CONFIG_TARGET_XM72020300)
#include "hw_decompress_xm72020300.c"
#endif

#if (defined CONFIG_TARGET_XM76050100)
#include "hw_decompress_xm76050100.c"
#endif

#if (defined CONFIG_TARGET_XM72050500)
#include "hw_decompress_xm72050500.c"
#endif

#if (defined CONFIG_TARGET_XM720XXX)
#include "hw_decompress_xm720xxx.c"
#endif


unsigned int hw_dec_type;
unsigned int hw_dec_sop;
unsigned int hw_dec_eop;
unsigned int hw_dec_cur_blk;
unsigned int hw_blk_total_num;

void hw_dec_sop_eop_first_set(int block_num)
{
	if (block_num == 1) {
		hw_dec_sop = 1;
		hw_dec_eop = 1;
	} else {
		hw_dec_sop = 1;
		hw_dec_eop = 0;
	}

	hw_dec_cur_blk = 0;
	hw_blk_total_num = block_num;
}

static inline void hw_dec_work_en_set(int work_en_flg)
{
	/* Enable the emar */
	writel(work_en_flg, HW_DEC_REG_BASE_ADDR + EAMR_WORK_EN_REG_OFST);
}

static inline void hw_dec_rtn_baddr_set(unsigned int addr)
{
	writel(addr, HW_DEC_REG_BASE_ADDR + DPRS_DATA_RTN_BADDR);
}

static inline void hw_dec_dprs_data_baddr_set(unsigned int addr)
{
	writel(addr, HW_DEC_REG_BASE_ADDR + DPRS_DATA_INFO_BADDR);
}

static inline void hw_dec_data_rtn_len_set(unsigned int len)
{
	writel(len, HW_DEC_REG_BASE_ADDR + DPRS_DATA_RTN_LEN);
}

static inline void hw_dec_dprs_data_len_set(unsigned int len)
{
	writel(len, HW_DEC_REG_BASE_ADDR + DPRS_DATA_INFO_LEN);
}

static inline void hw_dec_crc_check_en(unsigned int crc_en)
{
	writel(crc_en, HW_DEC_REG_BASE_ADDR + CRC_CHECK_EN);
}

static inline void hw_dec_data_crc32_set(unsigned int crc32)
{
	writel(crc32, HW_DEC_REG_BASE_ADDR + DPRS_DATA_CRC32);
}

static inline unsigned int hw_dec_buf_status_get(void)
{
	return readl(HW_DEC_REG_BASE_ADDR + BUF_INFO);
}

static inline unsigned int hw_dec_dprs_rtn_status_get(void)
{
	return readl(HW_DEC_REG_BASE_ADDR + DPRS_RTN_INFO);
}

static inline void hw_dec_buf_status_clr(void)
{
	writel(0x1, HW_DEC_REG_BASE_ADDR + BUF_INFO_CLR);
}

static inline void hw_dec_dprs_rtn_status_clr(void)
{
	writel(0x1, HW_DEC_REG_BASE_ADDR + RLT_INFO_CLR);
}

static void hw_dec_intr_en_set(int blk_intr_en, int task_intr_en)
{
	u_intr_en intr_en;
	intr_en.bits.task_intrpt_en = task_intr_en;
	intr_en.bits.block_intrpt_en = blk_intr_en;
	writel(intr_en.u32, HW_DEC_REG_BASE_ADDR + INT_EN_REG_ADDR);
}

static inline unsigned int hw_dec_intr_status_get(void)
{
	return readl(HW_DEC_REG_BASE_ADDR + INT_STATUS_REG_ADDR);
}

static void hw_dec_block_intr_status_clr(void)
{
	u_intr_clr intr_clr;

	intr_clr.u32 = readl(HW_DEC_REG_BASE_ADDR + INT_CLEAR_REG_ADDR);
	intr_clr.bits.block_intrpt_clr = 0x1;
	writel(intr_clr.u32, HW_DEC_REG_BASE_ADDR + INT_CLEAR_REG_ADDR);
}

static void hw_dec_task_intr_status_clr(void)
{
	u_intr_clr intr_clr;

	intr_clr.u32 = readl(HW_DEC_REG_BASE_ADDR + INT_CLEAR_REG_ADDR);
	intr_clr.bits.task_intrpt_clr = 0x1;
	writel(intr_clr.u32, HW_DEC_REG_BASE_ADDR + INT_CLEAR_REG_ADDR);
}

int hw_dec_intr_proc(int irq, const void *para)
{
	u_buf_status buf_status;
	u_intr_status intr_status;
	u_dprs_rtn_status dprs_status;
	int ret = 0;

	intr_status.u32 = hw_dec_intr_status_get();
	if (intr_status.bits.block_intrpt) {
		buf_status.u32 = hw_dec_buf_status_get();
		if (buf_status.bits.aval_flg)
			hw_dec_buf_status_clr();

		hw_dec_block_intr_status_clr();
	}

	if (intr_status.bits.task_intrpt) {
		dprs_status.u32 = hw_dec_dprs_rtn_status_get();
		if (dprs_status.bits.aval_flg) {
			if (dprs_status.bits.err_info)
				ret = -2; /* -2:failed */

			hw_dec_dprs_rtn_status_clr();
		}

		hw_dec_task_intr_status_clr();
		goto out;
	}

	ret = -1;
out:
	return ret;
}

void hw_dec_start(unsigned int src_baddr,
			unsigned int dst_baddr,
			unsigned int src_len,
			unsigned int dst_len,
			unsigned int crc_en,
			unsigned int crc32,
			unsigned int dec_type)
{
	unsigned int val;

	if (hw_dec_sop) {
		if (!dec_type) {
			/* set the parameters of output buffer */
			hw_dec_rtn_baddr_set(dst_baddr);
			hw_dec_data_rtn_len_set(dst_len);
		} else {
			/* set the parameter of output buffer */
			hw_dec_dprs_data_baddr_set(dst_baddr);
			hw_dec_dprs_data_len_set(page_nr(dst_len) * 4); /* 4:Align */
		}
	}

	/* set the parameter of input buffer */
	writel(src_baddr, HW_DEC_REG_BASE_ADDR + DPRS_DATA_SRC_BADDR);

	val = src_len |
		(hw_dec_eop << 28) | (hw_dec_sop << 29) | (!dec_type << 31); /* 28,29,31 Move Left bit */
	writel(val, HW_DEC_REG_BASE_ADDR + DPRS_DATA_SRC_LEN);

	hw_dec_crc_check_en(crc_en);
}

static inline void delay(unsigned int num)
{
	volatile unsigned int i;

	for (i = 0; i < (100 * num); i++) /* 100: Cycle */
		__asm__ __volatile__("nop");
}

int hw_dec_wait_finish(void)
{
	int ret;
	int times = 0;

	do {
		ret = hw_dec_intr_proc(HW_DEC_INTR, NULL);
		times++;
		if (times > 2000000) { /* 2000000 ms */
			error("hardware decompress overtime!\n");
			break;
		}
		delay(1);
	} while (-1 == ret);

	return ret;
}

int hw_dec_decompress(const unsigned char *dst, int* const dstlen,
			const unsigned char *src, int srclen,
			const void *unused)
{
	int ret;

	hw_dec_sop_eop_first_set(1);

	if (dstlen == NULL)
		return -1;

	hw_dec_start((unsigned int)(uintptr_t)src, (unsigned int)(uintptr_t)dst,
				 srclen, *dstlen, 1, 0, hw_dec_type);

	ret = hw_dec_wait_finish();

	*dstlen = readl(HW_DEC_REG_BASE_ADDR + DPRS_RTN_LEN);

	if (ret)
		return -1;

	return 0;
}

static int set_sop_and_eop(hw_decompress_op_type op_type)
{
	if (op_type == HW_DECOMPRESS_OP_START) {
		hw_dec_sop = 1;
		hw_dec_eop = 0;
	} else if (op_type == HW_DECOMPRESS_OP_CONTINUE) {
		hw_dec_sop = 0;
		hw_dec_eop = 0;
	} else if (op_type == HW_DECOMPRESS_OP_END) {
		hw_dec_sop = 0;
		hw_dec_eop = 1;
	} else if (op_type == HW_DECOMPRESS_OP_ONCE) {
		hw_dec_sop = 1;
		hw_dec_eop = 1;
	} else {
		return -1;
	}

	return 0;
}

/*
 * Support decompressing gzip file by sections
 */
int hw_dec_decompress_ex(hw_decompress_op_type op_type, const unsigned char *dst,
	int *dstlen, const unsigned char *src, int srclen)
{
	if ((dst == NULL) || (src == NULL) || (dstlen == NULL)) {
		return -1;
	}

	if ((*dstlen == 0) || (srclen == 0)) {
		return -1;
	}

	int ret = set_sop_and_eop(op_type);
	if (ret != 0) {
		return -1;
	}

	hw_dec_type = 0;
	const unsigned int crc_en = 1;
	hw_dec_start((unsigned int)(uintptr_t)src, (unsigned int)(uintptr_t)dst,
				 srclen, *dstlen, crc_en, 0, hw_dec_type);

	if ((op_type == HW_DECOMPRESS_OP_END) || (op_type == HW_DECOMPRESS_OP_ONCE)) {
		ret = hw_dec_wait_finish();
		*dstlen = readl(HW_DEC_REG_BASE_ADDR + DPRS_RTN_LEN);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

void hw_dec_init(void)
{
	/* enable decompress clock */
	enable_decompress_clock();
	/* Init the emar interface */
	writel(0, HW_DEC_REG_BASE_ADDR + EAMR_RID_REG_OFST);
	writel(0x3, HW_DEC_REG_BASE_ADDR + EAMR_ROSD_REG_OFST);
	writel(0, HW_DEC_REG_BASE_ADDR + EAMR_WID_REG_OFST);
	writel(0x3, HW_DEC_REG_BASE_ADDR + EAMR_WOSD_REG_OFST);

	/* Enable interrupt */
	hw_dec_intr_en_set(0x1, 0x1);

	/* Enable emar */
	hw_dec_work_en_set(0x1);
}

void hw_dec_uinit(void)
{
	hw_dec_work_en_set(0x0);
	hw_dec_intr_en_set(0x0, 0x0);

	/* disable decompress clock */
	disable_decompress_clock();
}

