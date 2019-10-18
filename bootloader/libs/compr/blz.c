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

const blz_footer *blz_get_footer(const unsigned char *compData, unsigned int compDataLen, blz_footer *outFooter)
{
	if (compDataLen < sizeof(blz_footer))
		return NULL;

	const blz_footer *srcFooter = (const blz_footer*)&compData[compDataLen - sizeof(blz_footer)];
	if (outFooter != NULL)
		memcpy(outFooter, srcFooter, sizeof(blz_footer)); // Must be a memcpy because no umaligned accesses on ARMv4.

	return srcFooter;
}

// From https://github.com/SciresM/hactool/blob/master/kip.c which is exactly how kernel does it, thanks SciresM!
int blz_uncompress_inplace(unsigned char *dataBuf, unsigned int compSize, const blz_footer *footer)
{
	u32 addl_size = footer->addl_size;
	u32 header_size = footer->header_size;
	u32 cmp_and_hdr_size = footer->cmp_and_hdr_size;

	unsigned char* cmp_start = &dataBuf[compSize] - cmp_and_hdr_size;
	u32 cmp_ofs = cmp_and_hdr_size - header_size;
	u32 out_ofs = cmp_and_hdr_size + addl_size;

	while (out_ofs)
	{
		unsigned char control = cmp_start[--cmp_ofs];
		for (unsigned int i=0; i<8; i++)
		{
			if (control & 0x80)
			{
				if (cmp_ofs < 2)
					return 0; // Out of bounds.

				cmp_ofs -= 2;
				u16 seg_val = ((unsigned int)(cmp_start[cmp_ofs + 1]) << 8) | cmp_start[cmp_ofs];
				u32 seg_size = ((seg_val >> 12) & 0xF) + 3;
				u32 seg_ofs = (seg_val & 0x0FFF) + 3;
				if (out_ofs < seg_size) // Kernel restricts segment copy to stay in bounds.
					seg_size = out_ofs;

				out_ofs -= seg_size;

				for (unsigned int j = 0; j < seg_size; j++)
					cmp_start[out_ofs + j] = cmp_start[out_ofs + j + seg_ofs];
			}
			else
			{
				// Copy directly.
				if (cmp_ofs < 1)
					return 0; //out of bounds

				cmp_start[--out_ofs] = cmp_start[--cmp_ofs];
			}
			control <<= 1;
			if (out_ofs == 0) // Blz works backwards, so if it reaches byte 0, it's done.
				return 1;
			}
		}

	return 1;
}

int blz_uncompress_srcdest(const unsigned char *compData, unsigned int compDataLen, unsigned char *dstData, unsigned int dstSize)
{
	blz_footer footer;
	const blz_footer *compFooterPtr = blz_get_footer(compData, compDataLen, &footer);
	if (compFooterPtr == NULL)
		return 0;

	// Decompression must be done in-place, so need to copy the relevant compressed data first.
	unsigned int numCompBytes = (const unsigned char*)(compFooterPtr)-compData;
	memcpy(dstData, compData, numCompBytes);
	memset(&dstData[numCompBytes], 0, dstSize - numCompBytes);

	return blz_uncompress_inplace(dstData, compDataLen, &footer);
}
