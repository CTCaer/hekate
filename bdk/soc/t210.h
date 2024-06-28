/*
* Copyright (c) 2018 naehrwert
* Copyright (c) 2018-2023 CTCaer
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

#define IROM_BASE          0x100000
#define IRAM_BASE        0x40000000
#define HOST1X_BASE      0x50000000
#define BPMP_CACHE_BASE  0x50040000
#define MSELECT_BASE     0x50060000
#define DPAUX1_BASE      0x54040000
#define TSEC2_BASE       0x54100000
#define DISPLAY_A_BASE   0x54200000
#define DISPLAY_B_BASE   0x54240000
#define DSI_BASE         0x54300000
#define VIC_BASE         0x54340000
#define NVJPG_BASE       0x54380000
#define NVDEC_BASE       0x54480000
#define NVENC_BASE       0x544C0000
#define TSEC_BASE        0x54500000
#define SOR1_BASE        0x54580000
#define GPU_BASE         0x57000000
#define GPU_USER_BASE    0x58000000
#define RES_SEMAPH_BASE  0x60001000
#define ARB_SEMAPH_BASE  0x60002000
#define ARBPRI_BASE      0x60003000
#define ICTLR_BASE       0x60004000
#define TMR_BASE         0x60005000
#define CLOCK_BASE       0x60006000
#define FLOW_CTLR_BASE   0x60007000
#define AHBDMA_BASE      0x60008000
#define SYSREG_BASE      0x6000C000
#define SB_BASE          (SYSREG_BASE + 0x200)
#define ACTMON_BASE      0x6000C800
#define GPIO_BASE        0x6000D000
#define EXCP_VEC_BASE    0x6000F000
#define IPATCH_BASE      0x6001DC00
#define APBDMA_BASE      0x60020000
#define VGPIO_BASE       0x60024000
#define APB_MISC_BASE    0x70000000
#define PINMUX_AUX_BASE  0x70003000
#define UART_BASE        0x70006000
#define PWM_BASE         0x7000A000
#define I2C_BASE         0x7000C000
#define RTC_BASE         0x7000E000
#define PMC_BASE         0x7000E400
#define FUSE_BASE        0x7000F800
#define KFUSE_BASE       0x7000FC00
#define SE_BASE          0x70012000
#define TSENSOR_BASE     0x70014000
#define ATOMICS_BASE     0x70016000
#define MC_BASE          0x70019000
#define EMC_BASE         0x7001B000
#define EMC0_BASE        0x7001E000
#define EMC1_BASE        0x7001F000
#define XUSB_HOST_BASE   0x70090000
#define XUSB_PADCTL_BASE 0x7009F000
#define XUSB_DEV_BASE    0x700D0000
#define SDMMC_BASE       0x700B0000
#define SOC_THERM_BASE   0x700E2000
#define MIPI_CAL_BASE    0x700E3000
#define SYSCTR0_BASE     0x700F0000
#define SYSCTR1_BASE     0x70100000
#define CL_DVFS_BASE     0x70110000
#define APE_BASE         0x702C0000
#define AHUB_BASE        0x702D0000
#define AXBAR_BASE       0x702D0800
#define I2S_BASE         0x702D1000
#define ADMA_BASE        0x702E2000
#define SE2_BASE         0x70412000
#define SE_PKA1_BASE     0x70420000
#define TZRAM_BASE       0x7C010000
#define  TZRAM_SIZE         0x10000
#define  TZRAM_T210B01_SIZE 0x3C000
#define USB_BASE         0x7D000000
#define USB_OTG_BASE     USB_BASE
#define USB1_BASE        0x7D004000
#define EMEM_BASE        0x80000000

#define MMIO_REG32(base, off) *(vu32 *)((base) + (off))

#define HOST1X(off)          MMIO_REG32(HOST1X_BASE, off)
#define BPMP_CACHE_CTRL(off) MMIO_REG32(BPMP_CACHE_BASE, off)
#define MSELECT(off)         MMIO_REG32(MSELECT_BASE, off)
#define DPAUX1(off)          MMIO_REG32(DPAUX1_BASE, off)
#define TSEC2(off)           MMIO_REG32(TSEC2_BASE, off)
#define DISPLAY_A(off)       MMIO_REG32(DISPLAY_A_BASE, off)
#define DISPLAY_B(off)       MMIO_REG32(DISPLAY_B_BASE, off)
#define DSI(off)             MMIO_REG32(DSI_BASE, off)
#define VIC(off)             MMIO_REG32(VIC_BASE, off)
#define NVJPG(off)           MMIO_REG32(NVJPG_BASE, off)
#define NVDEC(off)           MMIO_REG32(NVDEC_BASE, off)
#define NVENC(off)           MMIO_REG32(NVENC_BASE, off)
#define TSEC(off)            MMIO_REG32(TSEC_BASE, off)
#define SOR1(off)            MMIO_REG32(SOR1_BASE, off)
#define GPU(off)             MMIO_REG32(GPU_BASE, off)
#define GPU_USER(off)        MMIO_REG32(GPU_USER_BASE, off)
#define ICTLR(cidx, off)     MMIO_REG32(ICTLR_BASE + (0x100 * (cidx)), off)
#define TMR(off)             MMIO_REG32(TMR_BASE, off)
#define CLOCK(off)           MMIO_REG32(CLOCK_BASE, off)
#define FLOW_CTLR(off)       MMIO_REG32(FLOW_CTLR_BASE, off)
#define AHBDMA(off)          MMIO_REG32(AHBDMA_BASE, off)
#define SYSREG(off)          MMIO_REG32(SYSREG_BASE, off)
#define AHB_GIZMO(off)       MMIO_REG32(SYSREG_BASE, off)
#define SB(off)              MMIO_REG32(SB_BASE, off)
#define ACTMON(off)          MMIO_REG32(ACTMON_BASE, off)
#define GPIO(off)            MMIO_REG32(GPIO_BASE, off)
#define EXCP_VEC(off)        MMIO_REG32(EXCP_VEC_BASE, off)
#define APBDMA(off)          MMIO_REG32(APBDMA_BASE, off)
#define VGPIO(off)           MMIO_REG32(VGPIO_BASE, off)
#define APB_MISC(off)        MMIO_REG32(APB_MISC_BASE, off)
#define PINMUX_AUX(off)      MMIO_REG32(PINMUX_AUX_BASE, off)
#define PWM(off)             MMIO_REG32(PWM_BASE, off)
#define RTC(off)             MMIO_REG32(RTC_BASE, off)
#define PMC(off)             MMIO_REG32(PMC_BASE, off)
#define SYSCTR0(off)         MMIO_REG32(SYSCTR0_BASE, off)
#define SYSCTR1(off)         MMIO_REG32(SYSCTR1_BASE, off)
#define FUSE(off)            MMIO_REG32(FUSE_BASE, off)
#define KFUSE(off)           MMIO_REG32(KFUSE_BASE, off)
#define SE(off)              MMIO_REG32(SE_BASE, off)
#define MC(off)              MMIO_REG32(MC_BASE, off)
#define EMC(off)             MMIO_REG32(EMC_BASE, off)
#define EMC_CH0(off)         MMIO_REG32(EMC0_BASE, off)
#define EMC_CH1(off)         MMIO_REG32(EMC1_BASE, off)
#define XUSB_HOST(off)       MMIO_REG32(XUSB_HOST_BASE, off)
#define XUSB_PADCTL(off)     MMIO_REG32(XUSB_PADCTL_BASE, off)
#define XUSB_DEV(off)        MMIO_REG32(XUSB_DEV_BASE, off)
#define XUSB_DEV_XHCI(off)   MMIO_REG32(XUSB_DEV_BASE, off)
#define XUSB_DEV_PCI(off)    MMIO_REG32(XUSB_DEV_BASE + 0x8000, off)
#define XUSB_DEV_DEV(off)    MMIO_REG32(XUSB_DEV_BASE + 0x9000, off)
#define MIPI_CAL(off)        MMIO_REG32(MIPI_CAL_BASE, off)
#define CL_DVFS(off)         MMIO_REG32(CL_DVFS_BASE, off)
#define I2S(off)             MMIO_REG32(I2S_BASE, off)
#define ADMA(off)            MMIO_REG32(ADMA_BASE, off)
#define SE2(off)             MMIO_REG32(SE2_BASE, off)
#define SE_PKA1(off)         MMIO_REG32(SE_PKA1_BASE, off)
#define USB(off)             MMIO_REG32(USB_BASE, off)
#define USB1(off)            MMIO_REG32(USB1_BASE, off)
#define TEST_REG(off)        MMIO_REG32(0x0, off)

/* HOST1X v3 registers. */
#define HOST1X_CH0_SYNC_BASE        0x2100
#define HOST1X_CH0_SYNC_SYNCPT_BASE (HOST1X_CH0_SYNC_BASE + 0xF80)
#define HOST1X_CH0_SYNC_SYNCPT_9    (HOST1X_CH0_SYNC_SYNCPT_BASE + 0x24)
#define HOST1X_CH0_SYNC_SYNCPT_160  (HOST1X_CH0_SYNC_SYNCPT_BASE + 0x280)

/*! EVP registers. */
#define EVP_CPU_RESET_VECTOR          0x100
#define EVP_COP_RESET_VECTOR          0x200
#define EVP_COP_UNDEF_VECTOR          0x204
#define EVP_COP_SWI_VECTOR            0x208
#define EVP_COP_PREFETCH_ABORT_VECTOR 0x20C
#define EVP_COP_DATA_ABORT_VECTOR     0x210
#define EVP_COP_RSVD_VECTOR           0x214
#define EVP_COP_IRQ_VECTOR            0x218
#define EVP_COP_FIQ_VECTOR            0x21C
#define EVP_COP_IRQ_STS               0x220

/*! Primary Interrupt Controller registers. */
#define PRI_ICTLR_FIR           0x14
#define PRI_ICTLR_FIR_SET       0x18
#define PRI_ICTLR_FIR_CLR       0x1C
#define PRI_ICTLR_CPU_IER       0x20
#define PRI_ICTLR_CPU_IER_SET   0x24
#define PRI_ICTLR_CPU_IER_CLR   0x28
#define PRI_ICTLR_CPU_IEP_CLASS 0x2C
#define PRI_ICTLR_COP_IER       0x30
#define PRI_ICTLR_COP_IER_SET   0x34
#define PRI_ICTLR_COP_IER_CLR   0x38
#define PRI_ICTLR_COP_IEP_CLASS 0x3C

/*! AHB Gizmo registers. */
#define AHB_ARBITRATION_PRIORITY_CTRL        0x8
#define  PRIORITY_CTRL_WEIGHT(x)              (((x) & 7) << 29)
#define  PRIORITY_SELECT_USB                  BIT(6)   // USB-OTG.
#define  PRIORITY_SELECT_USB2                 BIT(18) // USB-HSIC.
#define  PRIORITY_SELECT_USB3                 BIT(17) // XUSB.
#define AHB_GIZMO_AHB_MEM                    0x10
#define  AHB_MEM_ENB_FAST_REARBITRATE         BIT(2)
#define  AHB_MEM_DONT_SPLIT_AHB_WR            BIT(7)
#define  AHB_MEM_IMMEDIATE                    BIT(18)
#define AHB_GIZMO_APB_DMA                    0x14
#define AHB_GIZMO_USB                        0x20
#define AHB_GIZMO_SDMMC4                     0x48
#define AHB_GIZMO_USB2                       0x7C
#define AHB_GIZMO_USB3                       0x80
#define  AHB_GIZMO_IMMEDIATE                  BIT(18)
#define AHB_ARBITRATION_XBAR_CTRL            0xE0
#define AHB_AHB_MEM_PREFETCH_CFG3            0xE4
#define AHB_AHB_MEM_PREFETCH_CFG4            0xE8
#define AHB_AHB_MEM_PREFETCH_CFG1            0xF0
#define AHB_AHB_MEM_PREFETCH_CFG2            0xF4
#define  MST_ID(x)                            (((x) & 0x1F) << 26)
#define  MEM_PREFETCH_AHBDMA_MST_ID           MST_ID(5)
#define  MEM_PREFETCH_USB_MST_ID              MST_ID(6)  // USB-OTG.
#define  MEM_PREFETCH_USB2_MST_ID             MST_ID(18) // USB-HSIC.
#define  MEM_PREFETCH_USB3_MST_ID             MST_ID(17) // XUSB.
#define  MEM_PREFETCH_ADDR_BNDRY(x)           (((x) & 0xF) << 21)
#define  MEM_PREFETCH_ENABLE                  BIT(31)
#define AHB_ARBITRATION_AHB_MEM_WRQUE_MST_ID 0xFC
#define  MEM_WRQUE_SE_MST_ID                  BIT(14)
#define AHB_AHB_SPARE_REG                    0x110

/*! Misc registers. */
#define APB_MISC_PP_STRAPPING_OPT_A           0x8
#define APB_MISC_PP_PINMUX_GLOBAL             0x40
#define APB_MISC_GP_HIDREV                    0x804
#define  GP_HIDREV_MAJOR_T210                  0x1
#define  GP_HIDREV_MAJOR_T210B01               0x2
#define APB_MISC_GP_ASDBGREG                  0x810
#define APB_MISC_GP_TRANSACTOR_SCRATCH        0x864
#define APB_MISC_GP_AVP_TRANSACTOR_SCRATCH    0x880
#define APB_MISC_GP_CPU0_TRANSACTOR_SCRATCH   0x884
#define APB_MISC_GP_CPU1_TRANSACTOR_SCRATCH   0x888
#define APB_MISC_GP_CPU2_TRANSACTOR_SCRATCH   0x88C
#define APB_MISC_GP_CPU3_TRANSACTOR_SCRATCH   0x890
#define APB_MISC_GP_AUD_MCLK_CFGPADCTRL       0x8F4
#define APB_MISC_GP_LCD_BL_PWM_CFGPADCTRL     0xA34
#define APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL     0xA98
#define APB_MISC_GP_EMMC2_PAD_CFGPADCTRL      0xA9C
#define APB_MISC_GP_EMMC4_PAD_CFGPADCTRL      0xAB4
#define APB_MISC_GP_EMMC4_PAD_PUPD_CFGPADCTRL 0xABC
#define APB_MISC_GP_DSI_PAD_CONTROL           0xAC0
#define APB_MISC_GP_WIFI_EN_CFGPADCTRL        0xB64
#define APB_MISC_GP_WIFI_RST_CFGPADCTRL       0xB68
#define APB_MISC_DAS_DAP_CTRL_SEL             0xC00
#define APB_MISC_DAS_DAC_INPUT_DATA_CLK_SEL   0xC40

/*! Secure boot registers. */
#define SB_CSR                       0x0
#define  SB_CSR_NS_RST_VEC_WR_DIS     BIT(1)
#define  SB_CSR_PIROM_DISABLE         BIT(4)
#define SB_AA64_RESET_LOW            0x30
#define  SB_AA64_RST_AARCH64_MODE_EN  BIT(0)
#define SB_AA64_RESET_HIGH           0x34

/*! SOR registers. */
#define SOR_DP_HDCP_BKSV_LSB   0x1E8
#define SOR_TMDS_HDCP_BKSV_LSB 0x21C
#define SOR_TMDS_HDCP_CN_MSB   0x208
#define SOR_TMDS_HDCP_CN_LSB   0x20C

/*! RTC registers. */
#define APBDEV_RTC_SECONDS        0x8
#define APBDEV_RTC_SHADOW_SECONDS 0xC
#define APBDEV_RTC_MILLI_SECONDS  0x10

/*! SYSCTR0 registers. */
#define SYSCTR0_CNTCR         0x00
#define SYSCTR0_CNTFID0       0x20
#define SYSCTR0_COUNTERS_BASE 0xFD0
#define  SYSCTR0_COUNTERS      12
#define SYSCTR0_COUNTERID0    0xFE0
#define SYSCTR0_COUNTERID1    0xFE4
#define SYSCTR0_COUNTERID2    0xFE8
#define SYSCTR0_COUNTERID3    0xFEC
#define SYSCTR0_COUNTERID4    0xFD0
#define SYSCTR0_COUNTERID5    0xFD4
#define SYSCTR0_COUNTERID6    0xFD8
#define SYSCTR0_COUNTERID7    0xFDC
#define SYSCTR0_COUNTERID8    0xFF0
#define SYSCTR0_COUNTERID9    0xFF4
#define SYSCTR0_COUNTERID10   0xFF8
#define SYSCTR0_COUNTERID11   0xFFC

/*! IPATCH registers. */
#define IPATCH_CAM_VALID   0x0
#define IPATCH_CAM_BASE    0x4
#define IPATCH_CAM(i)      (IPATCH_CAM_BASE + (i) * 4)
#define IPATCH_CAM_ENTRIES 12

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
#define FLOW_CTLR_HALT_COP_EVENTS          0x4
#define FLOW_CTLR_HALT_CPU0_EVENTS         0x0
#define FLOW_CTLR_HALT_CPU1_EVENTS         0x14
#define FLOW_CTLR_HALT_CPU2_EVENTS         0x1C
#define FLOW_CTLR_HALT_CPU3_EVENTS         0x24
#define  HALT_GIC_IRQ                       BIT(9)
#define  HALT_LIC_IRQ                       BIT(11)
#define  HALT_SEC                           BIT(23)
#define  HALT_MSEC                          BIT(24)
#define  HALT_USEC                          BIT(25)
#define  HALT_JTAG                          BIT(28)
#define  HALT_MODE_NONE                     (0 << 29u)
#define  HALT_MODE_RUN_AND_INT              (1 << 29u)
#define  HALT_MODE_WAITEVENT                (2 << 29u)
#define  HALT_MODE_WAITEVENT_AND_INT        (3 << 29u)
#define  HALT_MODE_STOP_UNTIL_IRQ           (4 << 29u)
#define  HALT_MODE_STOP_UNTIL_IRQ_AND_INT   (5 << 29u)
#define  HALT_MODE_STOP_UNTIL_EVENT_AND_IRQ (6 << 29u)
#define  HALT_MAX_CNT                       0xFF
#define FLOW_CTLR_COP_CSR                  0xC
#define FLOW_CTLR_CPU0_CSR                 0x8
#define FLOW_CTLR_CPU1_CSR                 0x18
#define FLOW_CTLR_CPU2_CSR                 0x20
#define FLOW_CTLR_CPU3_CSR                 0x28
#define  CSR_ENABLE                         BIT(0)
#define  CSR_WAIT_WFI_NONE                  (0 << 8u)
#define  CSR_WAIT_WFI_CPU0                  (BIT(0) << 8u)
#define  CSR_ENABLE_EXT_CPU_ONLY            (0 << 12u)
#define  CSR_ENABLE_EXT_CPU_NCPU            (1 << 12u)
#define  CSR_ENABLE_EXT_CPU_RAIL            (2 << 12u)
#define  CSR_EVENT_FLAG                     BIT(14)
#define  CSR_INTR_FLAG                      BIT(15)
#define  CSR_HALT                           BIT(22)
#define FLOW_CTLR_CPU_PWR_CSR              0x38
#define  CPU_PWR_RAIL_STS_MASK              (3 << 1u)
#define  CPU_PWR_RAIL_OFF                   0
#define FLOW_CTLR_RAM_REPAIR               0x40
#define  RAM_REPAIR_REQ                     BIT(0)
#define  RAM_REPAIR_STS                     BIT(1)
#define FLOW_CTLR_BPMP_CLUSTER_CONTROL     0x98
#define  CLUSTER_CTRL_ACTIVE_SLOW           BIT(0)

/* MSelect registers */
#define MSELECT_CONFIG 0x00
#define  MSELECT_CFG_ERR_RESP_EN_PCIE  BIT(24)
#define  MSELECT_CFG_ERR_RESP_EN_GPU   BIT(25)
#define  MSELECT_CFG_WRAP_TO_INCR_BPMP BIT(27)
#define  MSELECT_CFG_WRAP_TO_INCR_PCIE BIT(28)
#define  MSELECT_CFG_WRAP_TO_INCR_GPU  BIT(29)

/* NVDEC registers */
#define NVDEC_SA_KEYSLOT_FALCON    0x2100
#define NVDEC_SA_KEYSLOT_TZ        0x2104
#define NVDEC_SA_KEYSLOT_OTF       0x210C
#define NVDEC_SA_KEYSLOT_GLOBAL_RW 0x2118
#define NVDEC_VPR_ALL_OTF_GOTO_VPR 0x211C
#endif
