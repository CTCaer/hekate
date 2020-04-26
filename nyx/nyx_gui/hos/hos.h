/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 CTCaer
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

#ifndef _HOS_H_
#define _HOS_H_

#include "pkg1.h"
#include "pkg2.h"
#include "../utils/types.h"
#include "../config/ini.h"
#include "../sec/tsec.h"

#define KB_FIRMWARE_VERSION_100_200 0
#define KB_FIRMWARE_VERSION_300 1
#define KB_FIRMWARE_VERSION_301 2
#define KB_FIRMWARE_VERSION_400 3
#define KB_FIRMWARE_VERSION_500 4
#define KB_FIRMWARE_VERSION_600 5
#define KB_FIRMWARE_VERSION_620 6
#define KB_FIRMWARE_VERSION_700 7
#define KB_FIRMWARE_VERSION_810 8
#define KB_FIRMWARE_VERSION_900 9
#define KB_FIRMWARE_VERSION_910 10
#define KB_FIRMWARE_VERSION_MAX KB_FIRMWARE_VERSION_910

#define HOS_PKG11_MAGIC 0x31314B50

typedef struct _launch_ctxt_t
{
	void *keyblob;

	void *pkg1;
	const pkg1_id_t *pkg1_id;

	void *warmboot;
	u32   warmboot_size;
	void *secmon;
	u32   secmon_size;

	void *pkg2;
	u32   pkg2_size;

	bool   new_pkg2;
	void  *kernel;
	u32    kernel_size;
	link_t kip1_list;
	char*  kip1_patches;

	bool svcperm;
	bool debugmode;
	bool stock;
	bool atmosphere;
	bool exo_no_user_exceptions;
	bool exo_blank_prodinfo;
	bool exo_allow_writing_to_cal_sysmmc;
	bool emuMMC;

	ini_sec_t *cfg;
} launch_ctxt_t;

int keygen(u8 *keyblob, u32 kb, tsec_ctxt_t *tsec_ctxt);

#endif
