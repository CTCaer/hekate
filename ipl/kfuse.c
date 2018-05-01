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

#include "kfuse.h"
#include "clock.h"
#include "t210.h"

int kfuse_read(u32 *buf)
{
	int res = 0;

	clock_enable_kfuse();

	while (!(KFUSE(KFUSE_STATE) & KFUSE_STATE_DONE))
		;

	if (!(KFUSE(KFUSE_STATE) & KFUSE_STATE_CRCPASS))
		goto out;

	KFUSE(KFUSE_KEYADDR) = KFUSE_KEYADDR_AUTOINC;
	for (int i = 0; i < KFUSE_NUM_WORDS; i++)
		buf[i] = KFUSE(KFUSE_KEYS);

	res = 1;

out:;
	clock_disable_kfuse();
	return res;
}
