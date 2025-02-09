/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include "flash_read.h"
#include <common.h>
#if defined (CONFIG_FMC_SPI_NOR)
#include <spi_flash.h>
#endif
#if defined (CONFIG_FMC_SPI_NAND)
#include <nand.h>
#endif
#if defined(CONFIG_SUPPORT_EMMC_BOOT)
#include <mmc.h>
#endif

/*
 * read from spi nor flash by offset and size.
 */
#if defined (CONFIG_FMC_SPI_NOR)
static int read_flash_spi(unsigned long offset, unsigned int size, unsigned char *addr)
{
	/* init env_flash */
	struct spi_flash *env_flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
		CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE);
	if (env_flash == NULL) {
		printf("read_flash_spi spi_flash_probe failed\n");
		return -1;
	}

	/* read flash */
	int ret = spi_flash_read(env_flash, offset, size, addr);
	if (ret != 0) {
		printf("read_flash_spi spi_flash_read failed\n");
		goto EXIT_FAILURE;
	}

EXIT_FAILURE:
	spi_flash_free(env_flash);
	return ret;
}
#else
static int read_flash_spi(unsigned long offset, unsigned int size, unsigned char *addr)
{
	printf("Error: read_flash_spi not define CONFIG_CMD_SF\n");
	return -1;
}
#endif

/*
 * read from nand flash by offset and size.
 */
#if defined (CONFIG_FMC_SPI_NAND)
static int read_flash_nand(unsigned long offset, unsigned int size, unsigned char *addr)
{
	struct mtd_info *mtd;
	size_t rwsize = size;
	size_t block_size;
	int ret;

	mtd = nand_info[0];
	if (mtd == NULL) {
		printf("read_flash_nand mtd == NULL\n");
		return -1;
	}
	block_size = mtd->erasesize;
	if (nand_block_isbad(mtd, offset))
		offset += block_size;

	ret = nand_read_skip_bad(mtd, offset, &rwsize, NULL, mtd->size, addr);
	return ret;
}
#else
static int read_flash_nand(unsigned long offset, unsigned int size, unsigned char *addr)
{
	printf("Error: read_flash_nand not define CONFIG_CMD_NAND\n");
	return -1;
}
#endif

/*
 * read from emmc flash by offset and size.
 */
#if defined (CONFIG_SUPPORT_EMMC_BOOT)
static int read_flash_mmc(unsigned long offset, unsigned int size, unsigned char *addr)
{
	struct mmc *mmc = find_mmc_device(0);
	if (mmc == NULL) {
		printf("read_flash_mmc find_mmc_device failed\n");
		return -1;
	}

	lbaint_t blks;
	if ((size % MMC_MAX_BLOCK_LEN) != 0)
		blks = size / MMC_MAX_BLOCK_LEN + 1;
	else
		blks = size / MMC_MAX_BLOCK_LEN;

	ulong blks_read = blk_dread(mmc_get_blk_desc(mmc), (offset / MMC_MAX_BLOCK_LEN), blks, (void *)addr);
	if (blks_read != blks) {
		printf("mmc blk read failed:%lu.\n", blks_read);
		return -1;
	}

	return 0;
}
#else
static int read_flash_mmc(unsigned long offset, unsigned int size, unsigned char *addr)
{
	printf("Error: read_flash_mmc not define CONFIG_CMD_NAND\n");
	return -1;
}
#endif

/*
 * read flash into out_addr from offset.
 */
int flash_read(unsigned long offset, unsigned int size, unsigned char *out_addr)
{
	if ((offset % FLASH_ALIGNED_SIZE) != 0) {
		printf("offset should aligned with %d.\n", FLASH_ALIGNED_SIZE);
		return -1;
	}

	if (out_addr == NULL) {
		return -1;
	}

	int (*read_flash_choose)(unsigned long offset, unsigned int size, unsigned char* addr) = NULL;

	switch (get_boot_media()) {
	case BOOT_MEDIA_EMMC:
		read_flash_choose = read_flash_mmc;
		break;
	case BOOT_MEDIA_NAND:
		read_flash_choose = read_flash_nand;
		break;
	case BOOT_MEDIA_SPIFLASH:
		read_flash_choose = read_flash_spi;
		break;
	default:
		break;
	}

	if (read_flash_choose == NULL) {
		printf("flash_read not found flash type!\n");
		return -1;
	}
	return read_flash_choose(offset, size, out_addr);
}

int flash_read_aligned(unsigned long offset, unsigned int size, unsigned char *out_addr)
{
	if (out_addr == NULL) {
		printf("out_addr is null.\n");
		return -1;
	}

	if ((size % FLASH_ALIGNED_SIZE) != 0) {
		printf("size should aligned with %d.\n", FLASH_ALIGNED_SIZE);
		return -1;
	}

	return flash_read(offset, size, out_addr);
}
