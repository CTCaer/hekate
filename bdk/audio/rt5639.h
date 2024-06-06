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

#ifndef _RT5639_H_
#define _RT5639_H_

/* Info */
#define RT5639_RESET				0x00
#define RT5639_VENDOR_ID			0xfd
#define RT5639_VENDOR_ID1			0xfe
#define RT5639_VENDOR_ID2			0xff

/*  I/O - Output */
#define RT5639_SPK_VOL				0x01
#define RT5639_HP_VOL				0x02
#define RT5639_OUTPUT				0x03
#define RT5639_MONO_OUT				0x04

/* I/O - Input */
#define RT5639_IN1_IN2				0x0d
#define RT5639_IN3_IN4				0x0e
#define RT5639_INL_INR_VOL			0x0f

/* I/O - ADC/DAC/DMIC */
#define RT5639_DAC1_DIG_VOL			0x19
#define RT5639_DAC2_DIG_VOL			0x1a
#define RT5639_DAC2_CTRL			0x1b
#define RT5639_ADC_DIG_VOL			0x1c
#define RT5639_ADC_DATA				0x1d
#define RT5639_ADC_BST_VOL			0x1e

/* Mixer - D-D */
#define RT5639_STO_ADC_MIXER		0x27
#define RT5639_MONO_ADC_MIXER		0x28
#define RT5639_AD_DA_MIXER			0x29
#define RT5639_STO_DAC_MIXER		0x2a
#define RT5639_MONO_DAC_MIXER		0x2b
#define RT5639_DIG_MIXER			0x2c
#define RT5639_DSP_PATH1			0x2d
#define RT5639_DSP_PATH2			0x2e
#define RT5639_DIG_INF_DATA			0x2f

/* Mixer - ADC */
#define RT5639_REC_L1_MIXER			0x3b
#define RT5639_REC_L2_MIXER			0x3c
#define RT5639_REC_R1_MIXER			0x3d
#define RT5639_REC_R2_MIXER			0x3e

/* Mixer - DAC */
#define RT5639_HPO_MIXER			0x45
#define RT5639_SPK_L_MIXER			0x46
#define RT5639_SPK_R_MIXER			0x47
#define RT5639_SPO_L_MIXER			0x48
#define RT5639_SPO_R_MIXER			0x49
#define RT5639_SPO_CLSD_RATIO		0x4a
#define RT5639_MONO_MIXER			0x4c
#define RT5639_OUT_L1_MIXER			0x4d
#define RT5639_OUT_L2_MIXER			0x4e
#define RT5639_OUT_L3_MIXER			0x4f
#define RT5639_OUT_R1_MIXER			0x50
#define RT5639_OUT_R2_MIXER			0x51
#define RT5639_OUT_R3_MIXER			0x52
#define RT5639_LOUT_MIXER			0x53

/* Power */
#define RT5639_PWR_DIG1				0x61
#define RT5639_PWR_DIG2				0x62
#define RT5639_PWR_ANLG1			0x63
#define RT5639_PWR_ANLG2			0x64
#define RT5639_PWR_MIXER			0x65
#define RT5639_PWR_VOL				0x66

/* Private Register Control */
#define RT5639_PRIV_INDEX			0x6a
#define RT5639_PRIV_DATA			0x6c

/* Format - ADC/DAC */
#define RT5639_I2S1_SDP				0x70
#define RT5639_I2S2_SDP				0x71
#define RT5639_ADDA_CLK1			0x73
#define RT5639_ADDA_CLK2			0x74
#define RT5639_DMIC				    0x75

/* Function - Analog */
#define RT5639_GLB_CLK				0x80
#define RT5639_PLL_CTRL1			0x81
#define RT5639_PLL_CTRL2			0x82
#define RT5639_ASRC_1				0x83
#define RT5639_ASRC_2				0x84
#define RT5639_ASRC_3				0x85
#define RT5639_ASRC_4				0x89
#define RT5639_ASRC_5				0x8a
#define RT5639_HP_OVCD				0x8b
#define RT5639_CLS_D_OVCD			0x8c
#define RT5639_CLS_D_OUT			0x8d
#define RT5639_DEPOP_M1				0x8e
#define RT5639_DEPOP_M2				0x8f
#define RT5639_DEPOP_M3				0x90
#define RT5639_CHARGE_PUMP			0x91
#define RT5639_PV_DET_SPK_G			0x92
#define RT5639_MICBIAS				0x93

/* Function - Digital */
#define RT5639_EQ_CTRL1				0xb0
#define RT5639_EQ_CTRL2				0xb1
#define RT5639_WIND_FILTER			0xb2
#define RT5639_DRC_AGC_1			0xb4
#define RT5639_DRC_AGC_2			0xb5
#define RT5639_DRC_AGC_3			0xb6
#define RT5639_SVOL_ZC				0xb7
#define RT5639_ANC_CTRL1			0xb8
#define RT5639_ANC_CTRL2			0xb9
#define RT5639_ANC_CTRL3			0xba
#define RT5639_JD_CTRL				0xbb
#define RT5639_ANC_JD				0xbc
#define RT5639_IRQ_CTRL1			0xbd
#define RT5639_IRQ_CTRL2			0xbe
#define RT5639_INT_IRQ_ST			0xbf
#define RT5639_GPIO_CTRL1			0xc0
#define RT5639_GPIO_CTRL2			0xc1
#define RT5639_GPIO_CTRL3			0xc2
#define RT5639_DSP_CTRL1			0xc4
#define RT5639_DSP_CTRL2			0xc5
#define RT5639_DSP_CTRL3			0xc6
#define RT5639_DSP_CTRL4			0xc7
#define RT5639_PGM_REG_ARR1			0xc8
#define RT5639_PGM_REG_ARR2			0xc9
#define RT5639_PGM_REG_ARR3			0xca
#define RT5639_PGM_REG_ARR4			0xcb
#define RT5639_PGM_REG_ARR5			0xcc
#define RT5639_SCB_FUNC				0xcd
#define RT5639_SCB_CTRL				0xce
#define RT5639_BASE_BACK			0xcf
#define RT5639_MP3_PLUS1			0xd0
#define RT5639_MP3_PLUS2			0xd1
#define RT5639_3D_HP				0xd2
#define RT5639_ADJ_HPF				0xd3
#define RT5639_HP_CALIB_AMP_DET		0xd6
#define RT5639_HP_CALIB2			0xd7
#define RT5639_SV_ZCD1				0xd9
#define RT5639_SV_ZCD2				0xda

/* Dummy Register */
#define RT5639_DUMMY1				0xfa
#define RT5639_DUMMY2				0xfb
#define RT5639_DUMMY3				0xfc

/* Index of Codec Private Register definition */
#define RT5639_BIAS_CUR1			0x12
#define RT5639_BIAS_CUR3			0x14
#define RT5639_BIAS_CUR4			0x15
#define RT5639_CHPUMP_INT_REG1		0x24
#define RT5639_MAMP_INT_REG2		0x37
#define RT5639_CHOP_DAC_ADC			0x3d
#define RT5639_3D_SPK				0x63
#define RT5639_WND_1				0x6c
#define RT5639_WND_2				0x6d
#define RT5639_WND_3				0x6e
#define RT5639_WND_4				0x6f
#define RT5639_WND_5				0x70
#define RT5639_WND_8				0x73
#define RT5639_DIP_SPK_INF			0x75
#define RT5639_HP_DCC_INT1			0x77
#define RT5639_EQ_BW_LOP			0xa0
#define RT5639_EQ_GN_LOP			0xa1
#define RT5639_EQ_FC_BP1			0xa2
#define RT5639_EQ_BW_BP1			0xa3
#define RT5639_EQ_GN_BP1			0xa4
#define RT5639_EQ_FC_BP2			0xa5
#define RT5639_EQ_BW_BP2			0xa6
#define RT5639_EQ_GN_BP2			0xa7
#define RT5639_EQ_FC_BP3			0xa8
#define RT5639_EQ_BW_BP3			0xa9
#define RT5639_EQ_GN_BP3			0xaa
#define RT5639_EQ_FC_BP4			0xab
#define RT5639_EQ_BW_BP4			0xac
#define RT5639_EQ_GN_BP4			0xad
#define RT5639_EQ_FC_HIP1			0xae
#define RT5639_EQ_GN_HIP1			0xaf
#define RT5639_EQ_FC_HIP2			0xb0
#define RT5639_EQ_BW_HIP2			0xb1
#define RT5639_EQ_GN_HIP2			0xb2
#define RT5639_EQ_PRE_VOL			0xb3
#define RT5639_EQ_PST_VOL			0xb4

/* SPOMIX bits */
#define RT5639_HPOMIX_MUTE_DAC1     BIT(14)
#define RT5639_HPOMIX_MUTE_HPOVOL   BIT(13)

/* SPOLMIX bits */
#define RT5639_SPOLMIX_MUTE_DACR1   BIT(15)
#define RT5639_SPOLMIX_MUTE_DACL1   BIT(14)
#define RT5639_SPOLMIX_MUTE_SPKVOLR BIT(13)
#define RT5639_SPOLMIX_MUTE_SPKVOLL BIT(12)
#define RT5639_SPOLMIX_MUTE_BST1    BIT(11)

/* SPORMIX bits */
#define RT5639_SPORMIX_MUTE_DACR1   BIT(13)
#define RT5639_SPORMIX_MUTE_SPKVOLR BIT(12)
#define RT5639_SPORMIX_MUTE_BST1    BIT(11)

#define RT5639_I2C_ADDR 0x1C

void rt5639_init(void);
void rt5639_init_2(void);

#endif // _RT5639_H_
