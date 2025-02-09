/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <common.h>
#include <compiler.h>
#include <hw_decompress.h>
#include <securec.h>

#define CYCLE_MAGIC_HEAD        0x6379636c

#define CYCLE_MAGIC_ITEM_START  0x63796373

#define CYCLE_MAGIC_ITEM_END    0x63796365

/** Cycle Head Infomation */
typedef struct cycle_head_s {
	unsigned int u32magichead;
	unsigned int u32cycleflashsize; /* <include cycle_head size */
	unsigned int u32compress : 2;
	unsigned int u32writeflag : 3;
	unsigned int u32reserved : 27;
	unsigned int u32alignsize;
} cycle_head_s;

/** Cycle Item Infomation */
typedef struct cycle_item_start_s {
	unsigned int u32magicitemstart;
	unsigned int u32itemlen;
	unsigned int u32itemalllen;
	unsigned int u32itemoriginlen;
} cycle_item_start_s;

#define DIVIDE 3
#define BYTE_ALIGN ((unsigned int)16)  /* <needed by decompress */

#define cycle_err(fmt, args...) printf("ERR: <%s:%d> "fmt, __FUNCTION__, __LINE__, ##args)
#define cycle_dbg(fmt, args...) printf("DBG: <%s:%d> "fmt, __FUNCTION__, __LINE__, ##args)

static int cycle_get_initdata(unsigned long ulsrc, unsigned int u32srclen,
				cycle_item_start_s **ppstItem,
				unsigned int *pu32compress)
{
	cycle_head_s *psthead = NULL;
	cycle_item_start_s *pstitem = NULL;
	unsigned int u32itemmagicend;
	unsigned long ulbuffer = ulsrc;

	/* Cycle Head */
	psthead = (cycle_head_s *)ulsrc;

	if ((psthead->u32magichead !=  CYCLE_MAGIC_HEAD) ||
			(psthead->u32cycleflashsize > u32srclen) ||
			(psthead->u32alignsize == 0)) {
		cycle_err("psthead->u32magichead: %#x\n", psthead->u32magichead);
		cycle_err("SrcLen[%u] CycleFlashSize[%u]\n", u32srclen,
				  psthead->u32cycleflashsize);
		return -1;
	}

	/* Compress Type */
	*pu32compress = psthead->u32compress;
	cycle_dbg("Compress[%u]\n", *pu32compress);

	/* First Item */
	ulbuffer += sizeof(cycle_head_s);
	ulbuffer = (ulbuffer + psthead->u32alignsize - 1) &
			   ~(psthead->u32alignsize - 1);

	pstitem = (cycle_item_start_s *)ulbuffer;
	if ((pstitem->u32itemlen == 0) ||
		((pstitem->u32itemlen % BYTE_ALIGN) != 0)) {
		cycle_err("pstitem->u32itemlen: %#x\n", pstitem->u32itemlen);
		return -1;
	}

	u32itemmagicend = *(unsigned int *)(uintptr_t)(ulbuffer +
			(pstitem->u32itemlen + sizeof(cycle_item_start_s)));
	if ((pstitem->u32magicitemstart != CYCLE_MAGIC_ITEM_START) ||
		(pstitem->u32itemalllen >= (psthead->u32cycleflashsize / DIVIDE)) ||
		(u32itemmagicend != CYCLE_MAGIC_ITEM_END)) {
			cycle_err("Item MagicStart[%#x] Len[%u] MagicEnd[%#x] CycleFlashSize[%u]\n",
			pstitem->u32magicitemstart, pstitem->u32itemlen, u32itemmagicend,
			psthead->u32cycleflashsize);
		return -1;
	}

	while (1) {
		/* update item to last valid one */
		*ppstItem = (cycle_item_start_s *)ulbuffer;

		/* check next item valid or not */
		ulbuffer += pstitem->u32itemalllen;

		pstitem = (cycle_item_start_s *)ulbuffer;

		/* <check buffer finish */
		if (ulbuffer - ulsrc >= psthead->u32cycleflashsize)
			return 0;

		if (pstitem->u32magicitemstart != CYCLE_MAGIC_ITEM_START) {
			/* <unused zone, use previous item */
			if (pstitem->u32magicitemstart == 0xffffffff) {
				return 0;
			} else {
				cycle_err(
					"pstitem->u32magicitemstart(0x%x) wrong!\n",
					pstitem->u32magicitemstart);
				return -1;
			}
		}

		u32itemmagicend = *(unsigned int *)(uintptr_t)(ulbuffer +
					(pstitem->u32itemlen + sizeof(cycle_item_start_s)));
		/* <item data damaged */
		if ((pstitem->u32itemlen >= (psthead->u32cycleflashsize / DIVIDE)) ||
			(u32itemmagicend != CYCLE_MAGIC_ITEM_END)) {
			cycle_err("\n");
			return -1;
		}
	}

	return -1;
}

static int cycle_get_data(unsigned long ulsrc, unsigned int u32srclen, unsigned long uldst)
{
	int s32ret;

	cycle_item_start_s *pstitem = NULL;
	unsigned long  ulitemdata = 0;
	unsigned int ncompressed = 0;

	s32ret = cycle_get_initdata(ulsrc, u32srclen, &pstitem, &ncompressed);
	if ((s32ret == 0) && pstitem) {
		ulitemdata += (uintptr_t)pstitem + sizeof(cycle_item_start_s);

		if (ncompressed) {
			hw_dec_type = 0; /**<use direct address mode*/
			hw_dec_init(); /**<init hw decompress IP*/

			int s32sizecompressed = pstitem->u32itemoriginlen;
			s32ret = hw_dec_decompress_ex(HW_DECOMPRESS_OP_ONCE, (const unsigned char *)(uintptr_t)uldst,
				&s32sizecompressed, (const unsigned char *)(uintptr_t)ulitemdata, pstitem->u32itemlen);
			if (s32ret == 0 && s32sizecompressed == pstitem->u32itemoriginlen) {
				cycle_dbg("decompress ok!\n");
				s32ret = 0;
			} else {
				(void)memset_s((void *)(uintptr_t)uldst, 16, 0, 16); /* 16 byte */
				cycle_err("decompress fail[%#x]! uncompress size[%#x]\n",
					s32ret, s32sizecompressed);
				s32ret = -1;
			}

			hw_dec_uinit(); /**<uinit hw decompress IP*/
		} else {
			if (memcpy_s((void *)(uintptr_t)uldst, 16, (void *)(uintptr_t)ulitemdata,  /* 16 byte */
					pstitem->u32itemlen)) {
				cycle_err("memcpy_s  err : %s %d.\n", __func__, __LINE__);
				s32ret = -1;
			}
			s32ret = 0;
		}
	} else {
		(void)memset_s((void *)(uintptr_t)uldst, 16, 0, 16); /* 16 byte */
		cycle_err("Failed to get cycle data. dst: 0x%lx\n", uldst);
		s32ret = -1;
	}

	return s32ret;
}

int getcycledata(unsigned long ulsrc, unsigned long ulsrcbak, unsigned int u32srclen,
						   unsigned long uldst)
{
	int s32ret = cycle_get_data(ulsrc, u32srclen, uldst);
	if ((s32ret == -1) && (ulsrcbak != 0))
		s32ret = cycle_get_data(ulsrcbak, u32srclen, uldst);

	return s32ret;
}

static int do_cycle(cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
	unsigned long  ulsrc;
	unsigned long  ulsrcbak;
	unsigned long  uldst;
	unsigned int u32srclen;

	/* Check Input Args Count : four arguments needed */
	if (argc != 5)
		goto usage;

	ulsrc     = simple_strtoul(argv[1], NULL, 16); /* 16 byte */
	ulsrcbak  = simple_strtoul(argv[2], NULL, 16); /* 16 byte */
	u32srclen = simple_strtoul(argv[3], NULL, 16); /* 16 byte */
	uldst     = simple_strtoul(argv[4], NULL, 16); /* 16 byte */

	if (ulsrc & 0XF) {
		printf("ERR:\n  src[0X%08lx] is not 16Byte-aligned!\n", ulsrc);
		return 1;
	}

	if (ulsrcbak & 0XF) {
		printf("ERR:\n  src_backup[0X%08lx] is not 16Byte-aligned!\n", ulsrcbak);
		return 1;
	}

	if (u32srclen & 0XFFFF) {
		printf("ERR:\n  src_len[0X%08x] is not 0x10000Byte-aligned!\n", u32srclen);
		return 1;
	}

	if (uldst & 0XF) {
		printf("ERR:\n  dst[0X%08lx] is not 16Byte-aligned!\n", uldst);
		return 1;
	}

	return getcycledata(ulsrc, ulsrcbak, u32srclen, uldst);

usage:
	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(
	cread,  5,  1,  do_cycle,
	"get valid data from cycle_data buffer. 'cycle <src> <src_backup>  <src_len> <dst>'",
	"1. src_backup can be 0.  2. if src and src_backup are wrong, "
	"dst head (16 byte) will be set to 0.  3. src and dst must be 16Byte-aligned"
);

