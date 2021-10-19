/*
 * Copyright (c) 2020-2021 CTCaer
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
 */

#define DRAM_CFG_T210B01_SIZE 2104

static const sdram_params_t210b01_t _dram_cfg_08_10_12_14_samsung_hynix_4gb = {
	/* Specifies the type of memory device */
	.memory_type                                     = MEMORY_TYPE_LPDDR4,

	/* MC/EMC clock source configuration */
	.pllm_input_divider                              = 0x00000001, // M div.
	.pllm_feedback_divider                           = 0x00000022, // N div.
	.pllm_stable_time                                = 0x0000012C,
	.pllm_setup_control                              = 0x00000000,
	.pllm_post_divider                               = 0x00000000, // P div.
	.pllm_kcp                                        = 0x00000000,
	.pllm_kvco                                       = 0x00000000,

	/* Spare BCT params */
	.emc_bct_spare0                                  = 0x00000000,
	.emc_bct_spare1                                  = 0x00000000,
	.emc_bct_spare2                                  = 0x00000000,
	.emc_bct_spare3                                  = 0x00000000,
	.emc_bct_spare4                                  = 0x00000000,
	.emc_bct_spare5                                  = 0x00000000,
	.emc_bct_spare6                                  = 0x00000000,
	.emc_bct_spare7                                  = 0x00000000,
	.emc_bct_spare8                                  = 0x00000000,
	.emc_bct_spare9                                  = 0x00000000,
	.emc_bct_spare10                                 = 0x00000000,
	.emc_bct_spare11                                 = 0x00000000,
	.emc_bct_spare12                                 = 0x00000000,
	.emc_bct_spare13                                 = 0x00000000,

	.emc_bct_spare_secure0                           = 0x00000000,
	.emc_bct_spare_secure1                           = 0x00000000,
	.emc_bct_spare_secure2                           = 0x00000000,
	.emc_bct_spare_secure3                           = 0x00000000,
	.emc_bct_spare_secure4                           = 0x00000000,
	.emc_bct_spare_secure5                           = 0x00000000,
	.emc_bct_spare_secure6                           = 0x00000000,
	.emc_bct_spare_secure7                           = 0x00000000,
	.emc_bct_spare_secure8                           = 0x00000000,
	.emc_bct_spare_secure9                           = 0x00000000,
	.emc_bct_spare_secure10                          = 0x00000000,
	.emc_bct_spare_secure11                          = 0x00000000,
	.emc_bct_spare_secure12                          = 0x00000000,
	.emc_bct_spare_secure13                          = 0x00000000,
	.emc_bct_spare_secure14                          = 0x00000000,
	.emc_bct_spare_secure15                          = 0x00000000,
	.emc_bct_spare_secure16                          = 0x00000000,
	.emc_bct_spare_secure17                          = 0x00000000,
	.emc_bct_spare_secure18                          = 0x00000000,
	.emc_bct_spare_secure19                          = 0x00000000,
	.emc_bct_spare_secure20                          = 0x00000000,
	.emc_bct_spare_secure21                          = 0x00000000,
	.emc_bct_spare_secure22                          = 0x00000000,
	.emc_bct_spare_secure23                          = 0x00000000,

	/* EMC clock configuration */
	.emc_clock_source                                = 0x40188002,
	.emc_clock_source_dll                            = 0x40000000,

	.clk_rst_pllm_misc20_override                    = 0x00000000,
	.clk_rst_pllm_misc20_override_enable             = 0x00000000,

	.clear_clock2_mc1                                = 0x00000000,

	/* Auto-calibration of EMC pads */
	.emc_auto_cal_interval                           = 0x001FFFFF,

	.emc_auto_cal_config                             = 0xA01A51D8,
	.emc_auto_cal_config2                            = 0x00000000,
	.emc_auto_cal_config3                            = 0x00880000,

	.emc_auto_cal_config4                            = 0x00880000,
	.emc_auto_cal_config5                            = 0x00001220,
	.emc_auto_cal_config6                            = 0x00880000,
	.emc_auto_cal_config7                            = 0x00880000,
	.emc_auto_cal_config8                            = 0x00880000,
	.emc_auto_cal_config9                            = 0x00000000,

	.emc_auto_cal_vref_sel0                          = 0xB3C5BCBC,
	.emc_auto_cal_vref_sel1                          = 0x00009E3C,

	.emc_auto_cal_channel                            = 0xC1E00302,

	.emc_pmacro_auto_cal_cfg0                        = 0x04040404,
	.emc_pmacro_auto_cal_cfg1                        = 0x04040404,
	.emc_pmacro_auto_cal_cfg2                        = 0x04040404,

	.emc_pmacro_rx_term                              = 0x3F3F3F3F,
	.emc_pmacro_dq_tx_drive                          = 0x3F3F3F3F,
	.emc_pmacro_ca_tx_drive                          = 0x3F3F3F3F,
	.emc_pmacro_cmd_tx_drive                         = 0x00001220,
	.emc_pmacro_auto_cal_common                      = 0x00000804,
	.emc_pmacro_zcrtl                                = 0x00505050,

	/* Specifies the time for the calibration to stabilize (in microseconds) */
	.emc_auto_cal_wait                               = 0x000001A1,

	.emc_xm2_comp_pad_ctrl                           = 0x00000030,
	.emc_xm2_comp_pad_ctrl2                          = 0x16001000,
	.emc_xm2_comp_pad_ctrl3                          = 0x00901000,

	/*
	 * DRAM size information
	 * Specifies the value for EMC_ADR_CFG
	 */
	.emc_adr_cfg                                     = 0x00000000, // 1 Rank.

	/*
	 * Specifies the time to wait after asserting pin
	 * CKE (in microseconds)
	 */
	.emc_pin_program_wait                            = 0x00000002,
	/* Specifies the extra delay before/after pin RESET/CKE command */
	.emc_pin_extra_wait                              = 0x00000000,

	.emc_pin_gpio_enable                             = 0x00000003,
	.emc_pin_gpio                                    = 0x00000003,

	/* Specifies the extra delay after the first writing of EMC_TIMING_CONTROL */
	.emc_timing_control_wait                         = 0x0000001E,

	/* Timing parameters required for the SDRAM */
	.emc_rc                                          = 0x0000000D,
	.emc_rfc                                         = 0x0000003A,
	.emc_rfc_pb                                      = 0x0000001D,
	.emc_ref_ctrl2                                   = 0x00000000,
	.emc_rfc_slr                                     = 0x00000000,
	.emc_ras                                         = 0x00000009,
	.emc_rp                                          = 0x00000004,
	.emc_r2r                                         = 0x00000000,
	.emc_w2w                                         = 0x00000000,
	.emc_r2w                                         = 0x0000000B,
	.emc_w2r                                         = 0x0000000D,
	.emc_r2p                                         = 0x00000008,
	.emc_w2p                                         = 0x0000000B,
	.emc_tppd                                        = 0x00000004,
	.emc_trtm                                        = 0x00000017,
	.emc_twtm                                        = 0x00000015,
	.emc_tratm                                       = 0x00000017,
	.emc_twatm                                       = 0x0000001B,
	.emc_tr2ref                                      = 0x00000000,
	.emc_ccdmw                                       = 0x00000020,
	.emc_rd_rcd                                      = 0x00000006,
	.emc_wr_rcd                                      = 0x00000006,
	.emc_rrd                                         = 0x00000006,
	.emc_rext                                        = 0x00000003,
	.emc_wext                                        = 0x00000000,
	.emc_wdv                                         = 0x00000004,
	.emc_wdv_chk                                     = 0x00000006,
	.emc_wsv                                         = 0x00000002,
	.emc_wev                                         = 0x00000000,
	.emc_wdv_mask                                    = 0x00000004,
	.emc_ws_duration                                 = 0x00000008,
	.emc_we_duration                                 = 0x0000000E,
	.emc_quse                                        = 0x00000005,
	.emc_quse_width                                  = 0x00000006,
	.emc_ibdly                                       = 0x00000000,
	.emc_obdly                                       = 0x00000000,
	.emc_einput                                      = 0x00000002,
	.emc_einput_duration                             = 0x0000000D,
	.emc_puterm_extra                                = 0x00000001,
	.emc_puterm_width                                = 0x80000000,
	.emc_qrst                                        = 0x00010000,
	.emc_qsafe                                       = 0x00000012,
	.emc_rdv                                         = 0x00000018,
	.emc_rdv_mask                                    = 0x0000001A,
	.emc_rdv_early                                   = 0x00000016,
	.emc_rdv_early_mask                              = 0x00000018,
	.emc_qpop                                        = 0x0000000A,
	.emc_refresh                                     = 0x00000304,
	.emc_burst_refresh_num                           = 0x00000000,
	.emc_prerefresh_req_cnt                          = 0x000000C1,
	.emc_pdex2wr                                     = 0x00000008,
	.emc_pdex2rd                                     = 0x00000008,
	.emc_pchg2pden                                   = 0x00000003,
	.emc_act2pden                                    = 0x0000000A,
	.emc_ar2pden                                     = 0x00000003,
	.emc_rw2pden                                     = 0x00000014,
	.emc_cke2pden                                    = 0x00000005,
	.emc_pdex2che                                    = 0x00000002,
	.emc_pdex2mrr                                    = 0x0000000D,
	.emc_txsr                                        = 0x0000003B,
	.emc_txsr_dll                                    = 0x0000003B,
	.emc_tcke                                        = 0x00000005,
	.emc_tckesr                                      = 0x00000005,
	.emc_tpd                                         = 0x00000004,
	.emc_tfaw                                        = 0x00000009,
	.emc_trpab                                       = 0x00000005,
	.emc_tclkstable                                  = 0x00000004,
	.emc_tclkstop                                    = 0x00000009,
	.emc_trefbw                                      = 0x0000031C,

	/* FBIO configuration values */
	.emc_fbio_cfg5                                   = 0x9160A00D,
	.emc_fbio_cfg7                                   = 0x00003A3F,
	.emc_fbio_cfg8                                   = 0x0CF30000,

	/* Command mapping for CMD brick 0 */
	.emc_cmd_mapping_cmd0_0                          = 0x061B0504,
	.emc_cmd_mapping_cmd0_1                          = 0x1C070302,
	.emc_cmd_mapping_cmd0_2                          = 0x05252523,
	.emc_cmd_mapping_cmd1_0                          = 0x0A091D08,
	.emc_cmd_mapping_cmd1_1                          = 0x0D1E0B24,
	.emc_cmd_mapping_cmd1_2                          = 0x0326260C,
	.emc_cmd_mapping_cmd2_0                          = 0x231C1B02,
	.emc_cmd_mapping_cmd2_1                          = 0x05070403,
	.emc_cmd_mapping_cmd2_2                          = 0x02252506,
	.emc_cmd_mapping_cmd3_0                          = 0x0D1D0B0A,
	.emc_cmd_mapping_cmd3_1                          = 0x1E090C08,
	.emc_cmd_mapping_cmd3_2                          = 0x08262624,
	.emc_cmd_mapping_byte                            = 0x9A070624,

	.emc_fbio_spare                                  = 0x00000012,
	.emc_cfg_rsv                                     = 0xFF00FF00,

	/* MRS command values */
	.emc_mrs                                         = 0x00000000,
	.emc_emrs                                        = 0x00000000,
	.emc_emrs2                                       = 0x00000000,
	.emc_emrs3                                       = 0x00000000,
	.emc_mrw1                                        = 0x88010004,
	.emc_mrw2                                        = 0x88020000,
	.emc_mrw3                                        = 0x880D0000,
	.emc_mrw4                                        = 0xC0000000,
	.emc_mrw6                                        = 0x88033131,
	.emc_mrw8                                        = 0x880B0000,
	.emc_mrw9                                        = 0x8C0E5D5D,
	.emc_mrw10                                       = 0x880C5D5D,
	.emc_mrw12                                       = 0x8C0D0808,
	.emc_mrw13                                       = 0x8C0D0000,
	.emc_mrw14                                       = 0x88161616,
	.emc_mrw_extra                                   = 0x88010004,
	.emc_warm_boot_mrw_extra                         = 0x08110000,
	.emc_warm_boot_extramode_reg_write_enable        = 0x00000001,
	.emc_extramode_reg_write_enable                  = 0x00000000,
	.emc_mrw_reset_command                           = 0x00000000,
	.emc_mrw_reset_ninit_wait                        = 0x00000000,
	.emc_mrs_wait_cnt                                = 0x00CC0010,
	.emc_mrs_wait_cnt2                               = 0x0033000A,

	/* EMC miscellaneous configurations */
	.emc_cfg                                         = 0xF3200000,
	.emc_cfg2                                        = 0x00110825,
	.emc_cfg_pipe                                    = 0x0FFF0000,
	.emc_cfg_pipe_clk                                = 0x00000000,
	.emc_fdpd_ctrl_cmd_no_ramp                       = 0x00000001,
	.emc_cfg_update                                  = 0x70000301,
	.emc_dbg                                         = 0x01000C00,
	.emc_dbg_write_mux                               = 0x00000001,
	.emc_cmd_q                                       = 0x10004408,
	.emc_mc2emc_q                                    = 0x06000404,
	.emc_dyn_self_ref_control                        = 0x00000713,
	.ahb_arbitration_xbar_ctrl_meminit_done          = 0x00000001,
	.emc_cfg_dig_dll                                 = 0x002C00A0,
	.emc_cfg_dig_dll_1                               = 0x000F3701,
	.emc_cfg_dig_dll_period                          = 0x00008000,
	.emc_dev_select                                  = 0x00000002, // Rank 0 only.
	.emc_sel_dpd_ctrl                                = 0x0004000C,

	/* Pads trimmer delays */
	.emc_fdpd_ctrl_dq                                = 0x8020221F,
	.emc_fdpd_ctrl_cmd                               = 0x0220F40F,
	.emc_pmacro_ib_vref_dq_0                         = 0x29292929,
	.emc_pmacro_ib_vref_dq_1                         = 0x29292929,
	.emc_pmacro_ib_vref_dqs_0                        = 0x29292929,
	.emc_pmacro_ib_vref_dqs_1                        = 0x29292929,
	.emc_pmacro_ib_rxrt                              = 0x00000078,
	.emc_cfg_pipe1                                   = 0x0FFF0000,
	.emc_cfg_pipe2                                   = 0x00000000,

	.emc_pmacro_quse_ddll_rank0_0                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank0_1                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank0_2                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank0_3                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank0_4                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank0_5                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank1_0                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank1_1                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank1_2                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank1_3                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank1_4                    = 0x00000000,
	.emc_pmacro_quse_ddll_rank1_5                    = 0x00000000,

	.emc_pmacro_ob_ddll_long_dq_rank0_0              = 0x00000000,
	.emc_pmacro_ob_ddll_long_dq_rank0_1              = 0x00000000,
	.emc_pmacro_ob_ddll_long_dq_rank0_2              = 0x00000000,
	.emc_pmacro_ob_ddll_long_dq_rank0_3              = 0x00000000,
	.emc_pmacro_ob_ddll_long_dq_rank0_4              = 0x000D0016,
	.emc_pmacro_ob_ddll_long_dq_rank0_5              = 0x0017000B,
	.emc_pmacro_ob_ddll_long_dq_rank1_0              = 0x00000000,
	.emc_pmacro_ob_ddll_long_dq_rank1_1              = 0x00000000,
	.emc_pmacro_ob_ddll_long_dq_rank1_2              = 0x00000000,
	.emc_pmacro_ob_ddll_long_dq_rank1_3              = 0x00000000,
	.emc_pmacro_ob_ddll_long_dq_rank1_4              = 0x000D0016,
	.emc_pmacro_ob_ddll_long_dq_rank1_5              = 0x0017000B,

	.emc_pmacro_ob_ddll_long_dqs_rank0_0             = 0x00450043,
	.emc_pmacro_ob_ddll_long_dqs_rank0_1             = 0x00430045,
	.emc_pmacro_ob_ddll_long_dqs_rank0_2             = 0x00470046,
	.emc_pmacro_ob_ddll_long_dqs_rank0_3             = 0x00460041,
	.emc_pmacro_ob_ddll_long_dqs_rank0_4             = 0x0003000C,
	.emc_pmacro_ob_ddll_long_dqs_rank0_5             = 0x000D0000,
	.emc_pmacro_ob_ddll_long_dqs_rank1_0             = 0x00450043,
	.emc_pmacro_ob_ddll_long_dqs_rank1_1             = 0x00430045,
	.emc_pmacro_ob_ddll_long_dqs_rank1_2             = 0x00470046,
	.emc_pmacro_ob_ddll_long_dqs_rank1_3             = 0x00460041,
	.emc_pmacro_ob_ddll_long_dqs_rank1_4             = 0x0003000C,
	.emc_pmacro_ob_ddll_long_dqs_rank1_5             = 0x000D0000,

	.emc_pmacro_ib_ddll_long_dqs_rank0_0             = 0x00280028,
	.emc_pmacro_ib_ddll_long_dqs_rank0_1             = 0x00280028,
	.emc_pmacro_ib_ddll_long_dqs_rank0_2             = 0x00280028,
	.emc_pmacro_ib_ddll_long_dqs_rank0_3             = 0x00280028,
	.emc_pmacro_ib_ddll_long_dqs_rank1_0             = 0x00280028,
	.emc_pmacro_ib_ddll_long_dqs_rank1_1             = 0x00280028,
	.emc_pmacro_ib_ddll_long_dqs_rank1_2             = 0x00280028,
	.emc_pmacro_ib_ddll_long_dqs_rank1_3             = 0x00280028,

	.emc_pmacro_ddll_long_cmd_0                      = 0x00160016,
	.emc_pmacro_ddll_long_cmd_1                      = 0x000D000D,
	.emc_pmacro_ddll_long_cmd_2                      = 0x000B000B,
	.emc_pmacro_ddll_long_cmd_3                      = 0x00170017,
	.emc_pmacro_ddll_long_cmd_4                      = 0x00000016,
	.emc_pmacro_ddll_short_cmd_0                     = 0x00000000,
	.emc_pmacro_ddll_short_cmd_1                     = 0x00000000,
	.emc_pmacro_ddll_short_cmd_2                     = 0x00000000,

	.emc_pmacro_ddll_periodic_offset                 = 0x00000000,

	/*
	 * Specifies the delay after asserting CKE pin during a WarmBoot0
	 * sequence (in microseconds)
	 */
	.warm_boot_wait                                  = 0x00000001,

	.emc_odt_write                                   = 0x00000000,

	/* Periodic ZQ calibration */

	/*
	 * Specifies the value for EMC_ZCAL_INTERVAL
	 * Value 0 disables ZQ calibration
	 */
	.emc_zcal_interval                               = 0x00064000,
	.emc_zcal_wait_cnt                               = 0x000900CC,
	.emc_zcal_mrw_cmd                                = 0x8051004F,

	/* DRAM initialization sequence flow control */
	.emc_mrs_reset_dll                               = 0x00000000,
	.emc_zcal_init_dev0                              = 0x80000001,
	.emc_zcal_init_dev1                              = 0x00000000,
	/*
	 * Specifies the wait time after programming a ZQ initialization
	 * command (in microseconds)
	 */
	.emc_zcal_init_wait                              = 0x00000001,
	/*
	 * Specifies the enable for ZQ calibration at cold boot [bit 0]
	 * and warm boot [bit 1]
	 */
	.emc_zcal_warm_cold_boot_enables                 = 0x00000003,

	/*
	 * Specifies the MRW command to LPDDR2 for ZQ calibration
	 * on warmboot
	 */
	/* Is issued to both devices separately */
	.emc_mrw_lpddr2zcal_warm_boot                    = 0x040A00AB,
	/*
	 * Specifies the ZQ command to DDR3 for ZQ calibration on warmboot
	 * Is issued to both devices separately
	 */
	.emc_zqcal_ddr3_warm_boot                        = 0x00000011,
	.emc_zqcal_lpddr4_warm_boot                      = 0x00000001,

	/*
	 * Specifies the wait time for ZQ calibration on warmboot
	 * (in microseconds)
	 */
	.emc_zcal_warm_boot_wait                         = 0x00000001,
	/*
	 * Specifies the enable for DRAM Mode Register programming
	 * at warm boot
	 */
	.emc_mrs_warm_boot_enable                        = 0x00000001,
	.emc_mrs_reset_dll_wait                          = 0x00000000,
	.emc_mrs_extra                                   = 0x00000000,
	.emc_warm_boot_mrs_extra                         = 0x00000000,
	.emc_emrs_ddr2_dll_enable                        = 0x00000000,
	.emc_mrs_ddr2_dll_reset                          = 0x00000000,
	.emc_emrs_ddr2_ocd_calib                         = 0x00000000,
	/*
	 * Specifies the wait between initializing DDR and setting OCD
	 * calibration (in microseconds)
	 */
	.emc_ddr2_wait                                   = 0x00000000,
	.emc_clken_override                              = 0x00000000,
	/*
	 * Specifies LOG2 of the extra refresh numbers after booting
	 * Program 0 to disable
	 */
	.emc_extra_refresh_num                           = 0x00000002,
	.emc_clken_override_allwarm_boot                 = 0x00000000,
	.mc_clken_override_allwarm_boot                  = 0x00000000,
	/* Specifies digital dll period, choosing between 4 to 64 ms */
	.emc_cfg_dig_dll_period_warm_boot                = 0x00000003,

	/* Pad controls */
	.pmc_vddp_sel                                    = 0x00000001,
	.pmc_vddp_sel_wait                               = 0x00000002,
	.pmc_ddr_cfg                                     = 0x04220100,
	.pmc_io_dpd3_req                                 = 0x4FAF9FFF,
	.pmc_io_dpd3_req_wait                            = 0x00000001,
	.pmc_io_dpd4_req_wait                            = 0x00000002,
	.pmc_reg_short                                   = 0x00000000,
	.pmc_no_io_power                                 = 0x00000000,
	.pmc_ddr_ctrl_wait                               = 0x00000000,
	.pmc_ddr_ctrl                                    = 0x0037FF9F,
	.emc_acpd_control                                = 0x00000000,

	.emc_swizzle_rank0_byte0                         = 0x76543201,
	.emc_swizzle_rank0_byte1                         = 0x65324710,
	.emc_swizzle_rank0_byte2                         = 0x25763410,
	.emc_swizzle_rank0_byte3                         = 0x25673401,
	.emc_swizzle_rank1_byte0                         = 0x32647501,
	.emc_swizzle_rank1_byte1                         = 0x34567201,
	.emc_swizzle_rank1_byte2                         = 0x56742310,
	.emc_swizzle_rank1_byte3                         = 0x67324501,

	.emc_txdsrvttgen                                 = 0x00000000,

	.emc_data_brlshft0                               = 0x00249249,
	.emc_data_brlshft1                               = 0x00249249,

	.emc_dqs_brlshft0                                = 0x00000000,
	.emc_dqs_brlshft1                                = 0x00000000,

	.emc_cmd_brlshft0                                = 0x00000000,
	.emc_cmd_brlshft1                                = 0x00000000,
	.emc_cmd_brlshft2                                = 0x00000012,
	.emc_cmd_brlshft3                                = 0x00000012,

	.emc_quse_brlshft0                               = 0x00000000,
	.emc_quse_brlshft1                               = 0x00000000,
	.emc_quse_brlshft2                               = 0x00000000,
	.emc_quse_brlshft3                               = 0x00000000,

	.emc_dll_cfg0                                    = 0x1F134120,
	.emc_dll_cfg1                                    = 0x00010014,

	.emc_pmc_scratch1                                = 0x4FAF9FFF,
	.emc_pmc_scratch2                                = 0x7FFFFFFF,
	.emc_pmc_scratch3                                = 0x4036D71F,

	.emc_pmacro_pad_cfg_ctrl                         = 0x00000000,
	.emc_pmacro_vttgen_ctrl0                         = 0x00090000,
	.emc_pmacro_vttgen_ctrl1                         = 0x00103400,
	.emc_pmacro_vttgen_ctrl2                         = 0x00000000,
	.emc_pmacro_dsr_vttgen_ctrl0                     = 0x00000009,
	.emc_pmacro_brick_ctrl_rfu1                      = 0x00000000,
	.emc_pmacro_cmd_brick_ctrl_fdpd                  = 0x00000000,
	.emc_pmacro_brick_ctrl_rfu2                      = 0x00000000,
	.emc_pmacro_data_brick_ctrl_fdpd                 = 0x00000000,
	.emc_pmacro_bg_bias_ctrl0                        = 0x00000000,
	.emc_pmacro_data_pad_rx_ctrl                     = 0x05050003,
	.emc_pmacro_cmd_pad_rx_ctrl                      = 0x05000000,
	.emc_pmacro_data_rx_term_mode                    = 0x00000210,
	.emc_pmacro_cmd_rx_term_mode                     = 0x00002000,
	.emc_pmacro_data_pad_tx_ctrl                     = 0x00000421,
	.emc_pmacro_cmd_pad_tx_ctrl                      = 0x00000000,

	.emc_cfg3                                        = 0x00000040,

	.emc_pmacro_tx_pwrd0                             = 0x10000000,
	.emc_pmacro_tx_pwrd1                             = 0x00000000,
	.emc_pmacro_tx_pwrd2                             = 0x00000000,
	.emc_pmacro_tx_pwrd3                             = 0x00000000,
	.emc_pmacro_tx_pwrd4                             = 0x00400080,
	.emc_pmacro_tx_pwrd5                             = 0x00801004,

	.emc_config_sample_delay                         = 0x00000020,

	.emc_pmacro_brick_mapping0                       = 0x28091081,
	.emc_pmacro_brick_mapping1                       = 0x44A53293,
	.emc_pmacro_brick_mapping2                       = 0x76678A5B,

	.emc_pmacro_tx_sel_clk_src0                      = 0x00000000,
	.emc_pmacro_tx_sel_clk_src1                      = 0x00000000,
	.emc_pmacro_tx_sel_clk_src2                      = 0x00000000,
	.emc_pmacro_tx_sel_clk_src3                      = 0x00000000,
	.emc_pmacro_tx_sel_clk_src4                      = 0x00000000,
	.emc_pmacro_tx_sel_clk_src5                      = 0x00000000,

	.emc_pmacro_perbit_fgcg_ctrl0                    = 0x00000000,
	.emc_pmacro_perbit_fgcg_ctrl1                    = 0x00000000,
	.emc_pmacro_perbit_fgcg_ctrl2                    = 0x00000000,
	.emc_pmacro_perbit_fgcg_ctrl3                    = 0x00000000,
	.emc_pmacro_perbit_fgcg_ctrl4                    = 0x00000000,
	.emc_pmacro_perbit_fgcg_ctrl5                    = 0x00000000,
	.emc_pmacro_perbit_rfu_ctrl0                     = 0x00000000,
	.emc_pmacro_perbit_rfu_ctrl1                     = 0x00000000,
	.emc_pmacro_perbit_rfu_ctrl2                     = 0x00000000,
	.emc_pmacro_perbit_rfu_ctrl3                     = 0x00000000,
	.emc_pmacro_perbit_rfu_ctrl4                     = 0x00000000,
	.emc_pmacro_perbit_rfu_ctrl5                     = 0x00000000,
	.emc_pmacro_perbit_rfu1_ctrl0                    = 0x00000000,
	.emc_pmacro_perbit_rfu1_ctrl1                    = 0x00000000,
	.emc_pmacro_perbit_rfu1_ctrl2                    = 0x00000000,
	.emc_pmacro_perbit_rfu1_ctrl3                    = 0x00000000,
	.emc_pmacro_perbit_rfu1_ctrl4                    = 0x00000000,
	.emc_pmacro_perbit_rfu1_ctrl5                    = 0x00000000,

	.emc_pmacro_data_pi_ctrl                         = 0x00001010,
	.emc_pmacro_cmd_pi_ctrl                          = 0x00001010,

	.emc_pmacro_ddll_bypass                          = 0xEF00EF00,

	.emc_pmacro_ddll_pwrd0                           = 0x00000000,
	.emc_pmacro_ddll_pwrd1                           = 0x00000000,
	.emc_pmacro_ddll_pwrd2                           = 0x1C1C1C1C,

	.emc_pmacro_cmd_ctrl0                            = 0x00000000,
	.emc_pmacro_cmd_ctrl1                            = 0x00000000,
	.emc_pmacro_cmd_ctrl2                            = 0x00000000,

	/* DRAM size information */
	.mc_emem_adr_cfg                                 = 0x00000000, // 1 Rank.
	.mc_emem_adr_cfg_dev0                            = 0x00080302, // Rank 0 Density 1024MB.
	.mc_emem_adr_cfg_dev1                            = 0x00080302, // Rank 1 Density 1024MB.
	.mc_emem_adr_cfg_channel_mask                    = 0xFFFF2400,
	.mc_emem_adr_cfg_bank_mask0                      = 0x6E574400,
	.mc_emem_adr_cfg_bank_mask1                      = 0x39722800,
	.mc_emem_adr_cfg_bank_mask2                      = 0x4B9C1000,
	/*
	 * Specifies the value for MC_EMEM_CFG which holds the external memory
	 * size (in KBytes)
	 */
	.mc_emem_cfg                                     = 0x00001000, // 4GB total density.

	/* MC arbitration configuration */
	.mc_emem_arb_cfg                                 = 0x08000001,
	.mc_emem_arb_outstanding_req                     = 0x8000004C,
	.emc_emem_arb_refpb_hp_ctrl                      = 0x000A1020,
	.emc_emem_arb_refpb_bank_ctrl                    = 0x80001028,

	.mc_emem_arb_timing_rcd                          = 0x00000001,
	.mc_emem_arb_timing_rp                           = 0x00000000,
	.mc_emem_arb_timing_rc                           = 0x00000003,
	.mc_emem_arb_timing_ras                          = 0x00000001,
	.mc_emem_arb_timing_faw                          = 0x00000002,
	.mc_emem_arb_timing_rrd                          = 0x00000001,
	.mc_emem_arb_timing_rap2pre                      = 0x00000002,
	.mc_emem_arb_timing_wap2pre                      = 0x00000005,
	.mc_emem_arb_timing_r2r                          = 0x00000001,
	.mc_emem_arb_timing_w2w                          = 0x00000001,
	.mc_emem_arb_timing_r2w                          = 0x00000004,
	.mc_emem_arb_timing_w2r                          = 0x00000005,
	.mc_emem_arb_timing_rfcpb                        = 0x00000007,

	.mc_emem_arb_da_turns                            = 0x02020000,
	.mc_emem_arb_da_covers                           = 0x00030201,
	.mc_emem_arb_misc0                               = 0x72A30504,
	.mc_emem_arb_misc1                               = 0x70000F0F,
	.mc_emem_arb_misc2                               = 0x00000000,

	.mc_emem_arb_ring1_throttle                      = 0x001F0000,
	.mc_emem_arb_override                            = 0x10000000,
	.mc_emem_arb_override1                           = 0x00000000,
	.mc_emem_arb_rsv                                 = 0xFF00FF00,

	.mc_da_cfg0                                      = 0x00000001,
	.mc_emem_arb_timing_ccdmw                        = 0x00000008,

	.mc_clken_override                               = 0x00008000,

	.mc_stat_control                                 = 0x00000000,
	.mc_video_protect_bom                            = 0xFFF00000,
	.mc_video_protect_bom_adr_hi                     = 0x00000000,
	.mc_video_protect_size_mb                        = 0x00000000,
	.mc_video_protect_vpr_override                   = 0xE4BAC343,
	.mc_video_protect_vpr_override1                  = 0x06001ED3, // Add SE2, SE2B.
	.mc_video_protect_gpu_override0                  = 0x00000000,
	.mc_video_protect_gpu_override1                  = 0x00000000,
	.mc_sec_carveout_bom                             = 0xFFF00000,
	.mc_sec_carveout_adr_hi                          = 0x00000000,
	.mc_sec_carveout_size_mb                         = 0x00000000,
	.mc_video_protect_write_access                   = 0x00000000,
	.mc_sec_carveout_protect_write_access            = 0x00000000,

	.mc_generalized_carveout1_bom                    = 0x00000000,
	.mc_generalized_carveout1_bom_hi                 = 0x00000000,
	.mc_generalized_carveout1_size_128kb             = 0x00000008,
	.mc_generalized_carveout1_access0                = 0x00000000,
	.mc_generalized_carveout1_access1                = 0x00000000,
	.mc_generalized_carveout1_access2                = 0x00300000,
	.mc_generalized_carveout1_access3                = 0x03000000,
	.mc_generalized_carveout1_access4                = 0x00000000,
	.mc_generalized_carveout1_force_internal_access0 = 0x00000000,
	.mc_generalized_carveout1_force_internal_access1 = 0x00000000,
	.mc_generalized_carveout1_force_internal_access2 = 0x00000000,
	.mc_generalized_carveout1_force_internal_access3 = 0x00000000,
	.mc_generalized_carveout1_force_internal_access4 = 0x00000000,
	.mc_generalized_carveout1_cfg0                   = 0x04000C76,

	.mc_generalized_carveout2_bom                    = 0x00000000,
	.mc_generalized_carveout2_bom_hi                 = 0x00000000,
	.mc_generalized_carveout2_size_128kb             = 0x00000002,
	.mc_generalized_carveout2_access0                = 0x00000000,
	.mc_generalized_carveout2_access1                = 0x00000000,
	.mc_generalized_carveout2_access2                = 0x03000000,
	.mc_generalized_carveout2_access3                = 0x00000000,
	.mc_generalized_carveout2_access4                = 0x00000300,
	.mc_generalized_carveout2_force_internal_access0 = 0x00000000,
	.mc_generalized_carveout2_force_internal_access1 = 0x00000000,
	.mc_generalized_carveout2_force_internal_access2 = 0x00000000,
	.mc_generalized_carveout2_force_internal_access3 = 0x00000000,
	.mc_generalized_carveout2_force_internal_access4 = 0x00000000,
	.mc_generalized_carveout2_cfg0                   = 0x0440167E,

	.mc_generalized_carveout3_bom                    = 0x00000000,
	.mc_generalized_carveout3_bom_hi                 = 0x00000000,
	.mc_generalized_carveout3_size_128kb             = 0x00000000,
	.mc_generalized_carveout3_access0                = 0x00000000,
	.mc_generalized_carveout3_access1                = 0x00000000,
	.mc_generalized_carveout3_access2                = 0x03000000,
	.mc_generalized_carveout3_access3                = 0x00000000,
	.mc_generalized_carveout3_access4                = 0x00000300,
	.mc_generalized_carveout3_force_internal_access0 = 0x00000000,
	.mc_generalized_carveout3_force_internal_access1 = 0x00000000,
	.mc_generalized_carveout3_force_internal_access2 = 0x00000000,
	.mc_generalized_carveout3_force_internal_access3 = 0x00000000,
	.mc_generalized_carveout3_force_internal_access4 = 0x00000000,
	.mc_generalized_carveout3_cfg0                   = 0x04401E7E,

	.mc_generalized_carveout4_bom                    = 0x00000000,
	.mc_generalized_carveout4_bom_hi                 = 0x00000000,
	.mc_generalized_carveout4_size_128kb             = 0x00000008,
	.mc_generalized_carveout4_access0                = 0x00000000,
	.mc_generalized_carveout4_access1                = 0x00000000,
	.mc_generalized_carveout4_access2                = 0x00300000,
	.mc_generalized_carveout4_access3                = 0x00000000,
	.mc_generalized_carveout4_access4                = 0x000000C0,
	.mc_generalized_carveout4_force_internal_access0 = 0x00000000,
	.mc_generalized_carveout4_force_internal_access1 = 0x00000000,
	.mc_generalized_carveout4_force_internal_access2 = 0x00000000,
	.mc_generalized_carveout4_force_internal_access3 = 0x00000000,
	.mc_generalized_carveout4_force_internal_access4 = 0x00000000,
	.mc_generalized_carveout4_cfg0                   = 0x04002446,

	.mc_generalized_carveout5_bom                    = 0x00000000,
	.mc_generalized_carveout5_bom_hi                 = 0x00000000,
	.mc_generalized_carveout5_size_128kb             = 0x00000008,
	.mc_generalized_carveout5_access0                = 0x00000000,
	.mc_generalized_carveout5_access1                = 0x00000000,
	.mc_generalized_carveout5_access2                = 0x00300000,
	.mc_generalized_carveout5_access3                = 0x00000000,
	.mc_generalized_carveout5_access4                = 0x00000000,
	.mc_generalized_carveout5_force_internal_access0 = 0x00000000,
	.mc_generalized_carveout5_force_internal_access1 = 0x00000000,
	.mc_generalized_carveout5_force_internal_access2 = 0x00000000,
	.mc_generalized_carveout5_force_internal_access3 = 0x00000000,
	.mc_generalized_carveout5_force_internal_access4 = 0x00000000,
	.mc_generalized_carveout5_cfg0                   = 0x04002C46,

	/* Specifies enable for CA training */
	.emc_ca_training_enable                          = 0x00000000,
	/* Set if bit 6 select is greater than bit 7 select; uses aremc.spec packet SWIZZLE_BIT6_GT_BIT7 */
	.swizzle_rank_byte_encode                        = 0x000000EC,

	/* Specifies enable and offset for patched boot rom write */
	.boot_rom_patch_control                          = 0x00000000,
	/* Specifies data for patched boot rom write */
	.boot_rom_patch_data                             = 0x00000000,

	.mc_mts_carveout_bom                             = 0xFFF00000,
	.mc_mts_carveout_adr_hi                          = 0x00000000,
	.mc_mts_carveout_size_mb                         = 0x00000000,
	.mc_mts_carveout_reg_ctrl                        = 0x00000000,

	/* Specifies the untranslated memory access control */
	.mc_untranslated_region_check                    = 0x00000000,

	/* Just a place holder for special usage when there is no BCT for certain registers */
	.bct_na                                          = 0x00000000,
};

//!TODO Find out what mc_video_protect_gpu_override0 and mc_video_protect_gpu_override1 new bits are.

static const sdram_vendor_patch_t sdram_cfg_vendor_patches_t210b01[] = {

	// Samsung LPDDR4X 4GB X1X2 for prototype Iowa.
	{ 0x000E0022, 0x3AC / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dq_rank0_4.
	{ 0x001B0010, 0x3B0 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dq_rank0_5.
	{ 0x000E0022, 0x3C4 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dq_rank1_4.
	{ 0x001B0010, 0x3C8 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dq_rank1_5.
	{ 0x00490043, 0x3CC / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank0_0.
	{ 0x00420045, 0x3D0 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank0_1.
	{ 0x00490047, 0x3D4 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank0_2.
	{ 0x00460047, 0x3D8 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank0_3.
	{ 0x00000016, 0x3DC / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank0_4.
	{ 0x00100000, 0x3E0 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank0_5.
	{ 0x00490043, 0x3E4 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank1_0.
	{ 0x00420045, 0x3E8 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank1_1.
	{ 0x00490047, 0x3EC / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank1_2.
	{ 0x00460047, 0x3F0 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank1_3.
	{ 0x00000016, 0x3F4 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank1_4.
	{ 0x00100000, 0x3F8 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ob_ddll_long_dqs_rank1_5.
	{ 0x00220022, 0x41C / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ddll_long_cmd_0.
	{ 0x000E000E, 0x420 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ddll_long_cmd_1.
	{ 0x00100010, 0x424 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ddll_long_cmd_2.
	{ 0x001B001B, 0x428 / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ddll_long_cmd_3.
	{ 0x00000022, 0x42C / 4, LPDDR4X_4GB_SAMSUNG_X1X2 }, // emc_pmacro_ddll_long_cmd_4.

	// Samsung LPDDR4X 8GB K4UBE3D4AM-MGCJ Die-M for SDEV Iowa and Hoag.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_auto_cal_vref_sel0.
	{ 0x00000001, 0x134 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_adr_cfg. 2 Ranks.
	{ 0x00000006, 0x1CC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_quse.
	{ 0x00000005, 0x1D0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_quse_width.
	{ 0x00000003, 0x1DC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_einput.
	{ 0x0000000C, 0x1E0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_einput_duration.
	{ 0x08010004, 0x2B8 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw1.
	{ 0x08020000, 0x2BC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw2.
	{ 0x080D0000, 0x2C0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw3.
	{ 0x08033131, 0x2C8 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw6.
	{ 0x080B0000, 0x2CC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw8.
	{ 0x0C0E5D5D, 0x2D0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw9.
	{ 0x080C5D5D, 0x2D4 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw10.
	{ 0x0C0D0808, 0x2D8 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw12.
	{ 0x0C0D0000, 0x2DC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw13.
	{ 0x08161414, 0x2E0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw14.
	{ 0x08010004, 0x2E4 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_mrw_extra.
	{ 0x00000000, 0x340 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_dev_select. Both devices.
	{ 0x35353535, 0x350 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_vref_dq_0.
	{ 0x35353535, 0x354 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_vref_dq_1.
	{ 0x00100010, 0x3FC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_ddll_long_dqs_rank0_0.
	{ 0x00100010, 0x400 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_ddll_long_dqs_rank0_1.
	{ 0x00100010, 0x404 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_ddll_long_dqs_rank0_2.
	{ 0x00100010, 0x408 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_ddll_long_dqs_rank0_3.
	{ 0x00100010, 0x40C / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_ddll_long_dqs_rank1_0.
	{ 0x00100010, 0x410 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_ddll_long_dqs_rank1_1.
	{ 0x00100010, 0x414 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_ddll_long_dqs_rank1_2.
	{ 0x00100010, 0x418 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_ib_ddll_long_dqs_rank1_3.
	{ 0x0051004F, 0x450 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_zcal_mrw_cmd.
	{ 0x40000001, 0x45C / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_zcal_init_dev1.
	{ 0x00000000, 0x594 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_tx_pwrd4.
	{ 0x00001000, 0x598 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // emc_pmacro_tx_pwrd5.
	{ 0x00000001, 0x630 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // mc_emem_adr_cfg. 2 Ranks.
	{ 0x00002000, 0x64C / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // mc_emem_cfg. 8GB total density.
	{ 0x00000002, 0x680 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // mc_emem_arb_timing_r2r.
	{ 0x02020001, 0x694 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // mc_emem_arb_da_turns.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ }, // mc_video_protect_gpu_override1.

	// Micron LPDDR4X 4GB MT53D1024M32D1NP-053-WT:E Die-E for retail Iowa and Hoag.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE }, // emc_auto_cal_vref_sel0.
	{ 0x88161414, 0x2E0 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE }, // emc_mrw14.
	{ 0x80000713, 0x32C / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE }, // emc_dyn_self_ref_control.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE }, // mc_video_protect_gpu_override1.

	// Samsung LPDDR4X 4GB (Y01) Die-? for Iowa.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_auto_cal_vref_sel0.
	{ 0x88161414, 0x2E0 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_mrw14.
	{ 0x80000713, 0x32C / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_dyn_self_ref_control.
	{ 0x32323232, 0x350 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_vref_dq_0.
	{ 0x32323232, 0x354 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_vref_dq_1.
	{ 0x000F0018, 0x3AC / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dq_rank0_4.
	{ 0x000F0018, 0x3C4 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dq_rank1_4.
	{ 0x00440048, 0x3CC / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_0.
	{ 0x00440045, 0x3D0 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_1.
	{ 0x00470047, 0x3D4 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_2.
	{ 0x0005000D, 0x3DC / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_4.
	{ 0x00440048, 0x3E4 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_0.
	{ 0x00440045, 0x3E8 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_1.
	{ 0x00470047, 0x3EC / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_2.
	{ 0x0005000D, 0x3F4 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_4.
	{ 0x00780078, 0x3FC / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_ddll_long_dqs_rank0_0.
	{ 0x00780078, 0x400 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_ddll_long_dqs_rank0_1.
	{ 0x00780078, 0x404 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_ddll_long_dqs_rank0_2.
	{ 0x00780078, 0x408 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_ddll_long_dqs_rank0_3.
	{ 0x00780078, 0x40C / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_ddll_long_dqs_rank1_0.
	{ 0x00780078, 0x410 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_ddll_long_dqs_rank1_1.
	{ 0x00780078, 0x414 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_ddll_long_dqs_rank1_2.
	{ 0x00780078, 0x418 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ib_ddll_long_dqs_rank1_3.
	{ 0x00180018, 0x41C / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ddll_long_cmd_0.
	{ 0x000F000F, 0x420 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ddll_long_cmd_1.
	{ 0x00000018, 0x42C / 4, LPDDR4X_4GB_SAMSUNG_Y }, // emc_pmacro_ddll_long_cmd_4.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_4GB_SAMSUNG_Y }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_4GB_SAMSUNG_Y }, // mc_video_protect_gpu_override1.

	// Samsung LPDDR4X 4GB K4U6E3S4AA-MGCL 10nm-class (1y-X03) Die-A for retail Iowa, Hoag and Aula.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // emc_auto_cal_vref_sel0.
	{ 0x00000006, 0x1CC / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // emc_quse.
	{ 0x00000005, 0x1D0 / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // emc_quse_width.
	{ 0x00000003, 0x1DC / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // emc_einput.
	{ 0x0000000C, 0x1E0 / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // emc_einput_duration.
	{ 0x88161414, 0x2E0 / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // emc_mrw14.
	{ 0x80000713, 0x32C / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // emc_dyn_self_ref_control.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL }, // mc_video_protect_gpu_override1.

	// Samsung LPDDR4X 8GB K4UBE3D4AA-MGCL 10nm-class (1y-X03) Die-A for SDEV Iowa, Hoag and Aula.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_auto_cal_vref_sel0.
	{ 0x00000001, 0x134 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_adr_cfg. 2 Ranks.
	{ 0x00000006, 0x1CC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_quse.
	{ 0x00000005, 0x1D0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_quse_width.
	{ 0x00000003, 0x1DC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_einput.
	{ 0x0000000C, 0x1E0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_einput_duration.
	{ 0x00000008, 0x24C / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_tfaw.
	{ 0x08010004, 0x2B8 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw1.
	{ 0x08020000, 0x2BC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw2.
	{ 0x080D0000, 0x2C0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw3.
	{ 0x08033131, 0x2C8 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw6.
	{ 0x080B0000, 0x2CC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw8.
	{ 0x0C0E5D5D, 0x2D0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw9.
	{ 0x080C5D5D, 0x2D4 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw10.
	{ 0x0C0D0808, 0x2D8 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw12.
	{ 0x0C0D0000, 0x2DC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw13.
	{ 0x08161414, 0x2E0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw14.
	{ 0x08010004, 0x2E4 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_mrw_extra.
	{ 0x00000000, 0x340 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_dev_select. Both devices.
	{ 0x0051004F, 0x450 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_zcal_mrw_cmd.
	{ 0x40000001, 0x45C / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_zcal_init_dev1.
	{ 0x00000000, 0x594 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_pmacro_tx_pwrd4.
	{ 0x00001000, 0x598 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // emc_pmacro_tx_pwrd5.
	{ 0x00000001, 0x630 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // mc_emem_adr_cfg. 2 Ranks.
	{ 0x00002000, 0x64C / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // mc_emem_cfg. 8GB total density.
	{ 0x00000001, 0x670 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // mc_emem_arb_timing_faw.
	{ 0x00000002, 0x680 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // mc_emem_arb_timing_r2r.
	{ 0x02020001, 0x694 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // mc_emem_arb_da_turns.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL }, // mc_video_protect_gpu_override1.

	// Samsung LPDDR4X 4GB 10nm-class (1y-Y01) Die-? for Iowa.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_auto_cal_vref_sel0.
	{ 0x00000008, 0x24C / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_tfaw.
	{ 0x88161414, 0x2E0 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_mrw14.
	{ 0x80000713, 0x32C / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_dyn_self_ref_control.
	{ 0x000F0018, 0x3AC / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dq_rank0_4.
	{ 0x000F0018, 0x3C4 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dq_rank1_4.
	{ 0x00440048, 0x3CC / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_0.
	{ 0x00440045, 0x3D0 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_1.
	{ 0x00470047, 0x3D4 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_2.
	{ 0x0005000D, 0x3DC / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_4.
	{ 0x00440048, 0x3E4 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_0.
	{ 0x00440045, 0x3E8 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_1.
	{ 0x00470047, 0x3EC / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_2.
	{ 0x0005000D, 0x3F4 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_4.
	{ 0x00180018, 0x41C / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ddll_long_cmd_0.
	{ 0x000F000F, 0x420 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ddll_long_cmd_1.
	{ 0x00000018, 0x42C / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // emc_pmacro_ddll_long_cmd_4.
	{ 0x00000001, 0x670 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // mc_emem_arb_timing_faw.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_4GB_SAMSUNG_1Y_Y }, // mc_video_protect_gpu_override1.

	// Samsung LPDDR4X 8GB 10nm-class (1y-Y01) Die-? for SDEV Iowa.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_auto_cal_vref_sel0.
	{ 0x00000001, 0x134 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_adr_cfg. 2 Ranks.
	{ 0x00000008, 0x24C / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_tfaw.
	{ 0x08010004, 0x2B8 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw1.
	{ 0x08020000, 0x2BC / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw2.
	{ 0x080D0000, 0x2C0 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw3.
	{ 0x08033131, 0x2C8 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw6.
	{ 0x080B0000, 0x2CC / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw8.
	{ 0x0C0E5D5D, 0x2D0 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw9.
	{ 0x080C5D5D, 0x2D4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw10.
	{ 0x0C0D0808, 0x2D8 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw12.
	{ 0x0C0D0000, 0x2DC / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw13.
	{ 0x08161414, 0x2E0 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw14.
	{ 0x08010004, 0x2E4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_mrw_extra.
	{ 0x00000000, 0x340 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_dev_select. Both devices.
	{ 0x32323232, 0x350 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ib_vref_dq_0.
	{ 0x32323232, 0x354 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ib_vref_dq_1.
	{ 0x000F0018, 0x3AC / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dq_rank0_4.
	{ 0x000F0018, 0x3C4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dq_rank1_4.
	{ 0x00440048, 0x3CC / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_0.
	{ 0x00440045, 0x3D0 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_1.
	{ 0x00470047, 0x3D4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_2.
	{ 0x0005000D, 0x3DC / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank0_4.
	{ 0x00440048, 0x3E4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_0.
	{ 0x00440045, 0x3E8 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_1.
	{ 0x00470047, 0x3EC / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_2.
	{ 0x0005000D, 0x3F4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ob_ddll_long_dqs_rank1_4.
	{ 0x00180018, 0x41C / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ddll_long_cmd_0.
	{ 0x000F000F, 0x420 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ddll_long_cmd_1.
	{ 0x00000018, 0x42C / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_ddll_long_cmd_4.
	{ 0x0051004F, 0x450 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_zcal_mrw_cmd.
	{ 0x40000001, 0x45C / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_zcal_init_dev1.
	{ 0x00000000, 0x594 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_tx_pwrd4.
	{ 0x00001000, 0x598 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // emc_pmacro_tx_pwrd5.
	{ 0x00000001, 0x630 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // mc_emem_adr_cfg. 2 Ranks.
	{ 0x00002000, 0x64C / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // mc_emem_cfg. 8GB total density.
	{ 0x00000001, 0x670 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // mc_emem_arb_timing_faw.
	{ 0x00000002, 0x680 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // mc_emem_arb_timing_r2r.
	{ 0x02020001, 0x694 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // mc_emem_arb_da_turns.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_8GB_SAMSUNG_1Y_Y }, // mc_video_protect_gpu_override1.

/*
	// Samsung LPDDR4X 8GB 10nm-class (1y-A01) Die-? for SDEV Aula?
	{ 0x00000001, 0x134 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_adr_cfg. 2 Ranks.
	{ 0x08010004, 0x2B8 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw1.
	{ 0x08020000, 0x2BC / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw2.
	{ 0x080D0000, 0x2C0 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw3.
	{ 0x08033131, 0x2C8 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw6.
	{ 0x080B0000, 0x2CC / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw8.
	{ 0x0C0E5D5D, 0x2D0 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw9.
	{ 0x080C5D5D, 0x2D4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw10.
	{ 0x0C0D0808, 0x2D8 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw12.
	{ 0x0C0D0000, 0x2DC / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw13.
	{ 0x08161414, 0x2E0 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw14.
	{ 0x08010004, 0x2E4 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_mrw_extra.
	{ 0x00000000, 0x340 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_dev_select. Both devices.
	{ 0x35353535, 0x350 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_vref_dq_0.
	{ 0x35353535, 0x354 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_vref_dq_1.
	{ 0x35353535, 0x358 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_vref_dqs_0.
	{ 0x35353535, 0x35C / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_vref_dqs_1.
	{ 0x00480048, 0x3FC / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_ddll_long_dqs_rank0_0.
	{ 0x00480048, 0x400 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_ddll_long_dqs_rank0_1.
	{ 0x00480048, 0x404 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_ddll_long_dqs_rank0_2.
	{ 0x00480048, 0x408 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_ddll_long_dqs_rank0_3.
	{ 0x00480048, 0x40C / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_ddll_long_dqs_rank1_0.
	{ 0x00480048, 0x410 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_ddll_long_dqs_rank1_1.
	{ 0x00480048, 0x414 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_ddll_long_dqs_rank1_2.
	{ 0x00480048, 0x418 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_ib_ddll_long_dqs_rank1_3.
	{ 0x0051004F, 0x450 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_zcal_mrw_cmd.
	{ 0x40000001, 0x45C / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_zcal_init_dev1.
	{ 0x00010100, 0x594 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_tx_pwrd4.
	{ 0x00400010, 0x598 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // emc_pmacro_tx_pwrd5.
	{ 0x00000001, 0x630 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // mc_emem_adr_cfg. 2 Ranks.
	{ 0x00002000, 0x64C / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // mc_emem_cfg. 8GB total density.
	{ 0x00000002, 0x670 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // mc_emem_arb_timing_faw.
	{ 0x00000002, 0x680 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // mc_emem_arb_timing_r2r.
	{ 0x02020001, 0x694 / 4, LPDDR4X_8GB_SAMSUNG_1Y_A }, // mc_emem_arb_da_turns.
*/

	// Micron LPDDR4X 4GB MT53D1024M32D1NP-053-WT:F 10nm-class (1y-01) Die-F for Newer Iowa/Hoag/Aula.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_auto_cal_vref_sel0.
	{ 0x00000006, 0x1CC / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_quse.
	{ 0x00000005, 0x1D0 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_quse_width.
	{ 0x00000003, 0x1DC / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_einput.
	{ 0x0000000C, 0x1E0 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_einput_duration.
	{ 0x00000008, 0x24C / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_tfaw.
	{ 0x88161414, 0x2E0 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_mrw14.
	{ 0x80000713, 0x32C / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // emc_dyn_self_ref_control.
	{ 0x00000001, 0x670 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // mc_emem_arb_timing_faw.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF }, // mc_video_protect_gpu_override1.

	// Hynix LPDDR4X 4GB 10nm-class (1y-01) Die-A for Unknown Iowa/Hoag/Aula.
	{ 0x05500000, 0x0D4 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_auto_cal_config2.
	{ 0xC9AFBCBC, 0x0F4 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_auto_cal_vref_sel0.
	{ 0x00000006, 0x1CC / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_quse.
	{ 0x00000005, 0x1D0 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_quse_width.
	{ 0x00000003, 0x1DC / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_einput.
	{ 0x0000000C, 0x1E0 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_einput_duration.
	{ 0x00000008, 0x24C / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_tfaw.
	{ 0x88161414, 0x2E0 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_mrw14.
	{ 0x80000713, 0x32C / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // emc_dyn_self_ref_control.
	{ 0x00000001, 0x670 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // mc_emem_arb_timing_faw.
	{ 0xE4FACB43, 0x6D4 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // mc_video_protect_vpr_override. + TSEC, NVENC.
	{ 0x0600FED3, 0x6D8 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // mc_video_protect_vpr_override1. + TSECB, TSEC1, TSECB1.
	{ 0x2A800000, 0x6DC / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // mc_video_protect_gpu_override0.
	{ 0x00000002, 0x6E0 / 4, LPDDR4X_4GB_HYNIX_1Y_A }, // mc_video_protect_gpu_override1.

	//!TODO: Too many duplicates.
};
