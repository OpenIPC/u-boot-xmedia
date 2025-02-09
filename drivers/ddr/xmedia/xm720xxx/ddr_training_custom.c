/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <command.h>
#include "ddr_interface.h"
#include "ddr_training_impl.h"

#define CRG_BASE            0x12010000U
#define PERI_CRG_DDRT           0x198U
#define PERI_CRG_DDRCKSEL       0x80U
/* [SYSCTRL]RAM Retention control register 0 */
#define SYSCTRL_MISC_CTRL4      0x8010U


/*
 * Do some prepare before copy code from DDR to SRAM.
 * Keep empty when nothing to do.
 */
void ddr_cmd_prepare_copy(void)
{
	return;
}

/*
 * Save site before DDR training command execute .
 * Keep empty when nothing to do.
 */
void ddr_cmd_site_save(void)
{
	return;
}

/*
 * Restore site after DDR training command execute.
 * Keep empty when nothing to do.
 */
void ddr_cmd_site_restore(void)
{
	return;
}

/*
 * Save site before DDR training:include boot and command execute.
 * Keep empty when nothing to do.
 */
void ddr_boot_cmd_save(void *reg)
{
	struct tr_relate_reg *relate_reg = (struct tr_relate_reg *)reg;

	if (relate_reg == NULL)
		return;

	/* select ddrt bus path */
	relate_reg->custom.ive_ddrt_mst_sel = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_MISC_CTRL4);
	reg_write(relate_reg->custom.ive_ddrt_mst_sel & 0xffffffdf, DDR_REG_BASE_SYSCTRL + SYSCTRL_MISC_CTRL4);

	/* turn on ddrt clock */
	relate_reg->custom.ddrt_clk_reg = reg_read(CRG_BASE + PERI_CRG_DDRT);
	/* enable ddrt0 clock */
	reg_write(relate_reg->custom.ddrt_clk_reg | (1U << 1), CRG_BASE + PERI_CRG_DDRT);
	__asm__ __volatile__("nop");
	/* disable ddrt0 soft reset */
	reg_write(reg_read(CRG_BASE + PERI_CRG_DDRT) & (~(1U << 0)), CRG_BASE + PERI_CRG_DDRT);

	/* disable rdqs anti-aging */
	relate_reg->custom.phy0_age_compst_en = reg_read(DDR_REG_BASE_PHY0 + DDR_PHY_PHYRSCTRL);
	reg_write((relate_reg->custom.phy0_age_compst_en & (~(0x1 << PHY_CFG_RX_AGE_COMPST_EN_BIT))),
		DDR_REG_BASE_PHY0 + DDR_PHY_PHYRSCTRL);
#ifdef DDR_REG_BASE_PHY1
	relate_reg->custom.phy1_age_compst_en = reg_read(DDR_REG_BASE_PHY1 + DDR_PHY_PHYRSCTRL);
	reg_write((relate_reg->custom.phy1_age_compst_en & (~(0x1 << PHY_CFG_RX_AGE_COMPST_EN_BIT))),
		DDR_REG_BASE_PHY1 + DDR_PHY_PHYRSCTRL);
#endif
}

/*
 * Restore site after DDR training:include boot and command execute.
 * Keep empty when nothing to do.
 */
void ddr_boot_cmd_restore(const void *reg)
{
	struct tr_relate_reg *relate_reg = (struct tr_relate_reg *)reg;

	if (relate_reg == NULL)
		return;

	/* restore ddrt bus path */
	reg_write(relate_reg->custom.ive_ddrt_mst_sel, DDR_REG_BASE_SYSCTRL + SYSCTRL_MISC_CTRL4);

	/* restore ddrt clock */
	reg_write(relate_reg->custom.ddrt_clk_reg, CRG_BASE + PERI_CRG_DDRT);

	/* restore rdqs anti-aging */
	reg_write(relate_reg->custom.phy0_age_compst_en, DDR_REG_BASE_PHY0 + DDR_PHY_PHYRSCTRL);
#ifdef DDR_REG_BASE_PHY1
	reg_write(relate_reg->custom.phy1_age_compst_en, DDR_REG_BASE_PHY1 + DDR_PHY_PHYRSCTRL);
#endif
}

/*
 * DDR clock select.
 * For ddr osc training.
 */
#ifdef DDR_PCODE_TRAINING_CONFIG
int ddr_get_cksel(void)
{
	int freq;
	unsigned int ddr_cksel;
	ddr_cksel = (reg_read(CRG_BASE + PERI_CRG_DDRCKSEL) >> 0x3) & 0x7;
	switch (ddr_cksel) {
	case 0x0:
		freq = 24; /* 24MHz */
		break;
	case 0x1:
		freq = 450; /* 450MHz */
		break;
	case 0x3:
		freq = 300; /* 300MHz */
		break;
	case 0x4:
		freq = 297; /* 297MHz */
		break;
	default:
		freq = 300; /* 300MHz */
		break;
	}
	return freq;
}
#endif

/* ddr DMC auto power down config
 * the value should config after trainning, or it will cause chip compatibility problems
 */
void ddr_dmc_auto_power_down_cfg(void)
{
	if ((reg_read(DDR_REG_BASE_PHY0 + DDR_PHY_DRAMCFG) &
		PHY_DRAMCFG_TYPE_MASK) == PHY_DRAMCFG_TYPE_LPDDR4) {
		reg_write(DMC_CFG_PD_VAL, DDR_REG_BASE_DMC0 + DDR_DMC_CFG_PD);
		reg_write(DMC_CFG_PD_VAL, DDR_REG_BASE_DMC1 + DDR_DMC_CFG_PD);
	} else {
		reg_write(DMC_CFG_PD_VAL, DDR_REG_BASE_DMC0 + DDR_DMC_CFG_PD);
	}
#ifdef DDR_REG_BASE_PHY1
	if ((reg_read(DDR_REG_BASE_PHY1 + DDR_PHY_DRAMCFG) &
		PHY_DRAMCFG_TYPE_MASK) == PHY_DRAMCFG_TYPE_LPDDR4) {
		reg_write(DMC_CFG_PD_VAL, DDR_REG_BASE_DMC2 + DDR_DMC_CFG_PD);
		reg_write(DMC_CFG_PD_VAL, DDR_REG_BASE_DMC3 + DDR_DMC_CFG_PD);
	} else {
		reg_write(DMC_CFG_PD_VAL, DDR_REG_BASE_DMC1 + DDR_DMC_CFG_PD);
	}
#endif
}
