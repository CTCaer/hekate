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

#ifndef _ARM64_H_
#define _ARM64_H_

#include "types.h"

#define LSL0 0
#define LSL16 16
#define LSL32 32

#define _PAGEOFF(x) ((x) & 0xFFFFF000)

#define _ADDXI(d, n, i) 0x91000000 | (((i) & 0xFFF) << 10) | (((n) & 0x1F) << 5) | ((d) & 0x1F)
#define _ADRP(r, o) 0x90000000 | ((((o) >> 12) & 0x3) << 29) | ((((o) >> 12) & 0x1FFFFC) << 3) | ((r) & 0x1F)
#define _BL(a, o) 0x94000000 | ((((o) - (a)) >> 2) & 0x3FFFFFF)
#define _B(a, o) 0x14000000 | ((((o) - (a)) >> 2) & 0x3FFFFFF)
#define _MOVKX(r, i, s) 0xF2800000 | (((s) & 0x30) << 17) | (((i) & 0xFFFF) << 5) | ((r) & 0x1F)
#define _MOVZX(r, i, s) 0xD2800000 | (((s) & 0x30) << 17) | (((i) & 0xFFFF) << 5) | ((r) & 0x1F)
#define _NOP() 0xD503201F
#define _STRUI(t, n, i) 0xF9000000 | (((i) & 0xFFF) << 10) | (((n) & 0x1F) << 5) | ((t) & 0x1F)
#endif
