/******************************************************************************
* File:             hwzimage.c
*
* Author:           Lynn
* Created:          12/08/22
* Description:
*   This module is to support "go <hwzImage-dtb>", hwzImage-dtb = hwzImage + dtb
*   To create hwzImage-dtb: 1.hwgzip -c Image > hwzImage_noaligned;(or old command:./bsp_gzip Image hwzImage;)
*                          2. dd if=hwzImage_noaligned of=hwzImage bs=4 conv=sync
*                          3. cat hwzImage xm72050200-demb.dtb > hwzImage-dtb
*   The hwzImage-dtb format:
*   |--------------------------------------------------------------------------------------------|    0
*   |HW GZIP HEAD, 16 bytes:                                                                     |
*   |     4bytes             4bytes                  4bytes                     4bytes           |
*   |compressed_size ,  decompressed_size ,   magic_num0: 0x70697A67,   magci_num1: 0x64616568   |
*   |--------------------------------------------------------------------------------------------|    16bytes
*   |HW zImage Body, compressed_size bytes                                                       |
*   |--------------------------------------------------------------------------------------------|    16bytes + compressed_size + 4bytes align pad
*   |The fdt dtb data                                                                            |
*   |--------------------------------------------------------------------------------------------|
*****************************************************************************/

#include <common.h>
#include <command.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/libfdt.h>
#include <image.h>
#include <asm/cache.h>
#include <bootm.h>
#include <usb.h>
#include <hw_decompress.h>
#include <xmedia_timestamp.h>
#include <irq_func.h>
#include <dm/device.h>
#include <dm/root.h>
#include <asm/setup.h>
#include <linux/sizes.h>
#include <serial.h>

#include <linux/lhs_tags.h>

/************************************************************************************************/
//#define CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT
#ifdef CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT
#ifndef CONFIG_OF_LIBFDT
	#error "CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT depend on CONFIG_OF_LIBFDT"
#endif /* ifndef CONFIG_OF_LIBFDT */
#endif /* ifdef CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT */

//#define DEBUG_HWZIMAGE
#ifdef DEBUG_HWZIMAGE
#define HWZIMAGE_DBG(fmt, args...) printf("## HW zImage: "fmt, ##args)
#else
#define HWZIMAGE_DBG(fmt, args...)
#endif
#define HWZIMAGE_ERR(fmt, args...) printf("## HW zImage ERR: "fmt, ##args)

#define KENREL_ZRELADDR             0x40008000

#define FOUR_BYTES_ALIGNED	    0x3

DECLARE_GLOBAL_DATA_PTR;

/************************************************************************************************/

bool check_hwzimage_header(unsigned char *header)
{
	unsigned int magic_num0 = *((unsigned int *)(header + HEAD_MAGIC_NUM0_OFFSET));
	unsigned int magic_num1 = *((unsigned int *)(header + HEAD_MAGIC_NUM1_OFFSET));
	if ((magic_num0 != HEAD_MAGIC_NUM0) || (magic_num1 != HEAD_MAGIC_NUM1))
		return false;

	return true;
}


/******************************************************************************
* Function:         static int hwzimage_hw_decompress
* Description:      Decompress hwzimage and return the FDT
* In:
*                   ulong src - location of hwzimage
*                   ulong dst - the destination decompressed to
* Out:
*                   ulong *fdt_addr - FDT Address get from hwzimage
*
* Return:           0 - Success
*                   1 - Failed
*****************************************************************************/
static int hwzimage_hw_decompress(ulong src, ulong dst, ulong *fdt_addr)
{
	int size_comparessed, size_uncomparessed;
	unsigned int magic_num0, magic_num1;
	ulong fdt;
	int ret = 0;

	dcache_disable();

	if (src & 0xF) {
		HWZIMAGE_ERR("src[0x%08lx] is not 16Bytes-aligned!\n", src);
		return 1;
	}
	if (dst & 0xF) {
		HWZIMAGE_ERR("dst[0x%08lx] is not 16Bytes-aligned!\n", dst);
		return 1;
	}

	magic_num0 = *(unsigned int *)(src + HEAD_MAGIC_NUM0_OFFSET);
	magic_num1 = *(unsigned int *)(src + HEAD_MAGIC_NUM1_OFFSET);
	if ((magic_num0 != HEAD_MAGIC_NUM0) || (magic_num1 != HEAD_MAGIC_NUM1)) {
		HWZIMAGE_ERR("The magic numbers are not correct!\n"\
			   "    Please check the source data!\n");
		return 1;
	}

	size_comparessed = *(int *)(src + COMPRESSED_SIZE_OFFSET);
	size_uncomparessed = *(int *)(src + UNCOMPRESSED_SIZE_OFFSET);

	/* Use direct address mode */
	hw_dec_type = 0;

	/* Init hardware decompress controller */
	hw_dec_init();

	/* Start decompressing */
	ret = hw_dec_decompress((unsigned char *)dst, &size_uncomparessed, \
					(unsigned char *)(src + GZIP_HEAD_SIZE), \
					size_comparessed, NULL);
	if (ret)
		HWZIMAGE_ERR("Decompress fail!\n");

	/* Deinit hardware decompress controller */
	hw_dec_uinit();

	/* Find out DTB location in the image */
	fdt = src + size_comparessed + GZIP_HEAD_SIZE;
	if (fdt & FOUR_BYTES_ALIGNED)
		fdt = (fdt + FOUR_BYTES_ALIGNED) & ~FOUR_BYTES_ALIGNED;

	if (FDT_MAGIC != fdt_magic((unsigned int *)fdt)) {
		HWZIMAGE_ERR("Invalid dtb\n");
		HWZIMAGE_ERR("     fdt addr=0x%lX, MAGIC=0x%x\n", fdt, fdt_magic((unsigned int *)fdt));
		return 1;
	}

	*fdt_addr = fdt;

	HWZIMAGE_DBG("Compress size %d, Decompress_size %d, FDT addr 0x%08lx \n",
				size_comparessed, size_uncomparessed, *fdt_addr);

	dcache_enable();

	return ret;
}


#ifdef CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT
static int setup_libfdt(void *blob, int of_size)
{
	int ret = -EPERM;

	if (fdt_root(blob) < 0) {
		HWZIMAGE_ERR("Root node setup failed\n");
		goto err;
	}
	if (fdt_chosen(blob) < 0) {
		HWZIMAGE_ERR("/chosen node create failed\n");
		goto err;
	}
	return 0;
err:
	return ret;
}
#endif // CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT

/* Copy from announce_and_cleanup() */
void hwzimage_cleanup_before_linux(void)
{
#ifdef CONFIG_USB_DEVICE
	udc_disconnect();
#endif

	/*
	 * Call remove function of all devices with a removal flag set.
	 * This may be useful for last-stage operations, like cancelling
	 * of DMA operation or releasing device internal buffers.
	 */
	dm_remove_devices_flags(DM_REMOVE_ACTIVE_ALL);

	cleanup_before_linux();
}

/* Copy from bootm_disable_interrupts() */
ulong hwzimage_disable_interrupts(void)
{
	ulong iflag;

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

int do_go_hwzimage(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong	addr;
	ulong	kernel_entry_point = KENREL_ZRELADDR;
	int     ret = 0;
	int	machid = 0;
	ulong	fdt_addr = 0;
	ulong	fdt_image_addr = 0;
	ulong   r2;
	char    *show_timestamp;
#ifdef CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT
	ulong	fdt_len = SZ_16K;
	char	*fdt_blob = NULL;
#endif /* CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT */
	bd_t	*bd = gd->bd;
	void (*kernel_entry)(int zero, int arch, uint params);

	if (argc < 2 || argc >= 3)
		return CMD_RET_USAGE;

#ifdef CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT
	if(IMAGE_ENABLE_OF_LIBFDT == 0) {
		HWZIMAGE_ERR("U-boot not support FDT \n");
		return 0;
	}
#endif

	TIME_STAMP(0);

	hwzimage_disable_interrupts();

	addr = simple_strtoul(argv[1], NULL, 16);
	machid = bd->bi_arch_number;

	/* Decompress zImage from addr to kernel_entry_point(zreladdr) */
	ret = hwzimage_hw_decompress(addr, kernel_entry_point, &fdt_addr);
	if (ret) {
		HWZIMAGE_ERR("Decompress fail\n");
		goto error;
	}

	/* FDT must be 4bytes-aligned */
	if (fdt_addr & FOUR_BYTES_ALIGNED) {
		HWZIMAGE_ERR("Invalid fdt address 0x%lX, MUST be 4 bytes aligned\n", fdt_addr);
		goto error;
	}
	else {
		r2 = fdt_image_addr = fdt_addr;
		HWZIMAGE_DBG("FDT image addr 0x%08lx do not move as well\n", fdt_image_addr);
	}

#ifdef CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT
	fdt_blob = (void *)((unsigned long) fdt_image_addr);
	HWZIMAGE_DBG("FDT blob  %p \n", fdt_blob);
	if ( fdt_check_header(fdt_blob) != 0 ) {
		HWZIMAGE_ERR("Image is not a fdt \n");
		goto error;
	}

	fdt_len = fdt_totalsize(fdt_blob);
	HWZIMAGE_DBG("FDT length 0x%lu \n", fdt_len);
	if ((fdt_len == 0) || (fdt_len > SZ_16K)) {
		HWZIMAGE_ERR("FDT image is empty or larger than 16KB %lu\n", fdt_len);
		goto error;
	}

	/* Increase some bytes for bootargs/chosen */
	fdt_increase_size(fdt_blob, 512);
	fdt_len = fdt_totalsize(fdt_blob);
	HWZIMAGE_DBG("Increase FDT length to 0x%lx \n", fdt_len);
	ret = setup_libfdt(fdt_blob, fdt_len);
	if (ret) {
		HWZIMAGE_ERR("FDT image setup libfdt fail \n");
		goto error;
	}
#else
	setup_atags((char *)fdt_image_addr, fdt_totalsize(fdt_image_addr));
	r2 = fdt_image_addr;
#endif /* CONFIG_SETUP_BOOT_PARAMS_BY_LIBFDT */


	kernel_entry = (void (*)(int, int, uint))kernel_entry_point;

	HWZIMAGE_DBG("Decompress hwzimage from 0x%08lx to 0x%08lx,\n \
		machid %d , fdt 0x%08lx, r2 0x%08lx\n",
		addr, kernel_entry_point, machid, fdt_image_addr, r2);

	TIME_STAMP(0);
	stopwatch_trigger();

	show_timestamp = env_get("timestamp");
	if (*show_timestamp == 'y') {
		serial_enable_output(true);
		timestamp_print(0);
		stopwatch_print();
	}

	hwzimage_cleanup_before_linux();

	kernel_entry(0, machid, r2);

error:
	enable_interrupts();

	return -1;
}

/************************************************************************************************/
U_BOOT_CMD(
	gohwzimage, CONFIG_SYS_MAXARGS, 1,	do_go_hwzimage,
	"Start hwzImage(Hardware Compressed Linux Kernel Image)",
	"goimage <hwzimage addr> \n \
	- <hwzimage addr> must not be zreladdr \n"
);


