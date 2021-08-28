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

#ifndef _T210_H_
#define _T210_H_

#include <utils/types.h>

#define BOOTROM_BASE 0x100000
#define IRAM_BASE 0x40000000
#define HOST1X_BASE 0x50000000
#define BPMP_CACHE_BASE 0x50040000
#define DISPLAY_A_BASE 0x54200000
#define DSI_BASE 0x54300000
#define VIC_BASE 0x54340000
#define TSEC_BASE 0x54500000
#define SOR1_BASE 0x54580000
#define ICTLR_BASE 0x60004000
#define TMR_BASE 0x60005000
#define CLOCK_BASE 0x60006000
#define FLOW_CTLR_BASE 0x60007000
#define AHBDMA_BASE 0x60008000
#define SYSREG_BASE 0x6000C000
#define SB_BASE (SYSREG_BASE + 0x200)
#define GPIO_BASE 0x6000D000
#define GPIO_1_BASE (GPIO_BASE)
#define GPIO_2_BASE (GPIO_BASE + 0x100)
#define GPIO_3_BASE (GPIO_BASE + 0x200)
#define GPIO_4_BASE (GPIO_BASE + 0x300)
#define GPIO_5_BASE (GPIO_BASE + 0x400)
#define GPIO_6_BASE (GPIO_BASE + 0x500)
#define GPIO_7_BASE (GPIO_BASE + 0x600)
#define GPIO_8_BASE (GPIO_BASE + 0x700)
#define EXCP_VEC_BASE 0x6000F000
#define IPATCH_BASE 0x6001DC00
#define APBDMA_BASE 0x60020000
#define APB_MISC_BASE 0x70000000
#define PINMUX_AUX_BASE 0x70003000
#define UART_BASE 0x70006000
#define PWM_BASE 0x7000A000
#define RTC_BASE 0x7000E000
#define PMC_BASE 0x7000E400
#define SYSCTR0_BASE 0x700F0000
#define FUSE_BASE 0x7000F800
#define KFUSE_BASE 0x7000FC00
#define SE_BASE 0x70012000
#define MC_BASE 0x70019000
#define EMC_BASE 0x7001B000
#define EMC0_BASE 0x7001E000
#define EMC1_BASE 0x7001F000
#define XUSB_HOST_BASE 0x70090000
#define XUSB_PADCTL_BASE 0x7009F000
#define XUSB_DEV_BASE 0x700D0000
#define MIPI_CAL_BASE 0x700E3000
#define CL_DVFS_BASE 0x70110000
#define I2S_BASE 0x702D1000
#define ADMA_BASE 0x702E2000
#define TZRAM_BASE 0x7C010000
#define USB_BASE 0x7D000000
#define USB_OTG_BASE USB_BASE
#define USB1_BASE 0x7D004000

#define _REG(base, off) *(vu32 *)((base) + (off))

#define HOST1X(off) _REG(HOST1X_BASE, off)
#define BPMP_CACHE_CTRL(off) _REG(BPMP_CACHE_BASE, off)
#define DISPLAY_A(off) _REG(DISPLAY_A_BASE, off)
#define DSI(off) _REG(DSI_BASE, off)
#define VIC(off) _REG(VIC_BASE, off)
#define TSEC(off) _REG(TSEC_BASE, off)
#define SOR1(off) _REG(SOR1_BASE, off)
#define ICTLR(cidx, off) _REG(ICTLR_BASE + (0x100 * (cidx)), off)
#define TMR(off) _REG(TMR_BASE, off)
#define CLOCK(off) _REG(CLOCK_BASE, off)
#define FLOW_CTLR(off) _REG(FLOW_CTLR_BASE, off)
#define SYSREG(off) _REG(SYSREG_BASE, off)
#define AHB_GIZMO(off) _REG(SYSREG_BASE, off)
#define SB(off) _REG(SB_BASE, off)
#define GPIO(off) _REG(GPIO_BASE, off)
#define GPIO_1(off) _REG(GPIO_1_BASE, off)
#define GPIO_2(off) _REG(GPIO_2_BASE, off)
#define GPIO_3(off) _REG(GPIO_3_BASE, off)
#define GPIO_4(off) _REG(GPIO_4_BASE, off)
#define GPIO_5(off) _REG(GPIO_5_BASE, off)
#define GPIO_6(off) _REG(GPIO_6_BASE, off)
#define GPIO_7(off) _REG(GPIO_7_BASE, off)
#define GPIO_8(off) _REG(GPIO_8_BASE, off)
#define EXCP_VEC(off) _REG(EXCP_VEC_BASE, off)
#define APB_MISC(off) _REG(APB_MISC_BASE, off)
#define PINMUX_AUX(off) _REG(PINMUX_AUX_BASE, off)
#define PWM(off) _REG(PWM_BASE, off)
#define RTC(off) _REG(RTC_BASE, off)
#define PMC(off) _REG(PMC_BASE, off)
#define SYSCTR0(off) _REG(SYSCTR0_BASE, off)
#define FUSE(off) _REG(FUSE_BASE, off)
#define KFUSE(off) _REG(KFUSE_BASE, off)
#define SE(off) _REG(SE_BASE, off)
#define MC(off) _REG(MC_BASE, off)
#define EMC(off) _REG(EMC_BASE, off)
#define EMC_CH0(off) _REG(EMC0_BASE, off)
#define EMC_CH1(off) _REG(EMC1_BASE, off)
#define XUSB_HOST(off) _REG(XUSB_HOST_BASE, off)
#define XUSB_PADCTL(off) _REG(XUSB_PADCTL_BASE, off)
#define XUSB_DEV(off) _REG(XUSB_DEV_BASE, off)
#define XUSB_DEV_XHCI(off) _REG(XUSB_DEV_BASE, off)
#define XUSB_DEV_PCI(off) _REG(XUSB_DEV_BASE + 0x8000, off)
#define XUSB_DEV_DEV(off) _REG(XUSB_DEV_BASE + 0x9000, off)
#define MIPI_CAL(off) _REG(MIPI_CAL_BASE, off)
#define CL_DVFS(off) _REG(CL_DVFS_BASE, off)
#define I2S(off) _REG(I2S_BASE, off)
#define ADMA(off) _REG(ADMA_BASE, off)
#define USB(off) _REG(USB_BASE, off)
#define USB1(off) _REG(USB1_BASE, off)
#define TEST_REG(off) _REG(0x0, off)

/* HOST1X registers. */
#define HOST1X_CH0_SYNC_BASE 0x2100
#define HOST1X_CH0_SYNC_SYNCPT_9   (HOST1X_CH0_SYNC_BASE + 0xFA4)
#define HOST1X_CH0_SYNC_SYNCPT_160 (HOST1X_CH0_SYNC_BASE + 0x1200)

/*! EVP registers. */
#define EVP_CPU_RESET_VECTOR 0x100
#define EVP_COP_RESET_VECTOR 0x200
#define EVP_COP_UNDEF_VECTOR 0x204
#define EVP_COP_SWI_VECTOR 0x208
#define EVP_COP_PREFETCH_ABORT_VECTOR 0x20C
#define EVP_COP_DATA_ABORT_VECTOR 0x210
#define EVP_COP_RSVD_VECTOR 0x214
#define EVP_COP_IRQ_VECTOR 0x218
#define EVP_COP_FIQ_VECTOR 0x21C
#define EVP_COP_IRQ_STS 0x220

/*! Primary Interrupt Controller registers. */
#define PRI_ICTLR_FIR 0x14
#define PRI_ICTLR_FIR_SET 0x18
#define PRI_ICTLR_FIR_CLR 0x1C
#define PRI_ICTLR_CPU_IER 0x20
#define PRI_ICTLR_CPU_IER_SET 0x24
#define PRI_ICTLR_CPU_IER_CLR 0x28
#define PRI_ICTLR_CPU_IEP_CLASS 0x2C
#define PRI_ICTLR_COP_IER 0x30
#define PRI_ICTLR_COP_IER_SET 0x34
#define PRI_ICTLR_COP_IER_CLR 0x38
#define PRI_ICTLR_COP_IEP_CLASS 0x3C

/*! AHB Gizmo registers. */
#define AHB_ARBITRATION_PRIORITY_CTRL 0x8
#define  PRIORITY_CTRL_WEIGHT(x) (((x) & 7) << 29)
#define  PRIORITY_SELECT_USB BIT(6)   // USB-OTG.
#define  PRIORITY_SELECT_USB2 BIT(18) // USB-HSIC.
#define  PRIORITY_SELECT_USB3 BIT(17) // XUSB.
#define AHB_GIZMO_AHB_MEM 0x10
#define  AHB_MEM_ENB_FAST_REARBITRATE BIT(2)
#define  AHB_MEM_DONT_SPLIT_AHB_WR    BIT(7)
#define  AHB_MEM_IMMEDIATE            BIT(18)
#define AHB_GIZMO_APB_DMA 0x14
#define AHB_GIZMO_USB 0x20
#define AHB_GIZMO_SDMMC4 0x48
#define AHB_GIZMO_USB2 0x7C
#define AHB_GIZMO_USB3 0x80
#define  AHB_GIZMO_IMMEDIATE BIT(18)
#define AHB_ARBITRATION_XBAR_CTRL 0xE0
#define AHB_AHB_MEM_PREFETCH_CFG3 0xE4
#define AHB_AHB_MEM_PREFETCH_CFG4 0xE8
#define AHB_AHB_MEM_PREFETCH_CFG1 0xF0
#define AHB_AHB_MEM_PREFETCH_CFG2 0xF4
#define  MST_ID(x) (((x) & 0x1F) << 26)
#define  MEM_PREFETCH_AHBDMA_MST_ID MST_ID(5)
#define  MEM_PREFETCH_USB_MST_ID MST_ID(6)   // USB-OTG.
#define  MEM_PREFETCH_USB2_MST_ID MST_ID(18) // USB-HSIC.
#define  MEM_PREFETCH_USB3_MST_ID MST_ID(17) // XUSB.
#define  MEM_PREFETCH_ADDR_BNDRY(x) (((x) & 0xF) << 21)
#define  MEM_PREFETCH_ENABLE BIT(31)
#define AHB_AHB_SPARE_REG 0x110

/*! Misc registers. */
#define APB_MISC_PP_STRAPPING_OPT_A 0x08
#define APB_MISC_PP_PINMUX_GLOBAL 0x40
#define APB_MISC_GP_HIDREV 0x804
#define  GP_HIDREV_MAJOR_T210    0x1
#define  GP_HIDREV_MAJOR_T210B01 0x2
#define APB_MISC_GP_AUD_MCLK_CFGPADCTRL 0x8F4
#define APB_MISC_GP_LCD_BL_PWM_CFGPADCTRL 0xA34
#define APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL 0xA98
#define APB_MISC_GP_EMMC2_PAD_CFGPADCTRL 0xA9C
#define APB_MISC_GP_EMMC4_PAD_CFGPADCTRL 0xAB4
#define APB_MISC_GP_EMMC4_PAD_PUPD_CFGPADCTRL 0xABC
#define APB_MISC_GP_DSI_PAD_CONTROL 0xAC0
#define APB_MISC_GP_WIFI_EN_CFGPADCTRL 0xB64
#define APB_MISC_GP_WIFI_RST_CFGPADCTRL 0xB68

/*! Secure boot registers. */
#define SB_CSR 0x0
#define  SB_CSR_NS_RST_VEC_WR_DIS    BIT(1)
#define  SB_CSR_PIROM_DISABLE        BIT(4)
#define SB_AA64_RESET_LOW  0x30
#define  SB_AA64_RST_AARCH64_MODE_EN BIT(0)
#define SB_AA64_RESET_HIGH 0x34

/*! SOR registers. */
#define SOR_NV_PDISP_SOR_DP_HDCP_BKSV_LSB   0x1E8
#define SOR_NV_PDISP_SOR_TMDS_HDCP_BKSV_LSB 0x21C
#define SOR_NV_PDISP_SOR_TMDS_HDCP_CN_MSB   0x208
#define SOR_NV_PDISP_SOR_TMDS_HDCP_CN_LSB   0x20C

/*! RTC registers. */
#define APBDEV_RTC_SECONDS        0x8
#define APBDEV_RTC_SHADOW_SECONDS 0xC
#define APBDEV_RTC_MILLI_SECONDS  0x10

/*! SYSCTR0 registers. */
#define SYSCTR0_CNTFID0     0x20
#define SYSCTR0_CNTCR       0x00
#define SYSCTR0_COUNTERID0  0xFE0
#define SYSCTR0_COUNTERID1  0xFE4
#define SYSCTR0_COUNTERID2  0xFE8
#define SYSCTR0_COUNTERID3  0xFEC
#define SYSCTR0_COUNTERID4  0xFD0
#define SYSCTR0_COUNTERID5  0xFD4
#define SYSCTR0_COUNTERID6  0xFD8
#define SYSCTR0_COUNTERID7  0xFDC
#define SYSCTR0_COUNTERID8  0xFF0
#define SYSCTR0_COUNTERID9  0xFF4
#define SYSCTR0_COUNTERID10 0xFF8
#define SYSCTR0_COUNTERID11 0xFFC

/*! TMR registers. */
#define TIMERUS_CNTR_1US          (0x10 + 0x0)
#define TIMERUS_USEC_CFG          (0x10 + 0x4)
#define TIMER_TMR8_TMR_PTV        0x78
#define TIMER_TMR9_TMR_PTV        0x80
#define  TIMER_PER_EN      BIT(30)
#define  TIMER_EN          BIT(31)
#define TIMER_TMR8_TMR_PCR        0x7C
#define TIMER_TMR9_TMR_PCR        0x8C
#define  TIMER_INTR_CLR    BIT(30)

#define TIMER_WDT4_CONFIG         (0x100 + 0x80)
#define  TIMER_SRC(TMR) ((TMR) & 0xF)
#define  TIMER_PER(PER) (((PER) & 0xFF) << 4)
#define  TIMER_SYSRESET_EN BIT(14)
#define  TIMER_PMCRESET_EN BIT(15)
#define TIMER_WDT4_COMMAND        (0x108 + 0x80)
#define  TIMER_START_CNT   BIT(0)
#define  TIMER_CNT_DISABLE BIT(1)
#define TIMER_WDT4_UNLOCK_PATTERN (0x10C + 0x80)
#define  TIMER_MAGIC_PTRN 0xC45A

/*! I2S registers. */
#define I2S1_CG   0x88
#define I2S1_CTRL 0xA0
#define I2S2_CG   0x188
#define I2S2_CTRL 0x1A0
#define I2S3_CG   0x288
#define I2S3_CTRL 0x2A0
#define I2S4_CG   0x388
#define I2S4_CTRL 0x3A0
#define I2S5_CG   0x488
#define I2S5_CTRL 0x4A0
#define  I2S_CG_SLCG_ENABLE BIT(0)
#define  I2S_CTRL_MASTER_EN BIT(10)

/*! PWM registers. */
#define PWM_CONTROLLER_PWM_CSR_0 0x00
#define PWM_CONTROLLER_PWM_CSR_1 0x10
#define  PWM_CSR_EN BIT(31)

/*! Special registers. */
#define EMC_SCRATCH0 0x324
#define  EMC_HEKA_UPD BIT(30)

/*! Flow controller registers. */
#define FLOW_CTLR_HALT_COP_EVENTS  0x4
#define  HALT_COP_GIC_IRQ        BIT(9)
#define  HALT_COP_LIC_IRQ        BIT(11)
#define  HALT_COP_SEC            BIT(23)
#define  HALT_COP_MSEC           BIT(24)
#define  HALT_COP_USEC           BIT(25)
#define  HALT_COP_JTAG           BIT(28)
#define  HALT_COP_WAIT_EVENT     BIT(30)
#define  HALT_COP_STOP_UNTIL_IRQ BIT(31)
#define  HALT_COP_MAX_CNT        0xFF
#define FLOW_CTLR_HALT_CPU0_EVENTS 0x0
#define FLOW_CTLR_HALT_CPU1_EVENTS 0x14
#define FLOW_CTLR_HALT_CPU2_EVENTS 0x1C
#define FLOW_CTLR_HALT_CPU3_EVENTS 0x24
#define FLOW_CTLR_CPU0_CSR 0x8
#define FLOW_CTLR_CPU1_CSR 0x18
#define FLOW_CTLR_CPU2_CSR 0x20
#define FLOW_CTLR_CPU3_CSR 0x28
#define FLOW_CTLR_RAM_REPAIR 0x40
#define FLOW_CTLR_BPMP_CLUSTER_CONTROL 0x98

#endif
