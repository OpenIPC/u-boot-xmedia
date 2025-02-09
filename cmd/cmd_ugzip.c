/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <common.h>
#include <memalign.h>
#include <hw_decompress.h>
#include <mmc.h>
#include "flash_read.h"
#include <asm/cache.h>
#include <usb.h>
#include <xmedia_timestamp.h>
#include <irq_func.h>
#include <dm/device.h>
#include <serial.h>
#include <linux/lhs_tags.h>
#include <linux/ctype.h>

/*
 * note: change it larger if rugzip cmd is abnormal
 */
#define RUGZIP_MIN_FRAGMENTATION_SIZE 0x20000
#define RUGZIP_READ_TMP_SIZE MMC_MAX_BLOCK_LEN

static int check_and_get_gzip_info(unsigned long offset, unsigned int *size)
{
	if (RUGZIP_READ_TMP_SIZE < GZIP_HEAD_SIZE) {
		printf("[error] tmp size is smaller than gzip head size!\n");
		return -1;
	}

	unsigned char *tmp_addr = memalign(16, RUGZIP_READ_TMP_SIZE); /* 16: aligned size */
	if (tmp_addr == NULL) {
		printf("[error] memalign 0x%x memeroy failed!\n", RUGZIP_READ_TMP_SIZE);
		return -1;
	}

	int ret = flash_read(offset, RUGZIP_READ_TMP_SIZE, tmp_addr);
	if (ret < 0) {
		printf("[error] read kernel header from flash failed!\n");
		return -1;
	}

	unsigned int magic_num0, magic_num1;
	magic_num0 = *(unsigned int *)(tmp_addr + HEAD_MAGIC_NUM0_OFFSET);
	magic_num1 = *(unsigned int *)(tmp_addr + HEAD_MAGIC_NUM1_OFFSET);
	if ((magic_num0 != HEAD_MAGIC_NUM0) || (magic_num1 != HEAD_MAGIC_NUM1)) {
		printf("[error] invalid gzip file(0x%x, 0x%x).\n", magic_num0, magic_num1);
		return -1;
	}

	unsigned int gzip_file_size;
	gzip_file_size = *(unsigned int *)(tmp_addr + COMPRESSED_SIZE_OFFSET);
	gzip_file_size += GZIP_HEAD_SIZE;
	if (gzip_file_size > GZIP_MAX_LEN) {
		printf("[error] gzip file(0x%x) greater than :0x%x.\n", gzip_file_size, GZIP_MAX_LEN);
		return -1;
	}

	*size = gzip_file_size;

	return 0;
}

static int read_and_ugzip_once(unsigned long offset, unsigned int gzip_file_size,
	unsigned char *src_addr, unsigned char *dst_addr)
{
	int ret = flash_read(offset, gzip_file_size, src_addr);
	if (ret < 0) {
		printf("[error] read gzip file failed(0x%x).\n", ret);
		return -1;
	}

	int comparessed_size = *(int *)(src_addr + COMPRESSED_SIZE_OFFSET);
	int uncomparessed_size = *(int *)(src_addr + UNCOMPRESSED_SIZE_OFFSET);
	hw_dec_init();
	ret = hw_dec_decompress_ex(HW_DECOMPRESS_OP_ONCE,
		dst_addr, &uncomparessed_size, src_addr + GZIP_HEAD_SIZE, comparessed_size);
	if (ret != 0) {
		printf("[error] decompress file failed.\n");
		hw_dec_uinit();
		return -1;
	}
	hw_dec_uinit();

	return 0;
}

static int read_and_ugzip_multi(unsigned long offset, unsigned int gzip_file_size,
	unsigned char *src_addr, unsigned char *dst_addr, unsigned int fragmentation_size)
{
	if (gzip_file_size <= fragmentation_size) {
		printf("[error] gzip file size should less than block size.\n");
		return CMD_RET_USAGE;
	}

	unsigned int cur_read_size = fragmentation_size;
	int ret = flash_read(offset, cur_read_size, src_addr);
	if (ret < 0) {
		printf("[error] read gzip file failed(0x%x).\n", ret);
		return CMD_RET_USAGE;
	}

	int size_uncomparessed = *(int *)(src_addr + UNCOMPRESSED_SIZE_OFFSET);

	hw_dec_init();
	ret = hw_dec_decompress_ex(HW_DECOMPRESS_OP_START,
		dst_addr, &size_uncomparessed, src_addr + GZIP_HEAD_SIZE, cur_read_size - GZIP_HEAD_SIZE);
	if (ret != 0) {
		printf("[error] [%s] [%d]decompress file failed.\n", __func__, __LINE__);
		hw_dec_uinit();
		return CMD_RET_USAGE;
	}

	hw_decompress_op_type op_type;
	unsigned int cur_offset = cur_read_size;
	while (cur_offset < gzip_file_size) {
		if ((cur_offset + fragmentation_size) < gzip_file_size) {
			cur_read_size = fragmentation_size;
			op_type = HW_DECOMPRESS_OP_CONTINUE;
		} else {
			cur_read_size = gzip_file_size - cur_offset;
			op_type = HW_DECOMPRESS_OP_END;
		}

		ret = flash_read(offset + cur_offset, cur_read_size, src_addr + cur_offset);
		if (ret < 0) {
			printf("[error] read compress file failed(0x%x).\n", ret);
			hw_dec_uinit();
			return CMD_RET_USAGE;
		}

		ret = hw_dec_decompress_ex(op_type,
			dst_addr, &size_uncomparessed, src_addr + cur_offset, cur_read_size);
		if (ret != 0) {
			printf("[error] [%s] [%d]decompress file failed.\n", __func__, __LINE__);
			hw_dec_uinit();
			return CMD_RET_USAGE;
		}

		cur_offset += cur_read_size;
	}

	hw_dec_uinit();

	return 0;
}

/* Copy from bootm_disable_interrupts() */
static unsigned long boothz_disable_interrupts(void)
{
	unsigned long iflag;

	/*
	 * We have reached the point of no return: we are going to
	 * overwrite all exception vector code, so we cannot easily
	 * recover from any failures any more...
	 */
	iflag = disable_interrupts();
#ifdef CONFIG_NETCONSOLE
	/* Stop the ethernet stack if NetConsole could have left it up */
	eth_halt();
# ifndef CONFIG_DM_ETH
	eth_unregister(eth_get_dev());
# endif
#endif

#if defined(CONFIG_CMD_USB)
	/*
	 * turn off USB to prevent the host controller from writing to the
	 * SDRAM while Linux is booting. This could happen (at least for OHCI
	 * controller), because the HCCA (Host Controller Communication Area)
	 * lies within the SDRAM and the host controller writes continously to
	 * this area (as busmaster!). The HccaFrameNumber is for example
	 * updated every 1 ms within the HCCA structure in SDRAM! For more
	 * details see the OpenHCI specification.
	 */
	usb_stop();
#endif
	return iflag;
}

void boot_kernel(unsigned long kernel_zreladdr, unsigned long fdt)
{
	bd_t *bd = gd->bd;
	int machid = 0;
	void (*kernel_entry)(int zero, int arch, uint params);
	unsigned long r2;
	char *show_timestamp;

	TIME_STAMP(0);
	boothz_disable_interrupts();

	machid = bd->bi_arch_number;

	/* FDT must be 4bytes-aligned */
	if (fdt & 0x3)
		fdt = (fdt + 0x3) & ~0x3;

	if (fdt_magic((unsigned int *)fdt) != FDT_MAGIC) {
		printf("Invalid dtb\n");
		printf("fdt addr=0x%lX, MAGIC=0x%x\n", fdt, fdt_magic((unsigned int *)fdt));
		goto error;
	}
	setup_atags((char *)fdt, fdt_totalsize(fdt));

	r2 = fdt;
	kernel_entry = (void (*)(int, int, uint))kernel_zreladdr;

	TIME_STAMP(0);
	stopwatch_trigger();
	show_timestamp = env_get("timestamp");
	if (*show_timestamp == 'y') {
		serial_enable_output(true);
		timestamp_print(0);
		stopwatch_print();
	}

	kernel_entry(0, machid, r2);

	return;

	error:
		enable_interrupts();
	return;
}

static int do_ugzip(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc != 3)
		return CMD_RET_USAGE;

	uintptr_t src = simple_strtoul(argv[1], NULL, 16); /* 1: src argc, 16: base */
	uintptr_t dst = simple_strtoul(argv[2], NULL, 16); /* 2: dst argc, 16: base */

	if (src & 0XF) {
		printf("[error] src[0X%08lx] is not 16Byte-aligned!\n", src);
		return CMD_RET_USAGE;
	}
	if (dst & 0XF) {
		printf("[error] dst[0X%08lx] is not 16Byte-aligned!\n", dst);
		return CMD_RET_USAGE;
	}

	unsigned int magic_num0 = *(unsigned int *)(src + HEAD_MAGIC_NUM0_OFFSET);
	unsigned int magic_num1 = *(unsigned int *)(src + HEAD_MAGIC_NUM1_OFFSET);
	if ((magic_num0 != HEAD_MAGIC_NUM0) || (magic_num1 != HEAD_MAGIC_NUM1)) {
		printf("[error] The magic numbers are not correct!\n"\
			   "    Please check the source data!\n");
		return CMD_RET_USAGE;
	}
	int size_comparessed = *(int *)(src + COMPRESSED_SIZE_OFFSET);
	int size_uncomparessed = *(int *)(src + UNCOMPRESSED_SIZE_OFFSET);

	dcache_disable(); /* Recommend to disable dcache */
	hw_dec_init();
	int ret = hw_dec_decompress_ex(HW_DECOMPRESS_OP_ONCE, (unsigned char *)dst,
		&size_uncomparessed, (unsigned char *)(src + GZIP_HEAD_SIZE), size_comparessed);
	if (ret != 0) {
		printf("[error] decompress fail!\n");
		hw_dec_uinit();
		return CMD_RET_USAGE;
	}
	hw_dec_uinit();

	return 1;
}

static int do_read_gzip(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 3) {
		return CMD_RET_USAGE;
	}

	/* 1: offset argc, 16: base */
	unsigned long offset = (unsigned long)simple_strtoul(argv[1], NULL, 16);
	/* 2: dst_addr argc, 16: base */
	unsigned char *dst_addr = (unsigned char *)simple_strtoul(argv[2], NULL, 16);

	unsigned int gzip_file_size;
	int ret = check_and_get_gzip_info(offset, &gzip_file_size);
	if (ret != 0) {
		printf("[error] get gzip file size failed.\n");
		return CMD_RET_USAGE;
	}

	ret = flash_read(offset, gzip_file_size, dst_addr);
	if (ret < 0) {
		printf("[error] read gzip file failed!\n");
		return CMD_RET_USAGE;
	}

	return 0;
}

/*
 * read gzip file from flash and uncomress it
 * arg:
 *     offset: gzip file offset in flash
 *     compress_addr: gzip file addr in ddr
 *     uncompress_addr: uncompress file addr in ddr
 *     fragmentation_size: fragmentation size for reading once
 */
static int do_read_and_ugzip(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 5) {
		return CMD_RET_USAGE;
	}

	/* 1: offset argc, 16: base */
	unsigned long offset = (unsigned long)simple_strtoul(argv[1], NULL, 16);
	/* 2: compress_addr argc, 16: base */
	unsigned char *compress_addr = (unsigned char *)(uintptr_t)simple_strtoul(argv[2], NULL, 16);
	/* 3: uncompress_addr argc, 16: base */
	unsigned char *uncompress_addr = (unsigned char *)(uintptr_t)simple_strtoul(argv[3], NULL, 16);
	/* 4: fragmentation_size argc, 16: base */
	unsigned int fragmentation_size = (unsigned int)(uintptr_t)simple_strtoul(argv[4], NULL, 16);
	if (fragmentation_size < RUGZIP_MIN_FRAGMENTATION_SIZE) {
		printf("[error] fragmentation_size less than 0x%x!\n", RUGZIP_MIN_FRAGMENTATION_SIZE);
		return CMD_RET_USAGE;
	}

	if ((fragmentation_size % RUGZIP_MIN_FRAGMENTATION_SIZE) != 0) {
		printf("[error] fragmentation_size should been aligned with 0x%x!\n", RUGZIP_MIN_FRAGMENTATION_SIZE);
		return CMD_RET_USAGE;
	}

	unsigned int gzip_file_size;
	int ret = check_and_get_gzip_info(offset, &gzip_file_size);
	if (ret != 0) {
		printf("[error] gzip file is invalid!\n");
		return CMD_RET_USAGE;
	}

	dcache_disable(); /* Recommend to disable dcache */
	if (gzip_file_size <= fragmentation_size) {
		ret = read_and_ugzip_once(offset, gzip_file_size, compress_addr, uncompress_addr);
	} else {
		ret = read_and_ugzip_multi(offset, gzip_file_size,
			compress_addr, uncompress_addr, fragmentation_size);
	}
	if (ret != 0) {
		printf("[error] read and decompress failed!\n");
		return CMD_RET_USAGE;
	}

	return 0;
}

static int read_dtb(unsigned long offset, unsigned int image_size,
	unsigned long src_addr, unsigned int gzip_file_size)
{
	int ret;

	if (gzip_file_size & (MMC_MAX_BLOCK_LEN - 1)) /* 512Byte-aligned */
		gzip_file_size = gzip_file_size & ~(MMC_MAX_BLOCK_LEN - 1);

	ret = flash_read(offset + gzip_file_size, image_size - gzip_file_size,
						(unsigned char *)(src_addr + gzip_file_size));
	if (ret < 0) {
		printf("[error] read DTB file failed(0x%x).\n", ret);
		return CMD_RET_USAGE;
	}
	return 0;
}

static int do_boothz(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char *src_addr = NULL;
	unsigned char *dst_addr = NULL;
	unsigned long offset;
	unsigned int image_size;
	unsigned int fragmentation_size;
	unsigned int gzip_file_size;
	int ret;
	char *sub_cmd = NULL;

#define DEFAULT_CHUNK_SIZE 0x40000
	switch (argc) {
		case 5:
			fragmentation_size = DEFAULT_CHUNK_SIZE;
			break;
		case 6:
			if (isdigit(*(argv[5]))) {
				fragmentation_size = \
					(unsigned int)(uintptr_t)simple_strtoul(argv[5], NULL, 16);
			} else {
				fragmentation_size = DEFAULT_CHUNK_SIZE;
				sub_cmd = argv[5];
			}
			break;
		case 7:
			if (isdigit(*(argv[5])) || !isdigit(*(argv[6]))) {
				return CMD_RET_USAGE;
			}
			sub_cmd = argv[5];
			fragmentation_size = \
				(unsigned int)(uintptr_t)simple_strtoul(argv[6], NULL, 16);
		default:
			return CMD_RET_USAGE;
	}

	src_addr = (unsigned char *)(uintptr_t)simple_strtoul(argv[1], NULL, 16);
	dst_addr = (unsigned char *)(uintptr_t)simple_strtoul(argv[2], NULL, 16);
	offset = (unsigned long)simple_strtoul(argv[3], NULL, 16);
	image_size = (unsigned long)simple_strtoul(argv[4], NULL, 16);

	if (fragmentation_size < RUGZIP_MIN_FRAGMENTATION_SIZE) {
		printf("[error] fragmentation_size less than 0x%x!\n", RUGZIP_MIN_FRAGMENTATION_SIZE);
		return CMD_RET_USAGE;
	}

	if ((fragmentation_size % RUGZIP_MIN_FRAGMENTATION_SIZE) != 0) {
		printf("[error] fragmentation_size should been aligned with 0x%x!\n", RUGZIP_MIN_FRAGMENTATION_SIZE);
		return CMD_RET_USAGE;
	}

	ret = check_and_get_gzip_info(offset, &gzip_file_size);
	if (ret != 0) {
		printf("[error] gzip file is invalid!\n");
		return CMD_RET_USAGE;
	}

	dcache_disable(); /* Recommend to disable dcache */
	if (gzip_file_size <= fragmentation_size) {
		ret = read_and_ugzip_once(offset, gzip_file_size, src_addr, dst_addr);
	} else {
		ret = read_and_ugzip_multi(offset, gzip_file_size,
			src_addr, dst_addr, fragmentation_size);
	}
	if (ret != 0) {
		printf("[error] read and decompress failed!\n");
		return CMD_RET_USAGE;
	}

	ret = read_dtb(offset, image_size, (unsigned long)src_addr, gzip_file_size);
	if (ret != 0) {
		return CMD_RET_USAGE;
	}

	if (sub_cmd)
		run_command(sub_cmd, 0);

	boot_kernel((unsigned long)dst_addr, (unsigned long)src_addr + gzip_file_size);

	return 0;
}

U_BOOT_CMD(
	rgzip,  3,  0,  do_read_gzip,
	"read gzip file to a memory region, eg:[rgzip offset compress_addr]",
	"offset dstaddr\n"
);

U_BOOT_CMD(
	ugzip,  3,  0,  do_ugzip,
	"uncompress file with hardware IP, eg:[ugzip compress_addr uncompress_addr]",
	"ugzip <src> <dst>\n"
	"src and dst must be 16Byte-aligned"
);

U_BOOT_CMD(
	rugzip,  5,  0,  do_read_and_ugzip,
	"read and uncompress file, eg:[rugzip offset compress_addr uncompress_addr fragmentation_size]",
	"rugzip offset src dst fragmentation_size\n"
	"src and dst must be 16Byte-aligned"
);

U_BOOT_CMD(
	boothz,  7,  0,  do_boothz,
	"Read and decompress image by hardware: boothz <src> <dst> <flash offset> <size> [sub command | chunk size]",
	"<src> <dst> <flash offset> <size> [sub command | chunk size]\n"
	"       <src> <dst> <flash offset> <size> <sub command> <chunk size>\n"
 	"    - Read image from <flash offset> to <src>, and decompress to <dst> by hardware decompressed module.\n"
	"      eg:\n"
	"        boothz 0x41000000 0x40008000 0x100000 0x400000\n"
	"        boothz 0x41000000 0x40008000 0x100000 0x400000 xmediaapp\n"
	"        boothz 0x41000000 0x40008000 0x100000 0x400000 0x40000\n"
	"        boothz 0x41000000 0x40008000 0x100000 0x400000 xmediaapp 0x40000\n"
	"      <src> and <dst> is DDR location, and must be 16Byte-aligned;\n"
	"      <flash offset> and <size> must be flash-block-size-aligned;\n"
	"      <chunk size> is one step size for reading and decompressing one time;\n"
	"      <sub command> is a command that will be run after decompressing image and before going to kernel."
);
