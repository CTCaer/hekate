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

#ifndef LV_SYMBOL_DEF_H
#define LV_SYMBOL_DEF_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_conf.h"
#else
#include "../../lv_conf.h"
#endif

/*
 * With no UTF-8 support (192- 255) (192..241 is used)
 *
 * With UTF-8 support (in Supplemental Private Use Area-A): 0xF800 .. 0xF831
 * - Basic symbols:     0xE000..0xE01F
 * - File symbols:      0xE020..0xE03F
 * - Feedback symbols:  0xE040..0xE05F
 * - Reserved:          0xE060..0xE07F
 */

#if LV_TXT_UTF8 == 0
#define LV_SYMBOL_GLYPH_FIRST  0xC0
#define SYMBOL_DOT             _SYMBOL_VALUE1(C0)
#define SYMBOL_CLOCK           _SYMBOL_VALUE1(C1)
#define SYMBOL_LIST            _SYMBOL_VALUE1(C2)
#define SYMBOL_OK              _SYMBOL_VALUE1(C3)
#define SYMBOL_CLOSE           _SYMBOL_VALUE1(C4)
#define SYMBOL_POWER           _SYMBOL_VALUE1(C5)
#define SYMBOL_SETTINGS        _SYMBOL_VALUE1(C6)
#define SYMBOL_TRASH           _SYMBOL_VALUE1(C7)
#define SYMBOL_HOME            _SYMBOL_VALUE1(C8)
#define SYMBOL_DOWNLOAD        _SYMBOL_VALUE1(C9)
#define SYMBOL_DRIVE           _SYMBOL_VALUE1(CA)
#define SYMBOL_REFRESH         _SYMBOL_VALUE1(CB)
#define SYMBOL_REBOOT          _SYMBOL_VALUE1(CC)
#define SYMBOL_CHIP            _SYMBOL_VALUE1(CD)
#define SYMBOL_SD              _SYMBOL_VALUE1(CE)
#define SYMBOL_CIRCUIT         _SYMBOL_VALUE1(CF)
#define SYMBOL_EDIT            _SYMBOL_VALUE1(D0)
#define SYMBOL_FILE_ALT        _SYMBOL_VALUE1(D1)
#define SYMBOL_FILE_CODE       _SYMBOL_VALUE1(D2)
#define SYMBOL_FILE_ARC        _SYMBOL_VALUE1(D3)
#define SYMBOL_TEMPERATURE     _SYMBOL_VALUE1(D4)
#define SYMBOL_MODULES         _SYMBOL_VALUE1(D5)
#define SYMBOL_MODULES_ALT     _SYMBOL_VALUE1(D6)
#define SYMBOL_LEFT            _SYMBOL_VALUE1(D7)
#define SYMBOL_RIGHT           _SYMBOL_VALUE1(D8)
#define SYMBOL_KEY             _SYMBOL_VALUE1(D9)
#define SYMBOL_INFO            _SYMBOL_VALUE1(DA)
#define SYMBOL_WARNING         _SYMBOL_VALUE1(DB)
#define SYMBOL_SHUFFLE         _SYMBOL_VALUE1(DC)
#define SYMBOL_UP              _SYMBOL_VALUE1(DD)
#define SYMBOL_DOWN            _SYMBOL_VALUE1(DE)
#define SYMBOL_BRIGHTNESS      _SYMBOL_VALUE1(DF)
#define SYMBOL_DIRECTORY       _SYMBOL_VALUE1(E0)
#define SYMBOL_UPLOAD          _SYMBOL_VALUE1(E1)
#define SYMBOL_USB             _SYMBOL_VALUE1(E2)
#define SYMBOL_TOOLS           _SYMBOL_VALUE1(E3)
#define SYMBOL_COPY            _SYMBOL_VALUE1(E4)
#define SYMBOL_SAVE            _SYMBOL_VALUE1(E5)
#define SYMBOL_CHARGE          _SYMBOL_VALUE1(E6)
#define SYMBOL_HINT            _SYMBOL_VALUE1(E7)
#define SYMBOL_KEYBOARD        _SYMBOL_VALUE1(E8)
#define SYMBOL_GPS             _SYMBOL_VALUE1(E9)
#define SYMBOL_FILE            _SYMBOL_VALUE1(EA)
#define SYMBOL_CAMERA          _SYMBOL_VALUE1(EB)
#define SYMBOL_BATTERY_FULL    _SYMBOL_VALUE1(EC)
#define SYMBOL_BATTERY_3       _SYMBOL_VALUE1(ED)
#define SYMBOL_BATTERY_2       _SYMBOL_VALUE1(EE)
#define SYMBOL_BATTERY_1       _SYMBOL_VALUE1(EF)
#define SYMBOL_BATTERY_EMPTY   _SYMBOL_VALUE1(F0)
#define SYMBOL_SHRK            _SYMBOL_VALUE1(F1)
#define LV_SYMBOL_GLYPH_LAST   0xF1
#define SYMBOL_DUMMY           _SYMBOL_VALUE1(FF)       /*Invalid symbol. If written before a string then `lv_img` will show it as a label*/

#else
#define LV_SYMBOL_GLYPH_FIRST  0xF800
#define SYMBOL_DOT             _SYMBOL_VALUE3(EF,A0,80)
#define SYMBOL_CLOCK           _SYMBOL_VALUE3(EF,A0,81)
#define SYMBOL_LIST            _SYMBOL_VALUE3(EF,A0,82)
#define SYMBOL_OK              _SYMBOL_VALUE3(EF,A0,83)
#define SYMBOL_CLOSE           _SYMBOL_VALUE3(EF,A0,84)
#define SYMBOL_POWER           _SYMBOL_VALUE3(EF,A0,85)
#define SYMBOL_SETTINGS        _SYMBOL_VALUE3(EF,A0,86)
#define SYMBOL_TRASH           _SYMBOL_VALUE3(EF,A0,87)
#define SYMBOL_HOME            _SYMBOL_VALUE3(EF,A0,88)
#define SYMBOL_DOWNLOAD        _SYMBOL_VALUE3(EF,A0,89)
#define SYMBOL_DRIVE           _SYMBOL_VALUE3(EF,A0,8A)
#define SYMBOL_REFRESH         _SYMBOL_VALUE3(EF,A0,8B)
#define SYMBOL_REBOOT          _SYMBOL_VALUE3(EF,A0,8C)
#define SYMBOL_CHIP            _SYMBOL_VALUE3(EF,A0,8D)
#define SYMBOL_SD              _SYMBOL_VALUE3(EF,A0,8E)
#define SYMBOL_CIRCUIT         _SYMBOL_VALUE3(EF,A0,8F)
#define SYMBOL_EDIT            _SYMBOL_VALUE3(EF,A0,90)
#define SYMBOL_FILE_ALT        _SYMBOL_VALUE3(EF,A0,91)
#define SYMBOL_FILE_CODE       _SYMBOL_VALUE3(EF,A0,92)
#define SYMBOL_FILE_ARC        _SYMBOL_VALUE3(EF,A0,93)
#define SYMBOL_TEMPERATURE     _SYMBOL_VALUE3(EF,A0,94)
#define SYMBOL_MODULES         _SYMBOL_VALUE3(EF,A0,95)
#define SYMBOL_MODULES_ALT     _SYMBOL_VALUE3(EF,A0,96)
#define SYMBOL_LEFT            _SYMBOL_VALUE3(EF,A0,97)
#define SYMBOL_RIGHT           _SYMBOL_VALUE3(EF,A0,98)
#define SYMBOL_KEY             _SYMBOL_VALUE3(EF,A0,99)
#define SYMBOL_INFO            _SYMBOL_VALUE3(EF,A0,9A)
#define SYMBOL_WARNING         _SYMBOL_VALUE3(EF,A0,9B)
#define SYMBOL_SHUFFLE         _SYMBOL_VALUE3(EF,A0,9C)
#define SYMBOL_UP              _SYMBOL_VALUE3(EF,A0,9D)
#define SYMBOL_DOWN            _SYMBOL_VALUE3(EF,A0,9E)
#define SYMBOL_BRIGHTNESS      _SYMBOL_VALUE3(EF,A0,9F)
#define SYMBOL_DIRECTORY       _SYMBOL_VALUE3(EF,A0,A0)
#define SYMBOL_UPLOAD          _SYMBOL_VALUE3(EF,A0,A1)
#define SYMBOL_USB             _SYMBOL_VALUE3(EF,A0,A2)
#define SYMBOL_TOOLS           _SYMBOL_VALUE3(EF,A0,A3)
#define SYMBOL_COPY            _SYMBOL_VALUE3(EF,A0,A4)
#define SYMBOL_SAVE            _SYMBOL_VALUE3(EF,A0,A5)
#define SYMBOL_CHARGE          _SYMBOL_VALUE3(EF,A0,A6)
#define SYMBOL_HINT            _SYMBOL_VALUE3(EF,A0,A7)
#define SYMBOL_KEYBOARD        _SYMBOL_VALUE3(EF,A0,A8)
#define SYMBOL_GPS             _SYMBOL_VALUE3(EF,A0,A9)
#define SYMBOL_FILE            _SYMBOL_VALUE3(EF,A0,AA)
#define SYMBOL_CAMERA          _SYMBOL_VALUE3(EF,A0,AB)
#define SYMBOL_BATTERY_FULL    _SYMBOL_VALUE3(EF,A0,AC)
#define SYMBOL_BATTERY_3       _SYMBOL_VALUE3(EF,A0,AD)
#define SYMBOL_BATTERY_2       _SYMBOL_VALUE3(EF,A0,AE)
#define SYMBOL_BATTERY_1       _SYMBOL_VALUE3(EF,A0,AF)
#define SYMBOL_BATTERY_EMPTY   _SYMBOL_VALUE3(EF,A0,B0)
#define SYMBOL_SHRK            _SYMBOL_VALUE3(EF,A0,B1)
#define LV_SYMBOL_GLYPH_LAST   0xF831
#define SYMBOL_DUMMY           _SYMBOL_VALUE3(EF,A3,BF)       /*Invalid symbol at (U+F831). If written before a string then `lv_img` will show it as a label*/
#endif

#define _SYMBOL_VALUE1(x) (0x ## x)
#define _SYMBOL_VALUE3(x, y, z) (0x ## z ## y ## x)
#define _SYMBOL_NUMSTR(sym) LV_ ## sym ## _NUMSTR = sym

enum
{
    _SYMBOL_NUMSTR(SYMBOL_DOT),
    _SYMBOL_NUMSTR(SYMBOL_CLOCK),
    _SYMBOL_NUMSTR(SYMBOL_LIST),
    _SYMBOL_NUMSTR(SYMBOL_OK),
    _SYMBOL_NUMSTR(SYMBOL_CLOSE),
    _SYMBOL_NUMSTR(SYMBOL_POWER),
    _SYMBOL_NUMSTR(SYMBOL_SETTINGS),
    _SYMBOL_NUMSTR(SYMBOL_TRASH),
    _SYMBOL_NUMSTR(SYMBOL_HOME),
    _SYMBOL_NUMSTR(SYMBOL_DOWNLOAD),
    _SYMBOL_NUMSTR(SYMBOL_DRIVE),
    _SYMBOL_NUMSTR(SYMBOL_REFRESH),
    _SYMBOL_NUMSTR(SYMBOL_REBOOT),
    _SYMBOL_NUMSTR(SYMBOL_CHIP),
    _SYMBOL_NUMSTR(SYMBOL_SD),
    _SYMBOL_NUMSTR(SYMBOL_CIRCUIT),
    _SYMBOL_NUMSTR(SYMBOL_EDIT),
    _SYMBOL_NUMSTR(SYMBOL_FILE_ALT),
    _SYMBOL_NUMSTR(SYMBOL_FILE_CODE),
    _SYMBOL_NUMSTR(SYMBOL_FILE_ARC),
    _SYMBOL_NUMSTR(SYMBOL_TEMPERATURE),
    _SYMBOL_NUMSTR(SYMBOL_MODULES),
    _SYMBOL_NUMSTR(SYMBOL_MODULES_ALT),
    _SYMBOL_NUMSTR(SYMBOL_LEFT),
    _SYMBOL_NUMSTR(SYMBOL_RIGHT),
    _SYMBOL_NUMSTR(SYMBOL_KEY),
    _SYMBOL_NUMSTR(SYMBOL_INFO),
    _SYMBOL_NUMSTR(SYMBOL_WARNING),
    _SYMBOL_NUMSTR(SYMBOL_SHUFFLE),
    _SYMBOL_NUMSTR(SYMBOL_UP),
    _SYMBOL_NUMSTR(SYMBOL_DOWN),
    _SYMBOL_NUMSTR(SYMBOL_BRIGHTNESS),
    _SYMBOL_NUMSTR(SYMBOL_DIRECTORY),
    _SYMBOL_NUMSTR(SYMBOL_UPLOAD),
    _SYMBOL_NUMSTR(SYMBOL_USB),
    _SYMBOL_NUMSTR(SYMBOL_TOOLS),
    _SYMBOL_NUMSTR(SYMBOL_COPY),
    _SYMBOL_NUMSTR(SYMBOL_SAVE),
    _SYMBOL_NUMSTR(SYMBOL_CHARGE),
    _SYMBOL_NUMSTR(SYMBOL_HINT),
    _SYMBOL_NUMSTR(SYMBOL_KEYBOARD),
    _SYMBOL_NUMSTR(SYMBOL_GPS),
    _SYMBOL_NUMSTR(SYMBOL_FILE),
    _SYMBOL_NUMSTR(SYMBOL_CAMERA),
    _SYMBOL_NUMSTR(SYMBOL_BATTERY_FULL),
    _SYMBOL_NUMSTR(SYMBOL_BATTERY_3),
    _SYMBOL_NUMSTR(SYMBOL_BATTERY_2),
    _SYMBOL_NUMSTR(SYMBOL_BATTERY_1),
    _SYMBOL_NUMSTR(SYMBOL_BATTERY_EMPTY),
    _SYMBOL_NUMSTR(SYMBOL_SHRK),
    _SYMBOL_NUMSTR(SYMBOL_DUMMY),
};

#undef _SYMBOL_VALUE1
#undef _SYMBOL_VALUE3

#define _SYMBOL_STR_(x) #x
#define _SYMBOL_STR(x) _SYMBOL_STR_(x)
#define _SYMBOL_CHAR(c) \x ## c
#define _SYMBOL_VALUE1(x) _SYMBOL_STR(_SYMBOL_CHAR(x))
#define _SYMBOL_VALUE3(x, y, z) _SYMBOL_STR(_SYMBOL_CHAR(x)_SYMBOL_CHAR(y)_SYMBOL_CHAR(z))

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /*LV_SYMBOL_DEF_H*/
