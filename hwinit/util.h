/*
* Copyright (c) 2018 naehrwert
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

#ifndef _UTIL_H_
#define _UTIL_H_

#include "types.h"

typedef struct _cfg_op_t
{
	u32 off;
	u32 val;
} cfg_op_t;

void sleep(u32 ticks);
void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops);

#endif
