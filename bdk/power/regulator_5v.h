/*
 * Copyright (c) 2019 CTCaer
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

#ifndef _REGULATOR_5V_H_
#define _REGULATOR_5V_H_

#include <utils/types.h>

enum
{
	REGULATOR_5V_FAN  = BIT(0),
	REGULATOR_5V_JC_R = BIT(1),
	REGULATOR_5V_JC_L = BIT(2),
	REGULATOR_5V_ALL  = 0xFF
};

void regulator_5v_enable(u8 dev);
void regulator_5v_disable(u8 dev);
bool regulator_5v_get_dev_enabled(u8 dev);
void regulator_5v_usb_src_enable(bool enable);

#endif