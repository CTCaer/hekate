/*
 * Copyright (c) 2018 naehrwert
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

#ifndef _SDMMC_DRIVER_H_
#define _SDMMC_DRIVER_H_

#include <utils/types.h>
#include <storage/sdmmc_t210.h>

/*! SDMMC controller IDs. */
#define SDMMC_1 0 // Version 4.00.
#define SDMMC_2 1 // Version 5.1.
#define SDMMC_3 2 // Version 4.00.
#define SDMMC_4 3 // Version 5.1.

/*! SDMMC power types. */
#define SDMMC_POWER_OFF 0
#define SDMMC_POWER_1_8 1
#define SDMMC_POWER_3_3 2

/*! SDMMC bus widths. */
#define SDMMC_BUS_WIDTH_1 0
#define SDMMC_BUS_WIDTH_4 1
#define SDMMC_BUS_WIDTH_8 2

/*! SDMMC response types. */
#define SDMMC_RSP_TYPE_0 0
#define SDMMC_RSP_TYPE_1 1
#define SDMMC_RSP_TYPE_2 2
#define SDMMC_RSP_TYPE_3 3
#define SDMMC_RSP_TYPE_4 4
#define SDMMC_RSP_TYPE_5 5

/*! SDMMC mask interrupt status. */
#define SDMMC_MASKINT_MASKED   0
#define SDMMC_MASKINT_NOERROR  1
#define SDMMC_MASKINT_ERROR    2

/*! SDMMC present state. */
#define SDHCI_CMD_INHIBIT      BIT(0)
#define SDHCI_DATA_INHIBIT     BIT(1)
#define SDHCI_DAT_LINE_ACTIVE  BIT(2)
#define SDHCI_RETUNING_REQUEST BIT(3)
#define SDHCI_EMMC_LINE_LVL_MASK 0xF0
#define  SDHCI_DATA_4_LVL      BIT(4) // eMMC only.
#define  SDHCI_DATA_5_LVL      BIT(5) // eMMC only.
#define  SDHCI_DATA_6_LVL      BIT(6) // eMMC only.
#define  SDHCI_DATA_7_LVL      BIT(7) // eMMC only.
#define SDHCI_DOING_WRITE      BIT(8)
#define SDHCI_DOING_READ       BIT(9) // SD only.
#define SDHCI_SPACE_AVAILABLE  BIT(10)
#define SDHCI_DATA_AVAILABLE   BIT(11)
#define SDHCI_CARD_PRESENT     BIT(16)
#define SDHCI_CD_STABLE        BIT(17)
#define SDHCI_CD_LVL           BIT(18)
#define SDHCI_WRITE_PROTECT    BIT(19)
#define SDHCI_DATA_LVL_MASK    0xF00000
#define  SDHCI_DATA_0_LVL      BIT(20)
#define  SDHCI_DATA_1_LVL      BIT(21)
#define  SDHCI_DATA_2_LVL      BIT(22)
#define  SDHCI_DATA_3_LVL      BIT(23)
#define SDHCI_CMD_LVL          BIT(24)
#define SDHCI_CMD_NOT_ISSUED   BIT(27)

/*! SDMMC transfer mode. */
#define SDHCI_TRNS_DMA        BIT(0)
#define SDHCI_TRNS_BLK_CNT_EN BIT(1)
#define SDHCI_TRNS_AUTO_CMD12 BIT(2)
#define SDHCI_TRNS_AUTO_CMD23 BIT(3)
#define SDHCI_TRNS_WRITE      0x00 // Bit4.
#define SDHCI_TRNS_READ       BIT(4)
#define SDHCI_TRNS_MULTI      BIT(5)

/*! SDMMC command. */
#define SDHCI_CMD_RESP_MASK       0x3
#define SDHCI_CMD_RESP_NO_RESP    0x0
#define SDHCI_CMD_RESP_LEN136     0x1
#define SDHCI_CMD_RESP_LEN48      0x2
#define SDHCI_CMD_RESP_LEN48_BUSY 0x3
#define SDHCI_CMD_CRC             BIT(3)
#define SDHCI_CMD_INDEX           BIT(4)
#define SDHCI_CMD_DATA            BIT(5)
#define SDHCI_CMD_ABORTCMD        0xC0

/*! SDMMC host control. */
#define SDHCI_CTRL_LED        BIT(0)
#define SDHCI_CTRL_4BITBUS    BIT(1) // SD only.
#define SDHCI_CTRL_HISPD      BIT(2) // SD only.
#define SDHCI_CTRL_DMA_MASK   0x18
#define  SDHCI_CTRL_SDMA      0x00
#define  SDHCI_CTRL_ADMA1     0x08
#define  SDHCI_CTRL_ADMA32    0x10
#define  SDHCI_CTRL_ADMA64    0x18
#define SDHCI_CTRL_8BITBUS    BIT(5) // eMMC only (or UHS-II).
#define SDHCI_CTRL_CDTEST_INS BIT(6)
#define SDHCI_CTRL_CDTEST_EN  BIT(7)

/*! SDMMC host control 2. */
#define SDHCI_CTRL_UHS_MASK       0x7
#define SDHCI_CTRL_VDD_180        BIT(3)
#define SDHCI_CTRL_DRV_TYPE_B     (0U << 4)
#define SDHCI_CTRL_DRV_TYPE_A     (1U << 4)
#define SDHCI_CTRL_DRV_TYPE_C     (2U << 4)
#define SDHCI_CTRL_DRV_TYPE_D     (3U << 4)
#define SDHCI_CTRL_EXEC_TUNING    BIT(6)
#define SDHCI_CTRL_TUNED_CLK      BIT(7)
#define SDHCI_HOST_VERSION_4_EN   BIT(12)
#define SDHCI_ADDRESSING_64BIT_EN BIT(13)
#define SDHCI_CTRL_PRESET_VAL_EN  BIT(15)

/*! SDMMC power control. */
#define SDHCI_POWER_ON   BIT(0)
#define SDHCI_POWER_180  0x0A
#define SDHCI_POWER_300  0x0C
#define SDHCI_POWER_330  0x0E
#define SDHCI_POWER_MASK 0xF1 // UHS-II only.

// /*! SDMMC max current. */
// #define SDHCI_MAX_CURRENT_330_MASK   0xFF
// #define SDHCI_MAX_CURRENT_180_MASK   0xFF0000
// #define SDHCI_MAX_CURRENT_MULTIPLIER 4

/*! SDMMC clock control. */
#define SDHCI_CLOCK_INT_EN     BIT(0)
#define SDHCI_CLOCK_INT_STABLE BIT(1)
#define SDHCI_CLOCK_CARD_EN    BIT(2)
#define SDHCI_PROG_CLOCK_MODE  BIT(5)
#define SDHCI_DIVIDER_HI_SHIFT 6
#define SDHCI_DIV_HI_MASK      (3U << SDHCI_DIVIDER_HI_SHIFT)
#define SDHCI_DIVIDER_SHIFT    8
#define SDHCI_DIV_MASK         (0xFFU << SDHCI_DIVIDER_SHIFT)


/*! SDMMC software reset. */
#define SDHCI_RESET_ALL  BIT(0)
#define SDHCI_RESET_CMD  BIT(1)
#define SDHCI_RESET_DATA BIT(2)

/*! SDMMC interrupt status and control. */
#define SDHCI_INT_RESPONSE     BIT(0)
#define SDHCI_INT_DATA_END     BIT(1)
#define SDHCI_INT_BLK_GAP      BIT(2)
#define SDHCI_INT_DMA_END      BIT(3)
#define SDHCI_INT_SPACE_AVAIL  BIT(4)
#define SDHCI_INT_DATA_AVAIL   BIT(5)
#define SDHCI_INT_CARD_INSERT  BIT(6)
#define SDHCI_INT_CARD_REMOVE  BIT(7)
#define SDHCI_INT_CARD_INT     BIT(8)
#define SDHCI_INT_RETUNE       BIT(12)
#define SDHCI_INT_CQE          BIT(14)
#define SDHCI_INT_ERROR        BIT(15)

/*! SDMMC error interrupt status and control. */
#define SDHCI_ERR_INT_TIMEOUT      BIT(0)
#define SDHCI_ERR_INT_CRC          BIT(1)
#define SDHCI_ERR_INT_END_BIT      BIT(2)
#define SDHCI_ERR_INT_INDEX        BIT(3)
#define SDHCI_ERR_INT_DATA_TIMEOUT BIT(4)
#define SDHCI_ERR_INT_DATA_CRC     BIT(5)
#define SDHCI_ERR_INT_DATA_END_BIT BIT(6)
#define SDHCI_ERR_INT_BUS_POWER    BIT(7)
#define SDHCI_ERR_INT_AUTO_CMD_ERR BIT(8)
#define SDHCI_ERR_INT_ADMA_ERROR   BIT(9)
#define SDHCI_ERR_INT_TUNE_ERROR   BIT(10)
#define SDHCI_ERR_INT_RSP_ERROR    BIT(11)

#define SDHCI_ERR_INT_ALL_EXCEPT_ADMA_BUSPWR \
	(SDHCI_ERR_INT_AUTO_CMD_ERR | SDHCI_ERR_INT_DATA_END_BIT | \
	 SDHCI_ERR_INT_DATA_CRC | SDHCI_ERR_INT_DATA_TIMEOUT | \
	 SDHCI_ERR_INT_INDEX | SDHCI_ERR_INT_END_BIT | \
	 SDHCI_ERR_INT_CRC | SDHCI_ERR_INT_TIMEOUT)

/*! SD bus speeds. */
#define UHS_SDR12_BUS_SPEED  0
#define HIGH_SPEED_BUS_SPEED 1
#define UHS_SDR25_BUS_SPEED  1
#define UHS_SDR50_BUS_SPEED  2
#define UHS_SDR104_BUS_SPEED 3
#define UHS_DDR50_BUS_SPEED  4
#define HS400_BUS_SPEED      5

/*! SDMMC timmings. */
#define SDHCI_TIMING_MMC_ID     0
#define SDHCI_TIMING_MMC_LS26   1
#define SDHCI_TIMING_MMC_HS52   2
#define SDHCI_TIMING_MMC_HS200  3
#define SDHCI_TIMING_MMC_HS400  4
#define SDHCI_TIMING_SD_ID      5
#define SDHCI_TIMING_SD_DS12    6
#define SDHCI_TIMING_SD_HS25    7
#define SDHCI_TIMING_UHS_SDR12  8
#define SDHCI_TIMING_UHS_SDR25  9
#define SDHCI_TIMING_UHS_SDR50  10
#define SDHCI_TIMING_UHS_SDR104 11
#define SDHCI_TIMING_UHS_SDR82  12 // SDR104 with a 163.2MHz -> 81.6MHz clock.
#define SDHCI_TIMING_UHS_DDR50  13
#define SDHCI_TIMING_MMC_HS102  14

#define SDHCI_CAN_64BIT BIT(28)

/*! SDMMC Low power features. */
#define SDMMC_POWER_SAVE_DISABLE 0
#define SDMMC_POWER_SAVE_ENABLE  1

/*! Helper for SWITCH command argument. */
#define SDMMC_SWITCH(mode, index, value) (((mode) << 24) | ((index) << 16) | ((value) << 8))

/*! SDMMC controller context. */
typedef struct _sdmmc_t
{
	t210_sdmmc_t *regs;
	u32 id;
	u32 divisor;
	u32 clock_stopped;
	int powersave_enabled;
	int manual_cal;
	int card_clock_enabled;
	int venclkctl_set;
	u32 venclkctl_tap;
	u32 expected_rsp_type;
	u32 dma_addr_next;
	u32 rsp[4];
	u32 rsp3;
	int t210b01;
} sdmmc_t;

/*! SDMMC command. */
typedef struct _sdmmc_cmd_t
{
	u16 cmd;
	u32 arg;
	u32 rsp_type;
	u32 check_busy;
} sdmmc_cmd_t;

/*! SDMMC request. */
typedef struct _sdmmc_req_t
{
	void *buf;
	u32 blksize;
	u32 num_sectors;
	int is_write;
	int is_multi_block;
	int is_auto_stop_trn;
} sdmmc_req_t;

int  sdmmc_get_io_power(sdmmc_t *sdmmc);
u32  sdmmc_get_bus_width(sdmmc_t *sdmmc);
void sdmmc_set_bus_width(sdmmc_t *sdmmc, u32 bus_width);
void sdmmc_save_tap_value(sdmmc_t *sdmmc);
int  sdmmc_setup_clock(sdmmc_t *sdmmc, u32 type);
void sdmmc_card_clock_powersave(sdmmc_t *sdmmc, int powersave_enable);
int  sdmmc_get_rsp(sdmmc_t *sdmmc, u32 *rsp, u32 size, u32 type);
int  sdmmc_tuning_execute(sdmmc_t *sdmmc, u32 type, u32 cmd);
int  sdmmc_stop_transmission(sdmmc_t *sdmmc, u32 *rsp);
bool sdmmc_get_sd_inserted();
int  sdmmc_init(sdmmc_t *sdmmc, u32 id, u32 power, u32 bus_width, u32 type, int powersave_enable);
void sdmmc_end(sdmmc_t *sdmmc);
void sdmmc_init_cmd(sdmmc_cmd_t *cmdbuf, u16 cmd, u32 arg, u32 rsp_type, u32 check_busy);
int  sdmmc_execute_cmd(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, sdmmc_req_t *req, u32 *blkcnt_out);
int  sdmmc_enable_low_voltage(sdmmc_t *sdmmc);

#endif
