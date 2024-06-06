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

#include <utils/types.h>

#include <audio/ahub.h>
#include <soc/timer.h>
#include <soc/bpmp.h>

#include <stdio.h>
#include <stdlib.h>
//#include <avpc.h>
#include <malloc.h>
#include <string.h>

#define OUT_BUF_SIZE 0x80000
#define IN_BUF_SIZE 0x8000

s16 *output_buf;
s16 *input_buf;

u32 ahub_get_cif(int is_receive, u32 channels, u32 bits_per_sample, u32 fifo_threshold)
{
	u32 audio_bits = (bits_per_sample >> 2) - 1;

	return (channels - 1) << 20 |
	       (channels - 1) << 16 |
	       audio_bits << 12 |
	       audio_bits << 8 |
	       (fifo_threshold - 1) << 24;
	       //(is_receive ? 1 << 2 : 0);
}


void ahub_init(void)
{
    output_buf = memalign(0x1000, OUT_BUF_SIZE);
    input_buf = memalign(0x1000, IN_BUF_SIZE);

    memset(output_buf, 0, OUT_BUF_SIZE);
    memset(input_buf, 0, IN_BUF_SIZE);

    u32 fifo_threshold = 4;
    u8 bits_per_sample = 16;


    ADMAIF_GLOBAL_ENABLE_0 = 1;
    ADMA_GLOBAL_CMD_0 = 1;

    I2S_CG_0(1) = 0;
    I2S_CTRL_0(1) = 0x400;
    for (int i = 0; i < 5; i++)
    {
        if (!I2S_SOFT_RESET_0(1) & 1) break;
        usleep(10);
    }
    I2S_SOFT_RESET_0(1) = 0;
    I2S_ENABLE_0(1) = 1;

    // Timing stuff, currently "set" for 46.84KHz.
    // Heavily dependent on CAR I2S + AUD_MCLK (EXTPERIPH1) clocks
    I2S_CTRL_0(1) = I2S_CTRL_FRAME_FORMAT_BASIC | I2S_CTRL_BIT_CNT_16 | I2S_CTRL_LRCK_POLARITY_LOW
                    | I2S_FSYNC_WIDTH(bits_per_sample) | I2S_CTRL_EDGE_CTRL_POS_EDGE | I2S_CTRL_MASTER;
    I2S_TIMING_0(1) = 31;

    // Set I2S (spkrs) AXBAR settings for what data it should expect
    I2S_AXBAR_TX_CTRL_0(1) = I2S_RXTX_DATA_OFFSET(1) | I2S_NOHIGHZ;
    I2S_SLOT_CTRL_0(1) = 0;
    I2S_AXBAR_TX_SLOT_CTRL_0(1) = 0;
    I2S_AXBAR_TX_CIF_CTRL_0(1) = ahub_get_cif(1, 2, bits_per_sample, fifo_threshold);

    // Set I2S (mic) AXBAR settings for what data it should be sending
    I2S_AXBAR_RX_CTRL_0(1) = I2S_RXTX_DATA_OFFSET(1) | I2S_NOHIGHZ;
    I2S_AXBAR_RX_SLOT_CTRL_0(1) = 0;
    I2S_AXBAR_RX_CIF_CTRL_0(1) = ahub_get_cif(1, 2, bits_per_sample, fifo_threshold);

    // Set up ADMAIF TX0 and RX0 settings, set I2S (spkr) to receive from ADMAIF TX1 and ADMAIF RX1 to receive from I2S (mic)
    ADMAIF_AXBAR_TX0_CIF_CTRL_0 = ahub_get_cif(1, 2, bits_per_sample, fifo_threshold) | CIF_UNPACK16;
    ADMAIF_AXBAR_RX0_CIF_CTRL_0 = ahub_get_cif(1, 2, bits_per_sample, fifo_threshold) | CIF_UNPACK16;
    ADMAIF_AXBAR_TX0_FIFO_CTRL_0 |= BIT(31); // PIO, for manual driving w/ FIFO_WRITE
    ADMAIF_AXBAR_RX0_FIFO_CTRL_0 |= BIT(31);
    //ADMAIF_AXBAR_TX0_FIFO_CTRL_0 &= ~BIT(31); // DMA
    //ADMAIF_AXBAR_RX0_FIFO_CTRL_0 &= ~BIT(31);
    AXBAR_PART_0_I2S1_RX1_0 = ADMAIF_TX1;
    AXBAR_PART_0_ADMAIF_RX0_0 = I2S1_TX1;

    // Enable AXBAR + I2S TX
    ADMAIF_AXBAR_TX0_ENABLE_0 = 1; // added
    I2S_AXBAR_RX_ENABLE_0(1) = 1; // added

    // Enable AXBAR + I2S RX (mic)
    ADMAIF_AXBAR_RX0_ENABLE_0 = 1; // added for mic
    I2S_AXBAR_TX_ENABLE_0(1) = 1; //added for mic

    // Final pipeline is
    // ADMA -> AXBAR TX0 -> I2S
    // I2S -> AXBAR RX0 -> ADMA
}

void ahub_shutdown(void)
{
    adma_stop(0);
}

void adma_init_out(int channel, bool loop)
{
    if (channel >= ADMA_NUM_CHANNELS) return;

    // Reset DMA channel
    ADMA_CHn_SOFT_RESET_0(channel) = 1;
    for (int i = 0; i < 5; i++)
    {
        if (!ADMA_CHn_SOFT_RESET_0(channel) & 1) break;
        usleep(10);
    }
    ADMA_CHn_SOFT_RESET_0(channel) = 0;

    // Configured to output to TX1
    ADMA_CHn_CTRL_0(channel) = ADMA_FLOWCTRL_ENABLE | (loop ? ADMA_TRANSFER_MODE_CONTINUOUS : ADMA_TRANSFER_MODE_ONCE) | ADMA_TRANSFER_DIRECTION_MEMORY_TO_AHUB | ADMA_TX_REQUEST_SELECT(1);
    ADMA_CHn_CONFIG_0(channel) = ADMA_TARGET_ADDR_WRAP_ON_1_WORD | ADMA_BURST_SIZE_WORDS_16;
}

void adma_init_in(int channel, bool loop)
{
    if (channel >= ADMA_NUM_CHANNELS) return;

    // Reset DMA channel
    ADMA_CHn_SOFT_RESET_0(channel) = 1;
    for (int i = 0; i < 5; i++)
    {
        if (!ADMA_CHn_SOFT_RESET_0(channel) & 1) break;
        usleep(10);
    }
    ADMA_CHn_SOFT_RESET_0(channel) = 0;

    // Configured to read from RX0
    ADMA_CHn_CTRL_0(channel) = ADMA_FLOWCTRL_ENABLE | (loop ? ADMA_TRANSFER_MODE_CONTINUOUS : ADMA_TRANSFER_MODE_ONCE) | ADMA_TRANSFER_DIRECTION_AHUB_TO_MEMORY | ADMA_RX_REQUEST_SELECT(1);
    ADMA_CHn_CONFIG_0(channel) = ADMA_SOURCE_ADDR_WRAP_ON_1_WORD | ADMA_BURST_SIZE_WORDS_16;
}

void adma_set_source(int channel, void* addr, u32 size)
{
    ADMA_CHn_LOWER_SOURCE_ADDR_0(channel) = (intptr_t)addr;
    ADMA_CHn_TC_0(channel) = size;
}

void adma_set_target(int channel, void* addr, u32 size)
{
    ADMA_CHn_LOWER_TARGET_ADDR_0(channel) = (intptr_t)addr;
    ADMA_CHn_TC_0(channel) = size;
}

void adma_wait(int channel)
{
    while (ADMA_CHn_STATUS_0(channel) & ADMA_TRANSFER_ENABLED);
}

void adma_start(int channel)
{
    ADMA_CHn_CMD_0(channel) = ADMA_TRANSFER_ENABLE;
}

void adma_stop(int channel)
{
    ADMA_CHn_CMD_0(channel) = ADMA_TRANSFER_DISABLE;
}

void ahub_tx_test(void)
{
    // mic in
    //adma_init_in(1, false);
    //adma_set_target(1, input_buf, IN_BUF_SIZE);

#if 1
    // Waveform generation
    int index = 0;
    int amt = 80;
    int vol = 0x10;
    u32 buf_slice = 0x1C000/8;
    while (index + 2 < buf_slice / sizeof(s16))
    {
        for (int i = 0; i < amt/2; i++)
        {
            // Left/Right
            output_buf[index++] = vol;
            output_buf[index++] = vol;
        }

        if (index + 2 >= buf_slice / sizeof(s16)) break;

        for (int i = 0; i < amt/2; i++)
        {
            output_buf[index++] = -vol;
            output_buf[index++] = -vol;
        }
    }

    // Flush to DRAM
    bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLEAN_WAY, false);

    /*adma_stop(0);
    adma_init_out(0, false);
    adma_set_source(0, output_buf, buf_slice);
    adma_start(0);*/
    //adma_wait(0);

    for (int i = 0; i < buf_slice; i++)
    {
        while(ADMAIF_GLOBAL_TX_ACIF_FIFO_FULL_STATUS_0 & BIT(0));
        //u16 val = (i >> 3) & 1 ? 0x7FFF : 0x8000;
        ADMAIF_AXBAR_TX0_FIFO_WRITE_0 = output_buf[i];
        //usleep(1);
    }

    /*for (int i = 0; i < 1000; i++)
    {
        adma_wait(0);
        adma_start(0);
        msleep(1);
    }*/
#endif

#if 0
    // WAV read
    vfile_t file = { 0 };
    off_t read = 0;

    vfile_open(&file, "sdmc:/apple_startup.wav", FA_READ);
    vfile_read(&file, 0, output_buf, 0x2C, &read);
    vfile_read(&file, 0, output_buf, 0x72E84-0x2C, &read);
    vfile_close(&file);

    vfile_open(&file, "sdmc:/apple_logo.bin", FA_READ);
    vfile_read(&file, 0, (void*)0xC0000000+0x100000, 1280*768*4, &read);
    vfile_close(&file);

    fb_framebuffer((void*)0xC0000000+0x100000, false);

    adma_start(0);
    while (1);
#endif

    // Non-continuous
    /*for (int i = 0; i < 1000; i++)
    {
        adma_wait(0);
        adma_start(0);
    }*/

    // Continuous
    //adma_start(0);
    //adma_start(1); // mic

    // Direct feeding
    /*for (int i = 0; i < 3000000; i++)
    {
        u16 val = (i >> 3) & 1 ? 0x7FFF : 0x8000;
        ADMAIF_AXBAR_TX0_FIFO_WRITE_0 = val;
        usleep(1);
    }*/
}
