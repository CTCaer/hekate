/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 balika011
 * Copyright (c) 2019-2021 CTCaer
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

#include <string.h>

#include <mem/mc.h>
#include <mem/emc.h>
#include <mem/sdram.h>
#include <mem/sdram_param_t210.h>
#include <mem/sdram_param_t210b01.h>
#include <memory_map.h>
#include <power/max77620.h>
#include <power/max7762x.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <utils/util.h>

#define CONFIG_SDRAM_KEEP_ALIVE

typedef struct _sdram_vendor_patch_t
{
	u32 val;
	u32 offset:16;
	u32 dramcf:16;
} sdram_vendor_patch_t;

static const u8 dram_encoding_t210b01[] = {
	LPDDR4X_UNUSED,
	LPDDR4X_UNUSED,
	LPDDR4X_UNUSED,
	LPDDR4X_4GB_HYNIX_1Y_A,
	LPDDR4X_UNUSED,
	LPDDR4X_4GB_HYNIX_1Y_A,
	LPDDR4X_4GB_HYNIX_1Y_A,
	LPDDR4X_4GB_SAMSUNG_X1X2,
	LPDDR4X_NO_PATCH,
	LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ,
	LPDDR4X_NO_PATCH,
	LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE,
	LPDDR4X_NO_PATCH,
	LPDDR4X_8GB_SAMSUNG_K4UBE3D4AM_MGCJ,
	LPDDR4X_NO_PATCH,
	LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTE,
	LPDDR4X_4GB_SAMSUNG_Y,
	LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL,
	LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL,
	LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL,
	LPDDR4X_4GB_SAMSUNG_1Y_Y,
	LPDDR4X_8GB_SAMSUNG_1Y_Y,
	LPDDR4X_UNUSED, // Removed.
	LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL,
	LPDDR4X_4GB_SAMSUNG_K4U6E3S4AA_MGCL,
	LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF,
	LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF,
	LPDDR4X_4GB_MICRON_MT53E512M32D2NP_046_WTF,
	LPDDR4X_8GB_SAMSUNG_K4UBE3D4AA_MGCL,
};

#include "sdram_config.inl"
#include "sdram_config_t210b01.inl"

static bool _sdram_wait_emc_status(u32 reg_offset, u32 bit_mask, bool updated_state, s32 emc_channel)
{
	bool err = true;

	for (s32 i = 0; i < EMC_STATUS_UPDATE_TIMEOUT; i++)
	{
		if (emc_channel)
		{
			if (emc_channel != 1)
				goto done;

			if (((EMC_CH1(reg_offset) & bit_mask) != 0) == updated_state)
			{
				err = false;
				break;
			}
		}
		else if (((EMC(reg_offset) & bit_mask) != 0) == updated_state)
		{
			err = false;
			break;
		}
		usleep(1);
	}

done:
	return err;
}

static void _sdram_req_mrr_data(u32 data, bool dual_channel)
{
	EMC(EMC_MRR) = data;
	_sdram_wait_emc_status(EMC_EMC_STATUS, EMC_STATUS_MRR_DIVLD, true, EMC_CHAN0);
	if (dual_channel)
		_sdram_wait_emc_status(EMC_EMC_STATUS, EMC_STATUS_MRR_DIVLD, true, EMC_CHAN1);
}

emc_mr_data_t sdram_read_mrx(emc_mr_t mrx)
{
	emc_mr_data_t data;

	/*
	 * When a dram chip has only one rank, then the info from the 2 ranks differs.
	 * Info not matching is only allowed on different channels.
	 */

	// Get Device 0 (Rank 0) info from both dram chips (channels).
	_sdram_req_mrr_data(BIT(31) | (mrx << 16), EMC_CHAN0);
	data.rank0_ch0 = EMC(EMC_MRR) & 0xFF;
	data.rank0_ch1 = (EMC(EMC_MRR) & 0xFF00 >> 8);

	// Get Device 1 (Rank 1) info from both dram chips (channels).
	_sdram_req_mrr_data(BIT(30) | (mrx << 16), EMC_CHAN1);
	data.rank1_ch0 = EMC(EMC_MRR) & 0xFF;
	data.rank1_ch1 = (EMC(EMC_MRR) & 0xFF00 >> 8);

	return data;
}

static void _sdram_config_t210(const sdram_params_t210_t *params)
{
	// Program DPD3/DPD4 regs (coldboot path).
	// Enable sel_dpd on unused pins.
	u32 dpd_req = (params->emc_pmc_scratch1 & 0x3FFFFFFF) | 0x80000000;
	PMC(APBDEV_PMC_IO_DPD3_REQ) = (dpd_req ^ 0xFFFF) & 0xC000FFFF;
	usleep(params->pmc_io_dpd3_req_wait);

	// Disable e_dpd_vttgen.
	dpd_req = (params->emc_pmc_scratch2 & 0x3FFFFFFF) | 0x80000000;
	PMC(APBDEV_PMC_IO_DPD4_REQ) = (dpd_req & 0xFFFF0000) ^ 0x3FFF0000;
	usleep(params->pmc_io_dpd4_req_wait);

	// Disable e_dpd_bg.
	PMC(APBDEV_PMC_IO_DPD4_REQ) = (dpd_req ^ 0xFFFF) & 0xC000FFFF;
	usleep(params->pmc_io_dpd4_req_wait);

	PMC(APBDEV_PMC_WEAK_BIAS) = 0;
	usleep(1);

	// Start clocks.
	CLOCK(CLK_RST_CONTROLLER_PLLM_MISC1) = params->pllm_setup_control;
	CLOCK(CLK_RST_CONTROLLER_PLLM_MISC2) = 0;

#ifdef CONFIG_SDRAM_KEEP_ALIVE
	CLOCK(CLK_RST_CONTROLLER_PLLM_BASE) =
		(params->pllm_feedback_divider << 8) | params->pllm_input_divider | ((params->pllm_post_divider & 0xFFFF) << 20) | PLLCX_BASE_ENABLE;
#else
	u32 pllm_div = (params->pllm_feedback_divider << 8) | params->pllm_input_divider | ((params->pllm_post_divider & 0xFFFF) << 20);
	CLOCK(CLK_RST_CONTROLLER_PLLM_BASE) = pllm_div;
	CLOCK(CLK_RST_CONTROLLER_PLLM_BASE) = pllm_div | PLLCX_BASE_ENABLE;
#endif

	u32 wait_end = get_tmr_us() + 300;
	while (!(CLOCK(CLK_RST_CONTROLLER_PLLM_BASE) & 0x8000000))
	{
		if (get_tmr_us() >= wait_end)
			goto break_nosleep;
	}
	usleep(10);

break_nosleep:
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) = ((params->mc_emem_arb_misc0 >> 11) & 0x10000) | (params->emc_clock_source & 0xFFFEFFFF);
	if (params->emc_clock_source_dll)
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL) = params->emc_clock_source_dll;
	if (params->clear_clock2_mc1)
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_CLR) = 0x40000000; // Clear Reset to MC1.

	// Enable and clear reset for memory clocks.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = BIT(CLK_H_EMC) | BIT(CLK_H_MEM);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) = BIT(CLK_X_EMC_DLL);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_CLR) = BIT(CLK_H_EMC) | BIT(CLK_H_MEM);

	// Set pad macros.
	EMC(EMC_PMACRO_VTTGEN_CTRL_0) = params->emc_pmacro_vttgen_ctrl0;
	EMC(EMC_PMACRO_VTTGEN_CTRL_1) = params->emc_pmacro_vttgen_ctrl1;
	EMC(EMC_PMACRO_VTTGEN_CTRL_2) = params->emc_pmacro_vttgen_ctrl2;

	EMC(EMC_TIMING_CONTROL) = 1; // Trigger timing update so above writes take place.
	usleep(10); // Ensure the regulators settle.

	// Select EMC write mux.
	EMC(EMC_DBG) = (params->emc_dbg_write_mux << 1) | params->emc_dbg;

	// Patch 2 using BCT spare variables.
	if (params->emc_bct_spare2)
		*(vu32 *)params->emc_bct_spare2 = params->emc_bct_spare3;

	// Program CMD mapping. Required before brick mapping, else
	// we can't guarantee CK will be differential at all times.
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

	// Program brick mapping.
	EMC(EMC_PMACRO_BRICK_MAPPING_0) = params->emc_pmacro_brick_mapping0;
	EMC(EMC_PMACRO_BRICK_MAPPING_1) = params->emc_pmacro_brick_mapping1;
	EMC(EMC_PMACRO_BRICK_MAPPING_2) = params->emc_pmacro_brick_mapping2;

	EMC(EMC_PMACRO_BRICK_CTRL_RFU1) = (params->emc_pmacro_brick_ctrl_rfu1 & 0x1120112) | 0x1EED1EED;

	// This is required to do any reads from the pad macros.
	EMC(EMC_CONFIG_SAMPLE_DELAY) = params->emc_config_sample_delay;

	EMC(EMC_FBIO_CFG8) = params->emc_fbio_cfg8;

	// Set swizzle for Rank 0.
	EMC(EMC_SWIZZLE_RANK0_BYTE0) = params->emc_swizzle_rank0_byte0;
	EMC(EMC_SWIZZLE_RANK0_BYTE1) = params->emc_swizzle_rank0_byte1;
	EMC(EMC_SWIZZLE_RANK0_BYTE2) = params->emc_swizzle_rank0_byte2;
	EMC(EMC_SWIZZLE_RANK0_BYTE3) = params->emc_swizzle_rank0_byte3;
	// Set swizzle for Rank 1.
	EMC(EMC_SWIZZLE_RANK1_BYTE0) = params->emc_swizzle_rank1_byte0;
	EMC(EMC_SWIZZLE_RANK1_BYTE1) = params->emc_swizzle_rank1_byte1;
	EMC(EMC_SWIZZLE_RANK1_BYTE2) = params->emc_swizzle_rank1_byte2;
	EMC(EMC_SWIZZLE_RANK1_BYTE3) = params->emc_swizzle_rank1_byte3;

	// Patch 3 using BCT spare variables.
	if (params->emc_bct_spare6)
		*(vu32 *)params->emc_bct_spare6 = params->emc_bct_spare7;

	// Set pad controls.
	EMC(EMC_XM2COMPPADCTRL) = params->emc_xm2_comp_pad_ctrl;
	EMC(EMC_XM2COMPPADCTRL2) = params->emc_xm2_comp_pad_ctrl2;
	EMC(EMC_XM2COMPPADCTRL3) = params->emc_xm2_comp_pad_ctrl3;

	// Program Autocal controls with shadowed register fields.
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

	// Common pad macro (cpm).
	EMC(EMC_PMACRO_COMMON_PAD_TX_CTRL) = (params->emc_pmacro_common_pad_tx_ctrl & 1) | 0xE;

	// Patch 4 using BCT spare variables.
	if (params->emc_bct_spare4)
		*(vu32 *)params->emc_bct_spare4 = params->emc_bct_spare5;

	EMC(EMC_TIMING_CONTROL) = 1; // Trigger timing update so above writes take place.

	// Initialize MC VPR settings.
	MC(MC_VIDEO_PROTECT_BOM) = params->mc_video_protect_bom;
	MC(MC_VIDEO_PROTECT_BOM_ADR_HI) = params->mc_video_protect_bom_adr_hi;
	MC(MC_VIDEO_PROTECT_SIZE_MB) = params->mc_video_protect_size_mb;
	MC(MC_VIDEO_PROTECT_VPR_OVERRIDE) = params->mc_video_protect_vpr_override;
	MC(MC_VIDEO_PROTECT_VPR_OVERRIDE1) = params->mc_video_protect_vpr_override1;
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_0) = params->mc_video_protect_gpu_override0;
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_1) = params->mc_video_protect_gpu_override1;

	// Program SDRAM geometry parameters.
	MC(MC_EMEM_ADR_CFG) = params->mc_emem_adr_cfg;
	MC(MC_EMEM_ADR_CFG_DEV0) = params->mc_emem_adr_cfg_dev0;
	MC(MC_EMEM_ADR_CFG_DEV1) = params->mc_emem_adr_cfg_dev1;
	MC(MC_EMEM_ADR_CFG_CHANNEL_MASK) = params->mc_emem_adr_cfg_channel_mask;

	// Program bank swizzling.
	MC(MC_EMEM_ADR_CFG_BANK_MASK_0) = params->mc_emem_adr_cfg_bank_mask0;
	MC(MC_EMEM_ADR_CFG_BANK_MASK_1) = params->mc_emem_adr_cfg_bank_mask1;
	MC(MC_EMEM_ADR_CFG_BANK_MASK_2) = params->mc_emem_adr_cfg_bank_mask2;

	// Program external memory aperture (base and size).
	MC(MC_EMEM_CFG) = params->mc_emem_cfg;

	// Program SEC carveout (base and size).
	MC(MC_SEC_CARVEOUT_BOM) = params->mc_sec_carveout_bom;
	MC(MC_SEC_CARVEOUT_ADR_HI) = params->mc_sec_carveout_adr_hi;
	MC(MC_SEC_CARVEOUT_SIZE_MB) = params->mc_sec_carveout_size_mb;

	// Program MTS carveout (base and size).
	MC(MC_MTS_CARVEOUT_BOM) = params->mc_mts_carveout_bom;
	MC(MC_MTS_CARVEOUT_ADR_HI) = params->mc_mts_carveout_adr_hi;
	MC(MC_MTS_CARVEOUT_SIZE_MB) = params->mc_mts_carveout_size_mb;

	// Program the memory arbiter.
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

	MC(MC_TIMING_CONTROL) = 1; // Trigger MC timing update.

	// Program second-level clock enable overrides.
	MC(MC_CLKEN_OVERRIDE) = params->mc_clken_override;

	// Program statistics gathering.
	MC(MC_STAT_CONTROL) = params->mc_stat_control;

	// Program SDRAM geometry parameters.
	EMC(EMC_ADR_CFG) = params->emc_adr_cfg;

	// Program second-level clock enable overrides.
	EMC(EMC_CLKEN_OVERRIDE) = params->emc_clken_override;

	// Program EMC pad auto calibration.
	EMC(EMC_PMACRO_AUTOCAL_CFG_0) = params->emc_pmacro_auto_cal_cfg0;
	EMC(EMC_PMACRO_AUTOCAL_CFG_1) = params->emc_pmacro_auto_cal_cfg1;
	EMC(EMC_PMACRO_AUTOCAL_CFG_2) = params->emc_pmacro_auto_cal_cfg2;

	EMC(EMC_AUTO_CAL_VREF_SEL_0) = params->emc_auto_cal_vref_sel0;
	EMC(EMC_AUTO_CAL_VREF_SEL_1) = params->emc_auto_cal_vref_sel1;

	EMC(EMC_AUTO_CAL_INTERVAL) = params->emc_auto_cal_interval;
	EMC(EMC_AUTO_CAL_CONFIG) = params->emc_auto_cal_config;
	usleep(params->emc_auto_cal_wait);

	// Patch 5 using BCT spare variables.
	if (params->emc_bct_spare8)
		*(vu32 *)params->emc_bct_spare8 = params->emc_bct_spare9;

	// Program EMC timing configuration.
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
	EMC(EMC_ISSUE_QRST) = 1;
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

	// Don't write CFG_ADR_EN (bit 1) here - lock bit written later.
	EMC(EMC_FBIO_SPARE) = params->emc_fbio_spare & 0xFFFFFFFD;
	EMC(EMC_CFG_RSV) = params->emc_cfg_rsv;
	EMC(EMC_PMC_SCRATCH1) = params->emc_pmc_scratch1;
	EMC(EMC_PMC_SCRATCH2) = params->emc_pmc_scratch2;
	EMC(EMC_PMC_SCRATCH3) = params->emc_pmc_scratch3;
	EMC(EMC_ACPD_CONTROL) = params->emc_acpd_control;
	EMC(EMC_TXDSRVTTGEN) = params->emc_txdsrvttgen;

	// Set pipe bypass enable bits before sending any DRAM commands.
	EMC(EMC_CFG) = (params->emc_cfg & 0xE) | 0x3C00000;

	// Patch BootROM.
	if (params->boot_rom_patch_control & BIT(31))
	{
		*(vu32 *)(APB_MISC_BASE + params->boot_rom_patch_control * 4) = params->boot_rom_patch_data;
		MC(MC_TIMING_CONTROL) = 1; // Trigger MC timing update.
	}

	// Release SEL_DPD_CMD.
	PMC(APBDEV_PMC_IO_DPD3_REQ) = ((params->emc_pmc_scratch1 & 0x3FFFFFFF) | 0x40000000) & 0xCFFF0000;
	usleep(params->pmc_io_dpd3_req_wait);

	// Set autocal interval if not configured.
	if (!params->emc_auto_cal_interval)
		EMC(EMC_AUTO_CAL_CONFIG) = params->emc_auto_cal_config | 0x200;

	EMC(EMC_PMACRO_BRICK_CTRL_RFU2) = params->emc_pmacro_brick_ctrl_rfu2;

	// ZQ CAL setup (not actually issuing ZQ CAL now).
	if (params->emc_zcal_warm_cold_boot_enables & 1)
	{
		if (params->memory_type == MEMORY_TYPE_DDR3L)
			EMC(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt << 3;
		if (params->memory_type == MEMORY_TYPE_LPDDR4)
		{
			EMC(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt;
			EMC(EMC_ZCAL_MRW_CMD) = params->emc_zcal_mrw_cmd;
		}
	}

	EMC(EMC_TIMING_CONTROL) = 1; // Trigger timing update so above writes take place.
	usleep(params->emc_timing_control_wait);

	// Deassert HOLD_CKE_LOW.
	PMC(APBDEV_PMC_DDR_CNTRL) &= 0xFFF8007F;
	usleep(params->pmc_ddr_ctrl_wait);

	// Set clock enable signal.
	u32 pin_gpio_cfg = (params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12);
	if (params->memory_type == MEMORY_TYPE_DDR3L || params->memory_type == MEMORY_TYPE_LPDDR4)
	{
		EMC(EMC_PIN) = pin_gpio_cfg;
		(void)EMC(EMC_PIN);
		usleep(params->emc_pin_extra_wait + 200);
		EMC(EMC_PIN) = pin_gpio_cfg | 0x100;
		(void)EMC(EMC_PIN);
	}

	if (params->memory_type == MEMORY_TYPE_LPDDR4)
		usleep(params->emc_pin_extra_wait + 2000);
	else if (params->memory_type == MEMORY_TYPE_DDR3L)
		usleep(params->emc_pin_extra_wait + 500);

	// Enable clock enable signal.
	EMC(EMC_PIN) = pin_gpio_cfg | 0x101;
	(void)EMC(EMC_PIN);
	usleep(params->emc_pin_program_wait);

	// Send NOP (trigger just needs to be non-zero).
	if (params->memory_type != MEMORY_TYPE_LPDDR4)
		EMC(EMC_NOP) = (params->emc_dev_select << 30) + 1;

	// On coldboot w/LPDDR2/3, wait 200 uSec after asserting CKE high.
	if (params->memory_type == MEMORY_TYPE_LPDDR2)
		usleep(params->emc_pin_extra_wait + 200);

	// Init zq calibration,
	if (params->memory_type == MEMORY_TYPE_LPDDR4)
	{
		// Patch 6 using BCT spare variables.
		if (params->emc_bct_spare10)
			*(vu32 *)params->emc_bct_spare10 = params->emc_bct_spare11;

		// Write mode registers.
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
			// Issue ZQCAL start, device 0.
			EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev0;
			usleep(params->emc_zcal_init_wait);

			// Issue ZQCAL latch.
			EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev0 ^ 3;
			// Same for device 1.
			if (!(params->emc_dev_select & 2))
			{
				EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev1;
				usleep(params->emc_zcal_init_wait);
				EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev1 ^ 3;
			}
		}
	}

	// Set package and DPD pad control.
	PMC(APBDEV_PMC_DDR_CFG) = params->pmc_ddr_cfg;

	// Start periodic ZQ calibration (LPDDRx only).
	if (params->memory_type && params->memory_type <= MEMORY_TYPE_LPDDR4)
	{
		EMC(EMC_ZCAL_INTERVAL) = params->emc_zcal_interval;
		EMC(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt;
		EMC(EMC_ZCAL_MRW_CMD) = params->emc_zcal_mrw_cmd;
	}

	// Patch 7 using BCT spare variables.
	if (params->emc_bct_spare12)
		*(vu32 *)params->emc_bct_spare12 = params->emc_bct_spare13;

	EMC(EMC_TIMING_CONTROL) = 1; // Trigger timing update so above writes take place.

	if (params->emc_extra_refresh_num)
		EMC(EMC_REF) = (((1 << params->emc_extra_refresh_num) - 1) << 8) | (params->emc_dev_select << 30) | 3;

	// Enable refresh.
	EMC(EMC_REFCTRL) = params->emc_dev_select | 0x80000000;

	EMC(EMC_DYN_SELF_REF_CONTROL) = params->emc_dyn_self_ref_control;
	EMC(EMC_CFG_UPDATE) = params->emc_cfg_update;
	EMC(EMC_CFG) = params->emc_cfg;
	EMC(EMC_FDPD_CTRL_DQ) = params->emc_fdpd_ctrl_dq;
	EMC(EMC_FDPD_CTRL_CMD) = params->emc_fdpd_ctrl_cmd;
	EMC(EMC_SEL_DPD_CTRL) = params->emc_sel_dpd_ctrl;

	// Write addr swizzle lock bit.
	EMC(EMC_FBIO_SPARE) = params->emc_fbio_spare | 2;

	EMC(EMC_TIMING_CONTROL) = 1; // Re-trigger timing to latch power saving functions.

	// Enable EMC pipe clock gating.
	EMC(EMC_CFG_PIPE_CLK) = params->emc_cfg_pipe_clk;

	// Depending on freqency, enable CMD/CLK fdpd.
	EMC(EMC_FDPD_CTRL_CMD_NO_RAMP) = params->emc_fdpd_ctrl_cmd_no_ramp;

	// Enable arbiter.
	SYSREG(AHB_ARBITRATION_XBAR_CTRL) = (SYSREG(AHB_ARBITRATION_XBAR_CTRL) & 0xFFFEFFFF) | (params->ahb_arbitration_xbar_ctrl_meminit_done << 16);

	// Lock carveouts per BCT cfg.
	MC(MC_VIDEO_PROTECT_REG_CTRL) = params->mc_video_protect_write_access;
	MC(MC_SEC_CARVEOUT_REG_CTRL) = params->mc_sec_carveout_protect_write_access;
	MC(MC_MTS_CARVEOUT_REG_CTRL) = params->mc_mts_carveout_reg_ctrl;

	// Disable write access to a bunch of EMC registers.
	MC(MC_EMEM_CFG_ACCESS_CTRL) = 1;
}

static void _sdram_config_t210b01(const sdram_params_t210b01_t *params)
{
	u32 pmc_scratch1 = ~params->emc_pmc_scratch1;
	u32 pmc_scratch2 = ~params->emc_pmc_scratch2;

	// Override HW FSM if needed.
	if (params->clk_rst_pllm_misc20_override_enable)
		CLOCK(CLK_RST_CONTROLLER_PLLM_MISC2) = params->clk_rst_pllm_misc20_override;

	// Program DPD3/DPD4 regs (coldboot path).
	// Enable sel_dpd on unused pins.
	PMC(APBDEV_PMC_WEAK_BIAS) = (pmc_scratch1 & 0x1000) << 19 | (pmc_scratch1 & 0xFFF) << 18 | (pmc_scratch1 & 0x8000) << 15;
	PMC(APBDEV_PMC_IO_DPD3_REQ) = (pmc_scratch1 & 0x9FFF) + 0x80000000;
	usleep(params->pmc_io_dpd3_req_wait);

	// Disable e_dpd_vttgen.
	PMC(APBDEV_PMC_IO_DPD4_REQ) = (pmc_scratch2 & 0x3FFF0000) | 0x80000000;
	usleep(params->pmc_io_dpd4_req_wait);

	// Disable e_dpd_bg.
	PMC(APBDEV_PMC_IO_DPD4_REQ) = (pmc_scratch2 & 0x1FFF) | 0x80000000;
	usleep(1);

	// Program CMD mapping. Required before brick mapping, else
	// we can't guarantee CK will be differential at all times.
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

	// Program brick mapping.
	EMC(EMC_PMACRO_BRICK_MAPPING_0) = params->emc_pmacro_brick_mapping0;
	EMC(EMC_PMACRO_BRICK_MAPPING_1) = params->emc_pmacro_brick_mapping1;
	EMC(EMC_PMACRO_BRICK_MAPPING_2) = params->emc_pmacro_brick_mapping2;

	// Set pad macros.
	EMC(EMC_PMACRO_VTTGEN_CTRL_0) = params->emc_pmacro_vttgen_ctrl0;
	EMC(EMC_PMACRO_VTTGEN_CTRL_1) = params->emc_pmacro_vttgen_ctrl1;
	EMC(EMC_PMACRO_VTTGEN_CTRL_2) = params->emc_pmacro_vttgen_ctrl2;

	// Set pad macros bias.
	EMC(EMC_PMACRO_BG_BIAS_CTRL_0) = params->emc_pmacro_bg_bias_ctrl0;

	// Patch 1 to 3 using BCT spare secure variables.
	if (params->emc_bct_spare_secure0)
		*(vu32 *)params->emc_bct_spare_secure0 = params->emc_bct_spare_secure1;
	if (params->emc_bct_spare_secure2)
		*(vu32 *)params->emc_bct_spare_secure2 = params->emc_bct_spare_secure3;
	if (params->emc_bct_spare_secure4)
		*(vu32 *)params->emc_bct_spare_secure4 = params->emc_bct_spare_secure5;

	EMC(EMC_TIMING_CONTROL) = 1; // Trigger timing update so above writes take place.
	usleep(params->pmc_vddp_sel_wait + 2); // Ensure the regulators settle.

	// Set clock sources.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) = params->emc_clock_source;
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL) = params->emc_clock_source_dll;

	// Select EMC write mux.
	EMC(EMC_DBG) = (params->emc_dbg_write_mux << 1) | params->emc_dbg;

	// Patch 2 using BCT spare variables.
	if (params->emc_bct_spare2)
		*(vu32 *)params->emc_bct_spare2 = params->emc_bct_spare3;

	// This is required to do any reads from the pad macros.
	EMC(EMC_CONFIG_SAMPLE_DELAY) = params->emc_config_sample_delay;

	EMC(EMC_FBIO_CFG8) = params->emc_fbio_cfg8;

	// Set swizzle for Rank 0.
	EMC(EMC_SWIZZLE_RANK0_BYTE0) = params->emc_swizzle_rank0_byte0;
	EMC(EMC_SWIZZLE_RANK0_BYTE1) = params->emc_swizzle_rank0_byte1;
	EMC(EMC_SWIZZLE_RANK0_BYTE2) = params->emc_swizzle_rank0_byte2;
	EMC(EMC_SWIZZLE_RANK0_BYTE3) = params->emc_swizzle_rank0_byte3;
	// Set swizzle for Rank 1.
	EMC(EMC_SWIZZLE_RANK1_BYTE0) = params->emc_swizzle_rank1_byte0;
	EMC(EMC_SWIZZLE_RANK1_BYTE1) = params->emc_swizzle_rank1_byte1;
	EMC(EMC_SWIZZLE_RANK1_BYTE2) = params->emc_swizzle_rank1_byte2;
	EMC(EMC_SWIZZLE_RANK1_BYTE3) = params->emc_swizzle_rank1_byte3;

	// Patch 3 using BCT spare variables.
	if (params->emc_bct_spare6)
		*(vu32 *)params->emc_bct_spare6 = params->emc_bct_spare7;

	// Set pad controls.
	EMC(EMC_XM2COMPPADCTRL) = params->emc_xm2_comp_pad_ctrl;
	EMC(EMC_XM2COMPPADCTRL2) = params->emc_xm2_comp_pad_ctrl2;
	EMC(EMC_XM2COMPPADCTRL3) = params->emc_xm2_comp_pad_ctrl3;

	// Program Autocal controls with shadowed register fields.
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

	EMC(EMC_PMACRO_BRICK_CTRL_RFU1) = params->emc_pmacro_brick_ctrl_rfu1;
	EMC(EMC_PMACRO_PAD_CFG_CTRL) = params->emc_pmacro_pad_cfg_ctrl;

	EMC(EMC_PMACRO_CMD_BRICK_CTRL_FDPD) = params->emc_pmacro_cmd_brick_ctrl_fdpd;
	EMC(EMC_PMACRO_BRICK_CTRL_RFU2) = params->emc_pmacro_brick_ctrl_rfu2;
	EMC(EMC_PMACRO_DATA_BRICK_CTRL_FDPD) = params->emc_pmacro_data_brick_ctrl_fdpd;
	EMC(EMC_PMACRO_DATA_PAD_RX_CTRL) = params->emc_pmacro_data_pad_rx_ctrl;
	EMC(EMC_PMACRO_CMD_PAD_RX_CTRL) = params->emc_pmacro_cmd_pad_rx_ctrl;
	EMC(EMC_PMACRO_DATA_PAD_TX_CTRL) = params->emc_pmacro_data_pad_tx_ctrl;
	EMC(EMC_PMACRO_DATA_RX_TERM_MODE) = params->emc_pmacro_data_rx_term_mode;
	EMC(EMC_PMACRO_CMD_RX_TERM_MODE) = params->emc_pmacro_cmd_rx_term_mode;
	EMC(EMC_PMACRO_CMD_PAD_TX_CTRL) = params->emc_pmacro_cmd_pad_tx_ctrl & 0xEFFFFFFF;

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

	// Program per bit pad macros.
	EMC(EMC_PMACRO_PERBIT_FGCG_CTRL_0) = params->emc_pmacro_perbit_fgcg_ctrl0;
	EMC(EMC_PMACRO_PERBIT_FGCG_CTRL_1) = params->emc_pmacro_perbit_fgcg_ctrl1;
	EMC(EMC_PMACRO_PERBIT_FGCG_CTRL_2) = params->emc_pmacro_perbit_fgcg_ctrl2;
	EMC(EMC_PMACRO_PERBIT_FGCG_CTRL_3) = params->emc_pmacro_perbit_fgcg_ctrl3;
	EMC(EMC_PMACRO_PERBIT_FGCG_CTRL_4) = params->emc_pmacro_perbit_fgcg_ctrl4;
	EMC(EMC_PMACRO_PERBIT_FGCG_CTRL_5) = params->emc_pmacro_perbit_fgcg_ctrl5;
	EMC(EMC_PMACRO_PERBIT_RFU_CTRL_0) = params->emc_pmacro_perbit_rfu_ctrl0;
	EMC(EMC_PMACRO_PERBIT_RFU_CTRL_1) = params->emc_pmacro_perbit_rfu_ctrl1;
	EMC(EMC_PMACRO_PERBIT_RFU_CTRL_2) = params->emc_pmacro_perbit_rfu_ctrl2;
	EMC(EMC_PMACRO_PERBIT_RFU_CTRL_3) = params->emc_pmacro_perbit_rfu_ctrl3;
	EMC(EMC_PMACRO_PERBIT_RFU_CTRL_4) = params->emc_pmacro_perbit_rfu_ctrl4;
	EMC(EMC_PMACRO_PERBIT_RFU_CTRL_5) = params->emc_pmacro_perbit_rfu_ctrl5;
	EMC(EMC_PMACRO_PERBIT_RFU1_CTRL_0) = params->emc_pmacro_perbit_rfu1_ctrl0;
	EMC(EMC_PMACRO_PERBIT_RFU1_CTRL_1) = params->emc_pmacro_perbit_rfu1_ctrl1;
	EMC(EMC_PMACRO_PERBIT_RFU1_CTRL_2) = params->emc_pmacro_perbit_rfu1_ctrl2;
	EMC(EMC_PMACRO_PERBIT_RFU1_CTRL_3) = params->emc_pmacro_perbit_rfu1_ctrl3;
	EMC(EMC_PMACRO_PERBIT_RFU1_CTRL_4) = params->emc_pmacro_perbit_rfu1_ctrl4;
	EMC(EMC_PMACRO_PERBIT_RFU1_CTRL_5) = params->emc_pmacro_perbit_rfu1_ctrl5;
	EMC(EMC_PMACRO_DATA_PI_CTRL) = params->emc_pmacro_data_pi_ctrl;
	EMC(EMC_PMACRO_CMD_PI_CTRL) = params->emc_pmacro_cmd_pi_ctrl;

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

	// Set DLL periodic offset.
	EMC(EMC_PMACRO_DDLL_PERIODIC_OFFSET) = params->emc_pmacro_ddll_periodic_offset;

	// Patch 4 using BCT spare variables.
	if (params->emc_bct_spare4)
		*(vu32 *)params->emc_bct_spare4 = params->emc_bct_spare5;

	// Patch 4 to 6 using BCT spare secure variables.
	if (params->emc_bct_spare_secure6)
		*(vu32 *)params->emc_bct_spare_secure6 = params->emc_bct_spare_secure7;
	if (params->emc_bct_spare_secure8)
		*(vu32 *)params->emc_bct_spare_secure8 = params->emc_bct_spare_secure9;
	if (params->emc_bct_spare_secure10)
		*(vu32 *)params->emc_bct_spare_secure10 = params->emc_bct_spare_secure11;

	EMC(EMC_TIMING_CONTROL) = 1; // Trigger timing update so above writes take place.

	// Initialize MC VPR settings.
	MC(MC_VIDEO_PROTECT_BOM) = params->mc_video_protect_bom;
	MC(MC_VIDEO_PROTECT_BOM_ADR_HI) = params->mc_video_protect_bom_adr_hi;
	MC(MC_VIDEO_PROTECT_SIZE_MB) = params->mc_video_protect_size_mb;
	MC(MC_VIDEO_PROTECT_VPR_OVERRIDE) = params->mc_video_protect_vpr_override;
	MC(MC_VIDEO_PROTECT_VPR_OVERRIDE1) = params->mc_video_protect_vpr_override1;
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_0) = params->mc_video_protect_gpu_override0;
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_1) = params->mc_video_protect_gpu_override1;

	// Program SDRAM geometry parameters.
	MC(MC_EMEM_ADR_CFG) = params->mc_emem_adr_cfg;
	MC(MC_EMEM_ADR_CFG_DEV0) = params->mc_emem_adr_cfg_dev0;
	MC(MC_EMEM_ADR_CFG_DEV1) = params->mc_emem_adr_cfg_dev1;
	MC(MC_EMEM_ADR_CFG_CHANNEL_MASK) = params->mc_emem_adr_cfg_channel_mask;

	// Program bank swizzling.
	MC(MC_EMEM_ADR_CFG_BANK_MASK_0) = params->mc_emem_adr_cfg_bank_mask0;
	MC(MC_EMEM_ADR_CFG_BANK_MASK_1) = params->mc_emem_adr_cfg_bank_mask1;
	MC(MC_EMEM_ADR_CFG_BANK_MASK_2) = params->mc_emem_adr_cfg_bank_mask2;

	// Program external memory aperture (base and size).
	MC(MC_EMEM_CFG) = params->mc_emem_cfg;

	// Program SEC carveout (base and size).
	MC(MC_SEC_CARVEOUT_BOM) = params->mc_sec_carveout_bom;
	MC(MC_SEC_CARVEOUT_ADR_HI) = params->mc_sec_carveout_adr_hi;
	MC(MC_SEC_CARVEOUT_SIZE_MB) = params->mc_sec_carveout_size_mb;

	// Program MTS carveout (base and size).
	MC(MC_MTS_CARVEOUT_BOM) = params->mc_mts_carveout_bom;
	MC(MC_MTS_CARVEOUT_ADR_HI) = params->mc_mts_carveout_adr_hi;
	MC(MC_MTS_CARVEOUT_SIZE_MB) = params->mc_mts_carveout_size_mb;

	// Program the memory arbiter.
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

	MC(MC_TIMING_CONTROL) = 1; // Trigger MC timing update.

	// Program second-level clock enable overrides.
	MC(MC_CLKEN_OVERRIDE) = params->mc_clken_override;

	// Program statistics gathering.
	MC(MC_STAT_CONTROL) = params->mc_stat_control;

	// Program SDRAM geometry parameters.
	EMC(EMC_ADR_CFG) = params->emc_adr_cfg;

	// Program second-level clock enable overrides.
	EMC(EMC_CLKEN_OVERRIDE) = params->emc_clken_override;

	// Program EMC pad auto calibration.
	EMC(EMC_PMACRO_AUTOCAL_CFG_0) = params->emc_pmacro_auto_cal_cfg0;
	EMC(EMC_PMACRO_AUTOCAL_CFG_1) = params->emc_pmacro_auto_cal_cfg1;
	EMC(EMC_PMACRO_AUTOCAL_CFG_2) = params->emc_pmacro_auto_cal_cfg2;

	EMC(EMC_AUTO_CAL_VREF_SEL_0) = params->emc_auto_cal_vref_sel0;
	EMC(EMC_AUTO_CAL_VREF_SEL_1) = params->emc_auto_cal_vref_sel1;

	EMC(EMC_AUTO_CAL_INTERVAL) = params->emc_auto_cal_interval;
	EMC(EMC_AUTO_CAL_CONFIG) = params->emc_auto_cal_config;
	usleep(params->emc_auto_cal_wait);

	// Patch 5 using BCT spare variables.
	if (params->emc_bct_spare8)
		*(vu32 *)params->emc_bct_spare8 = params->emc_bct_spare9;

	EMC(EMC_AUTO_CAL_CONFIG9) = params->emc_auto_cal_config9;

	// Program EMC timing configuration.
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
	EMC(EMC_CTT) = params->emc_trtm;
	EMC(EMC_FBIO_TWTM) = params->emc_twtm;
	EMC(EMC_FBIO_TRATM) = params->emc_tratm;
	EMC(EMC_FBIO_TWATM) = params->emc_twatm;
	EMC(EMC_FBIO_TR2REF) = params->emc_tr2ref;
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
	EMC(EMC_DBG) = params->emc_dbg;
	EMC(EMC_QRST) = params->emc_qrst;
	EMC(EMC_ISSUE_QRST) = 1;
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

	// Don't write CFG_ADR_EN (bit 1) here - lock bit written later.
	EMC(EMC_FBIO_SPARE) = params->emc_fbio_spare & 0xFFFFFFFD;
	EMC(EMC_CFG_RSV) = params->emc_cfg_rsv;
	EMC(EMC_PMC_SCRATCH1) = params->emc_pmc_scratch1;
	EMC(EMC_PMC_SCRATCH2) = params->emc_pmc_scratch2;
	EMC(EMC_PMC_SCRATCH3) = params->emc_pmc_scratch3;
	EMC(EMC_ACPD_CONTROL) = params->emc_acpd_control;
	EMC(EMC_TXDSRVTTGEN) = params->emc_txdsrvttgen;
	EMC(EMC_PMACRO_DSR_VTTGEN_CTRL0) = params->emc_pmacro_dsr_vttgen_ctrl0;

	// Set pipe bypass enable bits before sending any DRAM commands.
	EMC(EMC_CFG) = (params->emc_cfg & 0xE) | 0x3C00000;

	// BootROM patching is used as a generic patch here.
	if (params->boot_rom_patch_control)
	{
		*(vu32 *)params->boot_rom_patch_control = params->boot_rom_patch_data;
		MC(MC_TIMING_CONTROL) = 1; // Trigger MC timing update.
	}

	// Patch 7 to 9 using BCT spare secure variables.
	if (params->emc_bct_spare_secure12)
		*(vu32 *)params->emc_bct_spare_secure12 = params->emc_bct_spare_secure13;
	if (params->emc_bct_spare_secure14)
		*(vu32 *)params->emc_bct_spare_secure14 = params->emc_bct_spare_secure15;
	if (params->emc_bct_spare_secure16)
		*(vu32 *)params->emc_bct_spare_secure16 = params->emc_bct_spare_secure17;

	// Release SEL_DPD_CMD.
	PMC(APBDEV_PMC_IO_DPD3_REQ) = ((params->emc_pmc_scratch1 & 0x3FFFFFFF) | 0x40000000) & 0xCFFF0000;
	usleep(params->pmc_io_dpd3_req_wait);

	// Set transmission pad control parameters.
	EMC(EMC_PMACRO_CMD_PAD_TX_CTRL) = params->emc_pmacro_cmd_pad_tx_ctrl;

	// ZQ CAL setup (not actually issuing ZQ CAL now).
	if (params->emc_zcal_warm_cold_boot_enables & 1)
	{
		if (params->memory_type == MEMORY_TYPE_DDR3L)
			EMC(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt << 3;
		if (params->memory_type == MEMORY_TYPE_LPDDR4)
		{
			EMC(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt;
			EMC(EMC_ZCAL_MRW_CMD) = params->emc_zcal_mrw_cmd;
		}
	}

	EMC(EMC_TIMING_CONTROL) = 1; // Trigger timing update so above writes take place.
	usleep(params->emc_timing_control_wait);

	// Deassert HOLD_CKE_LOW.
	PMC(APBDEV_PMC_DDR_CNTRL) &= 0xFF78007F;
	usleep(params->pmc_ddr_ctrl_wait);

	// Set clock enable signal.
	u32 pin_gpio_cfg = (params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12);
	if (params->memory_type == MEMORY_TYPE_DDR3L || params->memory_type == MEMORY_TYPE_LPDDR4)
	{
		EMC(EMC_PIN) = pin_gpio_cfg;
		(void)EMC(EMC_PIN);
		usleep(params->emc_pin_extra_wait + 200);
		EMC(EMC_PIN) = pin_gpio_cfg | 0x100;
		(void)EMC(EMC_PIN);
	}

	if (params->memory_type == MEMORY_TYPE_LPDDR4)
		usleep(params->emc_pin_extra_wait + 2000);
	else if (params->memory_type == MEMORY_TYPE_DDR3L)
		usleep(params->emc_pin_extra_wait + 500);

	// Enable clock enable signal.
	EMC(EMC_PIN) = pin_gpio_cfg | 0x101;
	(void)EMC(EMC_PIN);
	usleep(params->emc_pin_program_wait);

	// Send NOP (trigger just needs to be non-zero).
	if (params->memory_type != MEMORY_TYPE_LPDDR4)
		EMC(EMC_NOP) = (params->emc_dev_select << 30) + 1;

	// On coldboot w/LPDDR2/3, wait 200 uSec after asserting CKE high.
	if (params->memory_type == MEMORY_TYPE_LPDDR2)
		usleep(params->emc_pin_extra_wait + 200);

	// Init zq calibration,
	if (params->memory_type == MEMORY_TYPE_LPDDR4)
	{
		// Patch 6 using BCT spare variables.
		if (params->emc_bct_spare10)
			*(vu32 *)params->emc_bct_spare10 = params->emc_bct_spare11;

		// Write mode registers.
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
			// Issue ZQCAL start, device 0.
			EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev0;
			usleep(params->emc_zcal_init_wait);

			// Issue ZQCAL latch.
			EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev0 ^ 3;
			// Same for device 1.
			if (!(params->emc_dev_select & 2))
			{
				EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev1;
				usleep(params->emc_zcal_init_wait);
				EMC(EMC_ZQ_CAL) = params->emc_zcal_init_dev1 ^ 3;
			}
		}
	}

	// Patch 10 to 12 using BCT spare secure variables.
	if (params->emc_bct_spare_secure18)
		*(vu32 *)params->emc_bct_spare_secure18 = params->emc_bct_spare_secure19;
	if (params->emc_bct_spare_secure20)
		*(vu32 *)params->emc_bct_spare_secure20 = params->emc_bct_spare_secure21;
	if (params->emc_bct_spare_secure22)
		*(vu32 *)params->emc_bct_spare_secure22 = params->emc_bct_spare_secure23;

	// Set package and DPD pad control.
	PMC(APBDEV_PMC_DDR_CFG) = params->pmc_ddr_cfg;

	// Start periodic ZQ calibration (LPDDRx only).
	if (params->memory_type == MEMORY_TYPE_LPDDR2 || params->memory_type == MEMORY_TYPE_DDR3L || params->memory_type == MEMORY_TYPE_LPDDR4)
	{
		EMC(EMC_ZCAL_INTERVAL) = params->emc_zcal_interval;
		EMC(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt;
		EMC(EMC_ZCAL_MRW_CMD) = params->emc_zcal_mrw_cmd;
	}

	// Patch 7 using BCT spare variables.
	if (params->emc_bct_spare12)
		*(vu32 *)params->emc_bct_spare12 = params->emc_bct_spare13;

	EMC(EMC_TIMING_CONTROL) = 1; // Trigger timing update so above writes take place.

	if (params->emc_extra_refresh_num)
		EMC(EMC_REF) = ((1 << params->emc_extra_refresh_num << 8) - 253) | (params->emc_dev_select << 30);

	// Enable refresh.
	EMC(EMC_REFCTRL) = params->emc_dev_select | 0x80000000;

	EMC(EMC_DYN_SELF_REF_CONTROL) = params->emc_dyn_self_ref_control;
	EMC(EMC_CFG) = params->emc_cfg;
	EMC(EMC_FDPD_CTRL_DQ) = params->emc_fdpd_ctrl_dq;
	EMC(EMC_FDPD_CTRL_CMD) = params->emc_fdpd_ctrl_cmd;
	EMC(EMC_SEL_DPD_CTRL) = params->emc_sel_dpd_ctrl;

	// Write addr swizzle lock bit.
	EMC(EMC_FBIO_SPARE) = params->emc_fbio_spare | 2;

	EMC(EMC_TIMING_CONTROL) = 1; // Re-trigger timing to latch power saving functions.

	EMC(EMC_CFG_UPDATE) = params->emc_cfg_update;

	// Enable EMC pipe clock gating.
	EMC(EMC_CFG_PIPE_CLK) = params->emc_cfg_pipe_clk;

	// Depending on freqency, enable CMD/CLK fdpd.
	EMC(EMC_FDPD_CTRL_CMD_NO_RAMP) = params->emc_fdpd_ctrl_cmd_no_ramp;

	// Set untranslated region requirements.
	MC(MC_UNTRANSLATED_REGION_CHECK) = params->mc_untranslated_region_check;

	// Lock carveouts per BCT cfg.
	MC(MC_VIDEO_PROTECT_REG_CTRL) = params->mc_video_protect_write_access;
	MC(MC_SEC_CARVEOUT_REG_CTRL) = params->mc_sec_carveout_protect_write_access;
	MC(MC_MTS_CARVEOUT_REG_CTRL) = params->mc_mts_carveout_reg_ctrl;

	// Disable write access to a bunch of EMC registers.
	MC(MC_EMEM_CFG_ACCESS_CTRL) = 1;

	// Enable arbiter.
	SYSREG(AHB_ARBITRATION_XBAR_CTRL) = (SYSREG(AHB_ARBITRATION_XBAR_CTRL) & 0xFFFEFFFF) | (params->ahb_arbitration_xbar_ctrl_meminit_done << 16);
}

static void *_sdram_get_params_t210()
{
	// Check if id is proper.
	u32 dramid = fuse_read_dramid(false);

	// Copy base parameters.
	u32 *params = (u32 *)SDRAM_PARAMS_ADDR;
	memcpy(params, &_dram_cfg_0_samsung_4gb, sizeof(sdram_params_t210_t));

	// Patch parameters if needed.
	for (u32 i = 0; i < ARRAY_SIZE(sdram_cfg_vendor_patches_t210); i++)
		if (sdram_cfg_vendor_patches_t210[i].dramcf & DRAM_ID(dramid))
			params[sdram_cfg_vendor_patches_t210[i].offset] = sdram_cfg_vendor_patches_t210[i].val;

	return (void *)params;
}

void *sdram_get_params_t210b01()
{
	// Check if id is proper.
	u32 dramid = fuse_read_dramid(false);

	// Copy base parameters.
	u32 *params = (u32 *)SDRAM_PARAMS_ADDR;
	memcpy(params, &_dram_cfg_08_10_12_14_samsung_hynix_4gb, sizeof(sdram_params_t210b01_t));

	// Patch parameters if needed.
	u8 dram_code = dram_encoding_t210b01[dramid];
	if (!dram_code)
		return (void *)params;

	for (u32 i = 0; i < ARRAY_SIZE(sdram_cfg_vendor_patches_t210b01); i++)
		if (sdram_cfg_vendor_patches_t210b01[i].dramcf == dram_code)
			params[sdram_cfg_vendor_patches_t210b01[i].offset] = sdram_cfg_vendor_patches_t210b01[i].val;

	return (void *)params;
}

/*
 * Function: sdram_get_params_patched
 *
 * This code implements a warmboot exploit. Warmboot, that is actually so hot, it burns Nvidia once again.
 * If the boot_rom_patch_control's MSB is set, it uses it as an index to
 * APB_MISC_BASE (u32 array) and sets it to the value of boot_rom_patch_data.
 * (The MSB falls out when it gets multiplied by sizeof(u32)).
 * Because the bootrom does not do any boundary checks, it lets us write anywhere and anything.
 * Ipatch hardware let us apply 12 changes to the bootrom and can be changed any time.
 * The first patch is not needed any more when the exploit is triggered, so we overwrite that.
 * 0x10459E is the address where it returns an error when the signature is not valid.
 * We change that to MOV R0, #0, so we pass the check.
 *
 * Note: The modulus in the header must match and validated.
 */

void *sdram_get_params_patched()
{
	#define IPATCH_CONFIG(addr, data) ((((addr) - 0x100000) / 2) << 16 | ((data) & 0xffff))
	sdram_params_t210_t *sdram_params = _sdram_get_params_t210();

	// Disable Warmboot signature check.
	sdram_params->boot_rom_patch_control = BIT(31) | (((IPATCH_BASE + 4) - APB_MISC_BASE) / 4);
	sdram_params->boot_rom_patch_data = IPATCH_CONFIG(0x10459E, 0x2000);
/*
	// Disable SBK lock.
	sdram_params->emc_bct_spare8 = (IPATCH_BASE + 7 * 4);
	sdram_params->emc_bct_spare9 = IPATCH_CONFIG(0x10210E, 0x2000);

	// Disable bootrom read lock.
	sdram_params->emc_bct_spare10 = (IPATCH_BASE + 10 * 4);
	sdram_params->emc_bct_spare11 = IPATCH_CONFIG(0x100FDC, 0xF000);
	sdram_params->emc_bct_spare12 = (IPATCH_BASE + 11 * 4);
	sdram_params->emc_bct_spare13 = IPATCH_CONFIG(0x100FDE, 0xE320);
*/
	return (void *)sdram_params;
}

static void _sdram_init_t210()
{
	const sdram_params_t210_t *params = (const sdram_params_t210_t *)_sdram_get_params_t210();

	// Set DRAM voltage.
	max7762x_regulator_set_voltage(REGULATOR_SD1, 1100000); // HOS uses 1.125V

	// VDDP Select.
	PMC(APBDEV_PMC_VDDP_SEL) = params->pmc_vddp_sel;
	usleep(params->pmc_vddp_sel_wait);

	// Set DDR pad voltage.
	PMC(APBDEV_PMC_DDR_PWR) = PMC(APBDEV_PMC_DDR_PWR);

	// Turn on MEM IO Power.
	PMC(APBDEV_PMC_NO_IOPOWER) = params->pmc_no_io_power;
	PMC(APBDEV_PMC_REG_SHORT) = params->pmc_reg_short;

	PMC(APBDEV_PMC_DDR_CNTRL) = params->pmc_ddr_ctrl;

	// Patch 1 using BCT spare variables
	if (params->emc_bct_spare0)
		*(vu32 *)params->emc_bct_spare0 = params->emc_bct_spare1;

	_sdram_config_t210(params);
}

static void _sdram_init_t210b01()
{
	const sdram_params_t210b01_t *params = (const sdram_params_t210b01_t *)sdram_get_params_t210b01();

	// VDDP Select.
	PMC(APBDEV_PMC_VDDP_SEL) = params->pmc_vddp_sel;
	usleep(params->pmc_vddp_sel_wait);

	// Turn on MEM IO Power.
	PMC(APBDEV_PMC_NO_IOPOWER) = params->pmc_no_io_power;
	PMC(APBDEV_PMC_REG_SHORT) = params->pmc_reg_short;

	PMC(APBDEV_PMC_DDR_CNTRL) = params->pmc_ddr_ctrl;

	// Patch 1 using BCT spare variables
	if (params->emc_bct_spare0)
		*(vu32 *)params->emc_bct_spare0 = params->emc_bct_spare1;

	_sdram_config_t210b01(params);
}

void sdram_init()
{
	// Disable remote sense for SD1.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_SD_CFG2, MAX77620_SD_CNF2_ROVS_EN_SD0 | MAX77620_SD_CNF2_RSVD);

	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210)
		_sdram_init_t210();
	else
		_sdram_init_t210b01();
}
