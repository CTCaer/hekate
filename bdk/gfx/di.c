/*
 * Copyright (c) 2018 naehrwert
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

#include "di.h"
#include <power/max77620.h>
#include <power/max7762x.h>
#include <soc/clock.h>
#include <soc/gpio.h>
#include <soc/i2c.h>
#include <soc/pinmux.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <utils/util.h>

#include "di.inl"

extern volatile nyx_storage_t *nyx_str;

static u32 _display_id = 0;

void display_end();

static void _display_dsi_wait(u32 timeout, u32 off, u32 mask)
{
	u32 end = get_tmr_us() + timeout;
	while (get_tmr_us() < end && DSI(off) & mask)
		;
	usleep(5);
}

static void _display_dsi_send_cmd(u8 cmd, u32 param, u32 wait)
{
	DSI(_DSIREG(DSI_WR_DATA)) = (param << 8) | cmd;
	DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;

	if (wait)
		usleep(wait);
}

void display_init()
{
	// Check if display is already initialized.
	if (CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) & 0x18000000)
		display_end();

	// Power on.
	max77620_regulator_set_volt_and_flags(REGULATOR_LDO0, 1200000, MAX77620_POWER_MODE_NORMAL); // Configure to 1.2V.
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_GPIO7, MAX77620_CNFG_GPIO_OUTPUT_VAL_HIGH | MAX77620_CNFG_GPIO_DRV_PUSHPULL);

	// Enable Display Interface specific clocks.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_CLR) = 0x1010000;  // Clear reset DSI, MIPI_CAL.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = 0x1010000;  // Set enable clock DSI, MIPI_CAL.

	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = 0x18000000; // Clear reset DISP1, HOST1X.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = 0x18000000; // Set enable clock DISP1, HOST1X.

	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) = 0x20000;    // Set enable clock UART_FST_MIPI_CAL.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL) = 10; // Set PLLP_OUT3 and div 6 (17MHz).

	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_SET) = 0x80000;    // Set enable clock DSIA_LP.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP) = 10;    // Set PLLP_OUT and div 6 (68MHz).

	// Disable deep power down.
	PMC(APBDEV_PMC_IO_DPD_REQ) = 0x40000000;
	PMC(APBDEV_PMC_IO_DPD2_REQ) = 0x40000000;

	// Config LCD and Backlight pins.
	PINMUX_AUX(PINMUX_AUX_NFC_EN) &= ~PINMUX_TRISTATE;     // PULL_DOWN
	PINMUX_AUX(PINMUX_AUX_NFC_INT) &= ~PINMUX_TRISTATE;    // PULL_DOWN
	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) &= ~PINMUX_TRISTATE; // PULL_DOWN | 1
	PINMUX_AUX(PINMUX_AUX_LCD_BL_EN) &= ~PINMUX_TRISTATE;  // PULL_DOWN
	PINMUX_AUX(PINMUX_AUX_LCD_RST) &= ~PINMUX_TRISTATE;    // PULL_DOWN

	// Set Backlight +-5V pins mode and direction
	gpio_config(GPIO_PORT_I, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_I, GPIO_PIN_0 | GPIO_PIN_1, GPIO_OUTPUT_ENABLE);

	// Enable Backlight power.
	gpio_write(GPIO_PORT_I, GPIO_PIN_0, GPIO_HIGH); // Backlight +5V enable.
	usleep(10000);
	gpio_write(GPIO_PORT_I, GPIO_PIN_1, GPIO_HIGH); // Backlight -5V enable.
	usleep(10000);

	// Configure Backlight pins (PWM, EN, RST).
	gpio_config(GPIO_PORT_V, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_V, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_OUTPUT_ENABLE);
	gpio_write(GPIO_PORT_V, GPIO_PIN_1, GPIO_HIGH); // Enable Backlight EN.

	// Power up supply regulator for display interface.
	MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG2)) = 0;

	// Set DISP1 clock source and parent clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_DISP1) = 0x40000000; // PLLD_OUT.
	u32 plld_div = (3 << 20) | (20 << 11) | 1; // DIVM: 1, DIVN: 20, DIVP: 3. PLLD_OUT: 768 MHz, PLLD_OUT0 (DSI): 96 MHz.
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) = PLLCX_BASE_ENABLE | PLLCX_BASE_LOCK | plld_div;
	CLOCK(CLK_RST_CONTROLLER_PLLD_MISC1) = 0x20;    // PLLD_SETUP.
	CLOCK(CLK_RST_CONTROLLER_PLLD_MISC) = 0x2D0AAA; // PLLD_ENABLE_CLK.

	// Setup display communication interfaces.
	exec_cfg((u32 *)DISPLAY_A_BASE, _display_dc_setup_win_config, 94);
	exec_cfg((u32 *)DSI_BASE, _display_dsi_init_config, 61);
	usleep(10000);

	// Enable Backlight Reset.
	gpio_write(GPIO_PORT_V, GPIO_PIN_2, GPIO_HIGH);
	usleep(60000);

	// Setups DSI packet configuration and request display id.
	DSI(_DSIREG(DSI_BTA_TIMING)) = 0x50204;
	_display_dsi_send_cmd(MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE, 3, 0);
	_display_dsi_wait(250000, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	_display_dsi_send_cmd(MIPI_DSI_DCS_READ, MIPI_DCS_GET_DISPLAY_ID, 0);
	_display_dsi_wait(250000, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	DSI(_DSIREG(DSI_HOST_CONTROL)) = DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_IMM_BTA | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC;
	_display_dsi_wait(150000, _DSIREG(DSI_HOST_CONTROL), DSI_HOST_CONTROL_IMM_BTA);

	usleep(5000);

	// MIPI_DCS_GET_DISPLAY_ID reply is a long read, size 3 u32.
	for (u32 i = 0; i < 3; i++)
		_display_id = DSI(_DSIREG(DSI_RD_DATA)); // Skip ack and msg type info and get the payload (display id).

	// Save raw Display ID to Nyx storage.
	nyx_str->info.disp_id = _display_id;

	// Decode Display ID.
	_display_id = ((_display_id >> 8) & 0xFF00) | (_display_id & 0xFF);

	if ((_display_id & 0xFF) == PANEL_JDI_XXX062M)
		_display_id = PANEL_JDI_XXX062M;

	// Initialize display panel.
	switch (_display_id)
	{
	case PANEL_JDI_XXX062M:
		exec_cfg((u32 *)DSI_BASE, _display_init_config_jdi, 43);
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 180000);
		break;
	case PANEL_INL_P062CCA_AZ1:
	case PANEL_AUO_A062TAN01:
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 180000);
		DSI(_DSIREG(DSI_WR_DATA)) = 0x439;          // MIPI_DSI_DCS_LONG_WRITE: 4 bytes.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x9483FFB9;     // Enable extension cmd. (Pass: FF 83 94).
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		DSI(_DSIREG(DSI_WR_DATA)) = 0x739;          // MIPI_DSI_DCS_LONG_WRITE: 7 bytes.
		if (_display_id == PANEL_INL_P062CCA_AZ1)
			DSI(_DSIREG(DSI_WR_DATA)) = 0x751548B1; // Set Power control. (Not deep standby, BT5 / XDK, VRH gamma volt adj 53 / x40).
		else
			DSI(_DSIREG(DSI_WR_DATA)) = 0x711148B1; // Set Power control. (Not deep standby, BT1 / XDK, VRH gamma volt adj 49 / x40).
		DSI(_DSIREG(DSI_WR_DATA)) = 0x143209;       // (NVRH gamma volt adj 9, Amplifier current small / x30, FS0 freq Fosc/80 / FS1 freq Fosc/32).
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		break;
	case PANEL_INL_P062CCA_AZ2:
	case PANEL_AUO_A062TAN02:
	default: // Allow spare part displays to work.
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 120000);
		break;
	}

	_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_SET_DISPLAY_ON, 20000);

	// Configure PLLD for DISP1.
	plld_div = (1 << 20) | (24 << 11) | 1; // DIVM: 1, DIVN: 24, DIVP: 1. PLLD_OUT: 768 MHz, PLLD_OUT0 (DSI): 460.8 MHz.
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) = PLLCX_BASE_ENABLE | PLLCX_BASE_LOCK | plld_div;
	CLOCK(CLK_RST_CONTROLLER_PLLD_MISC1) = 0x20;
	CLOCK(CLK_RST_CONTROLLER_PLLD_MISC) = 0x2DFC00; // Use new PLLD_SDM_DIN.

	// Finalize DSI configuration.
	exec_cfg((u32 *)DSI_BASE, _display_dsi_packet_config, 21);
	DISPLAY_A(_DIREG(DC_DISP_DISP_CLOCK_CONTROL)) = 4; // PCD1 | div3.
	exec_cfg((u32 *)DSI_BASE, _display_dsi_mode_config, 10);
	usleep(10000);

	// Calibrate display communication pads.
	exec_cfg((u32 *)MIPI_CAL_BASE, _display_mipi_pad_cal_config, 6);
	exec_cfg((u32 *)DSI_BASE, _display_dsi_pad_cal_config, 4);
	exec_cfg((u32 *)MIPI_CAL_BASE, _display_mipi_apply_dsi_cal_config, 16);
	usleep(10000);

	// Enable video display controller.
	exec_cfg((u32 *)DISPLAY_A_BASE, _display_video_disp_controller_enable_config, 113);
}

void display_backlight_pwm_init()
{
	clock_enable_pwm();

	PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN; // Enable PWM and set it to 25KHz PFM.

	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) & 0xFFFFFFFC) | 1; // PWM clock source.
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
			PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN | (i << 16); // Enable PWM and set it to 25KHz PFM.
			usleep(step_delay);
		}
	}
	else
	{
		for (u32 i = old_value; i > brightness; i--)
		{
			PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN | (i << 16); // Enable PWM and set it to 25KHz PFM.
			usleep(step_delay);
		}
	}
	if (!brightness)
		PWM(PWM_CONTROLLER_PWM_CSR_0) = 0;
}

void display_end()
{
	display_backlight_brightness(0, 1000);

	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = DSI_CMD_PKT_VID_ENABLE;
	DSI(_DSIREG(DSI_WR_DATA)) = 0x2805; // MIPI_DCS_SET_DISPLAY_OFF

	DISPLAY_A(_DIREG(DC_CMD_STATE_ACCESS)) = READ_MUX | WRITE_MUX;
	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 0; // Disable host cmd packet.

	// De-initialize video controller.
	exec_cfg((u32 *)DISPLAY_A_BASE, _display_video_disp_controller_disable_config, 17);
	exec_cfg((u32 *)DSI_BASE, _display_dsi_timing_deinit_config, 16);
	usleep(10000);

	// De-initialize display panel.
	switch (_display_id)
	{
	case PANEL_JDI_XXX062M:
		exec_cfg((u32 *)DSI_BASE, _display_deinit_config_jdi, 22);
		break;
	case PANEL_AUO_A062TAN01:
		exec_cfg((u32 *)DSI_BASE, _display_deinit_config_auo, 37);
		break;
	case PANEL_INL_P062CCA_AZ2:
	case PANEL_AUO_A062TAN02:
		DSI(_DSIREG(DSI_WR_DATA)) = 0x439; // MIPI_DSI_DCS_LONG_WRITE: 4 bytes.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x9483FFB9; // Enable extension cmd. (Pass: FF 83 94).
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		// Set Power.
		DSI(_DSIREG(DSI_WR_DATA)) = 0xB39; // MIPI_DSI_DCS_LONG_WRITE: 11 bytes.
		if (_display_id == PANEL_INL_P062CCA_AZ2)
			DSI(_DSIREG(DSI_WR_DATA)) = 0x751548B1; // Set Power control. (Not deep standby, BT5 / XDK, VRH gamma volt adj 53 / x40).
		else
			DSI(_DSIREG(DSI_WR_DATA)) = 0x711148B1; // Set Power control. (Not deep standby, BT1 / XDK, VRH gamma volt adj 49 / x40).
		// Set Power control. (NVRH gamma volt adj 9, Amplifier current small / x30, FS0 freq Fosc/80 / FS1 freq Fosc/32, Enter standby / PON / VCOMG).
		DSI(_DSIREG(DSI_WR_DATA)) = 0x71143209;
		DSI(_DSIREG(DSI_WR_DATA)) = 0x114D31; // Set Power control. (Unknown).
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		break;
	case PANEL_INL_P062CCA_AZ1:
	default:
		break;
	}

	_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_ENTER_SLEEP_MODE, 50000);

	// Disable display and backlight pins.
	gpio_write(GPIO_PORT_V, GPIO_PIN_2, GPIO_LOW); //Backlight Reset disable.
	usleep(10000);

	gpio_write(GPIO_PORT_I, GPIO_PIN_1, GPIO_LOW); //Backlight -5V disable.
	usleep(10000);

	gpio_write(GPIO_PORT_I, GPIO_PIN_0, GPIO_LOW); //Backlight +5V disable.
	usleep(10000);

	// Disable Display Interface specific clocks.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_SET) = 0x1010000;  // Set reset clock DSI, MIPI_CAL.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_CLR) = 0x1010000;  // Clear enable clock DSI, MIPI_CAL.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = 0x18000000; // Set reset DISP1, HOST1X.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = 0x18000000; // Clear enable DISP1, HOST1X.

	// Power down pads.
	DSI(_DSIREG(DSI_PAD_CONTROL_0)) = DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) | DSI_PAD_CONTROL_VS1_PDIO_CLK | DSI_PAD_CONTROL_VS1_PDIO(0xF);
	DSI(_DSIREG(DSI_POWER_CONTROL)) = 0;

	// Switch to automatic function mode.
	gpio_config(GPIO_PORT_V, GPIO_PIN_0, GPIO_MODE_SPIO); // Backlight PWM.

	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) & ~PINMUX_TRISTATE) | PINMUX_TRISTATE;
	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) & 0xFFFFFFFC)| 1;
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

u32 *display_init_framebuffer_pitch()
{
	// Sanitize framebuffer area.
	memset((u32 *)IPL_FB_ADDRESS, 0, 0x3C0000);

	// This configures the framebuffer @ IPL_FB_ADDRESS with a resolution of 1280x720 (line stride 720).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer_pitch, 32);
	usleep(35000);

	return (u32 *)IPL_FB_ADDRESS;
}

u32 *display_init_framebuffer_pitch_inv()
{
	// This configures the framebuffer @ NYX_FB_ADDRESS with a resolution of 1280x720 (line stride 720).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer_pitch_inv, 34);

	usleep(35000);

	return (u32 *)NYX_FB_ADDRESS;
}

u32 *display_init_framebuffer_block()
{
	// This configures the framebuffer @ NYX_FB_ADDRESS with a resolution of 1280x720 (line stride 720).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer_block, 34);

	usleep(35000);

	return (u32 *)NYX_FB_ADDRESS;
}

u32 *display_init_framebuffer_log()
{
	// This configures the framebuffer @ LOG_FB_ADDRESS with a resolution of 1280x720 (line stride 720).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer_log, 20);

	return (u32 *)LOG_FB_ADDRESS;
}

void display_activate_console()
{
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = WINDOW_D_SELECT; // Select window C.
	DISPLAY_A(_DIREG(DC_WIN_WIN_OPTIONS)) = WIN_ENABLE; // Enable window DD.
	DISPLAY_A(_DIREG(DC_WIN_POSITION)) = 0xFF80;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE | WIN_D_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;

	for (u32 i = 0xFF80; i < 0x10000; i++)
	{
		DISPLAY_A(_DIREG(DC_WIN_POSITION)) = i & 0xFFFF;
		DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE | WIN_D_UPDATE;
		DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;
		usleep(1000);
	}

	DISPLAY_A(_DIREG(DC_WIN_POSITION)) = 0;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE | WIN_D_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;
}

void display_deactivate_console()
{
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = WINDOW_D_SELECT; // Select window C.

	for (u32 i = 0xFFFF; i > 0xFF7F; i--)
	{
		DISPLAY_A(_DIREG(DC_WIN_POSITION)) = i & 0xFFFF;
		DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE | WIN_D_UPDATE;
		DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;
		usleep(500);
	}

	DISPLAY_A(_DIREG(DC_WIN_POSITION)) = 0;
	DISPLAY_A(_DIREG(DC_WIN_WIN_OPTIONS)) = 0; // Disable window DD.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE | WIN_D_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;
}

void display_init_cursor(void *crs_fb, u32 size)
{
	// Setup cursor.
	DISPLAY_A(_DIREG(DC_DISP_CURSOR_START_ADDR)) = CURSOR_CLIPPING(CURSOR_CLIP_WIN_A) | size | ((u32)crs_fb >> 10);
	DISPLAY_A(_DIREG(DC_DISP_BLEND_CURSOR_CONTROL)) =
		CURSOR_BLEND_R8G8B8A8 | CURSOR_BLEND_DST_FACTOR(CURSOR_BLEND_K1) | CURSOR_BLEND_SRC_FACTOR(CURSOR_BLEND_K1) | 0xFF;

	DISPLAY_A(_DIREG(DC_DISP_DISP_WIN_OPTIONS)) |= CURSOR_ENABLE;

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE | CURSOR_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | CURSOR_ACT_REQ;
}

void display_set_pos_cursor(u32 x, u32 y)
{
	DISPLAY_A(_DIREG(DC_DISP_CURSOR_POSITION)) = x | (y << 16);

	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE | CURSOR_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | CURSOR_ACT_REQ;
}

void display_deinit_cursor()
{
	DISPLAY_A(_DIREG(DC_DISP_BLEND_CURSOR_CONTROL)) = 0;
	DISPLAY_A(_DIREG(DC_DISP_DISP_WIN_OPTIONS)) &= ~CURSOR_ENABLE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE | CURSOR_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | CURSOR_ACT_REQ;
}
