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

#ifndef TEGRA_BIT_H
#define TEGRA_BIT_H

#include <utils/types.h>

#define BIT_ADDRESS 0x40000000

#define BIT_BOOT_TYPE_NONE     0
#define BIT_BOOT_TYPE_COLD     1
#define BIT_BOOT_TYPE_RCM      2
#define BIT_BOOT_TYPE_UART     3
#define BIT_BOOT_TYPE_EXIT_RCM 4

#define BIT_READ_STATUS_NONE             0
#define BIT_READ_STATUS_SUCCESS          1
#define BIT_READ_STATUS_VALIDATION_ERROR 2
#define BIT_READ_STATUS_HW_READ_ERROR    3

#define BIT_USB_CHARGE_DETECT_EN  BIT(0)
#define BIT_USB_CHARGE_LOW_BATT   BIT(1)
#define BIT_USB_CHARGE_CONNECTED  BIT(2)
#define BIT_USB_CHARGE_HI_CURRENT BIT(8)

// From T186 with some additions.
enum
{
	BIT_FLOW_STATUS_NONE                       = 0,
	BIT_FLOW_STATUS_IPATCHUNCORRECTED_ERROR    = 1,
	BIT_FLOW_STATUS_IPATCHSUCCESS              = 2,
	BIT_FLOW_STATUS_SETUPOSCCLK                = 3,
	BIT_FLOW_STATUS_LOWBAT_NOTCHARGED          = 4,
	BIT_FLOW_STATUS_LOWBAT_CHARGED             = 5,
	BIT_FLOW_STATUS_PLLPENABLED                = 6,
	BIT_FLOW_STATUS_MSSINITIALIZED             = 7,
	BIT_FLOW_STATUS_FABRICINITIALIZED          = 8,
	BIT_FLOW_STATUS_NONSECUREDISPATCHERENTRY   = 9,
	BIT_FLOW_STATUS_NONSECUREDISPATCHEREXIT    = 10,
	BIT_FLOW_STATUS_FAMODE                     = 11,
	BIT_FLOW_STATUS_PREPRODUCTIONMODEUART      = 12,
	BIT_FLOW_STATUS_PRODUCTIONMODE             = 13,
	BIT_FLOW_STATUS_ODMPRODUCTIONMODE          = 14,
	BIT_FLOW_STATUS_DBGRCMMODE                 = 15,
	BIT_FLOW_STATUS_RECOVERYMODE               = 16,
	BIT_FLOW_STATUS_SECUREDISPATCHERENTRY      = 17,
	BIT_FLOW_STATUS_SECUREDISPATCHEREXIT       = 18,
	BIT_FLOW_STATUS_RAMDUMPINIT                = 19,
	BIT_FLOW_STATUS_RAMDUMPEXIT                = 20,
	BIT_FLOW_STATUS_COLDBOOTENTRY              = 21,
	BIT_FLOW_STATUS_COLDBOOTEXIT               = 22,
	BIT_FLOW_STATUS_CBSETUPBOOTDEVICE          = 23,
	BIT_FLOW_STATUS_CBBCTDONE                  = 24,
	BIT_FLOW_STATUS_MSSREGIONUNINITIALIZED     = 25,
	BIT_FLOW_STATUS_MSSREGIONENABLEINITIALIZED = 26,
	BIT_FLOW_STATUS_CBMTSPREBOOTINIT           = 27,
	BIT_FLOW_STATUS_CBREINITSUCCESS            = 28,
	BIT_FLOW_STATUS_CBSDRAMINITSUCCESS         = 29,
	BIT_FLOW_STATUS_CBPAYLOADSUCCESS           = 30,
	BIT_FLOW_STATUS_RCMENTRY                   = 31,
	BIT_FLOW_STATUS_RCMEXIT                    = 32,
	BIT_FLOW_STATUS_SC7ENTRY                   = 33,
	BIT_FLOW_STATUS_SC7ACKWAYPOINT             = 34,
	BIT_FLOW_STATUS_SC7DBELLERROR              = 35,
	BIT_FLOW_STATUS_SC7EXIT                    = 36,
	BIT_FLOW_STATUS_SECUREBOOTENTRY            = 37,
	BIT_FLOW_STATUS_SECUREBOOTEXIT             = 38,
	BIT_FLOW_STATUS_DETECTEDWDTRESET           = 39,
	BIT_FLOW_STATUS_DETECTEDAOTAG_SENSORRESET  = 40,
	BIT_FLOW_STATUS_DETECTEDVFSENSORRESET      = 41,
	BIT_FLOW_STATUS_DETECTEDSWMAINRESET        = 42,
	BIT_FLOW_STATUS_DETECTEDHSMRESET           = 43,
	BIT_FLOW_STATUS_DETECTEDCSITEDBGRESET      = 44,
	BIT_FLOW_STATUS_DETECTEDSC7SPEWDT_0_RESET  = 45,
	BIT_FLOW_STATUS_DETECTEDSC7SPEWDT_1_RESET  = 46,
	BIT_FLOW_STATUS_DETECTEDSYSRESETN          = 47,
	BIT_FLOW_STATUS_CRYPTOINITENTRY            = 48,
	BIT_FLOW_STATUS_CRYPTOINITEXIT             = 49,
	BIT_FLOW_STATUS_SECUREEXITSTART            = 50
};

typedef struct _bit_flow_log_t
{
	u32	Init_time;
	u32	exit_time;
	u32	func_id;
	u32	func_status;
} bit_flow_log_t;

typedef struct _bit_boot_sdmmc_status_t210_t
{
	u8  fuses_bus_width;
	u8  fuses_voltage_range;
	u8  fuses_boo_mode_disable;
	u8  fuses_drd_mode;

	u32 card_type;
	u32 voltage_range;
	u8  bus_width;
	u8  power_class;
	u8  auto_cal_status;
	u8  padding;
	u32 cid[4];

	u32 pages_read;
	u32 crc_errors;
	u8  boot_from_boot_partition;
	u8  boot_mode_read_success;
} bit_boot_sdmmc_status_t210_t;

typedef struct _bit_boot_sdmmc_status_t210b01_t
{
	u8 clk_src;
	u8 clk_div;
	u8 clk_en;
	u8 clk_rst_status;
	u8 clk_div_internal;
	u8 data_mode;

	u8 fuses_bus_width;
	u8 fuses_drd_mode;
	u8 fuses_config;
	u8 fuses_read_mode;

	u8  card_type;
	u32 voltage_range;
	u8  bus_width;
	u8  power_class;
	u8  auto_cal_status;
	u32 cid[4];

	u32 pages_read;
	u32 crc_errors;
	u8 boot_from_boot_partition;

	u32 time_init;
	u32 time_controller_init;
	u32 time_card_enumeration;
	u32 time_cmd1_op_cond;
	u32 time_cmd2_cid;
	u32 time_cmd9_csd;
	u32 time_transfer_mode;
	u32 time_cmd8_ext_csd;
	u32 time_power_class;
	u32 time_bus_width_and_partition;
	u32 time_read;

	u32 payload_size;
} bit_boot_sdmmc_status_t210b01_t;

typedef struct _bit_boot_spi_status_t210_t
{
	u32 clk_src;
	u32 clk_div;
	u32 fast_read;

	u32 pages_read;
	u32 last_block_read;
	u32 last_page_read;

	u32 boot_status;
	u32 init_status;
	u32 read_status;
	u32 params_validated;
} bit_boot_spi_status_t210_t;

typedef struct _bit_boot_spi_status_t210b01_t
{
	u32 clk_en;
	u32 clk_rst_status;
	u32 mode;
	u32 bus_width;
	u32 clk_src;
	u32 clk_div;
	u32 fast_read;

	u32 pages_read;
	u32 last_block_read;
	u32 last_page_read;

	u32 boot_status;
	u32 init_status;
	u32 read_status;
	u32 params_validated;

	u32 time_qspi_init;
	u32 time_read_time;

	u32 payload_size;
} bit_boot_spi_status_t210b01_t;

typedef struct _bit_boot_usb3_status_t
{
	u8  port;
	u8  sense_key;
	u8  padding[2];
	u32 cur_csw_tag;
	u32 curr_cmd_csw_status;
	u32 curr_ep_xfer_failed_bytes;
	u32 periph_dev_type;
	u32 block_num;
	u32 last_logical_block_addr;
	u32 block_length;
	u32 usb3_ctxt;
	u32 init_res;
	u32 read_page_res;
	u32 xusb_driver_status;
	u32 dev_status;
	u32 ep_status;
} bit_boot_usb3_status_t;

typedef union _bit_boot_secondary_dev_status_t210_t
{
	bit_boot_sdmmc_status_t210_t sdmmc_status;
	bit_boot_spi_status_t210_t   spi_status;
	bit_boot_usb3_status_t       usb3_status;

	u8 padding[60];
} bit_boot_secondary_dev_status_t210_t;

typedef union _bit_boot_secondary_dev_status_t210b01_t
{
	bit_boot_sdmmc_status_t210b01_t sdmmc_status;
	bit_boot_spi_status_t210b01_t   spi_status;

	u8 padding[256];
} bit_boot_secondary_dev_status_t210b01_t;

typedef struct _bit_bl_state_t
{
	u32 read_status;

	u32 first_ecc_block;
	u32 first_ecc_page;
	u32 first_corrected_ecc_block;
	u32 first_corrected_ecc_page;
	u8  had_ecc_error;
	u8  had_crc_error;
	u8  had_corrected_ecc_error;
	u8  used_for_ecc_recovery;
} bit_bl_state_t;

typedef struct _tegra_bit_t210_t
{
	u32 brom_version;
	u32 data_version;
	u32 rcm_version;

	u32 boot_type;

	u32 primary_dev_type;
	u32 secondary_dev_type;

	u32 boot_time_log_init;
	u32 boot_time_log_exit;
	u32 bct_read_tick_cnt;
	u32 bl_read_tick_cnt;

	u32 osc_frequency;
	u8  dev_initialized;
	u8  dram_initialized;

	u8 force_recovery_bit_cleared; // APBDEV_PMC_SCRATCH0.
	u8 failback_bit_cleared;       // APBDEV_PMC_SCRATCH0.
	u8 failback_invoked;

	u8 irom_patch_status; // bit0-3: hamming status, bit7: patches exist.

	u8  bct_valid;
	u8  bct_status[9]; // Each bit is a block (72 blocks).
	u32 bct_last_journal_read;
	u32 bct_block;
	u32 bct_page;
	u32 bct_size;
	u32 bct_ptr;

	bit_bl_state_t bl_state[4];

	bit_boot_secondary_dev_status_t210_t secondary_dev_status;

	u32 usb_charging_status;

	u32 safe_start_addr; // Init: 0x400000F4 / UART: 0x40000100 / BL: 0x40002900.

	u8  padding[12];
} tegra_bit_t210_t;

typedef struct _tegra_bit_t210b01_t
{
	u32 brom_version;
	u32 data_version;
	u32 rcm_version;

	u32 boot_type;

	u32 primary_dev_type;
	u32 secondary_dev_type;

	u32 authentication_scheme;
	u32 encryption_enabled;

	u32 brom_flow_tracker;

	u32 reserved;
	u32 boot_time_log_exit;
	u32 setup_tick_cnt;
	u32 bct_read_tick_cnt;
	u32 bl_read_tick_cnt;

	bit_flow_log_t boot_flow_log[40];

	u32 osc_frequency;
	u8  dev_initialized;
	u8  dram_initialized;

	u8  force_recovery_bit_cleared;
	u8  failback_bit_cleared;
	u8  failback_invoked;

	u8  irom_patch_status; // bit0-3: hamming status, bit7: patches exist.

	u8  bct_size_valid;
	u8  bct_size_status[9];
	u32 bct_size_last_journal_read;
	u32 bct_size_block;
	u32 bct_size_page;

	u8  bct_valid;
	u8  bct_status[9];
	u8  padding[2];
	u32 bct_last_journal_read;
	u32 bct_block;
	u32 bct_page;
	u32 bct_size;
	u32 bct_ptr;

	bit_bl_state_t bl_state[4];

	bit_boot_secondary_dev_status_t210b01_t secondary_dev_status;

	u32 usb_charging_status;

	u8  pmu_boot_sel_read_error;

	u32 safe_start_addr; // Init: 0x40000464 / UART: 0x40000464 / BL: 0x40002C64.
} tegra_bit_t210b01_t;

#endif
