/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 st4rk
 * Copyright (c) 2018-2024 CTCaer
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

#ifndef _PMC_H_
#define _PMC_H_

#include <utils/types.h>

/*! PMC registers. */
#define APBDEV_PMC_CNTRL             0x0
#define  PMC_CNTRL_RTC_CLK_DIS             BIT(1)
#define  PMC_CNTRL_RTC_RST                 BIT(2)
#define  PMC_CNTRL_MAIN_RST                BIT(4)
#define  PMC_CNTRL_LATCHWAKE_EN            BIT(5)
#define  PMC_CNTRL_BLINK_EN                BIT(7)
#define  PMC_CNTRL_PWRREQ_OE               BIT(9)
#define  PMC_CNTRL_SYSCLK_OE               BIT(11)
#define  PMC_CNTRL_PWRGATE_DIS             BIT(12)
#define  PMC_CNTRL_SIDE_EFFECT_LP0         BIT(14)
#define  PMC_CNTRL_CPUPWRREQ_OE            BIT(16)
#define  PMC_CNTRL_FUSE_OVERRIDE           BIT(18)
#define  PMC_CNTRL_SHUTDOWN_OE             BIT(22)
#define APBDEV_PMC_SEC_DISABLE       0x4
#define APBDEV_PMC_PWRGATE_TOGGLE    0x30
#define  PMC_PWRGATE_TOGGLE_START          BIT(8)
#define APBDEV_PMC_PWRGATE_STATUS    0x38
#define APBDEV_PMC_NO_IOPOWER        0x44
#define  PMC_NO_IOPOWER_MEM                BIT(7)
#define  PMC_NO_IOPOWER_SDMMC1             BIT(12)
#define  PMC_NO_IOPOWER_SDMMC4             BIT(14)
#define  PMC_NO_IOPOWER_MEM_COMP           BIT(16)
#define  PMC_NO_IOPOWER_AUDIO_HV           BIT(18)
#define  PMC_NO_IOPOWER_GPIO               BIT(21)
#define APBDEV_PMC_SCRATCH0          0x50
#define  PMC_SCRATCH0_MODE_WARMBOOT        BIT(0)
#define  PMC_SCRATCH0_MODE_RCM             BIT(1)
#define  PMC_SCRATCH0_MODE_PAYLOAD         BIT(29)
#define  PMC_SCRATCH0_MODE_BOOTLOADER      BIT(30)
#define  PMC_SCRATCH0_MODE_RECOVERY        BIT(31)
#define  PMC_SCRATCH0_MODE_CUSTOM_ALL      (PMC_SCRATCH0_MODE_RECOVERY   | \
											PMC_SCRATCH0_MODE_BOOTLOADER | \
											PMC_SCRATCH0_MODE_PAYLOAD)
#define APBDEV_PMC_BLINK_TIMER       0x40
#define  PMC_BLINK_ON(n)                   ((n & 0x7FFF))
#define  PMC_BLINK_FORCE                   BIT(15)
#define  PMC_BLINK_OFF(n)                  ((u32)(n & 0xFFFF) << 16)
#define APBDEV_PMC_SCRATCH1          0x54
#define APBDEV_PMC_SCRATCH20         0xA0 // ODM data/config scratch.
#define APBDEV_PMC_SECURE_SCRATCH4   0xC0
#define APBDEV_PMC_SECURE_SCRATCH5   0xC4
#define APBDEV_PMC_PWR_DET_VAL       0xE4
#define  PMC_PWR_DET_33V_SDMMC1            BIT(12)
#define  PMC_PWR_DET_33V_AUDIO_HV          BIT(18)
#define  PMC_PWR_DET_33V_GPIO              BIT(21)
#define APBDEV_PMC_DDR_PWR           0xE8
#define APBDEV_PMC_USB_AO            0xF0
#define APBDEV_PMC_CRYPTO_OP         0xF4
#define  PMC_CRYPTO_OP_SE_ENABLE           0
#define  PMC_CRYPTO_OP_SE_DISABLE          1
#define APBDEV_PMC_PLLP_WB0_OVERRIDE 0xF8
#define  PMC_PLLP_WB0_OVERRIDE_PLLM_OVERRIDE_ENABLE BIT(11)
#define  PMC_PLLP_WB0_OVERRIDE_PLLM_ENABLE BIT(12)
#define APBDEV_PMC_SCRATCH33         0x120
#define APBDEV_PMC_SCRATCH37         0x130
#define  PMC_SCRATCH37_KERNEL_PANIC_MAGIC  0x4E415054 // "TPAN"
#define APBDEV_PMC_SCRATCH39         0x138
#define APBDEV_PMC_SCRATCH40         0x13C
#define APBDEV_PMC_OSC_EDPD_OVER     0x1A4
#define  PMC_OSC_EDPD_OVER_OSC_CTRL_OVER   BIT(22)
#define APBDEV_PMC_CLK_OUT_CNTRL     0x1A8
#define  PMC_CLK_OUT_CNTRL_CLK1_FORCE_EN   BIT(2)
#define  PMC_CLK_OUT_CNTRL_CLK2_FORCE_EN   BIT(10)
#define  PMC_CLK_OUT_CNTRL_CLK3_FORCE_EN   BIT(18)
#define  PMC_CLK_OUT_CNTRL_CLK1_SRC_SEL(src) (((src) & 3) << 6)
#define  PMC_CLK_OUT_CNTRL_CLK2_SRC_SEL(src) (((src) & 3) << 14)
#define  PMC_CLK_OUT_CNTRL_CLK3_SRC_SEL(src) (((src) & 3) << 22)
#define   OSC_DIV1                         0
#define   OSC_DIV2                         1
#define   OSC_DIV4                         2
#define   OSC_CAR                          3
#define APBDEV_PMC_RST_STATUS        0x1B4
#define  PMC_RST_STATUS_MASK               7
#define  PMC_RST_STATUS_POR                0
#define  PMC_RST_STATUS_WATCHDOG           1
#define  PMC_RST_STATUS_SENSOR             2
#define  PMC_RST_STATUS_SW_MAIN            3
#define  PMC_RST_STATUS_LP0                4
#define  PMC_RST_STATUS_AOTAG              5
#define APBDEV_PMC_IO_DPD_REQ        0x1B8
#define  PMC_IO_DPD_REQ_DPD_IDLE           (0 << 30u)
#define  PMC_IO_DPD_REQ_DPD_OFF            (1 << 30u)
#define  PMC_IO_DPD_REQ_DPD_ON             (2 << 30u)
#define APBDEV_PMC_IO_DPD2_REQ       0x1C0
#define APBDEV_PMC_VDDP_SEL          0x1CC
#define APBDEV_PMC_DDR_CFG           0x1D0
#define APBDEV_PMC_SECURE_SCRATCH6   0x224
#define APBDEV_PMC_SECURE_SCRATCH7   0x228
#define APBDEV_PMC_SCRATCH45         0x234
#define APBDEV_PMC_SCRATCH46         0x238
#define APBDEV_PMC_SCRATCH49         0x244
#define APBDEV_PMC_SCRATCH52         0x250
#define APBDEV_PMC_SCRATCH53         0x254
#define APBDEV_PMC_SCRATCH54         0x258
#define APBDEV_PMC_SCRATCH55         0x25C
#define APBDEV_PMC_TSC_MULT          0x2B4
#define APBDEV_PMC_STICKY_BITS       0x2C0
#define  PMC_STICKY_BITS_HDA_LPBK_DIS      BIT(0)
#define APBDEV_PMC_SEC_DISABLE2      0x2C4
#define APBDEV_PMC_WEAK_BIAS         0x2C8
#define APBDEV_PMC_REG_SHORT         0x2CC
#define APBDEV_PMC_SEC_DISABLE3      0x2D8
#define APBDEV_PMC_SECURE_SCRATCH21  0x334
#define  PMC_FUSE_PRIVATEKEYDISABLE_TZ_STICKY_BIT BIT(4)
#define APBDEV_PMC_SECURE_SCRATCH22  0x338 // AArch32 reset address.
#define APBDEV_PMC_SECURE_SCRATCH32  0x360
#define APBDEV_PMC_SECURE_SCRATCH34  0x368 // AArch64 reset address.
#define APBDEV_PMC_SECURE_SCRATCH35  0x36C // AArch64 reset hi-address.
#define APBDEV_PMC_SECURE_SCRATCH49  0x3A4
#define APBDEV_PMC_CNTRL2            0x440
#define  PMC_CNTRL2_WAKE_INT_EN            BIT(0)
#define  PMC_CNTRL2_WAKE_DET_EN            BIT(9)
#define  PMC_CNTRL2_SYSCLK_ORRIDE          BIT(10)
#define  PMC_CNTRL2_HOLD_CKE_LOW_EN        BIT(12)
#define  PMC_CNTRL2_ALLOW_PULSE_WAKE       BIT(14)
#define APBDEV_PMC_FUSE_CONTROL      0x450
#define  PMC_FUSE_CONTROL_PS18_LATCH_SET   BIT(8)
#define  PMC_FUSE_CONTROL_PS18_LATCH_CLR   BIT(9)
#define APBDEV_PMC_IO_DPD3_REQ       0x45C
#define APBDEV_PMC_IO_DPD4_REQ       0x464
#define APBDEV_PMC_UTMIP_PAD_CFG1    0x4C4
#define APBDEV_PMC_UTMIP_PAD_CFG3    0x4CC
#define APBDEV_PMC_DDR_CNTRL         0x4E4
#define APBDEV_PMC_SEC_DISABLE4      0x5B0
#define APBDEV_PMC_SEC_DISABLE5      0x5B4
#define APBDEV_PMC_SEC_DISABLE6      0x5B8
#define APBDEV_PMC_SEC_DISABLE7      0x5BC
#define APBDEV_PMC_SEC_DISABLE8      0x5C0
#define APBDEV_PMC_SEC_DISABLE9      0x5C4
#define APBDEV_PMC_SEC_DISABLE10     0x5C8
#define APBDEV_PMC_SCRATCH188        0x810
#define APBDEV_PMC_SCRATCH190        0x818
#define APBDEV_PMC_SCRATCH200        0x840
#define  PMC_NX_PANIC_SAFE_MODE            0x20
#define  PMC_NX_PANIC_BYPASS_FUSES         0x21
#define APBDEV_PMC_SCRATCH201        0x844
#define APBDEV_PMC_SCRATCH250        0x908
#define APBDEV_PMC_SECURE_SCRATCH108 0xB08
#define APBDEV_PMC_SECURE_SCRATCH109 0xB0C
#define APBDEV_PMC_SECURE_SCRATCH110 0xB10
#define APBDEV_PMC_SECURE_SCRATCH112 0xB18
#define APBDEV_PMC_SECURE_SCRATCH113 0xB1C
#define APBDEV_PMC_SECURE_SCRATCH114 0xB20
#define APBDEV_PMC_SECURE_SCRATCH119 0xB34

// Only in T210B01.
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE0 0xA48
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE1 0xA4C
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE2 0xA50
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE3 0xA54
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE4 0xA58
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE5 0xA5C
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE6 0xA60
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE7 0xA64
#define APBDEV_PMC_SCRATCH_WRITE_DISABLE8 0xA68
#define APBDEV_PMC_LED_BREATHING_CTRL         0xB48
#define  PMC_LED_BREATHING_CTRL_ENABLE              BIT(0)
#define  PMC_LED_BREATHING_CTRL_COUNTER1_EN         BIT(1)
#define APBDEV_PMC_LED_BREATHING_SLOPE_STEPS  0xB4C
#define APBDEV_PMC_LED_BREATHING_ON_COUNTER   0xB50
#define APBDEV_PMC_LED_BREATHING_OFF_COUNTER1 0xB54
#define APBDEV_PMC_LED_BREATHING_OFF_COUNTER0 0xB58
#define  PMC_LED_BREATHING_COUNTER_HZ               32768
#define APBDEV_PMC_LED_BREATHING_STATUS       0xB5C
#define  PMC_LED_BREATHING_FSM_STATUS_MASK          0x7
#define  PMC_LED_BREATHING_FSM_STS_IDLE             0
#define  PMC_LED_BREATHING_FSM_STS_UP_RAMP          1
#define  PMC_LED_BREATHING_FSM_STS_PLATEAU          2
#define  PMC_LED_BREATHING_FSM_STS_DOWN_RAMP        3
#define  PMC_LED_BREATHING_FSM_STS_SHORT_LOW_PERIOD 4
#define  PMC_LED_BREATHING_FSM_STS_LONG_LOW_PERIOD  5
#define APBDEV_PMC_TZRAM_PWR_CNTRL            0xBE8
#define  PMC_TZRAM_PWR_CNTRL_SD                     BIT(0)
#define APBDEV_PMC_TZRAM_SEC_DISABLE          0xBEC
#define APBDEV_PMC_TZRAM_NON_SEC_DISABLE      0xBF0
#define  PMC_TZRAM_DISABLE_REG_WRITE                BIT(0)
#define  PMC_TZRAM_DISABLE_REG_READ                 BIT(1)

typedef enum _pmc_sec_lock_t
{
	PMC_SEC_LOCK_MISC           = BIT(0),
	PMC_SEC_LOCK_LP0_PARAMS     = BIT(1),
	PMC_SEC_LOCK_RST_VECTOR     = BIT(2),
	PMC_SEC_LOCK_CARVEOUTS      = BIT(3),
	PMC_SEC_LOCK_TZ_CMAC_W      = BIT(4),
	PMC_SEC_LOCK_TZ_CMAC_R      = BIT(5),
	PMC_SEC_LOCK_TZ_KEK_W       = BIT(6),
	PMC_SEC_LOCK_TZ_KEK_R       = BIT(7),
	PMC_SEC_LOCK_SE_SRK         = BIT(8),
	PMC_SEC_LOCK_SE2_SRK_B01    = BIT(9),
	PMC_SEC_LOCK_MISC_B01       = BIT(10),
	PMC_SEC_LOCK_CARVEOUTS_L4T  = BIT(11),
	PMC_SEC_LOCK_LP0_PARAMS_B01 = BIT(12),
} pmc_sec_lock_t;

typedef enum _pmc_power_rail_t
{
	POWER_RAIL_CRAIL = 0,
	POWER_RAIL_VE    = 2,
	POWER_RAIL_PCIE  = 3,
	POWER_RAIL_NVENC = 6,
	POWER_RAIL_SATA  = 8,
	POWER_RAIL_CE1   = 9,
	POWER_RAIL_CE2   = 10,
	POWER_RAIL_CE3   = 11,
	POWER_RAIL_CELP  = 12,
	POWER_RAIL_CE0   = 14,
	POWER_RAIL_C0NC  = 15,
	POWER_RAIL_C1NC  = 16,
	POWER_RAIL_SOR   = 17,
	POWER_RAIL_DIS   = 18,
	POWER_RAIL_DISB  = 19,
	POWER_RAIL_XUSBA = 20,
	POWER_RAIL_XUSBB = 21,
	POWER_RAIL_XUSBC = 22,
	POWER_RAIL_VIC   = 23,
	POWER_RAIL_IRAM  = 24,
	POWER_RAIL_NVDEC = 25,
	POWER_RAIL_NVJPG = 26,
	POWER_RAIL_AUD   = 27,
	POWER_RAIL_DFD   = 28,
	POWER_RAIL_VE2   = 29
} pmc_power_rail_t;

void pmc_scratch_lock(pmc_sec_lock_t lock_mask);
int  pmc_domain_pwrgate_set(pmc_power_rail_t part, u32 enable);

#endif
