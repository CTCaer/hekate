/*
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

#include <string.h>

#include "hw_init.h"
#include "bpmp.h"
#include "clock.h"
#include "gpio.h"
#include "pmc.h"
#include "t210.h"
#include "../mem/minerva.h"
#include "../gfx/di.h"
#include "../input/touch.h"
#include "../storage/sdmmc.h"
#include "../utils/util.h"

extern sdmmc_t sd_sdmmc;


void reconfig_hw_workaround(bool extra_reconfig, u32 magic)
{
	// Flush and disable MMU.
	bpmp_mmu_disable();
	bpmp_clk_rate_set(BPMP_CLK_NORMAL);
	minerva_change_freq(FREQ_204);

	touch_power_off();

	// Re-enable clocks to Audio Processing Engine as a workaround to hanging.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_V) |= (1 << 10); // Enable AHUB clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_Y) |= (1 <<  6); // Enable APE clock.

	if (extra_reconfig)
	{
		msleep(10);
		PMC(APBDEV_PMC_PWR_DET_VAL) |= PMC_PWR_DET_SDMMC1_IO_EN;

		clock_disable_cl_dvfs();

		// Disable Joy-con GPIOs.
		gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_E, GPIO_PIN_6, GPIO_MODE_SPIO);
		gpio_config(GPIO_PORT_H, GPIO_PIN_6, GPIO_MODE_SPIO);
	}

	// Power off display.
	display_end();

	// Enable clock to USBD and init SDMMC1 to avoid hangs with bad hw inits.
	if (magic == 0xBAADF00D)
	{
		CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) |= (1 << 22);
		sdmmc_init(&sd_sdmmc, SDMMC_1, SDMMC_POWER_3_3, SDMMC_BUS_WIDTH_1, 5, 0);
		clock_disable_cl_dvfs();

		msleep(200);
	}
}