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

#ifndef _I2S_H_
#define _I2S_H_

#include <utils/types.h>

#define I2S1_BASE    ((void*)0x702D1000)
#define I2S2_BASE    ((void*)0x702D1100)
#define I2S3_BASE    ((void*)0x702D1200)
#define I2S4_BASE    ((void*)0x702D1300)
#define I2S5_BASE    ((void*)0x702D1400)

#define I2S_BASE(n)  (((n) == 1) ? I2S1_BASE : \
                      ((n) == 2) ? I2S2_BASE : \
                      ((n) == 3) ? I2S3_BASE : \
                      ((n) == 4) ? I2S4_BASE : \
                      ((n) == 5) ? I2S5_BASE : \
                      NULL)

#define I2S_AXBAR_RX_ENABLE_0(n)            (*(vu32*)(I2S_BASE(n) + 0x000))
#define I2S_AXBAR_RX_SOFT_RESET_0(n)        (*(vu32*)(I2S_BASE(n) + 0x004))
#define I2S_AXBAR_RX_STATUS_0(n)            (*(vu32*)(I2S_BASE(n) + 0x00c))
#define I2S_AXBAR_RX_INT_STATUS_0(n)        (*(vu32*)(I2S_BASE(n) + 0x010))
#define I2S_AXBAR_RX_INT_MASK_0(n)          (*(vu32*)(I2S_BASE(n) + 0x014))
#define I2S_AXBAR_RX_INT_SET_0(n)           (*(vu32*)(I2S_BASE(n) + 0x018))
#define I2S_AXBAR_RX_INT_CLEAR_0(n)         (*(vu32*)(I2S_BASE(n) + 0x01c))
#define I2S_AXBAR_RX_CIF_CTRL_0(n)          (*(vu32*)(I2S_BASE(n) + 0x020))
#define I2S_AXBAR_RX_CTRL_0(n)              (*(vu32*)(I2S_BASE(n) + 0x024))
#define I2S_AXBAR_RX_SLOT_CTRL_0(n)         (*(vu32*)(I2S_BASE(n) + 0x028))
#define I2S_AXBAR_RX_CLK_TRIM_0(n)          (*(vu32*)(I2S_BASE(n) + 0x02c))
#define I2S_AXBAR_TX_ENABLE_0(n)            (*(vu32*)(I2S_BASE(n) + 0x040))
#define I2S_AXBAR_TX_SOFT_RESET_0(n)        (*(vu32*)(I2S_BASE(n) + 0x044))
#define I2S_AXBAR_TX_STATUS_0(n)            (*(vu32*)(I2S_BASE(n) + 0x04c))
#define I2S_AXBAR_TX_INT_STATUS_0(n)        (*(vu32*)(I2S_BASE(n) + 0x050))
#define I2S_AXBAR_TX_INT_MASK_0(n)          (*(vu32*)(I2S_BASE(n) + 0x054))
#define I2S_AXBAR_TX_INT_SET_0(n)           (*(vu32*)(I2S_BASE(n) + 0x058))
#define I2S_AXBAR_TX_INT_CLEAR_0(n)         (*(vu32*)(I2S_BASE(n) + 0x05c))
#define I2S_AXBAR_TX_CIF_CTRL_0(n)          (*(vu32*)(I2S_BASE(n) + 0x060))
#define I2S_AXBAR_TX_CTRL_0(n)              (*(vu32*)(I2S_BASE(n) + 0x064))
#define I2S_AXBAR_TX_SLOT_CTRL_0(n)         (*(vu32*)(I2S_BASE(n) + 0x068))
#define I2S_AXBAR_TX_CLK_TRIM_0(n)          (*(vu32*)(I2S_BASE(n) + 0x06c))
#define I2S_ENABLE_0(n)                     (*(vu32*)(I2S_BASE(n) + 0x080))
#define I2S_SOFT_RESET_0(n)                 (*(vu32*)(I2S_BASE(n) + 0x084))
#define I2S_CG_0(n)                         (*(vu32*)(I2S_BASE(n) + 0x088))
#define I2S_STATUS_0(n)                     (*(vu32*)(I2S_BASE(n) + 0x08c))
#define I2S_INT_STATUS_0(n)                 (*(vu32*)(I2S_BASE(n) + 0x090))
#define I2S_CTRL_0(n)                       (*(vu32*)(I2S_BASE(n) + 0x0a0))
#define I2S_TIMING_0(n)                     (*(vu32*)(I2S_BASE(n) + 0x0a4))
#define I2S_SLOT_CTRL_0(n)                  (*(vu32*)(I2S_BASE(n) + 0x0a8))
#define I2S_CLK_TRIM_0(n)                   (*(vu32*)(I2S_BASE(n) + 0x0ac))

// I2S_AXBAR_RX_CTRL_0 / I2S_AXBAR_TX_CTRL_0
#define I2S_RXTX_DATA_OFFSET(n) ((n << 8) & 0x3F0)
#define I2S_NOHIGHZ               (0 << 1)
#define I2S_HIGHZ                 (1 << 1)
#define I2S_HIGHZ_ON_HALF_BIT_CLK (2 << 1)

// I2S_CTRL_0
#define I2S_CTRL_BIT_CNT_8  (1)
#define I2S_CTRL_BIT_CNT_12 (2)
#define I2S_CTRL_BIT_CNT_16 (3)
#define I2S_CTRL_BIT_CNT_20 (4)
#define I2S_CTRL_BIT_CNT_24 (5)
#define I2S_CTRL_BIT_CNT_28 (6)
#define I2S_CTRL_BIT_CNT_32 (7)
#define I2S_CTRL_LOOPBACK   BIT(8)
#define I2S_CTRL_LRCK_POLARITY_LOW  (0)
#define I2S_CTRL_LRCK_POLARITY_HIGH BIT(9)
#define I2S_CTRL_SLAVE  (0)
#define I2S_CTRL_MASTER BIT(10)
#define I2S_CTRL_FRAME_FORMAT_BASIC (0)
#define I2S_CTRL_FRAME_FORMAT_LJM   (0)
#define I2S_CTRL_FRAME_FORMAT_RJM   (0)
#define I2S_CTRL_FRAME_FORMAT_DSP   BIT(12)
#define I2S_CTRL_FRAME_FORMAT_PCM   BIT(12)
#define I2S_CTRL_FRAME_FORMAT_NW    BIT(12)
#define I2S_CTRL_FRAME_FORMAT_TDM   BIT(12)
#define I2S_CTRL_EDGE_CTRL_NEG_EDGE BIT(20)
#define I2S_CTRL_EDGE_CTRL_POS_EDGE (0)
#define I2S_FSYNC_WIDTH(n) ((n << 24) & 0xFF000000)

#endif // _I2S_H_
