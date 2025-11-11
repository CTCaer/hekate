/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 shuffle2
 * Copyright (c) 2018 balika011
 * Copyright (c) 2019-2025 CTCaer
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

#ifndef _FUSE_H_
#define _FUSE_H_

#include <utils/types.h>

/*! Fuse registers. */
#define FUSE_CTRL                             0x0
#define FUSE_ADDR                             0x4
#define FUSE_RDATA                            0x8
#define FUSE_WDATA                            0xC
#define FUSE_TIME_RD1                         0x10
#define FUSE_TIME_RD2                         0x14
#define FUSE_TIME_PGM1                        0x18
#define FUSE_TIME_PGM2                        0x1C
#define FUSE_PRIV2INTFC                       0x20
#define  FUSE_PRIV2INTFC_START_DATA            BIT(0)
#define  FUSE_PRIV2INTFC_SKIP_RECORDS          BIT(1)
#define FUSE_FUSEBYPASS                       0x24
#define FUSE_PRIVATEKEYDISABLE                0x28
#define  FUSE_PRIVKEY_DISABLE                  BIT(0)
#define  FUSE_PRIVKEY_TZ_STICKY_BIT            BIT(4)
#define FUSE_DISABLEREGPROGRAM                0x2C
#define FUSE_WRITE_ACCESS_SW                  0x30
#define FUSE_PWR_GOOD_SW                      0x34
#define FUSE_PRIV2RESHIFT                     0x3C
#define FUSE_FUSETIME_RD0                     0x40
#define FUSE_FUSETIME_RD1                     0x44
#define FUSE_FUSETIME_RD2                     0x48
#define FUSE_FUSETIME_RD3                     0x4C
#define FUSE_PRIVATE_KEY0_NONZERO             0x80
#define FUSE_PRIVATE_KEY1_NONZERO             0x84
#define FUSE_PRIVATE_KEY2_NONZERO             0x88
#define FUSE_PRIVATE_KEY3_NONZERO             0x8C
#define FUSE_PRIVATE_KEY4_NONZERO             0x90

/*! Fuse Cached registers */
#define FUSE_RESERVED_ODM8_B01                0x98
#define FUSE_RESERVED_ODM9_B01                0x9C
#define FUSE_RESERVED_ODM10_B01               0xA0
#define FUSE_RESERVED_ODM11_B01               0xA4
#define FUSE_RESERVED_ODM12_B01               0xA8
#define FUSE_RESERVED_ODM13_B01               0xAC
#define FUSE_RESERVED_ODM14_B01               0xB0
#define FUSE_RESERVED_ODM15_B01               0xB4
#define FUSE_RESERVED_ODM16_B01               0xB8
#define FUSE_RESERVED_ODM17_B01               0xBC
#define FUSE_RESERVED_ODM18_B01               0xC0
#define FUSE_RESERVED_ODM19_B01               0xC4
#define FUSE_RESERVED_ODM20_B01               0xC8
#define FUSE_RESERVED_ODM21_B01               0xCC
#define FUSE_KEK00_B01                        0xD0
#define FUSE_KEK01_B01                        0xD4
#define FUSE_KEK02_B01                        0xD8
#define FUSE_KEK03_B01                        0xDC
#define FUSE_BEK00_B01                        0xE0
#define FUSE_BEK01_B01                        0xE4
#define FUSE_BEK02_B01                        0xE8
#define FUSE_BEK03_B01                        0xEC
#define FUSE_OPT_RAM_RTSEL_TSMCSP_PO4SVT_B01  0xF0
#define FUSE_OPT_RAM_WTSEL_TSMCSP_PO4SVT_B01  0xF4
#define FUSE_OPT_RAM_RTSEL_TSMCPDP_PO4SVT_B01 0xF8
#define FUSE_OPT_RAM_MTSEL_TSMCPDP_PO4SVT_B01 0xFC

#define FUSE_PRODUCTION_MODE                  0x100
#define FUSE_JTAG_SECUREID_VALID              0x104
#define FUSE_ODM_LOCK                         0x108
#define FUSE_OPT_OPENGL_EN                    0x10C
#define FUSE_SKU_INFO                         0x110
#define FUSE_CPU_SPEEDO_0_CALIB               0x114
#define FUSE_CPU_IDDQ_CALIB                   0x118

#define FUSE_RESERVED_ODM22_B01               0x11C
#define FUSE_RESERVED_ODM23_B01               0x120
#define FUSE_RESERVED_ODM24_B01               0x124

#define FUSE_OPT_FT_REV                       0x128
#define FUSE_CPU_SPEEDO_1_CALIB               0x12C
#define FUSE_CPU_SPEEDO_2_CALIB               0x130
#define FUSE_SOC_SPEEDO_0_CALIB               0x134
#define FUSE_SOC_SPEEDO_1_CALIB               0x138
#define FUSE_SOC_SPEEDO_2_CALIB               0x13C
#define FUSE_SOC_IDDQ_CALIB                   0x140

#define FUSE_RESERVED_ODM25_B01               0x144

#define FUSE_FA                               0x148
#define FUSE_RESERVED_PRODUCTION              0x14C
#define FUSE_HDMI_LANE0_CALIB                 0x150
#define FUSE_HDMI_LANE1_CALIB                 0x154
#define FUSE_HDMI_LANE2_CALIB                 0x158
#define FUSE_HDMI_LANE3_CALIB                 0x15C
#define FUSE_ENCRYPTION_RATE                  0x160
#define FUSE_PUBLIC_KEY0                      0x164
#define FUSE_PUBLIC_KEY1                      0x168
#define FUSE_PUBLIC_KEY2                      0x16C
#define FUSE_PUBLIC_KEY3                      0x170
#define FUSE_PUBLIC_KEY4                      0x174
#define FUSE_PUBLIC_KEY5                      0x178
#define FUSE_PUBLIC_KEY6                      0x17C
#define FUSE_PUBLIC_KEY7                      0x180
#define FUSE_TSENSOR1_CALIB                   0x184 // CPU1.
#define FUSE_TSENSOR2_CALIB                   0x188 // CPU2.

#define FUSE_OPT_SECURE_SCC_DIS_B01           0x18C

#define FUSE_OPT_CP_REV                       0x190 // FUSE style revision - ATE. 0x101 0x100
#define FUSE_OPT_PFG                          0x194
#define FUSE_TSENSOR0_CALIB                   0x198 // CPU0.
#define FUSE_FIRST_BOOTROM_PATCH_SIZE         0x19C
#define FUSE_SECURITY_MODE                    0x1A0
#define FUSE_PRIVATE_KEY0                     0x1A4
#define FUSE_PRIVATE_KEY1                     0x1A8
#define FUSE_PRIVATE_KEY2                     0x1AC
#define FUSE_PRIVATE_KEY3                     0x1B0
#define FUSE_PRIVATE_KEY4                     0x1B4
#define FUSE_ARM_JTAG_DIS                     0x1B8
#define FUSE_BOOT_DEVICE_INFO                 0x1BC
#define FUSE_RESERVED_SW                      0x1C0
#define FUSE_OPT_VP9_DISABLE                  0x1C4

#define FUSE_RESERVED_ODM0                    0x1C8
#define FUSE_RESERVED_ODM1                    0x1CC
#define FUSE_RESERVED_ODM2                    0x1D0
#define FUSE_RESERVED_ODM3                    0x1D4
#define FUSE_RESERVED_ODM4                    0x1D8
#define FUSE_RESERVED_ODM5                    0x1DC
#define FUSE_RESERVED_ODM6                    0x1E0
#define FUSE_RESERVED_ODM7                    0x1E4

#define FUSE_OBS_DIS                          0x1E8

#define FUSE_OPT_NVJTAG_PROTECTION_ENABLE_B01 0x1EC

#define FUSE_USB_CALIB                        0x1F0
#define FUSE_SKU_DIRECT_CONFIG                0x1F4
#define FUSE_KFUSE_PRIVKEY_CTRL               0x1F8
#define FUSE_PACKAGE_INFO                     0x1FC
#define FUSE_OPT_VENDOR_CODE                  0x200
#define FUSE_OPT_FAB_CODE                     0x204
#define FUSE_OPT_LOT_CODE_0                   0x208
#define FUSE_OPT_LOT_CODE_1                   0x20C
#define FUSE_OPT_WAFER_ID                     0x210
#define FUSE_OPT_X_COORDINATE                 0x214
#define FUSE_OPT_Y_COORDINATE                 0x218
#define FUSE_OPT_SEC_DEBUG_EN                 0x21C
#define FUSE_OPT_OPS_RESERVED                 0x220
#define FUSE_SATA_CALIB                       0x224

#define FUSE_SPARE_REGISTER_ODM_B01           0x224

#define FUSE_GPU_IDDQ_CALIB	                  0x228
#define FUSE_TSENSOR3_CALIB                   0x22C // CPU3.
#define FUSE_CLOCK_BONDOUT0                   0x230
#define FUSE_CLOCK_BONDOUT1                   0x234

#define FUSE_RESERVED_ODM26_B01               0x238
#define FUSE_RESERVED_ODM27_B01               0x23C
#define FUSE_RESERVED_ODM28_B01               0x240 // MAX77812 phase configuration.

#define FUSE_OPT_SAMPLE_TYPE                  0x244
#define FUSE_OPT_SUBREVISION                  0x248 // "", "p", "q", "r". e.g: A01p.
#define FUSE_OPT_SW_RESERVED_0                0x24C
#define FUSE_OPT_SW_RESERVED_1                0x250
#define FUSE_TSENSOR4_CALIB                   0x254 // GPU.
#define FUSE_TSENSOR5_CALIB                   0x258 // MEM0.
#define FUSE_TSENSOR6_CALIB                   0x25C // MEM1.
#define FUSE_TSENSOR7_CALIB                   0x260 // PLLX.
#define FUSE_OPT_PRIV_SEC_DIS                 0x264
#define FUSE_PKC_DISABLE                      0x268

#define FUSE_BOOT_SECURITY_INFO_B01           0x268
#define FUSE_OPT_RAM_RTSEL_TSMCSP_PO4HVT_B01  0x26C
#define FUSE_OPT_RAM_WTSEL_TSMCSP_PO4HVT_B01  0x270
#define FUSE_OPT_RAM_RTSEL_TSMCPDP_PO4HVT_B01 0x274
#define FUSE_OPT_RAM_MTSEL_TSMCPDP_PO4HVT_B01 0x278

#define FUSE_FUSE2TSEC_DEBUG_DISABLE          0x27C
#define FUSE_TSENSOR_COMMON                   0x280
#define FUSE_OPT_CP_BIN                       0x284
#define FUSE_OPT_GPU_DISABLE                  0x288
#define FUSE_OPT_FT_BIN                       0x28C
#define FUSE_OPT_DONE_MAP                     0x290

#define FUSE_RESERVED_ODM29_B01               0x294

#define FUSE_APB2JTAG_DISABLE                 0x298
#define FUSE_ODM_INFO                         0x29C // Debug features disable.
#define FUSE_ARM_CRYPT_DE_FEATURE             0x2A8

#define FUSE_OPT_RAM_WTSEL_TSMCPDP_PO4SVT_B01 0x2B0
#define FUSE_OPT_RAM_RCT_TSMCDP_PO4SVT_B01    0x2B4
#define FUSE_OPT_RAM_WCT_TSMCDP_PO4SVT_B01    0x2B8
#define FUSE_OPT_RAM_KP_TSMCDP_PO4SVT_B01     0x2BC

#define FUSE_WOA_SKU_FLAG                     0x2C0
#define FUSE_ECO_RESERVE_1                    0x2C4
#define FUSE_GCPLEX_CONFIG_FUSE               0x2C8
#define  FUSE_GPU_VPR_AUTO_FETCH_DIS          BIT(0)
#define  FUSE_GPU_VPR_ENABLED                 BIT(1)
#define  FUSE_GPU_WPR_ENABLED                 BIT(2)
#define FUSE_PRODUCTION_MONTH                 0x2CC
#define FUSE_RAM_REPAIR_INDICATOR             0x2D0
#define FUSE_TSENSOR9_CALIB                   0x2D4 // AOTAG.
#define FUSE_VMIN_CALIBRATION                 0x2DC
#define FUSE_AGING_SENSOR_CALIBRATION         0x2E0
#define FUSE_DEBUG_AUTHENTICATION             0x2E4
#define FUSE_SECURE_PROVISION_INDEX           0x2E8
#define FUSE_SECURE_PROVISION_INFO            0x2EC
#define FUSE_OPT_GPU_DISABLE_CP1              0x2F0
#define FUSE_SPARE_ENDIS                      0x2F4
#define FUSE_ECO_RESERVE_0                    0x2F8 // AID.
#define FUSE_RESERVED_CALIB0                  0x304 // GPCPLL ADC Calibration.
#define FUSE_RESERVED_CALIB1                  0x308
#define FUSE_OPT_GPU_TPC0_DISABLE             0x30C
#define FUSE_OPT_GPU_TPC0_DISABLE_CP1         0x310
#define FUSE_OPT_CPU_DISABLE                  0x314
#define FUSE_OPT_CPU_DISABLE_CP1              0x318
#define FUSE_TSENSOR10_CALIB                  0x31C
#define FUSE_TSENSOR10_CALIB_AUX              0x320
#define FUSE_OPT_RAM_SVOP_DP                  0x324
#define FUSE_OPT_RAM_SVOP_PDP                 0x328
#define FUSE_OPT_RAM_SVOP_REG                 0x32C
#define FUSE_OPT_RAM_SVOP_SP                  0x330
#define FUSE_OPT_RAM_SVOP_SMPDP               0x334

#define FUSE_OPT_RAM_WTSEL_TSMCPDP_PO4HVT_B01 0x324
#define FUSE_OPT_RAM_RCT_TSMCDP_PO4HVT_B01    0x328
#define FUSE_OPT_RAM_WCT_TSMCDP_PO4HVT_B01    0x32c
#define FUSE_OPT_RAM_KP_TSMCDP_PO4HVT_B01     0x330
#define FUSE_OPT_RAM_SVOP_SP_B01              0x334

#define FUSE_OPT_GPU_TPC0_DISABLE_CP2         0x338
#define FUSE_OPT_GPU_TPC1_DISABLE             0x33C
#define FUSE_OPT_GPU_TPC1_DISABLE_CP1         0x340
#define FUSE_OPT_GPU_TPC1_DISABLE_CP2         0x344
#define FUSE_OPT_CPU_DISABLE_CP2              0x348
#define FUSE_OPT_GPU_DISABLE_CP2              0x34C
#define FUSE_USB_CALIB_EXT                    0x350
#define FUSE_RESERVED_FIELD                   0x354 // RMA.
#define FUSE_SPARE_REALIGNMENT_REG            0x37C
#define FUSE_SPARE_BIT_0                      0x380
//...
#define FUSE_SPARE_BIT_31                     0x3FC

/*! Fuse commands. */
#define FUSE_IDLE     0x0
#define FUSE_READ     0x1
#define FUSE_WRITE    0x2
#define FUSE_SENSE    0x3
#define FUSE_CMD_MASK 0x3

/*! Fuse status. */
#define FUSE_STATUS_RESET                   0
#define FUSE_STATUS_POST_RESET              1
#define FUSE_STATUS_LOAD_ROW0               2
#define FUSE_STATUS_LOAD_ROW1               3
#define FUSE_STATUS_IDLE                    4
#define FUSE_STATUS_READ_SETUP              5
#define FUSE_STATUS_READ_STROBE             6
#define FUSE_STATUS_SAMPLE_FUSES            7
#define FUSE_STATUS_READ_HOLD               8
#define FUSE_STATUS_FUSE_SRC_SETUP          9
#define FUSE_STATUS_WRITE_SETUP             10
#define FUSE_STATUS_WRITE_ADDR_SETUP        11
#define FUSE_STATUS_WRITE_PROGRAM           12
#define FUSE_STATUS_WRITE_ADDR_HOLD         13
#define FUSE_STATUS_FUSE_SRC_HOLD           14
#define FUSE_STATUS_LOAD_RIR                15
#define FUSE_STATUS_READ_BEFORE_WRITE_SETUP 16
#define FUSE_STATUS_READ_DEASSERT_PD        17

/*! Fuse cache registers. */
#define FUSE_RESERVED_ODMX(x) (0x1C8 + 4 * (x))

#define FUSE_ARRAY_WORDS_NUM     192
#define FUSE_ARRAY_WORDS_NUM_B01 256

enum
{
	FUSE_NX_HW_TYPE_ICOSA,
	FUSE_NX_HW_TYPE_IOWA,
	FUSE_NX_HW_TYPE_HOAG,
	FUSE_NX_HW_TYPE_AULA
};

enum
{
	FUSE_NX_HW_STATE_PROD,
	FUSE_NX_HW_STATE_DEV
};

void fuse_disable_program();
u32  fuse_read_odm(u32 idx);
u32  fuse_read_odm_keygen_rev();
u32  fuse_read_dramid(bool raw_id);
u32  fuse_read_hw_state();
u32  fuse_read_hw_type();
int  fuse_set_sbk();
void fuse_wait_idle();
void fuse_sense();
u32  fuse_read(u32 addr);
int  fuse_read_ipatch(void (*ipatch)(u32 offset, u32 value));
int  fuse_read_evp_thunk(u32 *iram_evp_thunks, u32 *iram_evp_thunks_len);
void fuse_read_array(u32 *words);
bool fuse_check_patched_rcm();

#endif
