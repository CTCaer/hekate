/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2024 CTCaer
 * Copyright (c) 2018-2024 shinyquagsire23
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

#ifndef _AHUB_H_
#define _AHUB_H_

#include <utils/types.h>
#include <soc/t210.h>
#include <audio/i2s.h>

//#define AHUB_BASE                           ((void*)0x702d0000)
#define ADMAIF_BASE                         ((void*)0x702d0000)
//#define ADMA_BASE                           ((void*)0x702e2000)
//#define AXBAR_BASE                          ((void*)0x702d0800)

#define ADMA_CH_SIZE                        (0x80)
#define ADMA_NUM_CHANNELS                   (22)
#define ADMA_CHn_CMD_0(n)                   (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x000))
#define ADMA_CHn_SOFT_RESET_0(n)            (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x004))
#define ADMA_CHn_STATUS_0(n)                (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x00C))
#define ADMA_CHn_INT_STATUS_0(n)            (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x010))
#define ADMA_CHn_INT_SET_0(n)               (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x018))
#define ADMA_CHn_INT_CLEAR_0(n)             (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x01C))
#define ADMA_CHn_CTRL_0(n)                  (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x024))
#define ADMA_CHn_CONFIG_0(n)                (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x028))
#define ADMA_CHn_AHUB_FIFO_CTRL_0(n)        (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x02C))
#define ADMA_CHn_TC_STATUS_0(n)             (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x030))
#define ADMA_CHn_LOWER_SOURCE_ADDR_0(n)     (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x034))
#define ADMA_CHn_LOWER_TARGET_ADDR_0(n)     (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x03C))
#define ADMA_CHn_TC_0(n)                    (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x044))
#define ADMA_CHn_LOWER_DESC_ADDR_0(n)       (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x048))
#define ADMA_CHn_TRANSFER_STATUS_0(n)       (*(vu32*)(ADMA_BASE + (n * ADMA_CH_SIZE) + 0x054))

#define ADMA_GLOBAL_CMD_0                   (*(vu32*)(ADMA_BASE + 0xC00))
#define ADMA_GLOBAL_SOFT_RESET_0            (*(vu32*)(ADMA_BASE + 0xC04))
#define ADMA_GLOBAL_CG_0                    (*(vu32*)(ADMA_BASE + 0xC08))
#define ADMA_GLOBAL_STATUS_0                (*(vu32*)(ADMA_BASE + 0xC10))
#define ADMA_GLOBAL_INT_STATUS_0            (*(vu32*)(ADMA_BASE + 0xC14))
#define ADMA_GLOBAL_INT_MASK_0              (*(vu32*)(ADMA_BASE + 0xC18))
#define ADMA_GLOBAL_INT_SET_0               (*(vu32*)(ADMA_BASE + 0xC1C))
#define ADMA_GLOBAL_INT_CLEAR_0             (*(vu32*)(ADMA_BASE + 0xC20))
#define ADMA_GLOBAL_CTRL_0                  (*(vu32*)(ADMA_BASE + 0xC24))
#define ADMA_GLOBAL_CH_INT_STATUS_0         (*(vu32*)(ADMA_BASE + 0xC28))
#define ADMA_GLOBAL_CH_ENABLE_STATUS_0      (*(vu32*)(ADMA_BASE + 0xC2C))
#define ADMA_GLOBAL_TX_REQUESTORS_0         (*(vu32*)(ADMA_BASE + 0xC30))
#define ADMA_GLOBAL_RX_REQUESTORS_0         (*(vu32*)(ADMA_BASE + 0xC34))
#define ADMA_GLOBAL_GLOBAL_TRIGGERS_0       (*(vu32*)(ADMA_BASE + 0xC38))
#define ADMA_GLOBAL_TRANSFER_ERROR_LOG_0    (*(vu32*)(ADMA_BASE + 0xC3C))

// ADMA_CHn_CTRL
#define ADMA_TX_REQUEST_SELECT(n)                (n << 28)
#define ADMA_RX_REQUEST_SELECT(n)                (n << 24)
#define ADMA_TRANSFER_DIRECTION_MEMORY_TO_MEMORY (1 << 12)
#define ADMA_TRANSFER_DIRECTION_AHUB_TO_MEMORY   (2 << 12)
#define ADMA_TRANSFER_DIRECTION_MEMORY_TO_AHUB   (4 << 12)
#define ADMA_TRANSFER_DIRECTION_AHUB_TO_AHUB     (7 << 12)
#define ADMA_TRANSFER_MODE_ONCE                  (1 << 8)
#define ADMA_TRANSFER_MODE_CONTINUOUS            (2 << 8)
#define ADMA_TRANSFER_MODE_LINKED_LIST           (4 << 8)
#define ADMA_FLOWCTRL_ENABLE BIT(1)
#define ADMA_TRANSFER_PAUSE  BIT(0)

// ADMA_CHn_CONFIG
#define ADMA_TARGET_ADDR_WRAP_NONE              (0)
#define ADMA_TARGET_ADDR_WRAP_ON_1_WORD         (1)
#define ADMA_SOURCE_ADDR_WRAP_NONE              (0 << 16)
#define ADMA_SOURCE_ADDR_WRAP_ON_1_WORD         (1 << 16)
#define ADMA_BURST_SIZE_WORDS_1                 (1 << 20)
#define ADMA_BURST_SIZE_WORDS_2                 (2 << 20)
#define ADMA_BURST_SIZE_WORDS_4                 (3 << 20)
#define ADMA_BURST_SIZE_WORDS_8                 (4 << 20)
#define ADMA_BURST_SIZE_WORDS_16                 (5 << 20)

// ADMA_CHn_CMD
#define ADMA_TRANSFER_DISABLE (0)
#define ADMA_TRANSFER_ENABLE  (1)

// ADMA_CHn_STATUS
#define ADMA_TRANSFER_ENABLED  (1)

#define ADMAIF_AXBAR_RX0_ENABLE_0           (*(vu32*)(ADMAIF_BASE + 0x000))
#define ADMAIF_AXBAR_RX0_SOFT_RESET_0       (*(vu32*)(ADMAIF_BASE + 0x004))
#define ADMAIF_AXBAR_RX0_STATUS_0           (*(vu32*)(ADMAIF_BASE + 0x00c))
#define ADMAIF_AXBAR_RX0_INT_STATUS_0       (*(vu32*)(ADMAIF_BASE + 0x010))
#define ADMAIF_AXBAR_RX0_INT_MASK_0         (*(vu32*)(ADMAIF_BASE + 0x014))
#define ADMAIF_AXBAR_RX0_INT_SET_0          (*(vu32*)(ADMAIF_BASE + 0x018))
#define ADMAIF_AXBAR_RX0_INT_CLEAR_0        (*(vu32*)(ADMAIF_BASE + 0x01c))
#define ADMAIF_AXBAR_RX0_CIF_CTRL_0         (*(vu32*)(ADMAIF_BASE + 0x020))
#define ADMAIF_AXBAR_RX0_FIFO_CTRL_0        (*(vu32*)(ADMAIF_BASE + 0x028))
#define ADMAIF_AXBAR_RX0_FIFO_READ_0        (*(vu32*)(ADMAIF_BASE + 0x02c))
#define ADMAIF_AXBAR_TX0_ENABLE_0           (*(vu32*)(ADMAIF_BASE + 0x300))
#define ADMAIF_AXBAR_TX0_SOFT_RESET_0       (*(vu32*)(ADMAIF_BASE + 0x304))
#define ADMAIF_AXBAR_TX0_STATUS_0           (*(vu32*)(ADMAIF_BASE + 0x30c))
#define ADMAIF_AXBAR_TX0_INT_STATUS_0       (*(vu32*)(ADMAIF_BASE + 0x310))
#define ADMAIF_AXBAR_TX0_INT_MASK_0         (*(vu32*)(ADMAIF_BASE + 0x314))
#define ADMAIF_AXBAR_TX0_INT_SET_0          (*(vu32*)(ADMAIF_BASE + 0x318))
#define ADMAIF_AXBAR_TX0_INT_CLEAR_0        (*(vu32*)(ADMAIF_BASE + 0x31c))
#define ADMAIF_AXBAR_TX0_CIF_CTRL_0         (*(vu32*)(ADMAIF_BASE + 0x320))
#define ADMAIF_AXBAR_TX0_FIFO_CTRL_0        (*(vu32*)(ADMAIF_BASE + 0x328))
#define ADMAIF_AXBAR_TX0_FIFO_WRITE_0       (*(vu32*)(ADMAIF_BASE + 0x32c))

#define ADMAIF_GLOBAL_ENABLE_0                         (*(vu32*)(ADMAIF_BASE + 0x700))
#define ADMAIF_GLOBAL_SOFT_RESET_0                     (*(vu32*)(ADMAIF_BASE + 0x704))
#define ADMAIF_GLOBAL_CG_0                             (*(vu32*)(ADMAIF_BASE + 0x708))
#define ADMAIF_GLOBAL_STATUS_0                         (*(vu32*)(ADMAIF_BASE + 0x710))
#define ADMAIF_GLOBAL_RX_ENABLE_STATUS_0               (*(vu32*)(ADMAIF_BASE + 0x720))
#define ADMAIF_GLOBAL_TX_ENABLE_STATUS_0               (*(vu32*)(ADMAIF_BASE + 0x724))
#define ADMAIF_GLOBAL_RX_DMA_FIFO_EMPTY_STATUS_0       (*(vu32*)(ADMAIF_BASE + 0x728))
#define ADMAIF_GLOBAL_RX_DMA_FIFO_FULL_STATUS_0        (*(vu32*)(ADMAIF_BASE + 0x72c))
#define ADMAIF_GLOBAL_TX_DMA_FIFO_EMPTY_STATUS_0       (*(vu32*)(ADMAIF_BASE + 0x730))
#define ADMAIF_GLOBAL_TX_DMA_FIFO_FULL_STATUS_0        (*(vu32*)(ADMAIF_BASE + 0x734))
#define ADMAIF_GLOBAL_RX_ACIF_FIFO_EMPTY_STATUS_0      (*(vu32*)(ADMAIF_BASE + 0x738))
#define ADMAIF_GLOBAL_RX_ACIF_FIFO_FULL_STATUS_0       (*(vu32*)(ADMAIF_BASE + 0x73c))
#define ADMAIF_GLOBAL_TX_ACIF_FIFO_EMPTY_STATUS_0      (*(vu32*)(ADMAIF_BASE + 0x740))
#define ADMAIF_GLOBAL_TX_ACIF_FIFO_FULL_STATUS_0       (*(vu32*)(ADMAIF_BASE + 0x744))
#define ADMAIF_GLOBAL_RX_DMA_OVERRUN_INT_STATUS_0      (*(vu32*)(ADMAIF_BASE + 0x748))
#define ADMAIF_GLOBAL_TX_DMA_UNDERRUN_INT_STATUS_0     (*(vu32*)(ADMAIF_BASE + 0x74c))

#define AXBAR_PART_0_ADMAIF_RX0_0           (*(vu32*)(AXBAR_BASE + 0x000))
#define AXBAR_PART_0_ADMAIF_RX1_0           (*(vu32*)(AXBAR_BASE + 0x004))
#define AXBAR_PART_0_ADMAIF_RX2_0           (*(vu32*)(AXBAR_BASE + 0x008))
#define AXBAR_PART_0_ADMAIF_RX3_0           (*(vu32*)(AXBAR_BASE + 0x00C))
#define AXBAR_PART_0_I2S1_RX1_0             (*(vu32*)(AXBAR_BASE + 0x040))
#define AXBAR_PART_0_I2S2_RX1_0             (*(vu32*)(AXBAR_BASE + 0x044))
#define AXBAR_PART_0_I2S3_RX1_0             (*(vu32*)(AXBAR_BASE + 0x048))
#define AXBAR_PART_0_I2S4_RX1_0             (*(vu32*)(AXBAR_BASE + 0x04C))
#define AXBAR_PART_0_I2S5_RX1_0             (*(vu32*)(AXBAR_BASE + 0x050))
#define AXBAR_PART_0_SFC1_RX1_0             (*(vu32*)(AXBAR_BASE + 0x060))
#define AXBAR_PART_0_SFC2_RX1_0             (*(vu32*)(AXBAR_BASE + 0x064))
#define AXBAR_PART_0_SFC3_RX1_0             (*(vu32*)(AXBAR_BASE + 0x068))
#define AXBAR_PART_0_SFC4_RX1_0             (*(vu32*)(AXBAR_BASE + 0x06C))
#define AXBAR_PART_0_MIXER1_RX0_0           (*(vu32*)(AXBAR_BASE + 0x080))
#define AXBAR_PART_0_MIXER1_RX1_0           (*(vu32*)(AXBAR_BASE + 0x084))
#define AXBAR_PART_0_MIXER1_RX2_0           (*(vu32*)(AXBAR_BASE + 0x088))
#define AXBAR_PART_0_MIXER1_RX3_0           (*(vu32*)(AXBAR_BASE + 0x08C))
#define AXBAR_PART_0_MIXER1_RX4_0           (*(vu32*)(AXBAR_BASE + 0x090))
#define AXBAR_PART_0_MIXER1_RX5_0           (*(vu32*)(AXBAR_BASE + 0x094))
#define AXBAR_PART_0_MIXER1_RX6_0           (*(vu32*)(AXBAR_BASE + 0x098))
#define AXBAR_PART_0_MIXER1_RX7_0           (*(vu32*)(AXBAR_BASE + 0x09C))
#define AXBAR_PART_0_MIXER1_RX8_0           (*(vu32*)(AXBAR_BASE + 0x0A0))
#define AXBAR_PART_0_MIXER1_RX9_0           (*(vu32*)(AXBAR_BASE + 0x0A4))

// AXBAR bits
#define SFC4_TX1 BIT(27)
#define SFC3_TX1 BIT(26)
#define SFC2_TX1 BIT(25)
#define SFC1_TX1 BIT(24)
#define I2S5_TX1 BIT(20)
#define I2S4_TX1 BIT(19)
#define I2S3_TX1 BIT(18)
#define I2S2_TX1 BIT(17)
#define I2S1_TX1 BIT(16)
#define ADMAIF_TX10 BIT(9)
#define ADMAIF_TX9 BIT(8)
#define ADMAIF_TX8 BIT(7)
#define ADMAIF_TX7 BIT(6)
#define ADMAIF_TX6 BIT(5)
#define ADMAIF_TX5 BIT(4)
#define ADMAIF_TX4 BIT(3)
#define ADMAIF_TX3 BIT(2)
#define ADMAIF_TX2 BIT(1)
#define ADMAIF_TX1 BIT(0)

#define CIF_UNPACK16 BIT(30)

void ahub_init(void);
void ahub_shutdown(void);
void adma_init_out(int channel, bool loop);
void adma_init_in(int channel, bool loop);
void adma_set_source(int channel, void* addr, u32 size);
void adma_set_target(int channel, void* addr, u32 size);
void adma_wait(int channel);
void adma_start(int channel);
void adma_stop(int channel);
void ahub_tx_test(void);

#endif // _AHUB_H_
