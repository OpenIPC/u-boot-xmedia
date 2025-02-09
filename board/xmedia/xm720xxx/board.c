/*
 * Copyright (c) XMEDIA. All rights reserved.
 */
#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/platform.h>
#include <spi_flash.h>
#include <linux/mtd/mtd.h>
#include <nand.h>
#include <netdev.h>
#include <mmc.h>
#include <asm/sections.h>
#include <sdhci.h>
#include <cpu_common.h>
#include <asm/mach-types.h>

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif
static int boot_media = BOOT_MEDIA_UNKNOWN;
int get_boot_media(void)
{
	unsigned int reg_val, boot_mode, spi_device_mode;
	int boot_media;

	reg_val = readl(SYS_CTRL_REG_BASE + REG_SYSSTAT);
	boot_mode = get_sys_boot_mode(reg_val);

	switch (boot_mode) {
	case BOOT_FROM_SPI:
		spi_device_mode = get_spi_device_type(reg_val);
		if (spi_device_mode)
			boot_media = BOOT_MEDIA_NAND;
		else
			boot_media = BOOT_MEDIA_SPIFLASH;
		break;
	case BOOT_FROM_EMMC:
		boot_media = BOOT_MEDIA_EMMC;
		break;
	default:
		boot_media = BOOT_MEDIA_UNKNOWN;
		break;
	}

	return boot_media;
}

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
}
#endif

static inline void delay(unsigned long loops)
{
	__asm__ volatile("1:\n"
			"subs %0, %1, #1\n"
			"bne 1b" : "=r"(loops) : "0"(loops));
}

int get_text_base(void)
{
	return CONFIG_SYS_TEXT_BASE;
}

static void boot_flag_init(void)
{
	unsigned int reg, boot_mode, spi_device_mode;

	/* get boot mode */
	reg = __raw_readl(SYS_CTRL_REG_BASE + REG_SYSSTAT);
	boot_mode = get_sys_boot_mode(reg);

	switch (boot_mode) {
	case BOOT_FROM_SPI:
		spi_device_mode = get_spi_device_type(reg);
		if (spi_device_mode)
			boot_media = BOOT_MEDIA_NAND;
		else
			boot_media = BOOT_MEDIA_SPIFLASH;
		break;
	case BOOT_FROM_EMMC:    /* emmc mode */
		boot_media = BOOT_MEDIA_EMMC;
		break;
	default:
		boot_media = BOOT_MEDIA_UNKNOWN;
		break;
	}
}

int board_early_init_f(void)
{
	return 0;
}

#define REG_SYS_BOOT6   0x0148
#define SD_UPDAT_MAGIC  0x444f574e

#define BIT_UPDATE_FLAG       30
#define BIT_UPDATE_FLAG_CLEAR 0
#define REG_SYS_STAT        0x008C
#define REG_MISC_CTRL51     0x80CC

#define REG_UPDATE_MODE       (SYS_CTRL_REG_BASE + REG_SYS_STAT)
#define REG_UPDATE_MODE_CLEAR (SYS_CTRL_REG_BASE + REG_MISC_CTRL51)

int check_update_button(void)
{
	/* to add some judgement if neccessary */
	unsigned int sd_update_flag;
	unsigned int update_mode = 0;

	/* sd update   */
	sd_update_flag = readl(SYS_CTRL_REG_BASE + REG_SYS_BOOT6);
	if (sd_update_flag == SD_UPDAT_MAGIC)
		return 1;

	/* usb device update  */
	update_mode = readl(REG_UPDATE_MODE);
	update_mode = ((update_mode >> BIT_UPDATE_FLAG) & 0x01);
	if (update_mode)
		return 1;    /* update enable */
	else
		return 0;
}

void clear_update_flag(int update_flag)
{
	unsigned int clear_value = 0x1;

	clear_value = (clear_value << BIT_UPDATE_FLAG_CLEAR);
	/* REG_UPDATE_MODE_CLEAR: write only */
	writel(clear_value, REG_UPDATE_MODE_CLEAR);
}

int misc_init_r(void)
{
#ifdef CONFIG_RANDOM_ETHADDR
	random_init_r();
#endif
	env_set("verify", "n");

#ifdef CFG_MMU_HANDLEOK
	dcache_stop();
	dcache_start();
#endif

	return 0;
}

int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_arch_number = MACH_TYPE_XM720XXX;
	gd->bd->bi_boot_params = CFG_BOOT_PARAMS;

	boot_flag_init();

	return 0;
}

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->ram_size = PHYS_SDRAM_1_SIZE;
	print_size(gd->ram_size, "\n");
	return 0;
}

void reset_cpu(ulong addr)
{
	/* 0x12345678:writing any value will cause a reset. */
	writel(0x12345678, SYS_CTRL_REG_BASE + REG_SC_SYSRES);
	while (1);
}

int timer_init(void)
{
	/*
	 * Under uboot, 0xffffffff is set to load register,
	 * timer_clk equals BUSCLK/2/256.
	 * e.g. BUSCLK equals 50M, it will roll back after 0xffffffff/timer_clk
	 * 43980s equals 12hours
	 */
	__raw_writel(0, TIMER0_REG_BASE + REG_TIMER_CONTROL);
	__raw_writel(~0, TIMER0_REG_BASE + REG_TIMER_RELOAD);

	/* 32 bit, periodic */
	__raw_writel(CFG_TIMER_CTRL, TIMER0_REG_BASE + REG_TIMER_CONTROL);

	return 0;
}

int board_eth_init(bd_t *bis)
{
	int rc = 0;

#ifdef CONFIG_NET_FEMAC
	rc = bspeth_initialize(bis);
#endif
	return rc;
}

#ifdef CONFIG_GENERIC_MMC
int board_mmc_init(bd_t *bis)
{
	int ret = 0;

#ifdef CONFIG_XMEDIA_SDHCI

#ifndef CONFIG_FMC
	ret = sdhci_add_port(0, EMMC_BASE_REG, MMC_TYPE_MMC);
	if (!ret) {
		ret = bsp_mmc_init(0);
		if (ret)
			printf("No EMMC device found !\n");
	}
#else

#ifdef CONFIG_AUTO_SD_UPDATE
	ret = sdhci_add_port(0, SDIO0_BASE_REG, MMC_TYPE_SD);
	if (ret)
		return ret;

	ret = bsp_mmc_init(0);
	if (ret)
		printf("No SD device found !\n");
#endif

#endif
#endif
	ret = sdhci_add_port(0, SDIO0_BASE_REG, MMC_TYPE_SD);
	if (ret)
		return ret;

	ret = bsp_mmc_init(0);
	if (ret)
		printf("No SD device found !\n");

	return ret;
}
#endif
#ifdef CONFIG_ARMV7_NONSEC
void smp_set_core_boot_addr(unsigned long addr, int corenr)
{
}

void smp_kick_all_cpus(void)
{
}

void smp_waitloop(unsigned previous_address)
{
}
#endif

