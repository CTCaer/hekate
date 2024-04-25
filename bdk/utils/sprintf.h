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

#ifndef _SPRINTF_H_
#define _SPRINTF_H_

#include <stdarg.h>

#include <utils/types.h>

/*
 * Padding:
 *  Numbers:
 *   %3d:   Fill: ' ', Count: 3.
 *   % 3d:  Fill: ' ', Count: 3.
 *   %23d:  Fill: '2', Count: 3.
 *   % 23d: Fill: ' ', Count: 23.
 *   %223d: Fill: '2', Count: 23.
 *
 * Strings, Fill: ' ':
 *  %3s:    Count: 5,   Left.
 *  %23s:   Count: 5,   Left.
 *  %223s:  Count: 25,  Left.
 *  %.3s:   Count: 5,   Right.
 *  %.23s:  Count: 25,  Right.
 *  %.223s: Count: 225, Right.
 */

void s_printf(char *out_buf, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void s_vprintf(char *out_buf, const char *fmt, va_list ap);

#endif
