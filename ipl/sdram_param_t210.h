/*
 * Copyright (c) 2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

/**
 * Defines the SDRAM parameter structure.
 *
 * Note that PLLM is used by EMC.
 */

#ifndef _SDRAM_PARAM_T210_H_
#define _SDRAM_PARAM_T210_H_

#define MEMORY_TYPE_NONE 0
#define MEMORY_TYPE_DDR 0
#define MEMORY_TYPE_LPDDR 0
#define MEMORY_TYPE_DDR2 0
#define MEMORY_TYPE_LPDDR2 1
#define MEMORY_TYPE_DDR3 2
#define MEMORY_TYPE_LPDDR4 3

/**
 * Defines the SDRAM parameter structure
 */
typedef struct _sdram_params {
	/* Specifies the type of memory device */
	u32 memory_type;

	/* MC/EMC clock source configuration */

	/* Specifies the M value for PllM */
	u32 pllm_input_divider;
	/* Specifies the N value for PllM */
	u32 pllm_feedback_divider;
	/* Specifies the time to wait for PLLM to lock (in microseconds) */
	u32 pllm_stable_time;
	/* Specifies misc. control bits */
	u32 pllm_setup_control;
	/* Specifies the P value for PLLM */
	u32 pllm_post_divider;
	/* Specifies value for Charge Pump Gain Control */
	u32 pllm_kcp;
	/* Specifies VCO gain */
	u32 pllm_kvco;
	/* Spare BCT param */
	u32 emc_bct_spare0;
	/* Spare BCT param */
	u32 emc_bct_spare1;
	/* Spare BCT param */
	u32 emc_bct_spare2;
	/* Spare BCT param */
	u32 emc_bct_spare3;
	/* Spare BCT param */
	u32 emc_bct_spare4;
	/* Spare BCT param */
	u32 emc_bct_spare5;
	/* Spare BCT param */
	u32 emc_bct_spare6;
	/* Spare BCT param */
	u32 emc_bct_spare7;
	/* Spare BCT param */
	u32 emc_bct_spare8;
	/* Spare BCT param */
	u32 emc_bct_spare9;
	/* Spare BCT param */
	u32 emc_bct_spare10;
	/* Spare BCT param */
	u32 emc_bct_spare11;
	/* Spare BCT param */
	u32 emc_bct_spare12;
	/* Spare BCT param */
	u32 emc_bct_spare13;

	/* Defines EMC_2X_CLK_SRC, EMC_2X_CLK_DIVISOR, EMC_INVERT_DCD */
	u32 emc_clock_source;
	u32 emc_clock_source_dll;

	/* Defines possible override for PLLLM_MISC2 */
	u32 clk_rst_pllm_misc20_override;
	/* enables override for PLLLM_MISC2 */
	u32 clk_rst_pllm_misc20_override_enable;
	/* defines CLK_ENB_MC1 in register clk_rst_controller_clk_enb_w_clr */
	u32 clear_clock2_mc1;

	/* Auto-calibration of EMC pads */

	/* Specifies the value for EMC_AUTO_CAL_INTERVAL */
	u32 emc_auto_cal_interval;
	/*
	 * Specifies the value for EMC_AUTO_CAL_CONFIG
	 * Note: Trigger bits are set by the SDRAM code.
	 */
	u32 emc_auto_cal_config;

	/* Specifies the value for EMC_AUTO_CAL_CONFIG2 */
	u32 emc_auto_cal_config2;

	/* Specifies the value for EMC_AUTO_CAL_CONFIG3 */
	u32 emc_auto_cal_config3;

	u32 emc_auto_cal_config4;
	u32 emc_auto_cal_config5;
	u32 emc_auto_cal_config6;
	u32 emc_auto_cal_config7;
	u32 emc_auto_cal_config8;
	/* Specifies the value for EMC_AUTO_CAL_VREF_SEL_0 */
	u32 emc_auto_cal_vref_sel0;
	u32 emc_auto_cal_vref_sel1;

	/* Specifies the value for EMC_AUTO_CAL_CHANNEL */
	u32 emc_auto_cal_channel;

	/* Specifies the value for EMC_PMACRO_AUTOCAL_CFG_0 */
	u32 emc_pmacro_auto_cal_cfg0;
	u32 emc_pmacro_auto_cal_cfg1;
	u32 emc_pmacro_auto_cal_cfg2;

	u32 emc_pmacro_rx_term;
	u32 emc_pmacro_dq_tx_drive;
	u32 emc_pmacro_ca_tx_drive;
	u32 emc_pmacro_cmd_tx_drive;
	u32 emc_pmacro_auto_cal_common;
	u32 emc_pmacro_zcrtl;

	/*
	 * Specifies the time for the calibration
	 * to stabilize (in microseconds)
	 */
	u32 emc_auto_cal_wait;

	u32 emc_xm2_comp_pad_ctrl;
	u32 emc_xm2_comp_pad_ctrl2;
	u32 emc_xm2_comp_pad_ctrl3;

	/*
	 * DRAM size information
	 * Specifies the value for EMC_ADR_CFG
	 */
	u32 emc_adr_cfg;

	/*
	 * Specifies the time to wait after asserting pin
	 * CKE (in microseconds)
	 */
	u32 emc_pin_program_wait;
	/* Specifies the extra delay before/after pin RESET/CKE command */
	u32 emc_pin_extra_wait;

	u32 emc_pin_gpio_enable;
	u32 emc_pin_gpio;

	/*
	 * Specifies the extra delay after the first writing
	 * of EMC_TIMING_CONTROL
	 */
	u32 emc_timing_control_wait;

	/* Timing parameters required for the SDRAM */

	/* Specifies the value for EMC_RC */
	u32 emc_rc;
	/* Specifies the value for EMC_RFC */
	u32 emc_rfc;

	u32 emc_rfc_pb;
	u32 emc_ref_ctrl2;

	/* Specifies the value for EMC_RFC_SLR */
	u32 emc_rfc_slr;
	/* Specifies the value for EMC_RAS */
	u32 emc_ras;
	/* Specifies the value for EMC_RP */
	u32 emc_rp;
	/* Specifies the value for EMC_R2R */
	u32 emc_r2r;
	/* Specifies the value for EMC_W2W */
	u32 emc_w2w;
	/* Specifies the value for EMC_R2W */
	u32 emc_r2w;
	/* Specifies the value for EMC_W2R */
	u32 emc_w2r;
	/* Specifies the value for EMC_R2P */
	u32 emc_r2p;
	/* Specifies the value for EMC_W2P */
	u32 emc_w2p;
	/* Specifies the value for EMC_RD_RCD */

	u32 emc_tppd;
	u32 emc_ccdmw;

	u32 emc_rd_rcd;
	/* Specifies the value for EMC_WR_RCD */
	u32 emc_wr_rcd;
	/* Specifies the value for EMC_RRD */
	u32 emc_rrd;
	/* Specifies the value for EMC_REXT */
	u32 emc_rext;
	/* Specifies the value for EMC_WEXT */
	u32 emc_wext;
	/* Specifies the value for EMC_WDV */
	u32 emc_wdv;

	u32 emc_wdv_chk;
	u32 emc_wsv;
	u32 emc_wev;

	/* Specifies the value for EMC_WDV_MASK */
	u32 emc_wdv_mask;

	u32 emc_ws_duration;
	u32 emc_we_duration;

	/* Specifies the value for EMC_QUSE */
	u32 emc_quse;
	/* Specifies the value for EMC_QUSE_WIDTH */
	u32 emc_quse_width;
	/* Specifies the value for EMC_IBDLY */
	u32 emc_ibdly;

	u32 emc_obdly;

	/* Specifies the value for EMC_EINPUT */
	u32 emc_einput;
	/* Specifies the value for EMC_EINPUT_DURATION */
	u32 emc_einput_duration;
	/* Specifies the value for EMC_PUTERM_EXTRA */
	u32 emc_puterm_extra;
	/* Specifies the value for EMC_PUTERM_WIDTH */
	u32 emc_puterm_width;

	u32 emc_qrst;
	u32 emc_qsafe;
	u32 emc_rdv;
	u32 emc_rdv_mask;

	u32 emc_rdv_early;
	u32 emc_rdv_early_mask;

	/* Specifies the value for EMC_QPOP */
	u32 emc_qpop;

	/* Specifies the value for EMC_REFRESH */
	u32 emc_refresh;
	/* Specifies the value for EMC_BURST_REFRESH_NUM */
	u32 emc_burst_refresh_num;
	/* Specifies the value for EMC_PRE_REFRESH_REQ_CNT */
	u32 emc_prerefresh_req_cnt;
	/* Specifies the value for EMC_PDEX2WR */
	u32 emc_pdex2wr;
	/* Specifies the value for EMC_PDEX2RD */
	u32 emc_pdex2rd;
	/* Specifies the value for EMC_PCHG2PDEN */
	u32 emc_pchg2pden;
	/* Specifies the value for EMC_ACT2PDEN */
	u32 emc_act2pden;
	/* Specifies the value for EMC_AR2PDEN */
	u32 emc_ar2pden;
	/* Specifies the value for EMC_RW2PDEN */
	u32 emc_rw2pden;

	u32 emc_cke2pden;
	u32 emc_pdex2che;
	u32 emc_pdex2mrr;

	/* Specifies the value for EMC_TXSR */
	u32 emc_txsr;
	/* Specifies the value for EMC_TXSRDLL */
	u32 emc_txsr_dll;
	/* Specifies the value for EMC_TCKE */
	u32 emc_tcke;
	/* Specifies the value for EMC_TCKESR */
	u32 emc_tckesr;
	/* Specifies the value for EMC_TPD */
	u32 emc_tpd;
	/* Specifies the value for EMC_TFAW */
	u32 emc_tfaw;
	/* Specifies the value for EMC_TRPAB */
	u32 emc_trpab;
	/* Specifies the value for EMC_TCLKSTABLE */
	u32 emc_tclkstable;
	/* Specifies the value for EMC_TCLKSTOP */
	u32 emc_tclkstop;
	/* Specifies the value for EMC_TREFBW */
	u32 emc_trefbw;

	/* FBIO configuration values */

	/* Specifies the value for EMC_FBIO_CFG5 */
	u32 emc_fbio_cfg5;
	/* Specifies the value for EMC_FBIO_CFG7 */
	u32 emc_fbio_cfg7;
	u32 emc_fbio_cfg8;

	/* Command mapping for CMD brick 0 */
	u32 emc_cmd_mapping_cmd0_0;
	u32 emc_cmd_mapping_cmd0_1;
	u32 emc_cmd_mapping_cmd0_2;
	u32 emc_cmd_mapping_cmd1_0;
	u32 emc_cmd_mapping_cmd1_1;
	u32 emc_cmd_mapping_cmd1_2;
	u32 emc_cmd_mapping_cmd2_0;
	u32 emc_cmd_mapping_cmd2_1;
	u32 emc_cmd_mapping_cmd2_2;
	u32 emc_cmd_mapping_cmd3_0;
	u32 emc_cmd_mapping_cmd3_1;
	u32 emc_cmd_mapping_cmd3_2;
	u32 emc_cmd_mapping_byte;

	/* Specifies the value for EMC_FBIO_SPARE */
	u32 emc_fbio_spare;

	/* Specifies the value for EMC_CFG_RSV */
	u32 emc_cfg_rsv;

	/* MRS command values */

	/* Specifies the value for EMC_MRS */
	u32 emc_mrs;
	/* Specifies the MP0 command to initialize mode registers */
	u32 emc_emrs;
	/* Specifies the MP2 command to initialize mode registers */
	u32 emc_emrs2;
	/* Specifies the MP3 command to initialize mode registers */
	u32 emc_emrs3;
	/* Specifies the programming to LPDDR2 Mode Register 1 at cold boot */
	u32 emc_mrw1;
	/* Specifies the programming to LPDDR2 Mode Register 2 at cold boot */
	u32 emc_mrw2;
	/* Specifies the programming to LPDDR2 Mode Register 3 at cold boot */
	u32 emc_mrw3;
	/* Specifies the programming to LPDDR2 Mode Register 11 at cold boot */
	u32 emc_mrw4;

	/* Specifies the programming to LPDDR4 Mode Register 3 at cold boot */
	u32 emc_mrw6;
	/* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
	u32 emc_mrw8;
	/* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
	u32 emc_mrw9;
	/* Specifies the programming to LPDDR4 Mode Register 12 at cold boot */
	u32 emc_mrw10;
	/* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
	u32 emc_mrw12;
	/* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
	u32 emc_mrw13;
	/* Specifies the programming to LPDDR4 Mode Register 22 at cold boot */
	u32 emc_mrw14;

	/*
	 * Specifies the programming to extra LPDDR2 Mode Register
	 * at cold boot
	 */
	u32 emc_mrw_extra;
	/*
	 * Specifies the programming to extra LPDDR2 Mode Register
	 * at warm boot
	 */
	u32 emc_warm_boot_mrw_extra;
	/*
	 * Specify the enable of extra Mode Register programming at
	 * warm boot
	 */
	u32 emc_warm_boot_extramode_reg_write_enable;
	/*
	 * Specify the enable of extra Mode Register programming at
	 * cold boot
	 */
	u32 emc_extramode_reg_write_enable;

	/* Specifies the EMC_MRW reset command value */
	u32 emc_mrw_reset_command;
	/* Specifies the EMC Reset wait time (in microseconds) */
	u32 emc_mrw_reset_ninit_wait;
	/* Specifies the value for EMC_MRS_WAIT_CNT */
	u32 emc_mrs_wait_cnt;
	/* Specifies the value for EMC_MRS_WAIT_CNT2 */
	u32 emc_mrs_wait_cnt2;

	/* EMC miscellaneous configurations */

	/* Specifies the value for EMC_CFG */
	u32 emc_cfg;
	/* Specifies the value for EMC_CFG_2 */
	u32 emc_cfg2;
	/* Specifies the pipe bypass controls */
	u32 emc_cfg_pipe;

	u32 emc_cfg_pipe_clk;
	u32 emc_fdpd_ctrl_cmd_no_ramp;
	u32 emc_cfg_update;

	/* Specifies the value for EMC_DBG */
	u32 emc_dbg;

	u32 emc_dbg_write_mux;

	/* Specifies the value for EMC_CMDQ */
	u32 emc_cmd_q;
	/* Specifies the value for EMC_MC2EMCQ */
	u32 emc_mc2emc_q;
	/* Specifies the value for EMC_DYN_SELF_REF_CONTROL */
	u32 emc_dyn_self_ref_control;

	/* Specifies the value for MEM_INIT_DONE */
	u32 ahb_arbitration_xbar_ctrl_meminit_done;

	/* Specifies the value for EMC_CFG_DIG_DLL */
	u32 emc_cfg_dig_dll;
	u32 emc_cfg_dig_dll_1;

	/* Specifies the value for EMC_CFG_DIG_DLL_PERIOD */
	u32 emc_cfg_dig_dll_period;
	/* Specifies the value of *DEV_SELECTN of various EMC registers */
	u32 emc_dev_select;

	/* Specifies the value for EMC_SEL_DPD_CTRL */
	u32 emc_sel_dpd_ctrl;

	/* Pads trimmer delays */
	u32 emc_fdpd_ctrl_dq;
	u32 emc_fdpd_ctrl_cmd;
	u32 emc_pmacro_ib_vref_dq_0;
	u32 emc_pmacro_ib_vref_dq_1;
	u32 emc_pmacro_ib_vref_dqs_0;
	u32 emc_pmacro_ib_vref_dqs_1;
	u32 emc_pmacro_ib_rxrt;
	u32 emc_cfg_pipe1;
	u32 emc_cfg_pipe2;

	/* Specifies the value for EMC_PMACRO_QUSE_DDLL_RANK0_0 */
	u32 emc_pmacro_quse_ddll_rank0_0;
	u32 emc_pmacro_quse_ddll_rank0_1;
	u32 emc_pmacro_quse_ddll_rank0_2;
	u32 emc_pmacro_quse_ddll_rank0_3;
	u32 emc_pmacro_quse_ddll_rank0_4;
	u32 emc_pmacro_quse_ddll_rank0_5;
	u32 emc_pmacro_quse_ddll_rank1_0;
	u32 emc_pmacro_quse_ddll_rank1_1;
	u32 emc_pmacro_quse_ddll_rank1_2;
	u32 emc_pmacro_quse_ddll_rank1_3;
	u32 emc_pmacro_quse_ddll_rank1_4;
	u32 emc_pmacro_quse_ddll_rank1_5;

	u32 emc_pmacro_ob_ddll_long_dq_rank0_0;
	u32 emc_pmacro_ob_ddll_long_dq_rank0_1;
	u32 emc_pmacro_ob_ddll_long_dq_rank0_2;
	u32 emc_pmacro_ob_ddll_long_dq_rank0_3;
	u32 emc_pmacro_ob_ddll_long_dq_rank0_4;
	u32 emc_pmacro_ob_ddll_long_dq_rank0_5;
	u32 emc_pmacro_ob_ddll_long_dq_rank1_0;
	u32 emc_pmacro_ob_ddll_long_dq_rank1_1;
	u32 emc_pmacro_ob_ddll_long_dq_rank1_2;
	u32 emc_pmacro_ob_ddll_long_dq_rank1_3;
	u32 emc_pmacro_ob_ddll_long_dq_rank1_4;
	u32 emc_pmacro_ob_ddll_long_dq_rank1_5;

	u32 emc_pmacro_ob_ddll_long_dqs_rank0_0;
	u32 emc_pmacro_ob_ddll_long_dqs_rank0_1;
	u32 emc_pmacro_ob_ddll_long_dqs_rank0_2;
	u32 emc_pmacro_ob_ddll_long_dqs_rank0_3;
	u32 emc_pmacro_ob_ddll_long_dqs_rank0_4;
	u32 emc_pmacro_ob_ddll_long_dqs_rank0_5;
	u32 emc_pmacro_ob_ddll_long_dqs_rank1_0;
	u32 emc_pmacro_ob_ddll_long_dqs_rank1_1;
	u32 emc_pmacro_ob_ddll_long_dqs_rank1_2;
	u32 emc_pmacro_ob_ddll_long_dqs_rank1_3;
	u32 emc_pmacro_ob_ddll_long_dqs_rank1_4;
	u32 emc_pmacro_ob_ddll_long_dqs_rank1_5;

	u32 emc_pmacro_ib_ddll_long_dqs_rank0_0;
	u32 emc_pmacro_ib_ddll_long_dqs_rank0_1;
	u32 emc_pmacro_ib_ddll_long_dqs_rank0_2;
	u32 emc_pmacro_ib_ddll_long_dqs_rank0_3;
	u32 emc_pmacro_ib_ddll_long_dqs_rank1_0;
	u32 emc_pmacro_ib_ddll_long_dqs_rank1_1;
	u32 emc_pmacro_ib_ddll_long_dqs_rank1_2;
	u32 emc_pmacro_ib_ddll_long_dqs_rank1_3;

	u32 emc_pmacro_ddll_long_cmd_0;
	u32 emc_pmacro_ddll_long_cmd_1;
	u32 emc_pmacro_ddll_long_cmd_2;
	u32 emc_pmacro_ddll_long_cmd_3;
	u32 emc_pmacro_ddll_long_cmd_4;
	u32 emc_pmacro_ddll_short_cmd_0;
	u32 emc_pmacro_ddll_short_cmd_1;
	u32 emc_pmacro_ddll_short_cmd_2;

	/*
	 * Specifies the delay after asserting CKE pin during a WarmBoot0
	 * sequence (in microseconds)
	 */
	u32 warm_boot_wait;

	/* Specifies the value for EMC_ODT_WRITE */
	u32 emc_odt_write;

	/* Periodic ZQ calibration */

	/*
	 * Specifies the value for EMC_ZCAL_INTERVAL
	 * Value 0 disables ZQ calibration
	 */
	u32 emc_zcal_interval;
	/* Specifies the value for EMC_ZCAL_WAIT_CNT */
	u32 emc_zcal_wait_cnt;
	/* Specifies the value for EMC_ZCAL_MRW_CMD */
	u32 emc_zcal_mrw_cmd;

	/* DRAM initialization sequence flow control */

	/* Specifies the MRS command value for resetting DLL */
	u32 emc_mrs_reset_dll;
	/* Specifies the command for ZQ initialization of device 0 */
	u32 emc_zcal_init_dev0;
	/* Specifies the command for ZQ initialization of device 1 */
	u32 emc_zcal_init_dev1;
	/*
	 * Specifies the wait time after programming a ZQ initialization
	 * command (in microseconds)
	 */
	u32 emc_zcal_init_wait;
	/*
	 * Specifies the enable for ZQ calibration at cold boot [bit 0]
	 * and warm boot [bit 1]
	 */
	u32 emc_zcal_warm_cold_boot_enables;

	/*
	 * Specifies the MRW command to LPDDR2 for ZQ calibration
	 * on warmboot
	 */
	/* Is issued to both devices separately */
	u32 emc_mrw_lpddr2zcal_warm_boot;
	/*
	 * Specifies the ZQ command to DDR3 for ZQ calibration on warmboot
	 * Is issued to both devices separately
	 */
	u32 emc_zqcal_ddr3_warm_boot;

	u32 emc_zqcal_lpddr4_warm_boot;

	/*
	 * Specifies the wait time for ZQ calibration on warmboot
	 * (in microseconds)
	 */
	u32 emc_zcal_warm_boot_wait;
	/*
	 * Specifies the enable for DRAM Mode Register programming
	 * at warm boot
	 */
	u32 emc_mrs_warm_boot_enable;
	/*
	 * Specifies the wait time after sending an MRS DLL reset command
	 * in microseconds)
	 */
	u32 emc_mrs_reset_dll_wait;
	/* Specifies the extra MRS command to initialize mode registers */
	u32 emc_mrs_extra;
	/* Specifies the extra MRS command at warm boot */
	u32 emc_warm_boot_mrs_extra;
	/* Specifies the EMRS command to enable the DDR2 DLL */
	u32 emc_emrs_ddr2_dll_enable;
	/* Specifies the MRS command to reset the DDR2 DLL */
	u32 emc_mrs_ddr2_dll_reset;
	/* Specifies the EMRS command to set OCD calibration */
	u32 emc_emrs_ddr2_ocd_calib;
	/*
	 * Specifies the wait between initializing DDR and setting OCD
	 * calibration (in microseconds)
	 */
	u32 emc_ddr2_wait;
	/* Specifies the value for EMC_CLKEN_OVERRIDE */
	u32 emc_clken_override;
	/*
	 * Specifies LOG2 of the extra refresh numbers after booting
	 * Program 0 to disable
	 */
	u32 emc_extra_refresh_num;
	/* Specifies the master override for all EMC clocks */
	u32 emc_clken_override_allwarm_boot;
	/* Specifies the master override for all MC clocks */
	u32 mc_clken_override_allwarm_boot;
	/* Specifies digital dll period, choosing between 4 to 64 ms */
	u32 emc_cfg_dig_dll_period_warm_boot;

	/* Pad controls */

	/* Specifies the value for PMC_VDDP_SEL */
	u32 pmc_vddp_sel;
	/* Specifies the wait time after programming PMC_VDDP_SEL */
	u32 pmc_vddp_sel_wait;
	/* Specifies the value for PMC_DDR_PWR */
	u32 pmc_ddr_pwr;
	/* Specifies the value for PMC_DDR_CFG */
	u32 pmc_ddr_cfg;
	/* Specifies the value for PMC_IO_DPD3_REQ */
	u32 pmc_io_dpd3_req;
	/* Specifies the wait time after programming PMC_IO_DPD3_REQ */
	u32 pmc_io_dpd3_req_wait;

	u32 pmc_io_dpd4_req_wait;

	/* Specifies the value for PMC_REG_SHORT */
	u32 pmc_reg_short;
	/* Specifies the value for PMC_NO_IOPOWER */
	u32 pmc_no_io_power;

	u32 pmc_ddr_ctrl_wait;
	u32 pmc_ddr_ctrl;

	/* Specifies the value for EMC_ACPD_CONTROL */
	u32 emc_acpd_control;

	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE0 */
	u32 emc_swizzle_rank0_byte0;
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE1 */
	u32 emc_swizzle_rank0_byte1;
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE2 */
	u32 emc_swizzle_rank0_byte2;
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE3 */
	u32 emc_swizzle_rank0_byte3;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE0 */
	u32 emc_swizzle_rank1_byte0;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE1 */
	u32 emc_swizzle_rank1_byte1;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE2 */
	u32 emc_swizzle_rank1_byte2;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE3 */
	u32 emc_swizzle_rank1_byte3;

	/* Specifies the value for EMC_TXDSRVTTGEN */
	u32 emc_txdsrvttgen;

	/* Specifies the value for EMC_DATA_BRLSHFT_0 */
	u32 emc_data_brlshft0;
	u32 emc_data_brlshft1;

	u32 emc_dqs_brlshft0;
	u32 emc_dqs_brlshft1;

	u32 emc_cmd_brlshft0;
	u32 emc_cmd_brlshft1;
	u32 emc_cmd_brlshft2;
	u32 emc_cmd_brlshft3;

	u32 emc_quse_brlshft0;
	u32 emc_quse_brlshft1;
	u32 emc_quse_brlshft2;
	u32 emc_quse_brlshft3;

	u32 emc_dll_cfg0;
	u32 emc_dll_cfg1;

	u32 emc_pmc_scratch1;
	u32 emc_pmc_scratch2;
	u32 emc_pmc_scratch3;

	u32 emc_pmacro_pad_cfg_ctrl;

	u32 emc_pmacro_vttgen_ctrl0;
	u32 emc_pmacro_vttgen_ctrl1;
	u32 emc_pmacro_vttgen_ctrl2;

	u32 emc_pmacro_brick_ctrl_rfu1;
	u32 emc_pmacro_cmd_brick_ctrl_fdpd;
	u32 emc_pmacro_brick_ctrl_rfu2;
	u32 emc_pmacro_data_brick_ctrl_fdpd;
	u32 emc_pmacro_bg_bias_ctrl0;
	u32 emc_pmacro_data_pad_rx_ctrl;
	u32 emc_pmacro_cmd_pad_rx_ctrl;
	u32 emc_pmacro_data_rx_term_mode;
	u32 emc_pmacro_cmd_rx_term_mode;
	u32 emc_pmacro_data_pad_tx_ctrl;
	u32 emc_pmacro_common_pad_tx_ctrl;
	u32 emc_pmacro_cmd_pad_tx_ctrl;
	u32 emc_cfg3;

	u32 emc_pmacro_tx_pwrd0;
	u32 emc_pmacro_tx_pwrd1;
	u32 emc_pmacro_tx_pwrd2;
	u32 emc_pmacro_tx_pwrd3;
	u32 emc_pmacro_tx_pwrd4;
	u32 emc_pmacro_tx_pwrd5;

	u32 emc_config_sample_delay;

	u32 emc_pmacro_brick_mapping0;
	u32 emc_pmacro_brick_mapping1;
	u32 emc_pmacro_brick_mapping2;

	u32 emc_pmacro_tx_sel_clk_src0;
	u32 emc_pmacro_tx_sel_clk_src1;
	u32 emc_pmacro_tx_sel_clk_src2;
	u32 emc_pmacro_tx_sel_clk_src3;
	u32 emc_pmacro_tx_sel_clk_src4;
	u32 emc_pmacro_tx_sel_clk_src5;

	u32 emc_pmacro_ddll_bypass;

	u32 emc_pmacro_ddll_pwrd0;
	u32 emc_pmacro_ddll_pwrd1;
	u32 emc_pmacro_ddll_pwrd2;

	u32 emc_pmacro_cmd_ctrl0;
	u32 emc_pmacro_cmd_ctrl1;
	u32 emc_pmacro_cmd_ctrl2;

	/* DRAM size information */

	/* Specifies the value for MC_EMEM_ADR_CFG */
	u32 mc_emem_adr_cfg;
	/* Specifies the value for MC_EMEM_ADR_CFG_DEV0 */
	u32 mc_emem_adr_cfg_dev0;
	/* Specifies the value for MC_EMEM_ADR_CFG_DEV1 */
	u32 mc_emem_adr_cfg_dev1;

	u32 mc_emem_adr_cfg_channel_mask;

	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG0 */
	u32 mc_emem_adr_cfg_bank_mask0;
	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG1 */
	u32 mc_emem_adr_cfg_bank_mask1;
	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG2 */
	u32 mc_emem_adr_cfg_bank_mask2;

	/*
	 * Specifies the value for MC_EMEM_CFG which holds the external memory
	 * size (in KBytes)
	 */
	u32 mc_emem_cfg;

	/* MC arbitration configuration */

	/* Specifies the value for MC_EMEM_ARB_CFG */
	u32 mc_emem_arb_cfg;
	/* Specifies the value for MC_EMEM_ARB_OUTSTANDING_REQ */
	u32 mc_emem_arb_outstanding_req;

	u32 emc_emem_arb_refpb_hp_ctrl;
	u32 emc_emem_arb_refpb_bank_ctrl;

	/* Specifies the value for MC_EMEM_ARB_TIMING_RCD */
	u32 mc_emem_arb_timing_rcd;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RP */
	u32 mc_emem_arb_timing_rp;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RC */
	u32 mc_emem_arb_timing_rc;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RAS */
	u32 mc_emem_arb_timing_ras;
	/* Specifies the value for MC_EMEM_ARB_TIMING_FAW */
	u32 mc_emem_arb_timing_faw;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RRD */
	u32 mc_emem_arb_timing_rrd;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RAP2PRE */
	u32 mc_emem_arb_timing_rap2pre;
	/* Specifies the value for MC_EMEM_ARB_TIMING_WAP2PRE */
	u32 mc_emem_arb_timing_wap2pre;
	/* Specifies the value for MC_EMEM_ARB_TIMING_R2R */
	u32 mc_emem_arb_timing_r2r;
	/* Specifies the value for MC_EMEM_ARB_TIMING_W2W */
	u32 mc_emem_arb_timing_w2w;
	/* Specifies the value for MC_EMEM_ARB_TIMING_R2W */
	u32 mc_emem_arb_timing_r2w;
	/* Specifies the value for MC_EMEM_ARB_TIMING_W2R */
	u32 mc_emem_arb_timing_w2r;

	u32 mc_emem_arb_timing_rfcpb;

	/* Specifies the value for MC_EMEM_ARB_DA_TURNS */
	u32 mc_emem_arb_da_turns;
	/* Specifies the value for MC_EMEM_ARB_DA_COVERS */
	u32 mc_emem_arb_da_covers;
	/* Specifies the value for MC_EMEM_ARB_MISC0 */
	u32 mc_emem_arb_misc0;
	/* Specifies the value for MC_EMEM_ARB_MISC1 */
	u32 mc_emem_arb_misc1;
	u32 mc_emem_arb_misc2;

	/* Specifies the value for MC_EMEM_ARB_RING1_THROTTLE */
	u32 mc_emem_arb_ring1_throttle;
	/* Specifies the value for MC_EMEM_ARB_OVERRIDE */
	u32 mc_emem_arb_override;
	/* Specifies the value for MC_EMEM_ARB_OVERRIDE_1 */
	u32 mc_emem_arb_override1;
	/* Specifies the value for MC_EMEM_ARB_RSV */
	u32 mc_emem_arb_rsv;

	u32 mc_da_cfg0;
	u32 mc_emem_arb_timing_ccdmw;

	/* Specifies the value for MC_CLKEN_OVERRIDE */
	u32 mc_clken_override;

	/* Specifies the value for MC_STAT_CONTROL */
	u32 mc_stat_control;
	/* Specifies the value for MC_VIDEO_PROTECT_BOM */
	u32 mc_video_protect_bom;
	/* Specifies the value for MC_VIDEO_PROTECT_BOM_ADR_HI */
	u32 mc_video_protect_bom_adr_hi;
	/* Specifies the value for MC_VIDEO_PROTECT_SIZE_MB */
	u32 mc_video_protect_size_mb;
	/* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE */
	u32 mc_video_protect_vpr_override;
	/* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE1 */
	u32 mc_video_protect_vpr_override1;
	/* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_0 */
	u32 mc_video_protect_gpu_override0;
	/* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_1 */
	u32 mc_video_protect_gpu_override1;
	/* Specifies the value for MC_SEC_CARVEOUT_BOM */
	u32 mc_sec_carveout_bom;
	/* Specifies the value for MC_SEC_CARVEOUT_ADR_HI */
	u32 mc_sec_carveout_adr_hi;
	/* Specifies the value for MC_SEC_CARVEOUT_SIZE_MB */
	u32 mc_sec_carveout_size_mb;
	/* Specifies the value for MC_VIDEO_PROTECT_REG_CTRL.VIDEO_PROTECT_WRITE_ACCESS */
	u32 mc_video_protect_write_access;
	/* Specifies the value for MC_SEC_CARVEOUT_REG_CTRL.SEC_CARVEOUT_WRITE_ACCESS */
	u32 mc_sec_carveout_protect_write_access;

	u32 mc_generalized_carveout1_bom;
	u32 mc_generalized_carveout1_bom_hi;
	u32 mc_generalized_carveout1_size_128kb;
	u32 mc_generalized_carveout1_access0;
	u32 mc_generalized_carveout1_access1;
	u32 mc_generalized_carveout1_access2;
	u32 mc_generalized_carveout1_access3;
	u32 mc_generalized_carveout1_access4;
	u32 mc_generalized_carveout1_force_internal_access0;
	u32 mc_generalized_carveout1_force_internal_access1;
	u32 mc_generalized_carveout1_force_internal_access2;
	u32 mc_generalized_carveout1_force_internal_access3;
	u32 mc_generalized_carveout1_force_internal_access4;
	u32 mc_generalized_carveout1_cfg0;

	u32 mc_generalized_carveout2_bom;
	u32 mc_generalized_carveout2_bom_hi;
	u32 mc_generalized_carveout2_size_128kb;
	u32 mc_generalized_carveout2_access0;
	u32 mc_generalized_carveout2_access1;
	u32 mc_generalized_carveout2_access2;
	u32 mc_generalized_carveout2_access3;
	u32 mc_generalized_carveout2_access4;
	u32 mc_generalized_carveout2_force_internal_access0;
	u32 mc_generalized_carveout2_force_internal_access1;
	u32 mc_generalized_carveout2_force_internal_access2;
	u32 mc_generalized_carveout2_force_internal_access3;
	u32 mc_generalized_carveout2_force_internal_access4;
	u32 mc_generalized_carveout2_cfg0;

	u32 mc_generalized_carveout3_bom;
	u32 mc_generalized_carveout3_bom_hi;
	u32 mc_generalized_carveout3_size_128kb;
	u32 mc_generalized_carveout3_access0;
	u32 mc_generalized_carveout3_access1;
	u32 mc_generalized_carveout3_access2;
	u32 mc_generalized_carveout3_access3;
	u32 mc_generalized_carveout3_access4;
	u32 mc_generalized_carveout3_force_internal_access0;
	u32 mc_generalized_carveout3_force_internal_access1;
	u32 mc_generalized_carveout3_force_internal_access2;
	u32 mc_generalized_carveout3_force_internal_access3;
	u32 mc_generalized_carveout3_force_internal_access4;
	u32 mc_generalized_carveout3_cfg0;

	u32 mc_generalized_carveout4_bom;
	u32 mc_generalized_carveout4_bom_hi;
	u32 mc_generalized_carveout4_size_128kb;
	u32 mc_generalized_carveout4_access0;
	u32 mc_generalized_carveout4_access1;
	u32 mc_generalized_carveout4_access2;
	u32 mc_generalized_carveout4_access3;
	u32 mc_generalized_carveout4_access4;
	u32 mc_generalized_carveout4_force_internal_access0;
	u32 mc_generalized_carveout4_force_internal_access1;
	u32 mc_generalized_carveout4_force_internal_access2;
	u32 mc_generalized_carveout4_force_internal_access3;
	u32 mc_generalized_carveout4_force_internal_access4;
	u32 mc_generalized_carveout4_cfg0;

	u32 mc_generalized_carveout5_bom;
	u32 mc_generalized_carveout5_bom_hi;
	u32 mc_generalized_carveout5_size_128kb;
	u32 mc_generalized_carveout5_access0;
	u32 mc_generalized_carveout5_access1;
	u32 mc_generalized_carveout5_access2;
	u32 mc_generalized_carveout5_access3;
	u32 mc_generalized_carveout5_access4;
	u32 mc_generalized_carveout5_force_internal_access0;
	u32 mc_generalized_carveout5_force_internal_access1;
	u32 mc_generalized_carveout5_force_internal_access2;
	u32 mc_generalized_carveout5_force_internal_access3;
	u32 mc_generalized_carveout5_force_internal_access4;
	u32 mc_generalized_carveout5_cfg0;

	/* Specifies enable for CA training */
	u32 emc_ca_training_enable;
	/* Set if bit 6 select is greater than bit 7 select; uses aremc.spec packet SWIZZLE_BIT6_GT_BIT7 */
	u32 swizzle_rank_byte_encode;
	/* Specifies enable and offset for patched boot rom write */
	u32 boot_rom_patch_control;
	/* Specifies data for patched boot rom write */
	u32 boot_rom_patch_data;

	/* Specifies the value for MC_MTS_CARVEOUT_BOM */
	u32 mc_mts_carveout_bom;
	/* Specifies the value for MC_MTS_CARVEOUT_ADR_HI */
	u32 mc_mts_carveout_adr_hi;
	/* Specifies the value for MC_MTS_CARVEOUT_SIZE_MB */
	u32 mc_mts_carveout_size_mb;
	/* Specifies the value for MC_MTS_CARVEOUT_REG_CTRL */
	u32 mc_mts_carveout_reg_ctrl;
} sdram_params_t;

#endif
