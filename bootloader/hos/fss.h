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

#ifndef _FSS_H_
#define _FSS_H_

#include "hos.h"

typedef struct _fss0_sept_t
{
	u32 kb;
	ini_sec_t *cfg_sec;
	void *sept_primary;
	void *sept_secondary;

} fss0_sept_t;

int parse_fss(launch_ctxt_t *ctxt, const char *path, fss0_sept_t *sept_ctxt);
int load_sept_from_ffs0(fss0_sept_t *sept_ctxt);

#endif
