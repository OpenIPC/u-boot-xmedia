/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __XM72050200_H
#define __XM72050200_H

#include <linux/sizes.h>
#include <asm/arch/platform.h>

#define CONFIG_SYS_CACHELINE_SIZE   64

/* Open it as you need #define CONFIG_REMAKE_ELF */

#define CONFIG_SUPPORT_RAW_INITRD

#define CONFIG_BOARD_EARLY_INIT_F

/* Physical Memory Map */

/* CONFIG_SYS_TEXT_BASE needs to align with where ATF loads bl33.bin */
#define CONFIG_SYS_TEXT_BASE        0x40800000
#define CONFIG_SYS_TEXT_BASE_ORI        0x40700000

#define SEC_UBOOT_DATA_ADDR     0x41000000

#define PHYS_SDRAM_1            0x40000000
#define PHYS_SDRAM_1_SIZE       0x4000000

#define CONFIG_SYS_SDRAM_BASE       PHYS_SDRAM_1


#define CONFIG_SYS_INIT_SP_ADDR     0x04014000

#define CONFIG_SYS_LOAD_ADDR        (CONFIG_SYS_SDRAM_BASE + 0x80000)
#define CONFIG_SYS_GBL_DATA_SIZE    128

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY       24000000

#define CONFIG_SYS_TIMER_RATE       CFG_TIMER_CLK
#define CONFIG_SYS_TIMER_COUNTER    (CFG_TIMERBASE + REG_TIMER_VALUE)
#define CONFIG_SYS_TIMER_COUNTS_DOWN


/* Generic Interrupt Controller Definitions */
#define GICD_BASE           0xf6801000
#define GICC_BASE           0xf6802000

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN       (CONFIG_ENV_SIZE + SZ_128K)

/* PL011 Serial Configuration */
#define CONFIG_PL011_CLOCK      24000000

#define CONFIG_PL01x_PORTS  \
	{(void *)UART0_REG_BASE, (void *)UART1_REG_BASE, \
	(void *)UART2_REG_BASE}

#define CONFIG_CUR_UART_BASE    UART0_REG_BASE

/* Flash Memory Configuration v100 */
#ifdef CONFIG_FMC
#define CONFIG_FMC_REG_BASE       FMC_REG_BASE
#define CONFIG_FMC_BUFFER_BASE    FMC_MEM_BASE
#define CONFIG_FMC_MAX_CS_NUM     1
#endif

#ifdef CONFIG_FMC_SPI_NOR
#define CONFIG_SPI_NOR_MAX_CHIP_NUM 1
#define CONFIG_SPI_NOR_QUIET_TEST
#endif

#ifdef CONFIG_FMC_SPI_NAND
#define CONFIG_SPI_NAND_MAX_CHIP_NUM    1
#define CONFIG_SYS_MAX_NAND_DEVICE  CONFIG_SPI_NAND_MAX_CHIP_NUM
#define CONFIG_SYS_NAND_MAX_CHIPS   CONFIG_SPI_NAND_MAX_CHIP_NUM
#define CONFIG_SYS_NAND_BASE        FMC_MEM_BASE
#endif

#define CONFIG_SYS_FAULT_ECHO_LINK_DOWN

/*
*-----------------------------------------------------------------------
* Fast ethernet Configuration
*-----------------------------------------------------------------------
*/
#ifdef CONFIG_NET_FEMAC
#define INNER_PHY
#define SFV_MII_MODE              0
#define SFV_RMII_MODE             1
#define BSPETH_MII_RMII_MODE_U           SFV_MII_MODE
#define BSPETH_MII_RMII_MODE_D           SFV_MII_MODE
#define SFV_PHY_U             0
#define SFV_PHY_D             2
#endif

/*
*-----------------------------------------------------------------------
* SD/MMC configuration
*-----------------------------------------------------------------------
*/
#define CONFIG_MISC_INIT_R

/* Command line configuration */
#define CONFIG_MENU
/* #define CONFIG_CMD_UNZIP */
#define CONFIG_CMD_ENV

#define CONFIG_MTD_PARTITIONS

/* BOOTP options */
#define CONFIG_BOOTP_BOOTFILESIZE

/* Initial environment variables */

/*
 * Defines where the kernel and FDT will be put in RAM
 */

/* Assume we boot with root on the seventh partition of eMMC */
#define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS 2
#define BOOT_TARGET_DEVICES(func) \
	func(USB, usb, 0) \
	func(MMC, mmc, 1) \
	func(DHCP, dhcp, na)
#include <config_distro_bootcmd.h>

/* allow change env */
#define  CONFIG_ENV_OVERWRITE

#define CONFIG_COMMAND_HISTORY

/* env in flash instead of CFG_ENV_IS_NOWHERE */
#define CONFIG_ENV_OFFSET       0x80000      /* environment starts here */

#define CONFIG_ENV_VARS_UBOOT_CONFIG

/* kernel parameter list phy addr */
#define CFG_BOOT_PARAMS         (CONFIG_SYS_SDRAM_BASE + 0x0100)

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE       1024    /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE       (CONFIG_SYS_CBSIZE + \
		    sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE     CONFIG_SYS_CBSIZE
#define CONFIG_SYS_MAXARGS      64  /* max command args */

#define CONFIG_SYS_NO_FLASH


/* Open it as you need #define DDR_SCRAMB_ENABLE */

#define CONFIG_PRODUCTNAME "xm72050200"

/* the flag for auto update. 1:enable; 0:disable */
#define CONFIG_AUTO_UPDATE          0

#if (CONFIG_AUTO_UPDATE == 1)
#define CONFIG_AUTO_UPDATE_ADAPTATION   1
/* Open it as you need #define CONFIG_AUTO_SD_UPDATE     1 */
/* Open it as you need #define CONFIG_AUTO_USB_UPDATE    1 */

#define CONFIG_FS_FAT 1
#define CONFIG_FS_FAT_MAX_CLUSTSIZE 65536
#endif

/*---------------------------------------------------------------------
 * sdcard system updae
 * ---------------------------------------------------------------------*/
#ifdef CONFIG_AUTO_SD_UPDATE

#ifndef CONFIG_SDHCI
#define CONFIG_MMC_WRITE  1
#define CONFIG_MMC_QUIRKS  1
#define CONFIG_MMC_HW_PARTITIONING  1
#define CONFIG_MMC_HS400_ES_SUPPORT  1
#define CONFIG_MMC_HS400_SUPPORT  1
#define CONFIG_MMC_HS200_SUPPORT  1
#define CONFIG_MMC_VERBOSE  1
#define CONFIG_MMC_SDHCI  1
#define CONFIG_MMC_SDHCI_ADMA  1
#endif

#ifndef CONFIG_MMC
#define CONFIG_MMC      1
#endif

#endif

/* SD/MMC configuration */
#ifdef CONFIG_MMC
#define CONFIG_SUPPORT_EMMC_BOOT
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_MMC
#define CONFIG_SYS_MMC_ENV_DEV  0
#define CONFIG_EXT4_SPARSE
#define CONFIG_SDHCI
#define CONFIG_XMEDIA_SDHCI
#define CONFIG_XMEDIA_SDHCI_MAX_FREQ  150000000
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_FS_EXT4
#define CONFIG_SDHCI_ADMA
#endif

#ifndef CONFIG_FMC
#define CONFIG_EMMC
#endif

#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_CMDLINE_TAG

#define GZIP_HEAD_SIZE   0X10
#define HEAD_MAGIC_NUM0 0X70697A67 /* 'g''z''i''p' */
#define HEAD_MAGIC_NUM0_OFFSET 0X8
#define HEAD_MAGIC_NUM1 0X64616568 /* 'h''e''a''d' */
#define HEAD_MAGIC_NUM1_OFFSET 0XC
#define COMPRESSED_SIZE_OFFSET      0X0
#define UNCOMPRESSED_SIZE_OFFSET    0X4

/* Open it as you need #define CONFIG_OSD_ENABLE */ /* For VO */
/* Open it as you need #define CONFIG_CIPHER_ENABLE */


/* Open it as you need #define CONFIG_AUDIO_ENABLE */

/* Open it as you need #definee CONFIG_EDMA_PLL_TRAINNING */

#define SECUREBOOT_OTP_REG_BASE_ADDR_PHY            (0x10090000)

#include "xm-common.h"

#endif /* __XM72050200_H */
