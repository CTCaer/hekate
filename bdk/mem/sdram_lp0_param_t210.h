/*
 * Copyright (c) 2013-2015, NVIDIA CORPORATION.  All rights reserved.
 * Copyright 2014 Google Inc.
 * Copyright (c) 2018 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

/**
 * Defines the SDRAM parameter structure.
 *
 * Note that PLLM is used by EMC. The field names are in camel case to ease
 * directly converting BCT config files (*.cfg) into C structure.
 */

#ifndef __TEGRA210_SDRAM_PARAM_H__
#define __TEGRA210_SDRAM_PARAM_H__

#include <utils/types.h>

enum
{
	/* Specifies the memory type to be undefined */
	NvBootMemoryType_None = 0,

	/* Specifies the memory type to be DDR SDRAM */
	NvBootMemoryType_Ddr = 0,

	/* Specifies the memory type to be LPDDR SDRAM */
	NvBootMemoryType_LpDdr = 0,

	/* Specifies the memory type to be DDR2 SDRAM */
	NvBootMemoryType_Ddr2 = 0,

	/* Specifies the memory type to be LPDDR2 SDRAM */
	NvBootMemoryType_LpDdr2,

	/* Specifies the memory type to be DDR3 SDRAM */
	NvBootMemoryType_Ddr3,

	/* Specifies the memory type to be LPDDR4 SDRAM */
	NvBootMemoryType_LpDdr4,

	NvBootMemoryType_Num,

	/* Specifies an entry in the ram_code table that's not in use */
	NvBootMemoryType_Unused = 0X7FFFFFF,
};

/**
 * Defines the SDRAM parameter structure
 */
struct sdram_params_t210
{

	/* Specifies the type of memory device */
	u32 MemoryType;

	/* MC/EMC clock source configuration */

	/* Specifies the M value for PllM */
	u32 PllMInputDivider;
	/* Specifies the N value for PllM */
	u32 PllMFeedbackDivider;
	/* Specifies the time to wait for PLLM to lock (in microseconds) */
	u32 PllMStableTime;
	/* Specifies misc. control bits */
	u32 PllMSetupControl;
	/* Specifies the P value for PLLM */
	u32 PllMPostDivider;
	/* Specifies value for Charge Pump Gain Control */
	u32 PllMKCP;
	/* Specifies VCO gain */
	u32 PllMKVCO;
	/* Spare BCT param */
	u32 EmcBctSpare0;
	/* Spare BCT param */
	u32 EmcBctSpare1;
	/* Spare BCT param */
	u32 EmcBctSpare2;
	/* Spare BCT param */
	u32 EmcBctSpare3;
	/* Spare BCT param */
	u32 EmcBctSpare4;
	/* Spare BCT param */
	u32 EmcBctSpare5;
	/* Spare BCT param */
	u32 EmcBctSpare6;
	/* Spare BCT param */
	u32 EmcBctSpare7;
	/* Spare BCT param */
	u32 EmcBctSpare8;
	/* Spare BCT param */
	u32 EmcBctSpare9;
	/* Spare BCT param */
	u32 EmcBctSpare10;
	/* Spare BCT param */
	u32 EmcBctSpare11;
	/* Spare BCT param */
	u32 EmcBctSpare12;
	/* Spare BCT param */
	u32 EmcBctSpare13;

	/* Defines EMC_2X_CLK_SRC, EMC_2X_CLK_DIVISOR, EMC_INVERT_DCD */
	u32 EmcClockSource;
	u32 EmcClockSourceDll;

	/* Defines possible override for PLLLM_MISC2 */
	u32 ClkRstControllerPllmMisc2Override;
	/* enables override for PLLLM_MISC2 */
	u32 ClkRstControllerPllmMisc2OverrideEnable;
	/* defines CLK_ENB_MC1 in register clk_rst_controller_clk_enb_w_clr */
	u32 ClearClk2Mc1;

	/* Auto-calibration of EMC pads */

	/* Specifies the value for EMC_AUTO_CAL_INTERVAL */
	u32 EmcAutoCalInterval;
	/*
	 * Specifies the value for EMC_AUTO_CAL_CONFIG
	 * Note: Trigger bits are set by the SDRAM code.
	 */
	u32 EmcAutoCalConfig;

	/* Specifies the value for EMC_AUTO_CAL_CONFIG2 */
	u32 EmcAutoCalConfig2;

	/* Specifies the value for EMC_AUTO_CAL_CONFIG3 */
	u32 EmcAutoCalConfig3;

	/* Specifies the values for EMC_AUTO_CAL_CONFIG4-8 */
	u32 EmcAutoCalConfig4;
	u32 EmcAutoCalConfig5;
	u32 EmcAutoCalConfig6;
	u32 EmcAutoCalConfig7;
	u32 EmcAutoCalConfig8;

	/* Specifies the value for EMC_AUTO_CAL_VREF_SEL_0 */
	u32 EmcAutoCalVrefSel0;
	u32 EmcAutoCalVrefSel1;

	/* Specifies the value for EMC_AUTO_CAL_CHANNEL */
	u32 EmcAutoCalChannel;

	/* Specifies the value for EMC_PMACRO_AUTOCAL_CFG_0 */
	u32 EmcPmacroAutocalCfg0;
	u32 EmcPmacroAutocalCfg1;
	u32 EmcPmacroAutocalCfg2;
	u32 EmcPmacroRxTerm;
	u32 EmcPmacroDqTxDrv;
	u32 EmcPmacroCaTxDrv;
	u32 EmcPmacroCmdTxDrv;
	u32 EmcPmacroAutocalCfgCommon;
	u32 EmcPmacroZctrl;

	/*
	 * Specifies the time for the calibration
	 * to stabilize (in microseconds)
	 */
	u32 EmcAutoCalWait;

	u32 EmcXm2CompPadCtrl;
	u32 EmcXm2CompPadCtrl2;
	u32 EmcXm2CompPadCtrl3;

	/*
	 * DRAM size information
	 * Specifies the value for EMC_ADR_CFG
	 */
	u32 EmcAdrCfg;

	/*
	 * Specifies the time to wait after asserting pin
	 * CKE (in microseconds)
	 */
	u32 EmcPinProgramWait;
	/* Specifies the extra delay before/after pin RESET/CKE command */
	u32 EmcPinExtraWait;

	u32 EmcPinGpioEn;
	u32 EmcPinGpio;

	/*
	 * Specifies the extra delay after the first writing
	 * of EMC_TIMING_CONTROL
	 */
	u32 EmcTimingControlWait;

	/* Timing parameters required for the SDRAM */

	/* Specifies the value for EMC_RC */
	u32 EmcRc;
	/* Specifies the value for EMC_RFC */
	u32 EmcRfc;
	/* Specifies the value for EMC_RFC_PB */
	u32 EmcRfcPb;
	/* Specifies the value for EMC_RFC_CTRL2 */
	u32 EmcRefctrl2;
	/* Specifies the value for EMC_RFC_SLR */
	u32 EmcRfcSlr;
	/* Specifies the value for EMC_RAS */
	u32 EmcRas;
	/* Specifies the value for EMC_RP */
	u32 EmcRp;
	/* Specifies the value for EMC_R2R */
	u32 EmcR2r;
	/* Specifies the value for EMC_W2W */
	u32 EmcW2w;
	/* Specifies the value for EMC_R2W */
	u32 EmcR2w;
	/* Specifies the value for EMC_W2R */
	u32 EmcW2r;
	/* Specifies the value for EMC_R2P */
	u32 EmcR2p;
	/* Specifies the value for EMC_W2P */
	u32 EmcW2p;

	u32 EmcTppd;
	u32 EmcCcdmw;

	/* Specifies the value for EMC_RD_RCD */
	u32 EmcRdRcd;
	/* Specifies the value for EMC_WR_RCD */
	u32 EmcWrRcd;
	/* Specifies the value for EMC_RRD */
	u32 EmcRrd;
	/* Specifies the value for EMC_REXT */
	u32 EmcRext;
	/* Specifies the value for EMC_WEXT */
	u32 EmcWext;
	/* Specifies the value for EMC_WDV */
	u32 EmcWdv;

	u32 EmcWdvChk;
	u32 EmcWsv;
	u32 EmcWev;

	/* Specifies the value for EMC_WDV_MASK */
	u32 EmcWdvMask;

	u32 EmcWsDuration;
	u32 EmcWeDuration;

	/* Specifies the value for EMC_QUSE */
	u32 EmcQUse;
	/* Specifies the value for EMC_QUSE_WIDTH */
	u32 EmcQuseWidth;
	/* Specifies the value for EMC_IBDLY */
	u32 EmcIbdly;
	/* Specifies the value for EMC_OBDLY */
	u32 EmcObdly;
	/* Specifies the value for EMC_EINPUT */
	u32 EmcEInput;
	/* Specifies the value for EMC_EINPUT_DURATION */
	u32 EmcEInputDuration;
	/* Specifies the value for EMC_PUTERM_EXTRA */
	u32 EmcPutermExtra;
	/* Specifies the value for EMC_PUTERM_WIDTH */
	u32 EmcPutermWidth;
	/* Specifies the value for EMC_PUTERM_ADJ */
	////u32 EmcPutermAdj;

	/* Specifies the value for EMC_QRST */
	u32 EmcQRst;
	/* Specifies the value for EMC_QSAFE */
	u32 EmcQSafe;
	/* Specifies the value for EMC_RDV */
	u32 EmcRdv;
	/* Specifies the value for EMC_RDV_MASK */
	u32 EmcRdvMask;
	/* Specifies the value for EMC_RDV_EARLY */
	u32 EmcRdvEarly;
	/* Specifies the value for EMC_RDV_EARLY_MASK */
	u32 EmcRdvEarlyMask;
	/* Specifies the value for EMC_QPOP */
	u32 EmcQpop;

	/* Specifies the value for EMC_REFRESH */
	u32 EmcRefresh;
	/* Specifies the value for EMC_BURST_REFRESH_NUM */
	u32 EmcBurstRefreshNum;
	/* Specifies the value for EMC_PRE_REFRESH_REQ_CNT */
	u32 EmcPreRefreshReqCnt;
	/* Specifies the value for EMC_PDEX2WR */
	u32 EmcPdEx2Wr;
	/* Specifies the value for EMC_PDEX2RD */
	u32 EmcPdEx2Rd;
	/* Specifies the value for EMC_PCHG2PDEN */
	u32 EmcPChg2Pden;
	/* Specifies the value for EMC_ACT2PDEN */
	u32 EmcAct2Pden;
	/* Specifies the value for EMC_AR2PDEN */
	u32 EmcAr2Pden;
	/* Specifies the value for EMC_RW2PDEN */
	u32 EmcRw2Pden;
	/* Specifies the value for EMC_CKE2PDEN */
	u32 EmcCke2Pden;
	/* Specifies the value for EMC_PDEX2CKE */
	u32 EmcPdex2Cke;
	/* Specifies the value for EMC_PDEX2MRR */
	u32 EmcPdex2Mrr;
	/* Specifies the value for EMC_TXSR */
	u32 EmcTxsr;
	/* Specifies the value for EMC_TXSRDLL */
	u32 EmcTxsrDll;
	/* Specifies the value for EMC_TCKE */
	u32 EmcTcke;
	/* Specifies the value for EMC_TCKESR */
	u32 EmcTckesr;
	/* Specifies the value for EMC_TPD */
	u32 EmcTpd;
	/* Specifies the value for EMC_TFAW */
	u32 EmcTfaw;
	/* Specifies the value for EMC_TRPAB */
	u32 EmcTrpab;
	/* Specifies the value for EMC_TCLKSTABLE */
	u32 EmcTClkStable;
	/* Specifies the value for EMC_TCLKSTOP */
	u32 EmcTClkStop;
	/* Specifies the value for EMC_TREFBW */
	u32 EmcTRefBw;

	/* FBIO configuration values */

	/* Specifies the value for EMC_FBIO_CFG5 */
	u32 EmcFbioCfg5;
	/* Specifies the value for EMC_FBIO_CFG7 */
	u32 EmcFbioCfg7;
	/* Specifies the value for EMC_FBIO_CFG8 */
	u32 EmcFbioCfg8;

	/* Command mapping for CMD brick 0 */
	u32 EmcCmdMappingCmd0_0;
	u32 EmcCmdMappingCmd0_1;
	u32 EmcCmdMappingCmd0_2;
	u32 EmcCmdMappingCmd1_0;
	u32 EmcCmdMappingCmd1_1;
	u32 EmcCmdMappingCmd1_2;
	u32 EmcCmdMappingCmd2_0;
	u32 EmcCmdMappingCmd2_1;
	u32 EmcCmdMappingCmd2_2;
	u32 EmcCmdMappingCmd3_0;
	u32 EmcCmdMappingCmd3_1;
	u32 EmcCmdMappingCmd3_2;
	u32 EmcCmdMappingByte;

	/* Specifies the value for EMC_FBIO_SPARE */
	u32 EmcFbioSpare;

	/* Specifies the value for EMC_CFG_RSV */
	u32 EmcCfgRsv;

	/* MRS command values */

	/* Specifies the value for EMC_MRS */
	u32 EmcMrs;
	/* Specifies the MP0 command to initialize mode registers */
	u32 EmcEmrs;
	/* Specifies the MP2 command to initialize mode registers */
	u32 EmcEmrs2;
	/* Specifies the MP3 command to initialize mode registers */
	u32 EmcEmrs3;
	/* Specifies the programming to LPDDR2 Mode Register 1 at cold boot */
	u32 EmcMrw1;
	/* Specifies the programming to LPDDR2 Mode Register 2 at cold boot */
	u32 EmcMrw2;
	/* Specifies the programming to LPDDR2 Mode Register 3 at cold boot */
	u32 EmcMrw3;
	/* Specifies the programming to LPDDR2 Mode Register 11 at cold boot */
	u32 EmcMrw4;
	/* Specifies the programming to LPDDR2 Mode Register 3? at cold boot */
	u32 EmcMrw6;
	/* Specifies the programming to LPDDR2 Mode Register 11 at cold boot */
	u32 EmcMrw8;
	/* Specifies the programming to LPDDR2 Mode Register 11? at cold boot */
	u32 EmcMrw9;
	/* Specifies the programming to LPDDR2 Mode Register 12 at cold boot */
	u32 EmcMrw10;
	/* Specifies the programming to LPDDR2 Mode Register 14 at cold boot */
	u32 EmcMrw12;
	/* Specifies the programming to LPDDR2 Mode Register 14? at cold boot */
	u32 EmcMrw13;
	/* Specifies the programming to LPDDR2 Mode Register 22 at cold boot */
	u32 EmcMrw14;
	/*
	 * Specifies the programming to extra LPDDR2 Mode Register
	 * at cold boot
	 */
	u32 EmcMrwExtra;
	/*
	 * Specifies the programming to extra LPDDR2 Mode Register
	 * at warm boot
	 */
	u32 EmcWarmBootMrwExtra;
	/*
	 * Specify the enable of extra Mode Register programming at
	 * warm boot
	 */
	u32 EmcWarmBootExtraModeRegWriteEnable;
	/*
	 * Specify the enable of extra Mode Register programming at
	 * cold boot
	 */
	u32 EmcExtraModeRegWriteEnable;

	/* Specifies the EMC_MRW reset command value */
	u32 EmcMrwResetCommand;
	/* Specifies the EMC Reset wait time (in microseconds) */
	u32 EmcMrwResetNInitWait;
	/* Specifies the value for EMC_MRS_WAIT_CNT */
	u32 EmcMrsWaitCnt;
	/* Specifies the value for EMC_MRS_WAIT_CNT2 */
	u32 EmcMrsWaitCnt2;

	/* EMC miscellaneous configurations */

	/* Specifies the value for EMC_CFG */
	u32 EmcCfg;
	/* Specifies the value for EMC_CFG_2 */
	u32 EmcCfg2;
	/* Specifies the pipe bypass controls */
	u32 EmcCfgPipe;
	u32 EmcCfgPipeClk;
	u32 EmcFdpdCtrlCmdNoRamp;
	u32 EmcCfgUpdate;

	/* Specifies the value for EMC_DBG */
	u32 EmcDbg;
	u32 EmcDbgWriteMux;

	/* Specifies the value for EMC_CMDQ */
	u32 EmcCmdQ;
	/* Specifies the value for EMC_MC2EMCQ */
	u32 EmcMc2EmcQ;
	/* Specifies the value for EMC_DYN_SELF_REF_CONTROL */
	u32 EmcDynSelfRefControl;

	/* Specifies the value for MEM_INIT_DONE */
	u32 AhbArbitrationXbarCtrlMemInitDone;

	/* Specifies the value for EMC_CFG_DIG_DLL */
	u32 EmcCfgDigDll;
	u32 EmcCfgDigDll_1;
	/* Specifies the value for EMC_CFG_DIG_DLL_PERIOD */
	u32 EmcCfgDigDllPeriod;
	/* Specifies the value of *DEV_SELECTN of various EMC registers */
	u32 EmcDevSelect;

	/* Specifies the value for EMC_SEL_DPD_CTRL */
	u32 EmcSelDpdCtrl;

	/* Pads trimmer delays */
	u32 EmcFdpdCtrlDq;
	u32 EmcFdpdCtrlCmd;
	u32 EmcPmacroIbVrefDq_0;
	u32 EmcPmacroIbVrefDq_1;
	u32 EmcPmacroIbVrefDqs_0;
	u32 EmcPmacroIbVrefDqs_1;
	u32 EmcPmacroIbRxrt;
	u32 EmcCfgPipe1;
	u32 EmcCfgPipe2;

	/* Specifies the value for EMC_PMACRO_QUSE_DDLL_RANK0_0 */
	u32 EmcPmacroQuseDdllRank0_0;
	u32 EmcPmacroQuseDdllRank0_1;
	u32 EmcPmacroQuseDdllRank0_2;
	u32 EmcPmacroQuseDdllRank0_3;
	u32 EmcPmacroQuseDdllRank0_4;
	u32 EmcPmacroQuseDdllRank0_5;
	u32 EmcPmacroQuseDdllRank1_0;
	u32 EmcPmacroQuseDdllRank1_1;
	u32 EmcPmacroQuseDdllRank1_2;
	u32 EmcPmacroQuseDdllRank1_3;
	u32 EmcPmacroQuseDdllRank1_4;
	u32 EmcPmacroQuseDdllRank1_5;

	u32 EmcPmacroObDdllLongDqRank0_0;
	u32 EmcPmacroObDdllLongDqRank0_1;
	u32 EmcPmacroObDdllLongDqRank0_2;
	u32 EmcPmacroObDdllLongDqRank0_3;
	u32 EmcPmacroObDdllLongDqRank0_4;
	u32 EmcPmacroObDdllLongDqRank0_5;
	u32 EmcPmacroObDdllLongDqRank1_0;
	u32 EmcPmacroObDdllLongDqRank1_1;
	u32 EmcPmacroObDdllLongDqRank1_2;
	u32 EmcPmacroObDdllLongDqRank1_3;
	u32 EmcPmacroObDdllLongDqRank1_4;
	u32 EmcPmacroObDdllLongDqRank1_5;

	u32 EmcPmacroObDdllLongDqsRank0_0;
	u32 EmcPmacroObDdllLongDqsRank0_1;
	u32 EmcPmacroObDdllLongDqsRank0_2;
	u32 EmcPmacroObDdllLongDqsRank0_3;
	u32 EmcPmacroObDdllLongDqsRank0_4;
	u32 EmcPmacroObDdllLongDqsRank0_5;
	u32 EmcPmacroObDdllLongDqsRank1_0;
	u32 EmcPmacroObDdllLongDqsRank1_1;
	u32 EmcPmacroObDdllLongDqsRank1_2;
	u32 EmcPmacroObDdllLongDqsRank1_3;
	u32 EmcPmacroObDdllLongDqsRank1_4;
	u32 EmcPmacroObDdllLongDqsRank1_5;

	u32 EmcPmacroIbDdllLongDqsRank0_0;
	u32 EmcPmacroIbDdllLongDqsRank0_1;
	u32 EmcPmacroIbDdllLongDqsRank0_2;
	u32 EmcPmacroIbDdllLongDqsRank0_3;
	u32 EmcPmacroIbDdllLongDqsRank1_0;
	u32 EmcPmacroIbDdllLongDqsRank1_1;
	u32 EmcPmacroIbDdllLongDqsRank1_2;
	u32 EmcPmacroIbDdllLongDqsRank1_3;

	u32 EmcPmacroDdllLongCmd_0;
	u32 EmcPmacroDdllLongCmd_1;
	u32 EmcPmacroDdllLongCmd_2;
	u32 EmcPmacroDdllLongCmd_3;
	u32 EmcPmacroDdllLongCmd_4;
	u32 EmcPmacroDdllShortCmd_0;
	u32 EmcPmacroDdllShortCmd_1;
	u32 EmcPmacroDdllShortCmd_2;

	/*
	 * Specifies the delay after asserting CKE pin during a WarmBoot0
	 * sequence (in microseconds)
	 */
	u32 WarmBootWait;

	/* Specifies the value for EMC_ODT_WRITE */
	u32 EmcOdtWrite;

	/* Periodic ZQ calibration */

	/*
	 * Specifies the value for EMC_ZCAL_INTERVAL
	 * Value 0 disables ZQ calibration
	 */
	u32 EmcZcalInterval;
	/* Specifies the value for EMC_ZCAL_WAIT_CNT */
	u32 EmcZcalWaitCnt;
	/* Specifies the value for EMC_ZCAL_MRW_CMD */
	u32 EmcZcalMrwCmd;

	/* DRAM initialization sequence flow control */

	/* Specifies the MRS command value for resetting DLL */
	u32 EmcMrsResetDll;
	/* Specifies the command for ZQ initialization of device 0 */
	u32 EmcZcalInitDev0;
	/* Specifies the command for ZQ initialization of device 1 */
	u32 EmcZcalInitDev1;
	/*
	 * Specifies the wait time after programming a ZQ initialization
	 * command (in microseconds)
	 */
	u32 EmcZcalInitWait;
	/*
	 * Specifies the enable for ZQ calibration at cold boot [bit 0]
	 * and warm boot [bit 1]
	 */
	u32 EmcZcalWarmColdBootEnables;

	/*
	 * Specifies the MRW command to LPDDR2 for ZQ calibration
	 * on warmboot
	 */
	/* Is issued to both devices separately */
	u32 EmcMrwLpddr2ZcalWarmBoot;
	/*
	 * Specifies the ZQ command to DDR3 for ZQ calibration on warmboot
	 * Is issued to both devices separately
	 */
	u32 EmcZqCalDdr3WarmBoot;
	u32 EmcZqCalLpDdr4WarmBoot;
	/*
	 * Specifies the wait time for ZQ calibration on warmboot
	 * (in microseconds)
	 */
	u32 EmcZcalWarmBootWait;
	/*
	 * Specifies the enable for DRAM Mode Register programming
	 * at warm boot
	 */
	u32 EmcMrsWarmBootEnable;
	/*
	 * Specifies the wait time after sending an MRS DLL reset command
	 * in microseconds)
	 */
	u32 EmcMrsResetDllWait;
	/* Specifies the extra MRS command to initialize mode registers */
	u32 EmcMrsExtra;
	/* Specifies the extra MRS command at warm boot */
	u32 EmcWarmBootMrsExtra;
	/* Specifies the EMRS command to enable the DDR2 DLL */
	u32 EmcEmrsDdr2DllEnable;
	/* Specifies the MRS command to reset the DDR2 DLL */
	u32 EmcMrsDdr2DllReset;
	/* Specifies the EMRS command to set OCD calibration */
	u32 EmcEmrsDdr2OcdCalib;
	/*
	 * Specifies the wait between initializing DDR and setting OCD
	 * calibration (in microseconds)
	 */
	u32 EmcDdr2Wait;
	/* Specifies the value for EMC_CLKEN_OVERRIDE */
	u32 EmcClkenOverride;

	/*
	 * Specifies LOG2 of the extra refresh numbers after booting
	 * Program 0 to disable
	 */
	u32 EmcExtraRefreshNum;
	/* Specifies the master override for all EMC clocks */
	u32 EmcClkenOverrideAllWarmBoot;
	/* Specifies the master override for all MC clocks */
	u32 McClkenOverrideAllWarmBoot;
	/* Specifies digital dll period, choosing between 4 to 64 ms */
	u32 EmcCfgDigDllPeriodWarmBoot;

	/* Pad controls */

	/* Specifies the value for PMC_VDDP_SEL */
	u32 PmcVddpSel;
	/* Specifies the wait time after programming PMC_VDDP_SEL */
	u32 PmcVddpSelWait;
	/* Specifies the value for PMC_DDR_PWR */
	u32 PmcDdrPwr;
	/* Specifies the value for PMC_DDR_CFG */
	u32 PmcDdrCfg;
	/* Specifies the value for PMC_IO_DPD3_REQ */
	u32 PmcIoDpd3Req;
	/* Specifies the wait time after programming PMC_IO_DPD3_REQ */
	u32 PmcIoDpd3ReqWait;
	u32 PmcIoDpd4ReqWait;

	/* Specifies the value for PMC_REG_SHORT */
	u32 PmcRegShort;
	/* Specifies the value for PMC_NO_IOPOWER */
	u32 PmcNoIoPower;

	u32 PmcDdrCntrlWait;
	u32 PmcDdrCntrl;

	/* Specifies the value for EMC_ACPD_CONTROL */
	u32 EmcAcpdControl;

	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE_CFG */
	////u32 EmcSwizzleRank0ByteCfg;
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE0 */
	u32 EmcSwizzleRank0Byte0;
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE1 */
	u32 EmcSwizzleRank0Byte1;
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE2 */
	u32 EmcSwizzleRank0Byte2;
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE3 */
	u32 EmcSwizzleRank0Byte3;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE_CFG */
	////u32 EmcSwizzleRank1ByteCfg;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE0 */
	u32 EmcSwizzleRank1Byte0;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE1 */
	u32 EmcSwizzleRank1Byte1;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE2 */
	u32 EmcSwizzleRank1Byte2;
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE3 */
	u32 EmcSwizzleRank1Byte3;

	/* Specifies the value for EMC_TXDSRVTTGEN */
	u32 EmcTxdsrvttgen;

	/* Specifies the value for EMC_DATA_BRLSHFT_0 */
	u32 EmcDataBrlshft0;
	u32 EmcDataBrlshft1;

	u32 EmcDqsBrlshft0;
	u32 EmcDqsBrlshft1;

	u32 EmcCmdBrlshft0;
	u32 EmcCmdBrlshft1;
	u32 EmcCmdBrlshft2;
	u32 EmcCmdBrlshft3;

	u32 EmcQuseBrlshft0;
	u32 EmcQuseBrlshft1;
	u32 EmcQuseBrlshft2;
	u32 EmcQuseBrlshft3;

	u32 EmcDllCfg0;
	u32 EmcDllCfg1;

	u32 EmcPmcScratch1;
	u32 EmcPmcScratch2;
	u32 EmcPmcScratch3;

	u32 EmcPmacroPadCfgCtrl;

	u32 EmcPmacroVttgenCtrl0;
	u32 EmcPmacroVttgenCtrl1;
	u32 EmcPmacroVttgenCtrl2;

	u32 EmcPmacroBrickCtrlRfu1;
	u32 EmcPmacroCmdBrickCtrlFdpd;
	u32 EmcPmacroBrickCtrlRfu2;
	u32 EmcPmacroDataBrickCtrlFdpd;
	u32 EmcPmacroBgBiasCtrl0;
	u32 EmcPmacroDataPadRxCtrl;
	u32 EmcPmacroCmdPadRxCtrl;
	u32 EmcPmacroDataRxTermMode;
	u32 EmcPmacroCmdRxTermMode;
	u32 EmcPmacroDataPadTxCtrl;
	u32 EmcPmacroCommonPadTxCtrl;
	u32 EmcPmacroCmdPadTxCtrl;
	u32 EmcCfg3;

	u32 EmcPmacroTxPwrd0;
	u32 EmcPmacroTxPwrd1;
	u32 EmcPmacroTxPwrd2;
	u32 EmcPmacroTxPwrd3;
	u32 EmcPmacroTxPwrd4;
	u32 EmcPmacroTxPwrd5;

	u32 EmcConfigSampleDelay;

	u32 EmcPmacroBrickMapping0;
	u32 EmcPmacroBrickMapping1;
	u32 EmcPmacroBrickMapping2;

	u32 EmcPmacroTxSelClkSrc0;
	u32 EmcPmacroTxSelClkSrc1;
	u32 EmcPmacroTxSelClkSrc2;
	u32 EmcPmacroTxSelClkSrc3;
	u32 EmcPmacroTxSelClkSrc4;
	u32 EmcPmacroTxSelClkSrc5;

	u32 EmcPmacroDdllBypass;

	u32 EmcPmacroDdllPwrd0;
	u32 EmcPmacroDdllPwrd1;
	u32 EmcPmacroDdllPwrd2;

	u32 EmcPmacroCmdCtrl0;
	u32 EmcPmacroCmdCtrl1;
	u32 EmcPmacroCmdCtrl2;

	/* DRAM size information */

	/* Specifies the value for MC_EMEM_ADR_CFG */
	u32 McEmemAdrCfg;
	/* Specifies the value for MC_EMEM_ADR_CFG_DEV0 */
	u32 McEmemAdrCfgDev0;
	/* Specifies the value for MC_EMEM_ADR_CFG_DEV1 */
	u32 McEmemAdrCfgDev1;
	u32 McEmemAdrCfgChannelMask;

	/* Specifies the value for MC_EMEM_BANK_SWIZZLECfg0 */
	u32 McEmemAdrCfgBankMask0;
	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG1 */
	u32 McEmemAdrCfgBankMask1;
	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG2 */
	u32 McEmemAdrCfgBankMask2;

	/*
	 * Specifies the value for MC_EMEM_CFG which holds the external memory
	 * size (in KBytes)
	 */
	u32 McEmemCfg;

	/* MC arbitration configuration */

	/* Specifies the value for MC_EMEM_ARB_CFG */
	u32 McEmemArbCfg;
	/* Specifies the value for MC_EMEM_ARB_OUTSTANDING_REQ */
	u32 McEmemArbOutstandingReq;

	u32 McEmemArbRefpbHpCtrl;
	u32 McEmemArbRefpbBankCtrl;

	/* Specifies the value for MC_EMEM_ARB_TIMING_RCD */
	u32 McEmemArbTimingRcd;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RP */
	u32 McEmemArbTimingRp;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RC */
	u32 McEmemArbTimingRc;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RAS */
	u32 McEmemArbTimingRas;
	/* Specifies the value for MC_EMEM_ARB_TIMING_FAW */
	u32 McEmemArbTimingFaw;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RRD */
	u32 McEmemArbTimingRrd;
	/* Specifies the value for MC_EMEM_ARB_TIMING_RAP2PRE */
	u32 McEmemArbTimingRap2Pre;
	/* Specifies the value for MC_EMEM_ARB_TIMING_WAP2PRE */
	u32 McEmemArbTimingWap2Pre;
	/* Specifies the value for MC_EMEM_ARB_TIMING_R2R */
	u32 McEmemArbTimingR2R;
	/* Specifies the value for MC_EMEM_ARB_TIMING_W2W */
	u32 McEmemArbTimingW2W;
	/* Specifies the value for MC_EMEM_ARB_TIMING_R2W */
	u32 McEmemArbTimingR2W;
	/* Specifies the value for MC_EMEM_ARB_TIMING_W2R */
	u32 McEmemArbTimingW2R;

	u32 McEmemArbTimingRFCPB;

	/* Specifies the value for MC_EMEM_ARB_DA_TURNS */
	u32 McEmemArbDaTurns;
	/* Specifies the value for MC_EMEM_ARB_DA_COVERS */
	u32 McEmemArbDaCovers;
	/* Specifies the value for MC_EMEM_ARB_MISC0 */
	u32 McEmemArbMisc0;
	/* Specifies the value for MC_EMEM_ARB_MISC1 */
	u32 McEmemArbMisc1;
	u32 McEmemArbMisc2;

	/* Specifies the value for MC_EMEM_ARB_RING1_THROTTLE */
	u32 McEmemArbRing1Throttle;
	/* Specifies the value for MC_EMEM_ARB_OVERRIDE */
	u32 McEmemArbOverride;
	/* Specifies the value for MC_EMEM_ARB_OVERRIDE_1 */
	u32 McEmemArbOverride1;
	/* Specifies the value for MC_EMEM_ARB_RSV */
	u32 McEmemArbRsv;

	u32 McDaCfg0;
	u32 McEmemArbTimingCcdmw;

	/* Specifies the value for MC_CLKEN_OVERRIDE */
	u32 McClkenOverride;

	/* Specifies the value for MC_STAT_CONTROL */
	u32 McStatControl;

	/* Specifies the value for MC_VIDEO_PROTECT_BOM */
	u32 McVideoProtectBom;
	/* Specifies the value for MC_VIDEO_PROTECT_BOM_ADR_HI */
	u32 McVideoProtectBomAdrHi;
	/* Specifies the value for MC_VIDEO_PROTECT_SIZE_MB */
	u32 McVideoProtectSizeMb;
	/* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE */
	u32 McVideoProtectVprOverride;
	/* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE1 */
	u32 McVideoProtectVprOverride1;
	/* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_0 */
	u32 McVideoProtectGpuOverride0;
	/* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_1 */
	u32 McVideoProtectGpuOverride1;
	/* Specifies the value for MC_SEC_CARVEOUT_BOM */
	u32 McSecCarveoutBom;
	/* Specifies the value for MC_SEC_CARVEOUT_ADR_HI */
	u32 McSecCarveoutAdrHi;
	/* Specifies the value for MC_SEC_CARVEOUT_SIZE_MB */
	u32 McSecCarveoutSizeMb;
	/* Specifies the value for MC_VIDEO_PROTECT_REG_CTRL.
	   VIDEO_PROTECT_WRITEAccess */
	u32 McVideoProtectWriteAccess;
	/* Specifies the value for MC_SEC_CARVEOUT_REG_CTRL.
	   SEC_CARVEOUT_WRITEAccess */
	u32 McSecCarveoutProtectWriteAccess;

	/* Write-Protect Regions (WPR) */
	u32 McGeneralizedCarveout1Bom;
	u32 McGeneralizedCarveout1BomHi;
	u32 McGeneralizedCarveout1Size128kb;
	u32 McGeneralizedCarveout1Access0;
	u32 McGeneralizedCarveout1Access1;
	u32 McGeneralizedCarveout1Access2;
	u32 McGeneralizedCarveout1Access3;
	u32 McGeneralizedCarveout1Access4;
	u32 McGeneralizedCarveout1ForceInternalAccess0;
	u32 McGeneralizedCarveout1ForceInternalAccess1;
	u32 McGeneralizedCarveout1ForceInternalAccess2;
	u32 McGeneralizedCarveout1ForceInternalAccess3;
	u32 McGeneralizedCarveout1ForceInternalAccess4;
	u32 McGeneralizedCarveout1Cfg0;

	u32 McGeneralizedCarveout2Bom;
	u32 McGeneralizedCarveout2BomHi;
	u32 McGeneralizedCarveout2Size128kb;
	u32 McGeneralizedCarveout2Access0;
	u32 McGeneralizedCarveout2Access1;
	u32 McGeneralizedCarveout2Access2;
	u32 McGeneralizedCarveout2Access3;
	u32 McGeneralizedCarveout2Access4;
	u32 McGeneralizedCarveout2ForceInternalAccess0;
	u32 McGeneralizedCarveout2ForceInternalAccess1;
	u32 McGeneralizedCarveout2ForceInternalAccess2;
	u32 McGeneralizedCarveout2ForceInternalAccess3;
	u32 McGeneralizedCarveout2ForceInternalAccess4;
	u32 McGeneralizedCarveout2Cfg0;

	u32 McGeneralizedCarveout3Bom;
	u32 McGeneralizedCarveout3BomHi;
	u32 McGeneralizedCarveout3Size128kb;
	u32 McGeneralizedCarveout3Access0;
	u32 McGeneralizedCarveout3Access1;
	u32 McGeneralizedCarveout3Access2;
	u32 McGeneralizedCarveout3Access3;
	u32 McGeneralizedCarveout3Access4;
	u32 McGeneralizedCarveout3ForceInternalAccess0;
	u32 McGeneralizedCarveout3ForceInternalAccess1;
	u32 McGeneralizedCarveout3ForceInternalAccess2;
	u32 McGeneralizedCarveout3ForceInternalAccess3;
	u32 McGeneralizedCarveout3ForceInternalAccess4;
	u32 McGeneralizedCarveout3Cfg0;

	u32 McGeneralizedCarveout4Bom;
	u32 McGeneralizedCarveout4BomHi;
	u32 McGeneralizedCarveout4Size128kb;
	u32 McGeneralizedCarveout4Access0;
	u32 McGeneralizedCarveout4Access1;
	u32 McGeneralizedCarveout4Access2;
	u32 McGeneralizedCarveout4Access3;
	u32 McGeneralizedCarveout4Access4;
	u32 McGeneralizedCarveout4ForceInternalAccess0;
	u32 McGeneralizedCarveout4ForceInternalAccess1;
	u32 McGeneralizedCarveout4ForceInternalAccess2;
	u32 McGeneralizedCarveout4ForceInternalAccess3;
	u32 McGeneralizedCarveout4ForceInternalAccess4;
	u32 McGeneralizedCarveout4Cfg0;

	u32 McGeneralizedCarveout5Bom;
	u32 McGeneralizedCarveout5BomHi;
	u32 McGeneralizedCarveout5Size128kb;
	u32 McGeneralizedCarveout5Access0;
	u32 McGeneralizedCarveout5Access1;
	u32 McGeneralizedCarveout5Access2;
	u32 McGeneralizedCarveout5Access3;
	u32 McGeneralizedCarveout5Access4;
	u32 McGeneralizedCarveout5ForceInternalAccess0;
	u32 McGeneralizedCarveout5ForceInternalAccess1;
	u32 McGeneralizedCarveout5ForceInternalAccess2;
	u32 McGeneralizedCarveout5ForceInternalAccess3;
	u32 McGeneralizedCarveout5ForceInternalAccess4;
	u32 McGeneralizedCarveout5Cfg0;

	/* Specifies enable for CA training */
	u32 EmcCaTrainingEnable;

	/* Set if bit 6 select is greater than bit 7 select; uses aremc.
	   spec packet SWIZZLE_BIT6_GT_BIT7 */
	u32 SwizzleRankByteEncode;
	/* Specifies enable and offset for patched boot ROM write */
	u32 BootRomPatchControl;
	/* Specifies data for patched boot ROM write */
	u32 BootRomPatchData;

	/* Specifies the value for MC_MTS_CARVEOUT_BOM */
	u32 McMtsCarveoutBom;
	/* Specifies the value for MC_MTS_CARVEOUT_ADR_HI */
	u32 McMtsCarveoutAdrHi;
	/* Specifies the value for MC_MTS_CARVEOUT_SIZE_MB */
	u32 McMtsCarveoutSizeMb;
	/* Specifies the value for MC_MTS_CARVEOUT_REG_CTRL */
	u32 McMtsCarveoutRegCtrl;

	/* End */
};

#endif /* __SOC_NVIDIA_TEGRA210_SDRAM_PARAM_H__ */
