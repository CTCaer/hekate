/*
* Copyright (c) 2018 naehrwert
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

#include "i2c.h"
#include "t210.h"
#include "mc.h"
#include "emc.h"
#include "pmc.h"
#include "util.h"
#include "fuse.h"
#include "max77620.h"
#include "sdram_param_t210.h"

#define CONFIG_SDRAM_COMPRESS_CFG

#ifdef CONFIG_SDRAM_COMPRESS_CFG
#include "lz.h"
#include "sdram_lz.inl"
#else
#include "sdram.inl"
#endif

static u32 _get_sdram_id()
{
	return (fuse_read_odm(4) & 0x38) >> 3;
}

static void _sdram_config(const sdram_params_t *params)
{
	PMC(0x45C) = (((4 * params->emc_pmc_scratch1 >> 2) + 0x80000000) ^ 0xFFFF) & 0xC000FFFF;
	sleep(params->pmc_io_dpd3_req_wait);

	u32 req = (4 * params->emc_pmc_scratch2 >> 2) + 0x80000000;
	PMC(APBDEV_PMC_IO_DPD4_REQ) = (req >> 16 << 16) ^ 0x3FFF0000;
	sleep(params->pmc_io_dpd4_req_wait);
	PMC(APBDEV_PMC_IO_DPD4_REQ) = (req ^ 0xFFFF) & 0xC000FFFF;
	sleep(params->pmc_io_dpd4_req_wait);
	PMC(APBDEV_PMC_WEAK_BIAS) = 0;
	sleep(1);

	CLOCK(0x98) = params->pllm_setup_control;
	CLOCK(0x9C) = 0;
	CLOCK(0x90) = (params->pllm_feedback_divider << 8) | params->pllm_input_divider | 0x40000000 | ((params->pllm_post_divider & 0xFFFF) << 20);

	u32 wait_end = TMR(0x10) + 300;
	while (!(CLOCK(0x90) & 0x8000000))
	{
		if (TMR(0x10) >= wait_end)
			goto break_nosleep;
	}
	sleep(10);
break_nosleep:

	CLOCK(0x19C) = ((params->mc_emem_arb_misc0 >> 11) & 0x10000) | (params->emc_clock_source & 0xFFFEFFFF);
	if (params->emc_clock_source_dll)
		CLOCK(0x664) = params->emc_clock_source_dll;
	if (params->clear_clock2_mc1)
		CLOCK(0x44C) = 0x40000000;
	CLOCK(0x328) = 0x2000001;
	CLOCK(0x284) = 0x4000;
	CLOCK(0x30C) = 0x2000001;
	EMC(EMC_PMACRO_VTTGEN_CTRL_0) = params->emc_pmacro_vttgen_ctrl0;
	EMC(EMC_PMACRO_VTTGEN_CTRL_1) = params->emc_pmacro_vttgen_ctrl1;
	EMC(EMC_PMACRO_VTTGEN_CTRL_2) = params->emc_pmacro_vttgen_ctrl2;
	EMC(EMC_TIMING_CONTROL) = 1;
	sleep(1);
	EMC(EMC_DBG) = (params->emc_dbg_write_mux << 1) | params->emc_dbg;
	if (params->emc_bct_spare2)
		*(vu32 *)params->emc_bct_spare2 = params->emc_bct_spare3;
	EMC(EMC_FBIO_CFG7) = params->emc_fbio_cfg7;
	EMC(EMC_CMD_MAPPING_CMD0_0) = params->emc_cmd_mapping_cmd0_0;
	EMC(EMC_CMD_MAPPING_CMD0_1) = params->emc_cmd_mapping_cmd0_1;
	EMC(EMC_CMD_MAPPING_CMD0_2) = params->emc_cmd_mapping_cmd0_2;
	EMC(EMC_CMD_MAPPING_CMD1_0) = params->emc_cmd_mapping_cmd1_0;
	EMC(EMC_CMD_MAPPING_CMD1_1) = params->emc_cmd_mapping_cmd1_1;
	EMC(EMC_CMD_MAPPING_CMD1_2) = params->emc_cmd_mapping_cmd1_2;
	EMC(EMC_CMD_MAPPING_CMD2_0) = params->emc_cmd_mapping_cmd2_0;
	EMC(EMC_CMD_MAPPING_CMD2_1) = params->emc_cmd_mapping_cmd2_1;
	EMC(EMC_CMD_MAPPING_CMD2_2) = params->emc_cmd_mapping_cmd2_2;
	EMC(EMC_CMD_MAPPING_CMD3_0) = params->emc_cmd_mapping_cmd3_0;
	EMC(EMC_CMD_MAPPING_CMD3_1) = params->emc_cmd_mapping_cmd3_1;
	EMC(EMC_CMD_MAPPING_CMD3_2) = params->emc_cmd_mapping_cmd3_2;
	EMC(EMC_CMD_MAPPING_BYTE) = params->emc_cmd_mapping_byte;
	EMC(EMC_PMACRO_BRICK_MAPPING_0) = params->emc_pmacro_brick_mapping0;
	EMC(EMC_PMACRO_BRICK_MAPPING_1) = params->emc_pmacro_brick_mapping1;
	EMC(EMC_PMACRO_BRICK_MAPPING_2) = params->emc_pmacro_brick_mapping2;
	EMC(EMC_PMACRO_BRICK_CTRL_RFU1) = (params->emc_pmacro_brick_ctrl_rfu1 & 0x1120112) | 0x1EED1EED;
	EMC(EMC_CONFIG_SAMPLE_DELAY) = params->emc_config_sample_delay;
	EMC(EMC_FBIO_CFG8) = params->emc_fbio_cfg8;
	EMC(EMC_SWIZZLE_RANK0_BYTE0) = params->emc_swizzle_rank0_byte0;
	EMC(EMC_SWIZZLE_RANK0_BYTE1) = params->emc_swizzle_rank0_byte1;
	EMC(EMC_SWIZZLE_RANK0_BYTE2) = params->emc_swizzle_rank0_byte2;
	EMC(EMC_SWIZZLE_RANK0_BYTE3) = params->emc_swizzle_rank0_byte3;
	EMC(EMC_SWIZZLE_RANK1_BYTE0) = params->emc_swizzle_rank1_byte0;
	EMC(EMC_SWIZZLE_RANK1_BYTE1) = params->emc_swizzle_rank1_byte1;
	EMC(EMC_SWIZZLE_RANK1_BYTE2) = params->emc_swizzle_rank1_byte2;
	EMC(EMC_SWIZZLE_RANK1_BYTE3) = params->emc_swizzle_rank1_byte3;
	if (params->emc_bct_spare6)
		*(vu32 *)params->emc_bct_spare6 = params->emc_bct_spare7;
	EMC(EMC_XM2COMPPADCTRL) = params->emc_xm2_comp_pad_ctrl;
	EMC(EMC_XM2COMPPADCTRL2) = params->emc_xm2_comp_pad_ctrl2;
	EMC(EMC_XM2COMPPADCTRL3) = params->emc_xm2_comp_pad_ctrl3;
	EMC(EMC_AUTO_CAL_CONFIG2) = params->emc_auto_cal_config2;
	EMC(EMC_AUTO_CAL_CONFIG3) = params->emc_auto_cal_config3;
	EMC(EMC_AUTO_CAL_CONFIG4) = params->emc_auto_cal_config4;
	EMC(EMC_AUTO_CAL_CONFIG5) = params->emc_auto_cal_config5;
	EMC(EMC_AUTO_CAL_CONFIG6) = params->emc_auto_cal_config6;
	EMC(EMC_AUTO_CAL_CONFIG7) = params->emc_auto_cal_config7;
	EMC(EMC_AUTO_CAL_CONFIG8) = params->emc_auto_cal_config8;
	EMC(EMC_PMACRO_RX_TERM) = params->emc_pmacro_rx_term;
	EMC(EMC_PMACRO_DQ_TX_DRV) = params->emc_pmacro_dq_tx_drive;
	EMC(EMC_PMACRO_CA_TX_DRV) = params->emc_pmacro_ca_tx_drive;
	EMC(EMC_PMACRO_CMD_TX_DRV) = params->emc_pmacro_cmd_tx_drive;
	EMC(EMC_PMACRO_AUTOCAL_CFG_COMMON) = params->emc_pmacro_auto_cal_common;
	EMC(EMC_AUTO_CAL_CHANNEL) = params->emc_auto_cal_channel;
	EMC(EMC_PMACRO_ZCTRL) = params->emc_pmacro_zcrtl;
	EMC(EMC_DLL_CFG_0) = params->emc_dll_cfg0;
	EMC(EMC_DLL_CFG_1) = params->emc_dll_cfg1;
	EMC(EMC_CFG_DIG_DLL_1) = params->emc_cfg_dig_dll_1;
	EMC(EMC_DATA_BRLSHFT_0) = params->emc_data_brlshft0;
	EMC(EMC_DATA_BRLSHFT_1) = params->emc_data_brlshft1;
	EMC(EMC_DQS_BRLSHFT_0) = params->emc_dqs_brlshft0;
	EMC(EMC_DQS_BRLSHFT_1) = params->emc_dqs_brlshft1;
	EMC(EMC_CMD_BRLSHFT_0) = params->emc_cmd_brlshft0;
	EMC(EMC_CMD_BRLSHFT_1) = params->emc_cmd_brlshft1;
	EMC(EMC_CMD_BRLSHFT_2) = params->emc_cmd_brlshft2;
	EMC(EMC_CMD_BRLSHFT_3) = params->emc_cmd_brlshft3;
	EMC(EMC_QUSE_BRLSHFT_0) = params->emc_quse_brlshft0;
	EMC(EMC_QUSE_BRLSHFT_1) = params->emc_quse_brlshft1;
	EMC(EMC_QUSE_BRLSHFT_2) = params->emc_quse_brlshft2;
	EMC(EMC_QUSE_BRLSHFT_3) = params->emc_quse_brlshft3;
	EMC(EMC_PMACRO_BRICK_CTRL_RFU1) = (params->emc_pmacro_brick_ctrl_rfu1 & 0x1BF01BF) | 0x1E401E40;
	EMC(EMC_PMACRO_PAD_CFG_CTRL) = params->emc_pmacro_pad_cfg_ctrl;
	EMC(EMC_PMACRO_CMD_BRICK_CTRL_FDPD) = params->emc_pmacro_cmd_brick_ctrl_fdpd;
	EMC(EMC_PMACRO_BRICK_CTRL_RFU2) = params->emc_pmacro_brick_ctrl_rfu2 & 0xFF7FFF7F;
	EMC(EMC_PMACRO_DATA_BRICK_CTRL_FDPD) = params->emc_pmacro_data_brick_ctrl_fdpd;
	EMC(EMC_PMACRO_BG_BIAS_CTRL_0) = params->emc_pmacro_bg_bias_ctrl0;
	EMC(EMC_PMACRO_DATA_PAD_RX_CTRL) = params->emc_pmacro_data_pad_rx_ctrl;
	EMC(EMC_PMACRO_CMD_PAD_RX_CTRL) = params->emc_pmacro_cmd_pad_rx_ctrl;
	EMC(EMC_PMACRO_DATA_PAD_TX_CTRL) = params->emc_pmacro_data_pad_tx_ctrl;
	EMC(EMC_PMACRO_DATA_RX_TERM_MODE) = params->emc_pmacro_data_rx_term_mode;
	EMC(EMC_PMACRO_CMD_RX_TERM_MODE) = params->emc_pmacro_cmd_rx_term_mode;
	EMC(EMC_PMACRO_CMD_PAD_TX_CTRL) = params->emc_pmacro_cmd_pad_tx_ctrl;
	EMC(EMC_CFG_3) = params->emc_cfg3;
	EMC(EMC_PMACRO_TX_PWRD_0) = params->emc_pmacro_tx_pwrd0;
	EMC(EMC_PMACRO_TX_PWRD_1) = params->emc_pmacro_tx_pwrd1;
	EMC(EMC_PMACRO_TX_PWRD_2) = params->emc_pmacro_tx_pwrd2;
	EMC(EMC_PMACRO_TX_PWRD_3) = params->emc_pmacro_tx_pwrd3;
	EMC(EMC_PMACRO_TX_PWRD_4) = params->emc_pmacro_tx_pwrd4;
	EMC(EMC_PMACRO_TX_PWRD_5) = params->emc_pmacro_tx_pwrd5;
	EMC(EMC_PMACRO_TX_SEL_CLK_SRC_0) = params->emc_pmacro_tx_sel_clk_src0;
	EMC(EMC_PMACRO_TX_SEL_CLK_SRC_1) = params->emc_pmacro_tx_sel_clk_src1;
	EMC(EMC_PMACRO_TX_SEL_CLK_SRC_2) = params->emc_pmacro_tx_sel_clk_src2;
	EMC(EMC_PMACRO_TX_SEL_CLK_SRC_3) = params->emc_pmacro_tx_sel_clk_src3;
	EMC(EMC_PMACRO_TX_SEL_CLK_SRC_4) = params->emc_pmacro_tx_sel_clk_src4;
	EMC(EMC_PMACRO_TX_SEL_CLK_SRC_5) = params->emc_pmacro_tx_sel_clk_src5;
	EMC(EMC_PMACRO_DDLL_BYPASS) = params->emc_pmacro_ddll_bypass;
	EMC(EMC_PMACRO_DDLL_PWRD_0) = params->emc_pmacro_ddll_pwrd0;
	EMC(EMC_PMACRO_DDLL_PWRD_1) = params->emc_pmacro_ddll_pwrd1;
	EMC(EMC_PMACRO_DDLL_PWRD_2) = params->emc_pmacro_ddll_pwrd2;
	EMC(EMC_PMACRO_CMD_CTRL_0) = params->emc_pmacro_cmd_ctrl0;
	EMC(EMC_PMACRO_CMD_CTRL_1) = params->emc_pmacro_cmd_ctrl1;
	EMC(EMC_PMACRO_CMD_CTRL_2) = params->emc_pmacro_cmd_ctrl2;
	EMC(EMC_PMACRO_IB_VREF_DQ_0) = params->emc_pmacro_ib_vref_dq_0;
	EMC(EMC_PMACRO_IB_VREF_DQ_1) = params->emc_pmacro_ib_vref_dq_1;
	EMC(EMC_PMACRO_IB_VREF_DQS_0) = params->emc_pmacro_ib_vref_dqs_0;
	EMC(EMC_PMACRO_IB_VREF_DQS_1) = params->emc_pmacro_ib_vref_dqs_1;
	EMC(EMC_PMACRO_IB_RXRT) = params->emc_pmacro_ib_rxrt;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK0_0) = params->emc_pmacro_quse_ddll_rank0_0;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK0_1) = params->emc_pmacro_quse_ddll_rank0_1;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK0_2) = params->emc_pmacro_quse_ddll_rank0_2;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK0_3) = params->emc_pmacro_quse_ddll_rank0_3;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK0_4) = params->emc_pmacro_quse_ddll_rank0_4;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK0_5) = params->emc_pmacro_quse_ddll_rank0_5;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK1_0) = params->emc_pmacro_quse_ddll_rank1_0;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK1_1) = params->emc_pmacro_quse_ddll_rank1_1;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK1_2) = params->emc_pmacro_quse_ddll_rank1_2;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK1_3) = params->emc_pmacro_quse_ddll_rank1_3;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK1_4) = params->emc_pmacro_quse_ddll_rank1_4;
	EMC(EMC_PMACRO_QUSE_DDLL_RANK1_5) = params->emc_pmacro_quse_ddll_rank1_5;
	EMC(EMC_PMACRO_BRICK_CTRL_RFU1) = params->emc_pmacro_brick_ctrl_rfu1;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0) = params->emc_pmacro_ob_ddll_long_dq_rank0_0;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1) = params->emc_pmacro_ob_ddll_long_dq_rank0_1;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2) = params->emc_pmacro_ob_ddll_long_dq_rank0_2;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3) = params->emc_pmacro_ob_ddll_long_dq_rank0_3;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4) = params->emc_pmacro_ob_ddll_long_dq_rank0_4;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5) = params->emc_pmacro_ob_ddll_long_dq_rank0_5;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0) = params->emc_pmacro_ob_ddll_long_dq_rank1_0;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1) = params->emc_pmacro_ob_ddll_long_dq_rank1_1;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2) = params->emc_pmacro_ob_ddll_long_dq_rank1_2;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3) = params->emc_pmacro_ob_ddll_long_dq_rank1_3;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_4) = params->emc_pmacro_ob_ddll_long_dq_rank1_4;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_5) = params->emc_pmacro_ob_ddll_long_dq_rank1_5;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_0) = params->emc_pmacro_ob_ddll_long_dqs_rank0_0;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_1) = params->emc_pmacro_ob_ddll_long_dqs_rank0_1;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_2) = params->emc_pmacro_ob_ddll_long_dqs_rank0_2;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_3) = params->emc_pmacro_ob_ddll_long_dqs_rank0_3;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_4) = params->emc_pmacro_ob_ddll_long_dqs_rank0_4;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_5) = params->emc_pmacro_ob_ddll_long_dqs_rank0_5;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_0) = params->emc_pmacro_ob_ddll_long_dqs_rank1_0;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_1) = params->emc_pmacro_ob_ddll_long_dqs_rank1_1;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_2) = params->emc_pmacro_ob_ddll_long_dqs_rank1_2;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_3) = params->emc_pmacro_ob_ddll_long_dqs_rank1_3;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_4) = params->emc_pmacro_ob_ddll_long_dqs_rank1_4;
	EMC(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_5) = params->emc_pmacro_ob_ddll_long_dqs_rank1_5;
	EMC(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0) = params->emc_pmacro_ib_ddll_long_dqs_rank0_0;
	EMC(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1) = params->emc_pmacro_ib_ddll_long_dqs_rank0_1;
	EMC(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2) = params->emc_pmacro_ib_ddll_long_dqs_rank0_2;
	EMC(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3) = params->emc_pmacro_ib_ddll_long_dqs_rank0_3;
	EMC(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0) = params->emc_pmacro_ib_ddll_long_dqs_rank1_0;
	EMC(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1) = params->emc_pmacro_ib_ddll_long_dqs_rank1_1;
	EMC(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2) = params->emc_pmacro_ib_ddll_long_dqs_rank1_2;
	EMC(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3) = params->emc_pmacro_ib_ddll_long_dqs_rank1_3;
	EMC(EMC_PMACRO_DDLL_LONG_CMD_0) = params->emc_pmacro_ddll_long_cmd_0;
	EMC(EMC_PMACRO_DDLL_LONG_CMD_1) = params->emc_pmacro_ddll_long_cmd_1;
	EMC(EMC_PMACRO_DDLL_LONG_CMD_2) = params->emc_pmacro_ddll_long_cmd_2;
	EMC(EMC_PMACRO_DDLL_LONG_CMD_3) = params->emc_pmacro_ddll_long_cmd_3;
	EMC(EMC_PMACRO_DDLL_LONG_CMD_4) = params->emc_pmacro_ddll_long_cmd_4;
	EMC(EMC_PMACRO_DDLL_SHORT_CMD_0) = params->emc_pmacro_ddll_short_cmd_0;
	EMC(EMC_PMACRO_DDLL_SHORT_CMD_1) = params->emc_pmacro_ddll_short_cmd_1;
	EMC(EMC_PMACRO_DDLL_SHORT_CMD_2) = params->emc_pmacro_ddll_short_cmd_2;
	EMC(EMC_PMACRO_COMMON_PAD_TX_CTRL) = (params->emc_pmacro_common_pad_tx_ctrl & 1) | 0xE;
	if (params->emc_bct_spare4)
		*(vu32 *)params->emc_bct_spare4 = params->emc_bct_spare5;
	EMC(EMC_TIMING_CONTROL) = 1;
	MC(MC_VIDEO_PROTECT_BOM) = params->mc_video_protect_bom;
	MC(MC_VIDEO_PROTECT_BOM_ADR_HI) = params->mc_video_protect_bom_adr_hi;
	MC(MC_VIDEO_PROTECT_SIZE_MB) = params->mc_video_protect_size_mb;
	MC(MC_VIDEO_PROTECT_VPR_OVERRIDE) = params->mc_video_protect_vpr_override;
	MC(MC_VIDEO_PROTECT_VPR_OVERRIDE1) = params->mc_video_protect_vpr_override1;
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_0) = params->mc_video_protect_gpu_override0;
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_1) = params->mc_video_protect_gpu_override1;
	MC(MC_EMEM_ADR_CFG) = params->mc_emem_adr_cfg;
	MC(MC_EMEM_ADR_CFG_DEV0) = params->mc_emem_adr_cfg_dev0;
	MC(MC_EMEM_ADR_CFG_DEV1) = params->mc_emem_adr_cfg_dev1;
	MC(MC_EMEM_ADR_CFG_CHANNEL_MASK) = params->mc_emem_adr_cfg_channel_mask;
	MC(MC_EMEM_ADR_CFG_BANK_MASK_0) = params->mc_emem_adr_cfg_bank_mask0;
	MC(MC_EMEM_ADR_CFG_BANK_MASK_1) = params->mc_emem_adr_cfg_bank_mask1;
	MC(MC_EMEM_ADR_CFG_BANK_MASK_2) = params->mc_emem_adr_cfg_bank_mask2;
	MC(MC_EMEM_CFG) = params->mc_emem_cfg;
	MC(MC_SEC_CARVEOUT_BOM) = params->mc_sec_carveout_bom;
	MC(MC_SEC_CARVEOUT_ADR_HI) = params->mc_sec_carveout_adr_hi;
	MC(MC_SEC_CARVEOUT_SIZE_MB) = params->mc_sec_carveout_size_mb;
	MC(MC_MTS_CARVEOUT_BOM) = params->mc_mts_carveout_bom;
	MC(MC_MTS_CARVEOUT_ADR_HI) = params->mc_mts_carveout_adr_hi;
	MC(MC_MTS_CARVEOUT_SIZE_MB) = params->mc_mts_carveout_size_mb;
	MC(MC_EMEM_ARB_CFG) = params->mc_emem_arb_cfg;
	MC(MC_EMEM_ARB_OUTSTANDING_REQ) = params->mc_emem_arb_outstanding_req;
	MC(MC_EMEM_ARB_REFPB_HP_CTRL) = params->emc_emem_arb_refpb_hp_ctrl;
	MC(MC_EMEM_ARB_REFPB_BANK_CTRL) = params->emc_emem_arb_refpb_bank_ctrl;
	MC(MC_EMEM_ARB_TIMING_RCD) = params->mc_emem_arb_timing_rcd;
	MC(MC_EMEM_ARB_TIMING_RP) = params->mc_emem_arb_timing_rp;
	MC(MC_EMEM_ARB_TIMING_RC) = params->mc_emem_arb_timing_rc;
	MC(MC_EMEM_ARB_TIMING_RAS) = params->mc_emem_arb_timing_ras;
	MC(MC_EMEM_ARB_TIMING_FAW) = params->mc_emem_arb_timing_faw;
	MC(MC_EMEM_ARB_TIMING_RRD) = params->mc_emem_arb_timing_rrd;
	MC(MC_EMEM_ARB_TIMING_RAP2PRE) = params->mc_emem_arb_timing_rap2pre;
	MC(MC_EMEM_ARB_TIMING_WAP2PRE) = params->mc_emem_arb_timing_wap2pre;
	MC(MC_EMEM_ARB_TIMING_R2R) = params->mc_emem_arb_timing_r2r;
	MC(MC_EMEM_ARB_TIMING_W2W) = params->mc_emem_arb_timing_w2w;
	MC(MC_EMEM_ARB_TIMING_CCDMW) = params->mc_emem_arb_timing_ccdmw;
	MC(MC_EMEM_ARB_TIMING_R2W) = params->mc_emem_arb_timing_r2w;
	MC(MC_EMEM_ARB_TIMING_W2R) = params->mc_emem_arb_timing_w2r;
	MC(MC_EMEM_ARB_TIMING_RFCPB) = params->mc_emem_arb_timing_rfcpb;
	MC(MC_EMEM_ARB_DA_TURNS) = params->mc_emem_arb_da_turns;
	MC(MC_EMEM_ARB_DA_COVERS) = params->mc_emem_arb_da_covers;
	MC(MC_EMEM_ARB_MISC0) = params->mc_emem_arb_misc0;
	MC(MC_EMEM_ARB_MISC1) = params->mc_emem_arb_misc1;
	MC(MC_EMEM_ARB_MISC2) = params->mc_emem_arb_misc2;
	MC(MC_EMEM_ARB_RING1_THROTTLE) = params->mc_emem_arb_ring1_throttle;
	MC(MC_EMEM_ARB_OVERRIDE) = params->mc_emem_arb_override;
	MC(MC_EMEM_ARB_OVERRIDE_1) = params->mc_emem_arb_override1;
	MC(MC_EMEM_ARB_RSV) = params->mc_emem_arb_rsv;
	MC(MC_DA_CONFIG0) = params->mc_da_cfg0;
	MC(MC_TIMING_CONTROL) = 1;
	MC(MC_CLKEN_OVERRIDE) = params->mc_clken_override;
	MC(MC_STAT_CONTROL) = params->mc_stat_control;
	EMC(EMC_ADR_CFG) = params->emc_adr_cfg;
	EMC(EMC_CLKEN_OVERRIDE) = params->emc_clken_override;
	EMC(EMC_PMACRO_AUTOCAL_CFG_0) = params->emc_pmacro_auto_cal_cfg0;
	EMC(EMC_PMACRO_AUTOCAL_CFG_1) = params->emc_pmacro_auto_cal_cfg1;
	EMC(EMC_PMACRO_AUTOCAL_CFG_2) = params->emc_pmacro_auto_cal_cfg2;
	EMC(EMC_AUTO_CAL_VREF_SEL_0) = params->emc_auto_cal_vref_sel0;
	EMC(EMC_AUTO_CAL_VREF_SEL_1) = params->emc_auto_cal_vref_sel1;
	EMC(EMC_AUTO_CAL_INTERVAL) = params->emc_auto_cal_interval;
	EMC(EMC_AUTO_CAL_CONFIG) = params->emc_auto_cal_config;
	sleep(params->emc_auto_cal_wait);
	if (params->emc_bct_spare8)
		*(vu32 *)params->emc_bct_spare8 = params->emc_bct_spare9;
	EMC(EMC_CFG_2) = params->emc_cfg2;
	EMC(EMC_CFG_PIPE) = params->emc_cfg_pipe;
	EMC(EMC_CFG_PIPE_1) = params->emc_cfg_pipe1;
	EMC(EMC_CFG_PIPE_2) = params->emc_cfg_pipe2;
	EMC(EMC_CMDQ) = params->emc_cmd_q;
	EMC(EMC_MC2EMCQ) = params->emc_mc2emc_q;
	EMC(EMC_MRS_WAIT_CNT) = params->emc_mrs_wait_cnt;
	EMC(EMC_MRS_WAIT_CNT2) = params->emc_mrs_wait_cnt2;
	EMC(EMC_FBIO_CFG5) = params->emc_fbio_cfg5;
	EMC(EMC_RC) = params->emc_rc;
	EMC(EMC_RFC) = params->emc_rfc;
	EMC(EMC_RFCPB) = params->emc_rfc_pb;
	EMC(EMC_REFCTRL2) = params->emc_ref_ctrl2;
	EMC(EMC_RFC_SLR) = params->emc_rfc_slr;
	EMC(EMC_RAS) = params->emc_ras;
	EMC(EMC_RP) = params->emc_rp;
	EMC(EMC_TPPD) = params->emc_tppd;
	EMC(EMC_R2R) = params->emc_r2r;
	EMC(EMC_W2W) = params->emc_w2w;
	EMC(EMC_R2W) = params->emc_r2w;
	EMC(EMC_W2R) = params->emc_w2r;
	EMC(EMC_R2P) = params->emc_r2p;
	EMC(EMC_W2P) = params->emc_w2p;
	EMC(EMC_CCDMW) = params->emc_ccdmw;
	EMC(EMC_RD_RCD) = params->emc_rd_rcd;
	EMC(EMC_WR_RCD) = params->emc_wr_rcd;
	EMC(EMC_RRD) = params->emc_rrd;
	EMC(EMC_REXT) = params->emc_rext;
	EMC(EMC_WEXT) = params->emc_wext;
	EMC(EMC_WDV) = params->emc_wdv;
	EMC(EMC_WDV_CHK) = params->emc_wdv_chk;
	EMC(EMC_WSV) = params->emc_wsv;
	EMC(EMC_WEV) = params->emc_wev;
	EMC(EMC_WDV_MASK) = params->emc_wdv_mask;
	EMC(EMC_WS_DURATION) = params->emc_ws_duration;
	EMC(EMC_WE_DURATION) = params->emc_we_duration;
	EMC(EMC_QUSE) = params->emc_quse;
	EMC(EMC_QUSE_WIDTH) = params->emc_quse_width;
	EMC(EMC_IBDLY) = params->emc_ibdly;
	EMC(EMC_OBDLY) = params->emc_obdly;
	EMC(EMC_EINPUT) = params->emc_einput;
	EMC(EMC_EINPUT_DURATION) = params->emc_einput_duration;
	EMC(EMC_PUTERM_EXTRA) = params->emc_puterm_extra;
	EMC(EMC_PUTERM_WIDTH) = params->emc_puterm_width;
	EMC(EMC_PMACRO_COMMON_PAD_TX_CTRL) = params->emc_pmacro_common_pad_tx_ctrl;
	EMC(EMC_DBG) = params->emc_dbg;
	EMC(EMC_QRST) = params->emc_qrst;
	EMC(EMC_ISSUE_QRST) = 0;
	EMC(EMC_QSAFE) = params->emc_qsafe;
	EMC(EMC_RDV) = params->emc_rdv;
	EMC(EMC_RDV_MASK) = params->emc_rdv_mask;
	EMC(EMC_RDV_EARLY) = params->emc_rdv_early;
	EMC(EMC_RDV_EARLY_MASK) = params->emc_rdv_early_mask;
	EMC(EMC_QPOP) = params->emc_qpop;
	EMC(EMC_REFRESH) = params->emc_refresh;
	EMC(EMC_BURST_REFRESH_NUM) = params->emc_burst_refresh_num;
	EMC(EMC_PRE_REFRESH_REQ_CNT) = params->emc_prerefresh_req_cnt;
	EMC(EMC_PDEX2WR) = params->emc_pdex2wr;
	EMC(EMC_PDEX2RD) = params->emc_pdex2rd;
	EMC(EMC_PCHG2PDEN) = params->emc_pchg2pden;
	EMC(EMC_ACT2PDEN) = params->emc_act2pden;
	EMC(EMC_AR2PDEN) = params->emc_ar2pden;
	EMC(EMC_RW2PDEN) = params->emc_rw2pden;
	EMC(EMC_CKE2PDEN) = params->emc_cke2pden;
	EMC(EMC_PDEX2CKE) = params->emc_pdex2che;
	EMC(EMC_PDEX2MRR) = params->emc_pdex2mrr;
	EMC(EMC_TXSR) = params->emc_txsr;
	EMC(EMC_TXSRDLL) = params->emc_txsr_dll;
	EMC(EMC_TCKE) = params->emc_tcke;
	EMC(EMC_TCKESR) = params->emc_tckesr;
	EMC(EMC_TPD) = params->emc_tpd;
	EMC(EMC_TFAW) = params->emc_tfaw;
	EMC(EMC_TRPAB) = params->emc_trpab;
	EMC(EMC_TCLKSTABLE) = params->emc_tclkstable;
	EMC(EMC_TCLKSTOP) = params->emc_tclkstop;
	EMC(EMC_TREFBW) = params->emc_trefbw;
	EMC(EMC_ODT_WRITE) = params->emc_odt_write;
	EMC(EMC_CFG_DIG_DLL) = params->emc_cfg_dig_dll;
	EMC(EMC_CFG_DIG_DLL_PERIOD) = params->emc_cfg_dig_dll_period;
	EMC(EMC_FBIO_SPARE) = params->emc_fbio_spare & 0xFFFFFFFD;
	EMC(EMC_CFG_RSV) = params->emc_cfg_rsv;
	EMC(EMC_PMC_SCRATCH1) = params->emc_pmc_scratch1;
	EMC(EMC_PMC_SCRATCH2) = params->emc_pmc_scratch2;
	EMC(EMC_PMC_SCRATCH3) = params->emc_pmc_scratch3;
	EMC(EMC_ACPD_CONTROL) = params->emc_acpd_control;
	EMC(EMC_TXDSRVTTGEN) = params->emc_txdsrvttgen;
	EMC(EMC_CFG) = (params->emc_cfg & 0xE) | 0x3C00000;
	if (params->boot_rom_patch_control & 0x80000000)
	{
		*(vu32 *)(4 * (params->boot_rom_patch_control + 0x1C000000)) = params->boot_rom_patch_data;
		MC(MC_TIMING_CONTROL) = 1;
	}
	PMC(0x45C) = ((4 * params->emc_pmc_scratch1 >> 2) + 0x40000000) & 0xCFFF0000;
	sleep(params->pmc_io_dpd3_req_wait);
	if (!params->emc_auto_cal_interval)
		EMC(EMC_AUTO_CAL_CONFIG) = params->emc_auto_cal_config | 0x200;
	EMC(EMC_PMACRO_BRICK_CTRL_RFU2) = params->emc_pmacro_brick_ctrl_rfu2;
	if (params->emc_zcal_warm_cold_boot_enables & 1)
	{
		if (params->memory_type == 2)
			EMC(EMC_ZCAL_WAIT_CNT) = 8 * params->emc_zcal_wait_cnt;
		if (params->memory_type == 3)
		{
			EMC(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt;
			EMC(EMC_ZCAL_MRW_CMD) = params->emc_zcal_mrw_cmd;
		}
	}
	EMC(EMC_TIMING_CONTROL) = 1;
	sleep(params->emc_timing_control_wait);
	PMC(0x4E4) &= 0xFFF8007F;
	sleep(params->pmc_ddr_ctrl_wait);
	if (params->memory_type == 2)
	{
		EMC(EMC_PIN) = (params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12);
		sleep(params->emc_pin_extra_wait + 200);
		EMC(EMC_PIN) = ((params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12)) + 256;
		sleep(params->emc_pin_extra_wait + 500);
	}
	if (params->memory_type == 3)
	{
		EMC(EMC_PIN) = (params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12);
		sleep(params->emc_pin_extra_wait + 200);
		EMC(EMC_PIN) = ((params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12)) + 256;
		sleep(params->emc_pin_extra_wait + 2000);
	}
	EMC(EMC_PIN) = ((params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12)) + 0x101;
	sleep(params->emc_pin_program_wait);
	if (params->memory_type != 3)
		EMC(EMC_NOP) = (params->emc_dev_select << 30) + 1;
	if (params->memory_type == 1)
		sleep(params->emc_pin_extra_wait + 200);
	if (params->memory_type == 3)
	{
		if (params->emc_bct_spare10)
			*(vu32 *)params->emc_bct_spare10 = params->emc_bct_spare11;
		EMC(EMC_MRW2) = params->emc_mrw2;
		EMC(EMC_MRW) = params->emc_mrw1;
		EMC(EMC_MRW3) = params->emc_mrw3;
		EMC(EMC_MRW4) = params->emc_mrw4;
		EMC(EMC_MRW6) = params->emc_mrw6;
		EMC(EMC_MRW14) = params->emc_mrw14;
		EMC(EMC_MRW8) = params->emc_mrw8;
		EMC(EMC_MRW12) = params->emc_mrw12;
		EMC(EMC_MRW9) = params->emc_mrw9;
		EMC(EMC_MRW13) = params->emc_mrw13;
		if (params->emc_zcal_warm_cold_boot_enables & 1)
		{
			EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev0;
			sleep(params->emc_zcal_init_wait);
			EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev0 ^ 3;
			if (!(params->emc_dev_select & 2))
			{
				EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev1;
				sleep(params->emc_zcal_init_wait);
				EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev1 ^ 3;
			}
		}
	}
	PMC(0x1D0) = params->pmc_ddr_cfg;
	if (params->memory_type - 1 <= 2)
	{
		EMC(EMC_ZCAL_INTERVAL) = params->emc_zcal_interval;
		EMC(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt;
		EMC(EMC_ZCAL_MRW_CMD) = params->emc_zcal_mrw_cmd;
	}
	if (params->emc_bct_spare12)
		*(vu32 *)params->emc_bct_spare12 = params->emc_bct_spare13;
	EMC(EMC_TIMING_CONTROL) = 1;
	if (params->emc_extra_refresh_num)
		EMC(EMC_REF) = ((1 << params->emc_extra_refresh_num << 8) - 0xFD) | (params->emc_pin_gpio << 30);
	EMC(EMC_REFCTRL) = params->emc_dev_select | 0x80000000;
	EMC(EMC_DYN_SELF_REF_CONTROL) = params->emc_dyn_self_ref_control;
	EMC(EMC_CFG_UPDATE) = params->emc_cfg_update;
	EMC(EMC_CFG) = params->emc_cfg;
	EMC(EMC_FDPD_CTRL_DQ) = params->emc_fdpd_ctrl_dq;
	EMC(EMC_FDPD_CTRL_CMD) = params->emc_fdpd_ctrl_cmd;
	EMC(EMC_SEL_DPD_CTRL) = params->emc_sel_dpd_ctrl;
	EMC(EMC_FBIO_SPARE) = params->emc_fbio_spare | 2;
	EMC(EMC_TIMING_CONTROL) = 1;
	EMC(EMC_CFG_PIPE_CLK) = params->emc_cfg_pipe_clk;
	EMC(EMC_FDPD_CTRL_CMD_NO_RAMP) = params->emc_fdpd_ctrl_cmd_no_ramp;
	SYSREG(AHB_ARBITRATION_XBAR_CTRL) = (SYSREG(AHB_ARBITRATION_XBAR_CTRL) & 0xFFFEFFFF) | ((params->ahb_arbitration_xbar_ctrl_meminit_done & 0xFFFF) << 16);
	MC(MC_VIDEO_PROTECT_REG_CTRL) = params->mc_video_protect_write_access;
	MC(MC_SEC_CARVEOUT_REG_CTRL) = params->mc_sec_carveout_protect_write_access;
	MC(MC_MTS_CARVEOUT_REG_CTRL) = params->mc_mts_carveout_reg_ctrl;
	MC(MC_EMEM_CFG_ACCESS_CTRL) = 1; //Disable write access to a bunch of MC registers.
}

const void *sdram_get_params()
{
	//TODO: sdram_id should be in [0, 7].

#ifdef CONFIG_SDRAM_COMPRESS_CFG
	u8 *buf = (u8 *)0x40030000;
	LZ_Uncompress(_dram_cfg_lz, buf, sizeof(_dram_cfg_lz));
	return (const void *)&buf[sizeof(sdram_params_t) * _get_sdram_id()];
#else
	return _dram_cfgs[_get_sdram_id()];
#endif
}

void sdram_init()
{
	//TODO: sdram_id should be in [0,4].
	const sdram_params_t *params = (const sdram_params_t *)sdram_get_params();

	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_SD_CFG2, 0x05);
	i2c_send_byte(I2C_5, 0x3C, MAX77620_REG_SD1, 40); //40 = (1000 * 1100 - 600000) / 12500 -> 1.1V

	PMC(APBDEV_PMC_VDDP_SEL) = params->pmc_vddp_sel;
	sleep(params->pmc_vddp_sel_wait);
	PMC(APBDEV_PMC_DDR_PWR) = PMC(APBDEV_PMC_DDR_PWR);
	PMC(APBDEV_PMC_NO_IOPOWER) = params->pmc_no_io_power;
	PMC(APBDEV_PMC_REG_SHORT) = params->pmc_reg_short;
	PMC(APBDEV_PMC_DDR_CNTRL) = params->pmc_ddr_ctrl;

	if (params->emc_bct_spare0)
		*(vu32 *)params->emc_bct_spare0 = params->emc_bct_spare1;

	_sdram_config(params);
}
