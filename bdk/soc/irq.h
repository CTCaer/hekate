/*
 * BPMP-Lite IRQ driver for Tegra X1
 *
 * Copyright (c) 2019 CTCaer
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

#ifndef IRQ_H
#define IRQ_H

#include <utils/types.h>

#define IRQ_MAX_HANDLERS 16

/* Primary interrupt controller ids */
#define IRQ_TMR1 0
#define IRQ_TMR2 1
#define IRQ_RTC 2
#define IRQ_CEC 3
#define IRQ_SHR_SEM_INBOX_FULL 4
#define IRQ_SHR_SEM_INBOX_EMPTY 5
#define IRQ_SHR_SEM_OUTBOX_FULL 6
#define IRQ_SHR_SEM_OUTBOX_EMPTY 7
#define IRQ_NVJPEG 8
#define IRQ_NVDEC 9
#define IRQ_QUAD_SPI 10
#define IRQ_DPAUX_INT1 11
#define IRQ_SATA_RX_STAT 13
#define IRQ_SDMMC1 14
#define IRQ_SDMMC2 15
#define IRQ_VGPIO_INT 16
#define IRQ_VII2C_INT 17
#define IRQ_SDMMC3 19
#define IRQ_USB 20
#define IRQ_USB2 21
#define IRQ_SATA_CTL 23
#define IRQ_PMC_INT 24
#define IRQ_FC_INT 25
#define IRQ_APB_DMA_CPU 26
#define IRQ_ARB_SEM_GNT_COP 28
#define IRQ_ARB_SEM_GNT_CPU 29
#define IRQ_SDMMC4 31

/* Secondary interrupt controller ids */
#define IRQ_GPIO1 32
#define IRQ_GPIO2 33
#define IRQ_GPIO3 34
#define IRQ_GPIO4 35
#define IRQ_UARTA 36
#define IRQ_UARTB 37
#define IRQ_I2C 38
#define IRQ_USB3_HOST_INT 39
#define IRQ_USB3_HOST_SMI 40
#define IRQ_TMR3 41
#define IRQ_TMR4 42
#define IRQ_USB3_HOST_PME 43
#define IRQ_USB3_DEV_HOST 44
#define IRQ_ACTMON 45
#define IRQ_UARTC 46
#define IRQ_THERMAL 48
#define IRQ_XUSB_PADCTL 49
#define IRQ_TSEC 50
#define IRQ_EDP 51
#define IRQ_I2C5 53
#define IRQ_GPIO5 55
#define IRQ_USB3_DEV_SMI 56
#define IRQ_USB3_DEV_PME 57
#define IRQ_SE 58
#define IRQ_SPI1 59
#define IRQ_APB_DMA_COP 60
#define IRQ_CLDVFS 62
#define IRQ_I2C6 63

/* Tertiary interrupt controller ids */
#define IRQ_HOST1X_SYNCPT_COP 64
#define IRQ_HOST1X_SYNCPT_CPU 65
#define IRQ_HOST1X_GEN_COP 66
#define IRQ_HOST1X_GEN_CPU 67
#define IRQ_NVENC 68
#define IRQ_VI 69
#define IRQ_ISPB 70
#define IRQ_ISP 71
#define IRQ_VIC 72
#define IRQ_DISPLAY 73
#define IRQ_DISPLAYB 74
#define IRQ_SOR1 75
#define IRQ_SOR 76
#define IRQ_MC 77
#define IRQ_EMC 78
#define IRQ_TSECB 80
#define IRQ_HDA 81
#define IRQ_SPI2 82
#define IRQ_SPI3 83
#define IRQ_I2C2 84
#define IRQ_PMU_EXT 86
#define IRQ_GPIO6 87
#define IRQ_GPIO7 89
#define IRQ_UARTD 90
#define IRQ_I2C3 92
#define IRQ_SPI4 93

/* Quaternary interrupt controller ids */
#define IRQ_DTV 96
#define IRQ_PCIE_INT 98
#define IRQ_PCIE_MSI 99
#define IRQ_AVP_CACHE 101
#define IRQ_APE_INT1 102
#define IRQ_APE_INT0 103
#define IRQ_APB_DMA_CH0 104
#define IRQ_APB_DMA_CH1 105
#define IRQ_APB_DMA_CH2 106
#define IRQ_APB_DMA_CH3 107
#define IRQ_APB_DMA_CH4 108
#define IRQ_APB_DMA_CH5 109
#define IRQ_APB_DMA_CH6 110
#define IRQ_APB_DMA_CH7 111
#define IRQ_APB_DMA_CH8 112
#define IRQ_APB_DMA_CH9 113
#define IRQ_APB_DMA_CH10 114
#define IRQ_APB_DMA_CH11 115
#define IRQ_APB_DMA_CH12 116
#define IRQ_APB_DMA_CH13 117
#define IRQ_APB_DMA_CH14 118
#define IRQ_APB_DMA_CH15 119
#define IRQ_I2C4 120
#define IRQ_TMR5 121
#define IRQ_WDT_CPU 123
#define IRQ_WDT_AVP 124
#define IRQ_GPIO8 125
#define IRQ_CAR 126

/* Quinary interrupt controller ids */
#define IRQ_APB_DMA_CH16 128
#define IRQ_APB_DMA_CH17 129
#define IRQ_APB_DMA_CH18 130
#define IRQ_APB_DMA_CH19 131
#define IRQ_APB_DMA_CH20 132
#define IRQ_APB_DMA_CH21 133
#define IRQ_APB_DMA_CH22 134
#define IRQ_APB_DMA_CH23 135
#define IRQ_APB_DMA_CH24 136
#define IRQ_APB_DMA_CH25 137
#define IRQ_APB_DMA_CH26 138
#define IRQ_APB_DMA_CH27 139
#define IRQ_APB_DMA_CH28 140
#define IRQ_APB_DMA_CH29 141
#define IRQ_APB_DMA_CH30 142
#define IRQ_APB_DMA_CH31 143
#define IRQ_CPU0_PMU_INTR 144
#define IRQ_CPU1_PMU_INTR 145
#define IRQ_CPU2_PMU_INTR 146
#define IRQ_CPU3_PMU_INTR 147
#define IRQ_SDMMC1_SYS 148
#define IRQ_SDMMC2_SYS 149
#define IRQ_SDMMC3_SYS 150
#define IRQ_SDMMC4_SYS 151
#define IRQ_TMR6 152
#define IRQ_TMR7 153
#define IRQ_TMR8 154
#define IRQ_TMR9 155
#define IRQ_TMR0 156
#define IRQ_GPU_STALL 157
#define IRQ_GPU_NONSTALL 158
#define IRQ_DPAUX 159

/* Senary interrupt controller ids */
#define IRQ_MPCORE_AXIERRIRQ 160
#define IRQ_MPCORE_INTERRIRQ 161
#define IRQ_EVENT_GPIO_A 162
#define IRQ_EVENT_GPIO_B 163
#define IRQ_EVENT_GPIO_C 164
#define IRQ_FLOW_RSM_CPU 168
#define IRQ_FLOW_RSM_COP 169
#define IRQ_TMR_SHARED 170
#define IRQ_MPCORE_CTIIRQ0 171
#define IRQ_MPCORE_CTIIRQ1 172
#define IRQ_MPCORE_CTIIRQ2 173
#define IRQ_MPCORE_CTIIRQ3 174
#define IRQ_MSELECT_ERROR 175
#define IRQ_TMR10 176
#define IRQ_TMR11 177
#define IRQ_TMR12 178
#define IRQ_TMR13 179

typedef int (*irq_handler_t)(u32 irq, void *data);

typedef enum _irq_status_t
{
	IRQ_NONE    = 0,
	IRQ_HANDLED = 1,
	IRQ_ERROR   = 2,

	IRQ_ENABLED            = 0,
	IRQ_NO_SLOTS_AVAILABLE = 1,
	IRQ_ALREADY_REGISTERED = 2
} irq_status_t;

typedef enum _irq_flags_t
{
	IRQ_FLAG_NONE        = 0,
	IRQ_FLAG_ONE_OFF     = BIT(0),
	IRQ_FLAG_REPLACEABLE = BIT(1)
} irq_flags_t;

void irq_end();
void irq_free(u32 irq);
void irq_wait_event();
void irq_disable_wait_event();
irq_status_t irq_request(u32 irq, irq_handler_t handler, void *data, irq_flags_t flags);

#endif