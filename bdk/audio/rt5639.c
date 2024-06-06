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

#include "rt5639.h"

#include <utils/types.h>
#include <soc/clock.h>
#include <soc/gpio.h>
#include <soc/i2c.h>
#include <soc/pinmux.h>
#include <soc/timer.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <power/max7762x.h>
#include <audio/ahub.h>

#include <stdio.h>

u16 rt5639_read16(u8 reg);

void rt5639_reset(void)
{
    i2c_send_buf_small(I2C_1, RT5639_I2C_ADDR, RT5639_RESET, (u8[2]){0,0}, 2);
}

void rt5639_write16(u8 reg, u16 val)
{
    u16 val_flip = byte_swap_16(val); // le -> be
    
    i2c_send_buf_small(I2C_1, RT5639_I2C_ADDR, reg, (u8*)&val_flip, 2);
}

u16 rt5639_read16(u8 reg)
{
    u16 val;

    i2c_recv_buf_small((u8*)&val, 2, I2C_1, RT5639_I2C_ADDR, reg);

    return byte_swap_16(val); // be -> le
}

void rt5639_pr_write16(u16 preg, u16 val)
{
    rt5639_write16(RT5639_PRIV_INDEX, preg);
    rt5639_write16(RT5639_PRIV_DATA, val);
}

u16 rt5639_pr_read16(u16 preg)
{
    rt5639_write16(RT5639_PRIV_INDEX, preg);
    return rt5639_read16(RT5639_PRIV_DATA);
}

void rt5639_setbits(u8 reg, u16 bits)
{
    rt5639_write16(reg, rt5639_read16(reg) | bits);
}

void rt5639_maskbits(u8 reg, u16 bits)
{
    rt5639_write16(reg, rt5639_read16(reg) & bits);
}

// TODO maybe just abstract this into clock.c
#define SET_CLK_ENB_APE                  (BIT(6))
#define CLR_APE_RST                      (BIT(6))
#define SET_CLK_ENB_APB2APE              (BIT(11))
#define CLR_APB2APE_RST              (BIT(11))
#define SET_APE_RST                     (BIT(6))
#define CLR_APE_RST                     (BIT(6))
void nx_ape_init(void)
{
    pmc_enable_partition(POWER_RAIL_AUD, 1);
    CLOCK(CLK_RST_CONTROLLER_CLK_ENB_V_SET) = SET_CLK_ENB_APB2APE; // TODO clock_enable_apb2ape()
    CLOCK(CLK_RST_CONTROLLER_CLK_ENB_Y_SET) = SET_CLK_ENB_APE; // TODO clock_enable_ape()

    usleep(2);
    pmc_unclamp_partition(POWER_RAIL_AUD);
    CLOCK(CLK_RST_CONTROLLER_RST_DEV_V_CLR) = CLR_APB2APE_RST;
    CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_CLR) = CLR_APE_RST;

    clock_enable_audio();
}

void nx_ape_fini(void)
{
    CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_SET) = SET_APE_RST;
}

void misc_audio_init()
{

    PINMUX_AUX(PINMUX_AUX_AUD_MCLK) = PINMUX_INPUT_ENABLE;
    PINMUX_AUX(PINMUX_AUX_DAP4_SCLK) = PINMUX_INPUT_ENABLE;
    PINMUX_AUX(PINMUX_AUX_DAP1_FS) = PINMUX_INPUT_ENABLE;
    PINMUX_AUX(PINMUX_AUX_DAP1_DIN) = PINMUX_INPUT_ENABLE;
    PINMUX_AUX(PINMUX_AUX_DAP1_DOUT) = PINMUX_INPUT_ENABLE;
    PINMUX_AUX(PINMUX_AUX_DAP1_SCLK) = PINMUX_INPUT_ENABLE;

    PMC(APBDEV_PMC_PWR_DET) |= (PMC_PWR_DET_AUDIO_HV);
    PMC(APBDEV_PMC_PWR_DET_VAL) &= ~(PMC_PWR_DET_AUDIO_HV); // 1.8v
    usleep(130);

    // AUDIO
    PMC(APBDEV_PMC_DPD_SAMPLE) = 1;
    PMC(APBDEV_PMC_IO_DPD_REQ) = PMC_IO_DPD_REQ_DPD_OFF | BIT(17) /*AUDIO*/;
    PMC(APBDEV_PMC_IO_DPD_OFF_MASK) &= ~BIT(17);
    usleep(250);
    PMC(APBDEV_PMC_DPD_SAMPLE) = 0;

    // AUDIO
    PMC(APBDEV_PMC_DPD_SAMPLE) = 1;
    PMC(APBDEV_PMC_IO_DPD2_REQ) = PMC_IO_DPD_REQ_DPD_OFF | BIT(29) /*AUDIO_HV*/;
    PMC(APBDEV_PMC_IO_DPD2_OFF_MASK) &= ~BIT(29);
    usleep(250);
    PMC(APBDEV_PMC_DPD_SAMPLE) = 0;

    PINMUX_AUX(PINMUX_AUX_AUD_MCLK) = 0;
    PINMUX_AUX(PINMUX_AUX_DAP1_FS) = 0;//PINMUX_INPUT;
    PINMUX_AUX(PINMUX_AUX_DAP1_DIN) = 0;//PINMUX_INPUT;
    PINMUX_AUX(PINMUX_AUX_DAP1_DOUT) = 0;//PINMUX_INPUT;
    PINMUX_AUX(PINMUX_AUX_DAP1_SCLK) = 0;//PINMUX_INPUT;

    // Accessing i2s registers (or any registers in the APE block)
    // will hang unless it is powered on.
    nx_ape_init();
}

void rt5639_init(void)
{
    misc_audio_init();

    PMC(APBDEV_PMC_NO_IOPOWER) &= ~PMC_NO_IOPOWER_AUDIO_HV;

    gpio_config(GPIO_PORT_BB, GPIO_PIN_0, GPIO_MODE_SPIO); // TODO what was this, i2s?
    gpio_config(GPIO_PORT_B, GPIO_PIN_0, GPIO_MODE_SPIO); // TODO what was this, i2s?
    gpio_config(GPIO_PORT_B, GPIO_PIN_1, GPIO_MODE_SPIO); // TODO what was this, i2s?
    gpio_config(GPIO_PORT_B, GPIO_PIN_2, GPIO_MODE_SPIO); // TODO what was this, i2s?
    gpio_config(GPIO_PORT_B, GPIO_PIN_3, GPIO_MODE_SPIO); // TODO what was this, i2s?

    APB_MISC(APB_MISC_GP_AUD_MCLK_CFGPADCTRL) = (0x10 << 20) | (0x10 << 12);
    APB_MISC(APB_MISC_DAS_DAP_CTRL_SEL) = BIT(31);
    APB_MISC(APB_MISC_DAS_DAC_INPUT_DATA_CLK_SEL) = 0;
    PMC(APBDEV_PMC_CLK_OUT_CNTRL) |= PMC_CLK_OUT_CNTRL_CLK1_FORCE_EN;

    //clock_enable_i2s_controller();
    clock_enable_i2s(0);

    /*max7762x_regulator_enable(REGULATOR_LDO0, true);
    max7762x_regulator_enable(REGULATOR_LDO1, true);
    max7762x_regulator_enable(REGULATOR_LDO2, true);
    max7762x_regulator_enable(REGULATOR_LDO3, true);
    max7762x_regulator_enable(REGULATOR_LDO4, true);
    max7762x_regulator_enable(REGULATOR_LDO5, true);
    max7762x_regulator_enable(REGULATOR_LDO6, true);
    max7762x_regulator_enable(REGULATOR_LDO7, true);
    max7762x_regulator_enable(REGULATOR_LDO8, true);*/
    pinmux_config_audio_codec();
    gpio_direction_output(GPIO_PORT_Z, GPIO_PIN_4, 1); // deassert codec reset
    gpio_direction_input(GPIO_PORT_BB, GPIO_PIN_4); // IRQ/alert, probably headphone removal
    msleep(20);

    rt5639_reset();
    msleep(200);

    // HOS init
    rt5639_write16(RT5639_HP_DCC_INT1, 0x9f00);
    rt5639_write16(RT5639_PWR_ANLG1, 0xA810);
    usleep(50000);
    rt5639_write16(RT5639_PWR_ANLG1, 0xE818);
    usleep(10000);
    

    rt5639_pr_write16(0x1B, 0x200); // unk priv
    rt5639_write16(RT5639_DUMMY1, 0x3E01); 
    //rt5639_write16(RT5639_ADDA_CLK1, 0x8814); 
	rt5639_write16(RT5639_ADDA_CLK1, 0x1114); // 16-bit
    
    rt5639_pr_write16(0x1D, 0x347); // unk priv
    rt5639_pr_write16(RT5639_CHOP_DAC_ADC, 0x3600); 
    rt5639_pr_write16(RT5639_BIAS_CUR1, 0xAA8); 
    rt5639_pr_write16(RT5639_BIAS_CUR3, 0x8AAA); 
    rt5639_pr_write16(0x20, 0x6110); // unk priv
    rt5639_pr_write16(0x23, 0x804); // unk priv
    
    
    rt5639_write16(RT5639_I2S1_SDP, 0x8000);
    rt5639_write16(RT5639_I2S2_SDP, 0x8000);
    rt5639_write16(RT5639_IRQ_CTRL1, 0x4000); 
    rt5639_write16(RT5639_IRQ_CTRL2, 0x8000); 
    rt5639_write16(RT5639_GPIO_CTRL1, 0x8400); 
    rt5639_write16(RT5639_GPIO_CTRL3, 4); 
    rt5639_write16(RT5639_JD_CTRL, 0); 
    rt5639_write16(RT5639_DUMMY2, 0); 
    rt5639_write16(RT5639_CLS_D_OUT, 0xA800); 
    rt5639_write16(RT5639_CLS_D_OVCD, 0x328); 
    rt5639_write16(RT5639_SPK_L_MIXER, 0x36); 
    rt5639_write16(RT5639_SPK_R_MIXER, 0x36); 
    rt5639_write16(RT5639_SPK_VOL, 0x8484); 
    rt5639_write16(RT5639_SPO_CLSD_RATIO, 4); 
    rt5639_write16(RT5639_SPO_L_MIXER, 0xE800); 
    rt5639_write16(RT5639_SPO_R_MIXER, 0x2800); 
    rt5639_write16(RT5639_DAC1_DIG_VOL, 0); 
    rt5639_write16(RT5639_OUT_L2_MIXER, 0); 
    rt5639_write16(RT5639_OUT_R2_MIXER, 0); 
    rt5639_write16(RT5639_OUT_L3_MIXER, 0x1FE); 
    rt5639_write16(RT5639_OUT_R3_MIXER, 0x1FE); 
    rt5639_write16(RT5639_HP_VOL, 0x1111); 
    rt5639_write16(RT5639_HPO_MIXER, 0xC000); 
    rt5639_write16(RT5639_STO_DAC_MIXER, 0x1414); 
    rt5639_pr_write16(RT5639_DEPOP_M3, 0x2000); 
    rt5639_pr_write16(RT5639_CHARGE_PUMP, 0x1000); 
    rt5639_write16(RT5639_DEPOP_M2, 0x1100); 
    rt5639_write16(RT5639_DEPOP_M3, 0x646); 
    rt5639_pr_write16(RT5639_MAMP_INT_REG2, 0xFC00); 
    rt5639_write16(RT5639_CHARGE_PUMP, 0xC00); 
    rt5639_write16(RT5639_IN1_IN2, 0x5000); 
    rt5639_write16(RT5639_IN3_IN4, 0); 
    rt5639_write16(RT5639_REC_L2_MIXER, 0x7F); 
    rt5639_write16(RT5639_REC_R2_MIXER, 0x7F); 
    rt5639_write16(RT5639_STO_ADC_MIXER, 0x3060); 
    rt5639_write16(RT5639_MICBIAS, 0x3800); 
    rt5639_write16(RT5639_DIG_INF_DATA, 0x2000); 
    rt5639_write16(RT5639_ADC_DIG_VOL, 0x2F80); 
    rt5639_write16(RT5639_PWR_DIG1, 0x8001); 
    rt5639_write16(RT5639_PWR_DIG2, 0); 
    rt5639_write16(RT5639_PWR_ANLG2, 0); 
    rt5639_write16(RT5639_PWR_MIXER, 0x3000); 
    rt5639_write16(RT5639_PWR_VOL, 0xC000); 


    rt5639_setbits(RT5639_PWR_DIG1, 0x1800);
    usleep(10000); 
    rt5639_maskbits(RT5639_SPK_VOL, 0x7F7F);

    bool headphones = false;

    // Activate speakers
    if (!headphones)
    {
        rt5639_write16(RT5639_DEPOP_M3, 0x646);
        rt5639_pr_write16(0x37, 0xFC00);
        rt5639_setbits(RT5639_DEPOP_M1, 0x4);
        rt5639_setbits(RT5639_DEPOP_M1, 0x20);
        rt5639_maskbits(RT5639_DEPOP_M1, 0xFCDF);
        rt5639_setbits(RT5639_DEPOP_M1, 0x300);
        rt5639_setbits(RT5639_HP_VOL, 0x8080);
        usleep(30000);
        rt5639_maskbits(RT5639_DEPOP_M1, 0x7CFF);
        rt5639_write16(RT5639_CHARGE_PUMP, 0xC00);
        rt5639_pr_write16(0xA0, 0xED87);
        rt5639_pr_write16(0xA1, 0);
        rt5639_pr_write16(RT5639_EQ_CTRL1, 0x1FB4);
        rt5639_pr_write16(RT5639_EQ_CTRL2, 0x4B);
        rt5639_pr_write16(0xB2, 0x1FB4);
        rt5639_pr_write16(0xB3, 0x800);
        rt5639_pr_write16(RT5639_DRC_AGC_1, 0x800);
        rt5639_write16(RT5639_EQ_CTRL2, 0xC1);
        rt5639_write16(RT5639_EQ_CTRL1, 0x6041);
        rt5639_write16(RT5639_DRC_AGC_2, 0x1F80);
        rt5639_write16(RT5639_DRC_AGC_3, 0x480);
        rt5639_write16(RT5639_DRC_AGC_1, 0x6B30);
        rt5639_maskbits(RT5639_PWR_MIXER, 0x3FFF);
        rt5639_maskbits(RT5639_PWR_VOL, 0xF3FF);
        rt5639_maskbits(RT5639_PWR_ANLG1, 0xFF1F);
        rt5639_maskbits(RT5639_DEPOP_M1, 0xFF66);
        rt5639_setbits(RT5639_DEPOP_M1, 0x80);
        rt5639_write16(RT5639_DEPOP_M2, 0x1100);
        rt5639_setbits(RT5639_PWR_DIG1, 1);
        rt5639_setbits(RT5639_PWR_MIXER, 0x3000);
        rt5639_setbits(RT5639_PWR_VOL, 0xC000);
        rt5639_maskbits(RT5639_SPK_VOL, 0x7F7F);
        
        rt5639_write16(RT5639_SPK_VOL, 0x0404); // 6dB gain left/right
    }
    else
    {
        // Activate headphones
        rt5639_setbits(RT5639_SPK_VOL, 0x8080);// mute sporp/spolp
        rt5639_maskbits(RT5639_PWR_DIG1, 0xFFFE);
        rt5639_maskbits(RT5639_PWR_MIXER, 0xCFFF);
        rt5639_maskbits(RT5639_PWR_VOL, 0x3FFF);
        rt5639_write16(RT5639_DEPOP_M2, 0x3100);
        rt5639_write16(RT5639_DEPOP_M1, 0x9);
        rt5639_setbits(RT5639_PWR_ANLG1, 0xE0);
        rt5639_setbits(RT5639_DEPOP_M1, 0x14);
        rt5639_setbits(RT5639_PWR_MIXER, 0xC000);
        rt5639_setbits(RT5639_PWR_VOL, 0xC00);
        rt5639_write16(RT5639_CHARGE_PUMP, 0xE00);
        rt5639_write16(RT5639_EQ_CTRL2, 0);
        rt5639_write16(RT5639_EQ_CTRL1, 0x6000);
        rt5639_write16(RT5639_DRC_AGC_1, 0x2206);
        rt5639_write16(RT5639_DEPOP_M3, 0x737);
        rt5639_write16(RT5639_PRIV_INDEX, 0x37);
        rt5639_write16(RT5639_PRIV_DATA, 0xFC00);
        rt5639_setbits(RT5639_DEPOP_M1, 0x8000);
        rt5639_setbits(RT5639_DEPOP_M1, 0x40);
        rt5639_maskbits(RT5639_DEPOP_M1, 0xFCBF);
        rt5639_setbits(RT5639_DEPOP_M1, 0x300);
        rt5639_maskbits(RT5639_HP_VOL, 0x7F7F);
        usleep(100000);
        rt5639_maskbits(RT5639_DEPOP_M1, 0xFCFB);
        
        rt5639_write16(RT5639_HP_VOL, 0x0000);
    }

    rt5639_write16(RT5639_DAC1_DIG_VOL, 0x7F7F); // 0dB attenuation left/right

#if 0
    // Enable mic
    rt5639_write16(RT5639_PWR_DIG2, 0x8000);   // stereo adc digital filter power control
    rt5639_write16(RT5639_PWR_ANLG2, 0x8800);  // mic bst1, micbias1
    rt5639_setbits(RT5639_PWR_DIG1, 4);        // analog adcl control
    rt5639_setbits(RT5639_PWR_MIXER, 0x800);   // RECMIXL power
    rt5639_setbits(RT5639_PWR_ANLG1, 4);       // LDO2 power
    rt5639_maskbits(RT5639_REC_L2_MIXER, 0xFFFD);
    rt5639_write16(RT5639_ADC_DIG_VOL, 0x2f80);
#endif

	ahub_init();
	ahub_tx_test();
}
