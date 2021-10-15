/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2021 CTCaer
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
#include <mem/heap.h>
#include <soc/clock.h>
#include <soc/fuse.h>
#include <soc/gpio.h>
#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/pinmux.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <utils/util.h>

#include "di.inl"

extern volatile nyx_storage_t *nyx_str;

static u32 _display_id = 0;
static bool nx_aula = false;

static void _display_panel_and_hw_end(bool no_panel_deinit);

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

static void _display_dsi_read_rx_fifo(u32 *data)
{
	u32 fifo_count = DSI(_DSIREG(DSI_STATUS)) & DSI_STATUS_RX_FIFO_SIZE;
	for (u32 i = 0; i < fifo_count; i++)
	{
		// Read or Drain RX FIFO.
		if (data)
			data[i] = DSI(_DSIREG(DSI_RD_DATA));
		else
			(void)DSI(_DSIREG(DSI_RD_DATA));
	}
}

int display_dsi_read(u8 cmd, u32 len, void *data, bool video_enabled)
{
	int res = 0;
	u32 host_control = 0;
	u32 cmd_timeout = video_enabled ? 0 : 250000;
	u32 fifo[DSI_STATUS_RX_FIFO_SIZE] = {0};

	// Drain RX FIFO.
	_display_dsi_read_rx_fifo(NULL);

	// Save host control and enable host cmd packets during video.
	if (video_enabled)
	{
		host_control = DSI(_DSIREG(DSI_HOST_CONTROL));

		// Enable vblank interrupt.
		DISPLAY_A(_DIREG(DC_CMD_INT_ENABLE)) = DC_CMD_INT_FRAME_END_INT;

		// Use the 4th line to transmit the host cmd packet.
		DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = DSI_CMD_PKT_VID_ENABLE | DSI_DSI_LINE_TYPE(4);

		// Wait for vblank before starting the transfer.
		DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) = DC_CMD_INT_FRAME_END_INT; // Clear interrupt.
		while (!(DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) & DC_CMD_INT_FRAME_END_INT))
			;
	}

	// Set reply size.
	_display_dsi_send_cmd(MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE, len, 0);
	_display_dsi_wait(cmd_timeout, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	// Request register read.
	_display_dsi_send_cmd(MIPI_DSI_DCS_READ, cmd, 0);
	_display_dsi_wait(cmd_timeout, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	// Transfer bus control to device for transmitting the reply.
	u32 high_speed = video_enabled ? DSI_HOST_CONTROL_HS : 0;
	DSI(_DSIREG(DSI_HOST_CONTROL)) = DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_IMM_BTA | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC | high_speed;
	_display_dsi_wait(150000, _DSIREG(DSI_HOST_CONTROL), DSI_HOST_CONTROL_IMM_BTA);

	// Wait a bit for the reply.
	usleep(5000);

	// Read RX FIFO.
	_display_dsi_read_rx_fifo(fifo);

	// Parse packet and copy over the data.
	if ((fifo[0] & 0xFF) == DSI_ESCAPE_CMD)
	{
		// Act based on reply type.
		switch (fifo[1] & 0xFF)
		{
		case GEN_LONG_RD_RES:
		case DCS_LONG_RD_RES:
			memcpy(data, &fifo[2], MIN((fifo[1] >> 8) & 0xFFFF, len));
			break;

		case GEN_1_BYTE_SHORT_RD_RES:
		case DCS_1_BYTE_SHORT_RD_RES:
			memcpy(data, &fifo[2], 1);
			break;

		case GEN_2_BYTE_SHORT_RD_RES:
		case DCS_2_BYTE_SHORT_RD_RES:
			memcpy(data, &fifo[2], 2);
			break;

		case ACK_ERROR_RES:
		default:
			res = 1;
			break;
		}
	}
	else
		res = 1;

	// Disable host cmd packets during video and restore host control.
	if (video_enabled)
	{
		// Wait for vblank before reseting sync points.
		DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) = DC_CMD_INT_FRAME_END_INT; // Clear interrupt.
		while (!(DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) & DC_CMD_INT_FRAME_END_INT))
			;

		// Reset all states of syncpt block.
		DSI(_DSIREG(DSI_INCR_SYNCPT_CNTRL)) = DSI_INCR_SYNCPT_SOFT_RESET;
		usleep(300); // Stabilization delay.

		// Clear syncpt block reset.
		DSI(_DSIREG(DSI_INCR_SYNCPT_CNTRL)) = 0;
		usleep(300); // Stabilization delay.

		// Restore video mode and host control.
		DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 0;
		DSI(_DSIREG(DSI_HOST_CONTROL)) = host_control;

		// Disable and clear vblank interrupt.
		DISPLAY_A(_DIREG(DC_CMD_INT_ENABLE)) = 0;
		DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) = DC_CMD_INT_FRAME_END_INT;
	}

	return res;
}

void display_dsi_write(u8 cmd, u32 len, void *data, bool video_enabled)
{
	u8  *fifo8;
	u32 *fifo32;
	u32 host_control;

	// Enable host cmd packets during video and save host control.
	if (video_enabled)
		DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = DSI_CMD_PKT_VID_ENABLE;
	host_control = DSI(_DSIREG(DSI_HOST_CONTROL));

	// Enable host transfer trigger.
	DSI(_DSIREG(DSI_HOST_CONTROL)) = host_control | DSI_HOST_CONTROL_TX_TRIG_HOST;

	switch (len)
	{
	case 0:
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, cmd, 0);
		break;

	case 1:
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE_PARAM, cmd | (*(u8 *)data << 8), 0);
		break;

	default:
		fifo32 = calloc(DSI_STATUS_RX_FIFO_SIZE * 8, 4);
		fifo8 = (u8 *)fifo32;
		fifo32[0] = (len << 8) | MIPI_DSI_DCS_LONG_WRITE;
		fifo8[4] = cmd;
		memcpy(&fifo8[5], data, len);
		len += 4 + 1; // Increase length by CMD/length word and DCS CMD.
		for (u32 i = 0; i < (ALIGN(len, 4) / 4); i++)
			DSI(_DSIREG(DSI_WR_DATA)) = fifo32[i];
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		free(fifo32);
		break;
	}

	// Wait for the write to happen.
	_display_dsi_wait(250000, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST);

	// Disable host cmd packets during video and restore host control.
	if (video_enabled)
		DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 0;
	DSI(_DSIREG(DSI_HOST_CONTROL)) = host_control;
}

void display_dsi_vblank_write(u8 cmd, u32 len, void *data)
{
	u8  *fifo8;
	u32 *fifo32;

	// Enable vblank interrupt.
	DISPLAY_A(_DIREG(DC_CMD_INT_ENABLE)) = DC_CMD_INT_FRAME_END_INT;

	// Use the 4th line to transmit the host cmd packet.
	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = DSI_CMD_PKT_VID_ENABLE | DSI_DSI_LINE_TYPE(4);

	// Wait for vblank before starting the transfer.
	DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) = DC_CMD_INT_FRAME_END_INT; // Clear interrupt.
	while (!(DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) & DC_CMD_INT_FRAME_END_INT))
		;

	switch (len)
	{
	case 0:
		DSI(_DSIREG(DSI_WR_DATA)) = (cmd << 8) | MIPI_DSI_DCS_SHORT_WRITE;
		break;

	case 1:
		DSI(_DSIREG(DSI_WR_DATA)) = ((cmd | (*(u8 *)data << 8)) << 8) | MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		break;

	default:
		fifo32 = calloc(DSI_STATUS_RX_FIFO_SIZE * 8, 4);
		fifo8 = (u8 *)fifo32;
		fifo32[0] = (len << 8) | MIPI_DSI_DCS_LONG_WRITE;
		fifo8[4] = cmd;
		memcpy(&fifo8[5], data, len);
		len += 4 + 1; // Increase length by CMD/length word and DCS CMD.
		for (u32 i = 0; i < (ALIGN(len, 4) / 4); i++)
			DSI(_DSIREG(DSI_WR_DATA)) = fifo32[i];
		free(fifo32);
		break;
	}

	// Wait for vblank before reseting sync points.
	DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) = DC_CMD_INT_FRAME_END_INT; // Clear interrupt.
	while (!(DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) & DC_CMD_INT_FRAME_END_INT))
		;

	// Reset all states of syncpt block.
	DSI(_DSIREG(DSI_INCR_SYNCPT_CNTRL)) = DSI_INCR_SYNCPT_SOFT_RESET;
	usleep(300); // Stabilization delay.

	// Clear syncpt block reset.
	DSI(_DSIREG(DSI_INCR_SYNCPT_CNTRL)) = 0;
	usleep(300); // Stabilization delay.

	// Restore video mode and host control.
	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 0;

	// Disable and clear vblank interrupt.
	DISPLAY_A(_DIREG(DC_CMD_INT_ENABLE)) = 0;
	DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) = DC_CMD_INT_FRAME_END_INT;
}

void display_init()
{
	// Get Hardware type, as it's used in various DI functions.
	nx_aula = fuse_read_hw_type() == FUSE_NX_HW_TYPE_AULA;

	// Check if display is already initialized.
	if (CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & BIT(CLK_L_DISP1))
		_display_panel_and_hw_end(true);

	// Get Chip ID.
	bool tegra_t210 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;

	// T210B01: Power on SD2 regulator for supplying LDO0.
	if (!tegra_t210)
	{
		// Set SD2 regulator voltage.
		max7762x_regulator_set_voltage(REGULATOR_SD2, 1325000);

		// Set slew rate and enable SD2 regulator.
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_SD2_CFG, (1 << MAX77620_SD_SR_SHIFT) | MAX77620_SD_CFG1_FSRADE_SD_ENABLE);
		max7762x_regulator_enable(REGULATOR_SD2, true);

	}

	// Enable power to display panel controller.
	max7762x_regulator_set_voltage(REGULATOR_LDO0, 1200000);
	max7762x_regulator_enable(REGULATOR_LDO0, true);

	if (tegra_t210)
		max77620_config_gpio(7, MAX77620_GPIO_OUTPUT_ENABLE); // T210: LD0 -> GPIO7 -> Display panel.

	// Enable Display Interface specific clocks.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_CLR) = BIT(CLK_H_MIPI_CAL) | BIT(CLK_H_DSI);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = BIT(CLK_H_MIPI_CAL) | BIT(CLK_H_DSI);

	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = BIT(CLK_L_HOST1X) | BIT(CLK_L_DISP1);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = BIT(CLK_L_HOST1X) | BIT(CLK_L_DISP1);

	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) = BIT(CLK_X_UART_FST_MIPI_CAL);
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL) = 10; // Set PLLP_OUT3 and div 6 (17MHz).

	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_SET) = BIT(CLK_W_DSIA_LP);
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP) = 10;           // Set PLLP_OUT and div 6 (68MHz).

	// Bring every IO rail out of deep power down.
	PMC(APBDEV_PMC_IO_DPD_REQ)  = PMC_IO_DPD_REQ_DPD_OFF;
	PMC(APBDEV_PMC_IO_DPD2_REQ) = PMC_IO_DPD_REQ_DPD_OFF;

	// Configure LCD pins.
	PINMUX_AUX(PINMUX_AUX_NFC_EN)     &= ~PINMUX_TRISTATE; // PULL_DOWN
	PINMUX_AUX(PINMUX_AUX_NFC_INT)    &= ~PINMUX_TRISTATE; // PULL_DOWN
	PINMUX_AUX(PINMUX_AUX_LCD_RST)    &= ~PINMUX_TRISTATE; // PULL_DOWN

	// Configure Backlight pins.
	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) &= ~PINMUX_TRISTATE; // PULL_DOWN | 1
	PINMUX_AUX(PINMUX_AUX_LCD_BL_EN)  &= ~PINMUX_TRISTATE; // PULL_DOWN

	if (nx_aula)
	{
		// Configure LCD RST pin.
		gpio_config(GPIO_PORT_V, GPIO_PIN_2, GPIO_MODE_GPIO);
		gpio_output_enable(GPIO_PORT_V, GPIO_PIN_2, GPIO_OUTPUT_ENABLE);
	}
	else
	{
		// Set LCD +-5V pins mode and direction
		gpio_config(GPIO_PORT_I, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_GPIO);
		gpio_output_enable(GPIO_PORT_I, GPIO_PIN_0 | GPIO_PIN_1, GPIO_OUTPUT_ENABLE);

		// Enable LCD power.
		gpio_write(GPIO_PORT_I, GPIO_PIN_0, GPIO_HIGH); // LCD +5V enable.
		usleep(10000);
		gpio_write(GPIO_PORT_I, GPIO_PIN_1, GPIO_HIGH); // LCD -5V enable.
		usleep(10000);

		// Configure Backlight PWM/EN and LCD RST pins (BL PWM, BL EN, LCD RST).
		gpio_config(GPIO_PORT_V, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_GPIO);
		gpio_output_enable(GPIO_PORT_V, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_OUTPUT_ENABLE);

		 // Enable Backlight power.
		gpio_write(GPIO_PORT_V, GPIO_PIN_1, GPIO_HIGH);
	}

	// Power up supply regulator for display interface.
	MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG2)) = 0;

	if (!tegra_t210)
	{
		MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG0)) = 0;
		APB_MISC(APB_MISC_GP_DSI_PAD_CONTROL) = 0;
	}

	// Set DISP1 clock source and parent clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_DISP1) = 0x40000000; // PLLD_OUT.
	u32 plld_div = (3 << 20) | (20 << 11) | 1; // DIVM: 1, DIVN: 20, DIVP: 3. PLLD_OUT: 768 MHz, PLLD_OUT0 (DSI): 97.5 MHz (offset).
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) = PLLCX_BASE_ENABLE | PLLCX_BASE_LOCK | plld_div;

	if (tegra_t210)
	{
		CLOCK(CLK_RST_CONTROLLER_PLLD_MISC1) = 0x20;     // PLLD_SETUP.
		CLOCK(CLK_RST_CONTROLLER_PLLD_MISC)  = 0x2D0AAA; // PLLD_ENABLE_CLK.
	}
	else
	{
		CLOCK(CLK_RST_CONTROLLER_PLLD_MISC1) = 0;
		CLOCK(CLK_RST_CONTROLLER_PLLD_MISC)  = 0x2DFC00; // PLLD_ENABLE_CLK.
	}

	// Setup Display Interface initial window configuration.
	exec_cfg((u32 *)DISPLAY_A_BASE, _display_dc_setup_win_config, 94);

	// Setup display communication interfaces.
	exec_cfg((u32 *)DSI_BASE, _display_dsi_init_config_part1, 8);
	if (tegra_t210)
		DSI(_DSIREG(DSI_INIT_SEQ_DATA_15)) = 0;
	else
		DSI(_DSIREG(DSI_INIT_SEQ_DATA_15_B01)) = 0;
	exec_cfg((u32 *)DSI_BASE, _display_dsi_init_config_part2, 14);
	if (!tegra_t210)
		exec_cfg((u32 *)DSI_BASE, _display_dsi_init_config_part3_t210b01, 7);
	exec_cfg((u32 *)DSI_BASE, _display_dsi_init_config_part4, 10);
	DSI(_DSIREG(DSI_PHY_TIMING_0)) = tegra_t210 ? 0x6070601 : 0x6070603;
	exec_cfg((u32 *)DSI_BASE, _display_dsi_init_config_part5, 12);
	DSI(_DSIREG(DSI_PHY_TIMING_0)) = tegra_t210 ? 0x6070601 : 0x6070603;
	exec_cfg((u32 *)DSI_BASE, _display_dsi_init_config_part6, 14);
	usleep(10000);

	// Enable LCD Reset.
	gpio_write(GPIO_PORT_V, GPIO_PIN_2, GPIO_HIGH);
	usleep(60000);

	// Setup DSI device takeover timeout.
	DSI(_DSIREG(DSI_BTA_TIMING)) = nx_aula ? 0x40103 : 0x50204;

	// Get Display ID.
	_display_id = 0xCCCCCC;
	for (u32 i = 0; i < 3; i++)
	{
		if (!display_dsi_read(MIPI_DCS_GET_DISPLAY_ID, 3, &_display_id, DSI_VIDEO_DISABLED))
			break;

		usleep(10000);
	}

	// Save raw Display ID to Nyx storage.
	nyx_str->info.disp_id = _display_id;

	// Decode Display ID.
	_display_id = ((_display_id >> 8) & 0xFF00) | (_display_id & 0xFF);

	if ((_display_id & 0xFF) == PANEL_JDI_XXX062M)
		_display_id = PANEL_JDI_XXX062M;

	// For Aula ensure that we have a compatible panel id.
	if (nx_aula && _display_id == 0xCCCC)
		_display_id = PANEL_SAM_AMS699VC01;

	// Initialize display panel.
	switch (_display_id)
	{
	case PANEL_SAM_AMS699VC01:
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 180000);
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xA0, 0); // Write 0 to 0xA0.
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE_PARAM, MIPI_DCS_SET_CONTROL_DISPLAY | (DCS_CONTROL_DISPLAY_BRIGHTNESS_CTRL << 8), 0); // Enable brightness control.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x339;    // MIPI_DSI_DCS_LONG_WRITE: 3 bytes.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x000051; // MIPI_DCS_SET_BRIGHTNESS 0000: 0%. FF07: 100%.
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		break;

	case PANEL_JDI_XXX062M:
		exec_cfg((u32 *)DSI_BASE, _display_init_config_jdi, 43);
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 180000);
		break;

	case PANEL_INL_P062CCA_AZ1:
	case PANEL_AUO_A062TAN01:
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 180000);

		// Unlock extension cmds.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x439;          // MIPI_DSI_DCS_LONG_WRITE: 4 bytes.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x9483FFB9;     // MIPI_DCS_PRIV_SET_EXTC. (Pass: FF 83 94).
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);

		// Set Power control.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x739;          // MIPI_DSI_DCS_LONG_WRITE: 7 bytes.
		if (_display_id == PANEL_INL_P062CCA_AZ1)
			DSI(_DSIREG(DSI_WR_DATA)) = 0x751548B1; // MIPI_DCS_PRIV_SET_POWER_CONTROL. (Not deep standby, BT5 / XDK, VRH gamma volt adj 53 / x40).
		else // PANEL_AUO_A062TAN01.
			DSI(_DSIREG(DSI_WR_DATA)) = 0x711148B1; // MIPI_DCS_PRIV_SET_POWER_CONTROL. (Not deep standby, BT1 / XDK, VRH gamma volt adj 49 / x40).
		DSI(_DSIREG(DSI_WR_DATA)) = 0x143209;       // (NVRH gamma volt adj 9, Amplifier current small / x30, FS0 freq Fosc/80 / FS1 freq Fosc/32).
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		break;

	case PANEL_INL_2J055IA_27A:
	case PANEL_AUO_A055TAN01:
	case PANEL_V40_55_UNK:
	default: // Allow spare part displays to work.
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 120000);
		break;
	}

	// Unblank display.
	_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_SET_DISPLAY_ON, 20000);

	// Configure PLLD for DISP1.
	plld_div = (1 << 20) | (24 << 11) | 1; // DIVM: 1, DIVN: 24, DIVP: 1. PLLD_OUT: 768 MHz, PLLD_OUT0 (DSI): 234 MHz (offset, it's ddr btw, so normally div2).
	CLOCK(CLK_RST_CONTROLLER_PLLD_BASE) = PLLCX_BASE_ENABLE | PLLCX_BASE_LOCK | plld_div;

	if (tegra_t210)
		CLOCK(CLK_RST_CONTROLLER_PLLD_MISC1) = 0x20; // PLLD_SETUP.
	else
		CLOCK(CLK_RST_CONTROLLER_PLLD_MISC1) = 0;
	CLOCK(CLK_RST_CONTROLLER_PLLD_MISC) = 0x2DFC00;  // Use new PLLD_SDM_DIN.

	// Finalize DSI configuration.
	DSI(_DSIREG(DSI_PAD_CONTROL_1)) = 0;
	DSI(_DSIREG(DSI_PHY_TIMING_0)) = tegra_t210 ? 0x6070601 : 0x6070603;
	exec_cfg((u32 *)DSI_BASE, _display_dsi_packet_config, 19);
	// Set pixel clock dividers: 234 / 3 / 1 = 78 MHz (offset) for 60 Hz.
	DISPLAY_A(_DIREG(DC_DISP_DISP_CLOCK_CONTROL)) = PIXEL_CLK_DIVIDER_PCD1 | SHIFT_CLK_DIVIDER(4); // 4: div3.
	exec_cfg((u32 *)DSI_BASE, _display_dsi_mode_config, 10);
	usleep(10000);

	// Calibrate display communication pads.
	u32 loops = tegra_t210 ? 1 : 2; // Find out why this is done 2 times on Mariko.
	exec_cfg((u32 *)MIPI_CAL_BASE, _display_mipi_pad_cal_config, 4);
	for (u32 i = 0; i < loops; i++)
	{
		// Set MIPI bias pad config.
		MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG2)) = 0x10010;
		MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG1)) = tegra_t210 ? 0x300 : 0;

		// Set pad trimmers and set MIPI DSI cal offsets.
		if (tegra_t210)
		{
			exec_cfg((u32 *)DSI_BASE, _display_dsi_pad_cal_config_t210, 4);
			exec_cfg((u32 *)MIPI_CAL_BASE, _display_mipi_dsi_cal_offsets_config_t210, 4);
		}
		else
		{
			exec_cfg((u32 *)DSI_BASE, _display_dsi_pad_cal_config_t210b01, 7);
			exec_cfg((u32 *)MIPI_CAL_BASE, _display_mipi_dsi_cal_offsets_config_t210b01, 4);
		}

		// Set the rest of MIPI cal offsets and apply calibration.
		exec_cfg((u32 *)MIPI_CAL_BASE, _display_mipi_apply_dsi_cal_config, 12);
	}
	usleep(10000);

	// Enable video display controller.
	exec_cfg((u32 *)DISPLAY_A_BASE, _display_video_disp_controller_enable_config, 113);
}

void display_backlight_pwm_init()
{
	if (_display_id == PANEL_SAM_AMS699VC01)
		return;

	clock_enable_pwm();

	PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN; // Enable PWM and set it to 25KHz PFM. 29.5KHz is stock.

	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) & ~PINMUX_FUNC_MASK) | 1; // Set PWM0 mode.
	gpio_config(GPIO_PORT_V, GPIO_PIN_0, GPIO_MODE_SPIO); // Backlight power mode.
}

void display_backlight(bool enable)
{
	gpio_write(GPIO_PORT_V, GPIO_PIN_0, enable ? GPIO_HIGH : GPIO_LOW); // Backlight PWM GPIO.
}

void display_dsi_backlight_brightness(u32 brightness)
{
	// Normalize brightness value by 82% and a base of 45 duty.
	if (brightness)
		brightness = (brightness * PANEL_OLED_BL_COEFF / 100) + PANEL_OLED_BL_OFFSET;

	u16 bl_ctrl = byte_swap_16((u16)(brightness * 8));
	display_dsi_vblank_write(MIPI_DCS_SET_BRIGHTNESS, 2, &bl_ctrl);
}

void display_pwm_backlight_brightness(u32 brightness, u32 step_delay)
{
	u32 old_value = (PWM(PWM_CONTROLLER_PWM_CSR_0) >> 16) & 0xFF;
	if (brightness == old_value)
		return;

	if (old_value < brightness)
	{
		for (u32 i = old_value; i < brightness + 1; i++)
		{
			PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN | (i << 16);
			usleep(step_delay);
		}
	}
	else
	{
		for (u32 i = old_value; i > brightness; i--)
		{
			PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN | (i << 16);
			usleep(step_delay);
		}
	}
	if (!brightness)
		PWM(PWM_CONTROLLER_PWM_CSR_0) = 0;
}

void display_backlight_brightness(u32 brightness, u32 step_delay)
{
	if (brightness > 255)
		brightness = 255;

	if (_display_id != PANEL_SAM_AMS699VC01)
		display_pwm_backlight_brightness(brightness, step_delay);
	else
		display_dsi_backlight_brightness(brightness);
}

u32 display_get_backlight_brightness()
{
	return ((PWM(PWM_CONTROLLER_PWM_CSR_0) >> 16) & 0xFF);
}

static void _display_panel_and_hw_end(bool no_panel_deinit)
{
	if (no_panel_deinit)
		goto skip_panel_deinit;

	display_backlight_brightness(0, 1000);

	// Enable host cmd packets during video.
	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = DSI_CMD_PKT_VID_ENABLE;

	// Blank display.
	DSI(_DSIREG(DSI_WR_DATA)) = (MIPI_DCS_SET_DISPLAY_OFF << 8) | MIPI_DSI_DCS_SHORT_WRITE;

	// Propagate changes to all register buffers and disable host cmd packets during video.
	DISPLAY_A(_DIREG(DC_CMD_STATE_ACCESS)) = READ_MUX | WRITE_MUX;
	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 0;

	// De-initialize video controller.
	exec_cfg((u32 *)DISPLAY_A_BASE, _display_video_disp_controller_disable_config, 17);
	exec_cfg((u32 *)DSI_BASE, _display_dsi_timing_deinit_config, 16);

	if (_display_id != PANEL_SAM_AMS699VC01)
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

	case PANEL_INL_2J055IA_27A:
	case PANEL_AUO_A055TAN01:
	case PANEL_V40_55_UNK:
		// Unlock extension cmds.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x439;          // MIPI_DSI_DCS_LONG_WRITE: 4 bytes.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x9483FFB9;     // MIPI_DCS_PRIV_SET_EXTC. (Pass: FF 83 94).
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);

		// Set Power control.
		DSI(_DSIREG(DSI_WR_DATA)) = 0xB39;          // MIPI_DSI_DCS_LONG_WRITE: 11 bytes.
		if (_display_id == PANEL_INL_2J055IA_27A)
			DSI(_DSIREG(DSI_WR_DATA)) = 0x751548B1; // MIPI_DCS_PRIV_SET_POWER_CONTROL. (Not deep standby, BT5 / XDK, VRH gamma volt adj 53 / x40).
		else if (_display_id == PANEL_AUO_A055TAN01)
			DSI(_DSIREG(DSI_WR_DATA)) = 0x711148B1; // MIPI_DCS_PRIV_SET_POWER_CONTROL. (Not deep standby, BT1 / XDK, VRH gamma volt adj 49 / x40).
		else // PANEL_V40_55_UNK.
			DSI(_DSIREG(DSI_WR_DATA)) = 0x731348B1; // MIPI_DCS_PRIV_SET_POWER_CONTROL. (Not deep standby, BT3 / XDK, VRH gamma volt adj 51 / x40).
		if (_display_id == PANEL_INL_2J055IA_27A || _display_id == PANEL_AUO_A055TAN01)
		{
			// (NVRH gamma volt adj 9, Amplifier current small / x30, FS0 freq Fosc/80 / FS1 freq Fosc/32, Enter standby / PON / VCOMG).
			DSI(_DSIREG(DSI_WR_DATA)) = 0x71143209;
			DSI(_DSIREG(DSI_WR_DATA)) = 0x114D31;   // (Unknown).
		}
		else // PANEL_V40_55_UNK.
		{
			// (NVRH gamma volt adj 9, Amplifier current small / x30, FS0 freq Fosc/80 / FS1 freq Fosc/48, Enter standby / PON / VCOMG).
			DSI(_DSIREG(DSI_WR_DATA)) = 0x71243209;
			DSI(_DSIREG(DSI_WR_DATA)) = 0x004C31;   // (Unknown).
		}
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		break;

	case PANEL_INL_P062CCA_AZ1:
	default:
		break;
	}

	// Blank - powerdown.
	_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_ENTER_SLEEP_MODE,
		(_display_id == PANEL_SAM_AMS699VC01) ? 120000 : 50000);

skip_panel_deinit:
	// Disable LCD power pins.
	gpio_write(GPIO_PORT_V, GPIO_PIN_2, GPIO_LOW);     // LCD Reset disable.

	if (!nx_aula) // HOS uses panel id.
	{
		usleep(10000);
		gpio_write(GPIO_PORT_I, GPIO_PIN_1, GPIO_LOW); // LCD -5V disable.
		usleep(10000);
		gpio_write(GPIO_PORT_I, GPIO_PIN_0, GPIO_LOW); // LCD +5V disable.
		usleep(10000);
	}
	else
		usleep(30000); // Aula Panel.

	// Disable Display Interface specific clocks.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_SET) = BIT(CLK_H_MIPI_CAL) | BIT(CLK_H_DSI);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_CLR) = BIT(CLK_H_MIPI_CAL) | BIT(CLK_H_DSI);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_HOST1X) | BIT(CLK_L_DISP1);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = BIT(CLK_L_HOST1X) | BIT(CLK_L_DISP1);

	// Power down pads.
	DSI(_DSIREG(DSI_PAD_CONTROL_0)) = DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) | DSI_PAD_CONTROL_VS1_PDIO_CLK | DSI_PAD_CONTROL_VS1_PDIO(0xF);
	DSI(_DSIREG(DSI_POWER_CONTROL)) = 0;

	// Switch LCD PWM backlight pin to special function mode and enable PWM0 mode.
	if (!nx_aula)
	{
		gpio_config(GPIO_PORT_V, GPIO_PIN_0, GPIO_MODE_SPIO); // Backlight PWM.
		PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) & ~PINMUX_TRISTATE) | PINMUX_TRISTATE;
		PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) & ~PINMUX_FUNC_MASK) | 1; // Set PWM0 mode.
	}
}

void display_end() { _display_panel_and_hw_end(false); };

u16 display_get_decoded_panel_id()
{
	return _display_id;
}

void display_set_decoded_panel_id(u32 id)
{
	// Get Hardware type, as it's used in various DI functions.
	nx_aula = fuse_read_hw_type() == FUSE_NX_HW_TYPE_AULA;

	// Decode Display ID.
	_display_id = ((id >> 8) & 0xFF00) | (id & 0xFF);

	if ((_display_id & 0xFF) == PANEL_JDI_XXX062M)
		_display_id = PANEL_JDI_XXX062M;

	// For Aula ensure that we have a compatible panel id.
	if (nx_aula && _display_id == 0xCCCC)
		_display_id = PANEL_SAM_AMS699VC01;
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
	usleep(35000); // No need to wait on Aula.

	if (_display_id != PANEL_SAM_AMS699VC01)
		display_backlight(true);
	else
		display_backlight_brightness(255, 0);
}

u32 *display_init_framebuffer_pitch()
{
	// Sanitize framebuffer area.
	memset((u32 *)IPL_FB_ADDRESS, 0, 0x3C0000);

	// This configures the framebuffer @ IPL_FB_ADDRESS with a resolution of 1280x720 (line stride 720).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer_pitch, 32);
	usleep(35000); // No need to wait on Aula.

	return (u32 *)IPL_FB_ADDRESS;
}

u32 *display_init_framebuffer_pitch_inv()
{
	// This configures the framebuffer @ NYX_FB_ADDRESS with a resolution of 1280x720 (line stride 720).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer_pitch_inv, 34);
	usleep(35000); // No need to wait on Aula.

	return (u32 *)NYX_FB_ADDRESS;
}

u32 *display_init_framebuffer_block()
{
	// This configures the framebuffer @ NYX_FB_ADDRESS with a resolution of 1280x720 (line stride 720).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer_block, 34);
	usleep(35000); // No need to wait on Aula.

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
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = WINDOW_D_SELECT; // Select window D.
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
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = WINDOW_D_SELECT; // Select window D.

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
