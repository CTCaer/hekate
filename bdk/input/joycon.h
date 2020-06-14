/*
 * Ambient light sensor driver for Nintendo Switch's Rohm BH1730
 *
 * Copyright (c) 2018 CTCaer
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

#ifndef __JOYCON_H_
#define __JOYCON_H_

#include <utils/types.h>

#define JC_BTNS_DIRECTION_PAD 0xF0000
#define JC_BTNS_PREV_NEXT     0x800080
#define JC_BTNS_ENTER         0x400008
#define JC_BTNS_ESC           0x4

#define JC_BTNS_ALL (JC_BTNS_PREV_NEXT | JC_BTNS_ENTER | JC_BTNS_DIRECTION_PAD | JC_BTNS_ESC)

typedef struct _jc_bt_conn_t
{
	u8 type;
	u8 mac[6];
	u8 host_mac[6];
	u8 ltk[16];
} jc_bt_conn_t;

typedef struct _jc_gamepad_rpt_t
{
	union
	{
		struct
		{
			// Joy-Con (R).
			u32 y:1;
			u32 x:1;
			u32 b:1;
			u32 a:1;
			u32 sr_r:1;
			u32 sl_r:1;
			u32 r:1;
			u32 zr:1;

			// Shared
			u32 minus:1;
			u32 plus:1;
			u32 r3:1;
			u32 l3:1;
			u32 home:1;
			u32 cap:1;
			u32 pad:1;
			u32 wired:1;

			// Joy-Con (L).
			u32 down:1;
			u32 up:1;
			u32 right:1;
			u32 left:1;
			u32 sr_l:1;
			u32 sl_l:1;
			u32 l:1;
			u32 zl:1;
		};
		u32 buttons;
	};

	u16 lstick_x;
	u16 lstick_y;
	u16 rstick_x;
	u16 rstick_y;
	bool center_stick_l;
	bool center_stick_r;
	bool conn_l;
	bool conn_r;
	u8 batt_info_l;
	u8 batt_info_r;
	jc_bt_conn_t bt_conn_l;
	jc_bt_conn_t bt_conn_r;
} jc_gamepad_rpt_t;

void jc_power_supply(u8 uart, bool enable);
void jc_init_hw();
void jc_deinit();
jc_gamepad_rpt_t *joycon_poll();
jc_gamepad_rpt_t *jc_get_bt_pairing_info(bool *is_l_hos, bool *is_r_hos);

#endif