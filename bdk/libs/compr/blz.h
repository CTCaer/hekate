/*
 * Copyright (c) 2018 rajkosto
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

#ifndef _BLZ_H_
#define _BLZ_H_

#include <utils/types.h>

typedef struct _blz_footer
{
	u32 cmp_and_hdr_size;
	u32 header_size;
	u32 addl_size;
} blz_footer;

// Returns pointer to footer in comp_data if present, additionally copies it to out_footer if not NULL.
const blz_footer *blz_get_footer(const u8 *comp_data, u32 comp_data_size, blz_footer *out_footer);
// Returns 0 on failure.
int blz_uncompress_inplace(u8 *data, u32 comp_size, const blz_footer *footer);
// Returns 0 on failure.
int blz_uncompress_srcdest(const u8 *comp_data, u32 comp_data_size, u8 *dst_data, u32 dst_size);

#endif
