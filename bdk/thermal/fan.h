/*
 * Fan driver for Nintendo Switch
 *
 * Copyright (c) 2018-2024 CTCaer
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

#ifndef __FAN_H_
#define __FAN_H_

#include <utils/types.h>

// Disable: 0 (0 RPM), min duty: 1 (960 RPM), max duty 235 (11000 RPM).
void fan_set_duty(u32 duty);
// Passing NULL ptr on either of the two, disables results.
void fan_get_speed(u32 *duty, u32 *rpm);

void fan_set_from_temp(u32 temp);

#endif /* __FAN_H_ */
