/*
 * Copyright (c) 2018-2025 CTCaer
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

#ifndef _MC_T210_H_
#define _MC_T210_H_

/* Memory Controller registers */
#define MC_INTSTATUS                                        0x0
#define MC_INTMASK                                          0x4
#define MC_ERR_STATUS                                       0x8
#define MC_ERR_ADR                                          0xC
#define MC_SMMU_CONFIG                                      0x10
#define MC_SMMU_TLB_CONFIG                                  0x14
#define MC_SMMU_PTC_CONFIG                                  0x18
#define MC_SMMU_PTB_ASID                                    0x1C
#define MC_SMMU_PTB_DATA                                    0x20
#define MC_SMMU_TLB_FLUSH                                   0x30
#define MC_SMMU_PTC_FLUSH                                   0x34
#define MC_SMMU_ASID_SECURITY                               0x38
#define MC_SMMU_ASID_SECURITY_1                             0x3C
#define MC_SMMU_CLIENT_CONFIG0                              0x40
#define MC_SMMU_CLIENT_CONFIG1                              0x44
#define MC_SMMU_CLIENT_CONFIG2                              0x48
#define MC_SMMU_CLIENT_CONFIG3                              0x4C
#define MC_EMEM_CFG                                         0x50
#define MC_EMEM_ADR_CFG                                     0x54
#define MC_EMEM_ADR_CFG_DEV0                                0x58
#define MC_EMEM_ADR_CFG_DEV1                                0x5C
#define MC_EMEM_ADR_CFG_CHANNEL_MASK                        0x60
#define MC_EMEM_ADR_CFG_BANK_MASK_0                         0x64
#define MC_EMEM_ADR_CFG_BANK_MASK_1                         0x68
#define MC_EMEM_ADR_CFG_BANK_MASK_2                         0x6C
#define MC_SECURITY_CFG0                                    0x70
#define MC_SECURITY_CFG1                                    0x74
#define MC_SECURITY_RSV                                     0x7C
#define MC_EMEM_ARB_CFG                                     0x90
#define MC_EMEM_ARB_OUTSTANDING_REQ                         0x94
#define MC_EMEM_ARB_TIMING_RCD                              0x98
#define MC_EMEM_ARB_TIMING_RP                               0x9C
#define MC_EMEM_ARB_TIMING_RC                               0xA0
#define MC_EMEM_ARB_TIMING_RAS                              0xA4
#define MC_EMEM_ARB_TIMING_FAW                              0xA8
#define MC_EMEM_ARB_TIMING_RRD                              0xAC
#define MC_EMEM_ARB_TIMING_RAP2PRE                          0xB0
#define MC_EMEM_ARB_TIMING_WAP2PRE                          0xB4
#define MC_EMEM_ARB_TIMING_R2R                              0xB8
#define MC_EMEM_ARB_TIMING_W2W                              0xBC
#define MC_EMEM_ARB_TIMING_R2W                              0xC0
#define MC_EMEM_ARB_TIMING_W2R                              0xC4
#define MC_EMEM_ARB_MISC2                                   0xC8
#define MC_EMEM_ARB_DA_TURNS                                0xD0
#define MC_EMEM_ARB_DA_COVERS                               0xD4
#define MC_EMEM_ARB_MISC0                                   0xD8
#define MC_EMEM_ARB_MISC1                                   0xDC
#define MC_EMEM_ARB_RING1_THROTTLE                          0xE0
#define MC_EMEM_ARB_RING3_THROTTLE                          0xE4
#define MC_EMEM_ARB_OVERRIDE                                0xE8
#define MC_EMEM_ARB_RSV                                     0xEC
#define MC_CLKEN_OVERRIDE                                   0xF4
#define MC_TIMING_CONTROL_DBG                               0xF8
#define MC_TIMING_CONTROL                                   0xFC
#define MC_STAT_CONTROL                                     0x100
#define MC_STAT_STATUS                                      0x104
#define MC_STAT_EMC_CLOCK_LIMIT                             0x108
#define MC_STAT_EMC_CLOCK_LIMIT_MSBS                        0x10C
#define MC_STAT_EMC_CLOCKS                                  0x110
#define MC_STAT_EMC_CLOCKS_MSBS                             0x114
#define MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_LO                0x118
#define MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_HI                0x11C
#define MC_STAT_EMC_FILTER_SET0_SLACK_LIMIT                 0x120
#define MC_STAT_EMC_FILTER_SET0_CLIENT_0                    0x128
#define MC_STAT_EMC_FILTER_SET0_CLIENT_1                    0x12C
#define MC_STAT_EMC_FILTER_SET0_CLIENT_2                    0x130
#define MC_STAT_EMC_FILTER_SET0_CLIENT_3                    0x134
#define MC_STAT_EMC_SET0_COUNT                              0x138
#define MC_STAT_EMC_SET0_COUNT_MSBS                         0x13C
#define MC_STAT_EMC_SET0_SLACK_ACCUM                        0x140
#define MC_STAT_EMC_SET0_SLACK_ACCUM_MSBS                   0x144
#define MC_STAT_EMC_SET0_HISTO_COUNT                        0x148
#define MC_STAT_EMC_SET0_HISTO_COUNT_MSBS                   0x14C
#define MC_STAT_EMC_SET0_MINIMUM_SLACK_OBSERVED             0x150
#define MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_LO                0x158
#define MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_HI                0x15C
#define MC_STAT_EMC_FILTER_SET1_SLACK_LIMIT                 0x160
#define MC_STAT_EMC_FILTER_SET1_CLIENT_0                    0x168
#define MC_STAT_EMC_FILTER_SET1_CLIENT_1                    0x16C
#define MC_STAT_EMC_FILTER_SET1_CLIENT_2                    0x170
#define MC_STAT_EMC_FILTER_SET1_CLIENT_3                    0x174
#define MC_STAT_EMC_SET1_COUNT                              0x178
#define MC_STAT_EMC_SET1_COUNT_MSBS                         0x17C
#define MC_STAT_EMC_SET1_SLACK_ACCUM                        0x180
#define MC_STAT_EMC_SET1_SLACK_ACCUM_MSBS                   0x184
#define MC_STAT_EMC_SET1_HISTO_COUNT                        0x188
#define MC_STAT_EMC_SET1_HISTO_COUNT_MSBS                   0x18C
#define MC_STAT_EMC_SET1_MINIMUM_SLACK_OBSERVED             0x190
#define MC_STAT_EMC_FILTER_SET0_VIRTUAL_ADR_LIMIT_LO        0x198
#define MC_STAT_EMC_FILTER_SET0_VIRTUAL_ADR_LIMIT_HI        0x19C
#define MC_STAT_EMC_FILTER_SET0_ASID                        0x1A0
#define MC_STAT_EMC_FILTER_SET1_VIRTUAL_ADR_LIMIT_LO        0x1A8
#define MC_STAT_EMC_FILTER_SET1_VIRTUAL_ADR_LIMIT_HI        0x1AC
#define MC_STAT_EMC_FILTER_SET1_ASID                        0x1B0
#define MC_STAT_EMC_SET0_IDLE_CYCLE_COUNT                   0x1B8
#define MC_STAT_EMC_SET0_IDLE_CYCLE_COUNT_MSBS              0x1BC
#define MC_STAT_EMC_SET0_IDLE_CYCLE_PARTITION_SELECT        0x1C0
#define MC_STAT_EMC_SET1_IDLE_CYCLE_COUNT                   0x1C8
#define MC_STAT_EMC_SET1_IDLE_CYCLE_COUNT_MSBS              0x1CC
#define MC_STAT_EMC_SET1_IDLE_CYCLE_PARTITION_SELECT        0x1D0
#define MC_SMMU_STATS_TLB_HIT_MISS_SOURCE                   0x1EC
#define MC_SMMU_STATS_TLB_HIT_COUNT                         0x1F0
#define MC_SMMU_STATS_TLB_MISS_COUNT                        0x1F4
#define MC_SMMU_STATS_PTC_HIT_COUNT                         0x1F8
#define MC_SMMU_STATS_PTC_MISS_COUNT                        0x1FC
#define MC_CLIENT_HOTRESET_CTRL                             0x200
#define MC_CLIENT_HOTRESET_STATUS                           0x204
#define MC_EMEM_ARB_ISOCHRONOUS_0                           0x208
#define MC_EMEM_ARB_ISOCHRONOUS_1                           0x20C
#define MC_EMEM_ARB_ISOCHRONOUS_2                           0x210
#define MC_EMEM_ARB_ISOCHRONOUS_3                           0x214
#define MC_EMEM_ARB_HYSTERESIS_0                            0x218
#define MC_EMEM_ARB_HYSTERESIS_1                            0x21C
#define MC_EMEM_ARB_HYSTERESIS_2                            0x220
#define MC_EMEM_ARB_HYSTERESIS_3                            0x224
#define MC_SMMU_TRANSLATION_ENABLE_0                        0x228
#define MC_SMMU_TRANSLATION_ENABLE_1                        0x22C
#define MC_SMMU_TRANSLATION_ENABLE_2                        0x230
#define MC_SMMU_TRANSLATION_ENABLE_3                        0x234
#define MC_SMMU_AFI_ASID                                    0x238
#define MC_SMMU_AVPC_ASID                                   0x23C
#define MC_SMMU_DC_ASID                                     0x240
#define MC_SMMU_DCB_ASID                                    0x244
#define MC_SMMU_HC_ASID                                     0x250
#define MC_SMMU_HDA_ASID                                    0x254
#define MC_SMMU_ISP2_ASID                                   0x258
#define MC_SMMU_NVENC_ASID                                  0x264
#define MC_SMMU_PPCS_ASID                                   0x270
#define MC_SMMU_SATA_ASID                                   0x274
#define MC_SMMU_VI_ASID                                     0x280
#define MC_SMMU_VIC_ASID                                    0x284
#define MC_SMMU_XUSB_HOST_ASID                              0x288
#define MC_SMMU_XUSB_DEV_ASID                               0x28C
#define MC_SMMU_A9AVP_ASID                                  0x290
#define MC_SMMU_TSEC_ASID                                   0x294
#define MC_SMMU_PPCS1_ASID                                  0x298
#define MC_AHB_EXTRA_SNAP_LEVELS                            0x2A0
#define MC_APB_EXTRA_SNAP_LEVELS                            0x2A4
#define MC_AVP_EXTRA_SNAP_LEVELS                            0x2A8
#define MC_DIS_EXTRA_SNAP_LEVELS                            0x2AC
#define MC_PCX_EXTRA_SNAP_LEVELS                            0x2B8
#define MC_FTOP_EXTRA_SNAP_LEVELS                           0x2BC
#define MC_SAX_EXTRA_SNAP_LEVELS                            0x2C0
#define MC_VE_EXTRA_SNAP_LEVELS                             0x2D8
#define MC_LATENCY_ALLOWANCE_AFI_0                          0x2E0
#define MC_LATENCY_ALLOWANCE_AVPC_0                         0x2E4
#define MC_LATENCY_ALLOWANCE_DC_0                           0x2E8
#define MC_LATENCY_ALLOWANCE_DC_1                           0x2EC
#define MC_LATENCY_ALLOWANCE_DC_2                           0x2F0
#define MC_LATENCY_ALLOWANCE_DCB_0                          0x2F4
#define MC_LATENCY_ALLOWANCE_DCB_1                          0x2F8
#define MC_LATENCY_ALLOWANCE_DCB_2                          0x2FC
#define MC_LATENCY_ALLOWANCE_HC_0                           0x310
#define MC_LATENCY_ALLOWANCE_HC_1                           0x314
#define MC_LATENCY_ALLOWANCE_HDA_0                          0x318
#define MC_LATENCY_ALLOWANCE_MPCORE_0                       0x320
#define MC_LATENCY_ALLOWANCE_NVENC_0                        0x328
#define MC_LATENCY_ALLOWANCE_PPCS_0                         0x344
#define MC_LATENCY_ALLOWANCE_PPCS_1                         0x348
#define MC_LATENCY_ALLOWANCE_PTC_0                          0x34C
#define MC_LATENCY_ALLOWANCE_SATA_0                         0x350
#define MC_LATENCY_ALLOWANCE_ISP2_0                         0x370
#define MC_LATENCY_ALLOWANCE_ISP2_1                         0x374
#define MC_LATENCY_ALLOWANCE_XUSB_0                         0x37C
#define MC_LATENCY_ALLOWANCE_XUSB_1                         0x380
#define MC_LATENCY_ALLOWANCE_ISP2B_0                        0x384
#define MC_LATENCY_ALLOWANCE_ISP2B_1                        0x388
#define MC_LATENCY_ALLOWANCE_TSEC_0                         0x390
#define MC_LATENCY_ALLOWANCE_VIC_0                          0x394
#define MC_LATENCY_ALLOWANCE_VI2_0                          0x398
#define MC_LATENCY_ALLOWANCE_AXIAP_0                        0x3A0
#define MC_LATENCY_ALLOWANCE_A9AVP_0                        0x3A4
#define MC_LATENCY_ALLOWANCE_GPU_0                          0x3AC
#define MC_LATENCY_ALLOWANCE_SDMMCA_0                       0x3B8
#define MC_LATENCY_ALLOWANCE_SDMMCAA_0                      0x3BC
#define MC_LATENCY_ALLOWANCE_SDMMC_0                        0x3C0
#define MC_LATENCY_ALLOWANCE_SDMMCAB_0                      0x3C4
#define MC_LATENCY_ALLOWANCE_DC_3                           0x3C8
#define MC_LATENCY_ALLOWANCE_NVDEC_0                        0x3D8
#define MC_LATENCY_ALLOWANCE_APE_0                          0x3DC
#define MC_LATENCY_ALLOWANCE_SE_0                           0x3E0
#define MC_LATENCY_ALLOWANCE_NVJPG_0                        0x3E4
#define MC_LATENCY_ALLOWANCE_GPU2_0                         0x3E8
#define MC_LATENCY_ALLOWANCE_ETR_0                          0x3EC
#define MC_LATENCY_ALLOWANCE_TSECB_0                        0x3F0
#define MC_RESERVED_RSV                                     0x3FC
#define MC_USBX_EXTRA_SNAP_LEVELS                           0x404
#define MC_DISB_EXTRA_SNAP_LEVELS                           0x408
#define MC_MSE_EXTRA_SNAP_LEVELS                            0x40C
#define MC_VE2_EXTRA_SNAP_LEVELS                            0x410
#define MC_A9AVPPC_EXTRA_SNAP_LEVELS                        0x414
#define MC_VIDEO_PROTECT_VPR_OVERRIDE                       0x418
#define MC_DIS_PTSA_RATE                                    0x41C
#define MC_DIS_PTSA_MIN                                     0x420
#define MC_DIS_PTSA_MAX                                     0x424
#define MC_DISB_PTSA_RATE                                   0x428
#define MC_DISB_PTSA_MIN                                    0x42C
#define MC_DISB_PTSA_MAX                                    0x430
#define MC_VE_PTSA_RATE                                     0x434
#define MC_VE_PTSA_MIN                                      0x438
#define MC_VE_PTSA_MAX                                      0x43C
#define MC_RING2_PTSA_RATE                                  0x440
#define MC_RING2_PTSA_MIN                                   0x444
#define MC_RING2_PTSA_MAX                                   0x448
#define MC_MLL_MPCORER_PTSA_RATE                            0x44C
#define MC_MLL_MPCORER_PTSA_MIN                             0x450
#define MC_MLL_MPCORER_PTSA_MAX                             0x454
#define MC_SMMU_SMMU_PTSA_RATE                              0x458
#define MC_SMMU_SMMU_PTSA_MIN                               0x45C
#define MC_SMMU_SMMU_PTSA_MAX                               0x460
#define MC_RING1_PTSA_RATE                                  0x47C
#define MC_RING1_PTSA_MIN                                   0x480
#define MC_RING1_PTSA_MAX                                   0x484
#define MC_A9AVPPC_PTSA_RATE                                0x488
#define MC_A9AVPPC_PTSA_MIN                                 0x48C
#define MC_A9AVPPC_PTSA_MAX                                 0x490
#define MC_VE2_PTSA_RATE                                    0x494
#define MC_VE2_PTSA_MIN                                     0x498
#define MC_VE2_PTSA_MAX                                     0x49C
#define MC_ISP_PTSA_RATE                                    0x4A0
#define MC_ISP_PTSA_MIN                                     0x4A4
#define MC_ISP_PTSA_MAX                                     0x4A8
#define MC_PCX_PTSA_RATE                                    0x4AC
#define MC_PCX_PTSA_MIN                                     0x4B0
#define MC_PCX_PTSA_MAX                                     0x4B4
#define MC_SAX_PTSA_RATE                                    0x4B8
#define MC_SAX_PTSA_MIN                                     0x4BC
#define MC_SAX_PTSA_MAX                                     0x4C0
#define MC_MSE_PTSA_RATE                                    0x4C4
#define MC_MSE_PTSA_MIN                                     0x4C8
#define MC_MSE_PTSA_MAX                                     0x4CC
#define MC_SD_PTSA_RATE                                     0x4D0
#define MC_SD_PTSA_MIN                                      0x4D4
#define MC_SD_PTSA_MAX                                      0x4D8
#define MC_AHB_PTSA_RATE                                    0x4DC
#define MC_AHB_PTSA_MIN                                     0x4E0
#define MC_AHB_PTSA_MAX                                     0x4E4
#define MC_APB_PTSA_RATE                                    0x4E8
#define MC_APB_PTSA_MIN                                     0x4EC
#define MC_APB_PTSA_MAX                                     0x4F0
#define MC_AVP_PTSA_RATE                                    0x4F4
#define MC_AVP_PTSA_MIN                                     0x4F8
#define MC_AVP_PTSA_MAX                                     0x4FC
#define MC_FTOP_PTSA_RATE                                   0x50C
#define MC_FTOP_PTSA_MIN                                    0x510
#define MC_FTOP_PTSA_MAX                                    0x514
#define MC_HOST_PTSA_RATE                                   0x518
#define MC_HOST_PTSA_MIN                                    0x51C
#define MC_HOST_PTSA_MAX                                    0x520
#define MC_USBX_PTSA_RATE                                   0x524
#define MC_USBX_PTSA_MIN                                    0x528
#define MC_USBX_PTSA_MAX                                    0x52C
#define MC_USBD_PTSA_RATE                                   0x530
#define MC_USBD_PTSA_MIN                                    0x534
#define MC_USBD_PTSA_MAX                                    0x538
#define MC_GK_PTSA_RATE                                     0x53C
#define MC_GK_PTSA_MIN                                      0x540
#define MC_GK_PTSA_MAX                                      0x544
#define MC_AUD_PTSA_RATE                                    0x548
#define MC_AUD_PTSA_MIN                                     0x54C
#define MC_AUD_PTSA_MAX                                     0x550
#define MC_VICPC_PTSA_RATE                                  0x554
#define MC_VICPC_PTSA_MIN                                   0x558
#define MC_VICPC_PTSA_MAX                                   0x55C
#define MC_JPG_PTSA_RATE                                    0x584
#define MC_JPG_PTSA_MIN                                     0x588
#define MC_JPG_PTSA_MAX                                     0x58C
#define MC_VIDEO_PROTECT_VPR_OVERRIDE1                      0x590
#define MC_SMMU_TLB_SET_SELECTION_MASK_0                    0x600
#define MC_GK2_PTSA_RATE                                    0x610
#define MC_GK2_PTSA_MIN                                     0x614
#define MC_GK2_PTSA_MAX                                     0x618
#define MC_SDM_PTSA_RATE                                    0x61C
#define MC_SDM_PTSA_MIN                                     0x620
#define MC_SDM_PTSA_MAX                                     0x624
#define MC_HDAPC_PTSA_RATE                                  0x628
#define MC_HDAPC_PTSA_MIN                                   0x62C
#define MC_HDAPC_PTSA_MAX                                   0x630
#define MC_DFD_PTSA_RATE                                    0x634
#define MC_DFD_PTSA_MIN                                     0x638
#define MC_DFD_PTSA_MAX                                     0x63C
#define MC_VIDEO_PROTECT_BOM                                0x648
#define MC_VIDEO_PROTECT_SIZE_MB                            0x64C
#define MC_VIDEO_PROTECT_REG_CTRL                           0x650
#define MC_ERR_VPR_STATUS                                   0x654
#define MC_ERR_VPR_ADR                                      0x658
#define MC_IRAM_BOM                                         0x65C
#define MC_IRAM_TOM                                         0x660
#define MC_EMEM_CFG_ACCESS_CTRL                             0x664
#define MC_TZ_SECURITY_CTRL                                 0x668
#define  TZ_SEC_CTRL_CPU_STRICT_TZ_APERTURE_CHECK                BIT(0)
#define MC_EMEM_ARB_OUTSTANDING_REQ_RING3                   0x66C
#define MC_SEC_CARVEOUT_BOM                                 0x670
#define MC_SEC_CARVEOUT_SIZE_MB                             0x674
#define MC_SEC_CARVEOUT_REG_CTRL                            0x678
#define MC_ERR_SEC_STATUS                                   0x67C
#define MC_ERR_SEC_ADR                                      0x680
#define MC_PC_IDLE_CLOCK_GATE_CONFIG                        0x684
#define MC_STUTTER_CONTROL                                  0x688
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0A               0x690
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0AB              0x694
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0B               0x698
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0BB              0x69C
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0C               0x6A0
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0CB              0x6A4
#define MC_EMEM_ARB_NISO_THROTTLE                           0x6B0
#define MC_EMEM_ARB_OUTSTANDING_REQ_NISO                    0x6B4
#define MC_EMEM_ARB_NISO_THROTTLE_MASK                      0x6B8
#define MC_EMEM_ARB_RING0_THROTTLE_MASK                     0x6BC
#define MC_EMEM_ARB_TIMING_RFCPB                            0x6C0
#define MC_EMEM_ARB_TIMING_CCDMW                            0x6C4
#define MC_EMEM_ARB_REFPB_HP_CTRL                           0x6F0
#define MC_EMEM_ARB_REFPB_BANK_CTRL                         0x6F4
#define MC_MIN_LENGTH_AFI_0                                 0x88C
#define MC_MIN_LENGTH_AVPC_0                                0x890
#define MC_MIN_LENGTH_DC_0                                  0x894
#define MC_MIN_LENGTH_DC_1                                  0x898
#define MC_MIN_LENGTH_DC_2                                  0x89C
#define MC_MIN_LENGTH_DCB_0                                 0x8A0
#define MC_MIN_LENGTH_DCB_1                                 0x8A4
#define MC_MIN_LENGTH_DCB_2                                 0x8A8
#define MC_MIN_LENGTH_HC_0                                  0x8BC
#define MC_MIN_LENGTH_HC_1                                  0x8C0
#define MC_MIN_LENGTH_HDA_0                                 0x8C4
#define MC_MIN_LENGTH_MPCORE_0                              0x8CC
#define MC_MIN_LENGTH_NVENC_0                               0x8D4
#define MC_MIN_LENGTH_PPCS_0                                0x8F0
#define MC_MIN_LENGTH_PPCS_1                                0x8F4
#define MC_MIN_LENGTH_PTC_0                                 0x8F8
#define MC_MIN_LENGTH_SATA_0                                0x8FC
#define MC_MIN_LENGTH_ISP2_0                                0x91C
#define MC_MIN_LENGTH_ISP2_1                                0x920
#define MC_MIN_LENGTH_XUSB_0                                0x928
#define MC_MIN_LENGTH_XUSB_1                                0x92C
#define MC_MIN_LENGTH_ISP2B_0                               0x930
#define MC_MIN_LENGTH_ISP2B_1                               0x934
#define MC_MIN_LENGTH_TSEC_0                                0x93C
#define MC_MIN_LENGTH_VIC_0                                 0x940
#define MC_MIN_LENGTH_VI2_0                                 0x944
#define MC_MIN_LENGTH_AXIAP_0                               0x94C
#define MC_MIN_LENGTH_A9AVP_0                               0x950
#define MC_RESERVED_RSV_1                                   0x958
#define MC_DVFS_PIPE_SELECT                                 0x95C
#define MC_PTSA_GRANT_DECREMENT                             0x960
#define MC_IRAM_REG_CTRL                                    0x964
#define MC_EMEM_ARB_OVERRIDE_1                              0x968
#define MC_CLIENT_HOTRESET_CTRL_1                           0x970
#define MC_CLIENT_HOTRESET_STATUS_1                         0x974
#define MC_VIDEO_PROTECT_BOM_ADR_HI                         0x978
#define MC_IRAM_ADR_HI                                      0x980
#define MC_VIDEO_PROTECT_GPU_OVERRIDE_0                     0x984
#define MC_VIDEO_PROTECT_GPU_OVERRIDE_1                     0x988
#define MC_EMEM_ARB_STATS_0                                 0x990
#define MC_EMEM_ARB_STATS_1                                 0x994
#define MC_MTS_CARVEOUT_BOM                                 0x9A0
#define MC_MTS_CARVEOUT_SIZE_MB                             0x9A4
#define MC_MTS_CARVEOUT_ADR_HI                              0x9A8
#define MC_MTS_CARVEOUT_REG_CTRL                            0x9AC
#define MC_ERR_MTS_STATUS                                   0x9B0
#define MC_ERR_MTS_ADR                                      0x9B4
#define MC_SMMU_PTC_FLUSH_1                                 0x9B8
#define MC_SECURITY_CFG3                                    0x9BC
#define MC_ERR_APB_ASID_UPDATE_STATUS                       0x9D0
#define MC_SEC_CARVEOUT_ADR_HI                              0x9D4
#define MC_DA_CONFIG0                                       0x9DC
#define MC_SMMU_ASID_SECURITY_2                             0x9E0
#define MC_SMMU_ASID_SECURITY_3                             0x9E4
#define MC_SMMU_ASID_SECURITY_4                             0x9E8
#define MC_SMMU_ASID_SECURITY_5                             0x9EC
#define MC_SMMU_ASID_SECURITY_6                             0x9F0
#define MC_SMMU_ASID_SECURITY_7                             0x9F4
#define MC_GK_EXTRA_SNAP_LEVELS                             0xA00
#define MC_SD_EXTRA_SNAP_LEVELS                             0xA04
#define MC_ISP_EXTRA_SNAP_LEVELS                            0xA08
#define MC_AUD_EXTRA_SNAP_LEVELS                            0xA10
#define MC_HOST_EXTRA_SNAP_LEVELS                           0xA14
#define MC_USBD_EXTRA_SNAP_LEVELS                           0xA18
#define MC_VICPC_EXTRA_SNAP_LEVELS                          0xA1C
#define MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_UPPER             0xA20
#define MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_UPPER             0xA24
#define MC_STAT_EMC_FILTER_SET0_VIRTUAL_ADR_LIMIT_UPPER     0xA28
#define MC_STAT_EMC_FILTER_SET1_VIRTUAL_ADR_LIMIT_UPPER     0xA2C
#define MC_JPG_EXTRA_SNAP_LEVELS                            0xA3C
#define MC_GK2_EXTRA_SNAP_LEVELS                            0xA40
#define MC_SDM_EXTRA_SNAP_LEVELS                            0xA44
#define MC_HDAPC_EXTRA_SNAP_LEVELS                          0xA48
#define MC_DFD_EXTRA_SNAP_LEVELS                            0xA4C
#define MC_SMMU_DC1_ASID                                    0xA88
#define MC_SMMU_SDMMC1A_ASID                                0xA94
#define MC_SMMU_SDMMC2A_ASID                                0xA98
#define MC_SMMU_SDMMC3A_ASID                                0xA9C
#define MC_SMMU_SDMMC4A_ASID                                0xAA0
#define MC_SMMU_ISP2B_ASID                                  0xAA4
#define MC_SMMU_GPU_ASID                                    0xAA8
#define MC_SMMU_GPUB_ASID                                   0xAAC
#define MC_SMMU_PPCS2_ASID                                  0xAB0
#define MC_SMMU_NVDEC_ASID                                  0xAB4
#define MC_SMMU_APE_ASID                                    0xAB8
#define MC_SMMU_SE_ASID                                     0xABC
#define MC_SMMU_NVJPG_ASID                                  0xAC0
#define MC_SMMU_HC1_ASID                                    0xAC4
#define MC_SMMU_SE1_ASID                                    0xAC8
#define MC_SMMU_AXIAP_ASID                                  0xACC
#define MC_SMMU_ETR_ASID                                    0xAD0
#define MC_SMMU_TSECB_ASID                                  0xAD4
#define MC_SMMU_TSEC1_ASID                                  0xAD8
#define MC_SMMU_TSECB1_ASID                                 0xADC
#define MC_SMMU_NVDEC1_ASID                                 0xAE0
#define MC_MIN_LENGTH_GPU_0                                 0xB04
#define MC_MIN_LENGTH_SDMMCA_0                              0xB10
#define MC_MIN_LENGTH_SDMMCAA_0                             0xB14
#define MC_MIN_LENGTH_SDMMC_0                               0xB18
#define MC_MIN_LENGTH_SDMMCAB_0                             0xB1C
#define MC_MIN_LENGTH_DC_3                                  0xB20
#define MC_MIN_LENGTH_NVDEC_0                               0xB30
#define MC_MIN_LENGTH_APE_0                                 0xB34
#define MC_MIN_LENGTH_SE_0                                  0xB38
#define MC_MIN_LENGTH_NVJPG_0                               0xB3C
#define MC_MIN_LENGTH_GPU2_0                                0xB40
#define MC_MIN_LENGTH_ETR_0                                 0xB44
#define MC_MIN_LENGTH_TSECB_0                               0xB48
#define MC_EMEM_ARB_NISO_THROTTLE_MASK_1                    0xB80
#define MC_EMEM_ARB_HYSTERESIS_4                            0xB84
#define MC_STAT_EMC_FILTER_SET0_CLIENT_4                    0xB88
#define MC_STAT_EMC_FILTER_SET1_CLIENT_4                    0xB8C
#define MC_EMEM_ARB_ISOCHRONOUS_4                           0xB94
#define MC_SMMU_TRANSLATION_ENABLE_4                        0xB98
#define MC_SMMU_CLIENT_CONFIG4                              0xB9C
#define MC_EMEM_ARB_DHYSTERESIS_0                           0xBB0
#define MC_EMEM_ARB_DHYSTERESIS_1                           0xBB4
#define MC_EMEM_ARB_DHYSTERESIS_2                           0xBB8
#define MC_EMEM_ARB_DHYSTERESIS_3                           0xBBC
#define MC_EMEM_ARB_DHYSTERESIS_4                           0xBC0
#define MC_EMEM_ARB_DHYST_CTRL                              0xBCC
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_0                    0xBD0
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_1                    0xBD4
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_2                    0xBD8
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_3                    0xBDC
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_4                    0xBE0
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_5                    0xBE4
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_6                    0xBE8
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_7                    0xBEC
#define MC_ERR_GENERALIZED_CARVEOUT_STATUS                  0xC00
#define MC_ERR_GENERALIZED_CARVEOUT_ADR                     0xC04
#define MC_SECURITY_CARVEOUT1_CFG0                          0xC08
#define MC_SECURITY_CARVEOUT1_BOM                           0xC0C
#define MC_SECURITY_CARVEOUT1_BOM_HI                        0xC10
#define MC_SECURITY_CARVEOUT1_SIZE_128KB                    0xC14
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS0                0xC18
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS1                0xC1C
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS2                0xC20
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS3                0xC24
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS4                0xC28
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS0 0xC2C
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS1 0xC30
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS2 0xC34
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS3 0xC38
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS4 0xC3C
#define MC_SECURITY_CARVEOUT2_CFG0                          0xC58
#define MC_SECURITY_CARVEOUT2_BOM                           0xC5C
#define MC_SECURITY_CARVEOUT2_BOM_HI                        0xC60
#define MC_SECURITY_CARVEOUT2_SIZE_128KB                    0xC64
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS0                0xC68
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS1                0xC6C
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS2                0xC70
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS3                0xC74
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS4                0xC78
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS0 0xC7C
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS1 0xC80
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS2 0xC84
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS3 0xC88
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS4 0xC8C
#define MC_SECURITY_CARVEOUT3_CFG0                          0xCA8
#define MC_SECURITY_CARVEOUT3_BOM                           0xCAC
#define MC_SECURITY_CARVEOUT3_BOM_HI                        0xCB0
#define MC_SECURITY_CARVEOUT3_SIZE_128KB                    0xCB4
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS0                0xCB8
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS1                0xCBC
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS2                0xCC0
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS3                0xCC4
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS4                0xCC8
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS0 0xCCC
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS1 0xCD0
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS2 0xCD4
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS3 0xCD8
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS4 0xCDC
#define MC_SECURITY_CARVEOUT4_CFG0                          0xCF8
#define MC_SECURITY_CARVEOUT4_BOM                           0xCFC
#define MC_SECURITY_CARVEOUT4_BOM_HI                        0xD00
#define MC_SECURITY_CARVEOUT4_SIZE_128KB                    0xD04
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS0                0xD08
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS1                0xD0C
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS2                0xD10
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS3                0xD14
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS4                0xD18
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS0 0xD1C
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS1 0xD20
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS2 0xD24
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS3 0xD28
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS4 0xD2C
#define MC_SECURITY_CARVEOUT5_CFG0                          0xD48
#define MC_SECURITY_CARVEOUT5_BOM                           0xD4C
#define MC_SECURITY_CARVEOUT5_BOM_HI                        0xD50
#define MC_SECURITY_CARVEOUT5_SIZE_128KB                    0xD54
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS0                0xD58
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS1                0xD5C
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS2                0xD60
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS3                0xD64
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS4                0xD68
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS0 0xD6C
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS1 0xD70
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS2 0xD74
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS3 0xD78
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS4 0xD7C
#define MC_PCFIFO_CLIENT_CONFIG0                            0xDD0
#define MC_PCFIFO_CLIENT_CONFIG1                            0xDD4
#define MC_PCFIFO_CLIENT_CONFIG2                            0xDD8
#define MC_PCFIFO_CLIENT_CONFIG3                            0xDDC
#define MC_PCFIFO_CLIENT_CONFIG4                            0xDE0

/* T210B01 only registers */
#define MC_SMMU_ISP21_ASID_B01                              0x804
#define MC_SMMU_ISP2B1_ASID_B01                             0x808
#define MC_UNTRANSLATED_REGION_CHECK_B01                    0x948

/*! MC_SECURITY_CARVEOUTX_CLIENT_ACCESS/CLIENT_FORCE_INTERNAL_ACCESS0 */
#define SEC_CARVEOUT_CA0_R_PTCR       BIT(0)
#define SEC_CARVEOUT_CA0_R_DISPLAY0A  BIT(1)
#define SEC_CARVEOUT_CA0_R_DISPLAY0AB BIT(2)
#define SEC_CARVEOUT_CA0_R_DISPLAY0B  BIT(3)
#define SEC_CARVEOUT_CA0_R_DISPLAY0BB BIT(4)
#define SEC_CARVEOUT_CA0_R_DISPLAY0C  BIT(5)
#define SEC_CARVEOUT_CA0_R_DISPLAY0CB BIT(6)
#define SEC_CARVEOUT_CA0_R_AFI        BIT(14)
#define SEC_CARVEOUT_CA0_R_BPMP_C     BIT(15)
#define SEC_CARVEOUT_CA0_R_DISPLAYHC  BIT(16)
#define SEC_CARVEOUT_CA0_R_DISPLAYHCB BIT(17)
#define SEC_CARVEOUT_CA0_R_HDA        BIT(21)
#define SEC_CARVEOUT_CA0_R_HOST1XDMA  BIT(22)
#define SEC_CARVEOUT_CA0_R_HOST1X     BIT(23)
#define SEC_CARVEOUT_CA0_R_NVENC      BIT(28)
#define SEC_CARVEOUT_CA0_R_PPCSAHBDMA BIT(29)
#define SEC_CARVEOUT_CA0_R_PPCSAHBSLV BIT(30)
#define SEC_CARVEOUT_CA0_R_SATAR      BIT(31)

/*! MC_SECURITY_CARVEOUTX_CLIENT_ACCESS/CLIENT_FORCE_INTERNAL_ACCESS1 */
#define SEC_CARVEOUT_CA1_R_VDEBSEV    BIT(2)
#define SEC_CARVEOUT_CA1_R_VDEMBE     BIT(3)
#define SEC_CARVEOUT_CA1_R_VDEMCE     BIT(4)
#define SEC_CARVEOUT_CA1_R_VDETPE     BIT(5)
#define SEC_CARVEOUT_CA1_R_CCPLEXLP_C BIT(6)
#define SEC_CARVEOUT_CA1_R_CCPLEX_C   BIT(7)
#define SEC_CARVEOUT_CA1_W_NVENC      BIT(11)
#define SEC_CARVEOUT_CA1_W_AFI        BIT(17)
#define SEC_CARVEOUT_CA1_W_BPMP_C     BIT(18)
#define SEC_CARVEOUT_CA1_W_HDA        BIT(21)
#define SEC_CARVEOUT_CA1_W_HOST1X     BIT(22)
#define SEC_CARVEOUT_CA1_W_CCPLEXLP_C BIT(24)
#define SEC_CARVEOUT_CA1_W_CCPLEX_C   BIT(25)
#define SEC_CARVEOUT_CA1_W_PPCSAHBDMA BIT(27)
#define SEC_CARVEOUT_CA1_W_PPCSAHBSLV BIT(28)
#define SEC_CARVEOUT_CA1_W_SATA       BIT(29)
#define SEC_CARVEOUT_CA1_W_VDEBSEV    BIT(30)
#define SEC_CARVEOUT_CA1_W_VDEDBG     BIT(31)

/*! MC_SECURITY_CARVEOUTX_CLIENT_ACCESS/CLIENT_FORCE_INTERNAL_ACCESS2 */
#define SEC_CARVEOUT_CA2_W_VDEMBE    BIT(0)
#define SEC_CARVEOUT_CA2_W_VDETPM    BIT(1)
#define SEC_CARVEOUT_CA2_R_ISPRA     BIT(4)
#define SEC_CARVEOUT_CA2_W_ISPWA     BIT(6)
#define SEC_CARVEOUT_CA2_W_ISPWB     BIT(7)
#define SEC_CARVEOUT_CA2_R_XUSB_HOST BIT(10)
#define SEC_CARVEOUT_CA2_W_XUSB_HOST BIT(11)
#define SEC_CARVEOUT_CA2_R_XUSB_DEV  BIT(12)
#define SEC_CARVEOUT_CA2_W_XUSB_DEV  BIT(13)
#define SEC_CARVEOUT_CA2_R_SE2       BIT(14)
#define SEC_CARVEOUT_CA2_W_SE2       BIT(16)
#define SEC_CARVEOUT_CA2_R_TSEC      BIT(20)
#define SEC_CARVEOUT_CA2_W_TSEC      BIT(21)
#define SEC_CARVEOUT_CA2_R_ADSP_SC   BIT(22)
#define SEC_CARVEOUT_CA2_W_ADSP_SC   BIT(23)
#define SEC_CARVEOUT_CA2_R_GPU       BIT(24)
#define SEC_CARVEOUT_CA2_W_GPU       BIT(25)
#define SEC_CARVEOUT_CA2_R_DISPLAYT  BIT(26)

/*! MC_SECURITY_CARVEOUTX_CLIENT_ACCESS/CLIENT_FORCE_INTERNAL_ACCESS3 */
#define SEC_CARVEOUT_CA3_R_SDMMCA   BIT(0)
#define SEC_CARVEOUT_CA3_R_SDMMCAA  BIT(1)
#define SEC_CARVEOUT_CA3_R_SDMMC    BIT(2)
#define SEC_CARVEOUT_CA3_R_SDMMCAB  BIT(3)
#define SEC_CARVEOUT_CA3_W_SDMMCA   BIT(4)
#define SEC_CARVEOUT_CA3_W_SDMMCAA  BIT(5)
#define SEC_CARVEOUT_CA3_W_SDMMC    BIT(6)
#define SEC_CARVEOUT_CA3_W_SDMMCAB  BIT(7)
#define SEC_CARVEOUT_CA3_R_VIC      BIT(12)
#define SEC_CARVEOUT_CA3_W_VIC      BIT(13)
#define SEC_CARVEOUT_CA3_W_VIW      BIT(18)
#define SEC_CARVEOUT_CA3_R_DISPLAYD BIT(19)
#define SEC_CARVEOUT_CA3_R_NVDEC    BIT(24)
#define SEC_CARVEOUT_CA3_W_NVDEC    BIT(25)
#define SEC_CARVEOUT_CA3_R_APE      BIT(26)
#define SEC_CARVEOUT_CA3_W_APE      BIT(27)
#define SEC_CARVEOUT_CA3_R_NVJPG    BIT(30)
#define SEC_CARVEOUT_CA3_W_NVJPG    BIT(31)

/*! MC_SECURITY_CARVEOUTX_CLIENT_ACCESS/CLIENT_FORCE_INTERNAL_ACCESS4 */
#define SEC_CARVEOUT_CA4_R_SE    BIT(0)
#define SEC_CARVEOUT_CA4_W_SE    BIT(1)
#define SEC_CARVEOUT_CA4_R_AXIAP BIT(2)
#define SEC_CARVEOUT_CA4_W_AXIAP BIT(3)
#define SEC_CARVEOUT_CA4_R_ETR   BIT(4)
#define SEC_CARVEOUT_CA4_W_ETR   BIT(5)
#define SEC_CARVEOUT_CA4_R_TSECB BIT(6)
#define SEC_CARVEOUT_CA4_W_TSECB BIT(7)
#define SEC_CARVEOUT_CA4_R_GPU2  BIT(8)
#define SEC_CARVEOUT_CA4_W_GPU2  BIT(9)

/*! MC_VIDEO_PROTECT_REG_CTRL */
#define VPR_LOCK_MODE_SHIFT 0
#define VPR_CTRL_UNLOCKED  (0 << VPR_LOCK_MODE_SHIFT)
#define VPR_CTRL_LOCKED    (1 << VPR_LOCK_MODE_SHIFT)
#define VPR_PROTECT_MODE_SHIFT 1
#define SEC_CTRL_SECURE    (0 << VPR_PROTECT_MODE_SHIFT)
#define VPR_CTRL_TZ_SECURE (1 << VPR_PROTECT_MODE_SHIFT)

/*! MC_SECURITY_CARVEOUTX_CFG0 */
// Mode of LOCK_MODE.
#define PROTECT_MODE_SHIFT 0
#define SEC_CARVEOUT_CFG_ALL_SECURE (0 << PROTECT_MODE_SHIFT)
#define SEC_CARVEOUT_CFG_TZ_SECURE  (1 << PROTECT_MODE_SHIFT)
// Enables PROTECT_MODE.
#define LOCK_MODE_SHIFT 1
#define SEC_CARVEOUT_CFG_UNLOCKED (0 << LOCK_MODE_SHIFT)
#define SEC_CARVEOUT_CFG_LOCKED   (1 << LOCK_MODE_SHIFT)

#define ADDRESS_TYPE_SHIFT 2
#define SEC_CARVEOUT_CFG_ANY_ADDRESS       (0 << ADDRESS_TYPE_SHIFT)
#define SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY (1 << ADDRESS_TYPE_SHIFT)

#define READ_ACCESS_LEVEL_SHIFT 3
#define SEC_CARVEOUT_CFG_RD_NS        (1 << READ_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_RD_SEC       (2 << READ_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_RD_FALCON_LS (4 << READ_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_RD_FALCON_HS (8 << READ_ACCESS_LEVEL_SHIFT)

#define WRITE_ACCESS_LEVEL_SHIFT 7
#define SEC_CARVEOUT_CFG_WR_NS        (1 << WRITE_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_WR_SEC       (2 << WRITE_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_WR_FALCON_LS (4 << WRITE_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_WR_FALCON_HS (8 << WRITE_ACCESS_LEVEL_SHIFT)

#define SEC_CARVEOUT_CFG_APERTURE_ID_MASK (3 << 11)
#define SEC_CARVEOUT_CFG_APERTURE_ID(id) ((id) << 11)

#define DISABLE_READ_CHECK_ACCESS_LEVEL_SHIFT 14
#define SEC_CARVEOUT_CFG_DIS_RD_CHECK_NS (1 << DISABLE_READ_CHECK_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_DIS_RD_CHECK_SEC (2 << DISABLE_READ_CHECK_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_DIS_RD_CHECK_FLCN_LS (4 << DISABLE_READ_CHECK_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_DIS_RD_CHECK_FLCN_HS (8 << DISABLE_READ_CHECK_ACCESS_LEVEL_SHIFT)

#define DISABLE_WRITE_CHECK_ACCESS_LEVEL_SHIFT 18
#define SEC_CARVEOUT_CFG_DIS_WR_CHECK_NS (1 << DISABLE_WRITE_CHECK_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_DIS_WR_CHECK_SEC (2 << DISABLE_WRITE_CHECK_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_DIS_WR_CHECK_FLCN_LS (4 << DISABLE_WRITE_CHECK_ACCESS_LEVEL_SHIFT)
#define SEC_CARVEOUT_CFG_DIS_WR_CHECK_FLCN_HS (8 << DISABLE_WRITE_CHECK_ACCESS_LEVEL_SHIFT)

#define SEC_CARVEOUT_CFG_SEND_CFG_TO_GPU BIT(22)

#define SEC_CARVEOUT_CFG_TZ_GLOBAL_WR_EN_BYPASS_CHECK BIT(23)
#define SEC_CARVEOUT_CFG_TZ_GLOBAL_RD_EN_BYPASS_CHECK BIT(24)

#define SEC_CARVEOUT_CFG_ALLOW_APERTURE_ID_MISMATCH BIT(25)
#define SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH    BIT(26)

#define SEC_CARVEOUT_CFG_IS_WPR BIT(27)

// WPR1 magic to enable WPR2.
#define ACR_GSC3_ENABLE_MAGIC 0xC0EDBBCC

/*! MC_VIDEO_PROTECT_GPU_OVERRIDE_0 */
// VPR CYA. Parsed as (vpr_gpu_ovr1 << 32 | vpr_gpu_ovr0) << 5.
#define VPR_TRUST_UNTRUSTED  0
#define VPR_TRUST_GRAPHICS   1
#define VPR_TRUST_QUARANTINE 2
#define VPR_TRUST_TRUSTED    3
// VPR CYA LO.
// Setting VPR_OVR0_CYA_TRUST_DEFAULT disables the overrides.
// Defaults: PD, SCC, SKED, L1, TEX, PE, RASTER, GCC and PROP as GRAPHICS. The rest UNTRUSTED.
#define VPR_OVR0_CYA_TRUST_OVERRIDE   0
#define VPR_OVR0_CYA_TRUST_DEFAULT    BIT(0)
#define VPR_OVR0_CYA_TRUST_CPU(t)     ((t) <<  1u) // HOST CPU.
#define VPR_OVR0_CYA_TRUST_HOST(t)    ((t) <<  3u)
#define VPR_OVR0_CYA_TRUST_PERF(t)    ((t) <<  5u)
#define VPR_OVR0_CYA_TRUST_PMU(t)     ((t) <<  7u)
#define VPR_OVR0_CYA_TRUST_CE2(t)     ((t) <<  9u) // GRCOPY.
#define VPR_OVR0_CYA_TRUST_SEC(t)     ((t) << 11u)
#define VPR_OVR0_CYA_TRUST_FE(t)      ((t) << 13u)
#define VPR_OVR0_CYA_TRUST_PD(t)      ((t) << 15u)
#define VPR_OVR0_CYA_TRUST_SCC(t)     ((t) << 17u)
#define VPR_OVR0_CYA_TRUST_SKED(t)    ((t) << 19u)
#define VPR_OVR0_CYA_TRUST_L1(t)      ((t) << 21u)
#define VPR_OVR0_CYA_TRUST_TEX(t)     ((t) << 23u)
#define VPR_OVR0_CYA_TRUST_PE(t)      ((t) << 25u)
// VPR CYA HI.
#define VPR_OVR0_CYA_TRUST_RASTER(t)  ((t) << 27u)
#define VPR_OVR0_CYA_TRUST_GCC(t)     ((t) << 29u)
// Setting GPCCS to anything other than untrusted, causes a hang.
#define VPR_OVR0_CYA_TRUST_GPCCS(t)   (((t) & 1) << 31u)

/*! MC_VIDEO_PROTECT_GPU_OVERRIDE_1 */
// VPR CYA HI.
#define VPR_OVR1_CYA_TRUST_GPCCS(t)   ((t) >> 1)
#define VPR_OVR1_CYA_TRUST_PROP(t)    ((t) <<  1u)
#define VPR_OVR1_CYA_TRUST_PROP_READ  BIT(3)
#define VPR_OVR1_CYA_TRUST_PROP_WRITE BIT(4)
#define VPR_OVR1_CYA_TRUST_DNISO(t)   ((t) <<  5u)
#define VPR_OVR1_CYA_TRUST_CE0(t)     VPR_OVR1_CYA_TRUST_DNISO(t)
#define VPR_OVR1_CYA_TRUST_CE1(t)     VPR_OVR1_CYA_TRUST_DNISO(t)
#define VPR_OVR1_CYA_TRUST_NVENC(t)   ((t) <<  7u) // Unused?
#define VPR_OVR1_CYA_TRUST_NVDEC(t)   ((t) <<  9u) // Unused?
#define VPR_OVR1_CYA_TRUST_MSPPP(t)   ((t) << 11u) // Unused? VIC/JPG?
#define VPR_OVR1_CYA_TRUST_MSVLD(t)   ((t) << 13u) // Unused? SEC2?

typedef struct _mc_regs_t210_t {
/* 0x000 */	u32 mc_intstatus;
/* 0x004 */	u32 mc_intmask;
/* 0x008 */	u32 mc_err_status;
/* 0x00c */	u32 mc_err_adr;
/* 0x010 */	u32 mc_smmu_config;
/* 0x014 */	u32 mc_smmu_tlb_config;
/* 0x018 */	u32 mc_smmu_ptc_config;
/* 0x01c */	u32 mc_smmu_ptb_asid;
/* 0x020 */	u32 mc_smmu_ptb_data;
/* 0x024 */	u32 rsvd_024[3];
/* 0x030 */	u32 mc_smmu_tlb_flush;
/* 0x034 */	u32 mc_smmu_ptc_flush;
/* 0x038 */	u32 mc_smmu_asid_security;
/* 0x03c */	u32 mc_smmu_asid_security_1;
/* 0x040 */	u32 mc_smmu_client_config0;
/* 0x044 */	u32 mc_smmu_client_config1;
/* 0x048 */	u32 mc_smmu_client_config2;
/* 0x04c */	u32 mc_smmu_client_config3;
/* 0x050 */	u32 mc_emem_cfg;
/* 0x054 */	u32 mc_emem_adr_cfg;
/* 0x058 */	u32 mc_emem_adr_cfg_dev0;
/* 0x05c */	u32 mc_emem_adr_cfg_dev1;
/* 0x060 */	u32 mc_emem_adr_cfg_channel_mask;
/* 0x064 */	u32 mc_emem_adr_cfg_bank_mask_0;
/* 0x068 */	u32 mc_emem_adr_cfg_bank_mask_1;
/* 0x06c */	u32 mc_emem_adr_cfg_bank_mask_2;
/* 0x070 */	u32 mc_security_cfg0;
/* 0x074 */	u32 mc_security_cfg1;
/* 0x078 */	u32 rsvd_078;
/* 0x07c */	u32 mc_security_rsv;
/* 0x080 */	u32 rsvd_080[4];
/* 0x090 */	u32 mc_emem_arb_cfg;
/* 0x094 */	u32 mc_emem_arb_outstanding_req;
/* 0x098 */	u32 mc_emem_arb_timing_rcd;
/* 0x09c */	u32 mc_emem_arb_timing_rp;
/* 0x0a0 */	u32 mc_emem_arb_timing_rc;
/* 0x0a4 */	u32 mc_emem_arb_timing_ras;
/* 0x0a8 */	u32 mc_emem_arb_timing_faw;
/* 0x0ac */	u32 mc_emem_arb_timing_rrd;
/* 0x0b0 */	u32 mc_emem_arb_timing_rap2pre;
/* 0x0b4 */	u32 mc_emem_arb_timing_wap2pre;
/* 0x0b8 */	u32 mc_emem_arb_timing_r2r;
/* 0x0bc */	u32 mc_emem_arb_timing_w2w;
/* 0x0c0 */	u32 mc_emem_arb_timing_r2w;
/* 0x0c4 */	u32 mc_emem_arb_timing_w2r;
/* 0x0c8 */	u32 mc_emem_arb_misc2;
/* 0x0cc */	u32 rsvd_0cc;
/* 0x0d0 */	u32 mc_emem_arb_da_turns;
/* 0x0d4 */	u32 mc_emem_arb_da_covers;
/* 0x0d8 */	u32 mc_emem_arb_misc0;
/* 0x0dc */	u32 mc_emem_arb_misc1;
/* 0x0e0 */	u32 mc_emem_arb_ring1_throttle;
/* 0x0e4 */	u32 mc_emem_arb_ring3_throttle;
/* 0x0e8 */	u32 mc_emem_arb_override;
/* 0x0ec */	u32 mc_emem_arb_rsv;
/* 0x0f0 */	u32 rsvd_0f0;
/* 0x0f4 */	u32 mc_clken_override;
/* 0x0f8 */	u32 mc_timing_control_dbg;
/* 0x0fc */	u32 mc_timing_control;
/* 0x100 */	u32 mc_stat_control;
/* 0x104 */	u32 mc_stat_status;
/* 0x108 */	u32 mc_stat_emc_clock_limit;
/* 0x10c */	u32 mc_stat_emc_clock_limit_msbs;
/* 0x110 */	u32 mc_stat_emc_clocks;
/* 0x114 */	u32 mc_stat_emc_clocks_msbs;
/* 0x118 */	u32 mc_stat_emc_filter_set0_adr_limit_lo;
/* 0x11c */	u32 mc_stat_emc_filter_set0_adr_limit_hi;
/* 0x120 */	u32 mc_stat_emc_filter_set0_slack_limit;
/* 0x124 */	u32 rsvd_124;
/* 0x128 */	u32 mc_stat_emc_filter_set0_client_0;
/* 0x12c */	u32 mc_stat_emc_filter_set0_client_1;
/* 0x130 */	u32 mc_stat_emc_filter_set0_client_2;
/* 0x134 */	u32 mc_stat_emc_filter_set0_client_3;
/* 0x138 */	u32 mc_stat_emc_set0_count;
/* 0x13c */	u32 mc_stat_emc_set0_count_msbs;
/* 0x140 */	u32 mc_stat_emc_set0_slack_accum;
/* 0x144 */	u32 mc_stat_emc_set0_slack_accum_msbs;
/* 0x148 */	u32 mc_stat_emc_set0_histo_count;
/* 0x14c */	u32 mc_stat_emc_set0_histo_count_msbs;
/* 0x150 */	u32 mc_stat_emc_set0_minimum_slack_observed;
/* 0x154 */	u32 rsvd_154;
/* 0x158 */	u32 mc_stat_emc_filter_set1_adr_limit_lo;
/* 0x15c */	u32 mc_stat_emc_filter_set1_adr_limit_hi;
/* 0x160 */	u32 mc_stat_emc_filter_set1_slack_limit;
/* 0x164 */	u32 rsvd_164;
/* 0x168 */	u32 mc_stat_emc_filter_set1_client_0;
/* 0x16c */	u32 mc_stat_emc_filter_set1_client_1;
/* 0x170 */	u32 mc_stat_emc_filter_set1_client_2;
/* 0x174 */	u32 mc_stat_emc_filter_set1_client_3;
/* 0x178 */	u32 mc_stat_emc_set1_count;
/* 0x17c */	u32 mc_stat_emc_set1_count_msbs;
/* 0x180 */	u32 mc_stat_emc_set1_slack_accum;
/* 0x184 */	u32 mc_stat_emc_set1_slack_accum_msbs;
/* 0x188 */	u32 mc_stat_emc_set1_histo_count;
/* 0x18c */	u32 mc_stat_emc_set1_histo_count_msbs;
/* 0x190 */	u32 mc_stat_emc_set1_minimum_slack_observed;
/* 0x194 */	u32 rsvd_194;
/* 0x198 */	u32 mc_stat_emc_filter_set0_virtual_adr_limit_lo;
/* 0x19c */	u32 mc_stat_emc_filter_set0_virtual_adr_limit_hi;
/* 0x1a0 */	u32 mc_stat_emc_filter_set0_asid;
/* 0x1a4 */	u32 rsvd_1a4;
/* 0x1a8 */	u32 mc_stat_emc_filter_set1_virtual_adr_limit_lo;
/* 0x1ac */	u32 mc_stat_emc_filter_set1_virtual_adr_limit_hi;
/* 0x1b0 */	u32 mc_stat_emc_filter_set1_asid;
/* 0x1b4 */	u32 rsvd_1b4;
/* 0x1b8 */	u32 mc_stat_emc_set0_idle_cycle_count;
/* 0x1bc */	u32 mc_stat_emc_set0_idle_cycle_count_msbs;
/* 0x1c0 */	u32 mc_stat_emc_set0_idle_cycle_partition_select;
/* 0x1c4 */	u32 rsvd_1c4;
/* 0x1c8 */	u32 mc_stat_emc_set1_idle_cycle_count;
/* 0x1cc */	u32 mc_stat_emc_set1_idle_cycle_count_msbs;
/* 0x1d0 */	u32 mc_stat_emc_set1_idle_cycle_partition_select;
/* 0x1d4 */	u32 rsvd_1d4[6];
/* 0x1ec */	u32 mc_smmu_stats_tlb_hit_miss_source;
/* 0x1f0 */	u32 mc_smmu_stats_tlb_hit_count;
/* 0x1f4 */	u32 mc_smmu_stats_tlb_miss_count;
/* 0x1f8 */	u32 mc_smmu_stats_ptc_hit_count;
/* 0x1fc */	u32 mc_smmu_stats_ptc_miss_count;
/* 0x200 */	u32 mc_client_hotreset_ctrl;
/* 0x204 */	u32 mc_client_hotreset_status;
/* 0x208 */	u32 mc_emem_arb_isochronous_0;
/* 0x20c */	u32 mc_emem_arb_isochronous_1;
/* 0x210 */	u32 mc_emem_arb_isochronous_2;
/* 0x214 */	u32 mc_emem_arb_isochronous_3;
/* 0x218 */	u32 mc_emem_arb_hysteresis_0;
/* 0x21c */	u32 mc_emem_arb_hysteresis_1;
/* 0x220 */	u32 mc_emem_arb_hysteresis_2;
/* 0x224 */	u32 mc_emem_arb_hysteresis_3;
/* 0x228 */	u32 mc_smmu_translation_enable_0;
/* 0x22c */	u32 mc_smmu_translation_enable_1;
/* 0x230 */	u32 mc_smmu_translation_enable_2;
/* 0x234 */	u32 mc_smmu_translation_enable_3;
/* 0x238 */	u32 mc_smmu_afi_asid;
/* 0x23c */	u32 mc_smmu_avpc_asid;
/* 0x240 */	u32 mc_smmu_dc_asid;
/* 0x244 */	u32 mc_smmu_dcb_asid;
/* 0x248 */	u32 rsvd_248[2];
/* 0x250 */	u32 mc_smmu_hc_asid;
/* 0x254 */	u32 mc_smmu_hda_asid;
/* 0x258 */	u32 mc_smmu_isp2_asid;
/* 0x25c */	u32 rsvd_25c[2];
/* 0x264 */	u32 mc_smmu_nvenc_asid;
/* 0x268 */	u32 rsvd_268[2];
/* 0x270 */	u32 mc_smmu_ppcs_asid;
/* 0x274 */	u32 mc_smmu_sata_asid;
/* 0x278 */	u32 rsvd_278[2];
/* 0x280 */	u32 mc_smmu_vi_asid;
/* 0x284 */	u32 mc_smmu_vic_asid;
/* 0x288 */	u32 mc_smmu_xusb_host_asid;
/* 0x28c */	u32 mc_smmu_xusb_dev_asid;
/* 0x290 */	u32 mc_smmu_a9avp_asid;
/* 0x294 */	u32 mc_smmu_tsec_asid;
/* 0x298 */	u32 mc_smmu_ppcs1_asid;
/* 0x29c */	u32 rsvd_29c;
/* 0x2a0 */	u32 mc_ahb_extra_snap_levels;
/* 0x2a4 */	u32 mc_apb_extra_snap_levels;
/* 0x2a8 */	u32 mc_avp_extra_snap_levels;
/* 0x2ac */	u32 mc_dis_extra_snap_levels;
/* 0x2b0 */	u32 rsvd_2b0[2];
/* 0x2b8 */	u32 mc_pcx_extra_snap_levels;
/* 0x2bc */	u32 mc_ftop_extra_snap_levels;
/* 0x2c0 */	u32 mc_sax_extra_snap_levels;
/* 0x2c4 */	u32 rsvd_2c4[5];
/* 0x2d8 */	u32 mc_ve_extra_snap_levels;
/* 0x2dc */	u32 rsvd_2dc;
/* 0x2e0 */	u32 mc_latency_allowance_afi_0;
/* 0x2e4 */	u32 mc_latency_allowance_avpc_0;
/* 0x2e8 */	u32 mc_latency_allowance_dc_0;
/* 0x2ec */	u32 mc_latency_allowance_dc_1;
/* 0x2f0 */	u32 mc_latency_allowance_dc_2;
/* 0x2f4 */	u32 mc_latency_allowance_dcb_0;
/* 0x2f8 */	u32 mc_latency_allowance_dcb_1;
/* 0x2fc */	u32 mc_latency_allowance_dcb_2;
/* 0x300 */	u32 rsvd_300[4];
/* 0x310 */	u32 mc_latency_allowance_hc_0;
/* 0x314 */	u32 mc_latency_allowance_hc_1;
/* 0x318 */	u32 mc_latency_allowance_hda_0;
/* 0x31c */	u32 rsvd_31c;
/* 0x320 */	u32 mc_latency_allowance_mpcore_0;
/* 0x324 */	u32 rsvd_324;
/* 0x328 */	u32 mc_latency_allowance_nvenc_0;
/* 0x32c */	u32 rsvd_32c[6];
/* 0x344 */	u32 mc_latency_allowance_ppcs_0;
/* 0x348 */	u32 mc_latency_allowance_ppcs_1;
/* 0x34c */	u32 mc_latency_allowance_ptc_0;
/* 0x350 */	u32 mc_latency_allowance_sata_0;
/* 0x354 */	u32 rsvd_354[7];
/* 0x370 */	u32 mc_latency_allowance_isp2_0;
/* 0x374 */	u32 mc_latency_allowance_isp2_1;
/* 0x378 */	u32 rsvd_378;
/* 0x37c */	u32 mc_latency_allowance_xusb_0;
/* 0x380 */	u32 mc_latency_allowance_xusb_1;
/* 0x384 */	u32 mc_latency_allowance_isp2b_0;
/* 0x388 */	u32 mc_latency_allowance_isp2b_1;
/* 0x38c */	u32 rsvd_38c;
/* 0x390 */	u32 mc_latency_allowance_tsec_0;
/* 0x394 */	u32 mc_latency_allowance_vic_0;
/* 0x398 */	u32 mc_latency_allowance_vi2_0;
/* 0x39c */	u32 rsvd_39c;
/* 0x3a0 */	u32 mc_latency_allowance_axiap_0;
/* 0x3a4 */	u32 mc_latency_allowance_a9avp_0;
/* 0x3a8 */	u32 rsvd_3a8;
/* 0x3ac */	u32 mc_latency_allowance_gpu_0;
/* 0x3b0 */	u32 rsvd_3b0[2];
/* 0x3b8 */	u32 mc_latency_allowance_sdmmca_0;
/* 0x3bc */	u32 mc_latency_allowance_sdmmcaa_0;
/* 0x3c0 */	u32 mc_latency_allowance_sdmmc_0;
/* 0x3c4 */	u32 mc_latency_allowance_sdmmcab_0;
/* 0x3c8 */	u32 mc_latency_allowance_dc_3;
/* 0x3cc */	u32 rsvd_3cc[3];
/* 0x3d8 */	u32 mc_latency_allowance_nvdec_0;
/* 0x3dc */	u32 mc_latency_allowance_ape_0;
/* 0x3e0 */	u32 mc_latency_allowance_se_0;
/* 0x3e4 */	u32 mc_latency_allowance_nvjpg_0;
/* 0x3e8 */	u32 mc_latency_allowance_gpu2_0;
/* 0x3ec */	u32 mc_latency_allowance_etr_0;
/* 0x3f0 */	u32 mc_latency_allowance_tsecb_0;
/* 0x3f4 */	u32 rsvd_3f4[2];
/* 0x3fc */	u32 mc_reserved_rsv;
/* 0x400 */	u32 rsvd_400;
/* 0x404 */	u32 mc_usbx_extra_snap_levels;
/* 0x408 */	u32 mc_disb_extra_snap_levels;
/* 0x40c */	u32 mc_mse_extra_snap_levels;
/* 0x410 */	u32 mc_ve2_extra_snap_levels;
/* 0x414 */	u32 mc_a9avppc_extra_snap_levels;
/* 0x418 */	u32 mc_video_protect_vpr_override;
/* 0x41c */	u32 mc_dis_ptsa_rate;
/* 0x420 */	u32 mc_dis_ptsa_min;
/* 0x424 */	u32 mc_dis_ptsa_max;
/* 0x428 */	u32 mc_disb_ptsa_rate;
/* 0x42c */	u32 mc_disb_ptsa_min;
/* 0x430 */	u32 mc_disb_ptsa_max;
/* 0x434 */	u32 mc_ve_ptsa_rate;
/* 0x438 */	u32 mc_ve_ptsa_min;
/* 0x43c */	u32 mc_ve_ptsa_max;
/* 0x440 */	u32 mc_ring2_ptsa_rate;
/* 0x444 */	u32 mc_ring2_ptsa_min;
/* 0x448 */	u32 mc_ring2_ptsa_max;
/* 0x44c */	u32 mc_mll_mpcorer_ptsa_rate;
/* 0x450 */	u32 mc_mll_mpcorer_ptsa_min;
/* 0x454 */	u32 mc_mll_mpcorer_ptsa_max;
/* 0x458 */	u32 mc_smmu_smmu_ptsa_rate;
/* 0x45c */	u32 mc_smmu_smmu_ptsa_min;
/* 0x460 */	u32 mc_smmu_smmu_ptsa_max;
/* 0x464 */	u32 rsvd_464[6];
/* 0x47c */	u32 mc_ring1_ptsa_rate;
/* 0x480 */	u32 mc_ring1_ptsa_min;
/* 0x484 */	u32 mc_ring1_ptsa_max;
/* 0x488 */	u32 mc_a9avppc_ptsa_rate;
/* 0x48c */	u32 mc_a9avppc_ptsa_min;
/* 0x490 */	u32 mc_a9avppc_ptsa_max;
/* 0x494 */	u32 mc_ve2_ptsa_rate;
/* 0x498 */	u32 mc_ve2_ptsa_min;
/* 0x49c */	u32 mc_ve2_ptsa_max;
/* 0x4a0 */	u32 mc_isp_ptsa_rate;
/* 0x4a4 */	u32 mc_isp_ptsa_min;
/* 0x4a8 */	u32 mc_isp_ptsa_max;
/* 0x4ac */	u32 mc_pcx_ptsa_rate;
/* 0x4b0 */	u32 mc_pcx_ptsa_min;
/* 0x4b4 */	u32 mc_pcx_ptsa_max;
/* 0x4b8 */	u32 mc_sax_ptsa_rate;
/* 0x4bc */	u32 mc_sax_ptsa_min;
/* 0x4c0 */	u32 mc_sax_ptsa_max;
/* 0x4c4 */	u32 mc_mse_ptsa_rate;
/* 0x4c8 */	u32 mc_mse_ptsa_min;
/* 0x4cc */	u32 mc_mse_ptsa_max;
/* 0x4d0 */	u32 mc_sd_ptsa_rate;
/* 0x4d4 */	u32 mc_sd_ptsa_min;
/* 0x4d8 */	u32 mc_sd_ptsa_max;
/* 0x4dc */	u32 mc_ahb_ptsa_rate;
/* 0x4e0 */	u32 mc_ahb_ptsa_min;
/* 0x4e4 */	u32 mc_ahb_ptsa_max;
/* 0x4e8 */	u32 mc_apb_ptsa_rate;
/* 0x4ec */	u32 mc_apb_ptsa_min;
/* 0x4f0 */	u32 mc_apb_ptsa_max;
/* 0x4f4 */	u32 mc_avp_ptsa_rate;
/* 0x4f8 */	u32 mc_avp_ptsa_min;
/* 0x4fc */	u32 mc_avp_ptsa_max;
/* 0x500 */	u32 rsvd_500[3];
/* 0x50c */	u32 mc_ftop_ptsa_rate;
/* 0x510 */	u32 mc_ftop_ptsa_min;
/* 0x514 */	u32 mc_ftop_ptsa_max;
/* 0x518 */	u32 mc_host_ptsa_rate;
/* 0x51c */	u32 mc_host_ptsa_min;
/* 0x520 */	u32 mc_host_ptsa_max;
/* 0x524 */	u32 mc_usbx_ptsa_rate;
/* 0x528 */	u32 mc_usbx_ptsa_min;
/* 0x52c */	u32 mc_usbx_ptsa_max;
/* 0x530 */	u32 mc_usbd_ptsa_rate;
/* 0x534 */	u32 mc_usbd_ptsa_min;
/* 0x538 */	u32 mc_usbd_ptsa_max;
/* 0x53c */	u32 mc_gk_ptsa_rate;
/* 0x540 */	u32 mc_gk_ptsa_min;
/* 0x544 */	u32 mc_gk_ptsa_max;
/* 0x548 */	u32 mc_aud_ptsa_rate;
/* 0x54c */	u32 mc_aud_ptsa_min;
/* 0x550 */	u32 mc_aud_ptsa_max;
/* 0x554 */	u32 mc_vicpc_ptsa_rate;
/* 0x558 */	u32 mc_vicpc_ptsa_min;
/* 0x55c */	u32 mc_vicpc_ptsa_max;
/* 0x560 */	u32 rsvd_560[9];
/* 0x584 */	u32 mc_jpg_ptsa_rate;
/* 0x588 */	u32 mc_jpg_ptsa_min;
/* 0x58c */	u32 mc_jpg_ptsa_max;
/* 0x590 */	u32 mc_video_protect_vpr_override1;
/* 0x594 */	u32 rsvd_594[27];
/* 0x600 */	u32 mc_smmu_tlb_set_selection_mask_0;
/* 0x604 */	u32 rsvd_604[3];
/* 0x610 */	u32 mc_gk2_ptsa_rate;
/* 0x614 */	u32 mc_gk2_ptsa_min;
/* 0x618 */	u32 mc_gk2_ptsa_max;
/* 0x61c */	u32 mc_sdm_ptsa_rate;
/* 0x620 */	u32 mc_sdm_ptsa_min;
/* 0x624 */	u32 mc_sdm_ptsa_max;
/* 0x628 */	u32 mc_hdapc_ptsa_rate;
/* 0x62c */	u32 mc_hdapc_ptsa_min;
/* 0x630 */	u32 mc_hdapc_ptsa_max;
/* 0x634 */	u32 mc_dfd_ptsa_rate;
/* 0x638 */	u32 mc_dfd_ptsa_min;
/* 0x63c */	u32 mc_dfd_ptsa_max;
/* 0x640 */	u32 rsvd_640[2];
/* 0x648 */	u32 mc_video_protect_bom;
/* 0x64c */	u32 mc_video_protect_size_mb;
/* 0x650 */	u32 mc_video_protect_reg_ctrl;
/* 0x654 */	u32 mc_err_vpr_status;
/* 0x658 */	u32 mc_err_vpr_adr;
/* 0x65c */	u32 mc_iram_bom;
/* 0x660 */	u32 mc_iram_tom;
/* 0x664 */	u32 mc_emem_cfg_access_ctrl;
/* 0x668 */	u32 mc_tz_security_ctrl;
/* 0x66c */	u32 mc_emem_arb_outstanding_req_ring3;
/* 0x670 */	u32 mc_sec_carveout_bom;
/* 0x674 */	u32 mc_sec_carveout_size_mb;
/* 0x678 */	u32 mc_sec_carveout_reg_ctrl;
/* 0x67c */	u32 mc_err_sec_status;
/* 0x680 */	u32 mc_err_sec_adr;
/* 0x684 */	u32 mc_pc_idle_clock_gate_config;
/* 0x688 */	u32 mc_stutter_control;
/* 0x68c */	u32 rsvd_68c;
/* 0x690 */	u32 mc_scaled_latency_allowance_display0a;
/* 0x694 */	u32 mc_scaled_latency_allowance_display0ab;
/* 0x698 */	u32 mc_scaled_latency_allowance_display0b;
/* 0x69c */	u32 mc_scaled_latency_allowance_display0bb;
/* 0x6a0 */	u32 mc_scaled_latency_allowance_display0c;
/* 0x6a4 */	u32 mc_scaled_latency_allowance_display0cb;
/* 0x6a8 */	u32 rsvd_6a8[2];
/* 0x6b0 */	u32 mc_emem_arb_niso_throttle;
/* 0x6b4 */	u32 mc_emem_arb_outstanding_req_niso;
/* 0x6b8 */	u32 mc_emem_arb_niso_throttle_mask;
/* 0x6bc */	u32 mc_emem_arb_ring0_throttle_mask;
/* 0x6c0 */	u32 mc_emem_arb_timing_rfcpb;
/* 0x6c4 */	u32 mc_emem_arb_timing_ccdmw;
/* 0x6c8 */	u32 rsvd_6c8[10];
/* 0x6f0 */	u32 mc_emem_arb_refpb_hp_ctrl;
/* 0x6f4 */	u32 mc_emem_arb_refpb_bank_ctrl;
/* 0x6f8 */	u32 rsvd_6f8[67];
/* 0x804 */	u32 mc_smmu_isp21_asid_b01;
/* 0x808 */	u32 mc_smmu_isp2b1_asid_b01;
/* 0x80c */	u32 rsvd_80c[32];
/* 0x88c */	u32 mc_min_length_afi_0;
/* 0x890 */	u32 mc_min_length_avpc_0;
/* 0x894 */	u32 mc_min_length_dc_0;
/* 0x898 */	u32 mc_min_length_dc_1;
/* 0x89c */	u32 mc_min_length_dc_2;
/* 0x8a0 */	u32 mc_min_length_dcb_0;
/* 0x8a4 */	u32 mc_min_length_dcb_1;
/* 0x8a8 */	u32 mc_min_length_dcb_2;
/* 0x8ac */	u32 rsvd_8ac[4];
/* 0x8bc */	u32 mc_min_length_hc_0;
/* 0x8c0 */	u32 mc_min_length_hc_1;
/* 0x8c4 */	u32 mc_min_length_hda_0;
/* 0x8c8 */	u32 rsvd_8c8;
/* 0x8cc */	u32 mc_min_length_mpcore_0;
/* 0x8d0 */	u32 rsvd_8d0;
/* 0x8d4 */	u32 mc_min_length_nvenc_0;
/* 0x8d8 */	u32 rsvd_8d8[6];
/* 0x8f0 */	u32 mc_min_length_ppcs_0;
/* 0x8f4 */	u32 mc_min_length_ppcs_1;
/* 0x8f8 */	u32 mc_min_length_ptc_0;
/* 0x8fc */	u32 mc_min_length_sata_0;
/* 0x900 */	u32 rsvd_900[7];
/* 0x91c */	u32 mc_min_length_isp2_0;
/* 0x920 */	u32 mc_min_length_isp2_1;
/* 0x924 */	u32 rsvd_924;
/* 0x928 */	u32 mc_min_length_xusb_0;
/* 0x92c */	u32 mc_min_length_xusb_1;
/* 0x930 */	u32 mc_min_length_isp2b_0;
/* 0x934 */	u32 mc_min_length_isp2b_1;
/* 0x938 */	u32 rsvd_938;
/* 0x93c */	u32 mc_min_length_tsec_0;
/* 0x940 */	u32 mc_min_length_vic_0;
/* 0x944 */	u32 mc_min_length_vi2_0;
/* 0x948 */	u32 mc_untranslated_region_check_b01;
/* 0x94c */	u32 mc_min_length_axiap_0;
/* 0x950 */	u32 mc_min_length_a9avp_0;
/* 0x954 */	u32 rsvd_954;
/* 0x958 */	u32 mc_reserved_rsv_1;
/* 0x95c */	u32 mc_dvfs_pipe_select;
/* 0x960 */	u32 mc_ptsa_grant_decrement;
/* 0x964 */	u32 mc_iram_reg_ctrl;
/* 0x968 */	u32 mc_emem_arb_override_1;
/* 0x96c */	u32 rsvd_96c;
/* 0x970 */	u32 mc_client_hotreset_ctrl_1;
/* 0x974 */	u32 mc_client_hotreset_status_1;
/* 0x978 */	u32 mc_video_protect_bom_adr_hi;
/* 0x97c */	u32 rsvd_97c;
/* 0x980 */	u32 mc_iram_adr_hi;
/* 0x984 */	u32 mc_video_protect_gpu_override_0;
/* 0x988 */	u32 mc_video_protect_gpu_override_1;
/* 0x98c */	u32 rsvd_98c;
/* 0x990 */	u32 mc_emem_arb_stats_0;
/* 0x994 */	u32 mc_emem_arb_stats_1;
/* 0x998 */	u32 rsvd_998[2];
/* 0x9a0 */	u32 mc_mts_carveout_bom;
/* 0x9a4 */	u32 mc_mts_carveout_size_mb;
/* 0x9a8 */	u32 mc_mts_carveout_adr_hi;
/* 0x9ac */	u32 mc_mts_carveout_reg_ctrl;
/* 0x9b0 */	u32 mc_err_mts_status;
/* 0x9b4 */	u32 mc_err_mts_adr;
/* 0x9b8 */	u32 mc_smmu_ptc_flush_1;
/* 0x9bc */	u32 mc_security_cfg3;
/* 0x9c0 */	u32 rsvd_9c0[4];
/* 0x9d0 */	u32 mc_err_apb_asid_update_status;
/* 0x9d4 */	u32 mc_sec_carveout_adr_hi;
/* 0x9d8 */	u32 rsvd_9d8;
/* 0x9dc */	u32 mc_da_config0;
/* 0x9e0 */	u32 mc_smmu_asid_security_2;
/* 0x9e4 */	u32 mc_smmu_asid_security_3;
/* 0x9e8 */	u32 mc_smmu_asid_security_4;
/* 0x9ec */	u32 mc_smmu_asid_security_5;
/* 0x9f0 */	u32 mc_smmu_asid_security_6;
/* 0x9f4 */	u32 mc_smmu_asid_security_7;
/* 0x9f8 */	u32 rsvd_9f8[2];
/* 0xa00 */	u32 mc_gk_extra_snap_levels;
/* 0xa04 */	u32 mc_sd_extra_snap_levels;
/* 0xa08 */	u32 mc_isp_extra_snap_levels;
/* 0xa0c */	u32 rsvd_a0c;
/* 0xa10 */	u32 mc_aud_extra_snap_levels;
/* 0xa14 */	u32 mc_host_extra_snap_levels;
/* 0xa18 */	u32 mc_usbd_extra_snap_levels;
/* 0xa1c */	u32 mc_vicpc_extra_snap_levels;
/* 0xa20 */	u32 mc_stat_emc_filter_set0_adr_limit_upper;
/* 0xa24 */	u32 mc_stat_emc_filter_set1_adr_limit_upper;
/* 0xa28 */	u32 mc_stat_emc_filter_set0_virtual_adr_limit_upper;
/* 0xa2c */	u32 mc_stat_emc_filter_set1_virtual_adr_limit_upper;
/* 0xa30 */	u32 rsvd_a30[3];
/* 0xa3c */	u32 mc_jpg_extra_snap_levels;
/* 0xa40 */	u32 mc_gk2_extra_snap_levels;
/* 0xa44 */	u32 mc_sdm_extra_snap_levels;
/* 0xa48 */	u32 mc_hdapc_extra_snap_levels;
/* 0xa4c */	u32 mc_dfd_extra_snap_levels;
/* 0xa50 */	u32 rsvd_a50[14];
/* 0xa88 */	u32 mc_smmu_dc1_asid;
/* 0xa8c */	u32 rsvd_a8c[2];
/* 0xa94 */	u32 mc_smmu_sdmmc1a_asid;
/* 0xa98 */	u32 mc_smmu_sdmmc2a_asid;
/* 0xa9c */	u32 mc_smmu_sdmmc3a_asid;
/* 0xaa0 */	u32 mc_smmu_sdmmc4a_asid;
/* 0xaa4 */	u32 mc_smmu_isp2b_asid;
/* 0xaa8 */	u32 mc_smmu_gpu_asid;
/* 0xaac */	u32 mc_smmu_gpub_asid;
/* 0xab0 */	u32 mc_smmu_ppcs2_asid;
/* 0xab4 */	u32 mc_smmu_nvdec_asid;
/* 0xab8 */	u32 mc_smmu_ape_asid;
/* 0xabc */	u32 mc_smmu_se_asid;
/* 0xac0 */	u32 mc_smmu_nvjpg_asid;
/* 0xac4 */	u32 mc_smmu_hc1_asid;
/* 0xac8 */	u32 mc_smmu_se1_asid;
/* 0xacc */	u32 mc_smmu_axiap_asid;
/* 0xad0 */	u32 mc_smmu_etr_asid;
/* 0xad4 */	u32 mc_smmu_tsecb_asid;
/* 0xad8 */	u32 mc_smmu_tsec1_asid;
/* 0xadc */	u32 mc_smmu_tsecb1_asid;
/* 0xae0 */	u32 mc_smmu_nvdec1_asid;
/* 0xae4 */	u32 rsvd_ae4[8];
/* 0xb04 */	u32 mc_min_length_gpu_0;
/* 0xb08 */	u32 rsvd_b08[2];
/* 0xb10 */	u32 mc_min_length_sdmmca_0;
/* 0xb14 */	u32 mc_min_length_sdmmcaa_0;
/* 0xb18 */	u32 mc_min_length_sdmmc_0;
/* 0xb1c */	u32 mc_min_length_sdmmcab_0;
/* 0xb20 */	u32 mc_min_length_dc_3;
/* 0xb24 */	u32 rsvd_b24[3];
/* 0xb30 */	u32 mc_min_length_nvdec_0;
/* 0xb34 */	u32 mc_min_length_ape_0;
/* 0xb38 */	u32 mc_min_length_se_0;
/* 0xb3c */	u32 mc_min_length_nvjpg_0;
/* 0xb40 */	u32 mc_min_length_gpu2_0;
/* 0xb44 */	u32 mc_min_length_etr_0;
/* 0xb48 */	u32 mc_min_length_tsecb_0;
/* 0xb4c */	u32 rsvd_b4c[13];
/* 0xb80 */	u32 mc_emem_arb_niso_throttle_mask_1;
/* 0xb84 */	u32 mc_emem_arb_hysteresis_4;
/* 0xb88 */	u32 mc_stat_emc_filter_set0_client_4;
/* 0xb8c */	u32 mc_stat_emc_filter_set1_client_4;
/* 0xb90 */	u32 rsvd_b90;
/* 0xb94 */	u32 mc_emem_arb_isochronous_4;
/* 0xb98 */	u32 mc_smmu_translation_enable_4;
/* 0xb9c */	u32 mc_smmu_client_config4;
/* 0xba0 */	u32 rsvd_ba0[4];
/* 0xbb0 */	u32 mc_emem_arb_dhysteresis_0;
/* 0xbb4 */	u32 mc_emem_arb_dhysteresis_1;
/* 0xbb8 */	u32 mc_emem_arb_dhysteresis_2;
/* 0xbbc */	u32 mc_emem_arb_dhysteresis_3;
/* 0xbc0 */	u32 mc_emem_arb_dhysteresis_4;
/* 0xbc4 */	u32 rsvd_bc4[2];
/* 0xbcc */	u32 mc_emem_arb_dhyst_ctrl;
/* 0xbd0 */	u32 mc_emem_arb_dhyst_timeout_util_0;
/* 0xbd4 */	u32 mc_emem_arb_dhyst_timeout_util_1;
/* 0xbd8 */	u32 mc_emem_arb_dhyst_timeout_util_2;
/* 0xbdc */	u32 mc_emem_arb_dhyst_timeout_util_3;
/* 0xbe0 */	u32 mc_emem_arb_dhyst_timeout_util_4;
/* 0xbe4 */	u32 mc_emem_arb_dhyst_timeout_util_5;
/* 0xbe8 */	u32 mc_emem_arb_dhyst_timeout_util_6;
/* 0xbec */	u32 mc_emem_arb_dhyst_timeout_util_7;
/* 0xbf0 */	u32 rsvd_bf0[4];
/* 0xc00 */	u32 mc_err_generalized_carveout_status;
/* 0xc04 */	u32 mc_err_generalized_carveout_adr;
/* 0xc08 */	u32 mc_security_carveout1_cfg0;
/* 0xc0c */	u32 mc_security_carveout1_bom;
/* 0xc10 */	u32 mc_security_carveout1_bom_hi;
/* 0xc14 */	u32 mc_security_carveout1_size_128kb;
/* 0xc18 */	u32 mc_security_carveout1_client_access0;
/* 0xc1c */	u32 mc_security_carveout1_client_access1;
/* 0xc20 */	u32 mc_security_carveout1_client_access2;
/* 0xc24 */	u32 mc_security_carveout1_client_access3;
/* 0xc28 */	u32 mc_security_carveout1_client_access4;
/* 0xc2c */	u32 mc_security_carveout1_client_force_internal_access0;
/* 0xc30 */	u32 mc_security_carveout1_client_force_internal_access1;
/* 0xc34 */	u32 mc_security_carveout1_client_force_internal_access2;
/* 0xc38 */	u32 mc_security_carveout1_client_force_internal_access3;
/* 0xc3c */	u32 mc_security_carveout1_client_force_internal_access4;
/* 0xc40 */	u32 rsvd_c40[6];
/* 0xc58 */	u32 mc_security_carveout2_cfg0;
/* 0xc5c */	u32 mc_security_carveout2_bom;
/* 0xc60 */	u32 mc_security_carveout2_bom_hi;
/* 0xc64 */	u32 mc_security_carveout2_size_128kb;
/* 0xc68 */	u32 mc_security_carveout2_client_access0;
/* 0xc6c */	u32 mc_security_carveout2_client_access1;
/* 0xc70 */	u32 mc_security_carveout2_client_access2;
/* 0xc74 */	u32 mc_security_carveout2_client_access3;
/* 0xc78 */	u32 mc_security_carveout2_client_access4;
/* 0xc7c */	u32 mc_security_carveout2_client_force_internal_access0;
/* 0xc80 */	u32 mc_security_carveout2_client_force_internal_access1;
/* 0xc84 */	u32 mc_security_carveout2_client_force_internal_access2;
/* 0xc88 */	u32 mc_security_carveout2_client_force_internal_access3;
/* 0xc8c */	u32 mc_security_carveout2_client_force_internal_access4;
/* 0xc90 */	u32 rsvd_c90[6];
/* 0xca8 */	u32 mc_security_carveout3_cfg0;
/* 0xcac */	u32 mc_security_carveout3_bom;
/* 0xcb0 */	u32 mc_security_carveout3_bom_hi;
/* 0xcb4 */	u32 mc_security_carveout3_size_128kb;
/* 0xcb8 */	u32 mc_security_carveout3_client_access0;
/* 0xcbc */	u32 mc_security_carveout3_client_access1;
/* 0xcc0 */	u32 mc_security_carveout3_client_access2;
/* 0xcc4 */	u32 mc_security_carveout3_client_access3;
/* 0xcc8 */	u32 mc_security_carveout3_client_access4;
/* 0xccc */	u32 mc_security_carveout3_client_force_internal_access0;
/* 0xcd0 */	u32 mc_security_carveout3_client_force_internal_access1;
/* 0xcd4 */	u32 mc_security_carveout3_client_force_internal_access2;
/* 0xcd8 */	u32 mc_security_carveout3_client_force_internal_access3;
/* 0xcdc */	u32 mc_security_carveout3_client_force_internal_access4;
/* 0xce0 */	u32 rsvd_ce0[6];
/* 0xcf8 */	u32 mc_security_carveout4_cfg0;
/* 0xcfc */	u32 mc_security_carveout4_bom;
/* 0xd00 */	u32 mc_security_carveout4_bom_hi;
/* 0xd04 */	u32 mc_security_carveout4_size_128kb;
/* 0xd08 */	u32 mc_security_carveout4_client_access0;
/* 0xd0c */	u32 mc_security_carveout4_client_access1;
/* 0xd10 */	u32 mc_security_carveout4_client_access2;
/* 0xd14 */	u32 mc_security_carveout4_client_access3;
/* 0xd18 */	u32 mc_security_carveout4_client_access4;
/* 0xd1c */	u32 mc_security_carveout4_client_force_internal_access0;
/* 0xd20 */	u32 mc_security_carveout4_client_force_internal_access1;
/* 0xd24 */	u32 mc_security_carveout4_client_force_internal_access2;
/* 0xd28 */	u32 mc_security_carveout4_client_force_internal_access3;
/* 0xd2c */	u32 mc_security_carveout4_client_force_internal_access4;
/* 0xd30 */	u32 rsvd_d30[6];
/* 0xd48 */	u32 mc_security_carveout5_cfg0;
/* 0xd4c */	u32 mc_security_carveout5_bom;
/* 0xd50 */	u32 mc_security_carveout5_bom_hi;
/* 0xd54 */	u32 mc_security_carveout5_size_128kb;
/* 0xd58 */	u32 mc_security_carveout5_client_access0;
/* 0xd5c */	u32 mc_security_carveout5_client_access1;
/* 0xd60 */	u32 mc_security_carveout5_client_access2;
/* 0xd64 */	u32 mc_security_carveout5_client_access3;
/* 0xd68 */	u32 mc_security_carveout5_client_access4;
/* 0xd6c */	u32 mc_security_carveout5_client_force_internal_access0;
/* 0xd70 */	u32 mc_security_carveout5_client_force_internal_access1;
/* 0xd74 */	u32 mc_security_carveout5_client_force_internal_access2;
/* 0xd78 */	u32 mc_security_carveout5_client_force_internal_access3;
/* 0xd7c */	u32 mc_security_carveout5_client_force_internal_access4;
/* 0xd80 */	u32 rsvd_d80[20];
/* 0xdd0 */	u32 mc_pcfifo_client_config0;
/* 0xdd4 */	u32 mc_pcfifo_client_config1;
/* 0xdd8 */	u32 mc_pcfifo_client_config2;
/* 0xddc */	u32 mc_pcfifo_client_config3;
/* 0xde0 */	u32 mc_pcfifo_client_config4;
} mc_regs_t210_t;

#endif
