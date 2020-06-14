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
 * @file lv_font_built_in.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_font_builtin.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the built-in fonts
 */
void lv_font_builtin_init(void)
{
    /*InterUI 20*/
#if USE_INTERUI_20 != 0
    lv_font_add(&interui_20, NULL);
#endif

    /*SYMBOL 20*/
#if USE_HEKATE_SYMBOL_20 != 0
#if USE_INTERUI_20 != 0
    lv_font_add(&hekate_symbol_20, &interui_20);
#else
    lv_font_add(&hekate_symbol_20, NULL);
#endif
#endif

    /*InterUI 30*/
#if USE_INTERUI_30 != 0
    lv_font_add(&interui_30, NULL);
#endif

    /*SYMBOL 30*/
#if USE_HEKATE_SYMBOL_30 != 0
#if USE_INTERUI_30 != 0
    lv_font_add(&hekate_symbol_30, &interui_30);
#else
    lv_font_add(&hekate_symbol_30, NULL);
#endif
#endif

	/*MONO 12*/
#if USE_UBUNTU_MONO != 0
	lv_font_add(&ubuntu_mono, NULL);
#if USE_INTERUI_20 != 0
    lv_font_add(&hekate_symbol_20, &ubuntu_mono);
#endif
#endif

	/*Symbol 120*/
#if USE_HEKATE_SYMBOL_120 != 0
	lv_font_add(&hekate_symbol_120, NULL);
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
