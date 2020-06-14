/*
 * Copyright (c) 2019-2020 CTCaer
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
 * @file lv_color.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_color.h"

/*********************
 *      DEFINES
 *********************/

#define HUE_DEGREE 512

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

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Convert a HSV color to RGB
 * @param h hue [0..359]
 * @param s saturation [0..100]
 * @param v value [0..100]
 * @return the given RGB color in RGB (with LV_COLOR_DEPTH depth)
 */
lv_color_t lv_color_hsv_to_rgb(uint16_t hue, uint8_t sat, uint8_t val)
{
    uint8_t r, g, b;

    uint32_t h = (hue * 360 * HUE_DEGREE -1) / 360;
    uint32_t s = sat * 255 / 100;
    uint32_t v = val * 255 / 100;
    uint32_t p = (256 * v - s * v) / 256;
    uint32_t region = h / (60 * 512);
    
    if(sat == 0)
        return LV_COLOR_MAKE(v, v, v);

    if (region & 1)
    {
        uint32_t q = (256 * 60 * HUE_DEGREE * v - h * s * v + 60 * HUE_DEGREE * s * v * region) /
            (256 * 60 * HUE_DEGREE);

        switch (region)
        {
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 5:
        default:
            r = v;
            g = p;
            b = q;
            break;
        }
    }
    else
    {
        uint32_t t = (256 * 60 * HUE_DEGREE * v + h * s * v - 60 * HUE_DEGREE * s * v * (region + 1)) /
            (256 * 60 * HUE_DEGREE);

        switch (region)
        {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 4:
        default:
            r = t;
            g = p;
            b = v;
            break;
        }
    }

    return LV_COLOR_MAKE(r, g, b);
}

/**
 * Convert an RGB color to HSV
 * @param r red
 * @param g green
 * @param b blue
 * @return the given RGB color n HSV
 */
lv_color_hsv_t lv_color_rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b)
{
    lv_color_hsv_t hsv;
    uint8_t rgbMin, rgbMax;

    rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
    rgbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

    hsv.v = rgbMax;
    if(hsv.v == 0) {
        hsv.h = 0;
        hsv.s = 0;
        return hsv;
    }

    hsv.s = 255 * (long)(rgbMax - rgbMin) / hsv.v;
    if(hsv.s == 0) {
        hsv.h = 0;
        return hsv;
    }

    if(rgbMax == r)
        hsv.h = 0 + 43 * (g - b) / (rgbMax - rgbMin);
    else if(rgbMax == g)
        hsv.h = 85 + 43 * (b - r) / (rgbMax - rgbMin);
    else
        hsv.h = 171 + 43 * (r - g) / (rgbMax - rgbMin);

    return hsv;
}
