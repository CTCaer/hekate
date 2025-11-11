/*
 * Copyright (c) 2019-2025 CTCaer
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

#ifndef _EMC_T210_H_
#define _EMC_T210_H_

/* External Memory Controller registers */
#define EMC_INTSTATUS                                                0x0
#define EMC_INTMASK                                                  0x4
#define EMC_DBG                                                      0x8
#define EMC_CFG                                                      0xC
#define EMC_ADR_CFG                                                  0x10
#define EMC_REFCTRL                                                  0x20
#define EMC_PIN                                                      0x24
#define EMC_TIMING_CONTROL                                           0x28
#define EMC_RC                                                       0x2C
#define EMC_RFC                                                      0x30
#define EMC_RAS                                                      0x34
#define EMC_RP                                                       0x38
#define EMC_R2W                                                      0x3C
#define EMC_W2R                                                      0x40
#define EMC_R2P                                                      0x44
#define EMC_W2P                                                      0x48
#define EMC_RD_RCD                                                   0x4C
#define EMC_WR_RCD                                                   0x50
#define EMC_RRD                                                      0x54
#define EMC_REXT                                                     0x58
#define EMC_WDV                                                      0x5C
#define EMC_QUSE                                                     0x60
#define EMC_QRST                                                     0x64
#define EMC_QSAFE                                                    0x68
#define EMC_RDV                                                      0x6C
#define EMC_REFRESH                                                  0x70
#define EMC_BURST_REFRESH_NUM                                        0x74
#define EMC_PDEX2WR                                                  0x78
#define EMC_PDEX2RD                                                  0x7C
#define EMC_PCHG2PDEN                                                0x80
#define EMC_ACT2PDEN                                                 0x84
#define EMC_AR2PDEN                                                  0x88
#define EMC_RW2PDEN                                                  0x8C
#define EMC_TXSR                                                     0x90
#define EMC_TCKE                                                     0x94
#define EMC_TFAW                                                     0x98
#define EMC_TRPAB                                                    0x9C
#define EMC_TCLKSTABLE                                               0xA0
#define EMC_TCLKSTOP                                                 0xA4
#define EMC_TREFBW                                                   0xA8
#define EMC_TPPD                                                     0xAC
#define EMC_ODT_WRITE                                                0xB0
#define EMC_PDEX2MRR                                                 0xB4
#define EMC_WEXT                                                     0xB8
#define EMC_RFC_SLR                                                  0xC0
#define EMC_MRS_WAIT_CNT2                                            0xC4
#define EMC_MRS_WAIT_CNT                                             0xC8
#define EMC_MRS                                                      0xCC
#define EMC_EMRS                                                     0xD0
#define EMC_REF                                                      0xD4
#define EMC_PRE                                                      0xD8
#define EMC_NOP                                                      0xDC
#define EMC_SELF_REF                                                 0xE0
#define EMC_DPD                                                      0xE4
#define EMC_MRW                                                      0xE8
#define EMC_MRR                                                      0xEC
#define EMC_CMDQ                                                     0xF0
#define EMC_MC2EMCQ                                                  0xF4
#define EMC_FBIO_SPARE                                               0x100
#define EMC_FBIO_CFG5                                                0x104
#define EMC_PDEX2CKE                                                 0x118
#define EMC_CKE2PDEN                                                 0x11C
#define EMC_CFG_RSV                                                  0x120
#define EMC_ACPD_CONTROL                                             0x124
#define EMC_MPC                                                      0x128
#define EMC_EMRS2                                                    0x12C
#define EMC_EMRS3                                                    0x130
#define EMC_MRW2                                                     0x134
#define EMC_MRW3                                                     0x138
#define EMC_MRW4                                                     0x13C
#define EMC_CLKEN_OVERRIDE                                           0x140
#define EMC_R2R                                                      0x144
#define EMC_W2W                                                      0x148
#define EMC_EINPUT                                                   0x14C
#define EMC_EINPUT_DURATION                                          0x150
#define EMC_PUTERM_EXTRA                                             0x154
#define EMC_TCKESR                                                   0x158
#define EMC_TPD                                                      0x15C
#define EMC_STAT_CONTROL                                             0x160
#define EMC_STAT_STATUS                                              0x164
#define EMC_STAT_DRAM_CLOCK_LIMIT_LO                                 0x19C
#define EMC_STAT_DRAM_CLOCK_LIMIT_HI                                 0x1A0
#define EMC_STAT_DRAM_CLOCKS_LO                                      0x1A4
#define EMC_STAT_DRAM_CLOCKS_HI                                      0x1A8
#define EMC_STAT_DRAM_DEV0_ACTIVATE_CNT_LO                           0x1AC
#define EMC_STAT_DRAM_DEV0_ACTIVATE_CNT_HI                           0x1B0
#define EMC_STAT_DRAM_DEV0_READ_CNT_LO                               0x1B4
#define EMC_STAT_DRAM_DEV0_READ_CNT_HI                               0x1B8
#define EMC_STAT_DRAM_DEV0_READ8_CNT_LO                              0x1BC
#define EMC_STAT_DRAM_DEV0_READ8_CNT_HI                              0x1C0
#define EMC_STAT_DRAM_DEV0_WRITE_CNT_LO                              0x1C4
#define EMC_STAT_DRAM_DEV0_WRITE_CNT_HI                              0x1C8
#define EMC_STAT_DRAM_DEV0_WRITE8_CNT_LO                             0x1CC
#define EMC_STAT_DRAM_DEV0_WRITE8_CNT_HI                             0x1D0
#define EMC_STAT_DRAM_DEV0_REF_CNT_LO                                0x1D4
#define EMC_STAT_DRAM_DEV0_REF_CNT_HI                                0x1D8
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO   0x1DC
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI   0x1E0
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO   0x1E4
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI   0x1E8
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO   0x1EC
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI   0x1F0
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO   0x1F4
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI   0x1F8
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO 0x1FC
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI 0x200
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO 0x204
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI 0x208
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO 0x20C
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI 0x210
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO 0x214
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI 0x218
#define EMC_STAT_DRAM_DEV0_SR_CKE_EQ0_CLKS_LO                        0x21C
#define EMC_STAT_DRAM_DEV0_SR_CKE_EQ0_CLKS_HI                        0x220
#define EMC_STAT_DRAM_DEV0_DSR                                       0x224
#define EMC_STAT_DRAM_DEV1_ACTIVATE_CNT_LO                           0x228
#define EMC_STAT_DRAM_DEV1_ACTIVATE_CNT_HI                           0x22C
#define EMC_STAT_DRAM_DEV1_READ_CNT_LO                               0x230
#define EMC_STAT_DRAM_DEV1_READ_CNT_HI                               0x234
#define EMC_STAT_DRAM_DEV1_READ8_CNT_LO                              0x238
#define EMC_STAT_DRAM_DEV1_READ8_CNT_HI                              0x23C
#define EMC_STAT_DRAM_DEV1_WRITE_CNT_LO                              0x240
#define EMC_STAT_DRAM_DEV1_WRITE_CNT_HI                              0x244
#define EMC_STAT_DRAM_DEV1_WRITE8_CNT_LO                             0x248
#define EMC_STAT_DRAM_DEV1_WRITE8_CNT_HI                             0x24C
#define EMC_STAT_DRAM_DEV1_REF_CNT_LO                                0x250
#define EMC_STAT_DRAM_DEV1_REF_CNT_HI                                0x254
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO   0x258
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI   0x25C
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO   0x260
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI   0x264
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO   0x268
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI   0x26C
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO   0x270
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI   0x274
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO 0x278
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI 0x27C
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO 0x280
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI 0x284
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO 0x288
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI 0x28C
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO 0x290
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI 0x294
#define EMC_STAT_DRAM_DEV1_SR_CKE_EQ0_CLKS_LO                        0x298
#define EMC_STAT_DRAM_DEV1_SR_CKE_EQ0_CLKS_HI                        0x29C
#define EMC_STAT_DRAM_DEV1_DSR                                       0x2A0
#define EMC_AUTO_CAL_CONFIG                                          0x2A4
#define EMC_AUTO_CAL_INTERVAL                                        0x2A8
#define EMC_AUTO_CAL_STATUS                                          0x2AC
#define EMC_REQ_CTRL                                                 0x2B0
#define EMC_EMC_STATUS                                               0x2B4
#define  EMC_STATUS_MRR_DIVLD                                         BIT(20)
#define EMC_CFG_2                                                    0x2B8
#define EMC_CFG_DIG_DLL                                              0x2BC
#define EMC_CFG_DIG_DLL_PERIOD                                       0x2C0
#define EMC_DIG_DLL_STATUS                                           0x2C4
#define EMC_CFG_DIG_DLL_1                                            0x2C8
#define EMC_RDV_MASK                                                 0x2CC
#define EMC_WDV_MASK                                                 0x2D0
#define EMC_RDV_EARLY_MASK                                           0x2D4
#define EMC_RDV_EARLY                                                0x2D8
#define EMC_AUTO_CAL_CONFIG8                                         0x2DC
#define EMC_ZCAL_INTERVAL                                            0x2E0
#define EMC_ZCAL_WAIT_CNT                                            0x2E4
#define EMC_ZCAL_MRW_CMD                                             0x2E8
#define EMC_ZQ_CAL                                                   0x2EC
#define EMC_XM2COMPPADCTRL3                                          0x2F4
#define EMC_AUTO_CAL_VREF_SEL_0                                      0x2F8
#define EMC_AUTO_CAL_VREF_SEL_1                                      0x300
#define EMC_XM2COMPPADCTRL                                           0x30C
#define EMC_FDPD_CTRL_DQ                                             0x310
#define EMC_FDPD_CTRL_CMD                                            0x314
#define EMC_PMACRO_CMD_BRICK_CTRL_FDPD                               0x318
#define EMC_PMACRO_DATA_BRICK_CTRL_FDPD                              0x31C
#define EMC_SCRATCH0                                                 0x324
#define EMC_PMACRO_BRICK_CTRL_RFU1                                   0x330
#define EMC_PMACRO_BRICK_CTRL_RFU2                                   0x334
#define EMC_CMD_MAPPING_CMD0_0                                       0x380
#define EMC_CMD_MAPPING_CMD0_1                                       0x384
#define EMC_CMD_MAPPING_CMD0_2                                       0x388
#define EMC_CMD_MAPPING_CMD1_0                                       0x38C
#define EMC_CMD_MAPPING_CMD1_1                                       0x390
#define EMC_CMD_MAPPING_CMD1_2                                       0x394
#define EMC_CMD_MAPPING_CMD2_0                                       0x398
#define EMC_CMD_MAPPING_CMD2_1                                       0x39C
#define EMC_CMD_MAPPING_CMD2_2                                       0x3A0
#define EMC_CMD_MAPPING_CMD3_0                                       0x3A4
#define EMC_CMD_MAPPING_CMD3_1                                       0x3A8
#define EMC_CMD_MAPPING_CMD3_2                                       0x3AC
#define EMC_CMD_MAPPING_BYTE                                         0x3B0
#define EMC_TR_TIMING_0                                              0x3B4
#define EMC_TR_CTRL_0                                                0x3B8
#define EMC_TR_CTRL_1                                                0x3BC
#define EMC_SWITCH_BACK_CTRL                                         0x3C0
#define EMC_TR_RDV                                                   0x3C4
#define EMC_STALL_THEN_EXE_BEFORE_CLKCHANGE                          0x3C8
#define EMC_STALL_THEN_EXE_AFTER_CLKCHANGE                           0x3CC
#define EMC_UNSTALL_RW_AFTER_CLKCHANGE                               0x3D0
#define EMC_AUTO_CAL_STATUS2                                         0x3D4
#define EMC_SEL_DPD_CTRL                                             0x3D8
#define EMC_PRE_REFRESH_REQ_CNT                                      0x3DC
#define EMC_DYN_SELF_REF_CONTROL                                     0x3E0
#define EMC_TXSRDLL                                                  0x3E4
#define EMC_CCFIFO_ADDR                                              0x3E8
#define EMC_CCFIFO_DATA                                              0x3EC
#define EMC_CCFIFO_STATUS                                            0x3F0
#define EMC_TR_QPOP                                                  0x3F4
#define EMC_TR_RDV_MASK                                              0x3F8
#define EMC_TR_QSAFE                                                 0x3FC
#define EMC_TR_QRST                                                  0x400
#define EMC_SWIZZLE_RANK0_BYTE0                                      0x404
#define EMC_SWIZZLE_RANK0_BYTE1                                      0x408
#define EMC_SWIZZLE_RANK0_BYTE2                                      0x40C
#define EMC_SWIZZLE_RANK0_BYTE3                                      0x410
#define EMC_SWIZZLE_RANK1_BYTE0                                      0x418
#define EMC_SWIZZLE_RANK1_BYTE1                                      0x41C
#define EMC_SWIZZLE_RANK1_BYTE2                                      0x420
#define EMC_SWIZZLE_RANK1_BYTE3                                      0x424
#define EMC_ISSUE_QRST                                               0x428
#define EMC_PMC_SCRATCH1                                             0x440
#define EMC_PMC_SCRATCH2                                             0x444
#define EMC_PMC_SCRATCH3                                             0x448
#define EMC_AUTO_CAL_CONFIG2                                         0x458
#define EMC_AUTO_CAL_CONFIG3                                         0x45C
#define EMC_TR_DVFS                                                  0x460
#define EMC_AUTO_CAL_CHANNEL                                         0x464
#define EMC_IBDLY                                                    0x468
#define EMC_OBDLY                                                    0x46C
#define EMC_TXDSRVTTGEN                                              0x480
#define EMC_WE_DURATION                                              0x48C
#define EMC_WS_DURATION                                              0x490
#define EMC_WEV                                                      0x494
#define EMC_WSV                                                      0x498
#define EMC_CFG_3                                                    0x49C
#define EMC_MRW5                                                     0x4A0
#define EMC_MRW6                                                     0x4A4
#define EMC_MRW7                                                     0x4A8
#define EMC_MRW8                                                     0x4AC
#define EMC_MRW9                                                     0x4B0
#define EMC_MRW10                                                    0x4B4
#define EMC_MRW11                                                    0x4B8
#define EMC_MRW12                                                    0x4BC
#define EMC_MRW13                                                    0x4C0
#define EMC_MRW14                                                    0x4C4
#define EMC_MRW15                                                    0x4D0
#define EMC_CFG_SYNC                                                 0x4D4
#define EMC_FDPD_CTRL_CMD_NO_RAMP                                    0x4D8
#define EMC_WDV_CHK                                                  0x4E0
#define EMC_CFG_PIPE_2                                               0x554
#define EMC_CFG_PIPE_CLK                                             0x558
#define EMC_CFG_PIPE_1                                               0x55C
#define EMC_CFG_PIPE                                                 0x560
#define EMC_QPOP                                                     0x564
#define EMC_QUSE_WIDTH                                               0x568
#define EMC_PUTERM_WIDTH                                             0x56C
#define EMC_AUTO_CAL_CONFIG7                                         0x574
#define EMC_XM2COMPPADCTRL2                                          0x578
#define EMC_COMP_PAD_SW_CTRL                                         0x57C
#define EMC_REFCTRL2                                                 0x580
#define EMC_FBIO_CFG7                                                0x584
#define EMC_DATA_BRLSHFT_0                                           0x588
#define EMC_DATA_BRLSHFT_1                                           0x58C
#define EMC_RFCPB                                                    0x590
#define EMC_DQS_BRLSHFT_0                                            0x594
#define EMC_DQS_BRLSHFT_1                                            0x598
#define EMC_CMD_BRLSHFT_0                                            0x59C
#define EMC_CMD_BRLSHFT_1                                            0x5A0
#define EMC_CMD_BRLSHFT_2                                            0x5A4
#define EMC_CMD_BRLSHFT_3                                            0x5A8
#define EMC_QUSE_BRLSHFT_0                                           0x5AC
#define EMC_AUTO_CAL_CONFIG4                                         0x5B0
#define EMC_AUTO_CAL_CONFIG5                                         0x5B4
#define EMC_QUSE_BRLSHFT_1                                           0x5B8
#define EMC_QUSE_BRLSHFT_2                                           0x5BC
#define EMC_CCDMW                                                    0x5C0
#define EMC_QUSE_BRLSHFT_3                                           0x5C4
#define EMC_FBIO_CFG8                                                0x5C8
#define EMC_AUTO_CAL_CONFIG6                                         0x5CC
#define EMC_PROTOBIST_CONFIG_ADR_1                                   0x5D0
#define EMC_PROTOBIST_CONFIG_ADR_2                                   0x5D4
#define EMC_PROTOBIST_MISC                                           0x5D8
#define EMC_PROTOBIST_WDATA_LOWER                                    0x5DC
#define EMC_PROTOBIST_WDATA_UPPER                                    0x5E0
#define EMC_DLL_CFG_0                                                0x5E4
#define EMC_DLL_CFG_1                                                0x5E8
#define EMC_PROTOBIST_RDATA                                          0x5EC
#define EMC_CONFIG_SAMPLE_DELAY                                      0x5F0
#define EMC_CFG_UPDATE                                               0x5F4
#define EMC_PMACRO_QUSE_DDLL_RANK0_0                                 0x600
#define EMC_PMACRO_QUSE_DDLL_RANK0_1                                 0x604
#define EMC_PMACRO_QUSE_DDLL_RANK0_2                                 0x608
#define EMC_PMACRO_QUSE_DDLL_RANK0_3                                 0x60C
#define EMC_PMACRO_QUSE_DDLL_RANK0_4                                 0x610
#define EMC_PMACRO_QUSE_DDLL_RANK0_5                                 0x614
#define EMC_PMACRO_QUSE_DDLL_RANK1_0                                 0x620
#define EMC_PMACRO_QUSE_DDLL_RANK1_1                                 0x624
#define EMC_PMACRO_QUSE_DDLL_RANK1_2                                 0x628
#define EMC_PMACRO_QUSE_DDLL_RANK1_3                                 0x62C
#define EMC_PMACRO_QUSE_DDLL_RANK1_4                                 0x630
#define EMC_PMACRO_QUSE_DDLL_RANK1_5                                 0x634
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0                           0x640
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1                           0x644
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2                           0x648
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3                           0x64C
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4                           0x650
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5                           0x654
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0                           0x660
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1                           0x664
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2                           0x668
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3                           0x66C
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_4                           0x670
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_5                           0x674
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_0                          0x680
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_1                          0x684
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_2                          0x688
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_3                          0x68C
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_4                          0x690
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_5                          0x694
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_0                          0x6A0
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_1                          0x6A4
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_2                          0x6A8
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_3                          0x6AC
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_4                          0x6B0
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_5                          0x6B4
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0                          0x6C0
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1                          0x6C4
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2                          0x6C8
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3                          0x6CC
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_4                          0x6D0
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_5                          0x6D4
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0                          0x6E0
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1                          0x6E4
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2                          0x6E8
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3                          0x6EC
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_4                          0x6F0
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_5                          0x6F4
#define EMC_PMACRO_AUTOCAL_CFG_0                                     0x700
#define EMC_PMACRO_AUTOCAL_CFG_1                                     0x704
#define EMC_PMACRO_AUTOCAL_CFG_2                                     0x708
#define EMC_PMACRO_TX_PWRD_0                                         0x720
#define EMC_PMACRO_TX_PWRD_1                                         0x724
#define EMC_PMACRO_TX_PWRD_2                                         0x728
#define EMC_PMACRO_TX_PWRD_3                                         0x72C
#define EMC_PMACRO_TX_PWRD_4                                         0x730
#define EMC_PMACRO_TX_PWRD_5                                         0x734
#define EMC_PMACRO_TX_SEL_CLK_SRC_0                                  0x740
#define EMC_PMACRO_TX_SEL_CLK_SRC_1                                  0x744
#define EMC_PMACRO_TX_SEL_CLK_SRC_2                                  0x748
#define EMC_PMACRO_TX_SEL_CLK_SRC_3                                  0x74C
#define EMC_PMACRO_TX_SEL_CLK_SRC_4                                  0x750
#define EMC_PMACRO_TX_SEL_CLK_SRC_5                                  0x754
#define EMC_PMACRO_DDLL_BYPASS                                       0x760
#define EMC_PMACRO_DDLL_PWRD_0                                       0x770
#define EMC_PMACRO_DDLL_PWRD_1                                       0x774
#define EMC_PMACRO_DDLL_PWRD_2                                       0x778
#define EMC_PMACRO_CMD_CTRL_0                                        0x780
#define EMC_PMACRO_CMD_CTRL_1                                        0x784
#define EMC_PMACRO_CMD_CTRL_2                                        0x788
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0                    0x800
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1                    0x804
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2                    0x808
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_3                    0x80C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0                    0x810
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1                    0x814
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2                    0x818
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_3                    0x81C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0                    0x820
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1                    0x824
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2                    0x828
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_3                    0x82C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0                    0x830
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1                    0x834
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2                    0x838
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_3                    0x83C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0                    0x840
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1                    0x844
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2                    0x848
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_3                    0x84C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0                    0x850
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1                    0x854
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2                    0x858
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_3                    0x85C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0                    0x860
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1                    0x864
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2                    0x868
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_3                    0x86C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0                    0x870
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1                    0x874
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2                    0x878
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_3                    0x87C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0                     0x880
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1                     0x884
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2                     0x888
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_3                     0x88C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0                     0x890
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1                     0x894
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2                     0x898
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_3                     0x89C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0                     0x8A0
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1                     0x8A4
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2                     0x8A8
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_3                     0x8AC
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0                     0x8B0
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1                     0x8B4
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2                     0x8B8
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_3                     0x8BC
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0                    0x900
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1                    0x904
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2                    0x908
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_3                    0x90C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0                    0x910
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1                    0x914
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2                    0x918
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_3                    0x91C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0                    0x920
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1                    0x924
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2                    0x928
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_3                    0x92C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0                    0x930
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1                    0x934
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2                    0x938
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_3                    0x93C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0                    0x940
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1                    0x944
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2                    0x948
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_3                    0x94C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0                    0x950
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1                    0x954
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2                    0x958
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_3                    0x95C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0                    0x960
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1                    0x964
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2                    0x968
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_3                    0x96C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0                    0x970
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1                    0x974
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2                    0x978
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_3                    0x97C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_0                     0x980
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_1                     0x984
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_2                     0x988
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_3                     0x98C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_0                     0x990
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_1                     0x994
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_2                     0x998
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_3                     0x99C
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_0                     0x9A0
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_1                     0x9A4
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_2                     0x9A8
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_3                     0x9AC
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_0                     0x9B0
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_1                     0x9B4
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_2                     0x9B8
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_3                     0x9BC
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0                    0xA00
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1                    0xA04
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2                    0xA08
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0                    0xA10
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1                    0xA14
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2                    0xA18
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0                    0xA20
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1                    0xA24
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2                    0xA28
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0                    0xA30
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1                    0xA34
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2                    0xA38
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0                    0xA40
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1                    0xA44
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2                    0xA48
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0                    0xA50
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1                    0xA54
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2                    0xA58
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0                    0xA60
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1                    0xA64
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2                    0xA68
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0                    0xA70
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1                    0xA74
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2                    0xA78
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD0_0                     0xA80
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD0_1                     0xA84
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD0_2                     0xA88
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD1_0                     0xA90
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD1_1                     0xA94
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD1_2                     0xA98
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD2_0                     0xAA0
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD2_1                     0xAA4
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD2_2                     0xAA8
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD3_0                     0xAB0
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD3_1                     0xAB4
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD3_2                     0xAB8
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0                    0xB00
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1                    0xB04
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2                    0xB08
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0                    0xB10
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1                    0xB14
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2                    0xB18
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0                    0xB20
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1                    0xB24
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2                    0xB28
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0                    0xB30
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1                    0xB34
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2                    0xB38
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0                    0xB40
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1                    0xB44
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2                    0xB48
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0                    0xB50
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1                    0xB54
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2                    0xB58
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0                    0xB60
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1                    0xB64
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2                    0xB68
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0                    0xB70
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1                    0xB74
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2                    0xB78
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD0_0                     0xB80
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD0_1                     0xB84
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD0_2                     0xB88
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD1_0                     0xB90
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD1_1                     0xB94
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD1_2                     0xB98
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD2_0                     0xBA0
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD2_1                     0xBA4
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD2_2                     0xBA8
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD3_0                     0xBB0
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD3_1                     0xBB4
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD3_2                     0xBB8
#define EMC_PMACRO_IB_VREF_DQ_0                                      0xBE0
#define EMC_PMACRO_IB_VREF_DQ_1                                      0xBE4
#define EMC_PMACRO_IB_VREF_DQ_2                                      0xBE8
#define EMC_PMACRO_IB_VREF_DQS_0                                     0xBF0
#define EMC_PMACRO_IB_VREF_DQS_1                                     0xBF4
#define EMC_PMACRO_IB_VREF_DQS_2                                     0xBF8
#define EMC_PMACRO_DDLL_LONG_CMD_0                                   0xC00
#define EMC_PMACRO_DDLL_LONG_CMD_1                                   0xC04
#define EMC_PMACRO_DDLL_LONG_CMD_2                                   0xC08
#define EMC_PMACRO_DDLL_LONG_CMD_3                                   0xC0C
#define EMC_PMACRO_DDLL_LONG_CMD_4                                   0xC10
#define EMC_PMACRO_DDLL_LONG_CMD_5                                   0xC14
#define EMC_PMACRO_DDLL_SHORT_CMD_0                                  0xC20
#define EMC_PMACRO_DDLL_SHORT_CMD_1                                  0xC24
#define EMC_PMACRO_DDLL_SHORT_CMD_2                                  0xC28
#define EMC_PMACRO_CFG_PM_GLOBAL_0                                   0xC30
#define EMC_PMACRO_VTTGEN_CTRL_0                                     0xC34
#define EMC_PMACRO_VTTGEN_CTRL_1                                     0xC38
#define EMC_PMACRO_BG_BIAS_CTRL_0                                    0xC3C
#define EMC_PMACRO_PAD_CFG_CTRL                                      0xC40
#define EMC_PMACRO_ZCTRL                                             0xC44
#define EMC_PMACRO_RX_TERM                                           0xC48
#define EMC_PMACRO_CMD_TX_DRV                                        0xC4C
#define EMC_PMACRO_CMD_PAD_RX_CTRL                                   0xC50
#define EMC_PMACRO_DATA_PAD_RX_CTRL                                  0xC54
#define EMC_PMACRO_CMD_RX_TERM_MODE                                  0xC58
#define EMC_PMACRO_DATA_RX_TERM_MODE                                 0xC5C
#define EMC_PMACRO_CMD_PAD_TX_CTRL                                   0xC60
#define EMC_PMACRO_DATA_PAD_TX_CTRL                                  0xC64
#define EMC_PMACRO_COMMON_PAD_TX_CTRL                                0xC68 // Only in T210.
#define EMC_PMACRO_DQ_TX_DRV                                         0xC70
#define EMC_PMACRO_CA_TX_DRV                                         0xC74
#define EMC_PMACRO_AUTOCAL_CFG_COMMON                                0xC78
#define EMC_PMACRO_BRICK_MAPPING_0                                   0xC80
#define EMC_PMACRO_BRICK_MAPPING_1                                   0xC84
#define EMC_PMACRO_BRICK_MAPPING_2                                   0xC88
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO     0xC8C
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI     0xC90
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO     0xC94
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI     0xC98
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO     0xC9C
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI     0xCA0
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO     0xCA4
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI     0xCA8
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO   0xCAC
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI   0xCB0
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO   0xCB4
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI   0xCB8
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO   0xCBC
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI   0xCC0
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO   0xCC4
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI   0xCC8
#define EMC_STAT_DRAM_IO_SR_CKE_EQ0_CLKS_LO                          0xCCC
#define EMC_STAT_DRAM_IO_SR_CKE_EQ0_CLKS_HI                          0xCD0
#define EMC_STAT_DRAM_IO_DSR                                         0xCD4
#define EMC_PMACRO_DDLLCAL_CAL                                       0xCE0 // Only in T210.
#define EMC_PMACRO_DDLL_OFFSET                                       0xCE4
#define EMC_PMACRO_DDLL_PERIODIC_OFFSET                              0xCE8
#define EMC_PMACRO_VTTGEN_CTRL_2                                     0xCF0
#define EMC_PMACRO_IB_RXRT                                           0xCF4
#define EMC_PMACRO_TRAINING_CTRL_0                                   0xCF8
#define EMC_PMACRO_TRAINING_CTRL_1                                   0xCFC
#define EMC_TRAINING_CMD                                             0xE00
#define EMC_TRAINING_CTRL                                            0xE04
#define EMC_TRAINING_STATUS                                          0xE08
#define EMC_TRAINING_QUSE_CORS_CTRL                                  0xE0C
#define EMC_TRAINING_QUSE_FINE_CTRL                                  0xE10
#define EMC_TRAINING_QUSE_CTRL_MISC                                  0xE14
#define EMC_TRAINING_WRITE_FINE_CTRL                                 0xE18
#define EMC_TRAINING_WRITE_CTRL_MISC                                 0xE1C
#define EMC_TRAINING_WRITE_VREF_CTRL                                 0xE20
#define EMC_TRAINING_READ_FINE_CTRL                                  0xE24
#define EMC_TRAINING_READ_CTRL_MISC                                  0xE28
#define EMC_TRAINING_READ_VREF_CTRL                                  0xE2C
#define EMC_TRAINING_CA_FINE_CTRL                                    0xE30
#define EMC_TRAINING_CA_CTRL_MISC                                    0xE34
#define EMC_TRAINING_CA_CTRL_MISC1                                   0xE38
#define EMC_TRAINING_CA_VREF_CTRL                                    0xE3C
#define EMC_TRAINING_CA_TADR_CTRL                                    0xE40
#define EMC_TRAINING_SETTLE                                          0xE44
#define EMC_TRAINING_DEBUG_CTRL                                      0xE48
#define EMC_TRAINING_DEBUG_DQ0                                       0xE4C
#define EMC_TRAINING_DEBUG_DQ1                                       0xE50
#define EMC_TRAINING_DEBUG_DQ2                                       0xE54
#define EMC_TRAINING_DEBUG_DQ3                                       0xE58
#define EMC_TRAINING_MPC                                             0xE5C
#define EMC_TRAINING_PATRAM_CTRL                                     0xE60
#define EMC_TRAINING_PATRAM_DQ                                       0xE64
#define EMC_TRAINING_PATRAM_DMI                                      0xE68
#define EMC_TRAINING_VREF_SETTLE                                     0xE6C
#define EMC_TRAINING_RW_EYE_CENTER_IB_BYTE0                          0xE70
#define EMC_TRAINING_RW_EYE_CENTER_IB_BYTE1                          0xE74
#define EMC_TRAINING_RW_EYE_CENTER_IB_BYTE2                          0xE78
#define EMC_TRAINING_RW_EYE_CENTER_IB_BYTE3                          0xE7C
#define EMC_TRAINING_RW_EYE_CENTER_IB_MISC                           0xE80
#define EMC_TRAINING_RW_EYE_CENTER_OB_BYTE0                          0xE84
#define EMC_TRAINING_RW_EYE_CENTER_OB_BYTE1                          0xE88
#define EMC_TRAINING_RW_EYE_CENTER_OB_BYTE2                          0xE8C
#define EMC_TRAINING_RW_EYE_CENTER_OB_BYTE3                          0xE90
#define EMC_TRAINING_RW_EYE_CENTER_OB_MISC                           0xE94
#define EMC_TRAINING_RW_OFFSET_IB_BYTE0                              0xE98
#define EMC_TRAINING_RW_OFFSET_IB_BYTE1                              0xE9C
#define EMC_TRAINING_RW_OFFSET_IB_BYTE2                              0xEA0
#define EMC_TRAINING_RW_OFFSET_IB_BYTE3                              0xEA4
#define EMC_TRAINING_RW_OFFSET_IB_MISC                               0xEA8
#define EMC_TRAINING_RW_OFFSET_OB_BYTE0                              0xEAC
#define EMC_TRAINING_RW_OFFSET_OB_BYTE1                              0xEB0
#define EMC_TRAINING_RW_OFFSET_OB_BYTE2                              0xEB4
#define EMC_TRAINING_RW_OFFSET_OB_BYTE3                              0xEB8
#define EMC_TRAINING_RW_OFFSET_OB_MISC                               0xEBC
#define EMC_TRAINING_OPT_CA_VREF                                     0xEC0
#define EMC_TRAINING_OPT_DQ_OB_VREF                                  0xEC4
#define EMC_TRAINING_OPT_DQ_IB_VREF_RANK0                            0xEC8
#define EMC_TRAINING_OPT_DQ_IB_VREF_RANK1                            0xECC
#define EMC_TRAINING_QUSE_VREF_CTRL                                  0xED0
#define EMC_TRAINING_OPT_DQS_IB_VREF_RANK0                           0xED4
#define EMC_TRAINING_OPT_DQS_IB_VREF_RANK1                           0xED8
#define EMC_TRAINING_DRAMC_TIMING                                    0xEDC

/* T210B01 only registers */
#define EMC_TRTM_B01                                                 0xBC
#define EMC_TWTM_B01                                                 0xF8
#define EMC_TRATM_B01                                                0xFC
#define EMC_TWATM_B01                                                0x108
#define EMC_TR2REF_B01                                               0x10C
#define EMC_PMACRO_DATA_PI_CTRL_B01                                  0x110
#define EMC_PMACRO_CMD_PI_CTRL_B01                                   0x114
#define EMC_PMACRO_PMU_CTRL_B01                                      0x304
#define EMC_PMACRO_XM2COMP_PMU_CTRL_B01                              0x308
#define EMC_AUTO_CAL_CONFIG9_B01                                     0x42C
#define EMC_PMACRO_DDLLCAL_EN_B01                                    0x44C
#define EMC_PMACRO_DLL_CFG_0_B01                                     0x5E4
#define EMC_PMACRO_DLL_CFG_1_B01                                     0x5E8
#define EMC_PMACRO_DLL_CFG_2_B01                                     0x5F8
#define EMC_PMACRO_DSR_VTTGEN_CTRL_0_B01                             0xC6C
#define EMC_PMACRO_DDLLCAL_CAL_0_B01                                 0xD00
#define EMC_PMACRO_DDLLCAL_CAL_1_B01                                 0xD04
#define EMC_PMACRO_DDLLCAL_CAL_2_B01                                 0xD08
#define EMC_PMACRO_DDLLCAL_CAL_3_B01                                 0xD0C
#define EMC_PMACRO_DDLLCAL_CAL_4_B01                                 0xD10
#define EMC_PMACRO_DDLLCAL_CAL_5_B01                                 0xD14
#define EMC_PMACRO_DIG_DLL_STATUS_0_B01                              0xD20
#define EMC_PMACRO_DIG_DLL_STATUS_1_B01                              0xD24
#define EMC_PMACRO_DIG_DLL_STATUS_2_B01                              0xD28
#define EMC_PMACRO_DIG_DLL_STATUS_3_B01                              0xD2C
#define EMC_PMACRO_DIG_DLL_STATUS_4_B01                              0xD30
#define EMC_PMACRO_DIG_DLL_STATUS_5_B01                              0xD34
#define EMC_PMACRO_PERBIT_FGCG_CTRL_0_B01                            0xD40
#define EMC_PMACRO_PERBIT_FGCG_CTRL_1_B01                            0xD44
#define EMC_PMACRO_PERBIT_FGCG_CTRL_2_B01                            0xD48
#define EMC_PMACRO_PERBIT_FGCG_CTRL_3_B01                            0xD4C
#define EMC_PMACRO_PERBIT_FGCG_CTRL_4_B01                            0xD50
#define EMC_PMACRO_PERBIT_FGCG_CTRL_5_B01                            0xD54
#define EMC_PMACRO_PERBIT_RFU_CTRL_0_B01                             0xD60
#define EMC_PMACRO_PERBIT_RFU_CTRL_1_B01                             0xD64
#define EMC_PMACRO_PERBIT_RFU_CTRL_2_B01                             0xD68
#define EMC_PMACRO_PERBIT_RFU_CTRL_3_B01                             0xD6C
#define EMC_PMACRO_PERBIT_RFU_CTRL_4_B01                             0xD70
#define EMC_PMACRO_PERBIT_RFU_CTRL_5_B01                             0xD74
#define EMC_PMACRO_PERBIT_RFU1_CTRL_0_B01                            0xD80
#define EMC_PMACRO_PERBIT_RFU1_CTRL_1_B01                            0xD84
#define EMC_PMACRO_PERBIT_RFU1_CTRL_2_B01                            0xD88
#define EMC_PMACRO_PERBIT_RFU1_CTRL_3_B01                            0xD8C
#define EMC_PMACRO_PERBIT_RFU1_CTRL_4_B01                            0xD90
#define EMC_PMACRO_PERBIT_RFU1_CTRL_5_B01                            0xD94
#define EMC_PMACRO_PMU_OUT_EOFF1_0_B01                               0xDA0
#define EMC_PMACRO_PMU_OUT_EOFF1_1_B01                               0xDA4
#define EMC_PMACRO_PMU_OUT_EOFF1_2_B01                               0xDA8
#define EMC_PMACRO_PMU_OUT_EOFF1_3_B01                               0xDAC
#define EMC_PMACRO_PMU_OUT_EOFF1_4_B01                               0xDB0
#define EMC_PMACRO_PMU_OUT_EOFF1_5_B01                               0xDB4
#define EMC_PMACRO_COMP_PMU_OUT_B01                                  0xDC0

#define EMC_STATUS_UPDATE_TIMEOUT 1000

typedef enum _emc_mr_t
{
	MR0_FEAT    = 0,
	MR4_TEMP    = 4,
	MR5_MAN_ID  = 5,
	MR6_REV_ID1 = 6,
	MR7_REV_ID2 = 7,
	MR8_DENSITY = 8,
} emc_mr_t;

enum
{
	EMC_CHAN0 = 0,
	EMC_CHAN1 = 1
};

typedef struct _emc_mr_chip_data_t
{
	// Device 0.
	u8 rank0_ch0;
	u8 rank0_ch1;

	// Device 1.
	u8 rank1_ch0;
	u8 rank1_ch1;
} emc_mr_chip_data_t;

typedef struct _emc_mr_data_t
{
	emc_mr_chip_data_t chip0;
	emc_mr_chip_data_t chip1;
} emc_mr_data_t;

typedef struct _emc_regs_t210_t {
/* 0x000 */	u32 emc_intstatus;
/* 0x004 */	u32 emc_intmask;
/* 0x008 */	u32 emc_dbg;
/* 0x00c */	u32 emc_cfg;
/* 0x010 */	u32 emc_adr_cfg;
/* 0x014 */	u32 rsvd_014[3];
/* 0x020 */	u32 emc_refctrl;
/* 0x024 */	u32 emc_pin;
/* 0x028 */	u32 emc_timing_control;
/* 0x02c */	u32 emc_rc;
/* 0x030 */	u32 emc_rfc;
/* 0x034 */	u32 emc_ras;
/* 0x038 */	u32 emc_rp;
/* 0x03c */	u32 emc_r2w;
/* 0x040 */	u32 emc_w2r;
/* 0x044 */	u32 emc_r2p;
/* 0x048 */	u32 emc_w2p;
/* 0x04c */	u32 emc_rd_rcd;
/* 0x050 */	u32 emc_wr_rcd;
/* 0x054 */	u32 emc_rrd;
/* 0x058 */	u32 emc_rext;
/* 0x05c */	u32 emc_wdv;
/* 0x060 */	u32 emc_quse;
/* 0x064 */	u32 emc_qrst;
/* 0x068 */	u32 emc_qsafe;
/* 0x06c */	u32 emc_rdv;
/* 0x070 */	u32 emc_refresh;
/* 0x074 */	u32 emc_burst_refresh_num;
/* 0x078 */	u32 emc_pdex2wr;
/* 0x07c */	u32 emc_pdex2rd;
/* 0x080 */	u32 emc_pchg2pden;
/* 0x084 */	u32 emc_act2pden;
/* 0x088 */	u32 emc_ar2pden;
/* 0x08c */	u32 emc_rw2pden;
/* 0x090 */	u32 emc_txsr;
/* 0x094 */	u32 emc_tcke;
/* 0x098 */	u32 emc_tfaw;
/* 0x09c */	u32 emc_trpab;
/* 0x0a0 */	u32 emc_tclkstable;
/* 0x0a4 */	u32 emc_tclkstop;
/* 0x0a8 */	u32 emc_trefbw;
/* 0x0ac */	u32 emc_tppd;
/* 0x0b0 */	u32 emc_odt_write;
/* 0x0b4 */	u32 emc_pdex2mrr;
/* 0x0b8 */	u32 emc_wext;
/* 0x0bc */	u32 emc_trtm_b01;
/* 0x0c0 */	u32 emc_rfc_slr;
/* 0x0c4 */	u32 emc_mrs_wait_cnt2;
/* 0x0c8 */	u32 emc_mrs_wait_cnt;
/* 0x0cc */	u32 emc_mrs;
/* 0x0d0 */	u32 emc_emrs;
/* 0x0d4 */	u32 emc_ref;
/* 0x0d8 */	u32 emc_pre;
/* 0x0dc */	u32 emc_nop;
/* 0x0e0 */	u32 emc_self_ref;
/* 0x0e4 */	u32 emc_dpd;
/* 0x0e8 */	u32 emc_mrw;
/* 0x0ec */	u32 emc_mrr;
/* 0x0f0 */	u32 emc_cmdq;
/* 0x0f4 */	u32 emc_mc2emcq;
/* 0x0f8 */	u32 emc_twtm_b01;
/* 0x0fc */	u32 emc_tratm_b01;
/* 0x100 */	u32 emc_fbio_spare;
/* 0x104 */	u32 emc_fbio_cfg5;
/* 0x108 */	u32 emc_twatm_b01;
/* 0x10c */	u32 emc_tr2ref_b01;
/* 0x110 */	u32 emc_pmacro_data_pi_ctrl_b01;
/* 0x114 */	u32 emc_pmacro_cmd_pi_ctrl_b01;
/* 0x118 */	u32 emc_pdex2cke;
/* 0x11c */	u32 emc_cke2pden;
/* 0x120 */	u32 emc_cfg_rsv;
/* 0x124 */	u32 emc_acpd_control;
/* 0x128 */	u32 emc_mpc;
/* 0x12c */	u32 emc_emrs2;
/* 0x130 */	u32 emc_emrs3;
/* 0x134 */	u32 emc_mrw2;
/* 0x138 */	u32 emc_mrw3;
/* 0x13c */	u32 emc_mrw4;
/* 0x140 */	u32 emc_clken_override;
/* 0x144 */	u32 emc_r2r;
/* 0x148 */	u32 emc_w2w;
/* 0x14c */	u32 emc_einput;
/* 0x150 */	u32 emc_einput_duration;
/* 0x154 */	u32 emc_puterm_extra;
/* 0x158 */	u32 emc_tckesr;
/* 0x15c */	u32 emc_tpd;
/* 0x160 */	u32 emc_stat_control;
/* 0x164 */	u32 emc_stat_status;
/* 0x168 */	u32 rsvd_168[13];
/* 0x19c */	u32 emc_stat_dram_clock_limit_lo;
/* 0x1a0 */	u32 emc_stat_dram_clock_limit_hi;
/* 0x1a4 */	u32 emc_stat_dram_clocks_lo;
/* 0x1a8 */	u32 emc_stat_dram_clocks_hi;
/* 0x1ac */	u32 emc_stat_dram_dev0_activate_cnt_lo;
/* 0x1b0 */	u32 emc_stat_dram_dev0_activate_cnt_hi;
/* 0x1b4 */	u32 emc_stat_dram_dev0_read_cnt_lo;
/* 0x1b8 */	u32 emc_stat_dram_dev0_read_cnt_hi;
/* 0x1bc */	u32 emc_stat_dram_dev0_read8_cnt_lo;
/* 0x1c0 */	u32 emc_stat_dram_dev0_read8_cnt_hi;
/* 0x1c4 */	u32 emc_stat_dram_dev0_write_cnt_lo;
/* 0x1c8 */	u32 emc_stat_dram_dev0_write_cnt_hi;
/* 0x1cc */	u32 emc_stat_dram_dev0_write8_cnt_lo;
/* 0x1d0 */	u32 emc_stat_dram_dev0_write8_cnt_hi;
/* 0x1d4 */	u32 emc_stat_dram_dev0_ref_cnt_lo;
/* 0x1d8 */	u32 emc_stat_dram_dev0_ref_cnt_hi;
/* 0x1dc */	u32 emc_stat_dram_dev0_extclks_cke_eq0_no_banks_active_clks_lo;
/* 0x1e0 */	u32 emc_stat_dram_dev0_extclks_cke_eq0_no_banks_active_clks_hi;
/* 0x1e4 */	u32 emc_stat_dram_dev0_clkstop_cke_eq0_no_banks_active_clks_lo;
/* 0x1e8 */	u32 emc_stat_dram_dev0_clkstop_cke_eq0_no_banks_active_clks_hi;
/* 0x1ec */	u32 emc_stat_dram_dev0_extclks_cke_eq1_no_banks_active_clks_lo;
/* 0x1f0 */	u32 emc_stat_dram_dev0_extclks_cke_eq1_no_banks_active_clks_hi;
/* 0x1f4 */	u32 emc_stat_dram_dev0_clkstop_cke_eq1_no_banks_active_clks_lo;
/* 0x1f8 */	u32 emc_stat_dram_dev0_clkstop_cke_eq1_no_banks_active_clks_hi;
/* 0x1fc */	u32 emc_stat_dram_dev0_extclks_cke_eq0_some_banks_active_clks_lo;
/* 0x200 */	u32 emc_stat_dram_dev0_extclks_cke_eq0_some_banks_active_clks_hi;
/* 0x204 */	u32 emc_stat_dram_dev0_clkstop_cke_eq0_some_banks_active_clks_lo;
/* 0x208 */	u32 emc_stat_dram_dev0_clkstop_cke_eq0_some_banks_active_clks_hi;
/* 0x20c */	u32 emc_stat_dram_dev0_extclks_cke_eq1_some_banks_active_clks_lo;
/* 0x210 */	u32 emc_stat_dram_dev0_extclks_cke_eq1_some_banks_active_clks_hi;
/* 0x214 */	u32 emc_stat_dram_dev0_clkstop_cke_eq1_some_banks_active_clks_lo;
/* 0x218 */	u32 emc_stat_dram_dev0_clkstop_cke_eq1_some_banks_active_clks_hi;
/* 0x21c */	u32 emc_stat_dram_dev0_sr_cke_eq0_clks_lo;
/* 0x220 */	u32 emc_stat_dram_dev0_sr_cke_eq0_clks_hi;
/* 0x224 */	u32 emc_stat_dram_dev0_dsr;
/* 0x228 */	u32 emc_stat_dram_dev1_activate_cnt_lo;
/* 0x22c */	u32 emc_stat_dram_dev1_activate_cnt_hi;
/* 0x230 */	u32 emc_stat_dram_dev1_read_cnt_lo;
/* 0x234 */	u32 emc_stat_dram_dev1_read_cnt_hi;
/* 0x238 */	u32 emc_stat_dram_dev1_read8_cnt_lo;
/* 0x23c */	u32 emc_stat_dram_dev1_read8_cnt_hi;
/* 0x240 */	u32 emc_stat_dram_dev1_write_cnt_lo;
/* 0x244 */	u32 emc_stat_dram_dev1_write_cnt_hi;
/* 0x248 */	u32 emc_stat_dram_dev1_write8_cnt_lo;
/* 0x24c */	u32 emc_stat_dram_dev1_write8_cnt_hi;
/* 0x250 */	u32 emc_stat_dram_dev1_ref_cnt_lo;
/* 0x254 */	u32 emc_stat_dram_dev1_ref_cnt_hi;
/* 0x258 */	u32 emc_stat_dram_dev1_extclks_cke_eq0_no_banks_active_clks_lo;
/* 0x25c */	u32 emc_stat_dram_dev1_extclks_cke_eq0_no_banks_active_clks_hi;
/* 0x260 */	u32 emc_stat_dram_dev1_clkstop_cke_eq0_no_banks_active_clks_lo;
/* 0x264 */	u32 emc_stat_dram_dev1_clkstop_cke_eq0_no_banks_active_clks_hi;
/* 0x268 */	u32 emc_stat_dram_dev1_extclks_cke_eq1_no_banks_active_clks_lo;
/* 0x26c */	u32 emc_stat_dram_dev1_extclks_cke_eq1_no_banks_active_clks_hi;
/* 0x270 */	u32 emc_stat_dram_dev1_clkstop_cke_eq1_no_banks_active_clks_lo;
/* 0x274 */	u32 emc_stat_dram_dev1_clkstop_cke_eq1_no_banks_active_clks_hi;
/* 0x278 */	u32 emc_stat_dram_dev1_extclks_cke_eq0_some_banks_active_clks_lo;
/* 0x27c */	u32 emc_stat_dram_dev1_extclks_cke_eq0_some_banks_active_clks_hi;
/* 0x280 */	u32 emc_stat_dram_dev1_clkstop_cke_eq0_some_banks_active_clks_lo;
/* 0x284 */	u32 emc_stat_dram_dev1_clkstop_cke_eq0_some_banks_active_clks_hi;
/* 0x288 */	u32 emc_stat_dram_dev1_extclks_cke_eq1_some_banks_active_clks_lo;
/* 0x28c */	u32 emc_stat_dram_dev1_extclks_cke_eq1_some_banks_active_clks_hi;
/* 0x290 */	u32 emc_stat_dram_dev1_clkstop_cke_eq1_some_banks_active_clks_lo;
/* 0x294 */	u32 emc_stat_dram_dev1_clkstop_cke_eq1_some_banks_active_clks_hi;
/* 0x298 */	u32 emc_stat_dram_dev1_sr_cke_eq0_clks_lo;
/* 0x29c */	u32 emc_stat_dram_dev1_sr_cke_eq0_clks_hi;
/* 0x2a0 */	u32 emc_stat_dram_dev1_dsr;
/* 0x2a4 */	u32 emc_auto_cal_config;
/* 0x2a8 */	u32 emc_auto_cal_interval;
/* 0x2ac */	u32 emc_auto_cal_status;
/* 0x2b0 */	u32 emc_req_ctrl;
/* 0x2b4 */	u32 emc_emc_status;
/* 0x2b8 */	u32 emc_cfg_2;
/* 0x2bc */	u32 emc_cfg_dig_dll;
/* 0x2c0 */	u32 emc_cfg_dig_dll_period;
/* 0x2c4 */	u32 emc_dig_dll_status;
/* 0x2c8 */	u32 emc_cfg_dig_dll_1;
/* 0x2cc */	u32 emc_rdv_mask;
/* 0x2d0 */	u32 emc_wdv_mask;
/* 0x2d4 */	u32 emc_rdv_early_mask;
/* 0x2d8 */	u32 emc_rdv_early;
/* 0x2dc */	u32 emc_auto_cal_config8;
/* 0x2e0 */	u32 emc_zcal_interval;
/* 0x2e4 */	u32 emc_zcal_wait_cnt;
/* 0x2e8 */	u32 emc_zcal_mrw_cmd;
/* 0x2ec */	u32 emc_zq_cal;
/* 0x2f0 */	u32 rsvd_2f0;
/* 0x2f4 */	u32 emc_xm2comppadctrl3;
/* 0x2f8 */	u32 emc_auto_cal_vref_sel_0;
/* 0x2fc */	u32 rsvd_2fc;
/* 0x300 */	u32 emc_auto_cal_vref_sel_1;
/* 0x304 */	u32 emc_pmacro_pmu_ctrl_b01;
/* 0x308 */	u32 emc_pmacro_xm2comp_pmu_ctrl_b01;
/* 0x30c */	u32 emc_xm2comppadctrl;
/* 0x310 */	u32 emc_fdpd_ctrl_dq;
/* 0x314 */	u32 emc_fdpd_ctrl_cmd;
/* 0x318 */	u32 emc_pmacro_cmd_brick_ctrl_fdpd;
/* 0x31c */	u32 emc_pmacro_data_brick_ctrl_fdpd;
/* 0x320 */	u32 rsvd_320;
/* 0x324 */	u32 emc_scratch0;
/* 0x328 */	u32 rsvd_328[2];
/* 0x330 */	u32 emc_pmacro_brick_ctrl_rfu1;
/* 0x334 */	u32 emc_pmacro_brick_ctrl_rfu2;
/* 0x338 */	u32 rsvd_338[18];
/* 0x380 */	u32 emc_cmd_mapping_cmd0_0;
/* 0x384 */	u32 emc_cmd_mapping_cmd0_1;
/* 0x388 */	u32 emc_cmd_mapping_cmd0_2;
/* 0x38c */	u32 emc_cmd_mapping_cmd1_0;
/* 0x390 */	u32 emc_cmd_mapping_cmd1_1;
/* 0x394 */	u32 emc_cmd_mapping_cmd1_2;
/* 0x398 */	u32 emc_cmd_mapping_cmd2_0;
/* 0x39c */	u32 emc_cmd_mapping_cmd2_1;
/* 0x3a0 */	u32 emc_cmd_mapping_cmd2_2;
/* 0x3a4 */	u32 emc_cmd_mapping_cmd3_0;
/* 0x3a8 */	u32 emc_cmd_mapping_cmd3_1;
/* 0x3ac */	u32 emc_cmd_mapping_cmd3_2;
/* 0x3b0 */	u32 emc_cmd_mapping_byte;
/* 0x3b4 */	u32 emc_tr_timing_0;
/* 0x3b8 */	u32 emc_tr_ctrl_0;
/* 0x3bc */	u32 emc_tr_ctrl_1;
/* 0x3c0 */	u32 emc_switch_back_ctrl;
/* 0x3c4 */	u32 emc_tr_rdv;
/* 0x3c8 */	u32 emc_stall_then_exe_before_clkchange;
/* 0x3cc */	u32 emc_stall_then_exe_after_clkchange;
/* 0x3d0 */	u32 emc_unstall_rw_after_clkchange;
/* 0x3d4 */	u32 emc_auto_cal_status2;
/* 0x3d8 */	u32 emc_sel_dpd_ctrl;
/* 0x3dc */	u32 emc_pre_refresh_req_cnt;
/* 0x3e0 */	u32 emc_dyn_self_ref_control;
/* 0x3e4 */	u32 emc_txsrdll;
/* 0x3e8 */	u32 emc_ccfifo_addr;
/* 0x3ec */	u32 emc_ccfifo_data;
/* 0x3f0 */	u32 emc_ccfifo_status;
/* 0x3f4 */	u32 emc_tr_qpop;
/* 0x3f8 */	u32 emc_tr_rdv_mask;
/* 0x3fc */	u32 emc_tr_qsafe;
/* 0x400 */	u32 emc_tr_qrst;
/* 0x404 */	u32 emc_swizzle_rank0_byte0;
/* 0x408 */	u32 emc_swizzle_rank0_byte1;
/* 0x40c */	u32 emc_swizzle_rank0_byte2;
/* 0x410 */	u32 emc_swizzle_rank0_byte3;
/* 0x414 */	u32 rsvd_414;
/* 0x418 */	u32 emc_swizzle_rank1_byte0;
/* 0x41c */	u32 emc_swizzle_rank1_byte1;
/* 0x420 */	u32 emc_swizzle_rank1_byte2;
/* 0x424 */	u32 emc_swizzle_rank1_byte3;
/* 0x428 */	u32 emc_issue_qrst;
/* 0x42c */	u32 emc_auto_cal_config9_b01;
/* 0x430 */	u32 rsvd_430[4];
/* 0x440 */	u32 emc_pmc_scratch1;
/* 0x444 */	u32 emc_pmc_scratch2;
/* 0x448 */	u32 emc_pmc_scratch3;
/* 0x44c */	u32 emc_pmacro_ddllcal_en_b01;
/* 0x450 */	u32 rsvd_450[2];
/* 0x458 */	u32 emc_auto_cal_config2;
/* 0x45c */	u32 emc_auto_cal_config3;
/* 0x460 */	u32 emc_tr_dvfs;
/* 0x464 */	u32 emc_auto_cal_channel;
/* 0x468 */	u32 emc_ibdly;
/* 0x46c */	u32 emc_obdly;
/* 0x470 */	u32 rsvd_470[4];
/* 0x480 */	u32 emc_txdsrvttgen;
/* 0x484 */	u32 rsvd_484[2];
/* 0x48c */	u32 emc_we_duration;
/* 0x490 */	u32 emc_ws_duration;
/* 0x494 */	u32 emc_wev;
/* 0x498 */	u32 emc_wsv;
/* 0x49c */	u32 emc_cfg_3;
/* 0x4a0 */	u32 emc_mrw5;
/* 0x4a4 */	u32 emc_mrw6;
/* 0x4a8 */	u32 emc_mrw7;
/* 0x4ac */	u32 emc_mrw8;
/* 0x4b0 */	u32 emc_mrw9;
/* 0x4b4 */	u32 emc_mrw10;
/* 0x4b8 */	u32 emc_mrw11;
/* 0x4bc */	u32 emc_mrw12;
/* 0x4c0 */	u32 emc_mrw13;
/* 0x4c4 */	u32 emc_mrw14;
/* 0x4c8 */	u32 rsvd_4c8[2];
/* 0x4d0 */	u32 emc_mrw15;
/* 0x4d4 */	u32 emc_cfg_sync;
/* 0x4d8 */	u32 emc_fdpd_ctrl_cmd_no_ramp;
/* 0x4dc */	u32 rsvd_4dc;
/* 0x4e0 */	u32 emc_wdv_chk;
/* 0x4e4 */	u32 rsvd_4e4[28];
/* 0x554 */	u32 emc_cfg_pipe_2;
/* 0x558 */	u32 emc_cfg_pipe_clk;
/* 0x55c */	u32 emc_cfg_pipe_1;
/* 0x560 */	u32 emc_cfg_pipe;
/* 0x564 */	u32 emc_qpop;
/* 0x568 */	u32 emc_quse_width;
/* 0x56c */	u32 emc_puterm_width;
/* 0x570 */	u32 rsvd_570;
/* 0x574 */	u32 emc_auto_cal_config7;
/* 0x578 */	u32 emc_xm2comppadctrl2;
/* 0x57c */	u32 emc_comp_pad_sw_ctrl;
/* 0x580 */	u32 emc_refctrl2;
/* 0x584 */	u32 emc_fbio_cfg7;
/* 0x588 */	u32 emc_data_brlshft_0;
/* 0x58c */	u32 emc_data_brlshft_1;
/* 0x590 */	u32 emc_rfcpb;
/* 0x594 */	u32 emc_dqs_brlshft_0;
/* 0x598 */	u32 emc_dqs_brlshft_1;
/* 0x59c */	u32 emc_cmd_brlshft_0;
/* 0x5a0 */	u32 emc_cmd_brlshft_1;
/* 0x5a4 */	u32 emc_cmd_brlshft_2;
/* 0x5a8 */	u32 emc_cmd_brlshft_3;
/* 0x5ac */	u32 emc_quse_brlshft_0;
/* 0x5b0 */	u32 emc_auto_cal_config4;
/* 0x5b4 */	u32 emc_auto_cal_config5;
/* 0x5b8 */	u32 emc_quse_brlshft_1;
/* 0x5bc */	u32 emc_quse_brlshft_2;
/* 0x5c0 */	u32 emc_ccdmw;
/* 0x5c4 */	u32 emc_quse_brlshft_3;
/* 0x5c8 */	u32 emc_fbio_cfg8;
/* 0x5cc */	u32 emc_auto_cal_config6;
/* 0x5d0 */	u32 emc_protobist_config_adr_1;
/* 0x5d4 */	u32 emc_protobist_config_adr_2;
/* 0x5d8 */	u32 emc_protobist_misc;
/* 0x5dc */	u32 emc_protobist_wdata_lower;
/* 0x5e0 */	u32 emc_protobist_wdata_upper;
	union {
/* 0x5e4 */	u32 emc_dll_cfg_0;
/* 0x5e4 */	u32 emc_pmacro_dll_cfg_0_b01;
	};
	union {
/* 0x5e8 */	u32 emc_dll_cfg_1;
/* 0x5e8 */	u32 emc_pmacro_dll_cfg_1_b01;
	};
/* 0x5ec */	u32 emc_protobist_rdata;
/* 0x5f0 */	u32 emc_config_sample_delay;
/* 0x5f4 */	u32 emc_cfg_update;
/* 0x5f8 */	u32 emc_pmacro_dll_cfg_2_b01;
/* 0x5fc */	u32 rsvd_5fc;
/* 0x600 */	u32 emc_pmacro_quse_ddll_rank0_0;
/* 0x604 */	u32 emc_pmacro_quse_ddll_rank0_1;
/* 0x608 */	u32 emc_pmacro_quse_ddll_rank0_2;
/* 0x60c */	u32 emc_pmacro_quse_ddll_rank0_3;
/* 0x610 */	u32 emc_pmacro_quse_ddll_rank0_4;
/* 0x614 */	u32 emc_pmacro_quse_ddll_rank0_5;
/* 0x618 */	u32 rsvd_618[2];
/* 0x620 */	u32 emc_pmacro_quse_ddll_rank1_0;
/* 0x624 */	u32 emc_pmacro_quse_ddll_rank1_1;
/* 0x628 */	u32 emc_pmacro_quse_ddll_rank1_2;
/* 0x62c */	u32 emc_pmacro_quse_ddll_rank1_3;
/* 0x630 */	u32 emc_pmacro_quse_ddll_rank1_4;
/* 0x634 */	u32 emc_pmacro_quse_ddll_rank1_5;
/* 0x638 */	u32 rsvd_638[2];
/* 0x640 */	u32 emc_pmacro_ob_ddll_long_dq_rank0_0;
/* 0x644 */	u32 emc_pmacro_ob_ddll_long_dq_rank0_1;
/* 0x648 */	u32 emc_pmacro_ob_ddll_long_dq_rank0_2;
/* 0x64c */	u32 emc_pmacro_ob_ddll_long_dq_rank0_3;
/* 0x650 */	u32 emc_pmacro_ob_ddll_long_dq_rank0_4;
/* 0x654 */	u32 emc_pmacro_ob_ddll_long_dq_rank0_5;
/* 0x658 */	u32 rsvd_658[2];
/* 0x660 */	u32 emc_pmacro_ob_ddll_long_dq_rank1_0;
/* 0x664 */	u32 emc_pmacro_ob_ddll_long_dq_rank1_1;
/* 0x668 */	u32 emc_pmacro_ob_ddll_long_dq_rank1_2;
/* 0x66c */	u32 emc_pmacro_ob_ddll_long_dq_rank1_3;
/* 0x670 */	u32 emc_pmacro_ob_ddll_long_dq_rank1_4;
/* 0x674 */	u32 emc_pmacro_ob_ddll_long_dq_rank1_5;
/* 0x678 */	u32 rsvd_678[2];
/* 0x680 */	u32 emc_pmacro_ob_ddll_long_dqs_rank0_0;
/* 0x684 */	u32 emc_pmacro_ob_ddll_long_dqs_rank0_1;
/* 0x688 */	u32 emc_pmacro_ob_ddll_long_dqs_rank0_2;
/* 0x68c */	u32 emc_pmacro_ob_ddll_long_dqs_rank0_3;
/* 0x690 */	u32 emc_pmacro_ob_ddll_long_dqs_rank0_4;
/* 0x694 */	u32 emc_pmacro_ob_ddll_long_dqs_rank0_5;
/* 0x698 */	u32 rsvd_698[2];
/* 0x6a0 */	u32 emc_pmacro_ob_ddll_long_dqs_rank1_0;
/* 0x6a4 */	u32 emc_pmacro_ob_ddll_long_dqs_rank1_1;
/* 0x6a8 */	u32 emc_pmacro_ob_ddll_long_dqs_rank1_2;
/* 0x6ac */	u32 emc_pmacro_ob_ddll_long_dqs_rank1_3;
/* 0x6b0 */	u32 emc_pmacro_ob_ddll_long_dqs_rank1_4;
/* 0x6b4 */	u32 emc_pmacro_ob_ddll_long_dqs_rank1_5;
/* 0x6b8 */	u32 rsvd_6b8[2];
/* 0x6c0 */	u32 emc_pmacro_ib_ddll_long_dqs_rank0_0;
/* 0x6c4 */	u32 emc_pmacro_ib_ddll_long_dqs_rank0_1;
/* 0x6c8 */	u32 emc_pmacro_ib_ddll_long_dqs_rank0_2;
/* 0x6cc */	u32 emc_pmacro_ib_ddll_long_dqs_rank0_3;
/* 0x6d0 */	u32 emc_pmacro_ib_ddll_long_dqs_rank0_4;
/* 0x6d4 */	u32 emc_pmacro_ib_ddll_long_dqs_rank0_5;
/* 0x6d8 */	u32 rsvd_6d8[2];
/* 0x6e0 */	u32 emc_pmacro_ib_ddll_long_dqs_rank1_0;
/* 0x6e4 */	u32 emc_pmacro_ib_ddll_long_dqs_rank1_1;
/* 0x6e8 */	u32 emc_pmacro_ib_ddll_long_dqs_rank1_2;
/* 0x6ec */	u32 emc_pmacro_ib_ddll_long_dqs_rank1_3;
/* 0x6f0 */	u32 emc_pmacro_ib_ddll_long_dqs_rank1_4;
/* 0x6f4 */	u32 emc_pmacro_ib_ddll_long_dqs_rank1_5;
/* 0x6f8 */	u32 rsvd_6f8[2];
/* 0x700 */	u32 emc_pmacro_autocal_cfg_0;
/* 0x704 */	u32 emc_pmacro_autocal_cfg_1;
/* 0x708 */	u32 emc_pmacro_autocal_cfg_2;
/* 0x70c */	u32 rsvd_70c[5];
/* 0x720 */	u32 emc_pmacro_tx_pwrd_0;
/* 0x724 */	u32 emc_pmacro_tx_pwrd_1;
/* 0x728 */	u32 emc_pmacro_tx_pwrd_2;
/* 0x72c */	u32 emc_pmacro_tx_pwrd_3;
/* 0x730 */	u32 emc_pmacro_tx_pwrd_4;
/* 0x734 */	u32 emc_pmacro_tx_pwrd_5;
/* 0x738 */	u32 rsvd_738[2];
/* 0x740 */	u32 emc_pmacro_tx_sel_clk_src_0;
/* 0x744 */	u32 emc_pmacro_tx_sel_clk_src_1;
/* 0x748 */	u32 emc_pmacro_tx_sel_clk_src_2;
/* 0x74c */	u32 emc_pmacro_tx_sel_clk_src_3;
/* 0x750 */	u32 emc_pmacro_tx_sel_clk_src_4;
/* 0x754 */	u32 emc_pmacro_tx_sel_clk_src_5;
/* 0x758 */	u32 rsvd_758[2];
/* 0x760 */	u32 emc_pmacro_ddll_bypass;
/* 0x764 */	u32 rsvd_764[3];
/* 0x770 */	u32 emc_pmacro_ddll_pwrd_0;
/* 0x774 */	u32 emc_pmacro_ddll_pwrd_1;
/* 0x778 */	u32 emc_pmacro_ddll_pwrd_2;
/* 0x77c */	u32 rsvd_77c;
/* 0x780 */	u32 emc_pmacro_cmd_ctrl_0;
/* 0x784 */	u32 emc_pmacro_cmd_ctrl_1;
/* 0x788 */	u32 emc_pmacro_cmd_ctrl_2;
/* 0x78c */	u32 rsvd_78c[29];
/* 0x800 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte0_0;
/* 0x804 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte0_1;
/* 0x808 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte0_2;
/* 0x80c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte0_3;
/* 0x810 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte1_0;
/* 0x814 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte1_1;
/* 0x818 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte1_2;
/* 0x81c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte1_3;
/* 0x820 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte2_0;
/* 0x824 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte2_1;
/* 0x828 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte2_2;
/* 0x82c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte2_3;
/* 0x830 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte3_0;
/* 0x834 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte3_1;
/* 0x838 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte3_2;
/* 0x83c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte3_3;
/* 0x840 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte4_0;
/* 0x844 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte4_1;
/* 0x848 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte4_2;
/* 0x84c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte4_3;
/* 0x850 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte5_0;
/* 0x854 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte5_1;
/* 0x858 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte5_2;
/* 0x85c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte5_3;
/* 0x860 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte6_0;
/* 0x864 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte6_1;
/* 0x868 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte6_2;
/* 0x86c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte6_3;
/* 0x870 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte7_0;
/* 0x874 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte7_1;
/* 0x878 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte7_2;
/* 0x87c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_byte7_3;
/* 0x880 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd0_0;
/* 0x884 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd0_1;
/* 0x888 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd0_2;
/* 0x88c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd0_3;
/* 0x890 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd1_0;
/* 0x894 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd1_1;
/* 0x898 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd1_2;
/* 0x89c */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd1_3;
/* 0x8a0 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd2_0;
/* 0x8a4 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd2_1;
/* 0x8a8 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd2_2;
/* 0x8ac */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd2_3;
/* 0x8b0 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd3_0;
/* 0x8b4 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd3_1;
/* 0x8b8 */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd3_2;
/* 0x8bc */	u32 emc_pmacro_ob_ddll_short_dq_rank0_cmd3_3;
/* 0x8c0 */	u32 rsvd_8c0[16];
/* 0x900 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte0_0;
/* 0x904 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte0_1;
/* 0x908 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte0_2;
/* 0x90c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte0_3;
/* 0x910 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte1_0;
/* 0x914 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte1_1;
/* 0x918 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte1_2;
/* 0x91c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte1_3;
/* 0x920 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte2_0;
/* 0x924 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte2_1;
/* 0x928 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte2_2;
/* 0x92c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte2_3;
/* 0x930 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte3_0;
/* 0x934 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte3_1;
/* 0x938 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte3_2;
/* 0x93c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte3_3;
/* 0x940 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte4_0;
/* 0x944 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte4_1;
/* 0x948 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte4_2;
/* 0x94c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte4_3;
/* 0x950 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte5_0;
/* 0x954 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte5_1;
/* 0x958 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte5_2;
/* 0x95c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte5_3;
/* 0x960 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte6_0;
/* 0x964 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte6_1;
/* 0x968 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte6_2;
/* 0x96c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte6_3;
/* 0x970 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte7_0;
/* 0x974 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte7_1;
/* 0x978 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte7_2;
/* 0x97c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_byte7_3;
/* 0x980 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd0_0;
/* 0x984 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd0_1;
/* 0x988 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd0_2;
/* 0x98c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd0_3;
/* 0x990 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd1_0;
/* 0x994 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd1_1;
/* 0x998 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd1_2;
/* 0x99c */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd1_3;
/* 0x9a0 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd2_0;
/* 0x9a4 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd2_1;
/* 0x9a8 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd2_2;
/* 0x9ac */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd2_3;
/* 0x9b0 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd3_0;
/* 0x9b4 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd3_1;
/* 0x9b8 */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd3_2;
/* 0x9bc */	u32 emc_pmacro_ob_ddll_short_dq_rank1_cmd3_3;
/* 0x9c0 */	u32 rsvd_9c0[16];
/* 0xa00 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte0_0;
/* 0xa04 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte0_1;
/* 0xa08 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte0_2;
/* 0xa0c */	u32 rsvd_a0c;
/* 0xa10 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte1_0;
/* 0xa14 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte1_1;
/* 0xa18 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte1_2;
/* 0xa1c */	u32 rsvd_a1c;
/* 0xa20 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte2_0;
/* 0xa24 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte2_1;
/* 0xa28 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte2_2;
/* 0xa2c */	u32 rsvd_a2c;
/* 0xa30 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte3_0;
/* 0xa34 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte3_1;
/* 0xa38 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte3_2;
/* 0xa3c */	u32 rsvd_a3c;
/* 0xa40 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte4_0;
/* 0xa44 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte4_1;
/* 0xa48 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte4_2;
/* 0xa4c */	u32 rsvd_a4c;
/* 0xa50 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte5_0;
/* 0xa54 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte5_1;
/* 0xa58 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte5_2;
/* 0xa5c */	u32 rsvd_a5c;
/* 0xa60 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte6_0;
/* 0xa64 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte6_1;
/* 0xa68 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte6_2;
/* 0xa6c */	u32 rsvd_a6c;
/* 0xa70 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte7_0;
/* 0xa74 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte7_1;
/* 0xa78 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_byte7_2;
/* 0xa7c */	u32 rsvd_a7c;
/* 0xa80 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd0_0;
/* 0xa84 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd0_1;
/* 0xa88 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd0_2;
/* 0xa8c */	u32 rsvd_a8c;
/* 0xa90 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd1_0;
/* 0xa94 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd1_1;
/* 0xa98 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd1_2;
/* 0xa9c */	u32 rsvd_a9c;
/* 0xaa0 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd2_0;
/* 0xaa4 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd2_1;
/* 0xaa8 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd2_2;
/* 0xaac */	u32 rsvd_aac;
/* 0xab0 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd3_0;
/* 0xab4 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd3_1;
/* 0xab8 */	u32 emc_pmacro_ib_ddll_short_dq_rank0_cmd3_2;
/* 0xabc */	u32 rsvd_abc[17];
/* 0xb00 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte0_0;
/* 0xb04 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte0_1;
/* 0xb08 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte0_2;
/* 0xb0c */	u32 rsvd_b0c;
/* 0xb10 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte1_0;
/* 0xb14 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte1_1;
/* 0xb18 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte1_2;
/* 0xb1c */	u32 rsvd_b1c;
/* 0xb20 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte2_0;
/* 0xb24 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte2_1;
/* 0xb28 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte2_2;
/* 0xb2c */	u32 rsvd_b2c;
/* 0xb30 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte3_0;
/* 0xb34 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte3_1;
/* 0xb38 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte3_2;
/* 0xb3c */	u32 rsvd_b3c;
/* 0xb40 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte4_0;
/* 0xb44 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte4_1;
/* 0xb48 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte4_2;
/* 0xb4c */	u32 rsvd_b4c;
/* 0xb50 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte5_0;
/* 0xb54 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte5_1;
/* 0xb58 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte5_2;
/* 0xb5c */	u32 rsvd_b5c;
/* 0xb60 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte6_0;
/* 0xb64 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte6_1;
/* 0xb68 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte6_2;
/* 0xb6c */	u32 rsvd_b6c;
/* 0xb70 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte7_0;
/* 0xb74 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte7_1;
/* 0xb78 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_byte7_2;
/* 0xb7c */	u32 rsvd_b7c;
/* 0xb80 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd0_0;
/* 0xb84 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd0_1;
/* 0xb88 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd0_2;
/* 0xb8c */	u32 rsvd_b8c;
/* 0xb90 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd1_0;
/* 0xb94 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd1_1;
/* 0xb98 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd1_2;
/* 0xb9c */	u32 rsvd_b9c;
/* 0xba0 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd2_0;
/* 0xba4 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd2_1;
/* 0xba8 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd2_2;
/* 0xbac */	u32 rsvd_bac;
/* 0xbb0 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd3_0;
/* 0xbb4 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd3_1;
/* 0xbb8 */	u32 emc_pmacro_ib_ddll_short_dq_rank1_cmd3_2;
/* 0xbbc */	u32 rsvd_bbc[9];
/* 0xbe0 */	u32 emc_pmacro_ib_vref_dq_0;
/* 0xbe4 */	u32 emc_pmacro_ib_vref_dq_1;
/* 0xbe8 */	u32 emc_pmacro_ib_vref_dq_2;
/* 0xbec */	u32 rsvd_bec;
/* 0xbf0 */	u32 emc_pmacro_ib_vref_dqs_0;
/* 0xbf4 */	u32 emc_pmacro_ib_vref_dqs_1;
/* 0xbf8 */	u32 emc_pmacro_ib_vref_dqs_2;
/* 0xbfc */	u32 rsvd_bfc;
/* 0xc00 */	u32 emc_pmacro_ddll_long_cmd_0;
/* 0xc04 */	u32 emc_pmacro_ddll_long_cmd_1;
/* 0xc08 */	u32 emc_pmacro_ddll_long_cmd_2;
/* 0xc0c */	u32 emc_pmacro_ddll_long_cmd_3;
/* 0xc10 */	u32 emc_pmacro_ddll_long_cmd_4;
/* 0xc14 */	u32 emc_pmacro_ddll_long_cmd_5;
/* 0xc18 */	u32 rsvd_c18[2];
/* 0xc20 */	u32 emc_pmacro_ddll_short_cmd_0;
/* 0xc24 */	u32 emc_pmacro_ddll_short_cmd_1;
/* 0xc28 */	u32 emc_pmacro_ddll_short_cmd_2;
/* 0xc2c */	u32 rsvd_c2c;
/* 0xc30 */	u32 emc_pmacro_cfg_pm_global_0;
/* 0xc34 */	u32 emc_pmacro_vttgen_ctrl_0;
/* 0xc38 */	u32 emc_pmacro_vttgen_ctrl_1;
/* 0xc3c */	u32 emc_pmacro_bg_bias_ctrl_0;
/* 0xc40 */	u32 emc_pmacro_pad_cfg_ctrl;
/* 0xc44 */	u32 emc_pmacro_zctrl;
/* 0xc48 */	u32 emc_pmacro_rx_term;
/* 0xc4c */	u32 emc_pmacro_cmd_tx_drv;
/* 0xc50 */	u32 emc_pmacro_cmd_pad_rx_ctrl;
/* 0xc54 */	u32 emc_pmacro_data_pad_rx_ctrl;
/* 0xc58 */	u32 emc_pmacro_cmd_rx_term_mode;
/* 0xc5c */	u32 emc_pmacro_data_rx_term_mode;
/* 0xc60 */	u32 emc_pmacro_cmd_pad_tx_ctrl;
/* 0xc64 */	u32 emc_pmacro_data_pad_tx_ctrl;
/* 0xc68 */	u32 emc_pmacro_common_pad_tx_ctrl_t210;
/* 0xc6c */	u32 emc_pmacro_dsr_vttgen_ctrl_0_b01;
/* 0xc70 */	u32 emc_pmacro_dq_tx_drv;
/* 0xc74 */	u32 emc_pmacro_ca_tx_drv;
/* 0xc78 */	u32 emc_pmacro_autocal_cfg_common;
/* 0xc7c */	u32 rsvd_c7c;
/* 0xc80 */	u32 emc_pmacro_brick_mapping_0;
/* 0xc84 */	u32 emc_pmacro_brick_mapping_1;
/* 0xc88 */	u32 emc_pmacro_brick_mapping_2;
/* 0xc8c */	u32 emc_stat_dram_io_extclks_cke_eq0_no_banks_active_clks_lo;
/* 0xc90 */	u32 emc_stat_dram_io_extclks_cke_eq0_no_banks_active_clks_hi;
/* 0xc94 */	u32 emc_stat_dram_io_clkstop_cke_eq0_no_banks_active_clks_lo;
/* 0xc98 */	u32 emc_stat_dram_io_clkstop_cke_eq0_no_banks_active_clks_hi;
/* 0xc9c */	u32 emc_stat_dram_io_extclks_cke_eq1_no_banks_active_clks_lo;
/* 0xca0 */	u32 emc_stat_dram_io_extclks_cke_eq1_no_banks_active_clks_hi;
/* 0xca4 */	u32 emc_stat_dram_io_clkstop_cke_eq1_no_banks_active_clks_lo;
/* 0xca8 */	u32 emc_stat_dram_io_clkstop_cke_eq1_no_banks_active_clks_hi;
/* 0xcac */	u32 emc_stat_dram_io_extclks_cke_eq0_some_banks_active_clks_lo;
/* 0xcb0 */	u32 emc_stat_dram_io_extclks_cke_eq0_some_banks_active_clks_hi;
/* 0xcb4 */	u32 emc_stat_dram_io_clkstop_cke_eq0_some_banks_active_clks_lo;
/* 0xcb8 */	u32 emc_stat_dram_io_clkstop_cke_eq0_some_banks_active_clks_hi;
/* 0xcbc */	u32 emc_stat_dram_io_extclks_cke_eq1_some_banks_active_clks_lo;
/* 0xcc0 */	u32 emc_stat_dram_io_extclks_cke_eq1_some_banks_active_clks_hi;
/* 0xcc4 */	u32 emc_stat_dram_io_clkstop_cke_eq1_some_banks_active_clks_lo;
/* 0xcc8 */	u32 emc_stat_dram_io_clkstop_cke_eq1_some_banks_active_clks_hi;
/* 0xccc */	u32 emc_stat_dram_io_sr_cke_eq0_clks_lo;
/* 0xcd0 */	u32 emc_stat_dram_io_sr_cke_eq0_clks_hi;
/* 0xcd4 */	u32 emc_stat_dram_io_dsr;
/* 0xcd8 */	u32 rsvd_cd8[2];
/* 0xce0 */	u32 emc_pmacro_ddllcal_cal_t210;
/* 0xce4 */	u32 emc_pmacro_ddll_offset;
/* 0xce8 */	u32 emc_pmacro_ddll_periodic_offset;
/* 0xcec */	u32 rsvd_cec;
/* 0xcf0 */	u32 emc_pmacro_vttgen_ctrl_2;
/* 0xcf4 */	u32 emc_pmacro_ib_rxrt;
/* 0xcf8 */	u32 emc_pmacro_training_ctrl_0;
/* 0xcfc */	u32 emc_pmacro_training_ctrl_1;
/* 0xd00 */	u32 emc_pmacro_ddllcal_cal_0_b01;
/* 0xd04 */	u32 emc_pmacro_ddllcal_cal_1_b01;
/* 0xd08 */	u32 emc_pmacro_ddllcal_cal_2_b01;
/* 0xd0c */	u32 emc_pmacro_ddllcal_cal_3_b01;
/* 0xd10 */	u32 emc_pmacro_ddllcal_cal_4_b01;
/* 0xd14 */	u32 emc_pmacro_ddllcal_cal_5_b01;
/* 0xd18 */	u32 rsvd_d18[2];
/* 0xd20 */	u32 emc_pmacro_dig_dll_status_0_b01;
/* 0xd24 */	u32 emc_pmacro_dig_dll_status_1_b01;
/* 0xd28 */	u32 emc_pmacro_dig_dll_status_2_b01;
/* 0xd2c */	u32 emc_pmacro_dig_dll_status_3_b01;
/* 0xd30 */	u32 emc_pmacro_dig_dll_status_4_b01;
/* 0xd34 */	u32 emc_pmacro_dig_dll_status_5_b01;
/* 0xd38 */	u32 rsvd_d38[2];
/* 0xd40 */	u32 emc_pmacro_perbit_fgcg_ctrl_0_b01;
/* 0xd44 */	u32 emc_pmacro_perbit_fgcg_ctrl_1_b01;
/* 0xd48 */	u32 emc_pmacro_perbit_fgcg_ctrl_2_b01;
/* 0xd4c */	u32 emc_pmacro_perbit_fgcg_ctrl_3_b01;
/* 0xd50 */	u32 emc_pmacro_perbit_fgcg_ctrl_4_b01;
/* 0xd54 */	u32 emc_pmacro_perbit_fgcg_ctrl_5_b01;
/* 0xd58 */	u32 rsvd_d58[2];
/* 0xd60 */	u32 emc_pmacro_perbit_rfu_ctrl_0_b01;
/* 0xd64 */	u32 emc_pmacro_perbit_rfu_ctrl_1_b01;
/* 0xd68 */	u32 emc_pmacro_perbit_rfu_ctrl_2_b01;
/* 0xd6c */	u32 emc_pmacro_perbit_rfu_ctrl_3_b01;
/* 0xd70 */	u32 emc_pmacro_perbit_rfu_ctrl_4_b01;
/* 0xd74 */	u32 emc_pmacro_perbit_rfu_ctrl_5_b01;
/* 0xd78 */	u32 rsvd_d78[2];
/* 0xd80 */	u32 emc_pmacro_perbit_rfu1_ctrl_0_b01;
/* 0xd84 */	u32 emc_pmacro_perbit_rfu1_ctrl_1_b01;
/* 0xd88 */	u32 emc_pmacro_perbit_rfu1_ctrl_2_b01;
/* 0xd8c */	u32 emc_pmacro_perbit_rfu1_ctrl_3_b01;
/* 0xd90 */	u32 emc_pmacro_perbit_rfu1_ctrl_4_b01;
/* 0xd94 */	u32 emc_pmacro_perbit_rfu1_ctrl_5_b01;
/* 0xd98 */	u32 rsvd_d98[2];
/* 0xda0 */	u32 emc_pmacro_pmu_out_eoff1_0_b01;
/* 0xda4 */	u32 emc_pmacro_pmu_out_eoff1_1_b01;
/* 0xda8 */	u32 emc_pmacro_pmu_out_eoff1_2_b01;
/* 0xdac */	u32 emc_pmacro_pmu_out_eoff1_3_b01;
/* 0xdb0 */	u32 emc_pmacro_pmu_out_eoff1_4_b01;
/* 0xdb4 */	u32 emc_pmacro_pmu_out_eoff1_5_b01;
/* 0xdb8 */	u32 rsvd_db8[2];
/* 0xdc0 */	u32 emc_pmacro_comp_pmu_out_b01;
/* 0xdc4 */	u32 rsvd_dc4[15];
/* 0xe00 */	u32 emc_training_cmd;
/* 0xe04 */	u32 emc_training_ctrl;
/* 0xe08 */	u32 emc_training_status;
/* 0xe0c */	u32 emc_training_quse_cors_ctrl;
/* 0xe10 */	u32 emc_training_quse_fine_ctrl;
/* 0xe14 */	u32 emc_training_quse_ctrl_misc;
/* 0xe18 */	u32 emc_training_write_fine_ctrl;
/* 0xe1c */	u32 emc_training_write_ctrl_misc;
/* 0xe20 */	u32 emc_training_write_vref_ctrl;
/* 0xe24 */	u32 emc_training_read_fine_ctrl;
/* 0xe28 */	u32 emc_training_read_ctrl_misc;
/* 0xe2c */	u32 emc_training_read_vref_ctrl;
/* 0xe30 */	u32 emc_training_ca_fine_ctrl;
/* 0xe34 */	u32 emc_training_ca_ctrl_misc;
/* 0xe38 */	u32 emc_training_ca_ctrl_misc1;
/* 0xe3c */	u32 emc_training_ca_vref_ctrl;
/* 0xe40 */	u32 emc_training_ca_tadr_ctrl;
/* 0xe44 */	u32 emc_training_settle;
/* 0xe48 */	u32 emc_training_debug_ctrl;
/* 0xe4c */	u32 emc_training_debug_dq0;
/* 0xe50 */	u32 emc_training_debug_dq1;
/* 0xe54 */	u32 emc_training_debug_dq2;
/* 0xe58 */	u32 emc_training_debug_dq3;
/* 0xe5c */	u32 emc_training_mpc;
/* 0xe60 */	u32 emc_training_patram_ctrl;
/* 0xe64 */	u32 emc_training_patram_dq;
/* 0xe68 */	u32 emc_training_patram_dmi;
/* 0xe6c */	u32 emc_training_vref_settle;
/* 0xe70 */	u32 emc_training_rw_eye_center_ib_byte0;
/* 0xe74 */	u32 emc_training_rw_eye_center_ib_byte1;
/* 0xe78 */	u32 emc_training_rw_eye_center_ib_byte2;
/* 0xe7c */	u32 emc_training_rw_eye_center_ib_byte3;
/* 0xe80 */	u32 emc_training_rw_eye_center_ib_misc;
/* 0xe84 */	u32 emc_training_rw_eye_center_ob_byte0;
/* 0xe88 */	u32 emc_training_rw_eye_center_ob_byte1;
/* 0xe8c */	u32 emc_training_rw_eye_center_ob_byte2;
/* 0xe90 */	u32 emc_training_rw_eye_center_ob_byte3;
/* 0xe94 */	u32 emc_training_rw_eye_center_ob_misc;
/* 0xe98 */	u32 emc_training_rw_offset_ib_byte0;
/* 0xe9c */	u32 emc_training_rw_offset_ib_byte1;
/* 0xea0 */	u32 emc_training_rw_offset_ib_byte2;
/* 0xea4 */	u32 emc_training_rw_offset_ib_byte3;
/* 0xea8 */	u32 emc_training_rw_offset_ib_misc;
/* 0xeac */	u32 emc_training_rw_offset_ob_byte0;
/* 0xeb0 */	u32 emc_training_rw_offset_ob_byte1;
/* 0xeb4 */	u32 emc_training_rw_offset_ob_byte2;
/* 0xeb8 */	u32 emc_training_rw_offset_ob_byte3;
/* 0xebc */	u32 emc_training_rw_offset_ob_misc;
/* 0xec0 */	u32 emc_training_opt_ca_vref;
/* 0xec4 */	u32 emc_training_opt_dq_ob_vref;
/* 0xec8 */	u32 emc_training_opt_dq_ib_vref_rank0;
/* 0xecc */	u32 emc_training_opt_dq_ib_vref_rank1;
/* 0xed0 */	u32 emc_training_quse_vref_ctrl;
/* 0xed4 */	u32 emc_training_opt_dqs_ib_vref_rank0;
/* 0xed8 */	u32 emc_training_opt_dqs_ib_vref_rank1;
/* 0xedc */	u32 emc_training_dramc_timing;
} emc_regs_t210_t;

#endif
