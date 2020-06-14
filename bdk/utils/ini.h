/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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

#ifndef _INI_H_
#define _INI_H_

#include <utils/types.h>
#include <utils/list.h>

#define INI_CHOICE  3
#define INI_CAPTION 5
#define INI_CHGLINE 6
#define INI_NEWLINE 0xFE
#define INI_COMMENT 0xFF

typedef struct _ini_kv_t
{
	char *key;
	char *val;
	link_t link;
} ini_kv_t;

typedef struct _ini_sec_t
{
	char *name;
	link_t kvs;
	link_t link;
	u32 type;
	u32 color;
} ini_sec_t;

int ini_parse(link_t *dst, char *ini_path, bool is_dir);
char *ini_check_payload_section(ini_sec_t *cfg);

#endif

