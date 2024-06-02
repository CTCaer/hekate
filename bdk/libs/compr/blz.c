/*
 * Copyright (c) 2018 rajkosto
 * Copyright (c) 2018 SciresM
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

#include <stdlib.h>
#include <string.h>

#include "blz.h"

const blz_footer *blz_get_footer(const u8 *comp_data, u32 comp_data_size, blz_footer *out_footer)
{
	if (comp_data_size < sizeof(blz_footer))
		return NULL;

	const blz_footer *src_footer = (const blz_footer *)&comp_data[comp_data_size - sizeof(blz_footer)];
	if (out_footer)
		memcpy(out_footer, src_footer, sizeof(blz_footer)); // Must be a memcpy because no unaligned accesses on ARMv4.

	return src_footer;
}

// From https://github.com/SciresM/hactool/blob/master/kip.c which is exactly how kernel does it, thanks SciresM!
int blz_uncompress_inplace(u8 *data, u32 comp_size, const blz_footer *footer)
{
	u32 addl_size = footer->addl_size;
	u32 header_size = footer->header_size;
	u32 cmp_and_hdr_size = footer->cmp_and_hdr_size;

	u8 *cmp_start = &data[comp_size] - cmp_and_hdr_size;
	u32 cmp_ofs = cmp_and_hdr_size - header_size;
	u32 out_ofs = cmp_and_hdr_size + addl_size;

	while (out_ofs)
	{
		u8 control = cmp_start[--cmp_ofs];
		for (u32 i = 0; i < 8; i++)
		{
			if (control & 0x80)
			{
				if (cmp_ofs < 2)
					return 0; // Out of bounds.

				cmp_ofs -= 2;
				u16 seg_val = ((u32)(cmp_start[cmp_ofs + 1]) << 8) | cmp_start[cmp_ofs];
				u32 seg_size = ((seg_val >> 12) & 0xF) + 3;
				u32 seg_ofs = (seg_val & 0x0FFF) + 3;

				// Kernel restricts segment copy to stay in bounds.
				if (out_ofs < seg_size)
					seg_size = out_ofs;

				out_ofs -= seg_size;

				for (u32 j = 0; j < seg_size; j++)
					cmp_start[out_ofs + j] = cmp_start[out_ofs + j + seg_ofs];
			}
			else // Copy directly.
			{
				if (cmp_ofs < 1)
					return 0; // Out of bounds.

				cmp_start[--out_ofs] = cmp_start[--cmp_ofs];
			}

			control <<= 1;

			if (!out_ofs) // Blz works backwards, so if it reaches byte 0, it's done.
				return 1;
		}
	}

	return 1;
}

int blz_uncompress_srcdest(const u8 *comp_data, u32 comp_data_size, u8 *dst_data, u32 dst_size)
{
	blz_footer footer;
	const blz_footer *comp_footer = blz_get_footer(comp_data, comp_data_size, &footer);
	if (!comp_footer)
		return 0;

	// Decompression happens in-place, so need to copy the relevant compressed data first.
	u32 comp_bytes = (const u8 *)comp_footer - comp_data;
	memcpy(dst_data, comp_data, comp_bytes);
	memset(&dst_data[comp_bytes], 0, dst_size - comp_bytes);

	return blz_uncompress_inplace(dst_data, comp_data_size, &footer);
}
