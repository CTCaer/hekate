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
	REGULATOR_5V_FAN  = (1 << 0),
	REGULATOR_5V_JC_R = (1 << 1),
	REGULATOR_5V_JC_L = (1 << 2),
	REGULATOR_5V_ALL  = 0xFF
};

void regulator_enable_5v(u8 dev);
void regulator_disable_5v(u8 dev);
bool regulator_get_5v_dev_enabled(u8 dev);

#endif