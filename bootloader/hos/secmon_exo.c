/*
 * Copyright (C) 2018 CTCaer
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

#include <string.h>

#include "hos.h"
#include "../utils/types.h"

// Exosphère mailbox defines.
#define EXO_MAGIC_ADDR 0x40002E40
#define  EXO_MAGIC_VAL     0x31434258
#define EXO_FWNO_ADDR  0x40002E44
#define EXO_FLAGS_ADDR 0x40002E48
#define  EXO_FLAG_620_KGN  (1 << 0)
#define  EXO_FLAG_DBG_PRIV (1 << 1)
#define  EXO_FLAG_DBG_USER (1 << 2)

void config_exosphere(const char *id, u32 kb, bool debug)
{
	u32 exoFwNo = 0;
	u32 exoFlags = 0;

	vu32 *mb_exo_magic = (vu32 *)EXO_MAGIC_ADDR;
	vu32 *mb_exo_fw_no = (vu32 *)EXO_FWNO_ADDR;
	vu32 *mb_exo_flags = (vu32 *)EXO_FLAGS_ADDR;

	switch (kb)
	{
	case KB_FIRMWARE_VERSION_100_200:
		if (!strcmp(id, "20161121183008"))
			exoFwNo = 1;
		else
			exoFwNo = 2;
		break;
	case KB_FIRMWARE_VERSION_300:
		exoFwNo = 3;
		break;
	default:
		exoFwNo = kb + 1;
		break;
	}

	if (kb >= KB_FIRMWARE_VERSION_620)
		exoFlags |= EXO_FLAG_620_KGN;

	if (debug)
		exoFlags |= EXO_FLAG_DBG_PRIV;

	// Set mailbox values.
	*mb_exo_magic = EXO_MAGIC_VAL;
	*mb_exo_fw_no = exoFwNo;
	*mb_exo_flags = exoFlags;
}
