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

/**
 * @file lv_font_builtin.h
 *
 */

#ifndef LV_FONT_BUILTIN_H
#define LV_FONT_BUILTIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *     INCLUDES
 *********************/
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_conf.h"
#else
#include "../../lv_conf.h"
#endif

#include "../lv_misc/lv_font.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the built-in fonts
 */
void lv_font_builtin_init(void);

/**********************
 *      MACROS
 **********************/

/**********************
 *  FONT DECLARATIONS
 **********************/

/*20 px */
#if USE_INTERUI_20
LV_FONT_DECLARE(interui_20);
#endif

#if USE_HEKATE_SYMBOL_20
LV_FONT_DECLARE(hekate_symbol_20);
#endif

/*30 px */
#if USE_INTERUI_30
LV_FONT_DECLARE(interui_30);
#endif

#if USE_HEKATE_SYMBOL_30
LV_FONT_DECLARE(hekate_symbol_30);
#endif

#if USE_UBUNTU_MONO
LV_FONT_DECLARE(ubuntu_mono);
#endif

#if USE_HEKATE_SYMBOL_120
LV_FONT_DECLARE(hekate_symbol_120);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_FONT_BUILTIN_H*/
