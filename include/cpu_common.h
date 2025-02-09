/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __CPU_COMMON_H__
#define __CPU_COMMON_H__

#include <command.h>
#include <hw_decompress.h>

#ifdef CONFIG_EMMC
extern int target_dev;
extern int target_paratition;
#endif

void reset_cpu(ulong addr);
void uart_early_init(void);
void uart_early_puts(const char *ss);
#ifdef CONFIG_BSP_NAND_SPL
extern uint32_t crc32(uint32_t crc, const char *p, unsigned int len);

extern unsigned long __bss_start;
extern unsigned char _blank_zone_end[];
extern unsigned long _blank_crc_start;
#endif
extern unsigned char input_data[];
extern unsigned char input_data_end[];
extern unsigned long _armboot_start;
extern int sdhci_add_port(int index, u32 regbase, u32 freq);
extern int bsp_mmc_init(int index);

#if (CONFIG_AUTO_UPDATE == 1)
extern int do_auto_update(void);
extern int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]);
#endif /* CONFIG_AUTO_UPDATE */

#ifdef CONFIG_GENERIC_MMC
extern int mci_probe(int dev_num);
#endif /* CONFIG_GENERIC_MMC */

#ifdef CONFIG_SUPPORT_EMMC_BOOT
extern int mci_add_port(int index, u32 reg_base, u32 freq);
#endif /* CONFIG_SUPPORT_EMMC_BOOT */

#ifdef CONFIG_AUTO_SD_UPDATE
extern int mci_add_port(int index, u32 reg_base, u32 freq);
#endif /* CONFIG_AUTO_SD_UPDATE */

extern int mmc_phy_init(void);

#ifdef CONFIG_CMD_NAND
extern int nand_saveenv(void);
extern void nand_env_relocate_spec(void);
#endif /* CONFIG_CMD_NAND */

#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
extern int sf_saveenv(void);
extern void sf_env_relocate_spec(void);
#endif /* CONFIG_ENV_IS_IN_SPI_FLASH */

#ifdef CONFIG_ENV_IS_IN_MMC
extern int emmc_saveenv(void);
extern void emmc_env_relocate_spec(void);
#endif /* CONFIG_ENV_IS_IN_MMC */

#ifdef CONFIG_ENV_IS_IN_UFS
extern int ufs_saveenv(void);
extern void ufs_env_relocate_spec(void);
#endif /* CONFIG_ENV_IS_IN_UFS */

#ifdef CONFIG_HWDEC
extern unsigned int hw_dec_type;
extern void hw_dec_init(void);
extern void hw_dec_uinit(void);

#endif /* CONFIG_HWDEC */

#ifdef CONFIG_ARM64_SUPPORT_LOAD_FIP
extern const void *fdt_getprop(const void *fdt, int nodeoffset,
				const char *name, int *lenp);
extern int fdt_next_node(const void *fdt, int offset, int *depth);
extern const char *fdt_get_name(const void *fdt, int nodeoffset, int *len);
extern int fdt_check_header(const void *fdt);
extern int do_load_secure_os(ulong addr, ulong org_offset, ulong img_dst,
				int run, uint32_t *rtos_load_addr);
#endif /* CONFIG_ARM64_SUPPORT_LOAD_FIP */
#endif /* __CPU_COMMON_H__ */

