/*
 * Touch driver for Nintendo Switch's STM FingerTip S (4CD60D) touch controller
 *
 * Copyright (c) 2018 langerhans
 * Copyright (c) 2018-2026 CTCaer
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

#ifndef __TOUCH_H_
#define __TOUCH_H_

#include <utils/types.h>

#define FTS4_I2C_ADDR 0x49

#define FTS4_I2C_CHIP_ID 0x3670

/* I2C commands. */
#define FTS4_CMD_READ_INFO           0x80
#define FTS4_CMD_READ_STATUS         0x84
#define FTS4_CMD_READ_ONE_EVENT      0x85
#define FTS4_CMD_READ_ALL_EVENT      0x86
#define FTS4_CMD_LATEST_EVENT        0x87
#define FTS4_CMD_SLEEP_IN            0x90
#define FTS4_CMD_SLEEP_OUT           0x91
#define FTS4_CMD_MS_MT_SENSE_OFF     0x92
#define FTS4_CMD_MS_MT_SENSE_ON      0x93
#define FTS4_CMD_SS_HOVER_SENSE_OFF  0x94
#define FTS4_CMD_SS_HOVER_SENSE_ON   0x95
#define FTS4_CMD_LP_TIMER_CALIB      0x97
#define FTS4_CMD_MS_KEY_SENSE_OFF    0x9A
#define FTS4_CMD_MS_KEY_SENSE_ON     0x9B
#define FTS4_CMD_SYSTEM_RESET        0xA0
#define FTS4_CMD_CLEAR_EVENT_STACK   0xA1
#define FTS4_CMD_FULL_FORCE_CALIB    0xA2
#define FTS4_CMD_MS_CX_TUNING        0xA3
#define FTS4_CMD_SS_CX_TUNING        0xA4
#define FTS4_CMD_ITO_CHECK           0xA7
#define FTS4_CMD_RELEASEINFO         0xAA
#define FTS4_CMD_HW_REG_READ         0xB6                 // u16be address offset. Any size read.
#define FTS4_CMD_HW_REG_WRITE        FTS4_CMD_HW_REG_READ // u16be address offset, bytes to write.
#define FTS4_CMD_SWITCH_SENSE_MODE   0xC3
#define FTS4_CMD_NOISE_WRITE         0xC7
#define FTS4_CMD_NOISE_READ          0xC8
#define FTS4_CMD_FB_REG_READ         0xD0
#define FTS4_CMD_FB_REG_WRITE        FTS4_CMD_FB_REG_READ
#define FTS4_CMD_SAVE_CX_TUNING      0xFC

#define FTS4_CMD_DETECTION_CONFIG    0xB0
#define FTS4_CMD_REQ_CX_DATA         0xB8
#define FTS4_CMD_VENDOR              0xCF
#define FTS4_CMD_FLASH_UNLOCK        0xF7
#define FTS4_CMD_FLASH_WRITE_64K     0xF8
#define FTS4_CMD_FLASH_STATUS        0xF9
#define FTS4_CMD_FLASH_OP            0xFA
#define FTS4_CMD_UNK_62 0x62

/* Command parameters. */
#define FTS4_VENDOR_GPIO_STATE       0x01
#define FTS4_VENDOR_SENSE_MODE       0x02
#define FTS4_STYLUS_MODE             0x00
#define FTS4_FINGER_MODE             0x01
#define FTS4_HOVER_MODE              0x02

/* HW Registers */
#define FTS4_HW_REG_CHIP_ID_INFO     0x0004
#define FTS4_HW_REG_SYS_RESET        0x0028

/* FB Addresses */
#define FTS4_FB_REG_FW_INFO_ADDRESS  0x0060

/* Events. */
#define FTS4_EV_NO_EVENT             0x00
#define FTS4_EV_MULTI_TOUCH_DETECTED 0x02
#define FTS4_EV_MULTI_TOUCH_ENTER    0x03
#define FTS4_EV_MULTI_TOUCH_LEAVE    0x04
#define FTS4_EV_MULTI_TOUCH_MOTION   0x05
#define FTS4_EV_HOVER_ENTER          0x07
#define FTS4_EV_HOVER_LEAVE          0x08
#define FTS4_EV_HOVER_MOTION         0x09
#define FTS4_EV_KEY_STATUS           0x0E
#define FTS4_EV_ERROR                0x0F
#define FTS4_EV_NOISE_READ           0x17
#define FTS4_EV_NOISE_WRITE          0x18
#define FTS4_EV_VENDOR               0x20

#define FTS4_EV_CONTROLLER_READY     0x10
#define FTS4_EV_STATUS               0x16
#define FTS4_EV_DEBUG                0xDB

#define FTS4_EV_STATUS_MS_CX_TUNING_DONE  1
#define FTS4_EV_STATUS_SS_CX_TUNING_DONE  2
#define FTS4_EV_STATUS_WRITE_CX_TUNE_DONE 4

/* Multi touch related event masks. */
#define FTS4_MASK_EVENT_ID   0x0F
#define FTS4_MASK_TOUCH_ID   0xF0
#define FTS4_MASK_LEFT_EVENT 0x0F
#define FTS4_MASK_X_MSB      0x0F
#define FTS4_MASK_Y_LSB      0xF0

/* Key related event masks. */
#define FTS4_MASK_KEY_NO_TOUCH 0x00
#define FTS4_MASK_KEY_MENU     0x01
#define FTS4_MASK_KEY_BACK     0x02

#define FTS4_EVENT_SIZE     8
#define FTS4_STACK_DEPTH   32
#define FTS4_DATA_MAX_SIZE (FTS4_EVENT_SIZE * FTS4_STACK_DEPTH)
#define FTS4_MAX_FINGERS   10

typedef enum _touch_ito_error {
	ITO_NO_ERROR = 0,
	ITO_FORCE_OPEN,
	ITO_SENSE_OPEN,
	ITO_FORCE_SHRT_GND,
	ITO_SENSE_SHRT_GND,
	ITO_FORCE_SHRT_VCM,
	ITO_SENSE_SHRT_VCM,
	ITO_FORCE_SHRT_FORCE,
	ITO_SENSE_SHRT_SENSE,
	ITO_F2E_SENSE,
	ITO_FPC_FORCE_OPEN,
	ITO_FPC_SENSE_OPEN,
	ITO_KEY_FORCE_OPEN,
	ITO_KEY_SENSE_OPEN,
	ITO_RESERVED0,
	ITO_RESERVED1,
	ITO_RESERVED2,
	ITO_MAX_ERR_REACHED = 0xFF
} touch_ito_error;

typedef struct _touch_event_t {
	u8   raw[FTS4_EVENT_SIZE];
	u8   fingers;
	u8  type; // Event type.
	u16  x, y; // Coordinates.
	u32  z;    // Orientation.
	bool touch;
} touch_event_t;

typedef struct _touch_panel_info_t
{
	u8 idx;
	u8 gpio0;
	u8 gpio1;
	u8 gpio2;
	char *vendor;
} touch_panel_info_t;

typedef struct _touch_info_t {
	u16 chip_id;
	u16 fw_ver;
	u16 config_id;
	u16 config_ver;
} touch_info_t;

typedef struct _touch_fw_info_t {
	u32 fw_id;
	u16 ftb_ver;
	u16 fw_rev;
} touch_fw_info_t;

void touch_poll(touch_event_t *event);
touch_info_t *touch_get_chip_info();
touch_panel_info_t *touch_get_panel_vendor();
int touch_get_fw_info(touch_fw_info_t *fw);
int touch_panel_ito_test(u8 *err);
int touch_execute_autotune();
int touch_switch_sense_mode(u8 mode, bool gis_6_2);
int touch_sense_enable();
int touch_power_on();
void touch_power_off();

#endif /* __TOUCH_H_ */
