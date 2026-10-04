/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __XMEDIA_NAND_FIT_ENV_H
#define __XMEDIA_NAND_FIT_ENV_H

#if defined(CONFIG_FIT) && defined(CONFIG_FMC_SPI_NAND)
void nand_fit_env_migrate(void);
#endif

#endif
