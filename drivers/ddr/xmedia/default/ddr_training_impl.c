/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include "ddr_training_impl.h"
#include "ddr_interface.h"

#define __COMMON__
void* ddrtr_copy_data(void *dst, const void *src, unsigned int len)
{
	const char *s = src;
	char *d = dst;

	while (len--)
		*d++ = *s++;

	return dst;
}

void* ddrtr_set_data(void *b, int c, unsigned int len)
{
	char *bp = b;

	while (len--)
		*bp++ = (unsigned char)c;

	return b;
}

static int ddr_training_by_dmc(struct ddr_cfg_st *cfg)
{
	if (cfg->cmd_st != NULL) {
#ifdef DDR_TRAINING_CMD
		return ddr_training_cmd_func(cfg);
#endif
	} else {
		return ddr_training_boot_func(cfg);
	}

	return 0;
}

static int ddr_training_by_rank(struct ddr_cfg_st *cfg)
{
	int result = 0;
	unsigned int i;

	ddr_phy_switch_rank(cfg->cur_phy, cfg->rank_idx);
	if (cfg->phy[cfg->phy_idx].dmc_num > DDR_DMC_PER_PHY_MAX)
		return -1;

	for (i = 0; i < cfg->phy[cfg->phy_idx].dmc_num; i++) {
		cfg->dmc_idx = i;
		cfg->cur_dmc = cfg->phy[cfg->phy_idx].dmc[i].addr;
		cfg->cur_pattern = cfg->phy[cfg->phy_idx].dmc[i].ddrt_pattern;
		result += ddr_training_by_dmc(cfg);
	}

	return result;
}

static int ddr_training_by_phy(struct ddr_cfg_st *cfg)
{
	int result = 0;
	unsigned int i;
	unsigned int phy_mask;
	unsigned int rank_num;

	phy_mask = 1 << (cfg->phy_idx);
	rank_num = cfg->phy[cfg->phy_idx].rank_num;

	for (i = 0; i < rank_num; i++) {
		cfg->rank_idx = i;
		cfg->cur_item = cfg->phy[cfg->phy_idx].rank[i].item;
		if (ddr_training_check_bypass(cfg, phy_mask) != DDR_FALSE)
			continue;
		result += ddr_training_by_rank(cfg);
	}

	return result;
}

int ddr_training_all(struct ddr_cfg_st *cfg)
{
	int result = 0;
	unsigned int i;

	if (cfg == NULL)
		return -1;

	for (i = 0; i < cfg->phy_num; i++) {
		cfg->phy_idx = i;
		cfg->cur_phy = cfg->phy[i].addr;
		result += ddr_training_by_phy(cfg);
	}

	return result;
}

/* DDR training phy/dmc/dram_type config init */
static void ddr_training_cfg_set_dmc(struct ddr_cfg_st *cfg)
{
	unsigned int ddrt_pattern;

	if (cfg->phy[0].dram_type == PHY_DRAMCFG_TYPE_LPDDR4) {
		cfg->phy[0].dmc_num = 2; /* lpddr4: 2 dmc per phy */
		ddrt_pattern = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDRT_PATTERN);
		cfg->phy[0].dmc[0].addr = DDR_REG_BASE_DMC0;
		cfg->phy[0].dmc[0].ddrt_pattern = ddrt_pattern & 0xffff; /* bit[15-0]:dmc0 ddrt pattern */
		cfg->phy[0].dmc[0].byte_num = ddr_phy_get_byte_num(DDR_REG_BASE_DMC0);
		cfg->phy[0].dmc[1].addr = DDR_REG_BASE_DMC1;
		cfg->phy[0].dmc[1].ddrt_pattern = ddrt_pattern >> 16; /* bit[31-16]:dmc1 ddrt pattern */
		cfg->phy[0].dmc[1].byte_num = ddr_phy_get_byte_num(DDR_REG_BASE_DMC1);
		cfg->phy[0].total_byte_num = cfg->phy[0].dmc[0].byte_num + cfg->phy[0].dmc[1].byte_num;
	} else {
		cfg->phy[0].dmc_num = 1; /* other: 1 dmc per phy */
		cfg->phy[0].dmc[0].addr = DDR_REG_BASE_DMC0;
		cfg->phy[0].dmc[0].ddrt_pattern = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDRT_PATTERN);
		cfg->phy[0].dmc[0].byte_num = ddr_phy_get_byte_num(DDR_REG_BASE_DMC0);
		cfg->phy[0].total_byte_num = cfg->phy[0].dmc[0].byte_num;
	}
	ddr_info("phy[0] total_byte_num[%x] dram_type[%x]", cfg->phy[0].total_byte_num, cfg->phy[0].dram_type);

#ifdef DDR_REG_BASE_PHY1
	if (cfg->phy[1].dram_type == PHY_DRAMCFG_TYPE_LPDDR4) {
		cfg->phy[1].dmc_num = 2; /* lpddr4: 2 dmc per phy */
		ddrt_pattern = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDRT_PATTERN_SEC);
		cfg->phy[1].dmc[0].addr = DDR_REG_BASE_DMC2;
		cfg->phy[1].dmc[0].ddrt_pattern = ddrt_pattern & 0xffff; /* bit[15-0]:dmc0 ddrt pattern */
		cfg->phy[1].dmc[0].byte_num = ddr_phy_get_byte_num(DDR_REG_BASE_DMC2);
		cfg->phy[1].dmc[1].addr = DDR_REG_BASE_DMC3;
		cfg->phy[1].dmc[1].ddrt_pattern = ddrt_pattern >> 16; /* bit[31-16]:dmc1 ddrt pattern */
		cfg->phy[1].dmc[1].byte_num = ddr_phy_get_byte_num(DDR_REG_BASE_DMC3);
		cfg->phy[1].total_byte_num = cfg->phy[1].dmc[0].byte_num + cfg->phy[1].dmc[1].byte_num;
	} else {
		cfg->phy[1].dmc_num = 1; /* other: 1 dmc per phy */
		cfg->phy[1].dmc[0].addr = DDR_REG_BASE_DMC1;
		cfg->phy[1].dmc[0].ddrt_pattern = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDRT_PATTERN_SEC);
		cfg->phy[1].dmc[0].byte_num = ddr_phy_get_byte_num(DDR_REG_BASE_DMC1);
		cfg->phy[1].total_byte_num = cfg->phy[1].dmc[0].byte_num;
	}
	ddr_info("phy[1] total_byte_num[%x] dram_type[%x]", cfg->phy[1].total_byte_num, cfg->phy[1].dram_type);
#endif
}

static void ddr_training_cfg_set_rank(struct ddr_cfg_st *cfg)
{
	cfg->phy[0].rank_num = 1;
	cfg->phy[0].rank[0].item = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG);
	cfg->phy[0].rank[0].item_hw = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY0_RANK0);
#ifdef SYSCTRL_DDR_TRAINING_CFG_SEC
	cfg->phy[0].rank[1].item = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG_SEC);
#endif
	cfg->phy[0].rank[1].item_hw = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY0_RANK1);

	if (reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY0_RANK1))
		cfg->phy[0].rank_num = 2; /* rank number equal 2 if SYSCTRL_DDR_HW_PHY0_RANK1 has bean define in boot table */

	ddr_info("Rank number PHY0 [%x]", cfg->phy[0].rank_num);
	ddr_info("HW training item PHY0[%x = %x][%x = %x]",
		(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY0_RANK0), cfg->phy[0].rank[0].item_hw,
		(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY0_RANK1), cfg->phy[0].rank[1].item_hw);

#ifdef DDR_REG_BASE_PHY1
	cfg->phy[1].rank_num = 1;
	cfg->phy[1].rank[0].item = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG);
	cfg->phy[1].rank[0].item_hw = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY1_RANK0);

	cfg->phy[1].rank[1].item = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG_SEC);
	cfg->phy[1].rank[1].item_hw = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY1_RANK1);

	if (reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY1_RANK1))
		cfg->phy[1].rank_num = 2; /* rank number equal 2 if SYSCTRL_DDR_HW_PHY1_RANK1 has bean define in boot table */

	ddr_info("Rank number PHY1[%x]", cfg->phy[1].rank_num);
	ddr_info("HW training item PHY1[%x = %x][%x = %x]",
		(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY1_RANK0), cfg->phy[1].rank[0].item_hw,
		(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_HW_PHY1_RANK1), cfg->phy[1].rank[1].item_hw);
#endif

	ddr_info("SW training item Rank0[%x = %x]",
		(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG), cfg->phy[0].rank[0].item);
#ifdef SYSCTRL_DDR_TRAINING_CFG_SEC
	ddr_info("SW training item Rank1[%x = %x]",
		(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG_SEC), cfg->phy[0].rank[1].item);
#endif
}

static void ddr_training_cfg_set_phy(struct ddr_cfg_st *cfg)
{
	cfg->phy_num = DDR_PHY_NUM;
	cfg->phy[0].addr = DDR_REG_BASE_PHY0;
	cfg->phy[0].dram_type = reg_read(DDR_REG_BASE_PHY0 + DDR_PHY_DRAMCFG) &
		PHY_DRAMCFG_TYPE_MASK;
#ifdef DDR_REG_BASE_PHY1
	cfg->phy[1].addr = DDR_REG_BASE_PHY1;
	cfg->phy[1].dram_type = reg_read(DDR_REG_BASE_PHY1 + DDR_PHY_DRAMCFG) &
		PHY_DRAMCFG_TYPE_MASK;
#endif
}

void ddr_training_cfg_init(struct ddr_cfg_st *cfg)
{
	ddrtr_set_data(cfg, 0, sizeof(struct ddr_cfg_st));
	ddr_training_cfg_set_phy(cfg);
	ddr_training_cfg_set_dmc(cfg);
	ddr_training_cfg_set_rank(cfg);
}

/* 2GHz CPU run 2000 "nop" in 1 ns */
static inline void ddr_training_delay(unsigned int cnt)
{
	while (cnt--)
		asm("nop");
}

/* set auto refresh */
void ddr_training_set_timing(unsigned int base_dmc, unsigned int timing)
{
	ddr_training_delay(DDR_AUTO_TIMING_DELAY);
	reg_write(timing, base_dmc + DDR_DMC_TIMING2);
	/* need to delay 1 ns */
	ddr_training_delay(DDR_AUTO_TIMING_DELAY);
}

#ifdef DDR_TRAINING_STAT_CONFIG
/* Save training result in stat register */
static void ddr_training_save(unsigned int mask, unsigned int phy, int byte, int dq)
{
	unsigned int stat;
	unsigned int phy_index;

	stat = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_STAT);
	/* only record the first error */
	if (stat)
		return;

	stat = mask;
	if (phy != 0) {
		phy_index = ((phy == DDR_REG_BASE_PHY0) ? DDR_ERR_PHY0 : DDR_ERR_PHY1);
		stat |= phy_index;
	}

	if (byte != -1)
		stat |= ((unsigned int)byte << DDR_ERR_BYTE_BIT);

	if (dq != -1)
		stat |= ((unsigned int)dq << DDR_ERR_DQ_BIT);

	reg_write(stat, DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_STAT);
}
#endif

/* Record error code in register */
void ddr_training_stat(unsigned int mask, unsigned int phy, int byte, int dq)
{
	ddr_training_error(mask, phy, byte, dq);
#ifdef DDR_TRAINING_STAT_CONFIG
	ddr_training_save(mask, phy, byte, dq);
#endif
}

/* Check DDR training item whether by pass */
int ddr_training_check_bypass(const struct ddr_cfg_st *cfg, unsigned int mask)
{
	/* training item disable */
	if ((cfg->cur_item) & mask) {
		ddr_debug("DDR training [%x] is disable, rank[%x] cfg[%x]",
			mask, cfg->rank_idx, cfg->cur_item);
		return DDR_TRUE;
	} else {
		return DDR_FALSE;
	}
}

#if !defined(DDR_TRAINING_CUT_CODE_CONFIG) || defined(DDR_TRAINING_CMD)
/* Save register value before training */
void ddr_training_save_reg(const struct ddr_cfg_st *cfg,
	struct tr_relate_reg *relate_reg, unsigned int mask)
{
	unsigned int base_dmc;
	unsigned int base_phy;

	base_dmc = cfg->cur_dmc;
	base_phy = cfg->cur_phy;

	/* save reg value */
	relate_reg->auto_ref_timing = reg_read(base_dmc + DDR_DMC_TIMING2);
	relate_reg->power_down = reg_read(base_dmc + DDR_DMC_CFG_PD);
	relate_reg->misc_scramb = reg_read(base_phy + DDR_PHY_MISC);
	/* Static register have to read two times to get the right value. */
	relate_reg->ac_phy_ctl = reg_read(base_phy + DDR_PHY_ACPHYCTL4);
	relate_reg->ac_phy_ctl = reg_read(base_phy + DDR_PHY_ACPHYCTL4);

	/* set new value */
	switch (mask) {
	case DDR_BYPASS_WL_MASK:
	case DDR_BYPASS_LPCA_MASK:
		/* disable auto refresh */
		ddr_training_set_timing(base_dmc, relate_reg->auto_ref_timing & DMC_AUTO_TIMING_DIS);
		break;
	case DDR_BYPASS_GATE_MASK:
		/* disable auto refresh */
		ddr_training_set_timing(base_dmc, relate_reg->auto_ref_timing & DMC_AUTO_TIMING_DIS);
		if (!(reg_read(base_phy + DDR_PHY_DRAMCFG) & PHY_DRAMCFG_MA2T)) /* set 1T */
			reg_write(0x0, base_phy + DDR_PHY_ACPHYCTL4);
		break;
	case DDR_BYPASS_HW_MASK:
		if (!(reg_read(base_phy + DDR_PHY_DRAMCFG) & PHY_DRAMCFG_MA2T)) /* set 1T */
			reg_write(0x0, base_phy + DDR_PHY_ACPHYCTL4);
		break;
	default:
		break;
	}

	reg_write(relate_reg->power_down & DMC_POWER_DOWN_DIS,
		base_dmc + DDR_DMC_CFG_PD);
	reg_write(relate_reg->misc_scramb & PHY_MISC_SCRAMB_DIS,
		base_phy + DDR_PHY_MISC);

	ddr_dqsswap_save_func(relate_reg->swapdfibyte_en, base_phy);

	ddr_axi_save_func(relate_reg);

	ddr_rnkvol_save_func(relate_reg, base_dmc);

	/* save customer reg */
	ddr_training_save_reg_func((void *)relate_reg, mask);

	ddr_phy_cfg_update(base_phy);

	ddr_asm_dsb();
}

/* Restore register value after training */
void ddr_training_restore_reg(const struct ddr_cfg_st *cfg,
	const struct tr_relate_reg *relate_reg)
{
	unsigned int base_dmc;
	unsigned int base_phy;

	base_dmc = cfg->cur_dmc;
	base_phy = cfg->cur_phy;

	/* enable auto refresh */
	ddr_training_set_timing(base_dmc, relate_reg->auto_ref_timing);
	reg_write(relate_reg->power_down, base_dmc + DDR_DMC_CFG_PD);
	reg_write(relate_reg->misc_scramb, base_phy + DDR_PHY_MISC);
	if (!(reg_read(base_phy + DDR_PHY_DRAMCFG) & PHY_DRAMCFG_MA2T))
		reg_write(relate_reg->ac_phy_ctl, base_phy + DDR_PHY_ACPHYCTL4);

	ddr_dqsswap_restore_func(relate_reg->swapdfibyte_en, base_phy);

	ddr_axi_restore_func(relate_reg);

	ddr_rnkvol_restore_func(relate_reg, base_dmc);

	/* restore customer reg */
	ddr_training_restore_reg_func((void *)relate_reg);

	ddr_phy_cfg_update(base_phy);

	ddr_asm_dsb();
}

/* Switch AXI to DMC0/DMC1/DMC2/DMC3 for DDRT test */
void ddr_training_switch_axi(const struct ddr_cfg_st *cfg)
{
	ddr_axi_switch_func(cfg);
	ddr_debug("AXI region0[%x = %x]",
		(DDR_REG_BASE_AXI + DDR_AXI_REGION_ATTRIB0),
		reg_read(DDR_REG_BASE_AXI + DDR_AXI_REGION_ATTRIB0));
	ddr_debug("AXI region1[%x = %x]",
		(DDR_REG_BASE_AXI + DDR_AXI_REGION_ATTRIB1),
		reg_read(DDR_REG_BASE_AXI + DDR_AXI_REGION_ATTRIB1));

	ddr_rnkvol_set_func(cfg);
}
#endif

#if defined(DDR_WL_TRAINING_CONFIG) || defined(DDR_MPR_TRAINING_CONFIG)
/* Excute DMC sfc command. */
static void ddr_dmc_sfc_cmd(unsigned int base_dmc, unsigned int sfc_cmd,
	unsigned int sfc_addr, unsigned int sfc_bank)
{
	unsigned int count = 0;

	/* set sfc cmd */
	dmc_sfc_cmd_write(sfc_cmd, base_dmc + DDR_DMC_SFCCMD);
	/* set col and row */
	reg_write(sfc_addr, base_dmc + DDR_DMC_SFCADDR);
	/* set bank */
	dmc_sfc_bank_write(sfc_bank, base_dmc + DDR_DMC_SFCBANK);
	/* excute cmd */
	reg_write(0x1, base_dmc + DDR_DMC_SFCREQ);

	ddr_asm_dsb();

	while (count < DDR_SFC_WAIT_TIMEOUT) { /* wait command finished */
		if (!(reg_read(base_dmc + DDR_DMC_SFCREQ) & 0x1))
			break;
		count++;
	}

	if (count >= DDR_HWR_WAIT_TIMEOUT)
		ddr_error("SFC cmd wait timeout");
}
#endif

#if defined(DDR_HW_TRAINING_CONFIG) || defined(DDR_DCC_TRAINING_CONFIG)
/* Exit or enter auto self-refresh */
static int ddr_training_easr(unsigned int base_dmc, unsigned int sref_req)
{
	unsigned int count;

	count = DDR_HWR_WAIT_TIMEOUT;
	if (sref_req == DDR_EXIT_SREF) {
		/* Exit Auto-self refresh */
		reg_write(DMC_CTRL_SREF_EXIT, base_dmc + DDR_DMC_CTRL_SREF);
		while (count--) {
			if (!(reg_read(base_dmc + DDR_DMC_CURR_FUNC) &
				DMC_CURR_FUNC_IN_SREF_MASK))
				break;
		}
	} else if (sref_req == DDR_ENTER_SREF) {
		/* Enter Auto-self refresh */
		reg_write(DMC_CTRL_SREF_ENTER, base_dmc + DDR_DMC_CTRL_SREF);
		while (count--) {
			if (reg_read(base_dmc + DDR_DMC_CURR_FUNC) &
				DMC_CURR_FUNC_IN_SREF_MASK)
				break;
		}
	}

	if (count == 0xffffffff) {
		ddr_fatal("SREF wait timeout");
		ddr_training_stat(DDR_ERR_HW_RD_DATAEYE, -1, -1, -1);
		return -1;
	}
	return 0;
}

/* DDR hw/dcc training exit or enter auto self-refresh */
int ddr_training_ctrl_easr(const struct ddr_cfg_st *cfg, unsigned int sref_req)
{
	int result = 0;
	unsigned int i;

	const struct ddr_phy_st *phy_st = &cfg->phy[cfg->phy_idx];

	for (i = 0; i < phy_st->dmc_num; i++)
		result += ddr_training_easr(phy_st->dmc[i].addr, sref_req);

	return result;
}

void ddr_training_save_timing(const struct ddr_cfg_st *cfg, struct ddr_timing_st *timing_st)
{
	unsigned int i;
	const struct ddr_phy_st *phy_st = &cfg->phy[cfg->phy_idx];

	for (i = 0; i < phy_st->dmc_num; i++) {
		timing_st->val[i] = reg_read(phy_st->dmc[i].addr + DDR_DMC_TIMING2);
		/* disable auto refresh */
		ddr_training_set_timing(phy_st->dmc[i].addr, timing_st->val[i] & DMC_AUTO_TIMING_DIS);
	}
}

void ddr_training_restore_timing(const struct ddr_cfg_st *cfg,
	const struct ddr_timing_st *timing_st)
{
	unsigned int i;
	const struct ddr_phy_st *phy_st = &cfg->phy[cfg->phy_idx];

	for (i = 0; i < phy_st->dmc_num; i++)
		ddr_training_set_timing(phy_st->dmc[i].addr, timing_st->val[i]);
}
#endif /* DDR_HW_TRAINING_CONFIG || DDR_DCC_TRAINING_CONFIG */

/*
 * config ddrc exit self-refresh or exit powerdown
 * bit[3] 0x1:exit self-refresh
 * bit[3] 0x0:exit powerdown
 */
void ddr_sref_cfg(const struct ddr_cfg_st *cfg, struct dmc_cfg_sref_st *cfg_sref, unsigned int value)
{
	unsigned int i;
	const struct ddr_phy_st *phy_st = NULL;

	phy_st = &cfg->phy[cfg->phy_idx];
	for (i = 0; i < phy_st->dmc_num; i++) {
		cfg_sref->val[i] = reg_read(phy_st->dmc[i].addr + DDR_DMC_CFG_SREF);
		reg_write((cfg_sref->val[i] & (~DMC_CFG_INIT_XSREF_PD_MASK)) | value,
			phy_st->dmc[i].addr + DDR_DMC_CFG_SREF);
	}
}

/* Restore DMC_CFG_SREF config */
void ddr_sref_cfg_restore(const struct ddr_cfg_st *cfg, const struct dmc_cfg_sref_st *cfg_sref)
{
	unsigned int i;
	const struct ddr_phy_st *phy_st = &cfg->phy[cfg->phy_idx];

	for (i = 0; i < phy_st->dmc_num; i++)
		reg_write(cfg_sref->val[i], phy_st->dmc[i].addr + DDR_DMC_CFG_SREF);
}

/*
 * PHY reset
 * To issue reset signal to PHY, this field should be set to a '1'.
 */
void ddr_phy_reset(unsigned int base_phy)
{
	unsigned int tmp;

	tmp = reg_read(base_phy + DDR_PHY_PHYINITCTRL);
	/* set 1 to issue PHY reset signal */
	tmp |= (1 << PHY_RST_BIT);
	reg_write(tmp, base_phy + DDR_PHY_PHYINITCTRL);
	/* set 0 to end the PHY reset signal */
	tmp &= ~(1 << PHY_RST_BIT);
	reg_write(tmp, base_phy + DDR_PHY_PHYINITCTRL);
}

/*
 * Update delay setting in registers to PHY immediately.
 * Make delay setting take effect.
 */
void ddr_phy_cfg_update(unsigned int base_phy)
{
	unsigned int tmp;

	tmp = reg_read(base_phy + DDR_PHY_MISC);
	tmp |= (1 << PHY_MISC_UPDATE_BIT);
	/* update new config to PHY */
	reg_write(tmp, base_phy + DDR_PHY_MISC);
	tmp &= ~(1 << PHY_MISC_UPDATE_BIT);
	reg_write(tmp, base_phy + DDR_PHY_MISC);
	tmp = reg_read(base_phy + DDR_PHY_PHYINITCTRL);
	/* set 1 to issue PHY counter reset signal */
	tmp |= (1 << PHY_PHYCONN_RST_BIT);
	reg_write(tmp, base_phy + DDR_PHY_PHYINITCTRL);
	/* set 0 to end the reset signal */
	tmp &= ~(1 << PHY_PHYCONN_RST_BIT);
	reg_write(tmp, base_phy + DDR_PHY_PHYINITCTRL);

	ddr_asm_dsb();
}

/* Set delay value of the bit delay line of the DATA block */
void ddr_phy_set_dq_bdl(const struct ddr_cfg_st *cfg, unsigned int value)
{
	unsigned int val;
	unsigned int offset;
	unsigned int dq;
	unsigned int base_phy;
	unsigned int byte_index;
	unsigned int rank;

	base_phy = cfg->cur_phy;
	byte_index = cfg->cur_byte;
	rank = cfg->rank_idx;
	dq = cfg->cur_dq & 0x7;
	if (cfg->cur_mode == DDR_MODE_WRITE) {
		if (dq < DDR_DQ_NUM_EACH_REG) /* [DXNWDQNBDL0] 4 bdl: wdq0bdl-wdq3bdl */
			offset = ddr_phy_dxnwdqnbdl0(rank, byte_index);
		else /* [DXNWDQNBDL1] 4 bdl: wdq4bdl-wdq7bdl */
			offset = ddr_phy_dxnwdqnbdl1(rank, byte_index);
	} else {
		if (dq < DDR_DQ_NUM_EACH_REG) /* [DXNRDQNBDL0] 4 bdl: rdq0bdl-rdq3bdl */
			offset = ddr_phy_dxnrdqnbdl0(rank, byte_index);
		else /* [DXNRDQNBDL1] 4 bdl: rdq4bdl-rdq7bdl */
			offset = ddr_phy_dxnrdqnbdl1(rank, byte_index);
	}
	dq &= 0x3; /* one register contains 4 dq */
	val = reg_read(base_phy + offset);
	val &= ~(0xFF << (dq << DDR_DQBDL_SHIFT_BIT));
	val |= ((PHY_BDL_MASK & value) << ((dq << DDR_DQBDL_SHIFT_BIT) + PHY_BDL_DQ_BIT));
	reg_write(val, base_phy + offset);

	ddr_phy_cfg_update(base_phy);
}

/* Get PHY DQ value */
unsigned int ddr_phy_get_dq_bdl(const struct ddr_cfg_st *cfg)
{
	unsigned int val;
	unsigned int offset;
	unsigned int dq;
	unsigned int byte_index;
	unsigned int rank;

	byte_index = cfg->cur_byte;
	rank = cfg->rank_idx;
	dq = cfg->cur_dq & 0x7;
	if (cfg->cur_mode == DDR_MODE_WRITE) {
		if (dq < DDR_DQ_NUM_EACH_REG) /* [DXNWDQNBDL0] 4 bdl: wdq0bdl-wdq3bdl */
			offset = ddr_phy_dxnwdqnbdl0(rank, byte_index);
		else /* [DXNWDQNBDL1] 4 bdl: wdq4bdl-wdq7bdl */
			offset = ddr_phy_dxnwdqnbdl1(rank, byte_index);
	} else {
		if (dq < DDR_DQ_NUM_EACH_REG) /* [DXNRDQNBDL0] 4 bdl: rdq0bdl-rdq3bdl */
			offset = ddr_phy_dxnrdqnbdl0(rank, byte_index);
		else /* [DXNRDQNBDL1] 4 bdl: rdq4bdl-rdq7bdl */
			offset = ddr_phy_dxnrdqnbdl1(rank, byte_index);
	}

	dq &= 0x3;  /* one register contains 4 dq */
	val = (reg_read(cfg->cur_phy + offset) >>
		((dq << DDR_DQBDL_SHIFT_BIT) + PHY_BDL_DQ_BIT)) & PHY_BDL_MASK;

	return val;
}

/* Get byte number */
unsigned int ddr_phy_get_byte_num(unsigned int base_dmc)
{
	unsigned int byte_num;

	/* memery width -> byte number */
	byte_num = ((reg_read(base_dmc + DDR_DMC_CFG_DDRMODE) >>
		DMC_MEM_WIDTH_BIT) & DMC_MEM_WIDTH_MASK) << 1;
	/* for codedex */
	if (byte_num > DDR_PHY_BYTE_MAX) {
		byte_num = DDR_PHY_BYTE_MAX;
		ddr_error("get byte num fail");
	}

	return byte_num;
}

static void ddr_rdqs_sync_rdm(const struct ddr_cfg_st *cfg, int offset)
{
	unsigned int rdqnbdl;
	int rdm;

	rdqnbdl = reg_read(cfg->cur_phy + ddr_phy_dxnrdqnbdl2(cfg->rank_idx, cfg->cur_byte));
	rdm = (rdqnbdl >> PHY_RDM_BDL_BIT) & PHY_RDM_BDL_MASK;
	rdm += offset;
	rdm = ((rdm < 0) ? 0 : rdm);
	rdm = ((rdm > PHY_RDM_BDL_MASK) ? PHY_RDM_BDL_MASK : rdm);
	rdqnbdl = rdqnbdl & (~(PHY_RDM_BDL_MASK << PHY_RDM_BDL_BIT));
	reg_write(rdqnbdl | ((unsigned int)rdm << PHY_RDM_BDL_BIT),
		cfg->cur_phy + ddr_phy_dxnrdqnbdl2(cfg->rank_idx, cfg->cur_byte));
}

static void ddr_rdqs_sync_rank_rdq(struct ddr_cfg_st *cfg, int offset)
{
	int dq_val;
	int i;
	unsigned int cur_mode = cfg->cur_mode;

	cfg->cur_mode = DDR_MODE_READ;

	/* sync other rank rdm */
	ddr_rdqs_sync_rdm(cfg, offset);

	/* sync other rank rdq */
	ddr_debug("Before sync rank[%x] byte[%x] dq[%x = %x][%x = %x] offset[%x]",
		cfg->rank_idx, cfg->cur_byte,
		cfg->cur_phy + ddr_phy_dxnrdqnbdl0(cfg->rank_idx, cfg->cur_byte),
		reg_read(cfg->cur_phy + ddr_phy_dxnrdqnbdl0(cfg->rank_idx, cfg->cur_byte)),
		cfg->cur_phy + ddr_phy_dxnrdqnbdl1(cfg->rank_idx, cfg->cur_byte),
		reg_read(cfg->cur_phy + ddr_phy_dxnrdqnbdl1(cfg->rank_idx, cfg->cur_byte)), offset);

	for (i = 0; i < DDR_PHY_BIT_NUM; i++) {
		cfg->cur_dq = i;
		dq_val = (int)ddr_phy_get_dq_bdl(cfg);
		dq_val += offset;
		dq_val = ((dq_val < 0) ? 0 : dq_val);
		dq_val = ((dq_val > PHY_BDL_MASK) ? PHY_BDL_MASK : dq_val);
		ddr_phy_set_dq_bdl(cfg, dq_val);
	}

	cfg->cur_mode = cur_mode; /* restore to current mode */

	ddr_debug("After sync rank[%x] byte[%x] dq[%x = %x][%x = %x]",
		cfg->rank_idx, cfg->cur_byte,
		cfg->cur_phy + ddr_phy_dxnrdqnbdl0(cfg->rank_idx, cfg->cur_byte),
		reg_read(cfg->cur_phy + ddr_phy_dxnrdqnbdl0(cfg->rank_idx, cfg->cur_byte)),
		cfg->cur_phy + ddr_phy_dxnrdqnbdl1(cfg->rank_idx, cfg->cur_byte),
		reg_read(cfg->cur_phy + ddr_phy_dxnrdqnbdl1(cfg->rank_idx, cfg->cur_byte)));
}

static void ddr_rdqbdl_adj(struct ddr_cfg_st *cfg, struct ddr_bdl_dly_st *bdl_dly_s)
{
	int i;
	const int value_num = 10;
	unsigned int rank = cfg->rank_idx;
	unsigned int min = 0xffffffff;
	unsigned int rdm, rdqs;
	unsigned int cur_mode = cfg->cur_mode;

	cfg->cur_mode = DDR_MODE_READ;

	rdm = reg_read(cfg->cur_phy + ddr_phy_dxnrdqnbdl2(rank, cfg->cur_byte));
	rdqs = reg_read(cfg->cur_phy + ddr_phy_dxnrdqsdly(cfg->cur_byte));

	/* get dq value */
	for (i = 0; i < DDR_PHY_BIT_NUM; i++) {
		cfg->cur_dq = i;
		bdl_dly_s->value[i] = ddr_phy_get_dq_bdl(cfg);
	}
	bdl_dly_s->value[8] = (rdm >> PHY_RDM_BDL_BIT) & PHY_RDM_BDL_MASK; /* bdl[8]: save rdmbdl */
	bdl_dly_s->value[9] = (rdqs >> PHY_RDQS_BDL_BIT) & PHY_RDQS_BDL_MASK; /* bdl[9]: rdqsbdl */

	for (i = 0; i < value_num; i++) {
		if (bdl_dly_s->value[i] < min)
			min = bdl_dly_s->value[i];
	}

	/* subtract minimum */
	for (i = 0; i < value_num; i++)
		bdl_dly_s->value[i] = bdl_dly_s->value[i] - min;

	/* set dq value */
	for (i = 0; i < DDR_PHY_BIT_NUM; i++) {
		cfg->cur_dq = i;
		ddr_phy_set_dq_bdl(cfg, bdl_dly_s->value[i]);
	}

	rdm = (rdm & (~(PHY_RDM_BDL_MASK << PHY_RDM_BDL_BIT))) |
		(bdl_dly_s->value[8] << PHY_RDM_BDL_BIT); /* bdl[8]: save rdmbdl */
	rdqs = (rdqs & (~(PHY_RDQS_BDL_MASK << PHY_RDQS_BDL_BIT))) |
		(bdl_dly_s->value[9] << PHY_RDQS_BDL_BIT); /* bdl[9]: rdqsbdl */

	reg_write(rdm, cfg->cur_phy + ddr_phy_dxnrdqnbdl2(rank, cfg->cur_byte));
	reg_write(rdqs, cfg->cur_phy + ddr_phy_dxnrdqsdly(cfg->cur_byte));

	cfg->cur_mode = cur_mode; /* restore to current mode */
}

#ifdef DDR_TRAINING_DEBUG
#define ddr_trining_break_point_func(name) ddr_training_break_point(name)
#else
#define ddr_trining_break_point_func(name)
#endif

void ddr_training_break_point(const char* name)
{
	ddr_info(name);
	ddr_training_console_if();
}

#define __DDRT__
#ifdef DDR_DDRT_SPECIAL_CONFIG
/* Some special DDRT need read register repeatedly */
static unsigned int ddr_ddrt_read(unsigned int addr)
{
	int times = 0;
	unsigned int data0, data1, data2;
	do {
		data0 = reg_read(addr);
		data1 = reg_read(addr);
		data2 = reg_read(addr);
		times++;
	} while (((data0 != data1) || (data1 != data2)) &&
		(times < DDRT_READ_TIMEOUT));

	if (times >= DDRT_READ_TIMEOUT) {
		ddr_fatal("DDRT wait timeout");
		ddr_training_stat(DDR_ERR_DDRT_TIME_OUT, 0, -1, -1);
	}

	return data0;
}

/* Some special DDRT need write twice register */
static void ddr_ddrt_write(unsigned int data, unsigned int addr)
{
	unsigned int tmp;
	tmp = reg_read(addr);
	reg_write(data, addr);
	reg_write(data, addr);
}
#endif /* DDR_DDRT_SPECIAL_CONFIG */

static unsigned int ddr_get_rank_size(const struct ddr_cfg_st *cfg)
{
	unsigned int base_dmc = cfg->cur_dmc;
	unsigned int rnkvol;
	unsigned int mem_bank, mem_row, mem_col, mem_width;
	unsigned int size;

	mem_width = (reg_read(base_dmc + DDR_DMC_CFG_DDRMODE) >> DMC_MEM_WIDTH_BIT) & DMC_MEM_WIDTH_MASK;
	rnkvol = reg_read(base_dmc + ddr_dmc_cfg_rnkvol(0));
	mem_bank = (rnkvol >> DMC_RNKVOL_MEM_BANK_BIT) & DMC_RNKVOL_MEM_BANK_MASK;
	mem_row = (rnkvol >> DMC_RNKVOL_MEM_ROW_BIT) & DMC_RNKVOL_MEM_ROW_MASK;
	mem_col = rnkvol & DMC_RNKVOL_MEM_COL_MASK;

	/*
	 * mem_bank
	 * 0: 4 Bank(2 bank address)
	 * 1: 8 Bank(3 bank address)
	 * 2: 16 BANK(4 bank address)
	 * 3: reserved
	 * mem_row
	 * 000: 11 bit
	 * 001: 12 bit
	 * 010: 13 bit
	 * 011: 14 bit
	 * 100: 15 bit
	 * 101: 16 bit
	 * 110: 17 bit
	 * 111: 18 bit
	 * mem_width
	 * 000: 8 bit
	 * 001: 9 bit
	 * 010: 10 bit
	 * 011: 11 bit
	 * 100: 12 bit
	 */
	size = 1UL << ((mem_bank + 2) + (mem_row + 11) + (mem_col + 8) + mem_width); /* 2:2 bank; 11:11 bit; 8:8 bit */
	ddr_debug("rank size[%x]", size);

	return size;
}

/* Init DDRT register before DDRT test */
void ddr_ddrt_init(const struct ddr_cfg_st *cfg, unsigned int mode)
{
	unsigned int mem_width;
	unsigned int mem_config;
	unsigned int offset = 0;

	if (cfg->rank_idx == 1)
		offset = ddr_get_rank_size(cfg);

	ddr_training_ddrt_prepare_func();

	mem_width = ((reg_read(cfg->cur_dmc + DDR_DMC_CFG_DDRMODE) >>
		DMC_MEM_WIDTH_BIT) & DMC_MEM_WIDTH_MASK);
	mem_config = ((mem_width - 1) << DDRT_DDR_MEM_WIDTH) |
		DDRT_DDR_COL_WIDTH | DDRT_DDR_ROW_WIDTH | DDRT_DDR_BANK_WIDTH;
	/* DDRT SDRAM config */
	ddrt_reg_write(mem_config, DDR_REG_BASE_DDRT + DDRT_MEM_CONFIG);
	/* DDR Address Base */
	ddrt_reg_write(ddrt_get_test_addr(DDRT_CFG_BASE_ADDR),
		DDR_REG_BASE_DDRT + DDRT_DDR_BASE_ADDR);
	/* DDRT test DDR using space */
	ddrt_reg_write(ddrt_get_test_addr(ddr_ddrt_get_test_addr() + offset),
		DDR_REG_BASE_DDRT + DDRT_ADDR);
	ddrt_reg_write(DDRT_CFG_SEED, DDR_REG_BASE_DDRT + DDRT_SEED);

	if (mode == DDR_DDRT_MODE_GATE) {
		/* Read or Write Once */
		ddrt_reg_write(DDRT_CFG_BURST_CFG_GATE,
			DDR_REG_BASE_DDRT + DDRT_BURST_CONFIG);
		ddrt_reg_write(0x0,  DDR_REG_BASE_DDRT + DDRT_BURST_NUM);
		ddrt_reg_write(0x0,  DDR_REG_BASE_DDRT + DDRT_ADDR_NUM);
		ddrt_reg_write(0x0,  DDR_REG_BASE_DDRT + DDRT_LOOP_NUM);
		ddrt_reg_write(DDRT_CFG_REVERSED,
			DDR_REG_BASE_DDRT + DDRT_REVERSED_DQ);
	} else {
		/* reversed data form register init table */
		/* 128bit BURST4 */
		ddrt_reg_write(DDRT_CFG_BURST_CFG_DATAEYE,
			DDR_REG_BASE_DDRT + DDRT_BURST_CONFIG);
		ddrt_reg_write(cfg->phy[cfg->phy_idx].dmc[cfg->dmc_idx].ddrt_pattern,
			DDR_REG_BASE_DDRT + DDRT_REVERSED_DQ);
		ddrt_reg_write(DDRT_CFG_BURST_NUM,
			DDR_REG_BASE_DDRT + DDRT_BURST_NUM);
		ddrt_reg_write(DDRT_CFG_ADDR_NUM,
			DDR_REG_BASE_DDRT + DDRT_ADDR_NUM);
		ddrt_reg_write(DDRT_CFG_LOOP_NUM,
			DDR_REG_BASE_DDRT + DDRT_LOOP_NUM);
	}

	ddr_debug("DDRT ADDR[%x = %x]", (DDR_REG_BASE_DDRT + DDRT_ADDR),
		reg_read(DDR_REG_BASE_DDRT + DDRT_ADDR));
}

static int ddr_ddrt_test_process(int byte, int dq)
{
	unsigned int err_ovfl;
	unsigned int err_cnt;
	unsigned int dq_num;
	unsigned int dq_tmp;

	if (dq != -1) { /* check for dq */
		dq_num = ((unsigned int)byte << DDR_BYTE_DQ) + dq;
		err_ovfl = ddrt_reg_read(DDR_REG_BASE_DDRT +
			DDRT_DQ_ERR_OVFL) & (1 << dq_num);
		if (err_ovfl)
			return -1;

		if (dq > 3) /* 3: dq0-dq3 */
			dq_tmp = (unsigned int)(dq - 4) << DDR_BYTE_DQ; /* 4 dq: dq0-dq3,dq4-dq7 */
		else
			dq_tmp = (unsigned int)dq << DDR_BYTE_DQ;

		err_cnt = ddrt_reg_read(DDR_REG_BASE_DDRT +
			ddrt_dq_err_cnt(((unsigned int)byte << 1) + ((unsigned int)dq >> 2))); /* shift left 2: 4 dq */
		err_cnt = err_cnt & (0xff << dq_tmp);
		if (err_cnt)
			return -1;
	} else if (byte != -1) { /* check for byte */
		err_ovfl = ddrt_reg_read(DDR_REG_BASE_DDRT +
			DDRT_DQ_ERR_OVFL) & (0xff << ((unsigned int)byte << DDR_BYTE_DQ));
		if (err_ovfl)
			return -1;

		err_cnt  = ddrt_reg_read(DDR_REG_BASE_DDRT +
			ddrt_dq_err_cnt((unsigned int)byte << 1));
		err_cnt += ddrt_reg_read(DDR_REG_BASE_DDRT +
			ddrt_dq_err_cnt(((unsigned int)byte << 1) + 1));
		if (err_cnt)
			return -1;
	}

	return 0;
}

/*
 * @mask : DDRT option mask.
 * @byte : DDR byte index.
 * @dq   : DDR dq index.
 * DDRT test. Support read_only mode and write_read_compare mode.
 * Success return 0, fail return -1.
 */
int ddr_ddrt_test(unsigned int mask, int byte, int dq)
{
	int result;
	unsigned int regval;
	unsigned int times = 0;

	ddrt_reg_write(mask | DDRT_CFG_START, DDR_REG_BASE_DDRT + DDRT_OP);
	ddrt_reg_write(0, DDR_REG_BASE_DDRT + DDRT_STATUS);

	ddr_asm_dsb();

	do {
		regval = ddrt_reg_read(DDR_REG_BASE_DDRT + DDRT_STATUS);
		times++;
	} while ((!(regval & DDRT_TEST_DONE_MASK)) && (times < DDRT_WAIT_TIMEOUT));

	if (times >= DDRT_WAIT_TIMEOUT) {
		ddr_fatal("DDRT wait timeout");
		ddr_training_stat(DDR_ERR_DDRT_TIME_OUT, 0, -1, -1);
		return -1;
	}

	/* DDRT_READ_ONLY_MODE */
	if ((mask & DDRT_TEST_MODE_MASK) == DDRT_READ_ONLY_MODE)
		return 0;   /* return when DDRT finish */

	/* DDRT_WR_COMPRARE_MODE No error occurred, test pass. */
	if (regval & DDRT_TEST_PASS_MASK)
		return 0;

	result = ddr_ddrt_test_process(byte, dq);

	return result;
}

/* Check ddrt test result. Success return 0, fail return -1 */
static int ddr_ddrt_check(const struct ddr_cfg_st *cfg)
{
	unsigned int byte_index_to_dmc = cfg->cur_byte;

	/* ddrt test the byte relate to dmc, make sure not overflow */
	if (cfg->cur_byte >= (cfg->dmc_idx << 1))
		byte_index_to_dmc = cfg->cur_byte - (cfg->dmc_idx << 1);

	ddrt_reg_write(0, DDR_REG_BASE_DDRT + DDRT_REVERSED_DQ);
	if (ddr_ddrt_test(DDRT_WR_COMPRARE_MODE | DDRT_PATTERM_PRBS9, byte_index_to_dmc, cfg->cur_dq))
		return -1;

	ddrt_reg_write(cfg->cur_pattern, DDR_REG_BASE_DDRT + DDRT_REVERSED_DQ);
	if (ddr_ddrt_test(DDRT_WR_COMPRARE_MODE | DDRT_PATTERM_PRBS11, byte_index_to_dmc, cfg->cur_dq))
		return -1;

	return 0;
}

#define __DATAEYE_ADJUST__
#ifdef DDR_TRAINING_ADJUST_CONFIG
static unsigned int ddr_adjust_get_average(const struct ddr_cfg_st *cfg)
{
	unsigned int dq0_3, dq4_7, val;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int byte_index = cfg->cur_byte;
	unsigned int rank = cfg->rank_idx;

	if (cfg->cur_mode == DDR_MODE_WRITE)
		return (reg_read(base_phy + ddr_phy_dxnwdqnbdl2(rank, byte_index)) >>
			PHY_WDM_BDL_BIT) & PHY_BDL_MASK;

	/* read */
	dq0_3 = reg_read(base_phy + ddr_phy_dxnrdqnbdl0(rank, byte_index));
	dq4_7 = reg_read(base_phy + ddr_phy_dxnrdqnbdl1(rank, byte_index));

	val = ((dq0_3 >> PHY_BDL_DQ0_BIT) & PHY_BDL_MASK) +
		((dq0_3 >> PHY_BDL_DQ1_BIT) & PHY_BDL_MASK) +
		((dq0_3 >> PHY_BDL_DQ2_BIT) & PHY_BDL_MASK) +
		((dq0_3 >> PHY_BDL_DQ3_BIT) & PHY_BDL_MASK) +
		((dq4_7 >> PHY_BDL_DQ0_BIT) & PHY_BDL_MASK) +
		((dq4_7 >> PHY_BDL_DQ1_BIT) & PHY_BDL_MASK) +
		((dq4_7 >> PHY_BDL_DQ2_BIT) & PHY_BDL_MASK) +
		((dq4_7 >> PHY_BDL_DQ3_BIT) & PHY_BDL_MASK);

	val = val >> 3; /* shift 3: 8 dq */

	return val;
}

/*
 * @accel : Return a value to adjust quickly.
 * Check dataeye DQ window on left or right or middle.
 */
static unsigned int ddr_adjust_trend_check(const struct ddr_cfg_st *cfg, int *accel)
{
	unsigned int dq_bdl;
	unsigned int size;

	/* 32 BDL middle[13, 17]. 128 BDL middle[40, 56] */
	/* 1 Phase = (DDR_BDL_PHASE_TRANSFORM) BDL */
	size = DDR_BDL_PHASE_TRANSFORM >> 1;
	dq_bdl =  ddr_adjust_get_average(cfg);

	/* increase adjust step to accelerate */
	if (accel != NULL) {
		if (dq_bdl > PHY_DQ_BDL_MIDDLE)
			*accel = dq_bdl - PHY_DQ_BDL_MIDDLE;
		else if (dq_bdl < PHY_DQ_BDL_MIDDLE)
			*accel = PHY_DQ_BDL_MIDDLE - dq_bdl;

		ddr_info("byte[%x] bdl[%x] middle[%x] accel[%x] rdqs[%x]",
			cfg->cur_byte, dq_bdl, PHY_DQ_BDL_MIDDLE, *accel,
			(reg_read(cfg->cur_phy + ddr_phy_dxnrdqsdly(cfg->cur_byte)) >>
			PHY_RDQS_BDL_BIT) & PHY_RDQS_BDL_MASK);
	}

	/* window on left */
	if (dq_bdl < (PHY_DQ_BDL_MIDDLE - size))
		return DDR_WIN_LEFT;
	/* on right */
	else if (dq_bdl > (PHY_DQ_BDL_MIDDLE + size))
		return DDR_WIN_RIGHT;
	else
		return DDR_WIN_MIDDLE;
}

/* Check adjust value whether valid */
static int ddr_adjust_check_val(int val, unsigned int mode)
{
	if (mode == DDR_MODE_READ) {
		if (val < 0 || val > PHY_RDQS_BDL_MASK)
			return DDR_FALSE;
	} else {
		if (val < 0 || val > PHY_WDQ_PHASE_MASK)
			return DDR_FALSE;
	}

	return DDR_TRUE;
}

static void ddr_rdqs_sync(struct ddr_cfg_st *cfg, int val)
{
	unsigned int rdqsdly;
	unsigned int cur_rank = cfg->rank_idx;
	int old, offset;

	rdqsdly = reg_read(cfg->cur_phy + ddr_phy_dxnrdqsdly(cfg->cur_byte));
	old = (rdqsdly >> PHY_RDQS_BDL_BIT) & PHY_RDQS_BDL_MASK;
	offset = val - old;

	/* sync rdm */
	ddr_rdqs_sync_rank_rdq(cfg, offset);

	if (cfg->phy[cfg->phy_idx].rank_num == 1) {
		ddr_debug("Rank number[%x] not need sync another rank", cfg->phy[cfg->phy_idx].rank_num);
		return;
	}

	/* sync other rank rdm and rdq */
	cfg->rank_idx = DDR_SUPPORT_RANK_MAX - 1 - cur_rank; /* switch to another rank */
	ddr_rdqs_sync_rank_rdq(cfg, offset);
	cfg->rank_idx = cur_rank; /* resotre to cur rank */
}

static void ddr_set_rdqs(struct ddr_cfg_st *cfg, int val)
{
	unsigned int delay;
	delay = reg_read(cfg->cur_phy + ddr_phy_dxnrdqsdly(cfg->cur_byte));

	ddr_phy_rdqs_sync_rdm(cfg, val);

	/* clear rdqs bdl */
	delay = delay & (~(PHY_RDQS_BDL_MASK << PHY_RDQS_BDL_BIT));

	reg_write(delay | ((unsigned int)val << PHY_RDQS_BDL_BIT),
		cfg->cur_phy + ddr_phy_dxnrdqsdly(cfg->cur_byte));
}

/* Get value which need to adjust */
static int ddr_adjust_get_val(const struct ddr_cfg_st *cfg)
{
	if (cfg->cur_mode == DDR_MODE_READ)
		return (reg_read(cfg->cur_phy + ddr_phy_dxnrdqsdly(cfg->cur_byte)) >>
			PHY_RDQS_BDL_BIT) & PHY_RDQS_BDL_MASK;
	else
		return (reg_read(cfg->cur_phy + ddr_phy_dxnwdqdly(cfg->rank_idx, cfg->cur_byte)) >>
			PHY_WDQ_PHASE_BIT) & PHY_WDQ_PHASE_MASK;
}

/* Set value which need to adjust */
static void ddr_adjust_set_val(struct ddr_cfg_st *cfg, int val)
{
	unsigned int delay;

	if (cfg->cur_mode == DDR_MODE_READ) {
		ddr_set_rdqs(cfg, val);
	} else {
		delay = reg_read(cfg->cur_phy + ddr_phy_dxnwdqdly(cfg->rank_idx, cfg->cur_byte));
		/* clear wdq phase */
		delay = delay & (~(PHY_WDQ_PHASE_MASK << PHY_WDQ_PHASE_BIT));

		reg_write(delay | ((unsigned int)val << PHY_WDQ_PHASE_BIT),
			cfg->cur_phy + ddr_phy_dxnwdqdly(cfg->rank_idx, cfg->cur_byte));
	}

	ddr_phy_cfg_update(cfg->cur_phy);
}

/* Add or delete value to adjust */
static void ddr_adjust_change_val(unsigned int dir, int *val,
	int step, unsigned int mode)
{
	if (mode == DDR_MODE_READ) {
		if (dir == DDR_WIN_RIGHT)
			(*val) = (*val) + step;
		else
			(*val) = (*val) - step;
	} else {
		/* decrease wdq phase, window move to right */
		if (dir == DDR_WIN_RIGHT)
			(*val) = (*val) - step;
		else
			(*val) = (*val) + step;
	}
}

/*
 * @dir : move direction. DDR_TRUE move to right, DDR_FALSE move to left.
 * Move window to specified direction until the best DQ bdl beyond the midline.
 */
static void ddr_adjust_move_win(struct ddr_cfg_st *cfg,
	struct training_data *training, int step, unsigned int dir)
{
	int cur_val, def_val;
	int accel;
	unsigned int i;
	unsigned int trend;
	unsigned int max_value;

	max_value = ((cfg->cur_mode == DDR_MODE_WRITE) ? PHY_WDQ_PHASE_MASK : PHY_RDQS_BDL_MASK);

	def_val = ddr_adjust_get_val(cfg);
	cur_val = def_val;
	for (i = 0; i <= max_value; i++) {
		accel = step;
		/* write mode no need to accelerate */
		if (cfg->cur_mode == DDR_MODE_WRITE)
			trend = ddr_adjust_trend_check(cfg, 0);
		else
			trend = ddr_adjust_trend_check(cfg, &accel);

		if ((trend == DDR_WIN_MIDDLE) || (trend == dir)) {
			ddr_debug("Move byte[%x] window to middle suc", cfg->cur_byte);
			break;
		}

		ddr_adjust_change_val(dir, &cur_val, accel, cfg->cur_mode);
		if (ddr_adjust_check_val(cur_val, cfg->cur_mode) == DDR_FALSE) {
			ddr_warning("Move byte[%x] to middle fail. value[%x]",
				cfg->cur_byte, cur_val);
			break;
		}

		ddr_debug("Byte[%x] mode[%x] set value[%x]",
			cfg->cur_byte, cfg->cur_mode, cur_val);
		ddr_adjust_set_val(cfg, cur_val);
		if (ddr_dataeye_deskew(cfg, training)) {
			ddr_adjust_set_val(cfg, def_val);
			/* MUST deskew dataeye after restore rdqs */
			ddr_dataeye_deskew(cfg, training);
			ddr_error("Byte[%x] deskew fail, restore[%x]",
				cfg->cur_byte, def_val);
			break;
		}
	}
}

/* Adjust specified byte winodw to middle */
static void ddr_adjust_byte(struct ddr_cfg_st *cfg, struct training_data *training)
{
	unsigned int trend;

	trend = ddr_adjust_trend_check(cfg, 0);
	/* window on left, move to right */
	if (trend == DDR_WIN_LEFT)
		ddr_adjust_move_win(cfg, training, DDR_DQS_ADJ_STEP, DDR_WIN_RIGHT);
	/* window on right, move to left */
	else if (trend == DDR_WIN_RIGHT)
		ddr_adjust_move_win(cfg, training, DDR_DQS_ADJ_STEP, DDR_WIN_LEFT);
	/* window on middle, no need to move */
	else
		ddr_debug("Byte[%x] mode[%x] win on middle",
			cfg->cur_byte, cfg->cur_mode);
}

/*
 * Adjust PHY dataeye. On normal case,
 * read dateeye window on left after read dataeye hardware training,
 * write dataeye window on left after write leveling training.
 */
void ddr_adjust_dataeye(struct ddr_cfg_st *cfg, struct training_data *training)
{
	unsigned int i;

	/* dataeye adjust disable */
	if (ddr_training_check_bypass(cfg, DDR_BYPASS_DATAEYE_ADJ_MASK) != DDR_FALSE)
		return;

	ddr_debug("DDR dataeye adjust PHY[%x][%x] DMC[%x][%x] Rank[%x]",
		cfg->phy_idx, cfg->cur_phy, cfg->dmc_idx, cfg->cur_dmc, cfg->rank_idx);

	if (cfg->adjust == DDR_FALSE)
		return;

	for (i = 0; i < get_byte_num(cfg); i++) {
		cfg->cur_byte = i + (cfg->dmc_idx << 1); /* byte index accord to phy */
		ddr_adjust_byte(cfg, training);
	}
}
#else
#define ddr_adjust_dataeye(cfg, training)
#endif /* DDR_TRAINING_ADJUST_CONFIG */

#define __DATAEYE_TRAINING__
#ifdef DDR_DATAEYE_TRAINING_CONFIG
/* Check dataeye dq */
int ddr_dataeye_check_dq(const struct ddr_cfg_st *cfg)
{
	if (cfg->dq_check_type == DDR_CHECK_TYPE_DDRT)
		return ddr_ddrt_check(cfg);
	else if (cfg->dq_check_type == DDR_CHECK_TYPE_MPR)
		return ddr_mpr_check(cfg);
	else
		ddr_error("DDR dataeye dq check type not set");

	return 0;
}

/* Check dq whether valid and set mask to reduce search time */
static int ddr_dataeye_check_dir(unsigned int direction, unsigned int left,
	unsigned int right, unsigned int *mask, const struct ddr_cfg_st *cfg)
{
	int result;

	result = ddr_dataeye_check_dq(cfg);
	switch (direction) {
	case DDR_FIND_DQ_BOTH:
		*mask = DDR_FIND_DQ_LEFT | DDR_FIND_DQ_RIGHT;
		break;
	case DDR_FIND_DQ_LEFT:
		if (result) {
			/* ddr test error, search opposite side */
			*mask = DDR_FIND_DQ_RIGHT;
		} else { /* ddr test ok */
			ddr_phy_set_dq_bdl(cfg, left);
			if (!ddr_dataeye_check_dq(cfg))
				/* test ok, go on search this side */
				*mask = DDR_FIND_DQ_LEFT;
		}
		break;
	case DDR_FIND_DQ_RIGHT:
		if (result) {  /* ddr test error, search opposite side */
			*mask = DDR_FIND_DQ_LEFT;
		} else { /* ddr test ok */
			ddr_phy_set_dq_bdl(cfg, right);
			if (!ddr_dataeye_check_dq(cfg))
				/* test OK, go on search this side */
				*mask = DDR_FIND_DQ_RIGHT;
		}
		break;
	default:
		break;
	}

	return result;
}

/* Binary search the valid dq bdl */
static void ddr_dataeye_search_dq(unsigned int left, unsigned int right,
	int *target, unsigned int direction, const struct ddr_cfg_st *cfg)
{
	unsigned int middle;
	unsigned int mask = 0;

	middle = left + ((right - left) >> 1);

	ddr_phy_set_dq_bdl(cfg, middle);
	if (!ddr_dataeye_check_dir(direction, left, right, &mask, cfg)) { /* test ok */
		*target = (int)middle;
		return;
	}

	if (left == middle || middle == right) /* not found */
		return;

	/* find left side */
	if (DDR_FIND_DQ_LEFT & mask)
		ddr_dataeye_search_dq(left, middle, target, direction, cfg);

	/* find right side */
	if (DDR_FIND_DQ_RIGHT & mask)
		ddr_dataeye_search_dq(middle, right, target, direction, cfg);

	return;
}

/* Find DQ valid range */
static void ddr_dataeye_find_dq(const struct ddr_cfg_st *cfg, struct training_data *training)
{
	int cur_dq, left_dq, right_dq, def_dq;
	unsigned int dq_num;
	unsigned int win_num;

	dq_num = (cfg->cur_byte << DDR_BYTE_DQ) + cfg->cur_dq;
	def_dq = (int)ddr_phy_get_dq_bdl(cfg);
	cur_dq = def_dq;

	/* check default dq */
	if (ddr_dataeye_check_dq(cfg)) {
		/* test error */
		cur_dq = -1;
		ddr_dataeye_search_dq(0, PHY_BDL_MASK, &cur_dq, DDR_FIND_DQ_BOTH, cfg);
		ddr_debug("DQ[%x] def[%x] not ok, find new value[%x]", dq_num, def_dq, cur_dq);
		if (cur_dq == -1) {  /* no valid dq */
			training->ddr_bit_result[dq_num] = 0;
			training->ddr_bit_best[dq_num]   = 0;
			/* restore default value */
			ddr_phy_set_dq_bdl(cfg, def_dq);
			ddr_warning("DQ[%x] not found dq. restore[%x]", dq_num, def_dq);
			return;
		}
	}
	/* find the left boundary */
	left_dq = cur_dq;
	ddr_dataeye_search_dq(0, cur_dq, &left_dq, DDR_FIND_DQ_LEFT, cfg);
	while (left_dq > 0) {
		left_dq--;
		ddr_phy_set_dq_bdl(cfg, left_dq);
		if (ddr_dataeye_check_dq(cfg)) {
			/* test error */
			left_dq++;
			break;
		}
	}
	/* find the right boundary */
	right_dq = cur_dq;
	ddr_dataeye_search_dq(cur_dq, PHY_BDL_MASK, &right_dq, DDR_FIND_DQ_RIGHT, cfg);
	while (right_dq < PHY_BDL_MASK) {
		right_dq++;
		ddr_phy_set_dq_bdl(cfg, right_dq);
		if (ddr_dataeye_check_dq(cfg)) {
			/* test error */
			right_dq--;
			break;
		}
	}
	/* reset dq */
	ddr_phy_set_dq_bdl(cfg, def_dq);

	/*
	 * 0 1 2 3 4 5 6 7 8 9
	 * x x - - - - - x x x
	 *     |       |
	 * left_dq   right_dq
	 *
	 * so left_dq = 2, right_dq = 6
	 */
	win_num = right_dq - left_dq + 1;
	training->ddr_bit_result[dq_num] = ((unsigned int)left_dq << DDR_DATAEYE_RESULT_BIT) |
		(unsigned int)right_dq;
	training->ddr_bit_best[dq_num] = (win_num << DDR_DATAEYE_RESULT_BIT) |
		((win_num >> 1) + (unsigned int)left_dq);

	ddr_info("DQ[%x] range: left[%x] right[%x] best[%x] mode[%x] rank[%x]", dq_num,
		left_dq, right_dq, training->ddr_bit_best[dq_num], cfg->cur_mode, cfg->rank_idx);
}

/* DDR dataeye training one byte. */
int ddr_dataeye_deskew(struct ddr_cfg_st *cfg, struct training_data *training)
{
	unsigned int dq_num;
	unsigned int loop_times = 0;
	unsigned int win_num, dq_sum;
	unsigned int def_dq, best_dq;
	int i;

	if (training == NULL)
		return -1;

	dq_sum = 0;
	training->ddr_win_sum = 0;
	for (i = 0; i < DDR_PHY_BIT_NUM; i++) {
		cfg->cur_dq = i;
		dq_num = (cfg->cur_byte << DDR_BYTE_DQ) + i;
		def_dq = ddr_phy_get_dq_bdl(cfg);
		ddr_dataeye_find_dq(cfg, training);
		win_num = training->ddr_bit_best[dq_num] >> DDR_DATAEYE_RESULT_BIT;
		best_dq = training->ddr_bit_best[dq_num] & DDR_DATAEYE_RESULT_MASK;
		/* check window number */
		if (win_num < DDR_DATAEYE_WIN_NUM) {
			if (loop_times < DDR_LOOP_TIMES_LMT) {
				loop_times++;
				i--;
				continue;
			} else if (win_num == 0) {
				ddr_warning("Byte[%x] DQ[%x] no win", cfg->cur_byte, dq_num);
				/* restore default value */
				ddr_phy_set_dq_bdl(cfg, def_dq);
				ddr_training_stat(DDR_ERR_DATAEYE, cfg->cur_phy, cfg->cur_byte, i);
				continue;
			}
		}
		loop_times = 0;
		ddr_phy_set_dq_bdl(cfg, best_dq);
		dq_sum = dq_sum + best_dq;
		training->ddr_win_sum = training->ddr_win_sum + win_num;
	}
	dq_sum = dq_sum >> DDR_BYTE_DQ;

	/* only DDR_MODE_WRITE need to set */
	if (cfg->cur_mode == DDR_MODE_WRITE)
		reg_write((dq_sum & PHY_BDL_MASK) << PHY_WDM_BDL_BIT, cfg->cur_phy +
			ddr_phy_dxnwdqnbdl2(cfg->rank_idx, cfg->cur_byte));

	ddr_phy_cfg_update(cfg->cur_phy);

	return 0;
}

/* DDR write or read dataeye training */
static int ddr_dataeye_process(struct ddr_cfg_st *cfg, struct training_data *training)
{
	int result = 0;
	unsigned int i;

	/* dataeye training */
	for (i = 0; i < get_byte_num(cfg); i++) {
		cfg->cur_byte = i + (cfg->dmc_idx << 1); /* byte index accord to phy */
		result += ddr_dataeye_deskew(cfg, training);
	}

	if (result) {
		result = -1;
		ddr_error("PHY[%x] mode[%x] dataeye training fail", cfg->cur_phy, cfg->cur_mode);
	} else {
		/* dataeye training result adjust */
		ddr_adjust_dataeye(cfg, training);
	}
	/* save training result to printf */
	ddr_result_data_save(cfg, training);

	return result;
}

int ddr_dataeye_training(struct ddr_cfg_st *cfg)
{
	struct training_data tmp_result;
	struct training_data *training = &tmp_result;
	int result_read, result_write;

	ddr_debug("DDR dataeye training PHY[%x][%x] DMC[%x][%x] Rank[%x]",
		cfg->phy_idx, cfg->cur_phy, cfg->dmc_idx, cfg->cur_dmc, cfg->rank_idx);

	/* write dataeye training */
	cfg->cur_mode = DDR_MODE_WRITE;
	ddrtr_set_data(training, 0, sizeof(struct training_data));
	result_write = ddr_dataeye_process(cfg, training);

	/* read dataeye training */
	cfg->cur_mode = DDR_MODE_READ;
	ddrtr_set_data(training, 0, sizeof(struct training_data));
	result_read = ddr_dataeye_process(cfg, training);
	if (result_read || result_write)
		return -1;
	else
		return 0;
}

int ddr_dataeye_training_func(struct ddr_cfg_st *cfg)
{
	struct tr_relate_reg relate_reg;
	int result;

	/* dataeye training disable */
	if (ddr_training_check_bypass(cfg, DDR_BYPASS_DATAEYE_MASK) != DDR_FALSE)
		return 0;

	ddr_training_save_reg(cfg, &relate_reg, DDR_BYPASS_DATAEYE_MASK);
	ddr_training_switch_axi(cfg);
	ddr_ddrt_init(cfg, DDR_DDRT_MODE_DATAEYE);
	cfg->adjust = DDR_DATAEYE_NORMAL_ADJUST;
	cfg->dq_check_type = DDR_CHECK_TYPE_DDRT;
	result = ddr_dataeye_training(cfg);
	ddr_training_restore_reg(cfg, &relate_reg);

	return result;
}
#else
int ddr_dataeye_training_func(struct ddr_cfg_st *cfg)
{
	ddr_warning("Not support DDR dataeye training");

	return 0;
}
#endif  /* DDR_DATAEYE_TRAINING_CONFIG */

#define __HARDWARE_TRAINING__
#ifdef DDR_HW_TRAINING_CONFIG
#ifdef DDR_HW_READ_ADJ_CONFIG
/*
 * Adjust rdqs and dq after hw read training.
 * When define DDR_TRAINING_ADJUST_DISABLE, MUST define DDR_HW_READ_ADJ_CONFIG.
 */
static void ddr_hw_read_adj(const struct ddr_cfg_st *cfg)
{
	int i;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int byte_num = cfg->phy[cfg->phy_idx].total_byte_num;

	ddr_debug("DDR hw read adjust");
	/* check hw read adjust bypass bit */
	if (ddr_training_check_bypass(cfg, DDR_BYPASS_HW_ADJ_MASK) != DDR_FALSE)
		return;

	/* assume read dataeye window on left */
	for (i = 0; i < byte_num; i++) {
		reg_write(reg_read(base_phy + ddr_phy_dxnrdqnbdl0(cfg->rank_idx, i)) +
			(PHY_DQ_MIDDLE_VAL << PHY_BDL_DQ_BIT),
			base_phy + ddr_phy_dxnrdqnbdl0(cfg->rank_idx, i));
		reg_write(reg_read(base_phy + ddr_phy_dxnrdqnbdl1(cfg->rank_idx, i)) +
			(PHY_DQ_MIDDLE_VAL << PHY_BDL_DQ_BIT),
			base_phy + ddr_phy_dxnrdqnbdl1(cfg->rank_idx, i));
		reg_write(reg_read(base_phy + ddr_phy_dxnrdqsdly(i)) +
			(PHY_RDQS_MIDDLE_VAL << PHY_RDQS_BDL_BIT),
			base_phy + ddr_phy_dxnrdqsdly(i));
	}
}
#else
static void ddr_hw_read_adj(const struct ddr_cfg_st *cfg)
{
}
#endif /* DDR_HW_READ_ADJ_CONFIG */

static void ddr_training_get_rdqs(const struct ddr_cfg_st *cfg, struct ddr_bdl_st *rdqs)
{
	unsigned int i;
	unsigned int byte_num = cfg->phy[cfg->phy_idx].total_byte_num;
	unsigned int base_phy = cfg->cur_phy;

	for (i = 0; i < byte_num; i++)
		rdqs->bdl[i] = reg_read(base_phy + ddr_phy_dxnrdqsdly(i));
}

static void ddr_training_set_rdqs(const struct ddr_cfg_st *cfg, const struct ddr_bdl_st *rdqs)
{
	unsigned int i;
	unsigned int byte_num = cfg->phy[cfg->phy_idx].total_byte_num;
	unsigned int base_phy = cfg->cur_phy;

	for (i = 0; i < byte_num; i++)
		reg_write(rdqs->bdl[i], base_phy + ddr_phy_dxnrdqsdly(i));
}

static void ddr_hw_training_adjust_rdqs(struct ddr_cfg_st *cfg, const struct rdqs_data_st *rdqs_st)
{
	unsigned int i;
	unsigned int byte_num = cfg->phy[cfg->phy_idx].total_byte_num;
	unsigned int rdqs_rank0, rdqs_rank1;
	unsigned int cur_rank = cfg->rank_idx;
	int offset;

	for (i = 0; i < byte_num; i++) {
		/* struct rdqs_data_st store the whole register value */
		rdqs_rank0 = (rdqs_st->rank[0].bdl[i] >> PHY_RDQS_BDL_BIT) & PHY_RDQS_BDL_MASK;
		rdqs_rank1 = (rdqs_st->rank[1].bdl[i] >> PHY_RDQS_BDL_BIT) & PHY_RDQS_BDL_MASK;

		cfg->cur_byte = i;
		if (rdqs_rank0 > rdqs_rank1) {
			offset = rdqs_rank0 - rdqs_rank1;
			reg_write(rdqs_st->rank[0].bdl[i], cfg->cur_phy + ddr_phy_dxnrdqsdly(i));
			cfg->rank_idx = 1; /* switch to rank1 for sync rank1 rdq */
		} else {
			offset = rdqs_rank1 - rdqs_rank0;
			reg_write(rdqs_st->rank[1].bdl[i], cfg->cur_phy + ddr_phy_dxnrdqsdly(i));
			cfg->rank_idx = 0; /* switch to rank0 for sync rank0 rdq */
		}
		ddr_rdqs_sync_rank_rdq(cfg, offset);
	}
	cfg->rank_idx = cur_rank; /* restore to current rank */

	ddr_phy_cfg_update(cfg->cur_phy);
}

/* DDR HW training process */
int ddr_hw_training_process(const struct ddr_cfg_st *cfg, unsigned int item)
{
	unsigned int count;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int init_ctrl = reg_read(base_phy + DDR_PHY_PHYINITCTRL);

	if (!item)
		return 0;

	ddr_debug("base_phy[%x] itme[%x]", base_phy, item);
	/* hardware training enable */
	reg_write(item | PHY_PHYINITCTRL_INIT_EN | init_ctrl, base_phy + DDR_PHY_PHYINITCTRL);

	if ((item & PHY_PHYINITCTRL_DRAM_RST) && (item & PHY_PHYINITCTRL_DRAM_INIT_EN)) {
		if (ddr_training_ctrl_easr(cfg, DDR_EXIT_SREF))
			return -1;
	}

	count = DDR_HWR_WAIT_TIMEOUT;
	/* auto cleared to 0 after training finished */
	while (count--) {
		if (!(reg_read(base_phy + DDR_PHY_PHYINITCTRL) & PHY_PHYINITCTRL_MASK))
			break;
	}

	if (count == 0xffffffff) {
		ddr_fatal("HWR wait timeout");
		ddr_training_stat(DDR_ERR_HW_RD_DATAEYE, base_phy, item, reg_read(base_phy + DDR_PHY_PHYINITSTATUS));
		return -1;
	}

	if (reg_read(base_phy + DDR_PHY_PHYINITSTATUS)) {
		ddr_fatal("Phy[%x] hw[%x] failed[%x]", base_phy, item, reg_read(base_phy + DDR_PHY_PHYINITSTATUS));
		ddr_training_stat(DDR_ERR_HW_RD_DATAEYE, base_phy, item, reg_read(base_phy + DDR_PHY_PHYINITSTATUS));
		return -1;
	}

	return 0;
}

/* Dataeye hardware training */
int ddr_hw_dataeye_read(struct ddr_cfg_st *cfg)
{
	unsigned int base_phy;
	unsigned int byte_num;

	unsigned int i;
	int result;

	base_phy = cfg->cur_phy;
	byte_num = cfg->phy[cfg->phy_idx].total_byte_num;

	ddr_training_cfg_init(cfg);

	/* clear */
	for (i = 0; i < byte_num; i++) {
		reg_write(0, base_phy + ddr_phy_dxnrdqnbdl0(cfg->rank_idx, i));
		reg_write(0, base_phy + ddr_phy_dxnrdqnbdl1(cfg->rank_idx, i));
		reg_write(0, base_phy + ddr_phy_dxnrdqsdly(i));
	}
	ddr_phy_cfg_update(base_phy);

	result = ddr_hw_training_process(cfg, PHY_PHYINITCTRL_RDET_EN);

	ddr_hw_read_adj(cfg);

	return result;
}

/* DDR HW training adapt dram type */
static int ddr_hw_dataeye_adapt(const struct ddr_cfg_st *cfg, struct ddr_tmp_st *ddr_temp)
{
	int result;
	unsigned int temp = 0;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int item = cfg->cur_item;

	if (cfg->phy[cfg->phy_idx].dram_type == PHY_DRAMCFG_TYPE_LPDDR4) {
		temp = reg_read(base_phy + 0x64);
		reg_write(temp & 0x0fffffff, base_phy + 0x64); /* ca odt disable */

		result = ddr_hw_training_process(cfg, item & PHY_HW_GP_DRAM_RESET);
		reg_write(temp, base_phy + 0x64); /* restore */

		temp = reg_read(base_phy + 0x48);
		reg_write(temp & 0xfffffffe, base_phy + 0x48); /* todo rank0 */

		result += ddr_hw_training_process(cfg, item & PHY_HW_GP_VREF_AC);

		reg_write(temp | 0x1, base_phy + 0x48); /* rank1 */

		result += ddr_hw_training_process(cfg, item & PHY_HW_GP_VREF_AC);

		reg_write(temp, base_phy + 0x48); /* restore */

		result += ddr_hw_training_process(cfg, item & PHY_PHYINITCTRL_DRAM_INIT_EN);
	} else {
#ifdef DDR_WRITE_DM_DISABLE
		if (cfg->phy[cfg->phy_idx].dram_type == PHY_DRAMCFG_TYPE_DDR4) {
			temp = reg_read(base_phy + 0xe0);
			reg_write((temp & 0xFBFFFFFF) | 0x8000000, base_phy + 0xe0); /* write dm disable */
		}
#endif
		result = ddr_hw_training_process(cfg, item & PHY_HW_GP_DRAM_RESET);
	}
	ddr_temp->temp = temp;

	return result;
}

static int ddr_hw_dataeye_vref_set(const struct ddr_cfg_st *cfg)
{
	int result;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int item = cfg->cur_item;
	unsigned int dvrft_ctrl;

	dvrft_ctrl = reg_read(base_phy + DDR_PHY_DVRFTCTRL);
	reg_write(dvrft_ctrl & (~PHY_DVRFTCTRL_PDAEN_EN), base_phy + DDR_PHY_DVRFTCTRL);
	/* DDR_PHY_VREFTCTRL 31bit:1 do vref dram set twice */
	reg_write((reg_read(base_phy + DDR_PHY_VREFTCTRL) &
		(~(PHY_VREFS_MRS_ENTER_MASK << PHY_VREFS_MRS_ENTER_BIT))) |
		(PHY_VREFS_MRS_ENTER_MASK << PHY_VREFS_MRS_ENTER_BIT),
		base_phy + DDR_PHY_VREFTCTRL);
	result = ddr_hw_training_process(cfg, item & PHY_HW_GP_VREF_DQ);
	result += ddr_hw_training_process(cfg, item & PHY_HW_GP_VREF_DQ);
	/* DDR_PHY_VREFTCTRL 31bit:0 do vref dram set once */
	reg_write(reg_read(base_phy + DDR_PHY_VREFTCTRL) &
		(~(PHY_VREFS_MRS_ENTER_MASK << PHY_VREFS_MRS_ENTER_BIT)),
		base_phy + DDR_PHY_VREFTCTRL);
	result += ddr_hw_training_process(cfg, item & PHY_HW_GP_VREF_DQ);
	reg_write(dvrft_ctrl, base_phy + DDR_PHY_DVRFTCTRL);

	return result;
}

#ifdef DDR_WRITE_DM_DISABLE
static int ddr_hw_write_dm_disable(const struct ddr_cfg_st *cfg, const struct ddr_tmp_st *ddr_temp)
{
	int result = 0;
	unsigned int temp;
	unsigned int temp1;

	if (cfg->phy[cfg->phy_idx].dram_type == PHY_DRAMCFG_TYPE_DDR4) {
		reg_write(ddr_temp->temp, cfg->cur_phy + DDR_PHY_MODEREG45); /* restore */
		temp = reg_read(cfg->cur_phy + DDR_PHY_MRS_SEQ_PROG);
		temp1 = reg_read(cfg->cur_phy + DDR_PHY_DRAMCFG);
		reg_write(PHY_MRS_SEQ_PROG_VAL, cfg->cur_phy + DDR_PHY_MRS_SEQ_PROG); /* inti MR5 */
		reg_write(temp1 | PHY_WDM_DISABLE_VAL, cfg->cur_phy + DDR_PHY_DRAMCFG); /* write dm disable */
		result += ddr_hw_training_process(cfg, cfg->cur_item & PHY_PHYINITCTRL_DRAM_INIT_EN);
		reg_write(temp, cfg->cur_phy + DDR_PHY_MRS_SEQ_PROG); /* restore */
		reg_write(temp1, cfg->cur_phy + DDR_PHY_DRAMCFG); /* restore */
	}

	return result;
}
#endif

/* DDR HW training control */
int ddr_hw_training_ctl(struct ddr_cfg_st *cfg)
{
	int result = 0;
	unsigned int byte_idx;
	struct ddr_tmp_st ddr_temp;
	struct rdqs_data_st *rdqs_st = NULL;
	struct ddr_bdl_dly_st bdl_dly_s;

	rdqs_st = (struct rdqs_data_st *)cfg->res_st;

	if (cfg->cur_item == 0 || rdqs_st == NULL)
		return 0;

	ddr_phy_cfg_update(cfg->cur_phy);
	/* NOTE: not support array when boot */
	result += ddr_hw_training_process(cfg, cfg->cur_item & PHY_HW_GP_CNT_RESET_START);
	result += ddr_hw_training_process(cfg, cfg->cur_item & PHY_HW_GP_PLL);

	/* save rdqs bdl after PHY_PHYINITCTRL_DLYMEAS_EN */
	if (cfg->rank_idx == 0)
		ddr_training_get_rdqs(cfg, &rdqs_st->origin);
	if ((cfg->phy_idx >= DDR_PHY_NUM) || (cfg->phy[cfg->phy_idx].total_byte_num > DDR_PHY_BYTE_MAX)) {
		ddr_error("phy_idx or byte number error");
		return -1;
	}
	for (byte_idx = 0; byte_idx < (cfg->phy[cfg->phy_idx].total_byte_num); byte_idx++) {
		cfg->cur_byte = byte_idx;
		ddr_rdqbdl_adj(cfg, &bdl_dly_s);
	}
	result += ddr_hw_dataeye_adapt(cfg, &ddr_temp);
	result += ddr_hw_training_process(cfg, cfg->cur_item & PHY_PHYINITCTRL_CAT_EN);
	result += ddr_hw_training_process(cfg, cfg->cur_item & PHY_HW_GP_CS);
	result += ddr_hw_dataeye_vref_set(cfg);
	result += ddr_hw_training_process(cfg, cfg->cur_item & PHY_HW_GP_NORMAL);

#ifdef DDR_WRITE_DM_DISABLE
	result += ddr_hw_write_dm_disable(cfg, &ddr_temp);
#endif
	ddr_phy_cfg_update(cfg->cur_phy);

	return result;
}

static int ddr_hw_training_by_rank(struct ddr_cfg_st *cfg)
{
	ddr_debug("PHY[%x][%x] Rank[%x] itme[%x]",
		cfg->phy_idx, cfg->cur_phy, cfg->rank_idx, cfg->cur_item);

	/* 0:PHY_TRAINCTRL0_DTR_RANK0, 1:PHY_TRAINCTRL0_DTR_RANK1 */
	ddr_phy_switch_rank(cfg->cur_phy, cfg->rank_idx);

	return ddr_hw_training_ctl(cfg);
}

int ddr_hw_training_by_phy(struct ddr_cfg_st *cfg)
{
	int result = 0;
	unsigned int i;
	struct rdqs_data_st rdqs_data;
	struct rdqs_data_st *rdqs_st = &rdqs_data;
	struct ddr_timing_st timing_st;
	unsigned int rank_num = cfg->phy[cfg->phy_idx].rank_num;

	cfg->res_st = rdqs_st;

	/* disable auto refresh */
	ddr_training_save_timing(cfg, &timing_st);

	for (i = 0; i < rank_num; i++) {
		cfg->rank_idx = i;
		cfg->cur_item = cfg->phy[cfg->phy_idx].rank[i].item_hw;

		result += ddr_hw_training_by_rank(cfg);

		if (rank_num != DDR_SUPPORT_RANK_MAX)
			break;

		/* save rank rdqs bdl */
		ddr_training_get_rdqs(cfg, &(rdqs_st->rank[i]));

		/* restore PHY_PHYINITCTRL_DLYMEAS_EN rdqs before training next rank */
		if ((rank_num - 1) != i)
			ddr_training_set_rdqs(cfg, &(rdqs_st->origin));
	}

	if (rank_num == DDR_SUPPORT_RANK_MAX)
		ddr_hw_training_adjust_rdqs(cfg, rdqs_st);

	/* restore auto refresh */
	ddr_training_restore_timing(cfg, &timing_st);

	cfg->res_st = 0;

	return result;
}

/* DDR hardware training */
int ddr_hw_training(struct ddr_cfg_st *cfg)
{
	int result = 0;
	unsigned int i;
	struct tr_relate_reg reg;

	/* save customer reg */
	ddr_boot_cmd_save_func(&reg);
	for (i = 0; i < cfg->phy_num; i++) {
		cfg->phy_idx = i;
		cfg->cur_phy = cfg->phy[i].addr;
		result += ddr_hw_training_by_phy(cfg);
	}
	/* restore customer reg */
	ddr_boot_cmd_restore_func(&reg);

	return result;
}
#endif /* DDR_HW_TRAINING_CONFIG */

#define __DPMC_TRAINING__
#ifdef DDR_DPMC_TRAINING_CONFIG
/* Compare dpmc training result. */
static void ddr_dpmc_compare_result(unsigned int base_phy, unsigned int win_def, unsigned int win_max,
	int byte_index, int ctrl3_index, struct dpmc_data_st *dpmc)
{
	ddr_debug("win_def:[%x]  win_max:[%x]", win_def, win_max);
	ddr_debug("ctrl3_index:[%x]", ctrl3_index);

	if (win_def > win_max) {
		reg_write(dpmc->ctrl0_def, base_phy + dx_dxnmiscctrl0(byte_index));
		reg_write(dpmc->ctrl3_def, base_phy + dx_dxnmiscctrl3(byte_index));
	} else {
		reg_write(dpmc->ctrl0_cfg, base_phy + dx_dxnmiscctrl0(byte_index));
		reg_write(dpmc->ctrl3_cfg_clear | (ctrl3_index << DX_DXNMISCCTRL3_BIT),
			base_phy + dx_dxnmiscctrl3(byte_index));
	}
}

static int ddr_dpmc_process(struct ddr_cfg_st *cfg, unsigned int byte_index)
{
	int result;
	int ctrl3_index = 0;
	unsigned int i;
	unsigned int win_def;
	unsigned int win_max = 0;
	struct training_data training;
	struct dpmc_data_st dpmc;

	ddr_info("PHY[%X] byte[%X] mode[%X]", cfg->cur_phy, byte_index, cfg->cur_mode);
	dpmc.ctrl0_def = reg_read(cfg->cur_phy + dx_dxnmiscctrl0(byte_index));
	dpmc.ctrl3_def = reg_read(cfg->cur_phy + dx_dxnmiscctrl3(byte_index));
	ddr_debug("ctrl0_def:[%x]  ctrl3_def:[%x]", dpmc.ctrl0_def, dpmc.ctrl3_def);

	result = ddr_dataeye_deskew(cfg, &training);
	if (result) {
		result = -1;
		ddr_error("PHY[%x] mode[%x] dataeye training fail", cfg->cur_phy, cfg->cur_mode);
	}
	win_def = training.ddr_win_sum;

	/* Set dxctl_rxp_2nd_dq(15:8) and dxctl_rxp_2nd_dm(20) to 1 */
	dpmc.ctrl0_cfg = (reg_read(cfg->cur_phy + dx_dxnmiscctrl0(byte_index)) &
		(~(DX_DXNMISCCTRL0_DQ_MASK << DX_DXNMISCCTRL0_DQ_BIT))) |
		(DX_DXNMISCCTRL0_DQ_MASK << DX_DXNMISCCTRL0_DQ_BIT);
	dpmc.ctrl0_cfg = (dpmc.ctrl0_cfg & (~(DX_DXNMISCCTRL0_DM_MASK << DX_DXNMISCCTRL0_DM_BIT))) |
		(DX_DXNMISCCTRL0_DM_MASK << DX_DXNMISCCTRL0_DM_BIT);

	ddr_debug("ctrl0_cfg:[%x]", dpmc.ctrl0_cfg);
	reg_write(dpmc.ctrl0_cfg, cfg->cur_phy + dx_dxnmiscctrl0(byte_index));

	/* Set dxctl_pre_margin_code to 0-7, 24:22 */
	for (i = 0; i < DX_DXNMISCCTRL3_MAX; i++) {
		dpmc.ctrl3_cfg_clear = reg_read(cfg->cur_phy + dx_dxnmiscctrl3(byte_index)) &
			(~(DX_DXNMISCCTRL3_MASK << DX_DXNMISCCTRL3_BIT));
		reg_write(dpmc.ctrl3_cfg_clear | (i << DX_DXNMISCCTRL3_BIT),
			cfg->cur_phy + dx_dxnmiscctrl3(byte_index));
		ddr_debug("ctrl3_cfg:[%x]", reg_read(cfg->cur_phy + dx_dxnmiscctrl3(byte_index)));

		ddr_dataeye_deskew(cfg, &training);

		ddr_info("win_sum[%x]:%x", i, training.ddr_win_sum);
		if (training.ddr_win_sum > win_max) {
			win_max = training.ddr_win_sum;
			ctrl3_index = i;
		}
	}
	ddr_dpmc_compare_result(cfg->cur_phy, win_def, win_max, byte_index, ctrl3_index, &dpmc);

	return result;
}

/* dxctl pre margin code training. */
int ddr_dpmc_training(struct ddr_cfg_st *cfg)
{
	int result = 0;
	unsigned int byte_index;
	cfg->dq_check_type = DDR_CHECK_TYPE_DDRT;
	cfg->cur_mode = DDR_MODE_READ;
	struct tr_dq_data dq_data;

	ddr_save_dq_bdl(cfg, &dq_data);
	for (byte_index = 0; byte_index < get_byte_num(cfg); byte_index++) {
		if (byte_index == 0 || byte_index == 2) /* only training byte0 and byte2 */
			result = ddr_dpmc_process(cfg, byte_index);
		else
			continue;
	}
	ddr_restore_dq_bdl(cfg, &dq_data);

	return result;
}

int ddr_dpmc_training_func(struct ddr_cfg_st *cfg)
{
	struct tr_relate_reg relate_reg;
	int result = 0;

	/* dataeye training disable */
	if (ddr_training_check_bypass(cfg, DDR_BYPASS_DPMC_MASK))
		return 0;

	ddr_training_save_reg(cfg, &relate_reg, DDR_BYPASS_DPMC_MASK);
	ddr_training_switch_axi(cfg);
	ddr_ddrt_init(cfg, DDR_DDRT_MODE_DATAEYE);
	result += ddr_dpmc_training(cfg);
	ddr_training_restore_reg(cfg, &relate_reg);

	return result;
}
#else
int ddr_dpmc_training_func(struct ddr_cfg_st *cfg)
{
	ddr_warning("Not support DDR dpmc training");
	return 0;
}
#endif /* DDR_DPMC_TRAINING_CONFIG */

#define __VREF_TRAINING__
#ifdef DDR_VREF_TRAINING_CONFIG
#ifdef DDR_VREF_WITHOUT_BDL_CONFIG
/* Save dataeye dq bdl before vref training */
static void ddr_vref_save_bdl(const struct ddr_cfg_st *cfg, struct tr_dq_data *dq_data)
{
	int i;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int rank = cfg->rank_idx;
	unsigned int byte_index;

	for (i = 0; i < get_byte_num(cfg); i++) {
		byte_index = i + (cfg->dmc_idx << 1); /* byte index accord to phy */
		if (cfg->cur_mode == DDR_MODE_WRITE) {
			dq_data->dq03[i] = reg_read(base_phy + ddr_phy_dxnwdqnbdl0(rank, byte_index));
			dq_data->dq47[i] = reg_read(base_phy + ddr_phy_dxnwdqnbdl1(rank, byte_index));
			dq_data->wdm[i] = reg_read(base_phy + ddr_phy_dxnwdqnbdl2(rank, byte_index));
		} else {
			dq_data->dq03[i] = reg_read(base_phy + ddr_phy_dxnrdqnbdl0(rank, byte_index));
			dq_data->dq47[i] = reg_read(base_phy + ddr_phy_dxnrdqnbdl1(rank, byte_index));
		}
	}
}

/* Restore dataeye dq bdl after vref training */
static void ddr_vref_restore_bdl(const struct ddr_cfg_st *cfg, const struct tr_dq_data *dq_data)
{
	int i;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int rank = cfg->rank_idx;
	unsigned int byte_index;

	for (i = 0; i < get_byte_num(cfg); i++) {
		byte_index = i + (cfg->dmc_idx << 1); /* byte index accord to phy */
		if (cfg->cur_mode == DDR_MODE_WRITE) {
			reg_write(dq_data->dq03[i], base_phy + ddr_phy_dxnwdqnbdl0(rank, byte_index));
			reg_write(dq_data->dq47[i], base_phy + ddr_phy_dxnwdqnbdl1(rank, byte_index));
			reg_write(dq_data->wdm[i], base_phy + ddr_phy_dxnwdqnbdl2(rank, byte_index));
		} else {
			reg_write(dq_data->dq03[i], base_phy + ddr_phy_dxnrdqnbdl0(rank, byte_index));
			reg_write(dq_data->dq47[i], base_phy + ddr_phy_dxnrdqnbdl1(rank, byte_index));
		}
	}
}
#else
static void ddr_vref_save_bdl(const struct ddr_cfg_st *cfg, struct tr_dq_data *dq_data)
{
}
static void ddr_vref_restore_bdl(const struct ddr_cfg_st *cfg, const struct tr_dq_data *dq_data)
{
}
#endif /* DDR_VREF_WITHOUT_BDL_CONFIG */

/* phy s40 not support DRAM vref */
static int ddr_vref_dram_set_process(unsigned int base_phy, unsigned int val, unsigned int byte_index)
{
	unsigned int count;
	unsigned int dvrftctrl = reg_read(base_phy + DDR_PHY_DVRFTCTRL);
	unsigned int dvreft = reg_read(base_phy + ddr_phy_dvreft_status(byte_index)) &
		(~PHY_VRFTRES_DVREF_MASK);

	reg_write(dvrftctrl | PHY_DVRFTCTRL_PDAEN_EN, base_phy + DDR_PHY_DVRFTCTRL);
	reg_write(dvreft | val, base_phy + ddr_phy_dvreft_status(byte_index));
	reg_write(PHY_PHYINITCTRL_DVREFT_SYNC | PHY_PHYINITCTRL_INIT_EN,
		base_phy + DDR_PHY_PHYINITCTRL);

	count = DDR_HWR_WAIT_TIMEOUT;
	/* auto cleared to 0 after training finished */
	while (count--) {
		if (!(reg_read(base_phy + DDR_PHY_PHYINITCTRL) & PHY_PHYINITCTRL_INIT_EN))
			break;
	}

	if (count == 0xffffffff) {
		ddr_fatal("vref dram set wait timeout");
		ddr_training_stat(DDR_ERR_HW_RD_DATAEYE, base_phy, byte_index,
			reg_read(base_phy + DDR_PHY_PHYINITSTATUS));
		return -1;
	}

	reg_write(dvrftctrl & (~PHY_DVRFTCTRL_PDAEN_EN), base_phy + DDR_PHY_DVRFTCTRL);

	return 0;
}

#if defined(DDR_PHY_T12_V100_CONFIG) || defined(DDR_PHY_T12_V101_CONFIG)
static void ddr_phy_vref_host_set_process(unsigned int base_phy, unsigned int rank,
	unsigned int bytenum, unsigned int byte_index, unsigned int val)
{
	unsigned int hvreft;

	if (rank == 0) {
		hvreft = reg_read(base_phy + ddr_phy_hvreft_status(rank, byte_index)) &
			(~PHY_VRFTRES_HVREF_MASK);
		reg_write(hvreft | val, base_phy + ddr_phy_hvreft_status(rank, byte_index));
		reg_write(hvreft | val, base_phy + ddr_phy_hvreft_status(rank, byte_index + 1));
	} else {
		hvreft = reg_read(base_phy + ddr_phy_hvreft_status(rank, byte_index)) &
			(~(PHY_VRFTRES_RXDIFFCAL_MASK << PHY_VRFTRES_RXDIFFCAL_BIT));
		reg_write(hvreft | (val << PHY_VRFTRES_RXDIFFCAL_BIT),
			base_phy + ddr_phy_hvreft_status(rank, byte_index));
		reg_write(hvreft | (val << PHY_VRFTRES_RXDIFFCAL_BIT),
			base_phy + ddr_phy_hvreft_status(rank, byte_index + 1));
	}
}
#endif

/* Set DDR Vref value */
static void ddr_vref_set(const struct ddr_cfg_st *cfg, unsigned int val)
{
	if (cfg->cur_mode == DDR_MODE_READ) { /* HOST vref */
		ddr_phy_vref_host_set(cfg->cur_phy, cfg->rank_idx, get_byte_num(cfg), cfg->cur_byte, val);
	} else { /* DRAM vref */
		unsigned int auto_ref_timing = reg_read(cfg->cur_dmc + DDR_DMC_TIMING2);
		/* disable auto refresh */
		ddr_training_set_timing(cfg->cur_dmc, auto_ref_timing & DMC_AUTO_TIMING_DIS);

		/* DDR_PHY_VREFTCTRL 31bit:1 do vref dram set twice */
		reg_write((reg_read(cfg->cur_phy + DDR_PHY_VREFTCTRL) &
			(~(PHY_VREFS_MRS_ENTER_MASK << PHY_VREFS_MRS_ENTER_BIT))) |
			(PHY_VREFS_MRS_ENTER_MASK << PHY_VREFS_MRS_ENTER_BIT),
			cfg->cur_phy + DDR_PHY_VREFTCTRL);
		/* DRAM vref operations */
		ddr_phy_vref_dram_set(cfg->cur_phy, val, cfg->cur_byte);
		ddr_phy_vref_dram_set(cfg->cur_phy, val, cfg->cur_byte);
		/* DDR_PHY_VREFTCTRL 31bit:0 do vref dram set once */
		reg_write(reg_read(cfg->cur_phy + DDR_PHY_VREFTCTRL) &
			(~(PHY_VREFS_MRS_ENTER_MASK << PHY_VREFS_MRS_ENTER_BIT)),
			cfg->cur_phy + DDR_PHY_VREFTCTRL);
		/* DRAM vref operations */
		ddr_phy_vref_dram_set(cfg->cur_phy, val, cfg->cur_byte);
		/* enable auto refresh */
		ddr_training_set_timing(cfg->cur_dmc, auto_ref_timing);
	}
	ddr_info("byte[%x] mode[%x] set vref [%x]", cfg->cur_byte, cfg->cur_mode, val);
}

/* Get DDR Vref value */
static unsigned int ddr_vref_get(const struct ddr_cfg_st *cfg)
{
	unsigned int val = 0;

	if (cfg->cur_mode == DDR_MODE_READ) /* HOST vref */
		ddr_phy_vref_host_get(cfg->cur_phy, cfg->rank_idx, cfg->cur_byte, val);
	else /* DRAM vref */
		ddr_phy_vref_dram_get(cfg->cur_phy, val, cfg->cur_byte);

	ddr_info("byte[%x] mode[%x] get vref [%x]", cfg->cur_byte, cfg->cur_mode, val);

	return val;
}

/* Get totol win number of training result */
static unsigned int ddr_vref_get_win(struct ddr_cfg_st *cfg,
	struct training_data *training, int vref)
{
	unsigned int vref_min = 0;
	unsigned int vref_max = DDR_VREF_DRAM_VAL_MAX;
	int vref_set;

	training->ddr_win_sum = 0;

	if (cfg->cur_mode == DDR_MODE_READ)
		ddr_vref_get_host_max(cfg->rank_idx, vref_max);

	if (vref < vref_min)
		vref_set = vref_min;
	else if (vref > vref_max)
		vref_set = vref_max;
	else
		vref_set = vref;

	ddr_vref_set(cfg, vref_set);
	ddr_dataeye_deskew(cfg, training);

	return training->ddr_win_sum;
}

/* Find the best vref which win number is max */
static unsigned int ddr_vref_find_best(struct ddr_cfg_st *cfg,
	struct training_data *training, unsigned int vref, int step)
{
	int cur_vref;
	unsigned int best_vref;
	unsigned int cur_win;
	unsigned int max_win;
	unsigned int lower_times = 0;
	unsigned int vref_min = 0;
	unsigned int vref_max = DDR_VREF_DRAM_VAL_MAX;

	if (cfg->cur_mode == DDR_MODE_READ)
		ddr_vref_get_host_max(cfg->rank_idx, vref_max);

	max_win   = 0;
	cur_vref  = vref + step;

	if (vref < vref_min)
		best_vref = vref_min;
	else if (vref > vref_max)
		best_vref = vref_max;
	else
		best_vref = vref;

	/* find parabola vertex */
	while (cur_vref >= vref_min && cur_vref <= vref_max) {
		cur_win = ddr_vref_get_win(cfg, training, cur_vref);
		ddr_debug("byte[%x] vref[%x] win[%x] mode[%x]",
			cfg->cur_byte, cur_vref, cur_win, cfg->cur_mode);
		if (cur_win < max_win) {
			lower_times++;
			if (lower_times == DDR_VREF_COMPARE_TIMES)
				/* Continuous decline, mean found vertex */
				break;
		} else {
			lower_times = 0;
			max_win =  cur_win;
			best_vref = cur_vref;
		}
		cur_vref = cur_vref + step;
	}

	return best_vref;
}

/* DDR Vref calibrate and set the best value */
static void ddr_vref_cal(struct ddr_cfg_st *cfg, struct training_data *training)
{
	unsigned int def_vref;
	unsigned int best_vref;
	unsigned int left_win;
	unsigned int right_win;

	def_vref  = ddr_vref_get(cfg);
	left_win  = ddr_vref_get_win(cfg, training, def_vref - DDR_VREF_COMPARE_STEP);
	right_win = ddr_vref_get_win(cfg, training, def_vref + DDR_VREF_COMPARE_STEP);

	ddr_debug("byte[%x] default vref[%x] win[%x][%x] mode[%x]",
		cfg->cur_byte, def_vref, left_win, right_win, cfg->cur_mode);

	/* With vref increments, WIN number is a parabola.
	   So firstly determine the result on left or right. */
	/* parabola vertex */
	if (left_win < right_win) { /* the result on right */
		best_vref = ddr_vref_find_best(cfg, training, def_vref, 1);
	} else if (left_win > right_win) { /* the result on left */
		best_vref = ddr_vref_find_best(cfg, training, def_vref, -1);
	} else {
		/* when (left_win == right_win), check def_vref */
		unsigned int vref_max = DDR_VREF_DRAM_VAL_MAX;
		if (cfg->cur_mode == DDR_MODE_READ)
			ddr_vref_get_host_max(cfg->rank_idx, vref_max);

		if (def_vref < (vref_max >> 1))
			best_vref = ddr_vref_find_best(cfg, training, def_vref, 1);
		else
			best_vref = ddr_vref_find_best(cfg, training, def_vref, -1);
	}

	ddr_debug("byte[%x] best vref[%x] mode[%x]",
		cfg->cur_byte, best_vref, cfg->cur_mode);
	ddr_vref_set(cfg, best_vref);
}

/* vref write calibrate: support DDR4 and LPDDR4
 * if the dram type is not ddr4 or lpddr4, do nothing
 */
static int ddr_vref_write(struct ddr_cfg_st *cfg, struct training_data *training)
{
	unsigned int i;
	unsigned int dram_type = cfg->phy[cfg->phy_idx].dram_type;
	unsigned int bank_group = (reg_read(cfg->cur_dmc +
		ddr_dmc_cfg_rnkvol(cfg->rank_idx)) >> DMC_CFG_MEM_BG_BIT) & DMC_CFG_MEM_BG_MASK;

	if (dram_type != PHY_DRAMCFG_TYPE_LPDDR4 &&
		dram_type != PHY_DRAMCFG_TYPE_DDR4)
		return -1;

	if (dram_type == PHY_DRAMCFG_TYPE_LPDDR4)
		bank_group = DMC_CFG_MEM_2BG; /* lpddr4 not training byte1 byte3 */

	if (cfg->dmc_idx >= DDR_DMC_PER_PHY_MAX)
		return -1;

	if (get_byte_num(cfg) > DDR_PHY_BYTE_MAX)
		return -1;

	for (i = 0; i < get_byte_num(cfg); i++) {
		cfg->cur_byte = i + (cfg->dmc_idx << 1); /* byte index accord to phy */
		/* byte1 and byte3 bypass when 2 Bank Group */
		if ((bank_group == DMC_CFG_MEM_2BG) &&
			((i == 1) || (i == 3))) /* bypass byte1 and byte3 */
			continue;

		ddr_vref_cal(cfg, training);
	}

	return 0;
}

int ddr_vref_training(struct ddr_cfg_st *cfg)
{
	struct training_data tmp_result;
	struct training_data *training = &tmp_result;
	struct tr_dq_data dq_data;
	int result = 0;
	unsigned int i;

	ddr_debug("DDR Vref[%x] training PHY[%x][%x] DMC[%x][%x] Rank[%x]",
		cfg->cur_mode, cfg->phy_idx, cfg->cur_phy, cfg->dmc_idx, cfg->cur_dmc, cfg->rank_idx);

	ddr_vref_save_bdl(cfg, &dq_data);
	ddrtr_set_data(training, 0, sizeof(struct training_data));

	/* vref calibrate */
	if (cfg->cur_mode == DDR_MODE_READ) {
		for (i = 0; i < get_byte_num(cfg); i++) {
			cfg->cur_byte = i + (cfg->dmc_idx << 1); /* byte index accord to phy */
			if (cfg->cur_byte == 1 || cfg->cur_byte == 3) /* not training byte 1 and byte 3 */
				continue;

			ddr_vref_cal(cfg, training);
		}
	} else {
		if (ddr_vref_write(cfg, training))
			return 0; /* do nothing */
	}

#if !defined(DDR_VREF_WITHOUT_BDL_CONFIG) || defined(DDR_TRAINING_CMD)
	/* dataeye deskew again on best vref. */
	for (i = 0; i < get_byte_num(cfg); i++) {
		cfg->cur_byte = i + (cfg->dmc_idx << 1); /* byte index accord to phy */
		result += ddr_dataeye_deskew(cfg, training);
	}
#endif

	ddr_vref_restore_bdl(cfg, &dq_data);
	ddr_result_data_save(cfg, training);

	return result;
}

int ddr_vref_training_func(struct ddr_cfg_st *cfg)
{
	struct tr_relate_reg relate_reg;
	int result = 0;

	if (cfg == NULL) {
		ddr_error("Pointer parameter cfg is NULL!");
		return -1;
	}
	ddr_training_save_reg(cfg, &relate_reg, DDR_BYPASS_VREF_HOST_MASK);
	ddr_training_switch_axi(cfg);
	ddr_ddrt_init(cfg, DDR_DDRT_MODE_DATAEYE);
	cfg->dq_check_type = DDR_CHECK_TYPE_DDRT;

	/* host vref training disable */
	if (ddr_training_check_bypass(cfg, DDR_BYPASS_VREF_HOST_MASK)  == DDR_FALSE) {
		cfg->cur_mode = DDR_MODE_READ;
		result += ddr_vref_training(cfg);
	}

	/* dram vref training enable && DDR4 */
	if (ddr_training_check_bypass(cfg, DDR_BYPASS_VREF_DRAM_MASK) == DDR_FALSE) {
		cfg->cur_mode = DDR_MODE_WRITE;
		result += ddr_vref_training(cfg);
	}
	ddr_training_restore_reg(cfg, &relate_reg);

	return result;
}
#else
int ddr_vref_training_func(struct ddr_cfg_st *cfg)
{
	ddr_warning("Not support DDR vref training");

	return 0;
}
#endif /* DDR_VREF_TRAINING_CONFIG */

#define __GATE_TRAINING__
#ifdef DDR_GATE_TRAINING_CONFIG
/* Find gate phase */
static int ddr_gate_find_phase(const struct ddr_cfg_st *cfg, struct ddr_delay_st *rdqsg)
{
	int i;
	unsigned int base_phy = cfg->cur_phy;

	for (i = 0; i < get_byte_num(cfg); i++) {
		for (rdqsg->phase[i] = PHY_RDQSG_PHASE_MAX;
			rdqsg->phase[i] > PHY_GATE_PHASE_MARGIN;
			rdqsg->phase[i] -= PHY_RDQSG_PHASE_STEP) {
			reg_write(rdqsg->phase[i] << PHY_RDQSG_PHASE_BIT,
				base_phy + ddr_phy_dxnrdqsgdly(cfg->rank_idx, i));
			ddr_phy_cfg_update(base_phy);
			if (ddr_ddrt_test(DDRT_WR_COMPRARE_MODE, i, -1) == 0)
				break;
		}
		if (rdqsg->phase[i] <= PHY_GATE_PHASE_MARGIN) {
			/* find gate phase fail */
			ddr_fatal("find gate phase[%x] fail", rdqsg->phase[i]);
			ddr_training_stat(DDR_ERR_GATING, base_phy, -1, -1);
			return -1;
		} else {
			/* decrease one setp to find bdl */
			rdqsg->phase[i] -= PHY_RDQSG_PHASE_STEP;
			reg_write(rdqsg->phase[i] << PHY_RDQSG_PHASE_BIT,
				base_phy + ddr_phy_dxnrdqsgdly(cfg->rank_idx, i));
		}
	}
	ddr_phy_cfg_update(base_phy);

	return 0;
}

static void ddr_gate_find_bdl_by_byte(const struct ddr_cfg_st *cfg, struct ddr_delay_st *rdqsg,
	unsigned int byte_num, unsigned int gate_result)
{
	int j;
	unsigned int tmp;

	for (j = 0; j < byte_num; j++) {
		if (!(gate_result & (1 << j))) {
			rdqsg->bdl[j] += DDR_GATE_BDL_STEP;
			if (rdqsg->bdl[j] > PHY_BDL_MASK)
				tmp = ((rdqsg->bdl[j] - PHY_BDL_MASK - 1) << PHY_RDQSG_TX_BDL_BIT) +
					(rdqsg->phase[j] << PHY_RDQSG_PHASE_BIT) + (PHY_BDL_MASK - 1);
			else
				tmp = (rdqsg->phase[j] << PHY_RDQSG_PHASE_BIT) + rdqsg->bdl[j];

			reg_write(tmp, cfg->cur_phy + ddr_phy_dxnrdqsgdly(cfg->rank_idx, j));
		}
	}
}

/* Find gate bdl */
static int ddr_gate_find_bdl(const struct ddr_cfg_st *cfg, struct ddr_delay_st *rdqsg)
{
	int i, j;
	unsigned int gate_result;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int byte_num = get_byte_num(cfg);
	unsigned int swtmode = reg_read(base_phy + DDR_PHY_SWTMODE);

	for (i = 0; i < byte_num; i++)
		rdqsg->bdl[i] = 0;

	/* enable phy sw gate training mode */
	reg_write(swtmode | (1 << PHY_SWTMODE_SW_GTMODE_BIT), base_phy + DDR_PHY_SWTMODE);

	for (i = 0; i < PHY_GATE_BDL_MAX; i++) {
		ddr_phy_cfg_update(base_phy);
		ddr_ddrt_test(DDRT_READ_ONLY_MODE, -1, -1);
		gate_result = (reg_read(base_phy + DDR_PHY_SWTRLT) >> PHY_SWTRLT_GATE_BIT) &
			PHY_SWTRLT_GATE_MASK;
		if (gate_result == ((1 << byte_num) - 1))
			break;

		ddr_gate_find_bdl_by_byte(cfg, rdqsg, byte_num, gate_result);
	}

	/* disable phy sw gate training mode */
	reg_write(swtmode & (~(1 << PHY_SWTMODE_SW_GTMODE_BIT)),
		base_phy + DDR_PHY_SWTMODE);

	if (i == PHY_GATE_BDL_MAX) {  /* find gate bdl fail */
		ddr_fatal("PHY[%x] find gate bdl fail. result[%x]", base_phy, gate_result);
		for (j = 0; j < byte_num; j++) {
			if (!(gate_result & (1 << j)))
				ddr_training_stat(DDR_ERR_GATING, base_phy, j, -1);
		}
		return -1;
	} else {
		return 0;
	}
}

int ddr_gate_training(const struct ddr_cfg_st *cfg)
{
	unsigned int i, tmp;
	unsigned int byte_num;
	struct ddr_delay_st rdqsg;
	unsigned int def_delay[DDR_PHY_BYTE_MAX];
	int result;
	unsigned int base_phy = cfg->cur_phy;

	ddr_debug("DDR Gate training");

	byte_num = get_byte_num(cfg);

	for (i = 0; i < byte_num; i++)
		def_delay[i] = reg_read(base_phy + ddr_phy_dxnrdqsgdly(cfg->rank_idx, i));

	/* find phase first */
	result = ddr_gate_find_phase(cfg, &rdqsg);
	/* find bdl */
	if (!result)
		result = ddr_gate_find_bdl(cfg, &rdqsg);

	/* set new phase */
	if (!result) {
		for (i = 0; i < byte_num; i++) {
			rdqsg.phase[i] -= PHY_GATE_PHASE_MARGIN;
			tmp = reg_read(base_phy + ddr_phy_dxnrdqsgdly(cfg->rank_idx, i));
			tmp &= ~(PHY_RDQSG_PHASE_MASK << PHY_RDQSG_PHASE_BIT);
			tmp |= rdqsg.phase[i] << PHY_RDQSG_PHASE_BIT;
			reg_write(tmp, base_phy + ddr_phy_dxnrdqsgdly(cfg->rank_idx, i));
		}
	} else {
		/* restore default value */
		for (i = 0; i < byte_num; i++)
			reg_write(def_delay[i],
				base_phy + ddr_phy_dxnrdqsgdly(cfg->rank_idx, i));
	}
	ddr_phy_cfg_update(base_phy);

	return 0; /* use default value and not reset */
}

int ddr_gating_func(const struct ddr_cfg_st *cfg)
{
	struct tr_relate_reg relate_reg;
	int result = 0;

	/* gate training disable */
	if (ddr_training_check_bypass(cfg, DDR_BYPASS_GATE_MASK) != DDR_FALSE) {
		/* check hardware gating */
		if (reg_read(cfg->cur_phy + DDR_PHY_PHYINITSTATUS) &
			PHY_INITSTATUS_GT_MASK) {
			ddr_fatal("PHY[%x] hw gating fail", cfg->cur_phy);
			ddr_training_stat(DDR_ERR_HW_GATING, cfg->cur_phy, -1, -1);
			return -1;
		}
		return 0;
	}

	ddr_training_save_reg(cfg, &relate_reg, DDR_BYPASS_GATE_MASK);

	ddr_training_switch_axi(cfg);
	ddr_ddrt_init(cfg, DDR_DDRT_MODE_GATE);
	result += ddr_gate_training(cfg);

	ddr_training_restore_reg(cfg, &relate_reg);

	return result;
}
#else
int ddr_gating_func(const struct ddr_cfg_st *cfg)
{
	ddr_warning("Not support DDR gate training");

	return 0;
}
#endif /* DDR_GATE_TRAINING_CONFIG */

#define __PCODE_TRAINING__
#ifdef DDR_PCODE_TRAINING_CONFIG
/* Set pcode value to register IMPSTATUS and DDR_PHY_IMP_STATUS1 */
static void ddr_pcode_set_value(unsigned int base_phy, unsigned int pcode_value)
{
	unsigned int imp_ctrl1;

	reg_write((reg_read(base_phy + DDR_PHY_IMPSTATUS) &
		(~(PHY_ZCODE_PDRV_MASK << PHY_ZCODE_PDRV_BIT))) |
		(pcode_value << PHY_ZCODE_PDRV_BIT), base_phy + DDR_PHY_IMPSTATUS);
	ddr_debug("cur IMPSTATUS [%x] = [%x]",
		base_phy + DDR_PHY_IMPSTATUS, reg_read(base_phy + DDR_PHY_IMPSTATUS));

	imp_ctrl1 = reg_read(base_phy + DDR_PHY_IMP_CTRL1);
	/* ac_vddq_cal_en set 0 */
	reg_write(imp_ctrl1 & (~(0x1 << PHY_AC_VDDQ_CAL_EN_BIT)), base_phy + DDR_PHY_IMP_CTRL1);

	reg_write((reg_read(base_phy + DDR_PHY_IMP_STATUS1) &
		(~(PHY_ACCTL_PDRV_LATCH_MASK << PHY_ACCTL_PDRV_LATCH_BIT))) |
		(pcode_value << PHY_ACCTL_PDRV_LATCH_BIT), base_phy + DDR_PHY_IMP_STATUS1);
	ddr_debug("cur IMP_STATUS1 [%x] = [%x]",
		base_phy + DDR_PHY_IMP_STATUS1, reg_read(base_phy + DDR_PHY_IMP_STATUS1));

	/* restore ac_vddq_cal_en */
	reg_write(imp_ctrl1, base_phy + DDR_PHY_IMP_CTRL1);
}

static int ddr_pcode_trainig_by_phy(const struct ddr_cfg_st *cfg)
{
	unsigned int times = 0;
	unsigned int base_phy = cfg->cur_phy;
	unsigned int pcode_value;
	unsigned int osc_rpt_vld;
	unsigned int osc_cnt_rdata;
	int ddr_freq;

	/* test start */
	reg_write(reg_read(base_phy + DDR_PHY_CORNER_DETECTOR) | PHY_OSC_START_MASK,
		base_phy + DDR_PHY_CORNER_DETECTOR);

	do {
		osc_rpt_vld = (reg_read(base_phy + DDR_PHY_CORNER_DETECTOR) >>
			PHY_OSC_RPT_VLD) & PHY_OSC_RPT_VLD_MASK;
		times++;
	} while ((!osc_rpt_vld) && (times < DDRT_PCODE_WAIT_TIMEOUT));

	if (times >= DDRT_PCODE_WAIT_TIMEOUT) {
		ddr_fatal("IO pcode training wait timeout");
		return -1;
	}

	osc_cnt_rdata = (reg_read(base_phy + DDR_PHY_CORNER_DETECTOR) >>
		PHY_OSC_CNT_RDATA_BIT) & PHY_OSC_CNT_RDATA_MASK;

	/* test stop */
	reg_write(reg_read(base_phy + DDR_PHY_CORNER_DETECTOR) &
		(~PHY_OSC_START_MASK), base_phy + DDR_PHY_CORNER_DETECTOR);

	ddr_freq = ddr_get_cksel();

	/* get pcode_value: a formula based on simulation */
	pcode_value =
		(490960 - (89 * osc_cnt_rdata * ddr_freq) / 300) / 10000; /* y equal (490960 - (89*x*fre)/300)/10000 */

	ddr_debug("pcode value[%x]", pcode_value);
	if (pcode_value < PHY_PCODE_MIN) {
		pcode_value = PHY_PCODE_MIN;
	} else if (pcode_value > PHY_PCODE_MAX) {
		pcode_value = PHY_PCODE_MAX;
	}

	/* set pcode value */
	ddr_pcode_set_value(base_phy, pcode_value);
	return 0;
}

int ddr_pcode_training(struct ddr_cfg_st *cfg)
{
	struct tr_relate_reg relate_reg;
	int result = 0;
	int i;

	if (cfg == NULL)
		return -1;

	for (i = 0; i < cfg->phy_num; i++) {
		cfg->phy_idx = i;
		cfg->cur_phy = cfg->phy[i].addr;
		cfg->cur_item = cfg->phy[i].rank[0].item;

		if (ddr_training_check_bypass(cfg, 1 << (cfg->phy_idx)) != DDR_FALSE)
			continue;

		/* pcode training disable */
		if (ddr_training_check_bypass(cfg, DDR_BYPASS_PCODE_MASK) != DDR_FALSE)
			continue;

		ddr_training_save_reg(cfg, &relate_reg, DDR_BYPASS_PCODE_MASK);
		result += ddr_pcode_trainig_by_phy(cfg);
		ddr_training_restore_reg(cfg, &relate_reg);
	}

	return result;
}
#else
int ddr_pcode_training(struct ddr_cfg_st *cfg)
{
	ddr_warning("Not support DDR pcode training");
	return 0;
}
#endif
