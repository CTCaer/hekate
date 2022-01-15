/*
 * Copyright (c) 2020 CTCaer
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

#ifndef _FATFS_CFG_H_
#define _FATFS_CFG_H_

// define FFCFG_INC in a project to use a specific FatFS configuration.
// Example: FFCFG_INC := '"../$(PROJECT_DIR)/libs/fatfs/ffconf.h"'
#ifdef FFCFG_INC
#include FFCFG_INC
#else
#include "fatfs_conf.h"
#endif

#endif
