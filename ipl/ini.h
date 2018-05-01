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

#ifndef _INI_H_
#define _INI_H_

#include "types.h"
#include "list.h"

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
} ini_sec_t;

int ini_parse(link_t *dst, char *ini_path);

#endif
