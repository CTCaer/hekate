/*
 * Copyright (c) 2018-2026 CTCaer
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

#ifndef _PMC_T210_H_
#define _PMC_T210_H_

/*! PMC registers. */
#define APBDEV_PMC_CNTRL                       0x0
#define  PMC_CNTRL_RTC_CLK_DIS     BIT(1)
#define  PMC_CNTRL_RTC_RST         BIT(2)
#define  PMC_CNTRL_MAIN_RST        BIT(4)
#define  PMC_CNTRL_LATCHWAKE_EN    BIT(5)
#define  PMC_CNTRL_BLINK_EN        BIT(7)
#define  PMC_CNTRL_PWRREQ_OE       BIT(9)
#define  PMC_CNTRL_SYSCLK_OE       BIT(11)
#define  PMC_CNTRL_PWRGATE_DIS     BIT(12)
#define  PMC_CNTRL_SIDE_EFFECT_LP0 BIT(14)
#define  PMC_CNTRL_CPUPWRREQ_OE    BIT(16)
#define  PMC_CNTRL_FUSE_OVERRIDE   BIT(18)
#define  PMC_CNTRL_SHUTDOWN_OE     BIT(22)

#define APBDEV_PMC_SEC_DISABLE                 0x4
#define APBDEV_PMC_PMC_SWRST                   0x8
#define APBDEV_PMC_WAKE_MASK                   0xC
#define APBDEV_PMC_WAKE_LVL                    0x10
#define APBDEV_PMC_WAKE_STATUS                 0x14
#define APBDEV_PMC_SW_WAKE_STATUS              0x18
#define APBDEV_PMC_DPD_PADS_ORIDE              0x1C
#define APBDEV_PMC_DPD_SAMPLE                  0x20
#define APBDEV_PMC_DPD_ENABLE                  0x24
#define APBDEV_PMC_PWRGATE_TIMER_OFF           0x28
#define APBDEV_PMC_CLAMP_STATUS                0x2C

#define APBDEV_PMC_PWRGATE_TOGGLE              0x30
#define  PMC_PWRGATE_TOGGLE_START BIT(8)

#define APBDEV_PMC_REMOVE_CLAMPING_CMD         0x34
#define APBDEV_PMC_PWRGATE_STATUS              0x38
#define APBDEV_PMC_PWRGOOD_TIMER               0x3C
#define APBDEV_PMC_BLINK_TIMER                 0x40
#define  PMC_BLINK_ON(n)  ((n & 0x7FFF))
#define  PMC_BLINK_FORCE  BIT(15)
#define  PMC_BLINK_OFF(n) ((u32)(n & 0xFFFF) << 16)

#define APBDEV_PMC_NO_IOPOWER                  0x44
#define  PMC_NO_IOPOWER_MEM      BIT(7)
#define  PMC_NO_IOPOWER_SDMMC1   BIT(12)
#define  PMC_NO_IOPOWER_SDMMC4   BIT(14)
#define  PMC_NO_IOPOWER_MEM_COMP BIT(16)
#define  PMC_NO_IOPOWER_AUDIO_HV BIT(18)
#define  PMC_NO_IOPOWER_GPIO     BIT(21)

#define APBDEV_PMC_PWR_DET                     0x48
#define APBDEV_PMC_PWR_DET_LATCH               0x4C

#define APBDEV_PMC_SCRATCH0                    0x50
#define  PMC_SCRATCH0_MODE_WARMBOOT   BIT(0)
#define  PMC_SCRATCH0_MODE_RCM        BIT(1)
#define  PMC_SCRATCH0_MODE_PAYLOAD    BIT(29)
#define  PMC_SCRATCH0_MODE_BOOTLOADER BIT(30)
#define  PMC_SCRATCH0_MODE_RECOVERY   BIT(31)
#define  PMC_SCRATCH0_MODE_CUSTOM_ALL (PMC_SCRATCH0_MODE_RECOVERY   | \
									   PMC_SCRATCH0_MODE_BOOTLOADER | \
									   PMC_SCRATCH0_MODE_PAYLOAD)

#define APBDEV_PMC_SCRATCH1                    0x54
#define APBDEV_PMC_SCRATCH2                    0x58
#define APBDEV_PMC_SCRATCH3                    0x5C
#define APBDEV_PMC_SCRATCH4                    0x60
#define APBDEV_PMC_SCRATCH5                    0x64
#define APBDEV_PMC_SCRATCH6                    0x68
#define APBDEV_PMC_SCRATCH7                    0x6C
#define APBDEV_PMC_SCRATCH8                    0x70
#define APBDEV_PMC_SCRATCH9                    0x74
#define APBDEV_PMC_SCRATCH10                   0x78
#define APBDEV_PMC_SCRATCH11                   0x7C
#define APBDEV_PMC_SCRATCH12                   0x80
#define APBDEV_PMC_SCRATCH13                   0x84
#define APBDEV_PMC_SCRATCH14                   0x88
#define APBDEV_PMC_SCRATCH15                   0x8C
#define APBDEV_PMC_SCRATCH16                   0x90
#define APBDEV_PMC_SCRATCH17                   0x94
#define APBDEV_PMC_SCRATCH18                   0x98
#define APBDEV_PMC_SCRATCH19                   0x9C
#define APBDEV_PMC_SCRATCH20                   0xA0 // ODM data/config scratch.
#define APBDEV_PMC_SCRATCH21                   0xA4
#define APBDEV_PMC_SCRATCH22                   0xA8
#define APBDEV_PMC_SCRATCH23                   0xAC
#define APBDEV_PMC_SECURE_SCRATCH0             0xB0
#define APBDEV_PMC_SECURE_SCRATCH1             0xB4
#define APBDEV_PMC_SECURE_SCRATCH2             0xB8
#define APBDEV_PMC_SECURE_SCRATCH3             0xBC
#define APBDEV_PMC_SECURE_SCRATCH4             0xC0
#define APBDEV_PMC_SECURE_SCRATCH5             0xC4
#define APBDEV_PMC_CPUPWRGOOD_TIMER            0xC8
#define APBDEV_PMC_CPUPWROFF_TIMER             0xCC
#define APBDEV_PMC_PG_MASK                     0xD0
#define APBDEV_PMC_PG_MASK_1                   0xD4
#define APBDEV_PMC_AUTO_WAKE_LVL               0xD8
#define APBDEV_PMC_AUTO_WAKE_LVL_MASK          0xDC
#define APBDEV_PMC_WAKE_DELAY                  0xE0

#define APBDEV_PMC_PWR_DET_VAL                 0xE4
#define  PMC_PWR_DET_33V_SDMMC1   BIT(12)
#define  PMC_PWR_DET_33V_AUDIO_HV BIT(18)
#define  PMC_PWR_DET_33V_GPIO     BIT(21)

#define APBDEV_PMC_DDR_PWR                     0xE8
#define APBDEV_PMC_USB_DEBOUNCE_DEL            0xEC
#define APBDEV_PMC_USB_AO                      0xF0

#define APBDEV_PMC_CRYPTO_OP                   0xF4
#define  PMC_CRYPTO_OP_SE_ENABLE  0
#define  PMC_CRYPTO_OP_SE_DISABLE 1

#define APBDEV_PMC_PLLP_WB0_OVERRIDE           0xF8
#define  PMC_PLLP_WB0_OVR_PLLM_OVR_ENABLE BIT(11)
#define  PMC_PLLP_WB0_OVR_PLLM_ENABLE     BIT(12)

#define APBDEV_PMC_SCRATCH24                   0xFC
#define APBDEV_PMC_SCRATCH25                   0x100
#define APBDEV_PMC_SCRATCH26                   0x104
#define APBDEV_PMC_SCRATCH27                   0x108
#define APBDEV_PMC_SCRATCH28                   0x10C
#define APBDEV_PMC_SCRATCH29                   0x110
#define APBDEV_PMC_SCRATCH30                   0x114
#define APBDEV_PMC_SCRATCH31                   0x118
#define APBDEV_PMC_SCRATCH32                   0x11C
#define APBDEV_PMC_SCRATCH33                   0x120
#define APBDEV_PMC_SCRATCH34                   0x124
#define APBDEV_PMC_SCRATCH35                   0x128
#define APBDEV_PMC_SCRATCH36                   0x12C
#define APBDEV_PMC_SCRATCH37                   0x130
#define  PMC_SCRATCH37_KERNEL_PANIC_MAGIC 0x4E415054 // "TPAN"

#define APBDEV_PMC_SCRATCH38                   0x134
#define APBDEV_PMC_SCRATCH39                   0x138
#define APBDEV_PMC_SCRATCH40                   0x13C
#define APBDEV_PMC_SCRATCH41                   0x140
#define APBDEV_PMC_SCRATCH42                   0x144
#define APBDEV_PMC_BONDOUT_MIRROR0             0x148
#define APBDEV_PMC_BONDOUT_MIRROR1             0x14C
#define APBDEV_PMC_BONDOUT_MIRROR2             0x150
#define APBDEV_PMC_SYS_33V_EN                  0x154
#define APBDEV_PMC_BONDOUT_MIRROR_ACCESS       0x158
#define APBDEV_PMC_GATE                        0x15C
#define APBDEV_PMC_WAKE2_MASK                  0x160
#define APBDEV_PMC_WAKE2_LVL                   0x164
#define APBDEV_PMC_WAKE2_STATUS                0x168
#define APBDEV_PMC_SW_WAKE2_STATUS             0x16C
#define APBDEV_PMC_AUTO_WAKE2_LVL_MASK         0x170
#define APBDEV_PMC_PG_MASK_2                   0x174
#define APBDEV_PMC_PG_MASK_CE1                 0x178
#define APBDEV_PMC_PG_MASK_CE2                 0x17C
#define APBDEV_PMC_PG_MASK_CE3                 0x180
#define APBDEV_PMC_PWRGATE_TIMER_CE_0          0x184
#define APBDEV_PMC_PWRGATE_TIMER_CE_1          0x188
#define APBDEV_PMC_PWRGATE_TIMER_CE_2          0x18C
#define APBDEV_PMC_PWRGATE_TIMER_CE_3          0x190
#define APBDEV_PMC_PWRGATE_TIMER_CE_4          0x194
#define APBDEV_PMC_PWRGATE_TIMER_CE_5          0x198
#define APBDEV_PMC_PWRGATE_TIMER_CE_6          0x19C
#define APBDEV_PMC_PCX_EDPD_CNTRL              0x1A0

#define APBDEV_PMC_OSC_EDPD_OVER               0x1A4
#define  PMC_OSC_EDPD_OVER_OSC_CTRL_OVER BIT(22)

#define APBDEV_PMC_CLK_OUT_CNTRL               0x1A8
#define  PMC_CLK_OUT_CNTRL_CLK1_FORCE_EN     BIT(2)
#define  PMC_CLK_OUT_CNTRL_CLK2_FORCE_EN     BIT(10)
#define  PMC_CLK_OUT_CNTRL_CLK3_FORCE_EN     BIT(18)
#define  PMC_CLK_OUT_CNTRL_CLK1_SRC_SEL(src) (((src) & 3) << 6)
#define  PMC_CLK_OUT_CNTRL_CLK2_SRC_SEL(src) (((src) & 3) << 14)
#define  PMC_CLK_OUT_CNTRL_CLK3_SRC_SEL(src) (((src) & 3) << 22)
#define   OSC_DIV1                           0
#define   OSC_DIV2                           1
#define   OSC_DIV4                           2
#define   OSC_CAR                            3

#define APBDEV_PMC_SATA_PWRGT                  0x1AC
#define APBDEV_PMC_SENSOR_CTRL                 0x1B0

#define APBDEV_PMC_RST_STATUS                  0x1B4
#define  PMC_RST_STATUS_MASK     7
#define  PMC_RST_STATUS_POR      0
#define  PMC_RST_STATUS_WATCHDOG 1
#define  PMC_RST_STATUS_SENSOR   2
#define  PMC_RST_STATUS_SW_MAIN  3
#define  PMC_RST_STATUS_LP0      4
#define  PMC_RST_STATUS_AOTAG    5

#define APBDEV_PMC_IO_DPD_REQ                  0x1B8
#define  PMC_IO_DPD_REQ_DPD_IDLE (0 << 30u)
#define  PMC_IO_DPD_REQ_DPD_OFF  (1 << 30u)
#define  PMC_IO_DPD_REQ_DPD_ON   (2 << 30u)

#define APBDEV_PMC_IO_DPD_STATUS               0x1BC
#define APBDEV_PMC_IO_DPD2_REQ                 0x1C0
#define APBDEV_PMC_IO_DPD2_STATUS              0x1C4
#define APBDEV_PMC_SEL_DPD_TIM                 0x1C8
#define APBDEV_PMC_VDDP_SEL                    0x1CC
#define APBDEV_PMC_DDR_CFG                     0x1D0
#define APBDEV_PMC_E_NO_VTTGEN                 0x1D4
#define APBDEV_PMC_PLLM_WB0_OVERRIDE_FREQ      0x1DC
#define APBDEV_PMC_TEST_PWRGATE                0x1E0
#define APBDEV_PMC_PWRGATE_TIMER_MULT          0x1E4
#define APBDEV_PMC_DSI_SEL_DPD                 0x1E8
#define APBDEV_PMC_UTMIP_UHSIC_TRIGGERS        0x1EC
#define APBDEV_PMC_UTMIP_UHSIC_SAVED_STATE     0x1F0
#define APBDEV_PMC_UTMIP_TERM_PAD_CFG          0x1F8
#define APBDEV_PMC_UTMIP_UHSIC_SLEEP_CFG       0x1FC
#define APBDEV_PMC_UTMIP_UHSIC_SLEEPWALK_CFG   0x200
#define APBDEV_PMC_UTMIP_SLEEPWALK_P0          0x204
#define APBDEV_PMC_UTMIP_SLEEPWALK_P1          0x208
#define APBDEV_PMC_UTMIP_SLEEPWALK_P2          0x20C
#define APBDEV_PMC_UHSIC_SLEEPWALK_P0          0x210
#define APBDEV_PMC_UTMIP_UHSIC_STATUS          0x214
#define APBDEV_PMC_UTMIP_UHSIC_FAKE            0x218
#define APBDEV_PMC_BONDOUT_MIRROR3             0x21C
#define APBDEV_PMC_BONDOUT_MIRROR4             0x220
#define APBDEV_PMC_SECURE_SCRATCH6             0x224
#define APBDEV_PMC_SECURE_SCRATCH7             0x228
#define APBDEV_PMC_SCRATCH43                   0x22C
#define APBDEV_PMC_SCRATCH44                   0x230
#define APBDEV_PMC_SCRATCH45                   0x234
#define APBDEV_PMC_SCRATCH46                   0x238
#define APBDEV_PMC_SCRATCH47                   0x23C
#define APBDEV_PMC_SCRATCH48                   0x240
#define APBDEV_PMC_SCRATCH49                   0x244
#define APBDEV_PMC_SCRATCH50                   0x248
#define APBDEV_PMC_SCRATCH51                   0x24C
#define APBDEV_PMC_SCRATCH52                   0x250
#define APBDEV_PMC_SCRATCH53                   0x254
#define APBDEV_PMC_SCRATCH54                   0x258
#define APBDEV_PMC_SCRATCH55                   0x25C
#define APBDEV_PMC_SCRATCH0_ECO                0x260
#define APBDEV_PMC_POR_DPD_CTRL                0x264
#define APBDEV_PMC_SCRATCH2_ECO                0x268
#define APBDEV_PMC_UTMIP_UHSIC_LINE_WAKEUP     0x26C
#define APBDEV_PMC_UTMIP_BIAS_MASTER_CNTRL     0x270
#define APBDEV_PMC_UTMIP_MASTER_CONFIG         0x274
#define APBDEV_PMC_TD_PWRGATE_INTER_PART_TIMER 0x278
#define APBDEV_PMC_UTMIP_UHSIC2_TRIGGERS       0x27C
#define APBDEV_PMC_UTMIP_UHSIC2_SAVED_STATE    0x280
#define APBDEV_PMC_UTMIP_UHSIC2_SLEEP_CFG      0x284
#define APBDEV_PMC_UTMIP_UHSIC2_SLEEPWALK_CFG  0x288
#define APBDEV_PMC_UHSIC2_SLEEPWALK_P1         0x28C
#define APBDEV_PMC_UTMIP_UHSIC2_STATUS         0x290
#define APBDEV_PMC_UTMIP_UHSIC2_FAKE           0x294
#define APBDEV_PMC_UTMIP_UHSIC2_LINE_WAKEUP    0x298
#define APBDEV_PMC_UTMIP_MASTER2_CONFIG        0x29C
#define APBDEV_PMC_UTMIP_UHSIC_RPD_CFG         0x2A0
#define APBDEV_PMC_PG_MASK_CE0                 0x2A4
#define APBDEV_PMC_PG_MASK_3                   0x2A8
#define APBDEV_PMC_PG_MASK_4                   0x2AC
#define APBDEV_PMC_PLLM_WB0_OVERRIDE2          0x2B0
#define APBDEV_PMC_TSC_MULT                    0x2B4
#define APBDEV_PMC_CPU_VSENSE_OVERRIDE         0x2B8
#define APBDEV_PMC_GLB_AMAP_CFG                0x2BC

#define APBDEV_PMC_STICKY_BITS                 0x2C0
#define  PMC_STICKY_BITS_HDA_LPBK_DIS BIT(0)

#define APBDEV_PMC_SEC_DISABLE2                0x2C4
#define APBDEV_PMC_WEAK_BIAS                   0x2C8
#define APBDEV_PMC_REG_SHORT                   0x2CC
#define APBDEV_PMC_PG_MASK_ANDOR               0x2D0
#define APBDEV_PMC_GPU_RG_CNTRL                0x2D4
#define APBDEV_PMC_SEC_DISABLE3                0x2D8
#define APBDEV_PMC_PG_MASK_5                   0x2DC
#define APBDEV_PMC_PG_MASK_6                   0x2E0
#define APBDEV_PMC_SECURE_SCRATCH8             0x300
#define APBDEV_PMC_SECURE_SCRATCH9             0x304
#define APBDEV_PMC_SECURE_SCRATCH10            0x308
#define APBDEV_PMC_SECURE_SCRATCH11            0x30C
#define APBDEV_PMC_SECURE_SCRATCH12            0x310
#define APBDEV_PMC_SECURE_SCRATCH13            0x314
#define APBDEV_PMC_SECURE_SCRATCH14            0x318
#define APBDEV_PMC_SECURE_SCRATCH15            0x31C
#define APBDEV_PMC_SECURE_SCRATCH16            0x320
#define APBDEV_PMC_SECURE_SCRATCH17            0x324
#define APBDEV_PMC_SECURE_SCRATCH18            0x328
#define APBDEV_PMC_SECURE_SCRATCH19            0x32C
#define APBDEV_PMC_SECURE_SCRATCH20            0x330

#define APBDEV_PMC_SECURE_SCRATCH21            0x334
#define  PMC_FUSE_PRIVATEKEYDISABLE_TZ_STICKY_BIT BIT(4)

#define APBDEV_PMC_SECURE_SCRATCH22            0x338 // AArch32 reset address.
#define APBDEV_PMC_SECURE_SCRATCH23            0x33C
#define APBDEV_PMC_SECURE_SCRATCH24            0x340
#define APBDEV_PMC_SECURE_SCRATCH25            0x344
#define APBDEV_PMC_SECURE_SCRATCH26            0x348
#define APBDEV_PMC_SECURE_SCRATCH27            0x34C
#define APBDEV_PMC_SECURE_SCRATCH28            0x350
#define APBDEV_PMC_SECURE_SCRATCH29            0x354
#define APBDEV_PMC_SECURE_SCRATCH30            0x358
#define APBDEV_PMC_SECURE_SCRATCH31            0x35C
#define APBDEV_PMC_SECURE_SCRATCH32            0x360
#define APBDEV_PMC_SECURE_SCRATCH33            0x364
#define APBDEV_PMC_SECURE_SCRATCH34            0x368 // AArch64 reset address.
#define APBDEV_PMC_SECURE_SCRATCH35            0x36C // AArch64 reset hi-address.
#define APBDEV_PMC_SECURE_SCRATCH36            0x370
#define APBDEV_PMC_SECURE_SCRATCH37            0x374
#define APBDEV_PMC_SECURE_SCRATCH38            0x378
#define APBDEV_PMC_SECURE_SCRATCH39            0x37C
#define APBDEV_PMC_SECURE_SCRATCH40            0x380
#define APBDEV_PMC_SECURE_SCRATCH41            0x384
#define APBDEV_PMC_SECURE_SCRATCH42            0x388
#define APBDEV_PMC_SECURE_SCRATCH43            0x38C
#define APBDEV_PMC_SECURE_SCRATCH44            0x390
#define APBDEV_PMC_SECURE_SCRATCH45            0x394
#define APBDEV_PMC_SECURE_SCRATCH46            0x398
#define APBDEV_PMC_SECURE_SCRATCH47            0x39C
#define APBDEV_PMC_SECURE_SCRATCH48            0x3A0
#define APBDEV_PMC_SECURE_SCRATCH49            0x3A4
#define APBDEV_PMC_SECURE_SCRATCH50            0x3A8
#define APBDEV_PMC_SECURE_SCRATCH51            0x3AC
#define APBDEV_PMC_SECURE_SCRATCH52            0x3B0
#define APBDEV_PMC_SECURE_SCRATCH53            0x3B4
#define APBDEV_PMC_SECURE_SCRATCH54            0x3B8
#define APBDEV_PMC_SECURE_SCRATCH55            0x3BC
#define APBDEV_PMC_SECURE_SCRATCH56            0x3C0
#define APBDEV_PMC_SECURE_SCRATCH57            0x3C4
#define APBDEV_PMC_SECURE_SCRATCH58            0x3C8
#define APBDEV_PMC_SECURE_SCRATCH59            0x3CC
#define APBDEV_PMC_SECURE_SCRATCH60            0x3D0
#define APBDEV_PMC_SECURE_SCRATCH61            0x3D4
#define APBDEV_PMC_SECURE_SCRATCH62            0x3D8
#define APBDEV_PMC_SECURE_SCRATCH63            0x3DC
#define APBDEV_PMC_SECURE_SCRATCH64            0x3E0
#define APBDEV_PMC_SECURE_SCRATCH65            0x3E4
#define APBDEV_PMC_SECURE_SCRATCH66            0x3E8
#define APBDEV_PMC_SECURE_SCRATCH67            0x3EC
#define APBDEV_PMC_SECURE_SCRATCH68            0x3F0
#define APBDEV_PMC_SECURE_SCRATCH69            0x3F4
#define APBDEV_PMC_SECURE_SCRATCH70            0x3F8
#define APBDEV_PMC_SECURE_SCRATCH71            0x3FC
#define APBDEV_PMC_SECURE_SCRATCH72            0x400
#define APBDEV_PMC_SECURE_SCRATCH73            0x404
#define APBDEV_PMC_SECURE_SCRATCH74            0x408
#define APBDEV_PMC_SECURE_SCRATCH75            0x40C
#define APBDEV_PMC_SECURE_SCRATCH76            0x410
#define APBDEV_PMC_SECURE_SCRATCH77            0x414
#define APBDEV_PMC_SECURE_SCRATCH78            0x418
#define APBDEV_PMC_SECURE_SCRATCH79            0x41C

#define APBDEV_PMC_CNTRL2                      0x440
#define  PMC_CNTRL2_WAKE_INT_EN      BIT(0)
#define  PMC_CNTRL2_WAKE_DET_EN      BIT(9)
#define  PMC_CNTRL2_SYSCLK_ORRIDE    BIT(10)
#define  PMC_CNTRL2_HOLD_CKE_LOW_EN  BIT(12)
#define  PMC_CNTRL2_ALLOW_PULSE_WAKE BIT(14)

#define APBDEV_PMC_IO_DPD_OFF_MASK             0x444
#define APBDEV_PMC_IO_DPD2_OFF_MASK            0x448
#define APBDEV_PMC_EVENT_COUNTER               0x44C

#define APBDEV_PMC_FUSE_CONTROL                0x450
#define  PMC_FUSE_CONTROL_PS18_LATCH_SET BIT(8)
#define  PMC_FUSE_CONTROL_PS18_LATCH_CLR BIT(9)

#define APBDEV_PMC_SCRATCH1_ECO                0x454
#define APBDEV_PMC_IO_DPD3_REQ                 0x45C
#define APBDEV_PMC_IO_DPD3_STATUS              0x460
#define APBDEV_PMC_IO_DPD4_REQ                 0x464
#define APBDEV_PMC_IO_DPD4_STATUS              0x468
#define APBDEV_PMC_DIRECT_THERMTRIP_CFG        0x474
#define APBDEV_PMC_TSOSC_DELAY                 0x478
#define APBDEV_PMC_SET_SW_CLAMP                0x47C
#define APBDEV_PMC_DEBUG_AUTHENTICATION        0x480
#define APBDEV_PMC_AOTAG_CFG                   0x484
#define APBDEV_PMC_AOTAG_THRESH1_CFG           0x488
#define APBDEV_PMC_AOTAG_THRESH2_CFG           0x48C
#define APBDEV_PMC_AOTAG_THRESH3_CFG           0x490
#define APBDEV_PMC_AOTAG_STATUS                0x494
#define APBDEV_PMC_AOTAG_SECURITY              0x498
#define APBDEV_PMC_TSENSOR_CONFIG0             0x49C
#define APBDEV_PMC_TSENSOR_CONFIG1             0x4A0
#define APBDEV_PMC_TSENSOR_CONFIG2             0x4A4
#define APBDEV_PMC_TSENSOR_STATUS0             0x4A8
#define APBDEV_PMC_TSENSOR_STATUS1             0x4AC
#define APBDEV_PMC_TSENSOR_STATUS2             0x4B0
#define APBDEV_PMC_TSENSOR_PDIV                0x4B4
#define APBDEV_PMC_AOTAG_INTR_EN               0x4B8
#define APBDEV_PMC_AOTAG_INTR_DIS              0x4BC
#define APBDEV_PMC_UTMIP_PAD_CFG0              0x4C0
#define APBDEV_PMC_UTMIP_PAD_CFG1              0x4C4
#define APBDEV_PMC_UTMIP_PAD_CFG2              0x4C8
#define APBDEV_PMC_UTMIP_PAD_CFG3              0x4CC
#define APBDEV_PMC_UTMIP_UHSIC_SLEEP_CFG1      0x4D0
#define APBDEV_PMC_CC4_HVC_CONTROL             0x4D4
#define APBDEV_PMC_WAKE_DEBOUNCE_EN            0x4D8
#define APBDEV_PMC_RAMDUMP_CTL_STATUS          0x4DC
#define APBDEV_PMC_UTMIP_SLEEPWALK_P3          0x4E0
#define APBDEV_PMC_DDR_CNTRL                   0x4E4
#define APBDEV_PMC_SEC_DISABLE4                0x5B0
#define APBDEV_PMC_SEC_DISABLE5                0x5B4
#define APBDEV_PMC_SEC_DISABLE6                0x5B8
#define APBDEV_PMC_SEC_DISABLE7                0x5BC
#define APBDEV_PMC_SEC_DISABLE8                0x5C0
#define APBDEV_PMC_SCRATCH56                   0x600
#define APBDEV_PMC_SCRATCH57                   0x604
#define APBDEV_PMC_SCRATCH58                   0x608
#define APBDEV_PMC_SCRATCH59                   0x60C
#define APBDEV_PMC_SCRATCH60                   0x610
#define APBDEV_PMC_SCRATCH61                   0x614
#define APBDEV_PMC_SCRATCH62                   0x618
#define APBDEV_PMC_SCRATCH63                   0x61C
#define APBDEV_PMC_SCRATCH64                   0x620
#define APBDEV_PMC_SCRATCH65                   0x624
#define APBDEV_PMC_SCRATCH66                   0x628
#define APBDEV_PMC_SCRATCH67                   0x62C
#define APBDEV_PMC_SCRATCH68                   0x630
#define APBDEV_PMC_SCRATCH69                   0x634
#define APBDEV_PMC_SCRATCH70                   0x638
#define APBDEV_PMC_SCRATCH71                   0x63C
#define APBDEV_PMC_SCRATCH72                   0x640
#define APBDEV_PMC_SCRATCH73                   0x644
#define APBDEV_PMC_SCRATCH74                   0x648
#define APBDEV_PMC_SCRATCH75                   0x64C
#define APBDEV_PMC_SCRATCH76                   0x650
#define APBDEV_PMC_SCRATCH77                   0x654
#define APBDEV_PMC_SCRATCH78                   0x658
#define APBDEV_PMC_SCRATCH79                   0x65C
#define APBDEV_PMC_SCRATCH80                   0x660
#define APBDEV_PMC_SCRATCH81                   0x664
#define APBDEV_PMC_SCRATCH82                   0x668
#define APBDEV_PMC_SCRATCH83                   0x66C
#define APBDEV_PMC_SCRATCH84                   0x670
#define APBDEV_PMC_SCRATCH85                   0x674
#define APBDEV_PMC_SCRATCH86                   0x678
#define APBDEV_PMC_SCRATCH87                   0x67C
#define APBDEV_PMC_SCRATCH88                   0x680
#define APBDEV_PMC_SCRATCH89                   0x684
#define APBDEV_PMC_SCRATCH90                   0x688
#define APBDEV_PMC_SCRATCH91                   0x68C
#define APBDEV_PMC_SCRATCH92                   0x690
#define APBDEV_PMC_SCRATCH93                   0x694
#define APBDEV_PMC_SCRATCH94                   0x698
#define APBDEV_PMC_SCRATCH95                   0x69C
#define APBDEV_PMC_SCRATCH96                   0x6A0
#define APBDEV_PMC_SCRATCH97                   0x6A4
#define APBDEV_PMC_SCRATCH98                   0x6A8
#define APBDEV_PMC_SCRATCH99                   0x6AC
#define APBDEV_PMC_SCRATCH100                  0x6B0
#define APBDEV_PMC_SCRATCH101                  0x6B4
#define APBDEV_PMC_SCRATCH102                  0x6B8
#define APBDEV_PMC_SCRATCH103                  0x6BC
#define APBDEV_PMC_SCRATCH104                  0x6C0
#define APBDEV_PMC_SCRATCH105                  0x6C4
#define APBDEV_PMC_SCRATCH106                  0x6C8
#define APBDEV_PMC_SCRATCH107                  0x6CC
#define APBDEV_PMC_SCRATCH108                  0x6D0
#define APBDEV_PMC_SCRATCH109                  0x6D4
#define APBDEV_PMC_SCRATCH110                  0x6D8
#define APBDEV_PMC_SCRATCH111                  0x6DC
#define APBDEV_PMC_SCRATCH112                  0x6E0
#define APBDEV_PMC_SCRATCH113                  0x6E4
#define APBDEV_PMC_SCRATCH114                  0x6E8
#define APBDEV_PMC_SCRATCH115                  0x6EC
#define APBDEV_PMC_SCRATCH116                  0x6F0
#define APBDEV_PMC_SCRATCH117                  0x6F4
#define APBDEV_PMC_SCRATCH118                  0x6F8
#define APBDEV_PMC_SCRATCH119                  0x6FC
#define APBDEV_PMC_SCRATCH120                  0x700
#define APBDEV_PMC_SCRATCH121                  0x704
#define APBDEV_PMC_SCRATCH122                  0x708
#define APBDEV_PMC_SCRATCH123                  0x70C
#define APBDEV_PMC_SCRATCH124                  0x710
#define APBDEV_PMC_SCRATCH125                  0x714
#define APBDEV_PMC_SCRATCH126                  0x718
#define APBDEV_PMC_SCRATCH127                  0x71C
#define APBDEV_PMC_SCRATCH128                  0x720
#define APBDEV_PMC_SCRATCH129                  0x724
#define APBDEV_PMC_SCRATCH130                  0x728
#define APBDEV_PMC_SCRATCH131                  0x72C
#define APBDEV_PMC_SCRATCH132                  0x730
#define APBDEV_PMC_SCRATCH133                  0x734
#define APBDEV_PMC_SCRATCH134                  0x738
#define APBDEV_PMC_SCRATCH135                  0x73C
#define APBDEV_PMC_SCRATCH136                  0x740
#define APBDEV_PMC_SCRATCH137                  0x744
#define APBDEV_PMC_SCRATCH138                  0x748
#define APBDEV_PMC_SCRATCH139                  0x74C
#define APBDEV_PMC_SCRATCH140                  0x750
#define APBDEV_PMC_SCRATCH141                  0x754
#define APBDEV_PMC_SCRATCH142                  0x758
#define APBDEV_PMC_SCRATCH143                  0x75C
#define APBDEV_PMC_SCRATCH144                  0x760
#define APBDEV_PMC_SCRATCH145                  0x764
#define APBDEV_PMC_SCRATCH146                  0x768
#define APBDEV_PMC_SCRATCH147                  0x76C
#define APBDEV_PMC_SCRATCH148                  0x770
#define APBDEV_PMC_SCRATCH149                  0x774
#define APBDEV_PMC_SCRATCH150                  0x778
#define APBDEV_PMC_SCRATCH151                  0x77C
#define APBDEV_PMC_SCRATCH152                  0x780
#define APBDEV_PMC_SCRATCH153                  0x784
#define APBDEV_PMC_SCRATCH154                  0x788
#define APBDEV_PMC_SCRATCH155                  0x78C
#define APBDEV_PMC_SCRATCH156                  0x790
#define APBDEV_PMC_SCRATCH157                  0x794
#define APBDEV_PMC_SCRATCH158                  0x798
#define APBDEV_PMC_SCRATCH159                  0x79C
#define APBDEV_PMC_SCRATCH160                  0x7A0
#define APBDEV_PMC_SCRATCH161                  0x7A4
#define APBDEV_PMC_SCRATCH162                  0x7A8
#define APBDEV_PMC_SCRATCH163                  0x7AC
#define APBDEV_PMC_SCRATCH164                  0x7B0
#define APBDEV_PMC_SCRATCH165                  0x7B4
#define APBDEV_PMC_SCRATCH166                  0x7B8
#define APBDEV_PMC_SCRATCH167                  0x7BC
#define APBDEV_PMC_SCRATCH168                  0x7C0
#define APBDEV_PMC_SCRATCH169                  0x7C4
#define APBDEV_PMC_SCRATCH170                  0x7C8
#define APBDEV_PMC_SCRATCH171                  0x7CC
#define APBDEV_PMC_SCRATCH172                  0x7D0
#define APBDEV_PMC_SCRATCH173                  0x7D4
#define APBDEV_PMC_SCRATCH174                  0x7D8
#define APBDEV_PMC_SCRATCH175                  0x7DC
#define APBDEV_PMC_SCRATCH176                  0x7E0
#define APBDEV_PMC_SCRATCH177                  0x7E4
#define APBDEV_PMC_SCRATCH178                  0x7E8
#define APBDEV_PMC_SCRATCH179                  0x7EC
#define APBDEV_PMC_SCRATCH180                  0x7F0
#define APBDEV_PMC_SCRATCH181                  0x7F4
#define APBDEV_PMC_SCRATCH182                  0x7F8
#define APBDEV_PMC_SCRATCH183                  0x7FC
#define APBDEV_PMC_SCRATCH184                  0x800
#define APBDEV_PMC_SCRATCH185                  0x804
#define APBDEV_PMC_SCRATCH186                  0x808
#define APBDEV_PMC_SCRATCH187                  0x80C
#define APBDEV_PMC_SCRATCH188                  0x810
#define APBDEV_PMC_SCRATCH189                  0x814
#define APBDEV_PMC_SCRATCH190                  0x818
#define APBDEV_PMC_SCRATCH191                  0x81C
#define APBDEV_PMC_SCRATCH192                  0x820
#define APBDEV_PMC_SCRATCH193                  0x824
#define APBDEV_PMC_SCRATCH194                  0x828
#define APBDEV_PMC_SCRATCH195                  0x82C
#define APBDEV_PMC_SCRATCH196                  0x830
#define APBDEV_PMC_SCRATCH197                  0x834
#define APBDEV_PMC_SCRATCH198                  0x838
#define APBDEV_PMC_SCRATCH199                  0x83C

#define APBDEV_PMC_SCRATCH200                  0x840
#define  PMC_NX_PANIC_SAFE_MODE    0x20
#define  PMC_NX_PANIC_BYPASS_FUSES 0x21

#define APBDEV_PMC_SCRATCH201                  0x844
#define APBDEV_PMC_SCRATCH202                  0x848
#define APBDEV_PMC_SCRATCH203                  0x84C
#define APBDEV_PMC_SCRATCH204                  0x850
#define APBDEV_PMC_SCRATCH205                  0x854
#define APBDEV_PMC_SCRATCH206                  0x858
#define APBDEV_PMC_SCRATCH207                  0x85C
#define APBDEV_PMC_SCRATCH208                  0x860
#define APBDEV_PMC_SCRATCH209                  0x864
#define APBDEV_PMC_SCRATCH210                  0x868
#define APBDEV_PMC_SCRATCH211                  0x86C
#define APBDEV_PMC_SCRATCH212                  0x870
#define APBDEV_PMC_SCRATCH213                  0x874
#define APBDEV_PMC_SCRATCH214                  0x878
#define APBDEV_PMC_SCRATCH215                  0x87C
#define APBDEV_PMC_SCRATCH216                  0x880
#define APBDEV_PMC_SCRATCH217                  0x884
#define APBDEV_PMC_SCRATCH218                  0x888
#define APBDEV_PMC_SCRATCH219                  0x88C
#define APBDEV_PMC_SCRATCH220                  0x890
#define APBDEV_PMC_SCRATCH221                  0x894
#define APBDEV_PMC_SCRATCH222                  0x898
#define APBDEV_PMC_SCRATCH223                  0x89C
#define APBDEV_PMC_SCRATCH224                  0x8A0
#define APBDEV_PMC_SCRATCH225                  0x8A4
#define APBDEV_PMC_SCRATCH226                  0x8A8
#define APBDEV_PMC_SCRATCH227                  0x8AC
#define APBDEV_PMC_SCRATCH228                  0x8B0
#define APBDEV_PMC_SCRATCH229                  0x8B4
#define APBDEV_PMC_SCRATCH230                  0x8B8
#define APBDEV_PMC_SCRATCH231                  0x8BC
#define APBDEV_PMC_SCRATCH232                  0x8C0
#define APBDEV_PMC_SCRATCH233                  0x8C4
#define APBDEV_PMC_SCRATCH234                  0x8C8
#define APBDEV_PMC_SCRATCH235                  0x8CC
#define APBDEV_PMC_SCRATCH236                  0x8D0
#define APBDEV_PMC_SCRATCH237                  0x8D4
#define APBDEV_PMC_SCRATCH238                  0x8D8
#define APBDEV_PMC_SCRATCH239                  0x8DC
#define APBDEV_PMC_SCRATCH240                  0x8E0
#define APBDEV_PMC_SCRATCH241                  0x8E4
#define APBDEV_PMC_SCRATCH242                  0x8E8
#define APBDEV_PMC_SCRATCH243                  0x8EC
#define APBDEV_PMC_SCRATCH244                  0x8F0
#define APBDEV_PMC_SCRATCH245                  0x8F4
#define APBDEV_PMC_SCRATCH246                  0x8F8
#define APBDEV_PMC_SCRATCH247                  0x8FC
#define APBDEV_PMC_SCRATCH248                  0x900
#define APBDEV_PMC_SCRATCH249                  0x904
#define APBDEV_PMC_SCRATCH250                  0x908
#define APBDEV_PMC_SCRATCH251                  0x90C
#define APBDEV_PMC_SCRATCH252                  0x910
#define APBDEV_PMC_SCRATCH253                  0x914
#define APBDEV_PMC_SCRATCH254                  0x918
#define APBDEV_PMC_SCRATCH255                  0x91C
#define APBDEV_PMC_SCRATCH256                  0x920
#define APBDEV_PMC_SCRATCH257                  0x924
#define APBDEV_PMC_SCRATCH258                  0x928
#define APBDEV_PMC_SCRATCH259                  0x92C
#define APBDEV_PMC_SCRATCH260                  0x930
#define APBDEV_PMC_SCRATCH261                  0x934
#define APBDEV_PMC_SCRATCH262                  0x938
#define APBDEV_PMC_SCRATCH263                  0x93C
#define APBDEV_PMC_SCRATCH264                  0x940
#define APBDEV_PMC_SCRATCH265                  0x944
#define APBDEV_PMC_SCRATCH266                  0x948
#define APBDEV_PMC_SCRATCH267                  0x94C
#define APBDEV_PMC_SCRATCH268                  0x950
#define APBDEV_PMC_SCRATCH269                  0x954
#define APBDEV_PMC_SCRATCH270                  0x958
#define APBDEV_PMC_SCRATCH271                  0x95C
#define APBDEV_PMC_SCRATCH272                  0x960
#define APBDEV_PMC_SCRATCH273                  0x964
#define APBDEV_PMC_SCRATCH274                  0x968
#define APBDEV_PMC_SCRATCH275                  0x96C
#define APBDEV_PMC_SCRATCH276                  0x970
#define APBDEV_PMC_SCRATCH277                  0x974
#define APBDEV_PMC_SCRATCH278                  0x978
#define APBDEV_PMC_SCRATCH279                  0x97C
#define APBDEV_PMC_SCRATCH280                  0x980
#define APBDEV_PMC_SCRATCH281                  0x984
#define APBDEV_PMC_SCRATCH282                  0x988
#define APBDEV_PMC_SCRATCH283                  0x98C
#define APBDEV_PMC_SCRATCH284                  0x990
#define APBDEV_PMC_SCRATCH285                  0x994
#define APBDEV_PMC_SCRATCH286                  0x998
#define APBDEV_PMC_SCRATCH287                  0x99C
#define APBDEV_PMC_SCRATCH288                  0x9A0
#define APBDEV_PMC_SCRATCH289                  0x9A4
#define APBDEV_PMC_SCRATCH290                  0x9A8
#define APBDEV_PMC_SCRATCH291                  0x9AC
#define APBDEV_PMC_SCRATCH292                  0x9B0
#define APBDEV_PMC_SCRATCH293                  0x9B4
#define APBDEV_PMC_SCRATCH294                  0x9B8
#define APBDEV_PMC_SCRATCH295                  0x9BC
#define APBDEV_PMC_SCRATCH296                  0x9C0
#define APBDEV_PMC_SCRATCH297                  0x9C4
#define APBDEV_PMC_SCRATCH298                  0x9C8
#define APBDEV_PMC_SCRATCH299                  0x9CC
#define APBDEV_PMC_SECURE_SCRATCH80            0xA98
#define APBDEV_PMC_SECURE_SCRATCH81            0xA9C
#define APBDEV_PMC_SECURE_SCRATCH82            0xAA0
#define APBDEV_PMC_SECURE_SCRATCH83            0xAA4
#define APBDEV_PMC_SECURE_SCRATCH84            0xAA8
#define APBDEV_PMC_SECURE_SCRATCH85            0xAAC
#define APBDEV_PMC_SECURE_SCRATCH86            0xAB0
#define APBDEV_PMC_SECURE_SCRATCH87            0xAB4
#define APBDEV_PMC_SECURE_SCRATCH88            0xAB8
#define APBDEV_PMC_SECURE_SCRATCH89            0xABC
#define APBDEV_PMC_SECURE_SCRATCH90            0xAC0
#define APBDEV_PMC_SECURE_SCRATCH91            0xAC4
#define APBDEV_PMC_SECURE_SCRATCH92            0xAC8
#define APBDEV_PMC_SECURE_SCRATCH93            0xACC
#define APBDEV_PMC_SECURE_SCRATCH94            0xAD0
#define APBDEV_PMC_SECURE_SCRATCH95            0xAD4
#define APBDEV_PMC_SECURE_SCRATCH96            0xAD8
#define APBDEV_PMC_SECURE_SCRATCH97            0xADC
#define APBDEV_PMC_SECURE_SCRATCH98            0xAE0
#define APBDEV_PMC_SECURE_SCRATCH99            0xAE4
#define APBDEV_PMC_SECURE_SCRATCH100           0xAE8
#define APBDEV_PMC_SECURE_SCRATCH101           0xAEC
#define APBDEV_PMC_SECURE_SCRATCH102           0xAF0
#define APBDEV_PMC_SECURE_SCRATCH103           0xAF4
#define APBDEV_PMC_SECURE_SCRATCH104           0xAF8
#define APBDEV_PMC_SECURE_SCRATCH105           0xAFC
#define APBDEV_PMC_SECURE_SCRATCH106           0xB00
#define APBDEV_PMC_SECURE_SCRATCH107           0xB04
#define APBDEV_PMC_SECURE_SCRATCH108           0xB08
#define APBDEV_PMC_SECURE_SCRATCH109           0xB0C
#define APBDEV_PMC_SECURE_SCRATCH110           0xB10
#define APBDEV_PMC_SECURE_SCRATCH111           0xB14
#define APBDEV_PMC_SECURE_SCRATCH112           0xB18
#define APBDEV_PMC_SECURE_SCRATCH113           0xB1C
#define APBDEV_PMC_SECURE_SCRATCH114           0xB20
#define APBDEV_PMC_SECURE_SCRATCH115           0xB24
#define APBDEV_PMC_SECURE_SCRATCH116           0xB28
#define APBDEV_PMC_SECURE_SCRATCH117           0xB2C
#define APBDEV_PMC_SECURE_SCRATCH118           0xB30
#define APBDEV_PMC_SECURE_SCRATCH119           0xB34

/* T210B01 only registers */
#define APBDEV_PMC_SEC_DISABLE9_B01            0x5C4
#define APBDEV_PMC_SEC_DISABLE10_B01           0x5C8
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE0_B01  0xA48
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE1_B01  0xA4C
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE2_B01  0xA50
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE3_B01  0xA54
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE4_B01  0xA58
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE5_B01  0xA5C
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE6_B01  0xA60
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE7_B01  0xA64
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE8_B01  0xA68
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE9_B01  0xA6C
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE10_B01 0xA70
#define APBDEV_PMC_SCRATCH_WRITE_LOCK_DISABLE_STICKY_B01 0xA74
#define APBDEV_PMC_SECURE_SCRATCH120_B01       0xB38
#define APBDEV_PMC_SECURE_SCRATCH121_B01       0xB3C
#define APBDEV_PMC_SECURE_SCRATCH122_B01       0xB40
#define APBDEV_PMC_SECURE_SCRATCH123_B01       0xB44

#define APBDEV_PMC_LED_BREATHING_CTRL_B01      0xB48
#define  PMC_LED_BREATHING_CTRL_ENABLE      BIT(0)
#define  PMC_LED_BREATHING_CTRL_COUNTER1_EN BIT(1)

#define APBDEV_PMC_LED_BREATHING_SLOPE_STEPS_B01  0xB4C
#define APBDEV_PMC_LED_BREATHING_ON_COUNTER_B01   0xB50
#define APBDEV_PMC_LED_BREATHING_OFF_COUNTER1_B01 0xB54
#define APBDEV_PMC_LED_BREATHING_OFF_COUNTER0_B01 0xB58
#define  PMC_LED_BREATHING_COUNTER_HZ 32768

#define APBDEV_PMC_LED_BREATHING_STATUS_B01    0xB5C
#define  PMC_LED_BREATHING_FSM_STATUS_MASK          0x7
#define  PMC_LED_BREATHING_FSM_STS_IDLE             0
#define  PMC_LED_BREATHING_FSM_STS_UP_RAMP          1
#define  PMC_LED_BREATHING_FSM_STS_PLATEAU          2
#define  PMC_LED_BREATHING_FSM_STS_DOWN_RAMP        3
#define  PMC_LED_BREATHING_FSM_STS_SHORT_LOW_PERIOD 4
#define  PMC_LED_BREATHING_FSM_STS_LONG_LOW_PERIOD  5

#define APBDEV_PMC_SECURE_SCRATCH124_B01       0xB68
#define APBDEV_PMC_SECURE_SCRATCH125_B01       0xB6C
#define APBDEV_PMC_SECURE_SCRATCH126_B01       0xB70
#define APBDEV_PMC_SECURE_SCRATCH127_B01       0xB74
#define APBDEV_PMC_SECURE_SCRATCH128_B01       0xB78
#define APBDEV_PMC_SECURE_SCRATCH129_B01       0xB7C
#define APBDEV_PMC_SECURE_SCRATCH130_B01       0xB80
#define APBDEV_PMC_SECURE_SCRATCH131_B01       0xB84
#define APBDEV_PMC_SECURE_SCRATCH132_B01       0xB88
#define APBDEV_PMC_SECURE_SCRATCH133_B01       0xB8C
#define APBDEV_PMC_SECURE_SCRATCH134_B01       0xB90
#define APBDEV_PMC_SECURE_SCRATCH135_B01       0xB94
#define APBDEV_PMC_SECURE_SCRATCH136_B01       0xB98
#define APBDEV_PMC_SECURE_SCRATCH137_B01       0xB9C
#define APBDEV_PMC_SECURE_SCRATCH138_B01       0xBA0
#define APBDEV_PMC_SECURE_SCRATCH139_B01       0xBA4
#define APBDEV_PMC_SEC_DISABLE_NS_B01          0xBB0
#define APBDEV_PMC_SEC_DISABLE2_NS_B01         0xBB4
#define APBDEV_PMC_SEC_DISABLE3_NS_B01         0xBB8
#define APBDEV_PMC_SEC_DISABLE4_NS_B01         0xBBC
#define APBDEV_PMC_SEC_DISABLE5_NS_B01         0xBC0
#define APBDEV_PMC_SEC_DISABLE6_NS_B01         0xBC4
#define APBDEV_PMC_SEC_DISABLE7_NS_B01         0xBC8
#define APBDEV_PMC_SEC_DISABLE8_NS_B01         0xBCC
#define APBDEV_PMC_SEC_DISABLE9_NS_B01         0xBD0
#define APBDEV_PMC_SEC_DISABLE10_NS_B01        0xBD4

#define APBDEV_PMC_TZRAM_PWR_CNTRL_B01         0xBE8
#define  PMC_TZRAM_PWR_CNTRL_SD BIT(0)

#define APBDEV_PMC_TZRAM_SEC_DISABLE_B01       0xBEC
#define APBDEV_PMC_TZRAM_NON_SEC_DISABLE_B01   0xBF0
#define  PMC_TZRAM_DISABLE_REG_WRITE BIT(0)
#define  PMC_TZRAM_DISABLE_REG_READ  BIT(1)

typedef struct _pmc_regs_t210_t  {
/* 0x000 */	u32 pmc_cntrl;
/* 0x004 */	u32 pmc_sec_disable;
/* 0x008 */	u32 pmc_pmc_swrst;
/* 0x00c */	u32 pmc_wake_mask;
/* 0x010 */	u32 pmc_wake_lvl;
/* 0x014 */	u32 pmc_wake_status;
/* 0x018 */	u32 pmc_sw_wake_status;
/* 0x01c */	u32 pmc_dpd_pads_oride;
/* 0x020 */	u32 pmc_dpd_sample;
/* 0x024 */	u32 pmc_dpd_enable;
/* 0x028 */	u32 pmc_pwrgate_timer_off;
/* 0x02c */	u32 pmc_clamp_status;
/* 0x030 */	u32 pmc_pwrgate_toggle;
/* 0x034 */	u32 pmc_remove_clamping_cmd;
/* 0x038 */	u32 pmc_pwrgate_status;
/* 0x03c */	u32 pmc_pwrgood_timer;
/* 0x040 */	u32 pmc_blink_timer;
/* 0x044 */	u32 pmc_no_iopower;
/* 0x048 */	u32 pmc_pwr_det;
/* 0x04c */	u32 pmc_pwr_det_latch;
/* 0x050 */	u32 pmc_scratch0;
/* 0x054 */	u32 pmc_scratch1;
/* 0x058 */	u32 pmc_scratch2;
/* 0x05c */	u32 pmc_scratch3;
/* 0x060 */	u32 pmc_scratch4;
/* 0x064 */	u32 pmc_scratch5;
/* 0x068 */	u32 pmc_scratch6;
/* 0x06c */	u32 pmc_scratch7;
/* 0x070 */	u32 pmc_scratch8;
/* 0x074 */	u32 pmc_scratch9;
/* 0x078 */	u32 pmc_scratch10;
/* 0x07c */	u32 pmc_scratch11;
/* 0x080 */	u32 pmc_scratch12;
/* 0x084 */	u32 pmc_scratch13;
/* 0x088 */	u32 pmc_scratch14;
/* 0x08c */	u32 pmc_scratch15;
/* 0x090 */	u32 pmc_scratch16;
/* 0x094 */	u32 pmc_scratch17;
/* 0x098 */	u32 pmc_scratch18;
/* 0x09c */	u32 pmc_scratch19;
/* 0x0a0 */	u32 pmc_scratch20; // ODM data/config scratch.
/* 0x0a4 */	u32 pmc_scratch21;
/* 0x0a8 */	u32 pmc_scratch22;
/* 0x0ac */	u32 pmc_scratch23;
/* 0x0b0 */	u32 pmc_secure_scratch0;
/* 0x0b4 */	u32 pmc_secure_scratch1;
/* 0x0b8 */	u32 pmc_secure_scratch2;
/* 0x0bc */	u32 pmc_secure_scratch3;
/* 0x0c0 */	u32 pmc_secure_scratch4;
/* 0x0c4 */	u32 pmc_secure_scratch5;
/* 0x0c8 */	u32 pmc_cpupwrgood_timer;
/* 0x0cc */	u32 pmc_cpupwroff_timer;
/* 0x0d0 */	u32 pmc_pg_mask;
/* 0x0d4 */	u32 pmc_pg_mask_1;
/* 0x0d8 */	u32 pmc_auto_wake_lvl;
/* 0x0dc */	u32 pmc_auto_wake_lvl_mask;
/* 0x0e0 */	u32 pmc_wake_delay;
/* 0x0e4 */	u32 pmc_pwr_det_val;
/* 0x0e8 */	u32 pmc_ddr_pwr;
/* 0x0ec */	u32 pmc_usb_debounce_del;
/* 0x0f0 */	u32 pmc_usb_ao;
/* 0x0f4 */	u32 pmc_crypto_op;
/* 0x0f8 */	u32 pmc_pllp_wb0_override;
/* 0x0fc */	u32 pmc_scratch24;
/* 0x100 */	u32 pmc_scratch25;
/* 0x104 */	u32 pmc_scratch26;
/* 0x108 */	u32 pmc_scratch27;
/* 0x10c */	u32 pmc_scratch28;
/* 0x110 */	u32 pmc_scratch29;
/* 0x114 */	u32 pmc_scratch30;
/* 0x118 */	u32 pmc_scratch31;
/* 0x11c */	u32 pmc_scratch32;
/* 0x120 */	u32 pmc_scratch33;
/* 0x124 */	u32 pmc_scratch34;
/* 0x128 */	u32 pmc_scratch35;
/* 0x12c */	u32 pmc_scratch36;
/* 0x130 */	u32 pmc_scratch37;
/* 0x134 */	u32 pmc_scratch38;
/* 0x138 */	u32 pmc_scratch39;
/* 0x13c */	u32 pmc_scratch40;
/* 0x140 */	u32 pmc_scratch41;
/* 0x144 */	u32 pmc_scratch42;
/* 0x148 */	u32 pmc_bondout_mirror0;
/* 0x14c */	u32 pmc_bondout_mirror1;
/* 0x150 */	u32 pmc_bondout_mirror2;
/* 0x154 */	u32 pmc_sys_33v_en;
/* 0x158 */	u32 pmc_bondout_mirror_access;
/* 0x15c */	u32 pmc_gate;
/* 0x160 */	u32 pmc_wake2_mask;
/* 0x164 */	u32 pmc_wake2_lvl;
/* 0x168 */	u32 pmc_wake2_status;
/* 0x16c */	u32 pmc_sw_wake2_status;
/* 0x170 */	u32 pmc_auto_wake2_lvl_mask;
/* 0x174 */	u32 pmc_pg_mask_2;
/* 0x178 */	u32 pmc_pg_mask_ce1;
/* 0x17c */	u32 pmc_pg_mask_ce2;
/* 0x180 */	u32 pmc_pg_mask_ce3;
/* 0x184 */	u32 pmc_pwrgate_timer_ce_0;
/* 0x188 */	u32 pmc_pwrgate_timer_ce_1;
/* 0x18c */	u32 pmc_pwrgate_timer_ce_2;
/* 0x190 */	u32 pmc_pwrgate_timer_ce_3;
/* 0x194 */	u32 pmc_pwrgate_timer_ce_4;
/* 0x198 */	u32 pmc_pwrgate_timer_ce_5;
/* 0x19c */	u32 pmc_pwrgate_timer_ce_6;
/* 0x1a0 */	u32 pmc_pcx_edpd_cntrl;
/* 0x1a4 */	u32 pmc_osc_edpd_over;
/* 0x1a8 */	u32 pmc_clk_out_cntrl;
/* 0x1ac */	u32 pmc_sata_pwrgt;
/* 0x1b0 */	u32 pmc_sensor_ctrl;
/* 0x1b4 */	u32 pmc_rst_status;
/* 0x1b8 */	u32 pmc_io_dpd_req;
/* 0x1bc */	u32 pmc_io_dpd_status;
/* 0x1c0 */	u32 pmc_io_dpd2_req;
/* 0x1c4 */	u32 pmc_io_dpd2_status;
/* 0x1c8 */	u32 pmc_sel_dpd_tim;
/* 0x1cc */	u32 pmc_vddp_sel;
/* 0x1d0 */	u32 pmc_ddr_cfg;
/* 0x1d4 */	u32 pmc_e_no_vttgen;
/* 0x1d8 */	u32 rsvd_1d8;
/* 0x1dc */	u32 pmc_pllm_wb0_override_freq;
/* 0x1e0 */	u32 pmc_test_pwrgate;
/* 0x1e4 */	u32 pmc_pwrgate_timer_mult;
/* 0x1e8 */	u32 pmc_dsi_sel_dpd;
/* 0x1ec */	u32 pmc_utmip_uhsic_triggers;
/* 0x1f0 */	u32 pmc_utmip_uhsic_saved_state;
/* 0x1f4 */	u32 rsvd_1f4;
/* 0x1f8 */	u32 pmc_utmip_term_pad_cfg;
/* 0x1fc */	u32 pmc_utmip_uhsic_sleep_cfg;
/* 0x200 */	u32 pmc_utmip_uhsic_sleepwalk_cfg;
/* 0x204 */	u32 pmc_utmip_sleepwalk_p0;
/* 0x208 */	u32 pmc_utmip_sleepwalk_p1;
/* 0x20c */	u32 pmc_utmip_sleepwalk_p2;
/* 0x210 */	u32 pmc_uhsic_sleepwalk_p0;
/* 0x214 */	u32 pmc_utmip_uhsic_status;
/* 0x218 */	u32 pmc_utmip_uhsic_fake;
/* 0x21c */	u32 pmc_bondout_mirror3;
/* 0x220 */	u32 pmc_bondout_mirror4;
/* 0x224 */	u32 pmc_secure_scratch6;
/* 0x228 */	u32 pmc_secure_scratch7;
/* 0x22c */	u32 pmc_scratch43;
/* 0x230 */	u32 pmc_scratch44;
/* 0x234 */	u32 pmc_scratch45;
/* 0x238 */	u32 pmc_scratch46;
/* 0x23c */	u32 pmc_scratch47;
/* 0x240 */	u32 pmc_scratch48;
/* 0x244 */	u32 pmc_scratch49;
/* 0x248 */	u32 pmc_scratch50;
/* 0x24c */	u32 pmc_scratch51;
/* 0x250 */	u32 pmc_scratch52;
/* 0x254 */	u32 pmc_scratch53;
/* 0x258 */	u32 pmc_scratch54;
/* 0x25c */	u32 pmc_scratch55;
/* 0x260 */	u32 pmc_scratch0_eco;
/* 0x264 */	u32 pmc_por_dpd_ctrl;
/* 0x268 */	u32 pmc_scratch2_eco;
/* 0x26c */	u32 pmc_utmip_uhsic_line_wakeup;
/* 0x270 */	u32 pmc_utmip_bias_master_cntrl;
/* 0x274 */	u32 pmc_utmip_master_config;
/* 0x278 */	u32 pmc_td_pwrgate_inter_part_timer;
/* 0x27c */	u32 pmc_utmip_uhsic2_triggers;
/* 0x280 */	u32 pmc_utmip_uhsic2_saved_state;
/* 0x284 */	u32 pmc_utmip_uhsic2_sleep_cfg;
/* 0x288 */	u32 pmc_utmip_uhsic2_sleepwalk_cfg;
/* 0x28c */	u32 pmc_uhsic2_sleepwalk_p1;
/* 0x290 */	u32 pmc_utmip_uhsic2_status;
/* 0x294 */	u32 pmc_utmip_uhsic2_fake;
/* 0x298 */	u32 pmc_utmip_uhsic2_line_wakeup;
/* 0x29c */	u32 pmc_utmip_master2_config;
/* 0x2a0 */	u32 pmc_utmip_uhsic_rpd_cfg;
/* 0x2a4 */	u32 pmc_pg_mask_ce0;
/* 0x2a8 */	u32 pmc_pg_mask_3;
/* 0x2ac */	u32 pmc_pg_mask_4;
/* 0x2b0 */	u32 pmc_pllm_wb0_override2;
/* 0x2b4 */	u32 pmc_tsc_mult;
/* 0x2b8 */	u32 pmc_cpu_vsense_override;
/* 0x2bc */	u32 pmc_glb_amap_cfg;
/* 0x2c0 */	u32 pmc_sticky_bits;
/* 0x2c4 */	u32 pmc_sec_disable2;
/* 0x2c8 */	u32 pmc_weak_bias;
/* 0x2cc */	u32 pmc_reg_short;
/* 0x2d0 */	u32 pmc_pg_mask_andor;
/* 0x2d4 */	u32 pmc_gpu_rg_cntrl;
/* 0x2d8 */	u32 pmc_sec_disable3;
/* 0x2dc */	u32 pmc_pg_mask_5;
/* 0x2e0 */	u32 pmc_pg_mask_6;
/* 0x2e4 */	u32 rsvd_2e4[7];
/* 0x300 */	u32 pmc_secure_scratch8;
/* 0x304 */	u32 pmc_secure_scratch9;
/* 0x308 */	u32 pmc_secure_scratch10;
/* 0x30c */	u32 pmc_secure_scratch11;
/* 0x310 */	u32 pmc_secure_scratch12;
/* 0x314 */	u32 pmc_secure_scratch13;
/* 0x318 */	u32 pmc_secure_scratch14;
/* 0x31c */	u32 pmc_secure_scratch15;
/* 0x320 */	u32 pmc_secure_scratch16;
/* 0x324 */	u32 pmc_secure_scratch17;
/* 0x328 */	u32 pmc_secure_scratch18;
/* 0x32c */	u32 pmc_secure_scratch19;
/* 0x330 */	u32 pmc_secure_scratch20;
/* 0x334 */	u32 pmc_secure_scratch21;
/* 0x338 */	u32 pmc_secure_scratch22; // AArch32 reset address.
/* 0x33c */	u32 pmc_secure_scratch23;
/* 0x340 */	u32 pmc_secure_scratch24;
/* 0x344 */	u32 pmc_secure_scratch25;
/* 0x348 */	u32 pmc_secure_scratch26;
/* 0x34c */	u32 pmc_secure_scratch27;
/* 0x350 */	u32 pmc_secure_scratch28;
/* 0x354 */	u32 pmc_secure_scratch29;
/* 0x358 */	u32 pmc_secure_scratch30;
/* 0x35c */	u32 pmc_secure_scratch31;
/* 0x360 */	u32 pmc_secure_scratch32;
/* 0x364 */	u32 pmc_secure_scratch33;
/* 0x368 */	u32 pmc_secure_scratch34; // AArch64 reset address.
/* 0x36c */	u32 pmc_secure_scratch35; // AArch64 reset hi-address.
/* 0x370 */	u32 pmc_secure_scratch36;
/* 0x374 */	u32 pmc_secure_scratch37;
/* 0x378 */	u32 pmc_secure_scratch38;
/* 0x37c */	u32 pmc_secure_scratch39;
/* 0x380 */	u32 pmc_secure_scratch40;
/* 0x384 */	u32 pmc_secure_scratch41;
/* 0x388 */	u32 pmc_secure_scratch42;
/* 0x38c */	u32 pmc_secure_scratch43;
/* 0x390 */	u32 pmc_secure_scratch44;
/* 0x394 */	u32 pmc_secure_scratch45;
/* 0x398 */	u32 pmc_secure_scratch46;
/* 0x39c */	u32 pmc_secure_scratch47;
/* 0x3a0 */	u32 pmc_secure_scratch48;
/* 0x3a4 */	u32 pmc_secure_scratch49;
/* 0x3a8 */	u32 pmc_secure_scratch50;
/* 0x3ac */	u32 pmc_secure_scratch51;
/* 0x3b0 */	u32 pmc_secure_scratch52;
/* 0x3b4 */	u32 pmc_secure_scratch53;
/* 0x3b8 */	u32 pmc_secure_scratch54;
/* 0x3bc */	u32 pmc_secure_scratch55;
/* 0x3c0 */	u32 pmc_secure_scratch56;
/* 0x3c4 */	u32 pmc_secure_scratch57;
/* 0x3c8 */	u32 pmc_secure_scratch58;
/* 0x3cc */	u32 pmc_secure_scratch59;
/* 0x3d0 */	u32 pmc_secure_scratch60;
/* 0x3d4 */	u32 pmc_secure_scratch61;
/* 0x3d8 */	u32 pmc_secure_scratch62;
/* 0x3dc */	u32 pmc_secure_scratch63;
/* 0x3e0 */	u32 pmc_secure_scratch64;
/* 0x3e4 */	u32 pmc_secure_scratch65;
/* 0x3e8 */	u32 pmc_secure_scratch66;
/* 0x3ec */	u32 pmc_secure_scratch67;
/* 0x3f0 */	u32 pmc_secure_scratch68;
/* 0x3f4 */	u32 pmc_secure_scratch69;
/* 0x3f8 */	u32 pmc_secure_scratch70;
/* 0x3fc */	u32 pmc_secure_scratch71;
/* 0x400 */	u32 pmc_secure_scratch72;
/* 0x404 */	u32 pmc_secure_scratch73;
/* 0x408 */	u32 pmc_secure_scratch74;
/* 0x40c */	u32 pmc_secure_scratch75;
/* 0x410 */	u32 pmc_secure_scratch76;
/* 0x414 */	u32 pmc_secure_scratch77;
/* 0x418 */	u32 pmc_secure_scratch78;
/* 0x41c */	u32 pmc_secure_scratch79;
/* 0x420 */	u32 rsvd_420[8];
/* 0x440 */	u32 pmc_cntrl2;
/* 0x444 */	u32 pmc_io_dpd_off_mask;
/* 0x448 */	u32 pmc_io_dpd2_off_mask;
/* 0x44c */	u32 pmc_event_counter;
/* 0x450 */	u32 pmc_fuse_control;
/* 0x454 */	u32 pmc_scratch1_eco;
/* 0x458 */	u32 rsvd_458;
/* 0x45c */	u32 pmc_io_dpd3_req;
/* 0x460 */	u32 pmc_io_dpd3_status;
/* 0x464 */	u32 pmc_io_dpd4_req;
/* 0x468 */	u32 pmc_io_dpd4_status;
/* 0x46c */	u32 rsvd_46c[2];
/* 0x474 */	u32 pmc_direct_thermtrip_cfg;
/* 0x478 */	u32 pmc_tsosc_delay;
/* 0x47c */	u32 pmc_set_sw_clamp;
/* 0x480 */	u32 pmc_debug_authentication;
/* 0x484 */	u32 pmc_aotag_cfg;
/* 0x488 */	u32 pmc_aotag_thresh1_cfg;
/* 0x48c */	u32 pmc_aotag_thresh2_cfg;
/* 0x490 */	u32 pmc_aotag_thresh3_cfg;
/* 0x494 */	u32 pmc_aotag_status;
/* 0x498 */	u32 pmc_aotag_security;
/* 0x49c */	u32 pmc_tsensor_config0;
/* 0x4a0 */	u32 pmc_tsensor_config1;
/* 0x4a4 */	u32 pmc_tsensor_config2;
/* 0x4a8 */	u32 pmc_tsensor_status0;
/* 0x4ac */	u32 pmc_tsensor_status1;
/* 0x4b0 */	u32 pmc_tsensor_status2;
/* 0x4b4 */	u32 pmc_tsensor_pdiv;
/* 0x4b8 */	u32 pmc_aotag_intr_en;
/* 0x4bc */	u32 pmc_aotag_intr_dis;
/* 0x4c0 */	u32 pmc_utmip_pad_cfg0;
/* 0x4c4 */	u32 pmc_utmip_pad_cfg1;
/* 0x4c8 */	u32 pmc_utmip_pad_cfg2;
/* 0x4cc */	u32 pmc_utmip_pad_cfg3;
/* 0x4d0 */	u32 pmc_utmip_uhsic_sleep_cfg1;
/* 0x4d4 */	u32 pmc_cc4_hvc_control;
/* 0x4d8 */	u32 pmc_wake_debounce_en;
/* 0x4dc */	u32 pmc_ramdump_ctl_status;
/* 0x4e0 */	u32 pmc_utmip_sleepwalk_p3;
/* 0x4e4 */	u32 pmc_ddr_cntrl;
/* 0x4e8 */	u32 rsvd_4e8[50];
/* 0x5b0 */	u32 pmc_sec_disable4;
/* 0x5b4 */	u32 pmc_sec_disable5;
/* 0x5b8 */	u32 pmc_sec_disable6;
/* 0x5bc */	u32 pmc_sec_disable7;
/* 0x5c0 */	u32 pmc_sec_disable8;
/* 0x5c4 */	u32 pmc_sec_disable9_b01;
/* 0x5c8 */	u32 pmc_sec_disable10_b01;
/* 0x5cc */	u32 rsvd_5cc[13];
/* 0x600 */	u32 pmc_scratch56;
/* 0x604 */	u32 pmc_scratch57;
/* 0x608 */	u32 pmc_scratch58;
/* 0x60c */	u32 pmc_scratch59;
/* 0x610 */	u32 pmc_scratch60;
/* 0x614 */	u32 pmc_scratch61;
/* 0x618 */	u32 pmc_scratch62;
/* 0x61c */	u32 pmc_scratch63;
/* 0x620 */	u32 pmc_scratch64;
/* 0x624 */	u32 pmc_scratch65;
/* 0x628 */	u32 pmc_scratch66;
/* 0x62c */	u32 pmc_scratch67;
/* 0x630 */	u32 pmc_scratch68;
/* 0x634 */	u32 pmc_scratch69;
/* 0x638 */	u32 pmc_scratch70;
/* 0x63c */	u32 pmc_scratch71;
/* 0x640 */	u32 pmc_scratch72;
/* 0x644 */	u32 pmc_scratch73;
/* 0x648 */	u32 pmc_scratch74;
/* 0x64c */	u32 pmc_scratch75;
/* 0x650 */	u32 pmc_scratch76;
/* 0x654 */	u32 pmc_scratch77;
/* 0x658 */	u32 pmc_scratch78;
/* 0x65c */	u32 pmc_scratch79;
/* 0x660 */	u32 pmc_scratch80;
/* 0x664 */	u32 pmc_scratch81;
/* 0x668 */	u32 pmc_scratch82;
/* 0x66c */	u32 pmc_scratch83;
/* 0x670 */	u32 pmc_scratch84;
/* 0x674 */	u32 pmc_scratch85;
/* 0x678 */	u32 pmc_scratch86;
/* 0x67c */	u32 pmc_scratch87;
/* 0x680 */	u32 pmc_scratch88;
/* 0x684 */	u32 pmc_scratch89;
/* 0x688 */	u32 pmc_scratch90;
/* 0x68c */	u32 pmc_scratch91;
/* 0x690 */	u32 pmc_scratch92;
/* 0x694 */	u32 pmc_scratch93;
/* 0x698 */	u32 pmc_scratch94;
/* 0x69c */	u32 pmc_scratch95;
/* 0x6a0 */	u32 pmc_scratch96;
/* 0x6a4 */	u32 pmc_scratch97;
/* 0x6a8 */	u32 pmc_scratch98;
/* 0x6ac */	u32 pmc_scratch99;
/* 0x6b0 */	u32 pmc_scratch100;
/* 0x6b4 */	u32 pmc_scratch101;
/* 0x6b8 */	u32 pmc_scratch102;
/* 0x6bc */	u32 pmc_scratch103;
/* 0x6c0 */	u32 pmc_scratch104;
/* 0x6c4 */	u32 pmc_scratch105;
/* 0x6c8 */	u32 pmc_scratch106;
/* 0x6cc */	u32 pmc_scratch107;
/* 0x6d0 */	u32 pmc_scratch108;
/* 0x6d4 */	u32 pmc_scratch109;
/* 0x6d8 */	u32 pmc_scratch110;
/* 0x6dc */	u32 pmc_scratch111;
/* 0x6e0 */	u32 pmc_scratch112;
/* 0x6e4 */	u32 pmc_scratch113;
/* 0x6e8 */	u32 pmc_scratch114;
/* 0x6ec */	u32 pmc_scratch115;
/* 0x6f0 */	u32 pmc_scratch116;
/* 0x6f4 */	u32 pmc_scratch117;
/* 0x6f8 */	u32 pmc_scratch118;
/* 0x6fc */	u32 pmc_scratch119;
/* 0x700 */	u32 pmc_scratch120;
/* 0x704 */	u32 pmc_scratch121;
/* 0x708 */	u32 pmc_scratch122;
/* 0x70c */	u32 pmc_scratch123;
/* 0x710 */	u32 pmc_scratch124;
/* 0x714 */	u32 pmc_scratch125;
/* 0x718 */	u32 pmc_scratch126;
/* 0x71c */	u32 pmc_scratch127;
/* 0x720 */	u32 pmc_scratch128;
/* 0x724 */	u32 pmc_scratch129;
/* 0x728 */	u32 pmc_scratch130;
/* 0x72c */	u32 pmc_scratch131;
/* 0x730 */	u32 pmc_scratch132;
/* 0x734 */	u32 pmc_scratch133;
/* 0x738 */	u32 pmc_scratch134;
/* 0x73c */	u32 pmc_scratch135;
/* 0x740 */	u32 pmc_scratch136;
/* 0x744 */	u32 pmc_scratch137;
/* 0x748 */	u32 pmc_scratch138;
/* 0x74c */	u32 pmc_scratch139;
/* 0x750 */	u32 pmc_scratch140;
/* 0x754 */	u32 pmc_scratch141;
/* 0x758 */	u32 pmc_scratch142;
/* 0x75c */	u32 pmc_scratch143;
/* 0x760 */	u32 pmc_scratch144;
/* 0x764 */	u32 pmc_scratch145;
/* 0x768 */	u32 pmc_scratch146;
/* 0x76c */	u32 pmc_scratch147;
/* 0x770 */	u32 pmc_scratch148;
/* 0x774 */	u32 pmc_scratch149;
/* 0x778 */	u32 pmc_scratch150;
/* 0x77c */	u32 pmc_scratch151;
/* 0x780 */	u32 pmc_scratch152;
/* 0x784 */	u32 pmc_scratch153;
/* 0x788 */	u32 pmc_scratch154;
/* 0x78c */	u32 pmc_scratch155;
/* 0x790 */	u32 pmc_scratch156;
/* 0x794 */	u32 pmc_scratch157;
/* 0x798 */	u32 pmc_scratch158;
/* 0x79c */	u32 pmc_scratch159;
/* 0x7a0 */	u32 pmc_scratch160;
/* 0x7a4 */	u32 pmc_scratch161;
/* 0x7a8 */	u32 pmc_scratch162;
/* 0x7ac */	u32 pmc_scratch163;
/* 0x7b0 */	u32 pmc_scratch164;
/* 0x7b4 */	u32 pmc_scratch165;
/* 0x7b8 */	u32 pmc_scratch166;
/* 0x7bc */	u32 pmc_scratch167;
/* 0x7c0 */	u32 pmc_scratch168;
/* 0x7c4 */	u32 pmc_scratch169;
/* 0x7c8 */	u32 pmc_scratch170;
/* 0x7cc */	u32 pmc_scratch171;
/* 0x7d0 */	u32 pmc_scratch172;
/* 0x7d4 */	u32 pmc_scratch173;
/* 0x7d8 */	u32 pmc_scratch174;
/* 0x7dc */	u32 pmc_scratch175;
/* 0x7e0 */	u32 pmc_scratch176;
/* 0x7e4 */	u32 pmc_scratch177;
/* 0x7e8 */	u32 pmc_scratch178;
/* 0x7ec */	u32 pmc_scratch179;
/* 0x7f0 */	u32 pmc_scratch180;
/* 0x7f4 */	u32 pmc_scratch181;
/* 0x7f8 */	u32 pmc_scratch182;
/* 0x7fc */	u32 pmc_scratch183;
/* 0x800 */	u32 pmc_scratch184;
/* 0x804 */	u32 pmc_scratch185;
/* 0x808 */	u32 pmc_scratch186;
/* 0x80c */	u32 pmc_scratch187;
/* 0x810 */	u32 pmc_scratch188;
/* 0x814 */	u32 pmc_scratch189;
/* 0x818 */	u32 pmc_scratch190;
/* 0x81c */	u32 pmc_scratch191;
/* 0x820 */	u32 pmc_scratch192;
/* 0x824 */	u32 pmc_scratch193;
/* 0x828 */	u32 pmc_scratch194;
/* 0x82c */	u32 pmc_scratch195;
/* 0x830 */	u32 pmc_scratch196;
/* 0x834 */	u32 pmc_scratch197;
/* 0x838 */	u32 pmc_scratch198;
/* 0x83c */	u32 pmc_scratch199;
/* 0x840 */	u32 pmc_scratch200;
/* 0x844 */	u32 pmc_scratch201;
/* 0x848 */	u32 pmc_scratch202;
/* 0x84c */	u32 pmc_scratch203;
/* 0x850 */	u32 pmc_scratch204;
/* 0x854 */	u32 pmc_scratch205;
/* 0x858 */	u32 pmc_scratch206;
/* 0x85c */	u32 pmc_scratch207;
/* 0x860 */	u32 pmc_scratch208;
/* 0x864 */	u32 pmc_scratch209;
/* 0x868 */	u32 pmc_scratch210;
/* 0x86c */	u32 pmc_scratch211;
/* 0x870 */	u32 pmc_scratch212;
/* 0x874 */	u32 pmc_scratch213;
/* 0x878 */	u32 pmc_scratch214;
/* 0x87c */	u32 pmc_scratch215;
/* 0x880 */	u32 pmc_scratch216;
/* 0x884 */	u32 pmc_scratch217;
/* 0x888 */	u32 pmc_scratch218;
/* 0x88c */	u32 pmc_scratch219;
/* 0x890 */	u32 pmc_scratch220;
/* 0x894 */	u32 pmc_scratch221;
/* 0x898 */	u32 pmc_scratch222;
/* 0x89c */	u32 pmc_scratch223;
/* 0x8a0 */	u32 pmc_scratch224;
/* 0x8a4 */	u32 pmc_scratch225;
/* 0x8a8 */	u32 pmc_scratch226;
/* 0x8ac */	u32 pmc_scratch227;
/* 0x8b0 */	u32 pmc_scratch228;
/* 0x8b4 */	u32 pmc_scratch229;
/* 0x8b8 */	u32 pmc_scratch230;
/* 0x8bc */	u32 pmc_scratch231;
/* 0x8c0 */	u32 pmc_scratch232;
/* 0x8c4 */	u32 pmc_scratch233;
/* 0x8c8 */	u32 pmc_scratch234;
/* 0x8cc */	u32 pmc_scratch235;
/* 0x8d0 */	u32 pmc_scratch236;
/* 0x8d4 */	u32 pmc_scratch237;
/* 0x8d8 */	u32 pmc_scratch238;
/* 0x8dc */	u32 pmc_scratch239;
/* 0x8e0 */	u32 pmc_scratch240;
/* 0x8e4 */	u32 pmc_scratch241;
/* 0x8e8 */	u32 pmc_scratch242;
/* 0x8ec */	u32 pmc_scratch243;
/* 0x8f0 */	u32 pmc_scratch244;
/* 0x8f4 */	u32 pmc_scratch245;
/* 0x8f8 */	u32 pmc_scratch246;
/* 0x8fc */	u32 pmc_scratch247;
/* 0x900 */	u32 pmc_scratch248;
/* 0x904 */	u32 pmc_scratch249;
/* 0x908 */	u32 pmc_scratch250;
/* 0x90c */	u32 pmc_scratch251;
/* 0x910 */	u32 pmc_scratch252;
/* 0x914 */	u32 pmc_scratch253;
/* 0x918 */	u32 pmc_scratch254;
/* 0x91c */	u32 pmc_scratch255;
/* 0x920 */	u32 pmc_scratch256;
/* 0x924 */	u32 pmc_scratch257;
/* 0x928 */	u32 pmc_scratch258;
/* 0x92c */	u32 pmc_scratch259;
/* 0x930 */	u32 pmc_scratch260;
/* 0x934 */	u32 pmc_scratch261;
/* 0x938 */	u32 pmc_scratch262;
/* 0x93c */	u32 pmc_scratch263;
/* 0x940 */	u32 pmc_scratch264;
/* 0x944 */	u32 pmc_scratch265;
/* 0x948 */	u32 pmc_scratch266;
/* 0x94c */	u32 pmc_scratch267;
/* 0x950 */	u32 pmc_scratch268;
/* 0x954 */	u32 pmc_scratch269;
/* 0x958 */	u32 pmc_scratch270;
/* 0x95c */	u32 pmc_scratch271;
/* 0x960 */	u32 pmc_scratch272;
/* 0x964 */	u32 pmc_scratch273;
/* 0x968 */	u32 pmc_scratch274;
/* 0x96c */	u32 pmc_scratch275;
/* 0x970 */	u32 pmc_scratch276;
/* 0x974 */	u32 pmc_scratch277;
/* 0x978 */	u32 pmc_scratch278;
/* 0x97c */	u32 pmc_scratch279;
/* 0x980 */	u32 pmc_scratch280;
/* 0x984 */	u32 pmc_scratch281;
/* 0x988 */	u32 pmc_scratch282;
/* 0x98c */	u32 pmc_scratch283;
/* 0x990 */	u32 pmc_scratch284;
/* 0x994 */	u32 pmc_scratch285;
/* 0x998 */	u32 pmc_scratch286;
/* 0x99c */	u32 pmc_scratch287;
/* 0x9a0 */	u32 pmc_scratch288;
/* 0x9a4 */	u32 pmc_scratch289;
/* 0x9a8 */	u32 pmc_scratch290;
/* 0x9ac */	u32 pmc_scratch291;
/* 0x9b0 */	u32 pmc_scratch292;
/* 0x9b4 */	u32 pmc_scratch293;
/* 0x9b8 */	u32 pmc_scratch294;
/* 0x9bc */	u32 pmc_scratch295;
/* 0x9c0 */	u32 pmc_scratch296;
/* 0x9c4 */	u32 pmc_scratch297;
/* 0x9c8 */	u32 pmc_scratch298;
/* 0x9cc */	u32 pmc_scratch299;
/* 0x9d0 */	u32 rsvd_9d0[30];
/* 0xa48 */	u32 pmc_scratch_write_disable0_b01;
/* 0xa4c */	u32 pmc_scratch_write_disable1_b01;
/* 0xa50 */	u32 pmc_scratch_write_disable2_b01;
/* 0xa54 */	u32 pmc_scratch_write_disable3_b01;
/* 0xa58 */	u32 pmc_scratch_write_disable4_b01;
/* 0xa5c */	u32 pmc_scratch_write_disable5_b01;
/* 0xa60 */	u32 pmc_scratch_write_disable6_b01;
/* 0xa64 */	u32 pmc_scratch_write_disable7_b01;
/* 0xa68 */	u32 pmc_scratch_write_disable8_b01;
/* 0xa6c */	u32 pmc_scratch_write_disable9_b01;
/* 0xa70 */	u32 pmc_scratch_write_disable10_b01;
/* 0xa74 */	u32 pmc_scratch_write_lock_disable_sticky_b01;
/* 0xa78 */	u32 rsvd_a78[8];
/* 0xa98 */	u32 pmc_secure_scratch80;
/* 0xa9c */	u32 pmc_secure_scratch81;
/* 0xaa0 */	u32 pmc_secure_scratch82;
/* 0xaa4 */	u32 pmc_secure_scratch83;
/* 0xaa8 */	u32 pmc_secure_scratch84;
/* 0xaac */	u32 pmc_secure_scratch85;
/* 0xab0 */	u32 pmc_secure_scratch86;
/* 0xab4 */	u32 pmc_secure_scratch87;
/* 0xab8 */	u32 pmc_secure_scratch88;
/* 0xabc */	u32 pmc_secure_scratch89;
/* 0xac0 */	u32 pmc_secure_scratch90;
/* 0xac4 */	u32 pmc_secure_scratch91;
/* 0xac8 */	u32 pmc_secure_scratch92;
/* 0xacc */	u32 pmc_secure_scratch93;
/* 0xad0 */	u32 pmc_secure_scratch94;
/* 0xad4 */	u32 pmc_secure_scratch95;
/* 0xad8 */	u32 pmc_secure_scratch96;
/* 0xadc */	u32 pmc_secure_scratch97;
/* 0xae0 */	u32 pmc_secure_scratch98;
/* 0xae4 */	u32 pmc_secure_scratch99;
/* 0xae8 */	u32 pmc_secure_scratch100;
/* 0xaec */	u32 pmc_secure_scratch101;
/* 0xaf0 */	u32 pmc_secure_scratch102;
/* 0xaf4 */	u32 pmc_secure_scratch103;
/* 0xaf8 */	u32 pmc_secure_scratch104;
/* 0xafc */	u32 pmc_secure_scratch105;
/* 0xb00 */	u32 pmc_secure_scratch106;
/* 0xb04 */	u32 pmc_secure_scratch107;
/* 0xb08 */	u32 pmc_secure_scratch108;
/* 0xb0c */	u32 pmc_secure_scratch109;
/* 0xb10 */	u32 pmc_secure_scratch110;
/* 0xb14 */	u32 pmc_secure_scratch111;
/* 0xb18 */	u32 pmc_secure_scratch112;
/* 0xb1c */	u32 pmc_secure_scratch113;
/* 0xb20 */	u32 pmc_secure_scratch114;
/* 0xb24 */	u32 pmc_secure_scratch115;
/* 0xb28 */	u32 pmc_secure_scratch116;
/* 0xb2c */	u32 pmc_secure_scratch117;
/* 0xb30 */	u32 pmc_secure_scratch118;
/* 0xb34 */	u32 pmc_secure_scratch119;
/* 0xb38 */	u32 pmc_secure_scratch120_b01;
/* 0xb3c */	u32 pmc_secure_scratch121_b01;
/* 0xb40 */	u32 pmc_secure_scratch122_b01;
/* 0xb44 */	u32 pmc_secure_scratch123_b01;
/* 0xb48 */	u32 pmc_led_breathing_ctrl_b01;
/* 0xb4c */	u32 pmc_led_breathing_counter0_b01; // Slope Steps.
/* 0xb50 */	u32 pmc_led_breathing_counter1_b01; // ON counter.
/* 0xb54 */	u32 pmc_led_breathing_counter2_b01; // OFF counter1.
/* 0xb58 */	u32 pmc_led_breathing_counter3_b01; // OFF counter0.
/* 0xb5c */	u32 pmc_led_breathing_status_b01;
/* 0xb60 */	u32 rsvd_b60[2];
/* 0xb68 */	u32 pmc_secure_scratch124_b01;
/* 0xb6c */	u32 pmc_secure_scratch125_b01;
/* 0xb70 */	u32 pmc_secure_scratch126_b01;
/* 0xb74 */	u32 pmc_secure_scratch127_b01;
/* 0xb78 */	u32 pmc_secure_scratch128_b01;
/* 0xb7c */	u32 pmc_secure_scratch129_b01;
/* 0xb80 */	u32 pmc_secure_scratch130_b01;
/* 0xb84 */	u32 pmc_secure_scratch131_b01;
/* 0xb88 */	u32 pmc_secure_scratch132_b01;
/* 0xb8c */	u32 pmc_secure_scratch133_b01;
/* 0xb90 */	u32 pmc_secure_scratch134_b01;
/* 0xb94 */	u32 pmc_secure_scratch135_b01;
/* 0xb98 */	u32 pmc_secure_scratch136_b01;
/* 0xb9c */	u32 pmc_secure_scratch137_b01;
/* 0xba0 */	u32 pmc_secure_scratch138_b01;
/* 0xba4 */	u32 pmc_secure_scratch139_b01;
/* 0xba8 */	u32 rsvd_ba8[2];
/* 0xbb0 */	u32 pmc_sec_disable_ns_b01;
/* 0xbb4 */	u32 pmc_sec_disable2_ns_b01;
/* 0xbb8 */	u32 pmc_sec_disable3_ns_b01;
/* 0xbbc */	u32 pmc_sec_disable4_ns_b01;
/* 0xbc0 */	u32 pmc_sec_disable5_ns_b01;
/* 0xbc4 */	u32 pmc_sec_disable6_ns_b01;
/* 0xbc8 */	u32 pmc_sec_disable7_ns_b01;
/* 0xbcc */	u32 pmc_sec_disable8_ns_b01;
/* 0xbd0 */	u32 pmc_sec_disable9_ns_b01;
/* 0xbd4 */	u32 pmc_sec_disable10_ns_b01;
/* 0xbd8 */	u32 rsvd_bd8[4];
/* 0xbe8 */	u32 pmc_tzram_pwr_cntrl_b01;
/* 0xbec */	u32 pmc_tzram_sec_disable_b01;
/* 0xbf0 */	u32 pmc_tzram_non_sec_disable_b01;
} pmc_regs_t210_t;


#endif	/* _PMC_T210_H_ */
