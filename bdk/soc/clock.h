/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 CTCaer
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

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <utils/types.h>

/*! Clock registers. */
#define CLK_RST_CONTROLLER_RST_SOURCE 0x0
#define CLK_RST_CONTROLLER_RST_DEVICES_L 0x4
#define CLK_RST_CONTROLLER_RST_DEVICES_H 0x8
#define CLK_RST_CONTROLLER_RST_DEVICES_U 0xC
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_L 0x10
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_H 0x14
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_U 0x18
#define CLK_RST_CONTROLLER_CCLK_BURST_POLICY 0x20
#define CLK_RST_CONTROLLER_SUPER_CCLK_DIVIDER 0x24
#define CLK_RST_CONTROLLER_SCLK_BURST_POLICY 0x28
#define CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER 0x2C
#define CLK_RST_CONTROLLER_CLK_SYSTEM_RATE 0x30
#define CLK_RST_CONTROLLER_MISC_CLK_ENB 0x48
#define CLK_RST_CONTROLLER_OSC_CTRL 0x50
#define CLK_RST_CONTROLLER_OSC_FREQ_DET 0x58
#define CLK_RST_CONTROLLER_OSC_FREQ_DET_STATUS 0x5C
#define CLK_RST_CONTROLLER_PTO_CLK_CNT_CNTL 0x60
#define CLK_RST_CONTROLLER_PTO_CLK_CNT_STATUS 0x64
#define CLK_RST_CONTROLLER_PLLC_BASE 0x80
#define CLK_RST_CONTROLLER_PLLC_OUT 0x84
#define CLK_RST_CONTROLLER_PLLC_MISC 0x88
#define CLK_RST_CONTROLLER_PLLC_MISC_1 0x8C
#define CLK_RST_CONTROLLER_PLLM_BASE 0x90
#define CLK_RST_CONTROLLER_PLLM_MISC1 0x98
#define CLK_RST_CONTROLLER_PLLM_MISC2 0x9C
#define CLK_RST_CONTROLLER_PLLP_BASE 0xA0
#define CLK_RST_CONTROLLER_PLLA_BASE 0xB0
#define CLK_RST_CONTROLLER_PLLA_OUT 0xB4
#define CLK_RST_CONTROLLER_PLLA_MISC1 0xB8
#define CLK_RST_CONTROLLER_PLLA_MISC 0xBC
#define CLK_RST_CONTROLLER_PLLU_BASE 0xC0
#define CLK_RST_CONTROLLER_PLLU_OUTA 0xC4
#define CLK_RST_CONTROLLER_PLLU_MISC 0xCC
#define CLK_RST_CONTROLLER_PLLD_BASE 0xD0
#define CLK_RST_CONTROLLER_PLLD_MISC1 0xD8
#define CLK_RST_CONTROLLER_PLLD_MISC 0xDC
#define CLK_RST_CONTROLLER_PLLX_BASE 0xE0
#define CLK_RST_CONTROLLER_PLLX_MISC 0xE4
#define CLK_RST_CONTROLLER_PLLE_BASE 0xE8
#define CLK_RST_CONTROLLER_PLLE_MISC 0xEC
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA 0xF8
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB 0xFC
#define CLK_RST_CONTROLLER_CLK_SOURCE_I2S2 0x100
#define CLK_RST_CONTROLLER_CLK_SOURCE_PWM 0x110
#define CLK_RST_CONTROLLER_CLK_SOURCE_I2C1 0x124
#define CLK_RST_CONTROLLER_CLK_SOURCE_I2C5 0x128
#define CLK_RST_CONTROLLER_CLK_SOURCE_DISP1 0x138
#define CLK_RST_CONTROLLER_CLK_SOURCE_VI 0x148
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1 0x150
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC2 0x154
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4 0x164
#define CLK_RST_CONTROLLER_CLK_SOURCE_UARTA 0x178
#define CLK_RST_CONTROLLER_CLK_SOURCE_UARTB 0x17C
#define CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X 0x180
#define CLK_RST_CONTROLLER_CLK_SOURCE_I2C2 0x198
#define CLK_RST_CONTROLLER_CLK_SOURCE_EMC 0x19C
#define CLK_RST_CONTROLLER_CLK_SOURCE_UARTC 0x1A0
#define CLK_RST_CONTROLLER_CLK_SOURCE_I2C3 0x1B8
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3 0x1BC
#define CLK_RST_CONTROLLER_CLK_SOURCE_UARTD 0x1C0
#define CLK_RST_CONTROLLER_CLK_SOURCE_CSITE 0x1D4
#define CLK_RST_CONTROLLER_CLK_SOURCE_I2S1 0x1D8
#define CLK_RST_CONTROLLER_CLK_SOURCE_TSEC 0x1F4
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_X 0x280
#define CLK_RST_CONTROLLER_CLK_ENB_X_SET 0x284
#define CLK_RST_CONTROLLER_CLK_ENB_X_CLR 0x288
#define CLK_RST_CONTROLLER_RST_DEVICES_X 0x28C
#define CLK_RST_CONTROLLER_RST_DEV_X_SET 0x290
#define CLK_RST_CONTROLLER_RST_DEV_X_CLR 0x294
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_Y 0x298
#define CLK_RST_CONTROLLER_CLK_ENB_Y_SET 0x29C
#define CLK_RST_CONTROLLER_CLK_ENB_Y_CLR 0x2A0
#define CLK_RST_CONTROLLER_RST_DEVICES_Y 0x2A4
#define CLK_RST_CONTROLLER_RST_DEV_Y_SET 0x2A8
#define CLK_RST_CONTROLLER_RST_DEV_Y_CLR 0x2AC
#define CLK_RST_CONTROLLER_RST_DEV_L_SET 0x300
#define CLK_RST_CONTROLLER_RST_DEV_L_CLR 0x304
#define CLK_RST_CONTROLLER_RST_DEV_H_SET 0x308
#define CLK_RST_CONTROLLER_RST_DEV_H_CLR 0x30C
#define CLK_RST_CONTROLLER_RST_DEV_U_SET 0x310
#define CLK_RST_CONTROLLER_RST_DEV_U_CLR 0x314
#define CLK_RST_CONTROLLER_CLK_ENB_L_SET 0x320
#define CLK_RST_CONTROLLER_CLK_ENB_L_CLR 0x324
#define CLK_RST_CONTROLLER_CLK_ENB_H_SET 0x328
#define CLK_RST_CONTROLLER_CLK_ENB_H_CLR 0x32C
#define CLK_RST_CONTROLLER_CLK_ENB_U_SET 0x330
#define CLK_RST_CONTROLLER_CLK_ENB_U_CLR 0x334
#define CLK_RST_CONTROLLER_RST_DEVICES_V 0x358
#define CLK_RST_CONTROLLER_RST_DEVICES_W 0x35C
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_V 0x360
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_W 0x364
#define CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2 0x388
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC 0x3A0
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD 0x3A4
#define CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT 0x3B4
#define CLK_RST_CONTROLLER_CLK_SOURCE_I2C4 0x3C4
#define CLK_RST_CONTROLLER_CLK_SOURCE_EXTPERIPH1 0x3EC
#define CLK_RST_CONTROLLER_CLK_SOURCE_SYS 0x400
#define CLK_RST_CONTROLLER_CLK_SOURCE_SOR1 0x410
#define CLK_RST_CONTROLLER_CLK_SOURCE_SE 0x42C
#define CLK_RST_CONTROLLER_RST_DEV_V_SET 0x430
#define CLK_RST_CONTROLLER_RST_DEV_V_CLR 0x434
#define CLK_RST_CONTROLLER_RST_DEV_W_SET 0x438
#define CLK_RST_CONTROLLER_RST_DEV_W_CLR 0x43C
#define CLK_RST_CONTROLLER_CLK_ENB_V_SET 0x440
#define CLK_RST_CONTROLLER_CLK_ENB_V_CLR 0x444
#define CLK_RST_CONTROLLER_CLK_ENB_W_SET 0x448
#define CLK_RST_CONTROLLER_CLK_ENB_W_CLR 0x44C
#define CLK_RST_CONTROLLER_RST_CPUG_CMPLX_SET 0x450
#define CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR 0x454
#define CLK_RST_CONTROLLER_UTMIP_PLL_CFG0 0x480
#define CLK_RST_CONTROLLER_UTMIP_PLL_CFG1 0x484
#define CLK_RST_CONTROLLER_UTMIP_PLL_CFG2 0x488
#define CLK_RST_CONTROLLER_PLLE_AUX 0x48C
#define CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S0 0x4A0
#define CLK_RST_CONTROLLER_UTMIP_PLL_CFG3 0x4C0
#define CLK_RST_CONTROLLER_PLLX_MISC_3 0x518
#define CLK_RST_CONTROLLER_UTMIPLL_HW_PWRDN_CFG0 0x52C
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE 0x554
#define CLK_RST_CONTROLLER_SPARE_REG0 0x55C
#define CLK_RST_CONTROLLER_PLLC4_BASE 0x5A4
#define CLK_RST_CONTROLLER_PLLC4_MISC 0x5A8
#define CLK_RST_CONTROLLER_PLLC_MISC_2 0x5D0
#define CLK_RST_CONTROLLER_PLLC4_OUT 0x5E4
#define CLK_RST_CONTROLLER_PLLMB_BASE 0x5E8
#define CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_FS 0x608
#define CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_CORE_DEV 0x60C
#define CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_SS 0x610
#define CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP 0x620
#define CLK_RST_CONTROLLER_CLK_SOURCE_I2C6 0x65C
#define CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL 0x664
#define CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL 0x66C
#define CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC_LEGACY_TM 0x694
#define CLK_RST_CONTROLLER_CLK_SOURCE_NVENC 0x6A0
#define CLK_RST_CONTROLLER_CLK_SOURCE_USB2_HSIC_TRK 0x6CC
#define CLK_RST_CONTROLLER_SE_SUPER_CLK_DIVIDER 0x704
#define CLK_RST_CONTROLLER_CLK_SOURCE_UARTAPE 0x710

#define CLK_NO_SOURCE 0x0
#define CLK_NOT_USED  0x0

/*! PLL control and status bits */
#define PLLX_BASE_LOCK       BIT(27)
#define PLLX_BASE_REF_DIS    BIT(29)
#define PLLX_BASE_ENABLE     BIT(30)
#define PLLX_BASE_BYPASS     BIT(31)
#define PLLX_MISC_LOCK_EN    BIT(18)
#define PLLX_MISC3_IDDQ      BIT(3)

#define PLLCX_BASE_LOCK      BIT(27)
#define PLLCX_BASE_REF_DIS   BIT(29)
#define PLLCX_BASE_ENABLE    BIT(30)
#define PLLCX_BASE_BYPASS    BIT(31)

#define PLLA_OUT0_RSTN_CLR   BIT(0)
#define PLLA_OUT0_CLKEN      BIT(1)
#define PLLA_BASE_IDDQ       BIT(25)

#define PLLC_OUT1_RSTN_CLR   BIT(0)
#define PLLC_OUT1_CLKEN      BIT(1)
#define PLLC_MISC1_IDDQ      BIT(27)
#define PLLC_MISC_RESET      BIT(30)

#define PLLC4_OUT3_RSTN_CLR  BIT(0)
#define PLLC4_OUT3_CLKEN     BIT(1)
#define PLLC4_BASE_IDDQ      BIT(18)
#define PLLC4_MISC_EN_LCKDET BIT(30)

#define UTMIPLL_LOCK         BIT(31)

/*! PTO_CLK_CNT */
#define PTO_REF_CLK_WIN_CFG_MASK 0xF
#define PTO_REF_CLK_WIN_CFG_16P  0xF
#define PTO_CNT_EN               BIT(9)
#define PTO_CNT_RST              BIT(10)
#define PTO_CLK_ENABLE           BIT(13)
#define PTO_SRC_SEL_SHIFT        14
#define PTO_SRC_SEL_MASK         0x1FF
#define PTO_DIV_SEL_MASK         (3 << 23)
#define PTO_DIV_SEL_GATED        (0 << 23)
#define PTO_DIV_SEL_DIV1         (1 << 23)
#define PTO_DIV_SEL_DIV2_RISING  (2 << 23)
#define PTO_DIV_SEL_DIV2_FALLING (3 << 23)
#define PTO_DIV_SEL_CPU_EARLY    (0 << 23)
#define PTO_DIV_SEL_CPU_LATE     (1 << 23)

#define PTO_CLK_CNT_BUSY         BIT(31)
#define PTO_CLK_CNT              0xFFFFFF

/*! OSC_FREQ_DET */
#define OSC_REF_CLK_WIN_CFG_MASK 0xF
#define OSC_FREQ_DET_TRIG        BIT(31)

#define OSC_FREQ_DET_BUSY        BIT(31)
#define OSC_FREQ_DET_CNT         0xFFFF

/*! PTO IDs. */
typedef enum _clock_pto_id_t
{
	CLK_PTO_PCLK_SYS =          0x06,
	CLK_PTO_HCLK_SYS =          0x07,

	CLK_PTO_UTMIP_240 =         0x0C,

	CLK_PTO_CCLK_G =            0x12,
	CLK_PTO_CCLK_G_DIV2 =       0x13,

	CLK_PTO_SPI1 =              0x17,
	CLK_PTO_SPI2 =              0x18,
	CLK_PTO_SPI3 =              0x19,
	CLK_PTO_SPI4 =              0x1A,
	CLK_PTO_MAUD =              0x1B,
	CLK_PTO_SCLK =              0x1C,

	CLK_PTO_SDMMC1 =            0x20,
	CLK_PTO_SDMMC2 =            0x21,
	CLK_PTO_SDMMC3 =            0x22,
	CLK_PTO_SDMMC4 =            0x23,
	CLK_PTO_EMC =               0x24,

	CLK_PTO_CCLK_LP =           0x2B,
	CLK_PTO_CCLK_LP_DIV2 =      0x2C,

	CLK_PTO_MSELECT =           0x2F,

	CLK_PTO_VIC =               0x36,

	CLK_PTO_NVDEC =             0x39,

	CLK_PTO_NVENC =             0x3A,
	CLK_PTO_NVJPG =             0x3B,
	CLK_PTO_TSEC =              0x3C,
	CLK_PTO_TSECB =             0x3D,
	CLK_PTO_SE =                0x3E,

	CLK_PTO_DSIA_LP =           0x62,

	CLK_PTO_ISP =               0x64,
	CLK_PTO_MC =                0x6A,

	CLK_PTO_ACTMON =            0x6B,
	CLK_PTO_CSITE =             0x6C,

	CLK_PTO_HOST1X =            0x6F,

	CLK_PTO_SE_2 =              0x74, // Same as CLK_PTO_SE.
	CLK_PTO_SOC_THERM =         0x75,

	CLK_PTO_TSEC_2 =            0x77, // Same as CLK_PTO_TSEC.

	CLK_PTO_ACLK =              0x7C,
	CLK_PTO_QSPI =              0x7D,

	CLK_PTO_I2S1 =              0x80,
	CLK_PTO_I2S2 =              0x81,
	CLK_PTO_I2S3 =              0x82,
	CLK_PTO_I2S4 =              0x83,
	CLK_PTO_I2S5 =              0x84,
	CLK_PTO_AHUB =              0x85,
	CLK_PTO_APE =               0x86,

	CLK_PTO_DVFS_SOC =          0x88,
	CLK_PTO_DVFS_REF =          0x89,

	CLK_PTO_SPDIF =             0x8F,
	CLK_PTO_SPDIF_IN =          0x90,
	CLK_PTO_UART_FST_MIPI_CAL = 0x91,

	CLK_PTO_PWM =               0x93,
	CLK_PTO_I2C1 =              0x94,
	CLK_PTO_I2C2 =              0x95,
	CLK_PTO_I2C3 =              0x96,
	CLK_PTO_I2C4 =              0x97,
	CLK_PTO_I2C5 =              0x98,
	CLK_PTO_I2C6 =              0x99,
	CLK_PTO_I2C_SLOW =          0x9A,
	CLK_PTO_UARTAPE =           0x9B,

	CLK_PTO_EXTPERIPH1 =        0x9D,
	CLK_PTO_EXTPERIPH2 =        0x9E,

	CLK_PTO_ENTROPY =           0xA0,
	CLK_PTO_UARTA =             0xA1,
	CLK_PTO_UARTB =             0xA2,
	CLK_PTO_UARTC =             0xA3,
	CLK_PTO_UARTD =             0xA4,
	CLK_PTO_OWR =               0xA5,

	CLK_PTO_HDA2CODEC_2X =      0xA7,
	CLK_PTO_HDA =               0xA8,

	CLK_PTO_SDMMC_LEGACY_TM =   0xAB,

	CLK_PTO_SOR0 =              0xC0,
	CLK_PTO_SOR1 =              0xC1,

	CLK_PTO_DISP2 =             0xC4,
	CLK_PTO_DISP1 =             0xC5,

	CLK_PTO_XUSB_FALCON =       0x110,

	CLK_PTO_XUSB_FS =           0x136,
	CLK_PTO_XUSB_SS_HOST_DEV =  0x137,
	CLK_PTO_XUSB_CORE_HOST =    0x138,
	CLK_PTO_XUSB_CORE_DEV =     0x139,

	/*
	 * PLL need PTO enabled in MISC registers.
	 * Normal div is 2 so result is multiplied with it.
	 */
	CLK_PTO_PLLC_DIV2 =              0x01,
	CLK_PTO_PLLM_DIV2 =              0x02,
	CLK_PTO_PLLP_DIV2 =              0x03,
	CLK_PTO_PLLA_DIV2 =              0x04,
	CLK_PTO_PLLX_DIV2 =              0x05,

	CLK_PTO_PLLMB_DIV2 =             0x25,

	CLK_PTO_PLLC4_DIV2 =             0x51,

	CLK_PTO_PLLA1_DIV2 =             0x55,
	CLK_PTO_PLLC2_DIV2 =             0x58,
	CLK_PTO_PLLC3_DIV2 =             0x5A,

	CLK_PTO_PLLD_DIV2 =              0xCB,
	CLK_PTO_PLLD2_DIV2 =             0xCD,
	CLK_PTO_PLLDP_DIV2 =             0xCF,

	CLK_PTO_PLLU_DIV2 =              0x10D,

	CLK_PTO_PLLREFE_DIV2 =           0x10F,
} clock_pto_id_t;

/*
 * CLOCK Peripherals:
 * L   0 -  31
 * H  32 -  63
 * U  64 -  95
 * V  96 - 127
 * W 128 - 159
 * X 160 - 191
 * Y 192 - 223
 */

enum CLK_L_DEV
{
	CLK_L_CPU     = 0, // Only reset. Deprecated.
	CLK_L_BPMP    = 1, // Only reset.
	CLK_L_SYS     = 2, // Only reset.
	CLK_L_ISPB    = 3,
	CLK_L_RTC     = 4,
	CLK_L_TMR     = 5,
	CLK_L_UARTA   = 6,
	CLK_L_UARTB   = 7,
	CLK_L_GPIO    = 8,
	CLK_L_SDMMC2  = 9,
	CLK_L_SPDIF   = 10,
	CLK_L_I2S2    = 11, // I2S1
	CLK_L_I2C1    = 12,
	CLK_L_NDFLASH = 13, // HIDDEN.
	CLK_L_SDMMC1  = 14,
	CLK_L_SDMMC4  = 15,
	CLK_L_TWC     = 16, // HIDDEN.
	CLK_L_PWM     = 17,
	CLK_L_I2S3    = 18,
	CLK_L_EPP     = 19, // HIDDEN.
	CLK_L_VI      = 20,
	CLK_L_2D      = 21, // HIDDEN.
	CLK_L_USBD    = 22,
	CLK_L_ISP     = 23,
	CLK_L_3D      = 24, // HIDDEN.
	CLK_L_IDE     = 25, // RESERVED.
	CLK_L_DISP2   = 26,
	CLK_L_DISP1   = 27,
	CLK_L_HOST1X  = 28,
	CLK_L_VCP     = 29, // HIDDEN.
	CLK_L_I2S1    = 30, // I2S0
	CLK_L_BPMP_CACHE_CTRL = 31, // CONTROLLER
};

enum CLK_H_DEV
{
	CLK_H_MEM      = 0, // MC.
	CLK_H_AHBDMA   = 1,
	CLK_H_APBDMA   = 2,
	//CLK_H_       = 3,
	CLK_H_KBC      = 4, // HIDDEN.
	CLK_H_STAT_MON = 5,
	CLK_H_PMC      = 6,
	CLK_H_FUSE     = 7,
	CLK_H_KFUSE    = 8,
	CLK_H_SPI1     = 9,
	CLK_H_SNOR     = 10, // HIDDEN.
	CLK_H_JTAG2TBC = 11,
	CLK_H_SPI2     = 12,
	CLK_H_XIO      = 13, // HIDDEN.
	CLK_H_SPI3     = 14,
	CLK_H_I2C5     = 15,
	CLK_H_DSI      = 16,
	CLK_H_TVO      = 17, // RESERVED.
	CLK_H_HSI      = 18, // HIDDEN.
	CLK_H_HDMI     = 19, // HIDDEN.
	CLK_H_CSI      = 20,
	CLK_H_TVDAC    = 21, // RESERVED.
	CLK_H_I2C2     = 22,
	CLK_H_UARTC    = 23,
	CLK_H_MIPI_CAL = 24,
	CLK_H_EMC      = 25,
	CLK_H_USB2     = 26,
	CLK_H_USB3     = 27, // HIDDEN.
	CLK_H_MPE      = 28, // HIDDEN.
	CLK_H_VDE      = 29, // HIDDEN.
	CLK_H_BSEA     = 30, // HIDDEN.
	CLK_H_BSEV     = 31,
};

enum CLK_U_DEV
{
	CLK_U_SPEEDO         = 0, // RESERVED.
	CLK_U_UARTD          = 1,
	CLK_U_UARTE          = 2, // HIDDEN.
	CLK_U_I2C3           = 3,
	CLK_U_SPI4           = 4,
	CLK_U_SDMMC3         = 5,
	CLK_U_PCIE           = 6,
	CLK_U_OWR            = 7, // RESERVED.
	CLK_U_AFI            = 8,
	CLK_U_CSITE          = 9,
	CLK_U_PCIEXCLK       = 10, // Only reset.
	CLK_U_BPMPUCQ        = 11, // HIDDEN.
	CLK_U_LA             = 12,
	CLK_U_TRACECLKIN     = 13, // HIDDEN.
	CLK_U_SOC_THERM      = 14,
	CLK_U_DTV            = 15,
	CLK_U_NAND_SPEED     = 16, // HIDDEN.
	CLK_U_I2C_SLOW       = 17,
	CLK_U_DSIB           = 18,
	CLK_U_TSEC           = 19,
	CLK_U_IRAMA          = 20,
	CLK_U_IRAMB          = 21,
	CLK_U_IRAMC          = 22,
	CLK_U_IRAMD          = 23, // EMUCIF ON RESET
	CLK_U_BPMP_CACHE_RAM = 24,
	CLK_U_XUSB_HOST      = 25,
	CLK_U_CLK_M_DOUBLER  = 26,
	CLK_U_MSENC          = 27, // HIDDEN.
	CLK_U_SUS_OUT        = 28,
	CLK_U_DEV2_OUT       = 29,
	CLK_U_DEV1_OUT       = 30,
	CLK_U_XUSB_DEV       = 31,
};

enum CLK_V_DEV
{
	CLK_V_CPUG          = 0,
	CLK_V_CPULP         = 1, // Reserved.
	CLK_V_3D2           = 2, // HIDDEN.
	CLK_V_MSELECT       = 3,
	CLK_V_TSENSOR       = 4,
	CLK_V_I2S4          = 5,
	CLK_V_I2S5          = 6,
	CLK_V_I2C4          = 7,
	CLK_V_SPI5          = 8, // HIDDEN.
	CLK_V_SPI6          = 9, // HIDDEN.
	CLK_V_AHUB          = 10, // AUDIO.
	CLK_V_APB2APE       = 11, // APBIF.
	CLK_V_DAM0          = 12, // HIDDEN.
	CLK_V_DAM1          = 13, // HIDDEN.
	CLK_V_DAM2          = 14, // HIDDEN.
	CLK_V_HDA2CODEC_2X  = 15,
	CLK_V_ATOMICS       = 16,
	//CLK_V_            = 17,
	//CLK_V_            = 18,
	//CLK_V_            = 19,
	//CLK_V_            = 20,
	//CLK_V_            = 21,
	CLK_V_SPDIF_DOUBLER = 22,
	CLK_V_ACTMON        = 23,
	CLK_V_EXTPERIPH1    = 24,
	CLK_V_EXTPERIPH2    = 25,
	CLK_V_EXTPERIPH3    = 26,
	CLK_V_SATA_OOB      = 27,
	CLK_V_SATA          = 28,
	CLK_V_HDA           = 29,
	CLK_V_TZRAM         = 30, // HIDDEN.
	CLK_V_SE            = 31, // HIDDEN.
};

enum CLK_W_DEV
{
	CLK_W_HDA2HDMICODEC = 0,
	CLK_W_RESERVED0     = 1, //satacoldrstn
	CLK_W_PCIERX0       = 2,
	CLK_W_PCIERX1       = 3,
	CLK_W_PCIERX2       = 4,
	CLK_W_PCIERX3       = 5,
	CLK_W_PCIERX4       = 6,
	CLK_W_PCIERX5       = 7,
	CLK_W_CEC           = 8,
	CLK_W_PCIE2_IOBIST  = 9,
	CLK_W_EMC_IOBIST    = 10,
	CLK_W_HDMI_IOBIST   = 11, // HIDDEN.
	CLK_W_SATA_IOBIST   = 12,
	CLK_W_MIPI_IOBIST   = 13,
	CLK_W_XUSB_PADCTL   = 14, // Only reset.
	CLK_W_XUSB          = 15,
	CLK_W_CILAB         = 16,
	CLK_W_CILCD         = 17,
	CLK_W_CILEF         = 18,
	CLK_W_DSIA_LP       = 19,
	CLK_W_DSIB_LP       = 20,
	CLK_W_ENTROPY       = 21,
	CLK_W_DDS           = 22, // HIDDEN.
	//CLK_W_            = 23,
	CLK_W_DP2           = 24, // HIDDEN.
	CLK_W_AMX0          = 25, // HIDDEN.
	CLK_W_ADX0          = 26, // HIDDEN.
	CLK_W_DVFS          = 27,
	CLK_W_XUSB_SS       = 28,
	CLK_W_EMC_LATENCY   = 29,
	CLK_W_MC1           = 30,
	//CLK_W_            = 31,
};

enum CLK_X_DEV
{
	CLK_X_SPARE             = 0,
	CLK_X_DMIC1             = 1,
	CLK_X_DMIC2             = 2,
	CLK_X_ETR               = 3,
	CLK_X_CAM_MCLK          = 4,
	CLK_X_CAM_MCLK2         = 5,
	CLK_X_I2C6              = 6,
	CLK_X_MC_CAPA           = 7, // MC DAISY CHAIN1
	CLK_X_MC_CBPA           = 8, // MC DAISY CHAIN2
	CLK_X_MC_CPU            = 9,
	CLK_X_MC_BBC            = 10,
	CLK_X_VIM2_CLK          = 11,
	//CLK_X_                = 12,
	CLK_X_MIPIBIF           = 13, //RESERVED
	CLK_X_EMC_DLL           = 14,
	//CLK_X_                = 15,
	CLK_X_HDMI_AUDIO        = 16, // HIDDEN.
	CLK_X_UART_FST_MIPI_CAL = 17,
	CLK_X_VIC               = 18,
	//CLK_X_                = 19,
	CLK_X_ADX1              = 20, // HIDDEN.
	CLK_X_DPAUX             = 21,
	CLK_X_SOR0              = 22,
	CLK_X_SOR1              = 23,
	CLK_X_GPU               = 24,
	CLK_X_DBGAPB            = 25,
	CLK_X_HPLL_ADSP         = 26,
	CLK_X_PLLP_ADSP         = 27,
	CLK_X_PLLA_ADSP         = 28,
	CLK_X_PLLG_REF          = 29,
	//CLK_X_ = 30,
	//CLK_X_ = 31,
};

enum CLK_Y_DEV
{
	CLK_Y_SPARE1          = 0,
	CLK_Y_SDMMC_LEGACY_TM = 1,
	CLK_Y_NVDEC           = 2,
	CLK_Y_NVJPG           = 3,
	CLK_Y_AXIAP           = 4,
	CLK_Y_DMIC3           = 5,
	CLK_Y_APE             = 6,
	CLK_Y_ADSP            = 7,
	CLK_Y_MC_CDPA         = 8, // MC DAISY CHAIN4
	CLK_Y_MC_CCPA         = 9, // MC DAISY CHAIN3
	CLK_Y_MAUD            = 10,
	//CLK_Y_              = 11,
	CLK_Y_SATA_USB_UPHY   = 12, // Only reset.
	CLK_Y_PEX_USB_UPHY    = 13, // Only reset.
	CLK_Y_TSECB           = 14,
	CLK_Y_DPAUX1          = 15,
	CLK_Y_VI_I2C          = 16,
	CLK_Y_HSIC_TRK        = 17,
	CLK_Y_USB2_TRK        = 18,
	CLK_Y_QSPI            = 19,
	CLK_Y_UARTAPE         = 20,
	CLK_Y_ADSPINTF        = 21, // Only reset.
	CLK_Y_ADSPPERIPH      = 22, // Only reset.
	CLK_Y_ADSPDBG         = 23, // Only reset.
	CLK_Y_ADSPWDT         = 24, // Only reset.
	CLK_Y_ADSPSCU         = 25, // Only reset.
	CLK_Y_ADSPNEON        = 26,
	CLK_Y_NVENC           = 27,
	CLK_Y_IQC2            = 28,
	CLK_Y_IQC1            = 29,
	CLK_Y_SOR_SAFE        = 30,
	CLK_Y_PLLP_OUT_CPU    = 31,
};

/*! Generic clock descriptor. */
typedef struct _clock_t
{
	u16 reset;
	u16 enable;
	u16 source;
	u8 index;
	u8 clk_src;
	u8 clk_div;
} clock_t;

/*! Generic clock enable/disable. */
void clock_enable(const clock_t *clk);
void clock_disable(const clock_t *clk);

/*! Clock control for specific hardware portions. */
void clock_enable_fuse(bool enable);
void clock_enable_uart(u32 idx);
void clock_disable_uart(u32 idx);
int  clock_uart_use_src_div(u32 idx, u32 baud);
void clock_enable_i2c(u32 idx);
void clock_disable_i2c(u32 idx);
void clock_enable_se();
void clock_enable_tzram();
void clock_enable_host1x();
void clock_disable_host1x();
void clock_enable_tsec();
void clock_disable_tsec();
void clock_enable_sor_safe();
void clock_disable_sor_safe();
void clock_enable_sor0();
void clock_disable_sor0();
void clock_enable_sor1();
void clock_disable_sor1();
void clock_enable_kfuse();
void clock_disable_kfuse();
void clock_enable_cl_dvfs();
void clock_disable_cl_dvfs();
void clock_enable_coresight();
void clock_disable_coresight();
void clock_enable_pwm();
void clock_disable_pwm();
void clock_enable_pllx();
void clock_enable_pllc(u32 divn);
void clock_disable_pllc();
void clock_enable_pllu();
void clock_disable_pllu();
void clock_enable_utmipll();
void clock_sdmmc_config_clock_source(u32 *pclock, u32 id, u32 val);
void clock_sdmmc_get_card_clock_div(u32 *pclock, u16 *pdivisor, u32 type);
int  clock_sdmmc_is_not_reset_and_enabled(u32 id);
void clock_sdmmc_enable(u32 id, u32 val);
void clock_sdmmc_disable(u32 id);

u32 clock_get_osc_freq();
u32 clock_get_dev_freq(clock_pto_id_t id);

#endif
