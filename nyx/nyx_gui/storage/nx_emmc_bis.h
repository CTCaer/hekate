/*
 * Copyright (c) 2019 shchmue
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

#ifndef NX_EMMC_BIS_H
#define NX_EMMC_BIS_H

#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"

int nx_emmc_bis_read(u32 sector, u32 count, void *buff);
void nx_emmc_bis_init(sdmmc_storage_t *storage, emmc_part_t *part);

#endif