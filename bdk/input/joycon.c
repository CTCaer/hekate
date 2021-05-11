/*
 * Joy-Con UART driver for Nintendo Switch
 *
 * Copyright (c) 2019-2021 CTCaer
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
#include <soc/bpmp.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/gpio.h>
#include <soc/pinmux.h>
#include <soc/uart.h>
#include <soc/t210.h>
#include <utils/util.h>

// For disabling driver when logging is enabled.
#include <libs/lv_conf.h>

#define JC_WIRED_CMD             0x91
#define JC_WIRED_HID             0x92
#define JC_WIRED_INIT_REPLY      0x94
#define JC_INIT_HANDSHAKE        0xA5

#define JC_HORI_INPUT_RPT_CMD    0x9A
#define JC_HORI_INPUT_RPT        0x00

#define JC_WIRED_CMD_MAC         0x01
#define JC_WIRED_CMD_10          0x10

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

#define JC_BTN_MASK_L 0xFF2900 // 0xFFE900: with charge status.
#define JC_BTN_MASK_R 0x0056FF

#define JC_ID_L     0x01
#define JC_ID_R     0x02
#define JC_ID_HORI  0x20

#define JC_CRC8_INIT    0x00
#define JC_CRC8_POLY    0x8D

enum
{
	JC_BATT_EMTPY = 0,
	JC_BATT_CRIT  = 2,
	JC_BATT_LOW   = 4,
	JC_BATT_MID   = 6,
	JC_BATT_FULL  = 8
};

static const u8 init_jc[] = {
	0xA1, 0xA2, 0xA3, 0xA4
};

static const u8 init_handshake[] = {
	0x19, 0x01, 0x03, 0x07, 0x00,      // Uart header.
	JC_INIT_HANDSHAKE, 0x02,           // Wired cmd and wired subcmd.
	0x01, 0x7E, 0x00, 0x00, 0x00       // Wired subcmd data and crc.
};

static const u8 init_get_info[]  = {
	0x19, 0x01, 0x03, 0x07, 0x00,      // Uart header.
	JC_WIRED_CMD, JC_WIRED_CMD_MAC,    // Wired cmd and subcmd.
	0x00, 0x00, 0x00, 0x00, 0x24       // Wired subcmd data and crc.
};

static const u8 init_finalize[]  = {
	0x19, 0x01, 0x03, 0x07, 0x00,      // Uart header.
	JC_WIRED_CMD, JC_WIRED_CMD_10,     // Wired cmd and subcmd.
	0x00, 0x00, 0x00, 0x00, 0x3D       // Wired subcmd data and crc.
};

static const u8 nx_pad_status[]  = {
	0x19, 0x01, 0x03, 0x08, 0x00,      // Uart header.
	JC_WIRED_HID, 0x00,                // Wired cmd and hid cmd.
	0x01, 0x00, 0x00, 0x69, 0x2D, 0x1F // hid data and crc.
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
	u8 conn_info:4;
	u8 batt_info:4;
	u8 btn_right;
	u8 btn_shared;
	u8 btn_left;
	u8 stick_h_left;
	u8 stick_m_left;
	u8 stick_v_left;
	u8 stick_h_right;
	u8 stick_m_right;
	u8 stick_v_right;
	u8 vib_decider;
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

typedef struct _joycon_ctxt_t
{
	u8 buf[0x100]; //FIXME: If heap is used, dumping breaks.
	u8 uart;
	u8 type;
	u8 mac[6];
	u32 hw_init_done;
	u32 last_received_time;
	u32 last_status_req_time;
	u8 rumble_sent;
	u8 connected;
} joycon_ctxt_t;

static joycon_ctxt_t jc_l = {0};
static joycon_ctxt_t jc_r = {0};

static bool jc_init_done = false;
static u32 hid_pkt_inc = 0;

static jc_gamepad_rpt_t jc_gamepad;

void jc_power_supply(u8 uart, bool enable);

static u8 jc_crc(u8 *data, u16 len)
{
	u8 crc = JC_CRC8_INIT;
	u16 i, j;
	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (j = 0; j < 8; j++) {
			if ((crc & 0x80) != 0)
				crc = (u8)((crc << 1) ^ JC_CRC8_POLY);
			else
				crc <<= 1;
		}
	}
	return crc;
}

void joycon_send_raw(u8 uart_port, const u8 *buf, u16 size)
{
	uart_send(uart_port, buf, size);
	uart_wait_idle(uart_port, UART_TX_IDLE);
}

static u16 jc_packet_add_uart_hdr(jc_wired_hdr_t *out, u8 wired_cmd, u8 *data, u16 size, bool crc)
{
	out->uart_hdr.magic[0] = 0x19;
	out->uart_hdr.magic[1] = 0x01;
	out->uart_hdr.magic[2] = 0x3;

	out->uart_hdr.total_size_lsb = sizeof(jc_wired_hdr_t) - sizeof(jc_uart_hdr_t);
	out->uart_hdr.total_size_msb = 0;
	out->cmd = wired_cmd;

	if (data)
		memcpy(out->data, data, size);

	out->crc = crc ? jc_crc(&out->uart_hdr.total_size_msb, sizeof(out->uart_hdr.total_size_msb) + sizeof(out->cmd) + sizeof(out->data)) : 0;

	return sizeof(jc_wired_hdr_t);
}

static u16 jc_hid_output_rpt_craft(jc_wired_hdr_t *rpt, u8 *payload, u16 size, bool crc)
{
	u16 pkt_size = jc_packet_add_uart_hdr(rpt, JC_WIRED_HID, NULL, 0, crc);
	pkt_size += size;

	rpt->uart_hdr.total_size_lsb += size;
	rpt->data[0] = size >> 8;
	rpt->data[1] = size & 0xFF;

	if (payload)
		memcpy(rpt->payload, payload, size);

	return pkt_size;
}

void jc_send_hid_output_rpt(u8 uart, u8 *payload, u16 size, bool crc)
{
	u8 rpt[0x50];
	memset(rpt, 0, sizeof(rpt));

	u32 rpt_size = jc_hid_output_rpt_craft((jc_wired_hdr_t *)rpt, payload, size, crc);

	joycon_send_raw(uart, rpt, rpt_size);
}

static u8 jc_hid_pkt_id_incr()
{
	u8 curr_id = hid_pkt_inc;
	hid_pkt_inc++;

	return (curr_id & 0xF);
}

void jc_send_hid_cmd(u8 uart, u8 subcmd, u8 *data, u16 size)
{
	u8 temp[0x30];
	u8 rumble_neutral[8] = {0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40};
	u8 rumble_init[8]    = {0xc2, 0xc8, 0x03, 0x72, 0xc2, 0xc8, 0x03, 0x72};

	memset(temp, 0, sizeof(temp));

	jc_hid_out_rpt_t *hid_pkt = (jc_hid_out_rpt_t *)temp;

	memcpy(hid_pkt->rumble, rumble_neutral, sizeof(rumble_neutral));

	if (subcmd == JC_HID_SUBCMD_SND_RUMBLE)
	{
		bool send_r_rumble = jc_r.connected && !jc_r.rumble_sent;
		bool send_l_rumble = jc_l.connected && !jc_l.rumble_sent;

		// Enable rumble.
		hid_pkt->cmd = JC_HID_OUTPUT_RPT;
		hid_pkt->pkt_id = jc_hid_pkt_id_incr();
		hid_pkt->subcmd = JC_HID_SUBCMD_RUMBLE_CTL;
		hid_pkt->subcmd_data[0] = 1;
		if (send_r_rumble)
			jc_send_hid_output_rpt(UART_B, (u8 *)hid_pkt, 0x10, false);
		if (send_l_rumble)
			jc_send_hid_output_rpt(UART_C, (u8 *)hid_pkt, 0x10, false);

		// Send rumble.
		hid_pkt->cmd = JC_HID_RUMBLE_RPT;
		hid_pkt->pkt_id = jc_hid_pkt_id_incr();
		memcpy(hid_pkt->rumble, rumble_init, sizeof(rumble_init));
		if (send_r_rumble)
			jc_send_hid_output_rpt(UART_B, (u8 *)hid_pkt, 10, false);
		if (send_l_rumble)
			jc_send_hid_output_rpt(UART_C, (u8 *)hid_pkt, 10, false);

		msleep(15);

		// Disable rumble.
		hid_pkt->cmd = JC_HID_OUTPUT_RPT;
		hid_pkt->pkt_id = jc_hid_pkt_id_incr();
		hid_pkt->subcmd = JC_HID_SUBCMD_RUMBLE_CTL;
		hid_pkt->subcmd_data[0] = 0;
		memcpy(hid_pkt->rumble, rumble_neutral, sizeof(rumble_neutral));
		if (send_r_rumble)
			jc_send_hid_output_rpt(UART_B, (u8 *)hid_pkt, 0x10, false);
		if (send_l_rumble)
			jc_send_hid_output_rpt(UART_C, (u8 *)hid_pkt, 0x10, false);
	}
	else
	{
		bool crc_needed = (jc_l.uart == uart) ? (jc_l.type & JC_ID_HORI) : (jc_r.type & JC_ID_HORI);

		hid_pkt->cmd = JC_HID_OUTPUT_RPT;
		hid_pkt->pkt_id = jc_hid_pkt_id_incr();
		hid_pkt->subcmd = subcmd;
		if (data)
			memcpy(hid_pkt->subcmd_data, data, size);

		jc_send_hid_output_rpt(uart, (u8 *)hid_pkt, sizeof(jc_hid_out_rpt_t) + size, crc_needed);
	}
}

static void jc_charging_decider(u8 batt, u8 uart)
{
	u32 system_batt_enough = max17050_get_cached_batt_volt() > 4000;

	// Power supply control based on battery levels and charging.
	if ((batt >> 1 << 1) < JC_BATT_LOW) // Level without checking charging.
		jc_power_supply(uart, true);
	else if (batt > (system_batt_enough ? JC_BATT_FULL : JC_BATT_MID)) // Addresses the charging bit.
		jc_power_supply(uart, false);
}

static void jc_parse_wired_hid(joycon_ctxt_t *jc, const u8* packet, u32 size)
{
	u32 btn_tmp;
	jc_hid_in_rpt_t *hid_pkt = (jc_hid_in_rpt_t *)packet;

	switch (hid_pkt->cmd)
	{
	case JC_HORI_INPUT_RPT:
	case JC_HID_INPUT_RPT:
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

		jc_charging_decider(hid_pkt->batt_info, jc->uart);
		break;
	case JC_HID_SUBMCD_RPT:
		if (hid_pkt->subcmd == JC_HID_SUBCMD_SPI_READ)
		{
			jc_bt_conn_t *bt_conn;

			if (jc->type & JC_ID_L)
				bt_conn = &jc_gamepad.bt_conn_l;
			else
				bt_conn = &jc_gamepad.bt_conn_r;

			jc_hid_in_spi_read_t  *spi_info = (jc_hid_in_spi_read_t *)hid_pkt->subcmd_data;
			jc_hid_in_pair_data_t *pair_data = (jc_hid_in_pair_data_t *)spi_info->data;

			// Check if we reply is pairing info.
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
	jc->last_received_time = get_tmr_ms();
}

static void jc_parse_wired_init(joycon_ctxt_t *jc, const u8* data, u32 size)
{
	switch (data[0])
	{
	case JC_WIRED_CMD_MAC:
		for (int i = 12; i > 6; i--)
			jc->mac[12 - i] = data[i];
		jc->type = data[6];
		jc->connected = true;
	default:
		break;
	}
}

static void jc_uart_pkt_parse(joycon_ctxt_t *jc, const u8* packet, size_t size)
{
	jc_wired_hdr_t *pkt = (jc_wired_hdr_t *)packet;
	switch (pkt->cmd)
	{
	case JC_HORI_INPUT_RPT_CMD:
	case JC_WIRED_HID:
		jc_parse_wired_hid(jc, pkt->payload, (pkt->data[0] << 8) | pkt->data[1]);
		break;
	case JC_WIRED_INIT_REPLY:
		jc_parse_wired_init(jc, pkt->data, size - sizeof(jc_uart_hdr_t) - 1);
		break;
	default:
		break;
	}
}

static void jc_rcv_pkt(joycon_ctxt_t *jc)
{
	if (gpio_read(GPIO_PORT_E, GPIO_PIN_6) && jc->uart == UART_C)
		return;
	else if (gpio_read(GPIO_PORT_H, GPIO_PIN_6) && jc->uart == UART_B)
		return;

	// Check if device stopped sending data.
	u32 uart_irq = uart_get_IIR(jc->uart);
	if (uart_irq != UART_IIR_REDI)
		return;

	u32 len = uart_recv(jc->uart, (u8 *)jc->buf, 0x100);

	// Check valid size and uart reply magic.
	if (len > 7 && !memcmp(jc->buf, "\x19\x81\x03", 3))
	{
		jc_wired_hdr_t *pkt = (jc_wired_hdr_t *)(jc->buf);

		jc_uart_pkt_parse(jc, jc->buf, pkt->uart_hdr.total_size_lsb + sizeof(jc_uart_hdr_t));
	}
}

static bool jc_send_init_rumble(joycon_ctxt_t *jc)
{
	// Send init rumble or request nx pad status report.
	if ((jc_r.connected && !jc_r.rumble_sent) || (jc_l.connected && !jc_l.rumble_sent))
	{
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);

		jc_send_hid_cmd(jc->uart, JC_HID_SUBCMD_SND_RUMBLE, NULL, 0);

		if (jc_l.connected)
			jc_l.rumble_sent = true;
		if (jc_r.connected)
			jc_r.rumble_sent = true;

		if (jc->uart != UART_B)
			gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_GPIO);
		else
			gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);

		return 1;
	}

	return 0;
}

static void jc_req_nx_pad_status(joycon_ctxt_t *jc)
{
	bool is_nxpad = !(jc->type & JC_ID_HORI);

	if (is_nxpad)
	{
		bool sent_rumble = jc_send_init_rumble(jc);

		if (sent_rumble)
			return;
	}

	if (jc->last_status_req_time > get_tmr_ms() || !jc->connected)
		return;

	// Turn off Joy-Con detect.
	if (jc->uart == UART_B)
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
	else
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);

	if (is_nxpad)
		joycon_send_raw(jc->uart, nx_pad_status, sizeof(nx_pad_status));
	else
		joycon_send_raw(jc->uart, hori_pad_status, sizeof(hori_pad_status));

	// Turn Joy-Con detect on.
	if (jc->uart == UART_B)
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_GPIO);
	else
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);

	jc->last_status_req_time = get_tmr_ms() + 15;
}

static bool _jc_validate_pairing_info(u8 *buf, bool *is_hos)
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

	if (!jc_init_done)
		return NULL;

	bt_conn = &jc_gamepad.bt_conn_l;
	memset(bt_conn->host_mac, 0, 6);
	memset(bt_conn->ltk, 0, 16);

	bt_conn = &jc_gamepad.bt_conn_r;
	memset(bt_conn->host_mac, 0, 6);
	memset(bt_conn->ltk, 0, 16);

	while (jc_l.last_status_req_time > get_tmr_ms())
	{
		jc_rcv_pkt(&jc_r);
		jc_rcv_pkt(&jc_l);
	}

	jc_hid_in_spi_read_t subcmd_data_l;
	subcmd_data_l.addr = 0x2000;
	subcmd_data_l.size = 0x1A;

	jc_hid_in_spi_read_t subcmd_data_r;
	subcmd_data_r.addr = 0x2000;
	subcmd_data_r.size = 0x1A;

	// Turn off Joy-Con detect.
	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);

	bool jc_r_found = jc_r.connected ? false : true;
	bool jc_l_found = jc_l.connected ? false : true;

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
				jc_send_hid_cmd(jc_l.uart, JC_HID_SUBCMD_SPI_READ, (u8 *)&subcmd_data_l, 5);
				jc_l.last_status_req_time = get_tmr_ms() + 15;
			}

			if (!jc_r_found)
			{
				jc_send_hid_cmd(jc_r.uart, JC_HID_SUBCMD_SPI_READ, (u8 *)&subcmd_data_r, 5);
				jc_r.last_status_req_time = get_tmr_ms() + 15;
			}

			retries--;
		}

		if (!jc_l_found)
		{
			memset(jc_l.buf, 0, 0x100);
			jc_rcv_pkt(&jc_l);

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
			jc_rcv_pkt(&jc_r);

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
			memset(bt_conn->ltk, 0, 16);
		}

		if (!jc_r_found)
		{
			bt_conn = &jc_gamepad.bt_conn_r;
			memset(bt_conn->host_mac, 0, 6);
			memset(bt_conn->ltk, 0, 16);
		}
	}

	// Turn Joy-Con detect on.
	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);

	return &jc_gamepad;
}

void jc_deinit()
{
	// Disable power.
	jc_power_supply(UART_B, false);
	jc_power_supply(UART_C, false);

	// Turn off Joy-Con detect.
	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);

	// Send sleep command.
	u8 data = HCI_STATE_SLEEP;

	if (jc_r.connected && !(jc_r.type & JC_ID_HORI))
	{
		jc_send_hid_cmd(UART_B, JC_HID_SUBCMD_HCI_STATE, &data, 1);
		jc_rcv_pkt(&jc_r);
	}
	if (jc_l.connected && !(jc_l.type & JC_ID_HORI))
	{
		jc_send_hid_cmd(UART_C, JC_HID_SUBCMD_HCI_STATE, &data, 1);
		jc_rcv_pkt(&jc_l);
	}

	// Disable UART B and C clocks.
	clock_disable_uart(UART_B);
	clock_disable_uart(UART_C);
}

static void jc_init_conn(joycon_ctxt_t *jc)
{
	if (((u32)get_tmr_ms() - jc->last_received_time) > 1000)
	{
		jc_power_supply(jc->uart, true);

		// Turn off Joy-Con detect.
		if (jc->uart == UART_B)
		{
			jc_gamepad.buttons &= ~JC_BTN_MASK_R;
			jc_gamepad.conn_r = false;
			gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
		}
		else
		{
			jc_gamepad.buttons &= ~JC_BTN_MASK_L;
			jc_gamepad.conn_l = false;
			gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);
		}

		uart_init(jc->uart, 1000000);
		uart_invert(jc->uart, true, UART_INVERT_TXD);
		uart_set_IIR(jc->uart);

		joycon_send_raw(jc->uart, init_jc, 4);
		joycon_send_raw(jc->uart, init_handshake, sizeof(init_handshake));

		msleep(5);
		jc_rcv_pkt(jc);

		joycon_send_raw(jc->uart, init_get_info, sizeof(init_get_info));
		msleep(5);
		jc_rcv_pkt(jc);

		if (!(jc->type & JC_ID_HORI))
		{
			joycon_send_raw(jc->uart, init_finalize, sizeof(init_finalize));
			msleep(5);
			jc_rcv_pkt(jc);
		}

		// Turn Joy-Con detect on.
		if (jc->uart == UART_B)
			gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_GPIO);
		else
			gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);

		jc->last_received_time = get_tmr_ms();

		if (jc->connected)
			jc_power_supply(jc->uart, false);
	}
}

static void jc_conn_check()
{
	// Check if a Joy-Con was disconnected.
	if (gpio_read(GPIO_PORT_E, GPIO_PIN_6))
	{
		jc_power_supply(UART_C, false);

		hid_pkt_inc = 0;

		jc_l.connected = false;
		jc_l.rumble_sent = false;

		jc_gamepad.buttons &= ~JC_BTN_MASK_L;
		jc_gamepad.conn_l = false;

		jc_gamepad.batt_info_l = 0;
		jc_gamepad.bt_conn_l.type = 0;
	}

	if (gpio_read(GPIO_PORT_H, GPIO_PIN_6))
	{
		jc_power_supply(UART_B, false);

		hid_pkt_inc = 0;

		jc_r.connected = false;
		jc_r.rumble_sent = false;

		jc_gamepad.buttons &= ~JC_BTN_MASK_R;
		jc_gamepad.conn_r = false;

		jc_gamepad.batt_info_r = 0;
		jc_gamepad.bt_conn_r.type = 0;
	}
}

void jc_power_supply(u8 uart, bool enable)
{
	if (enable)
	{
		if (regulator_5v_get_dev_enabled(1 << uart))
			return;

		regulator_5v_enable(1 << uart);

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
			// Joy-Con(L) Charge Detect.
			PINMUX_AUX(PINMUX_AUX_SPDIF_IN) = PINMUX_PULL_DOWN | 1;
			gpio_config(GPIO_PORT_CC, GPIO_PIN_3, GPIO_MODE_GPIO);
			gpio_output_enable(GPIO_PORT_CC, GPIO_PIN_3, GPIO_OUTPUT_ENABLE);
			gpio_write(GPIO_PORT_CC, GPIO_PIN_3, GPIO_HIGH);
		}
		else
		{
			// Joy-Con(R) Charge Detect.
			PINMUX_AUX(PINMUX_AUX_GPIO_PK3) = PINMUX_DRIVE_4X | PINMUX_PULL_DOWN | 2;
			gpio_config(GPIO_PORT_K, GPIO_PIN_3, GPIO_MODE_GPIO);
			gpio_output_enable(GPIO_PORT_K, GPIO_PIN_3, GPIO_OUTPUT_ENABLE);
			gpio_write(GPIO_PORT_K, GPIO_PIN_3, GPIO_HIGH);
		}
	}
	else
	{
		if (!regulator_5v_get_dev_enabled(1 << uart))
			return;

		regulator_5v_disable(1 << uart);

		if (uart == UART_C)
			gpio_write(GPIO_PORT_CC, GPIO_PIN_3, GPIO_LOW);
		else
			gpio_write(GPIO_PORT_K, GPIO_PIN_3, GPIO_LOW);
	}
}

void jc_init_hw()
{
	jc_l.uart = UART_C;
	jc_r.uart = UART_B;

#if !defined(DEBUG_UART_PORT) || !(DEBUG_UART_PORT)
	if (fuse_read_hw_type() == FUSE_NX_HW_TYPE_HOAG)
		return;

	jc_power_supply(UART_C, true);
	jc_power_supply(UART_B, true);

	// Joy-Con (R) IsAttached.
	PINMUX_AUX(PINMUX_AUX_GPIO_PH6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;
	gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_GPIO);

	// Joy-Con (L) IsAttached.
	PINMUX_AUX(PINMUX_AUX_GPIO_PE6) = PINMUX_INPUT_ENABLE | PINMUX_TRISTATE;
	gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_GPIO);

	// Configure pinmuxing for UART B and C.
	pinmux_config_uart(UART_B);
	pinmux_config_uart(UART_C);

	// Ease the stress to APB.
	bpmp_freq_t prev_fid = bpmp_clk_rate_set(BPMP_CLK_NORMAL);

	// Enable UART B and C clocks.
	clock_enable_uart(UART_B);
	clock_enable_uart(UART_C);

	// Restore OC.
	bpmp_clk_rate_set(prev_fid);

	// Turn Joy-Con detect on.
	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_GPIO);
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_GPIO);

	jc_init_done = true;
#endif
}

jc_gamepad_rpt_t *joycon_poll()
{
	if (!jc_init_done)
		return NULL;

	if (!gpio_read(GPIO_PORT_H, GPIO_PIN_6))
		jc_init_conn(&jc_r);
	if (!gpio_read(GPIO_PORT_E, GPIO_PIN_6))
		jc_init_conn(&jc_l);

	if (!gpio_read(GPIO_PORT_H, GPIO_PIN_6))
		jc_req_nx_pad_status(&jc_r);
	if (!gpio_read(GPIO_PORT_E, GPIO_PIN_6))
		jc_req_nx_pad_status(&jc_l);

	if (!gpio_read(GPIO_PORT_H, GPIO_PIN_6))
		jc_rcv_pkt(&jc_r);
	if (!gpio_read(GPIO_PORT_E, GPIO_PIN_6))
		jc_rcv_pkt(&jc_l);

	jc_conn_check();

	return &jc_gamepad;
}
