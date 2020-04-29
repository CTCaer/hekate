/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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

#ifndef NX_SD_H
#define NX_SD_H

#include "sdmmc.h"
#include "sdmmc_driver.h"
#include "../libs/fatfs/ff.h"

sdmmc_t sd_sdmmc;
sdmmc_storage_t sd_storage;
FATFS sd_fs;

bool sd_mount();
void sd_unmount();
void *sd_file_read(const char *path, u32 *fsize);
int  sd_save_to_file(void *buf, u32 size, const char *filename);

#endif