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

#include <string.h>

#include "di.h"
#include "../soc/t210.h"
#include "../utils/util.h"
#include "../soc/i2c.h"
#include "../soc/pmc.h"
#include "../power/max77620.h"
#include "../power/max7762x.h"
#include "../soc/gpio.h"
#include "../soc/pinmux.h"
#include "../soc/clock.h"

#include "di.inl"

static u32 _display_ver = 0;

static void _display_dsi_wait(u32 timeout, u32 off, u32 mask)
{
	u32 end = get_tmr_us() + timeout;
	while (get_tmr_us() < end && DSI(off) & mask)
		;
	usleep(5);
}

void display_init()
{
	// Power on.
	max77620_regulator_set_volt_and_flags(REGULATOR_LDO0, 1200000, MAX77620_POWER_MODE_NORMAL); // Configure to 1.2V.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_GPIO7, MAX77620_CNFG_GPIO_OUTPUT_VAL_HIGH | MAX77620_CNFG_GPIO_DRV_PUSHPULL);

	// Enable MIPI CAL, DSI, DISP1, HOST1X, UART_FST_MIPI_CAL, DSIA LP clocks.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_CLR) = 0x1010000;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = 0x1010000;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = 0x18000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = 0x18000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) = 0x20000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIP_CAL) = 0xA;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_SET) = 0x80000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP) = 0xA;

	// DPD idle.
	PMC(APBDEV_PMC_IO_DPD_REQ) = 0x40000000;
	PMC(APBDEV_PMC_IO_DPD2_REQ) = 0x40000000;

	// Config pins.
	PINMUX_AUX(PINMUX_AUX_NFC_EN) &= ~PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_NFC_INT) &= ~PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) &= ~PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_LCD_BL_EN) &= ~PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_LCD_RST) &= ~PINMUX_TRISTATE;

	gpio_config(GPIO_PORT_I, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_GPIO); // Backlight +-5V.
	gpio_output_enable(GPIO_PORT_I, GPIO_PIN_0 | GPIO_PIN_1, GPIO_OUTPUT_ENABLE); // Backlight +-5V.
	gpio_write(GPIO_PORT_I, GPIO_PIN_0, GPIO_HIGH); // Backlight +5V enable.

	usleep(10000);

	gpio_write(GPIO_PORT_I, GPIO_PIN_1, GPIO_HIGH); // Backlight -5V enable.

	usleep(10000);

	gpio_config(GPIO_PORT_V, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_GPIO); // Backlight PWM, Enable, Reset.
	gpio_output_enable(GPIO_PORT_V, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_OUTPUT_ENABLE);
	gpio_write(GPIO_PORT_V, GPIO_PIN_1, GPIO_HIGH); // Backlight Enable enable.

	// Config display interface and display.
	MIPI_CAL(MIPI_CAL_MIPI_BIAS_PAD_CFG2) = 0;

	exec_cfg((u32 *)CLOCK_BASE, _display_config_1, 4);
	exec_cfg((u32 *)DISPLAY_A_BASE, _display_config_2, 94);
	exec_cfg((u32 *)DSI_BASE, _display_config_3, 61);

	usleep(10000);

	gpio_write(GPIO_PORT_V, GPIO_PIN_2, GPIO_HIGH); // Backlight Reset enable.

	usleep(60000);

	DSI(_DSIREG(DSI_BTA_TIMING)) = 0x50204;
	DSI(_DSIREG(DSI_WR_DATA)) = 0x337; // MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE
	DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
	_display_dsi_wait(250000, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	DSI(_DSIREG(DSI_WR_DATA)) = 0x406; // MIPI_DCS_GET_DISPLAY_ID
	DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
	_display_dsi_wait(250000, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	DSI(_DSIREG(DSI_HOST_CONTROL)) = DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_IMM_BTA | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC;
	_display_dsi_wait(150000, _DSIREG(DSI_HOST_CONTROL), DSI_HOST_CONTROL_IMM_BTA);

	usleep(5000);

	_display_ver = DSI(_DSIREG(DSI_RD_DATA));
	if (_display_ver == 0x10)
		exec_cfg((u32 *)DSI_BASE, _display_config_4, 43);

	DSI(_DSIREG(DSI_WR_DATA)) = 0x1105; // MIPI_DCS_EXIT_SLEEP_MODE
	DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;

	usleep(180000);

	DSI(_DSIREG(DSI_WR_DATA)) = 0x2905; // MIPI_DCS_SET_DISPLAY_ON
	DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;

	usleep(20000);

	exec_cfg((u32 *)CLOCK_BASE, _display_config_6, 3);
	exec_cfg((u32 *)DSI_BASE, _display_config_5, 21);
	DISPLAY_A(_DIREG(DC_DISP_DISP_CLOCK_CONTROL)) = 4;
	exec_cfg((u32 *)DSI_BASE, _display_config_7, 10);

	usleep(10000);

	exec_cfg((u32 *)MIPI_CAL_BASE, _display_config_8, 6);
	exec_cfg((u32 *)DSI_BASE, _display_config_9, 4);
	exec_cfg((u32 *)MIPI_CAL_BASE, _display_config_10, 16);

	usleep(10000);

	exec_cfg((u32 *)DISPLAY_A_BASE, _display_config_11, 113);
}

void display_backlight_pwm_init()
{
	clock_enable_pwm();

	PWM(PWM_CONTROLLER_PWM_CSR_0) = (1 << 31); // Enable PWM

	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) >> 2) << 2 | 1; // PWM clock source.
	gpio_config(GPIO_PORT_V, GPIO_PIN_0, GPIO_MODE_SPIO); // Backlight power mode.
	
}

void display_backlight(bool enable)
{
	gpio_write(GPIO_PORT_V, GPIO_PIN_0, enable ? GPIO_HIGH : GPIO_LOW); // Backlight PWM GPIO.
}

void display_backlight_brightness(u32 brightness, u32 step_delay)
{
	u32 old_value = (PWM(PWM_CONTROLLER_PWM_CSR_0) >> 16) & 0xFF;
	if (brightness == old_value)
		return;

	if (brightness > 255)
		brightness = 255;

	if (old_value < brightness)
	{
		for (u32 i = old_value; i < brightness + 1; i++)
		{
			PWM(PWM_CONTROLLER_PWM_CSR_0) = (1 << 31) | (i << 16); // Enable PWM
			usleep(step_delay);
		}
	}
	else
	{
		for (u32 i = old_value; i > brightness; i--)
		{
			PWM(PWM_CONTROLLER_PWM_CSR_0) = (1 << 31) | (i << 16); // Enable PWM
			usleep(step_delay);
		}
	}
	if (!brightness)
		PWM(PWM_CONTROLLER_PWM_CSR_0) = 0;
}

void display_end()
{
	display_backlight_brightness(0, 1000);

	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 1;
	DSI(_DSIREG(DSI_WR_DATA)) = 0x2805; // MIPI_DCS_SET_DISPLAY_OFF

	DISPLAY_A(_DIREG(DC_CMD_STATE_ACCESS)) = READ_MUX | WRITE_MUX;
	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 0;

	exec_cfg((u32 *)DISPLAY_A_BASE, _display_config_12, 17);
	exec_cfg((u32 *)DSI_BASE, _display_config_13, 16);

	usleep(10000);

	if (_display_ver == 0x10)
		exec_cfg((u32 *)DSI_BASE, _display_config_14, 22);

	DSI(_DSIREG(DSI_WR_DATA)) = 0x1005; // MIPI_DCS_ENTER_SLEEP_MODE
	DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;

	usleep(50000);

	gpio_write(GPIO_PORT_V, GPIO_PIN_2, GPIO_LOW); //Backlight Reset disable.

	usleep(10000);

	gpio_write(GPIO_PORT_I, GPIO_PIN_1, GPIO_LOW); //Backlight -5V disable.

	usleep(10000);

	gpio_write(GPIO_PORT_I, GPIO_PIN_0, GPIO_LOW); //Backlight +5V disable.

	usleep(10000);

	// Disable clocks.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_SET) = 0x1010000;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_CLR) = 0x1010000;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = 0x18000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = 0x18000000;

	DSI(_DSIREG(DSI_PAD_CONTROL_0)) = DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) | DSI_PAD_CONTROL_VS1_PDIO_CLK | DSI_PAD_CONTROL_VS1_PDIO(0xF);
	DSI(_DSIREG(DSI_POWER_CONTROL)) = 0;

	gpio_config(GPIO_PORT_V, GPIO_PIN_0, GPIO_MODE_SPIO); // Backlight PWM.

	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) & ~PINMUX_TRISTATE) | PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) >> 2) << 2 | 1;
}

void display_color_screen(u32 color)
{
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_one_color, 8);

	// Configure display to show single color.
	DISPLAY_A(_DIREG(DC_WIN_AD_WIN_OPTIONS)) = 0;
	DISPLAY_A(_DIREG(DC_WIN_BD_WIN_OPTIONS)) = 0;
	DISPLAY_A(_DIREG(DC_WIN_CD_WIN_OPTIONS)) = 0;
	DISPLAY_A(_DIREG(DC_DISP_BLEND_BACKGROUND_COLOR)) = color;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = (DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) & 0xFFFFFFFE) | GENERAL_ACT_REQ;

	usleep(35000);

	display_backlight(true);
}

u32 *display_init_framebuffer()
{
	// Sanitize framebuffer area.
	memset((u32 *)0xC0000000, 0, 0x3C0000);
	// This configures the framebuffer @ 0xC0000000 with a resolution of 1280x720 (line stride 768).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer, 32);

	usleep(35000);

	return (u32 *)0xC0000000;
}
