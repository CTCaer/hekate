/*
 * Copyright (c) 2018-2022 CTCaer
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

#ifndef TEGRA_BCT_H
#define TEGRA_BCT_H

#include <utils/types.h>

#include <mem/sdram_param_t210.h>
#include <mem/sdram_param_t210b01.h>

#define BCT_IRAM_ADDRESS     0x40000100
#define BCT_IRAM_ADDRESS_B01 0x40000464

#define BCT_BOOT_DEV_NONE  0
#define BCT_BOOT_DEV_SPI   3
#define BCT_BOOT_DEV_SDMMC 4
#define BCT_BOOT_DEV_USB3  9
#define BCT_BOOT_DEV_UART  11

// ECID is checked for the following.
#define BCT_SEC_DBG_JTAG  BIT(0)
#define BCT_SEC_DBG_DEV   BIT(1)
#define BCT_SEC_DBG_SPNID BIT(2)
#define BCT_SEC_DBG_SPID  BIT(3)
#define BCT_SEC_DBG_NID   BIT(4) // ECID not checked for T210B01.
#define BCT_SEC_DBG_DBG   BIT(5) // ECID not checked for T210B01.

#define BCT_SDMMC_CFG_SDR25_SINGLE_PAGE 0
#define BCT_SDMMC_CFG_SDR52_MULTI_PAGE  1
#define BCT_SDMMC_CFG_SDR25_MULTI_PAGE  2
#define BCT_SDMMC_CFG_DDR52_MULTI_PAGE  3
#define BCT_SDMMC_CFG_DDR25_MULTI_PAGE  4

#define BCT_SPI_CFG_PIO_X1_20_4MHZ_SREAD 0
#define BCT_SPI_CFG_PIO_X1_19_2MHZ_SREAD 1
#define BCT_SPI_CFG_PIO_X4_51_0MHZ_QREAD 2

typedef struct _bct_bad_blocks_t
{
	u32 used_entries;     // For block table.
	u8  virtual_block_size_log2;
	u8  block_size_log2;
	u8  block_table[512]; // Each bit is a block (4096 blocks).

	u8  padding[10];
} bct_bad_blocks_t;

typedef struct _bct_sdmmc_dev_t210_t
{
	u32 clk_div;
	u32 bus_width;
	u8  power_class_max;
	u8  multi_block_max;
} bct_sdmmc_dev_t210_t;

typedef struct _bct_sdmmc_dev_t210b01_t
{
	u8 sdmmc_config;
	u8 power_class_max;
} bct_sdmmc_dev_t210b01_t;

typedef struct _bct_spi_dev_t210_t
{
	u32 clk_src;
	u8  clk_div;
	u8  cmd_read_fast;
	u8  page_size; // 0: 2KB, 1: 16KB.
} bct_spi_dev_t210_t;

typedef struct _bct_spi_dev_t210b01_t
{
	u8 spi_config;
} bct_spi_dev_t210b01_t;

typedef struct _bct_usb3_dev_t
{
	u8 clk_div;
	u8 root_port;
	u8 page_size; // 0: 2KB, 1: 16KB.
	u8 oc_pin;
	u8 vbus_en;
} bct_usb3_dev_t;

typedef union
{
	bct_sdmmc_dev_t210_t sdmmc_params;
	bct_spi_dev_t210_t   spi_params;
	bct_usb3_dev_t  usb3_params;

	u8 padding[64];
} bct_boot_dev_t210_t;

typedef union
{
	bct_sdmmc_dev_t210b01_t sdmmc_params;
	bct_spi_dev_t210b01_t   spi_params;

	u8 padding[64];
} bct_boot_dev_t210b01_t;

typedef struct _bct_bootloader_t210_t
{
	u32 version;
	u32 start_block;
	u32 start_page;

	u32 length;     // Should be bl size + 16 and aligned to 16.
	u32 load_addr;
	u32 entrypoint;
	u32 attributes; // ODM.

	u8  aes_cmac_hash[16];
	u8  rsa_pss_signature[256];
} bct_bootloader_t210_t;

typedef struct _bct_bootloader_t210b01_t
{
	u32 start_block;
	u32 start_page;
	u32 version;
	u32 padding;
} bct_bootloader_t210b01_t;

typedef struct _tegra_bct_t210_t
{
	/* Unsigned section */
	bct_bad_blocks_t bad_block_table;

	u8  rsa_modulus[256];
	u8  aes_cmac_hash[16];
	u8  rsa_pss_signature[256];

	u32 secprov_aes_key_num_insecure;
	u8  secprov_aes_key[32];

	u8  customer_data[204];

	/* Signed section */
	u8 random_aes_block[16];

	u32 ecid[4];

	u32 data_version; // 0x210001.

	u32 block_size_log2;
	u32 page_size_log2;
	u32 partition_size;

	u32 boot_dev_params_num;
	u32 boot_dev_type;
	bct_boot_dev_t210_t boot_dev_params;

	u32 dram_params_num;
	sdram_params_t210_t dram_params[4];

	u32 bootloader_num;
	bct_bootloader_t210_t bootLoader[4];
	u32 bootloader_failback_en;

	u32 sec_dbg_ctrl; // Copied to APBDEV_PMC_DEBUG_AUTHENTICATION. ECID checked.

	u32 secprov_aes_key_num_secure;

	u8  padding[20];
} tegra_bct_t210_t;

typedef struct _tegra_bct_t210b01_t
{
	/* Unsigned section */
	u32 rsa_key_size;
	u32 padding0[3];
	u8  rsa_modulus[256];
	u8  rsa_exponent[256];

	u8  aes_cmac_hash[16];
	u8  rsa_pss_signature[256];

	u8  secprov_aes_key[32];
	u32 secprov_aes_key_num_insecure;

	u8  padding_unsigned[12];

	u8  customer_data0[208];

	/* Signed section */
	u8  random_aes_block0[16];

	u32 boot_config0[4]; // Customer controlled features.

	/// Unused space allocated for customer usage.
	u32 customer_data1_signed[16];

	/* Encrypted section (optionally) */
	u8  random_aes_block1[16];

	u32 ecid[4];

	u32 data_version; // 0x210001.

	u32 block_size_log2;
	u32 page_size_log2;
	u32 partition_size;

	u32 boot_dev_params_num;
	u32 boot_dev_type;
	bct_boot_dev_t210b01_t boot_dev_params;

	u32 dram_params_num;
	sdram_params_t210b01_t dram_params[4];

	u32 bootloader_num;
	bct_bootloader_t210b01_t bootLoader[4];

	u32 sec_dbg_ctrl;      // Copied to APBDEV_PMC_DEBUG_AUTHENTICATION. ECID not checked.
	u32 sec_dbg_ctrl_ecid; // Copied to APBDEV_PMC_DEBUG_AUTHENTICATION. ECID checked.

	u32 boot_config1[4];   // Customer controlled features. bit0 AON TZRAM powergating enable

	u32 customer_data2_signed[16]; // u32[0]: bl attributes.

	u32 secprov_aes_key_num_secure;

	u8 padding_signed[388];
} tegra_bct_t210b01_t;

typedef struct _bootLoader_hdr_t210b01_t
{
	/* Unsigned section */
	u8  aes_cmac_hash[16];
	u8  rsa_pss_signature[256];

	/* Signed section */
	u8  salt[16]; // random_aes_block.
	u8  bootLoader_sha256[16];
	u32 version;
	u32 length;
	u32 load_addr;
	u32 entrypoint;
	u8  padding[16];
} bootLoader_hdr_t210b01_t;

#endif
