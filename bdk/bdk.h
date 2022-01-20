/*
 * Copyright (c) 2022 CTCaer
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

#ifndef BDK_H
#define BDK_H

#include <memory_map.h>

#include <display/di.h>
#include <input/als.h>
#include <input/joycon.h>
#include <input/touch.h>
#include <mem/emc.h>
#include <mem/heap.h>
#include <mem/mc.h>
#include <mem/minerva.h>
#include <mem/sdram.h>
#include <mem/smmu.h>
#include <module.h>
#include <power/bm92t36.h>
#include <power/bq24193.h>
#include <power/max17050.h>
#include <power/max77620.h>
#include <power/max7762x.h>
#include <power/max77812.h>
#include <power/regulator_5v.h>
#include <rtc/max77620-rtc.h>
#include <sec/se.h>
#include <sec/tsec.h>
#include <soc/actmon.h>
#include <soc/bpmp.h>
#include <soc/ccplex.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/gpio.h>
#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/kfuse.h>
#include <soc/pinmux.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <soc/uart.h>
#include <storage/emmc.h>
#include <storage/mbr_gpt.h>
#include <storage/mmc.h>
#include <storage/nx_emmc_bis.h>
#include <storage/ramdisk.h>
#include <storage/sd.h>
#include <storage/sdmmc.h>
#include <thermal/fan.h>
#include <thermal/tmp451.h>
#include <usb/usbd.h>
#include <utils/aarch64_util.h>
#include <utils/btn.h>
#include <utils/dirlist.h>
#include <utils/ini.h>
#include <utils/list.h>
#include <utils/sprintf.h>
#include <utils/types.h>
#include <utils/util.h>

#include <gfx_utils.h>

#endif