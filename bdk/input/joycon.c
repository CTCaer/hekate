/*
 * Joy-Con UART driver for Nintendo Switch
 *
 * Copyright (c) 2019-2024 CTCaer
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

#include <string.h>

#include "joycon.h"
#include <gfx_utils.h>
#include <power/max17050.h>
#include <power/regulator_5v.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/gpio.h>
#include <soc/pinmux.h>
#include <soc/timer.h>
#include <soc/uart.h>
#include <soc/t210.h>

// For disabling driver when logging is enabled.
#include <libs/lv_conf.h>

#define JC_WIRED_CMD             0x91
#define JC_WIRED_HID             0x92
#define JC_WIRED_INIT_REPLY      0x94
#define JC_INIT_HANDSHAKE        0xA5

#define JC_HORI_INPUT_RPT_CMD    0x9A
#define JC_HORI_INPUT_RPT        0x00

#define JC_WIRED_CMD_GET_INFO    0x01
#define JC_WIRED_CMD_SET_CHARGER 0x02
#define JC_WIRED_CMD_GET_CHARGER 0x03
#define JC_WIRED_CMD_BATT_VOLT   0x06
#define JC_WIRED_CMD_WAKE_REASON 0x07
#define JC_WIRED_CMD_HID_CONN    0x10
#define JC_WIRED_CMD_HID_DISC    0x11
#define JC_WIRED_CMD_SET_HIDRATE 0x12 // Output report rate.
#define JC_WIRED_CMD_SET_BRATE   0x20

#define JC_HID_OUTPUT_RPT        0x01
#define JC_HID_RUMBLE_RPT        0x10

#define JC_HID_INPUT_RPT         0x30
#define JC_HID_SUBMCD_RPT        0x21

#define JC_HID_SUBCMD_HCI_STATE  0x06
#define  HCI_STATE_SLEEP         0x00
#define  HCI_STATE_RECONNECT     0x01
#define  HCI_STATE_PAIR          0x02
#define  HCI_STATE_HOME          0x04
#define JC_HID_SUBCMD_SPI_READ   0x10
#define  SPI_READ_OFFSET         0x20
#define JC_HID_SUBCMD_RUMBLE_CTL 0x48
#define JC_HID_SUBCMD_SND_RUMBLE 0xFF

#define JC_SIO_OUTPUT_RPT        0x91
#define JC_SIO_INPUT_RPT         0x92
#define  JC_SIO_CMD_ACK          0x80

#define JC_SIO_CMD_INIT          0x01
#define JC_SIO_CMD_UNK02         0x02
#define JC_SIO_CMD_VER_RPT       0x03
#define JC_SIO_CMD_UNK20         0x20 // JC_WIRED_CMD_SET_BRATE
#define JC_SIO_CMD_UNK21         0x21
#define JC_SIO_CMD_UNK22         0x22
#define JC_SIO_CMD_UNK40         0x40
#define JC_SIO_CMD_STATUS        0x41
#define JC_SIO_CMD_IAP_VER       0x42


#define JC_BTN_MASK_L 0xFF2900 // 0xFFE900: with charge status.
#define JC_BTN_MASK_R 0x0056FF

#define JC_ID_L     0x01 // Joycon (L). Mask for Hori (L).
#define JC_ID_R     0x02 // Joycon (R). Mask for Hori (R).
#define JC_ID_HORI  0x20 // Mask for Hori. Actual ids: 0x21, 0x22.

#define JC_CRC8_POLY 0x8D

enum
{
	JC_STATE_START         = 0,
	JC_STATE_HANDSHAKED    = 1,
	JC_STATE_BRATE_CHANGED = 2,
	JC_STATE_BRATE_OK      = 3,
	JC_STATE_INIT_DONE     = 4
};

enum
{
	JC_BATT_EMTPY = 0,
	JC_BATT_CRIT  = 1,
	JC_BATT_LOW   = 2,
	JC_BATT_MID   = 3,
	JC_BATT_FULL  = 4
};

static const u8 sio_init[] = {
	JC_SIO_OUTPUT_RPT, JC_SIO_CMD_INIT,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x95
};

static const u8 sio_set_rpt_version[] = {
	JC_SIO_OUTPUT_RPT, JC_SIO_CMD_VER_RPT,
	// old fw:   0x00, 0x0D (0.13). New 3.4.
	// force_update_en:      0x01
	0x00, 0x00, 0x03, 0x04, 0x00, 0xDA
};

// Every 8ms.
static const u8 sio_pad_status[] = {
	JC_SIO_OUTPUT_RPT, JC_SIO_CMD_STATUS,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xB0
};

static const u8 init_wake[] = {
	0xA1, 0xA2, 0xA3, 0xA4
};

static const u8 init_handshake[] = {
	0x19, 0x01, 0x03, 0x07, 0x00,      // Uart header.
	JC_INIT_HANDSHAKE, 0x02,           // Wired cmd and wired subcmd.
	0x01, 0x7E, 0x00, 0x00, 0x00       // Wired subcmd data and crc.
};

static const u8 init_get_info[]  = {
	0x19, 0x01, 0x03, 0x07, 0x00,        // Uart header.
	JC_WIRED_CMD, JC_WIRED_CMD_GET_INFO, // Wired cmd and subcmd.
	0x00, 0x00, 0x00, 0x00, 0x24         // Wired subcmd data and crc.
};

static const u8 init_switch_brate[]  = {
	0x19, 0x01, 0x03, 0x0F, 0x00,         // Uart header.
	JC_WIRED_CMD, JC_WIRED_CMD_SET_BRATE, // Wired cmd and subcmd.
	0x08, 0x00, 0x00, 0xBD, 0xB1,         // Wired subcmd data, data crc and crc.
	// Baudrate 3 megabaud.
	0xC0, 0xC6, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 init_hid_disconnect[]  = {
	0x19, 0x01, 0x03, 0x07, 0x00,        // Uart header.
	JC_WIRED_CMD, JC_WIRED_CMD_HID_DISC, // Wired cmd and subcmd.
	0x00, 0x00, 0x00, 0x00, 0x0E         // Wired subcmd data and crc.
};

static const u8 init_set_hid_rate[]  = {
	0x19, 0x01, 0x03, 0x0B, 0x00,           // Uart header.
	JC_WIRED_CMD, JC_WIRED_CMD_SET_HIDRATE, // Wired cmd and subcmd.
	0x04, 0x00, 0x00, 0x12, 0xA6,           // Wired subcmd data, data crc and crc.
	// Output report rate 15 ms.
	0x0F, 0x00, 0x00, 0x00

	// 5 ms.
	// 0x04, 0x00, 0x00, 0x0E, 0xD5,
	// 0x05, 0x00, 0x00, 0x00
};

static const u8 init_hid_connect[]  = {
	0x19, 0x01, 0x03, 0x07, 0x00,        // Uart header.
	JC_WIRED_CMD, JC_WIRED_CMD_HID_CONN, // Wired cmd and subcmd.
	0x00, 0x00, 0x00, 0x00, 0x3D         // Wired subcmd data and crc.
};

static const u8 nx_pad_status[]  = {
	0x19, 0x01, 0x03, 0x08, 0x00,      // Uart header.
	JC_WIRED_HID, 0x00,                // Wired cmd and hid cmd.
	0x01, 0x00, 0x00, 0x69, 0x2D, 0x1F // hid data, data crc and crc.
};

static const u8 hori_pad_status[] = {
	0x19, 0x01, 0x03, 0x07, 0x00,      // Uart header.
	JC_HORI_INPUT_RPT_CMD, 0x01,       // Hori cmd and hori subcmd.
	0x00, 0x00, 0x00, 0x00, 0x48       // Hori cmd data and crc.
};

typedef struct _jc_uart_hdr_t
{
	u8 magic[3];
	u8 total_size_lsb;
	u8 total_size_msb;
} jc_uart_hdr_t;

typedef struct _jc_wired_hdr_t
{
	jc_uart_hdr_t uart_hdr;
	u8 cmd;
	u8 data[5];
	u8 crc;
	u8 payload[];
} jc_wired_hdr_t;

typedef struct _jc_hid_out_rpt_t
{
	u8 cmd;
	u8 pkt_id;
	u8 rumble[8];
	u8 subcmd;
	u8 subcmd_data[];
} jc_hid_out_rpt_t;

typedef struct _jc_hid_out_spi_read_t
{
	u32 addr;
	u8  size;
} jc_hid_out_spi_read_t;

typedef struct _jc_hid_in_rpt_t
{
	u8 cmd;
	u8 pkt_id;
	u8 conn_info:4; // Connection detect.
	u8 batt_info:4; // Power info.
	u8 btn_right;
	u8 btn_shared;
	u8 btn_left;
	u8 stick_h_left;
	u8 stick_m_left;
	u8 stick_v_left;
	u8 stick_h_right;
	u8 stick_m_right;
	u8 stick_v_right;
	u8 vib_decider; // right:4, left:4 (bit3 en, bit2-0 buffer avail).
	u8 submcd_ack;
	u8 subcmd;
	u8 subcmd_data[];
} jc_hid_in_rpt_t;

typedef struct _jc_hid_in_spi_read_t
{
	u32 addr;
	u8  size;
	u8  data[];
} jc_hid_in_spi_read_t;

typedef struct _jc_hid_in_pair_data_t
{
	u8  magic;
	u8  size;
	u16 checksum;
	u8  mac[6];
	u8  ltk[16];
	u8  pad0[10];
	u8  bt_caps; // bit3: Secure conn supported host, bit5: Paired to TBFC supported host, bit6: iTBFC page supported
	u8  pad1;
} jc_hid_in_pair_data_t;

typedef struct _jc_sio_out_rpt_t
{
	u8  cmd;
	u8  subcmd;
	u16 payload_size;
	u8  data[2];
	u8  crc_payload;
	u8  crc_hdr;
	u8  payload[];
} jc_sio_out_rpt_t;

typedef struct _jc_sio_in_rpt_t
{
	u8  cmd;
	u8  ack;
	u16 payload_size;
	u8  status;
	u8  unk;
	u8  crc_payload;
	u8  crc_hdr;
	u8  payload[];
} jc_sio_in_rpt_t;

typedef struct _jc_hid_in_sixaxis_rpt_t
{
	s16 acc_x;
	s16 acc_y;
	s16 acc_z;
	s16 gyr_x;
	s16 gyr_y;
	s16 gyr_z;
} __attribute__((packed)) jc_hid_in_sixaxis_rpt_t;

typedef struct _jc_sio_hid_in_rpt_t
{
	u8 type;
	u8 pkt_id;
	u8 unk;
	u8 btn_right;
	u8 btn_shared;
	u8 btn_left;
	u8 stick_h_left;
	u8 stick_m_left;
	u8 stick_v_left;
	u8 stick_h_right;
	u8 stick_m_right;
	u8 stick_v_right;
	u8 siaxis_rpt; // bit0-3: report num. bit4-7: imu type.
	// Each report is 800 us?
	jc_hid_in_sixaxis_rpt_t sixaxis[15];
} jc_sio_hid_in_rpt_t;

typedef struct _joycon_ctxt_t
{
	u8  buf[0x100]; //FIXME: If heap is used, dumping breaks.
	u8  uart;
	u8  type;
	u8  state;
	u32 last_received_time;
	u32 last_status_req_time;
	u8  mac[6];
	u8  pkt_id;
	u8  rumble_sent;
	u8  connected;
	u8  detected;
	u8  sio_mode;
} joycon_ctxt_t;

static joycon_ctxt_t jc_l = {0};
static joycon_ctxt_t jc_r = {0};

static bool jc_init_done = false;

static jc_gamepad_rpt_t jc_gamepad;

static u8 _jc_crc(const u8 *data, u16 len, u8 init)
{
	u8 crc = init;
	for (u16 i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (u16 j = 0; j < 8; j++)
		{
			if ((crc & 0x80) != 0)
				crc = (u8)((crc << 1) ^ JC_CRC8_POLY);
			else
				crc <<= 1;
		}
	}
	return crc;
}

static void _jc_power_supply(u8 uart, bool enable)
{
	if (enable)
	{
		if (regulator_5v_get_dev_enabled(1 << uart))
			return;

		regulator_5v_enable(1 << uart);

		if (jc_gamepad.sio_mode)
			return;

		if (jc_init_done)
		{
			if (uart == UART_C)
				gpio_write(GPIO_PORT_CC, GPIO_PIN_3, GPIO_HIGH);
			else
				gpio_write(GPIO_PORT_K, GPIO_PIN_3, GPIO_HIGH);
			return;
		}

		if (uart == UART_C)
		{
			// Joy-Con(L) Charge Enable.
			PINMUX_AUX(PINMUX_AUX_SPDIF_IN) = PINMUX_PULL_DOWN | 1;
			gpio_direction_output(GPIO_PORT_CC, GPIO_PIN_3, GPIO_HIGH);
		}
		else
		{
			// Joy-Con(R) Charge Enable.
			PINMUX_AUX(PINMUX_AUX_GPIO_PK3) = PINMUX_DRIVE_4X | PINMUX_PULL_DOWN | 2;
			gpio_direction_output(GPIO_PORT_K, GPIO_PIN_3, GPIO_HIGH);
		}
	}
	else
	{
		if (!regulator_5v_get_dev_enabled(1 << uart))
			return;

		regulator_5v_disable(1 << uart);

		if (jc_gamepad.sio_mode)
			return;

		if (uart == UART_C)
			gpio_write(GPIO_PORT_CC, GPIO_PIN_3, GPIO_LOW);
		else
			gpio_write(GPIO_PORT_K, GPIO_PIN_3, GPIO_LOW);
	}
}

static void _jc_detect()
{
	if (!jc_gamepad.sio_mode)
	{
		// Turn on Joy-Con detect. (UARTB/C TX). UART CTS also if HW flow control and irq is enabled.
		PINMUX_AUX(PINMUX_AUX_UART2_TX) = PINMUX_INPUT_ENABLE;
		PINMUX_AUX(PINMUX_AUX_UART3_TX) = PINMUX_INPUT_ENABLE;
		gpio_direction_input(GPIO_PORT_G, GPIO_PIN_0);
		gpio_direction_input(GPIO_PORT_D, GPIO_PIN_1);
		usleep(20);

		//! HW BUG: Unlatch gpio buffer.
		(void)gpio_read(GPIO_PORT_H, GPIO_PIN_6);
		(void)gpio_read(GPIO_PORT_E, GPIO_PIN_6);

		// Read H6/E6 which are shared with UART TX pins.
		jc_r.detected = !gpio_read(GPIO_PORT_H, GPIO_PIN_6);
		jc_l.detected = !gpio_read(GPIO_PORT_E, GPIO_PIN_6);

		// Turn off Joy-Con detect. (UARTB/C TX).
		PINMUX_AUX(PINMUX_AUX_UART2_TX) = 0;
		PINMUX_AUX(PINMUX_AUX_UART3_TX) = 0;
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);
		usleep(20);
	}
	else
	{
		//! TODO: Is there a way to detect a broken Sio?
		jc_l.detected = true;
	}
}

static void _jc_conn_check()
{
	_jc_detect();

	if (jc_gamepad.sio_mode)
		return;

	// Check if a Joy-Con was disconnected.
	if (!jc_l.detected)
	{
		if (jc_l.connected)
			_jc_power_supply(UART_C, false);

		jc_l.pkt_id = 0;

		jc_l.connected   = false;
		jc_l.rumble_sent = false;

		jc_gamepad.conn_l = false;

		jc_gamepad.batt_info_l    = 0;
		jc_gamepad.bt_conn_l.type = 0;
		jc_gamepad.buttons &= ~JC_BTN_MASK_L;
	}

	if (!jc_r.detected)
	{
		if (jc_r.connected)
			_jc_power_supply(UART_B, false);

		jc_r.pkt_id = 0;

		jc_r.connected   = false;
		jc_r.rumble_sent = false;

		jc_gamepad.conn_r = false;

		jc_gamepad.batt_info_r    = 0;
		jc_gamepad.bt_conn_r.type = 0;
		jc_gamepad.buttons &= ~JC_BTN_MASK_R;
	}
}

static void _joycon_send_raw(u8 uart_port, const u8 *buf, u16 size)
{
	uart_send(uart_port, buf, size);
	uart_wait_xfer(uart_port, UART_TX_IDLE);
}

static u16 _jc_packet_add_uart_hdr(jc_wired_hdr_t *out, u8 wired_cmd, const u8 *data, u16 size, bool crc)
{
	out->uart_hdr.magic[0] = 0x19;
	out->uart_hdr.magic[1] = 0x01;
	out->uart_hdr.magic[2] = 0x3;

	out->uart_hdr.total_size_lsb = sizeof(jc_wired_hdr_t) - sizeof(jc_uart_hdr_t);
	out->uart_hdr.total_size_msb = 0;
	out->cmd = wired_cmd;

	if (data)
		memcpy(out->data, data, size);

	out->crc = crc ? _jc_crc(&out->uart_hdr.total_size_msb,
							 sizeof(out->uart_hdr.total_size_msb) +
							 sizeof(out->cmd) + sizeof(out->data), 0) : 0;

	return sizeof(jc_wired_hdr_t);
}

static u16 _jc_hid_output_rpt_craft(jc_wired_hdr_t *rpt, const u8 *payload, u16 size, bool crc)
{
	u16 pkt_size = _jc_packet_add_uart_hdr(rpt, JC_WIRED_HID, NULL, 0, crc);
	pkt_size += size;

	rpt->uart_hdr.total_size_lsb += size;
	rpt->data[0] = size >> 8;
	rpt->data[1] = size & 0xFF;

	if (payload)
		memcpy(rpt->payload, payload, size);

	return pkt_size;
}

static void _jc_send_hid_output_rpt(joycon_ctxt_t *jc, jc_hid_out_rpt_t *hid_pkt, u16 size, bool crc)
{
	u8 rpt[0x50];
	memset(rpt, 0, sizeof(rpt));

	hid_pkt->pkt_id = (jc->pkt_id++ & 0xF);
	u32 rpt_size = _jc_hid_output_rpt_craft((jc_wired_hdr_t *)rpt, (u8 *)hid_pkt, size, crc);

	_joycon_send_raw(jc->uart, rpt, rpt_size);
}

static void _jc_send_hid_cmd(joycon_ctxt_t *jc, u8 subcmd, const u8 *data, u16 size)
{
	static const u8 rumble_neutral[8] = { 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40 };
	static const u8 rumble_init[8]    = { 0xc2, 0xc8, 0x03, 0x72, 0xc2, 0xc8, 0x03, 0x72 };

	u8 temp[0x30] = {0};

	jc_hid_out_rpt_t *hid_pkt = (jc_hid_out_rpt_t *)temp;
	memcpy(hid_pkt->rumble, rumble_neutral, sizeof(rumble_neutral));

	if (subcmd == JC_HID_SUBCMD_SND_RUMBLE)
	{
		bool send_r_rumble = jc_r.connected && !jc_r.rumble_sent;
		bool send_l_rumble = jc_l.connected && !jc_l.rumble_sent;

		// Enable rumble.
		hid_pkt->cmd    = JC_HID_OUTPUT_RPT;
		hid_pkt->subcmd = JC_HID_SUBCMD_RUMBLE_CTL;
		hid_pkt->subcmd_data[0] = 1;
		if (send_r_rumble)
			_jc_send_hid_output_rpt(&jc_r, hid_pkt, 0x10, false);
		if (send_l_rumble)
			_jc_send_hid_output_rpt(&jc_l, hid_pkt, 0x10, false);

		// Send rumble.
		hid_pkt->cmd = JC_HID_RUMBLE_RPT;
		memcpy(hid_pkt->rumble, rumble_init, sizeof(rumble_init));
		if (send_r_rumble)
			_jc_send_hid_output_rpt(&jc_r, hid_pkt, 10, false);
		if (send_l_rumble)
			_jc_send_hid_output_rpt(&jc_l, hid_pkt, 10, false);

		msleep(15);

		// Disable rumble.
		hid_pkt->cmd    = JC_HID_OUTPUT_RPT;
		hid_pkt->subcmd = JC_HID_SUBCMD_RUMBLE_CTL;
		hid_pkt->subcmd_data[0] = 0;
		memcpy(hid_pkt->rumble, rumble_neutral, sizeof(rumble_neutral));
		if (send_r_rumble)
			_jc_send_hid_output_rpt(&jc_r, hid_pkt, 0x10, false);
		if (send_l_rumble)
			_jc_send_hid_output_rpt(&jc_l, hid_pkt, 0x10, false);
	}
	else
	{
		bool crc_needed = jc->type & JC_ID_HORI;

		hid_pkt->cmd    = JC_HID_OUTPUT_RPT;
		hid_pkt->subcmd = subcmd;
		if (data)
			memcpy(hid_pkt->subcmd_data, data, size);

		_jc_send_hid_output_rpt(jc, hid_pkt, sizeof(jc_hid_out_rpt_t) + size, crc_needed);
	}
}

static void _jc_charging_decider(u8 batt, u8 uart)
{
	u32 system_batt_enough = max17050_get_cached_batt_volt() > 4000;

	// Power supply control based on battery levels and charging.
	if ((batt >> 1) < JC_BATT_LOW) // Level without checking charging.
		_jc_power_supply(uart, true);
	else if (batt > (system_batt_enough ? JC_BATT_FULL : JC_BATT_MID) << 1) // Addresses the charging bit.
		_jc_power_supply(uart, false);
}

static void _jc_parse_wired_hid(joycon_ctxt_t *jc, const u8 *packet, int size)
{
	u32 btn_tmp;
	jc_hid_in_rpt_t *hid_pkt = (jc_hid_in_rpt_t *)packet;

	switch (hid_pkt->cmd)
	{
	case JC_HORI_INPUT_RPT:
		if (!(jc->type & JC_ID_HORI))
			return;

	case JC_HID_INPUT_RPT:
		// Discard incomplete hid packets.
		if (size < 12)
			break;

		btn_tmp = hid_pkt->btn_right | hid_pkt->btn_shared << 8 | hid_pkt->btn_left << 16;

		if (jc->type & JC_ID_L)
		{
			jc_gamepad.buttons &= ~JC_BTN_MASK_L;
			jc_gamepad.buttons |= (btn_tmp & JC_BTN_MASK_L);

			jc_gamepad.lstick_x = hid_pkt->stick_h_left | ((hid_pkt->stick_m_left & 0xF) << 8);
			jc_gamepad.lstick_y = (hid_pkt->stick_m_left >> 4) | (hid_pkt->stick_v_left << 4);

			jc_gamepad.batt_info_l = hid_pkt->batt_info;
		}
		else if (jc->type & JC_ID_R)
		{
			jc_gamepad.buttons &= ~JC_BTN_MASK_R;
			jc_gamepad.buttons |= (btn_tmp & JC_BTN_MASK_R);

			jc_gamepad.rstick_x = hid_pkt->stick_h_right | ((hid_pkt->stick_m_right & 0xF) << 8);
			jc_gamepad.rstick_y = (hid_pkt->stick_m_right >> 4) | (hid_pkt->stick_v_right << 4);

			jc_gamepad.batt_info_r = hid_pkt->batt_info;
		}

		jc_gamepad.conn_l = jc_l.connected;
		jc_gamepad.conn_r = jc_r.connected;

		if (hid_pkt->cmd == JC_HID_INPUT_RPT)
			_jc_charging_decider(hid_pkt->batt_info, jc->uart);
		break;
	case JC_HID_SUBMCD_RPT:
		if (hid_pkt->subcmd == JC_HID_SUBCMD_SPI_READ)
		{
			jc_bt_conn_t *bt_conn;

			if (jc->type & JC_ID_L)
				bt_conn = &jc_gamepad.bt_conn_l;
			else
				bt_conn = &jc_gamepad.bt_conn_r;

			jc_hid_in_spi_read_t  *spi_info  = (jc_hid_in_spi_read_t *)hid_pkt->subcmd_data;
			jc_hid_in_pair_data_t *pair_data = (jc_hid_in_pair_data_t *)spi_info->data;

			// Check if the reply is pairing info.
			if (spi_info->size == 0x1A && pair_data->magic == 0x95 && pair_data->size == 0x22)
			{
				bt_conn->type = jc->type;

				memcpy(bt_conn->mac, jc->mac, 6);
				memcpy(bt_conn->host_mac, pair_data->mac, 6);
				for (u32 i = 16; i > 0; i--)
					bt_conn->ltk[16 - i] = pair_data->ltk[i - 1];
			}
		}
		break;
	default:
		break;
	}
}

static void _jc_parse_wired_init(joycon_ctxt_t *jc, const u8 *data, int size)
{
	// Discard empty packets.
	if (size <= 0)
		return;

	switch (data[0])
	{
	case JC_WIRED_CMD_GET_INFO:
		for (int i = 12; i > 6; i--)
			jc->mac[12 - i] = data[i];
		jc->type = data[6];
		jc->connected = true;
		break;
	case JC_WIRED_CMD_SET_BRATE:
		jc->state = JC_STATE_BRATE_CHANGED;
		break;
	case JC_WIRED_CMD_HID_DISC:
		jc->state = JC_STATE_BRATE_OK;
		break;
	case JC_WIRED_CMD_HID_CONN:
	case JC_WIRED_CMD_SET_HIDRATE:
		// done.
	default:
		break;
	}
}

static void _jc_uart_pkt_parse(joycon_ctxt_t *jc, const jc_wired_hdr_t *pkt, int size)
{
	switch (pkt->cmd)
	{
	case JC_HORI_INPUT_RPT_CMD:
	case JC_WIRED_HID:
		_jc_parse_wired_hid(jc, pkt->payload, size - sizeof(jc_wired_hdr_t));
		break;
	case JC_WIRED_INIT_REPLY:
		_jc_parse_wired_init(jc, pkt->data, size - sizeof(jc_uart_hdr_t) - 1);
		break;
	case JC_INIT_HANDSHAKE:
		jc->state = JC_STATE_HANDSHAKED;
		break;
	default:
		break;
	}

	jc->last_received_time = get_tmr_ms();
}

static void _jc_sio_parse_payload(joycon_ctxt_t *jc, u8 cmd, const u8 *payload, int size)
{
	switch (cmd)
	{
	case JC_SIO_CMD_STATUS:
		// Discard incomplete packets.
		if (size < 12)
			break;

		jc_sio_hid_in_rpt_t *hid_pkt = (jc_sio_hid_in_rpt_t *)payload;
		jc_gamepad.buttons = hid_pkt->btn_right | hid_pkt->btn_shared << 8 | hid_pkt->btn_left << 16;
		jc_gamepad.home    = !gpio_read(GPIO_PORT_V, GPIO_PIN_3);

		jc_gamepad.lstick_x = hid_pkt->stick_h_left | ((hid_pkt->stick_m_left & 0xF) << 8);
		jc_gamepad.lstick_y = (hid_pkt->stick_m_left >> 4) | (hid_pkt->stick_v_left << 4);
		jc_gamepad.rstick_x = hid_pkt->stick_h_right | ((hid_pkt->stick_m_right & 0xF) << 8);
		jc_gamepad.rstick_y = (hid_pkt->stick_m_right >> 4) | (hid_pkt->stick_v_right << 4);

		jc_gamepad.batt_info_l = jc_l.connected;
		jc_gamepad.batt_info_r = gpio_read(GPIO_PORT_E, GPIO_PIN_7); // Set IRQ status.

		jc_gamepad.conn_l = jc_l.connected;
		jc_gamepad.conn_r = jc_l.connected;
		break;
	default:
		break;
	}
}

static void _jc_sio_uart_pkt_parse(joycon_ctxt_t *jc, const jc_sio_in_rpt_t *pkt, int size)
{
	if (pkt->crc_hdr != _jc_crc((u8 *)pkt, sizeof(jc_sio_in_rpt_t) - 1, 0))
		return;

	u8 cmd = pkt->ack & (~JC_SIO_CMD_ACK);
	switch (cmd)
	{
	case JC_SIO_CMD_INIT:
		jc->connected = pkt->status == 0;
		break;
	case JC_SIO_CMD_VER_RPT:
		if (jc->connected)
			jc->connected = pkt->status == 0;
		break;
	case JC_SIO_CMD_IAP_VER:
	case JC_SIO_CMD_STATUS:
		_jc_sio_parse_payload(jc, cmd, pkt->payload, size - sizeof(jc_sio_in_rpt_t));
		break;
	case JC_SIO_CMD_UNK02:
	case JC_SIO_CMD_UNK20:
	case JC_SIO_CMD_UNK21:
	case JC_SIO_CMD_UNK22:
	case JC_SIO_CMD_UNK40:
	default:
		break;
	}

	jc->last_received_time = get_tmr_ms();
}

static void _jc_rcv_pkt(joycon_ctxt_t *jc)
{
	if (!jc->detected)
		return;

	u32 len = uart_recv(jc->uart, (u8 *)jc->buf, 0x100);
	if (len < 8)
		return;

	// For Joycon, check uart reply magic.
	jc_wired_hdr_t *jc_pkt = (jc_wired_hdr_t *)jc->buf;
	if (!jc->sio_mode && !memcmp(jc_pkt->uart_hdr.magic, "\x19\x81\x03", 3))
	{
		_jc_uart_pkt_parse(jc, jc_pkt, len);

		return;
	}

	// For Sio, check uart output report and command ack.
	jc_sio_in_rpt_t *sio_pkt = (jc_sio_in_rpt_t *)(jc->buf);
	if (jc->sio_mode && sio_pkt->cmd == JC_SIO_INPUT_RPT && (sio_pkt->ack & JC_SIO_CMD_ACK) == JC_SIO_CMD_ACK)
	{
		_jc_sio_uart_pkt_parse(jc, sio_pkt, len);

		return;
	}
}

static bool _jc_send_init_rumble(joycon_ctxt_t *jc)
{
	// Send init rumble or request nx pad status report.
	if ((jc_r.connected && !jc_r.rumble_sent) || (jc_l.connected && !jc_l.rumble_sent))
	{
		_jc_send_hid_cmd(jc, JC_HID_SUBCMD_SND_RUMBLE, NULL, 0);

		if (jc_l.connected)
			jc_l.rumble_sent = true;
		if (jc_r.connected)
			jc_r.rumble_sent = true;

		return 1;
	}

	return 0;
}

static void _jc_req_nx_pad_status(joycon_ctxt_t *jc)
{
	if (!jc->detected)
		return;

	bool is_nxpad = !(jc->type & JC_ID_HORI) && !jc->sio_mode;

	if (jc->last_status_req_time > get_tmr_ms() || !jc->connected)
		return;

	if (is_nxpad)
	{
		bool sent_rumble = _jc_send_init_rumble(jc);

		if (sent_rumble)
			return;
	}

	if (is_nxpad)
		_joycon_send_raw(jc->uart, nx_pad_status,   sizeof(nx_pad_status));
	else if (jc->sio_mode)
		_joycon_send_raw(jc->uart, sio_pad_status,  sizeof(sio_pad_status));
	else
		_joycon_send_raw(jc->uart, hori_pad_status, sizeof(hori_pad_status));

	jc->last_status_req_time = get_tmr_ms() + (!jc->sio_mode ? 15 : 7);
}

static bool _jc_validate_pairing_info(const u8 *buf, bool *is_hos)
{
	u8 crc = 0;
	for (u32 i = 0; i < 0x22; i++)
		crc += buf[4 + i];

	crc += 0x68; // Host is Switch.

	if ((crc ^ 0x55) == buf[2])
		*is_hos = true;

	crc -= 0x68;
	crc += 0x08; // Host is PC.

	if (*is_hos || (crc ^ 0x55) == buf[2])
		return true;

	return false;
}

jc_gamepad_rpt_t *jc_get_bt_pairing_info(bool *is_l_hos, bool *is_r_hos)
{
	u8 retries;
	jc_bt_conn_t *bt_conn;

	if (!jc_init_done || jc_gamepad.sio_mode)
		return NULL;

	bt_conn = &jc_gamepad.bt_conn_l;
	memset(bt_conn->host_mac, 0, 6);
	memset(bt_conn->ltk,      0, 16);

	bt_conn = &jc_gamepad.bt_conn_r;
	memset(bt_conn->host_mac, 0, 6);
	memset(bt_conn->ltk,      0, 16);

	_jc_conn_check();

	while (jc_l.last_status_req_time > get_tmr_ms())
	{
		_jc_rcv_pkt(&jc_r);
		_jc_rcv_pkt(&jc_l);
	}

	jc_hid_in_spi_read_t subcmd_data_l;
	subcmd_data_l.addr = 0x2000;
	subcmd_data_l.size = 0x1A;

	jc_hid_in_spi_read_t subcmd_data_r;
	subcmd_data_r.addr = 0x2000;
	subcmd_data_r.size = 0x1A;

	bool jc_r_found = jc_r.connected ? false : true;
	bool jc_l_found = jc_l.connected ? false : true;

	// Set mode to HW controlled RTS.
	uart_set_mode(jc_l.uart, UART_AO_TX_HW_RX);
	uart_set_mode(jc_r.uart, UART_AO_TX_HW_RX);

	u32 total_retries = 10;
retry:
	retries = 10;
	while (retries)
	{
		u32 time_now = get_tmr_ms();
		if ((!jc_l_found && jc_l.last_status_req_time < time_now) || (!jc_r_found && jc_r.last_status_req_time < time_now))
		{
			if (!jc_l_found)
			{
				_jc_send_hid_cmd(&jc_l, JC_HID_SUBCMD_SPI_READ, (u8 *)&subcmd_data_l, 5);
				jc_l.last_status_req_time = get_tmr_ms() + 15;
			}

			if (!jc_r_found)
			{
				_jc_send_hid_cmd(&jc_r, JC_HID_SUBCMD_SPI_READ, (u8 *)&subcmd_data_r, 5);
				jc_r.last_status_req_time = get_tmr_ms() + 15;
			}

			retries--;
		}

		// Wait for the first 36 bytes to arrive.
		msleep(5);

		if (!jc_l_found)
		{
			memset(jc_l.buf, 0, 0x100);
			_jc_rcv_pkt(&jc_l);

			bool is_hos = false;
			if (_jc_validate_pairing_info(&jc_l.buf[SPI_READ_OFFSET], &is_hos))
			{
				bool is_active = jc_l.buf[SPI_READ_OFFSET] == 0x95;

				if (!is_active)
					subcmd_data_l.addr += 0x26; // Get next slot.
				else
					jc_l_found = true; // Entry is active.

				if (jc_l_found && is_hos)
					*is_l_hos = true;
			}
		}

		if (!jc_r_found)
		{
			memset(jc_r.buf, 0, 0x100);
			_jc_rcv_pkt(&jc_r);

			bool is_hos = false;
			if (_jc_validate_pairing_info(&jc_r.buf[SPI_READ_OFFSET], &is_hos))
			{
				bool is_active = jc_r.buf[SPI_READ_OFFSET] == 0x95;

				if (!is_active)
					subcmd_data_r.addr += 0x26; // Get next slot.
				else
					jc_r_found = true; // Entry is active.

				if (jc_r_found && is_hos)
					*is_r_hos = true;
			}
		}

		if (jc_l_found && jc_r_found)
			break;
	}

	if (!jc_l_found || !jc_r_found)
	{
		if (total_retries)
		{
			total_retries--;
			goto retry;
		}

		if (!jc_l_found)
		{
			bt_conn = &jc_gamepad.bt_conn_l;
			memset(bt_conn->host_mac, 0, 6);
			memset(bt_conn->ltk,      0, 16);
		}

		if (!jc_r_found)
		{
			bt_conn = &jc_gamepad.bt_conn_r;
			memset(bt_conn->host_mac, 0, 6);
			memset(bt_conn->ltk,      0, 16);
		}
	}

	// Restore mode to manual RTS.
	uart_set_mode(jc_l.uart, UART_AO_TX_MN_RX);
	uart_set_mode(jc_r.uart, UART_AO_TX_MN_RX);

	return &jc_gamepad;
}

static void _jc_init_conn(joycon_ctxt_t *jc)
{
	if (!jc->detected)
		return;

	if (((u32)get_tmr_ms() - jc->last_received_time) > 1000)
	{
		_jc_power_supply(jc->uart, true);

		// Mask out buttons and set connected to false.
		if (jc->uart == UART_B)
		{
			jc_gamepad.buttons &= ~JC_BTN_MASK_R;
			jc_gamepad.conn_r = false;
		}
		else
		{
			jc_gamepad.buttons &= ~JC_BTN_MASK_L;
			jc_gamepad.conn_l = false;
		}

		// Initialize uart to 1 megabaud and manual RTS.
		uart_init(jc->uart, 1000000, UART_AO_TX_MN_RX);

		if (!jc->sio_mode)
		{
			jc->state = JC_STATE_START;

			// Set TX and RTS inversion for Joycon.
			uart_invert(jc->uart, true, UART_INVERT_TXD | UART_INVERT_RTS);

			// Wake up the controller.
			_joycon_send_raw(jc->uart, init_wake, sizeof(init_wake));
			_jc_rcv_pkt(jc); // Clear RX FIFO.

			// Do a handshake.
			u32 retries = 10;
			while (retries && jc->state != JC_STATE_HANDSHAKED)
			{
				_joycon_send_raw(jc->uart, init_handshake, sizeof(init_handshake));
				msleep(5);
				_jc_rcv_pkt(jc);
				retries--;
			}

			if (jc->state != JC_STATE_HANDSHAKED)
				goto out;

			// Get info about the controller.
			_joycon_send_raw(jc->uart, init_get_info, sizeof(init_get_info));
			msleep(2);
			_jc_rcv_pkt(jc);

			if (!(jc->type & JC_ID_HORI))
			{
				// Request 3 megabaud change.
				_joycon_send_raw(jc->uart, init_switch_brate, sizeof(init_switch_brate));
				msleep(2);
				_jc_rcv_pkt(jc);

				if (jc->state == JC_STATE_BRATE_CHANGED)
				{
					// Reinitialize uart to 3 megabaud and manual RTS.
					uart_init(jc->uart, 3000000, UART_AO_TX_MN_RX);
					uart_invert(jc->uart, true, UART_INVERT_TXD | UART_INVERT_RTS);

					// Disconnect HID.
					retries = 10;
					while (retries && jc->state != JC_STATE_BRATE_OK)
					{
						_joycon_send_raw(jc->uart, init_hid_disconnect, sizeof(init_hid_disconnect));
						msleep(5);
						_jc_rcv_pkt(jc);
						retries--;
					}

					if (jc->state != JC_STATE_BRATE_OK)
						goto out;
				}

				// Create HID connection with the new rate.
				_joycon_send_raw(jc->uart, init_hid_connect, sizeof(init_hid_connect));
				msleep(2);
				_jc_rcv_pkt(jc);

				// Set hid packet rate.
				_joycon_send_raw(jc->uart, init_set_hid_rate, sizeof(init_set_hid_rate));
				msleep(2);
				_jc_rcv_pkt(jc);
			}
			else // Hori. Unset RTS inversion.
				uart_invert(jc->uart, false, UART_INVERT_RTS);
		}
		else
		{
			// Set Sio NPOR low to configure BOOT0 mode.
			gpio_write(GPIO_PORT_CC, GPIO_PIN_5, GPIO_LOW);
			usleep(300);
			gpio_write(GPIO_PORT_T, GPIO_PIN_0, GPIO_LOW);
			gpio_write(GPIO_PORT_CC, GPIO_PIN_5, GPIO_HIGH);
			msleep(100);

			// Clear RX FIFO.
			_jc_rcv_pkt(jc);

			// Initialize the controller.
			u32 retries = 10;
			while (!jc->connected && retries)
			{
				_joycon_send_raw(jc->uart, sio_init, sizeof(sio_init));
				msleep(5);
				_jc_rcv_pkt(jc);
				retries--;
			}

			if (!jc->connected)
				goto out;

			// Set output report version.
			_joycon_send_raw(jc->uart, sio_set_rpt_version, sizeof(sio_set_rpt_version));
			msleep(5);
			_jc_rcv_pkt(jc);
		}

		// Initialization done.
		jc->state = JC_STATE_INIT_DONE;

out:
		jc->last_received_time = get_tmr_ms();

		if (!jc->sio_mode && jc->connected && !(jc->type & JC_ID_HORI))
			_jc_power_supply(jc->uart, false);
	}
}

void jc_init_hw()
{
	jc_l.uart = UART_C;
	jc_r.uart = UART_B;

	jc_l.sio_mode = fuse_read_hw_type() == FUSE_NX_HW_TYPE_HOAG;
	jc_gamepad.sio_mode = jc_l.sio_mode;

#if !defined(DEBUG_UART_PORT) || !(DEBUG_UART_PORT)
	_jc_power_supply(UART_C, true);
	_jc_power_supply(UART_B, true);

	// Sio Initialization.
	if (jc_gamepad.sio_mode)
	{
		// Enable 4 MHz clock to Sio.
		clock_enable_extperiph2();
		PINMUX_AUX(PINMUX_AUX_TOUCH_CLK) = PINMUX_PULL_DOWN;

		// Configure Sio HOME BUTTON.
		PINMUX_AUX(PINMUX_AUX_LCD_GPIO1) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE | PINMUX_PULL_UP | 1;
		gpio_direction_input(GPIO_PORT_V, GPIO_PIN_3);

		// Configure Sio IRQ
		PINMUX_AUX(PINMUX_AUX_GPIO_PE7)  = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE | PINMUX_PULL_UP;
		gpio_direction_input(GPIO_PORT_E, GPIO_PIN_7);

		// Configure Sio NRST and BOOT0.
		PINMUX_AUX(PINMUX_AUX_CAM1_STROBE) = PINMUX_PULL_DOWN | 1;
		PINMUX_AUX(PINMUX_AUX_CAM2_PWDN)   = PINMUX_PULL_DOWN | 1;

		// Set BOOT0 to flash mode. (output high is sram mode).
		gpio_direction_output(GPIO_PORT_T, GPIO_PIN_0, GPIO_LOW);

		// NRST to pull down.
		gpio_direction_input(GPIO_PORT_T, GPIO_PIN_1);

		// Configure Sio NPOR.
		PINMUX_AUX(PINMUX_AUX_USB_VBUS_EN1) = PINMUX_IO_HV | PINMUX_LPDR | 1;
		gpio_direction_output(GPIO_PORT_CC, GPIO_PIN_5, GPIO_LOW);
	}

#if 0 // Already set by hw init.
	// Set Joy-Con IsAttached pinmux. Shared with UARTB/UARTC TX.
	PINMUX_AUX(PINMUX_AUX_GPIO_PE6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_GPIO_PH6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;

	// Set Joy-Con IsAttached mode. Shared with UARTB/UARTC TX.
	gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_GPIO);
#endif

	// Configure pinmuxing for UART B and C.
	if (!jc_gamepad.sio_mode)
		pinmux_config_uart(UART_B);
	pinmux_config_uart(UART_C);

	// Enable UART B and C clocks.
	if (!jc_gamepad.sio_mode)
		clock_enable_uart(UART_B);
	clock_enable_uart(UART_C);

	jc_init_done = true;
#endif
}

void jc_deinit()
{
	if (!jc_init_done)
		return;

	// Disable power.
	_jc_power_supply(UART_B, false);
	_jc_power_supply(UART_C, false);

	if (!jc_gamepad.sio_mode)
	{
		// Send sleep command.
		u8 data = HCI_STATE_SLEEP;
		if (jc_r.connected && !(jc_r.type & JC_ID_HORI))
		{
			_jc_send_hid_cmd(&jc_r, JC_HID_SUBCMD_HCI_STATE, &data, 1);
			_jc_rcv_pkt(&jc_r);
		}
		if (jc_l.connected && !(jc_l.type & JC_ID_HORI))
		{
			_jc_send_hid_cmd(&jc_l, JC_HID_SUBCMD_HCI_STATE, &data, 1);
			_jc_rcv_pkt(&jc_l);
		}
	}
	else
	{
		// Disable Sio NPOR.
		gpio_write(GPIO_PORT_CC, GPIO_PIN_5, GPIO_LOW);

		// Disable 4 MHz clock to Sio.
		clock_disable_extperiph2();
	}

	// Disable UART B and C clocks.
	if (!jc_gamepad.sio_mode)
		clock_disable_uart(UART_B);
	clock_disable_uart(UART_C);
}

jc_gamepad_rpt_t *joycon_poll()
{
	if (!jc_init_done)
		return NULL;

	_jc_conn_check();

	_jc_init_conn(&jc_r);
	_jc_init_conn(&jc_l);

	_jc_req_nx_pad_status(&jc_r);
	_jc_req_nx_pad_status(&jc_l);

	_jc_rcv_pkt(&jc_r);
	_jc_rcv_pkt(&jc_l);

	return &jc_gamepad;
}
