/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <command.h>
#include "ddr_interface.h"
#include "ddr_training_impl.h"

#ifdef DDR_SW_TRAINING_FUNC_PUBLIC
#ifdef DDR_TRAINING_CUT_CODE_CONFIG
/* lpca */
static int ddr_lpca_for_sw(struct ddr_cfg_st *cfg, unsigned int auto_ref_timing)
{
	int result = 0;

	if ((ddr_training_check_bypass(cfg, DDR_BYPASS_LPCA_MASK) == DDR_FALSE) &&
		((reg_read(cfg->cur_phy + DDR_PHY_DRAMCFG) & PHY_DRAMCFG_TYPE_LPDDR3) ==
		PHY_DRAMCFG_TYPE_LPDDR3)) {
		/* disable auto refresh */
		ddr_training_set_timing(cfg->cur_dmc, auto_ref_timing & DMC_AUTO_TIMING_DIS);

		result += ddr_lpca_training(cfg);

		/* enable auto refresh */
		ddr_training_set_timing(cfg->cur_dmc, auto_ref_timing);
	}

	return result;
}

/* write leveling */
static int ddr_wl_for_sw(struct ddr_cfg_st *cfg, unsigned int auto_ref_timing)
{
	int result = 0;

	if (ddr_training_check_bypass(cfg, DDR_BYPASS_WL_MASK) == DDR_FALSE) {
		/* disable auto refresh */
		ddr_training_set_timing(cfg->cur_dmc, auto_ref_timing & DMC_AUTO_TIMING_DIS);
		result += ddr_write_leveling(cfg);
		/* enable auto refresh */
		ddr_training_set_timing(cfg->cur_dmc, auto_ref_timing);
	}

	return result;
}

/* dataeye */
static int ddr_dataeye_for_sw(struct ddr_cfg_st *cfg)
{
	int result = 0;

	if (ddr_training_check_bypass(cfg, DDR_BYPASS_DATAEYE_MASK) == DDR_FALSE) {
		ddr_training_switch_axi(cfg);
		ddr_ddrt_init(cfg, DDR_DDRT_MODE_DATAEYE);
		result += ddr_dataeye_training(cfg);
	}

	return result;
}

/* hardware read */
static int ddr_hw_read_for_sw(struct ddr_cfg_st *cfg, unsigned int acphyctl, int result)
{
	unsigned int dramcfg_ma2t;

	dramcfg_ma2t = reg_read(cfg->cur_phy + DDR_PHY_DRAMCFG) & PHY_DRAMCFG_MA2T;

	if (result && (ddr_training_check_bypass(cfg, DDR_BYPASS_HW_MASK) == DDR_FALSE)) {
		if (!dramcfg_ma2t) /* set 1T */
			reg_write(0x0, cfg->cur_phy + DDR_PHY_ACPHYCTL4);

		result = ddr_hw_dataeye_read(cfg);
		if (!dramcfg_ma2t) /* restore */
			reg_write(acphyctl, cfg->cur_phy + DDR_PHY_ACPHYCTL4);

		result += ddr_dataeye_training(cfg);
	}

	return result;
}

/* mpr */
#ifdef DDR_MPR_TRAINING_CONFIG
static int ddr_mpr_for_sw(struct ddr_cfg_st *cfg)
{
	int result = 0;

	if (result && (ddr_training_check_bypass(cfg, DDR_BYPASS_MPR_MASK) == DDR_FALSE)) {
		result = ddr_mpr_training(cfg);
		result += ddr_dataeye_training(cfg);
	}

	return result;
}
#endif

/* gate */
static int ddr_gate_for_sw(struct ddr_cfg_st *cfg,
	unsigned int acphyctl, unsigned int auto_ref_timing)
{
	int result = 0;
	unsigned int dramcfg_ma2t;

	dramcfg_ma2t = reg_read(cfg->cur_phy + DDR_PHY_DRAMCFG) & PHY_DRAMCFG_MA2T;

	if (ddr_training_check_bypass(cfg, DDR_BYPASS_GATE_MASK) == DDR_FALSE) {
		ddr_training_switch_axi(cfg);
		ddr_ddrt_init(cfg, DDR_DDRT_MODE_GATE);
		/* disable auto refresh */
		ddr_training_set_timing(cfg->cur_dmc,
			auto_ref_timing & DMC_AUTO_TIMING_DIS);

		if (!dramcfg_ma2t) /* set 1T */
			reg_write(0x0, cfg->cur_phy + DDR_PHY_ACPHYCTL4);

		result += ddr_gate_training(cfg);

		/* enable auto refresh */
		ddr_training_set_timing(cfg->cur_dmc, auto_ref_timing);

		if (!dramcfg_ma2t) /* restore */
			reg_write(acphyctl, cfg->cur_phy + DDR_PHY_ACPHYCTL4);
	}

	return result;
}

/* vref */
static int ddr_vref_for_sw(struct ddr_cfg_st *cfg)
{
	int result = 0;

	if (ddr_training_check_bypass(cfg, DDR_BYPASS_VREF_HOST_MASK) == DDR_FALSE) {
		ddr_training_switch_axi(cfg);
		ddr_ddrt_init(cfg, DDR_DDRT_MODE_DATAEYE);
		result += ddr_vref_training(cfg);
	}

	return result;
}

static int ddr_training_boot_func(struct ddr_cfg_st *cfg)
{
	int result = 0;
	unsigned int auto_ref_timing;
	unsigned int acphyctl;

	auto_ref_timing = reg_read(cfg->cur_dmc + DDR_DMC_TIMING2);
	/* Static register have to read two times to get the right value. */
	acphyctl = reg_read(cfg->cur_phy + DDR_PHY_ACPHYCTL4);
	acphyctl = reg_read(cfg->cur_phy + DDR_PHY_ACPHYCTL4);

	/* check hardware gating */
	if (reg_read(cfg->cur_phy + DDR_PHY_PHYINITSTATUS) & PHY_INITSTATUS_GT_MASK) {
		ddr_fatal("PHY[%x] hw gating fail", cfg->cur_phy);
		ddr_training_stat(DDR_ERR_HW_GATING, cfg->cur_phy, -1, -1);
	}

#ifdef DDR_LPCA_TRAINING_CONFIG
	/* lpca */
	result += ddr_lpca_for_sw(cfg, auto_ref_timing);
#endif

#ifdef DDR_WL_TRAINING_CONFIG
	/* write leveling */
	result += ddr_wl_for_sw(cfg, auto_ref_timing);
#endif

#ifdef DDR_DATAEYE_TRAINING_CONFIG
	/* dataeye */
	result += ddr_dataeye_for_sw(cfg);
#endif

#ifdef DDR_HW_TRAINING_CONFIG
	/* hardware read */
	result += ddr_hw_read_for_sw(cfg, acphyctl, result);
#endif

#ifdef DDR_MPR_TRAINING_CONFIG
	/* mpr */
	result += ddr_mpr_for_sw(cfg);
#endif

#ifdef DDR_GATE_TRAINING_CONFIG
	/* gate */
	result += ddr_gate_for_sw(cfg, acphyctl, auto_ref_timing);
#endif

#ifdef DDR_VREF_TRAINING_CONFIG
	result += ddr_vref_for_sw(cfg);
#endif

	return result;
}

/*
 * Cut ddr training control code for less SRAM.
 * Support DDRC500.
 * Support DDRC510 with one PHY.
 */
int ddr_sw_training_func(void)
{
	int result;
	unsigned int misc_scramb;

	ddr_variable_declare(swapdfibyte_en);
	struct ddr_cfg_st ddr_cfg;
	struct ddr_cfg_st *cfg = &ddr_cfg;

	/* check sw ddr training enable */
	if (DDR_BYPASS_ALL_MASK == reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG))
		return 0;

	ddr_training_start();
	ddr_training_cfg_init(cfg);

	cfg->phy_idx = 0;
	cfg->cur_phy = cfg->phy[0].addr;
	cfg->dmc_idx = 0;
	cfg->cur_dmc = cfg->phy[0].dmc[0].addr;

	misc_scramb = reg_read(cfg->cur_phy + DDR_PHY_MISC);

#ifdef DDR_TRAINING_STAT_CONFIG
	/* clear stat register */
	reg_write(0x0, DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_STAT);
#endif

	/* disable scramb */
	reg_write(misc_scramb & PHY_MISC_SCRAMB_DIS, cfg->cur_phy + DDR_PHY_MISC);

	/* disable rdqs swap */
	ddr_dqsswap_save_func(swapdfibyte_en, cfg->cur_phy);

	result = ddr_training_boot_func(cfg);

	/* restore scramb */
	reg_write(misc_scramb, cfg->cur_phy + DDR_PHY_MISC);

	/* restore rdqs swap */
	ddr_dqsswap_restore_func(swapdfibyte_en, cfg->cur_phy);

	if (!result)
		ddr_training_suc();

	return result;
}
#else
int ddr_training_boot_func(struct ddr_cfg_st *cfg)
{
	int result;

	/* check hardware gating */
	if (reg_read(cfg->cur_phy + DDR_PHY_PHYINITSTATUS) &
		PHY_INITSTATUS_GT_MASK) {
		ddr_fatal("PHY[%x] hw gating fail", cfg->cur_phy);
		ddr_training_stat(DDR_ERR_HW_GATING, cfg->cur_phy, -1, -1);
	}

	/* lpca */
	result = ddr_lpca_training_func(cfg);
	/* write leveling */
	result += ddr_wl_func(cfg);
	/* dataeye/gate/vref need switch axi */
	/* dataeye */
	result += ddr_dataeye_training_func(cfg);
#ifdef DDR_HW_TRAINING_CONFIG
	/* hardware read */
	if (result && (ddr_training_check_bypass(cfg, DDR_BYPASS_HW_MASK) == DDR_FALSE)) {
		struct tr_relate_reg relate_reg_ac;
		ddr_training_save_reg(cfg, &relate_reg_ac, DDR_BYPASS_HW_MASK);
		result = ddr_hw_dataeye_read(cfg);
		ddr_training_restore_reg(cfg, &relate_reg_ac);
		cfg->adjust = DDR_DATAEYE_ABNORMAL_ADJUST;
		result += ddr_dataeye_training(cfg);
	}
#endif
	/* mpr */
	result += ddr_mpr_training_func(cfg);
	/* gate */
	result += ddr_gating_func(cfg);
	/* vref */
	result += ddr_vref_training_func(cfg);

	return result;
}

/* Support DDRC510 with two PHY */
int ddr_sw_training_func(void)
{
	struct ddr_cfg_st ddr_cfg;
	struct ddr_cfg_st *cfg = &ddr_cfg;
	struct tr_relate_reg reg;
	int result;

#ifdef SYSCTRL_DDR_TRAINING_VERSION_FLAG
	/* DDR training version flag */
	unsigned int tmp_reg = reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_VERSION_FLAG);
	tmp_reg = (tmp_reg & 0xffff0000) | DDR_VERSION;
	reg_write(tmp_reg, DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_VERSION_FLAG);
#endif

	/* check sw ddr training enable */
	if (DDR_BYPASS_ALL_MASK == reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG)
#ifdef SYSCTRL_DDR_TRAINING_CFG_SEC
	&& DDR_BYPASS_ALL_MASK == reg_read(DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_CFG_SEC)
#endif
	)
		return 0;

	ddr_training_start();

	/* save customer reg */
	ddr_boot_cmd_save_func(&reg);

#ifdef DDR_TRAINING_STAT_CONFIG
	/* clear stat register */
	reg_write(0x0, DDR_REG_BASE_SYSCTRL + SYSCTRL_DDR_TRAINING_STAT);
#endif

	ddr_training_cfg_init(cfg);
	cfg->cmd_st = 0;

	result = ddr_training_all(cfg);
	result += ddr_dcc_training_func(cfg);
	if (!result)
		ddr_training_suc();
	else
		ddr_training_console_if();

	/* restore customer reg */
	ddr_boot_cmd_restore_func(&reg);

	return result;
}
#endif /* DDR_TRAINING_CUT_CODE_CONFIG */
#endif /* DDR_SW_TRAINING_FUNC_PUBLIC */

#ifdef DDR_PCODE_TRAINING_CONFIG
int ddr_pcode_training_func(void)
{
	struct ddr_cfg_st ddr_cfg;
	struct ddr_cfg_st *cfg = &ddr_cfg;

	ddr_training_cfg_init(cfg);
	return ddr_pcode_training(cfg);
}
#else
int ddr_pcode_training_func(void)
{
	ddr_warning("Not support DDR pcode training");
	return 0;
}
#endif

#ifdef DDR_HW_TRAINING_CONFIG
int ddr_hw_training_func(void)
{
	struct ddr_cfg_st ddr_cfg;
	struct ddr_cfg_st *cfg = &ddr_cfg;

	ddr_training_cfg_init(cfg);

	return ddr_hw_training(cfg);
}
#else
int ddr_hw_training_func(void)
{
	ddr_warning("Not support DDR HW training");
	return 0;
}
#endif /* DDR_HW_TRAINING_CONFIG */

int ddr_sw_training_if(void)
{
	return DDR_SW_TRAINING_FUNC();
}

int ddr_hw_training_if(void)
{
	return DDR_HW_TRAINING_FUNC();
}

int ddr_pcode_training_if(void)
{
	return DDR_PCODE_TRAINING_FUNC();
}

