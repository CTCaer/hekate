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

#ifndef _KFUSE_H_
#define _KFUSE_H_

#include "types.h"

#define KFUSE_STATE_SOFTRESET (1<<31)
#define KFUSE_STATE_STOP (1<<25)
#define KFUSE_STATE_RESTART (1<<24)
#define KFUSE_STATE_CRCPASS (1<<17)
#define KFUSE_STATE_DONE (1<<16)
#define KFUSE_STATE_ERRBLOCK_MASK 0x3F00
#define KFUSE_STATE_ERRBLOCK_SHIFT 8
#define KFUSE_STATE_CURBLOCK_MASK 0x3F

#define KFUSE_KEYADDR_AUTOINC (1<<16)

#define KFUSE_STATE 0x80
#define KFUSE_KEYADDR 0x88
#define KFUSE_KEYS 0x8C

#define KFUSE_NUM_WORDS 144

int kfuse_read(u32 *buf);

#endif
