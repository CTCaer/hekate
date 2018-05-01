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

#ifndef _SDMMC_H_
#define _SDMMC_H_

#include "types.h"
#include "sdmmc_driver.h"

/*! SDMMC storage context. */
typedef struct _sdmmc_storage_t
{
	sdmmc_t *sdmmc;
	u32 rca;
	int has_sector_access;
	u32 sec_cnt;
	int is_low_voltage;
	u32 partition;
	u8 cid[0x10];
	u8 csd[0x10];
	u8 scr[8];
} sdmmc_storage_t;

int sdmmc_storage_end(sdmmc_storage_t *storage);
int sdmmc_storage_read(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf);
int sdmmc_storage_write(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf);
int sdmmc_storage_init_mmc(sdmmc_storage_t *storage, sdmmc_t *sdmmc, u32 id, u32 bus_width, u32 type);
int sdmmc_storage_set_mmc_partition(sdmmc_storage_t *storage, u32 partition);
int sdmmc_storage_init_sd(sdmmc_storage_t *storage, sdmmc_t *sdmmc, u32 id, u32 bus_width, u32 type);
int sdmmc_storage_init_gc(sdmmc_storage_t *storage, sdmmc_t *sdmmc);

#endif
