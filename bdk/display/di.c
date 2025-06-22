/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2025 CTCaer
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
#include <soc/fuse.h>
#include <soc/gpio.h>
#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/pinmux.h>
#include <soc/pmc.h>
#include <soc/timer.h>
#include <soc/t210.h>
#include <utils/util.h>

#include "di.inl"

extern volatile nyx_storage_t *nyx_str;

static u32  _display_id = 0;
static u32  _dsi_bl     = -1;
static bool _nx_aula    = false;

static void _display_panel_and_hw_end(bool no_panel_deinit);

void display_enable_interrupt(u32 intr)
{
	DISPLAY_A(_DIREG(DC_CMD_INT_ENABLE)) |= intr;
}

void display_disable_interrupt(u32 intr)
{
	DISPLAY_A(_DIREG(DC_CMD_INT_ENABLE)) &= ~intr;
	DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) = intr;
}

void display_wait_interrupt(u32 intr)
{
	DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) = intr;

	// Interrupts are masked. Poll status register for checking if fired.
	while (!(DISPLAY_A(_DIREG(DC_CMD_INT_STATUS)) & intr))
		;
}

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

static void _display_dsi_wait_vblank(bool enable)
{
	if (enable)
	{
		// Enable vblank interrupt.
		display_enable_interrupt(DC_CMD_INT_FRAME_END_INT);

		// Use the 4th line to transmit the host cmd packet.
		DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = DSI_CMD_PKT_VID_ENABLE | DSI_DSI_LINE_TYPE(4);

		// Wait for vblank before starting the transfer.
		display_wait_interrupt(DC_CMD_INT_FRAME_END_INT);
	}
	else
	{
		// Wait for vblank before resetting sync points.
		display_wait_interrupt(DC_CMD_INT_FRAME_END_INT);
		usleep(14);

		// Reset all states of syncpt block.
		DSI(_DSIREG(DSI_INCR_SYNCPT_CNTRL)) = DSI_INCR_SYNCPT_SOFT_RESET;
		usleep(300); // Stabilization delay.

		// Clear syncpt block reset.
		DSI(_DSIREG(DSI_INCR_SYNCPT_CNTRL)) = 0;
		usleep(300); // Stabilization delay.

		// Restore video mode and host control.
		DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 0;

		// Disable and clear vblank interrupt.
		display_disable_interrupt(DC_CMD_INT_FRAME_END_INT);
	}
}

static void _display_dsi_read_rx_fifo(u32 *data)
{
	u32 fifo_count = DSI(_DSIREG(DSI_STATUS)) & DSI_STATUS_RX_FIFO_SIZE;
	if (fifo_count)
		DSI(_DSIREG(DSI_TRIGGER)) = 0;

	for (u32 i = 0; i < fifo_count; i++)
	{
		// Read or Drain RX FIFO.
		if (data)
			data[i] = DSI(_DSIREG(DSI_RD_DATA));
		else
			(void)DSI(_DSIREG(DSI_RD_DATA));
	}
}

int display_dsi_read(u8 cmd, u32 len, void *data)
{
	u32 fifo[DSI_STATUS_RX_FIFO_SIZE] = {0};

	// Drain RX FIFO.
	_display_dsi_read_rx_fifo(NULL);

	// Set reply size.
	_display_dsi_send_cmd(MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE, len, 0);
	_display_dsi_wait(250000, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	// Request register read.
	_display_dsi_send_cmd(MIPI_DSI_DCS_READ, cmd, 0);
	_display_dsi_wait(250000, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	// Transfer bus control to device for transmitting the reply.
	DSI(_DSIREG(DSI_HOST_CONTROL)) |= DSI_HOST_CONTROL_IMM_BTA;

	// Wait for reply to complete. DSI_HOST_CONTROL_IMM_BTA bit acts as a DSI host read busy.
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
			return 1;
		}
	}
	else
		return 1;

	return 0;
}

int display_dsi_vblank_read(u8 cmd, u32 len, void *data)
{
	int res = 0;
	u32 host_control = 0;
	u32 fifo[DSI_STATUS_RX_FIFO_SIZE] = {0};

	// Drain RX FIFO.
	_display_dsi_read_rx_fifo(NULL);

	// Save host control and enable host cmd packets during video.
	host_control = DSI(_DSIREG(DSI_HOST_CONTROL));

	_display_dsi_wait_vblank(true);

	// Set reply size.
	_display_dsi_send_cmd(MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE, len, 0);
	_display_dsi_wait(0, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	// Request register read.
	_display_dsi_send_cmd(MIPI_DSI_DCS_READ, cmd, 0);
	_display_dsi_wait(0, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO);

	_display_dsi_wait_vblank(false);

	// Transfer bus control to device for transmitting the reply.
	DSI(_DSIREG(DSI_HOST_CONTROL)) |= DSI_HOST_CONTROL_IMM_BTA;

	// Wait for reply to complete. DSI_HOST_CONTROL_IMM_BTA bit acts as a DSI host read busy.
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

	// Restore host control.
	DSI(_DSIREG(DSI_HOST_CONTROL)) = host_control;

	return res;
}

void display_dsi_write(u8 cmd, u32 len, void *data)
{
	u32 host_control;
	u32 fifo32[DSI_STATUS_TX_FIFO_SIZE] = {0};
	u8 *fifo8 = (u8 *)fifo32;

	// Prepare data for long write.
	if (len >= 2)
	{
		memcpy(&fifo8[5], data, len);
		memset(&fifo8[5] + len, 0, len % sizeof(u32));
		len++; // Increase length by CMD.
	}

	// Save host control.
	host_control = DSI(_DSIREG(DSI_HOST_CONTROL));

	// Enable host transfer trigger.
	DSI(_DSIREG(DSI_HOST_CONTROL)) = (host_control & ~(DSI_HOST_CONTROL_TX_TRIG_MASK)) | DSI_HOST_CONTROL_TX_TRIG_HOST;

	switch (len)
	{
	case 0:
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, cmd, 0);
		break;

	case 1:
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE_PARAM, cmd | (*(u8 *)data << 8), 0);
		break;

	default:
		fifo32[0] = (len << 8) | MIPI_DSI_DCS_LONG_WRITE;
		fifo8[4] = cmd;
		len += sizeof(u32); // Increase length by length word and DCS CMD.
		for (u32 i = 0; i < (ALIGN(len, sizeof(u32)) / sizeof(u32)); i++)
			DSI(_DSIREG(DSI_WR_DATA)) = fifo32[i];
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		break;
	}

	// Wait for the write to happen.
	_display_dsi_wait(250000, _DSIREG(DSI_TRIGGER), DSI_TRIGGER_HOST);

	// Restore host control.
	DSI(_DSIREG(DSI_HOST_CONTROL)) = host_control;
}

void display_dsi_vblank_write(u8 cmd, u32 len, void *data)
{
	u32 fifo32[DSI_STATUS_TX_FIFO_SIZE] = {0};
	u8 *fifo8 = (u8 *)fifo32;

	// Prepare data for long write.
	if (len >= 2)
	{
		memcpy(&fifo8[5], data, len);
		memset(&fifo8[5] + len, 0, len % sizeof(u32));
		len++; // Increase length by CMD.
	}

	_display_dsi_wait_vblank(true);

	switch (len)
	{
	case 0:
		DSI(_DSIREG(DSI_WR_DATA)) = (cmd << 8) | MIPI_DSI_DCS_SHORT_WRITE;
		break;

	case 1:
		DSI(_DSIREG(DSI_WR_DATA)) = ((cmd | (*(u8 *)data << 8)) << 8) | MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		break;

	default:
		fifo32[0] = (len << 8) | MIPI_DSI_DCS_LONG_WRITE;
		fifo8[4] = cmd;
		len += sizeof(u32); // Increase length by length word and DCS CMD.
		for (u32 i = 0; i < (ALIGN(len, sizeof(u32)) / sizeof(u32)); i++)
			DSI(_DSIREG(DSI_WR_DATA)) = fifo32[i];
		break;
	}

	_display_dsi_wait_vblank(false);
}

void display_init()
{
	// Get Hardware type, as it's used in various DI functions.
	_nx_aula = fuse_read_hw_type() == FUSE_NX_HW_TYPE_AULA;

	// Check if display is already initialized.
	if (CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & BIT(CLK_L_DISP1))
		_display_panel_and_hw_end(true);

	// Get Chip ID.
	bool tegra_t210 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;

	// Enable DSI AVDD.
	max7762x_regulator_set_voltage(REGULATOR_LDO0, 1200000);
	max7762x_regulator_enable(REGULATOR_LDO0, true);

	// Enable Display Interface specific clocks.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_CLR) = BIT(CLK_H_MIPI_CAL) | BIT(CLK_H_DSI);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = BIT(CLK_H_MIPI_CAL) | BIT(CLK_H_DSI);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = BIT(CLK_L_DISP1);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = BIT(CLK_L_DISP1);

	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) = BIT(CLK_X_UART_FST_MIPI_CAL);
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL) = CLK_SRC_DIV(6); // Set PLLP_OUT3 and div 6 (68MHz).

	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_W_SET) = BIT(CLK_W_DSIA_LP);
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_DSIA_LP) = CLK_SRC_DIV(6);           // Set PLLP_OUT  and div 6 (68MHz).

	// Bring every IO rail out of deep power down. (Though no rail bit is set.)
	PMC(APBDEV_PMC_IO_DPD_REQ)  = PMC_IO_DPD_REQ_DPD_OFF;
	PMC(APBDEV_PMC_IO_DPD2_REQ) = PMC_IO_DPD_REQ_DPD_OFF;

	// Configure LCD/BL pins.
	if (!_nx_aula)
	{
		// Configure LCD pins.
		PINMUX_AUX(PINMUX_AUX_NFC_EN)     = PINMUX_PULL_DOWN;
		PINMUX_AUX(PINMUX_AUX_NFC_INT)    = PINMUX_PULL_DOWN;

		// Configure Backlight pins.
		PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = PINMUX_PULL_DOWN;
		PINMUX_AUX(PINMUX_AUX_LCD_BL_EN)  = PINMUX_PULL_DOWN;

		// Enable LCD AVDD.
		gpio_direction_output(GPIO_PORT_I, GPIO_PIN_0 | GPIO_PIN_1, GPIO_HIGH);
		usleep(10000); // Wait minimum 4.2ms to stabilize.

		// Configure Backlight PWM/EN pins (BL PWM, BL EN).
		gpio_direction_output(GPIO_PORT_V, GPIO_PIN_0 | GPIO_PIN_1, GPIO_LOW);

		 // Enable Backlight power.
		gpio_write(GPIO_PORT_V, GPIO_PIN_1, GPIO_HIGH);
	}

	// Configure LCD RST pin.
	PINMUX_AUX(PINMUX_AUX_LCD_RST) = PINMUX_PULL_DOWN;
	gpio_direction_output(GPIO_PORT_V, GPIO_PIN_2, GPIO_LOW);

	// Power up supply regulator for display interface.
	MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG2)) = 0;

	if (!tegra_t210)
	{
		MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG0)) = 0;
		APB_MISC(APB_MISC_GP_DSI_PAD_CONTROL) = 0;
	}

	// Set DISP1 clock source, parent clock and DSI/PCLK to command mode.
	// T210:    DIVM: 1, DIVN: 20, DIVP: 3. PLLD_OUT: 100.0 MHz, PLLD_OUT0 (DSI-BCLK): 50.0 MHz. (PCLK: 16.66 MHz)
	// T210B01: DIVM: 1, DIVN: 20, DIVP: 3. PLLD_OUT:  97.8 MHz, PLLD_OUT0 (DSI-BCLK): 48.9 MHz. (PCLK: 16.30 MHz)
	clock_enable_plld(3, 20, true, tegra_t210);

	// Setup Display Interface initial window configuration.
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_dc_setup_win_config, ARRAY_SIZE(_di_dc_setup_win_config));

	// Setup dsi init sequence packets.
	reg_write_array((u32 *)DSI_BASE, _di_dsi_seq_pkt_reset_config0,  ARRAY_SIZE(_di_dsi_seq_pkt_reset_config0));
	DSI(_DSIREG(tegra_t210 ? DSI_INIT_SEQ_DATA_15 : DSI_INIT_SEQ_DATA_15_B01)) = 0;
	reg_write_array((u32 *)DSI_BASE, _di_dsi_seq_pkt_reset_config1,  ARRAY_SIZE(_di_dsi_seq_pkt_reset_config1));

	// Reset pad trimmers for T210B01.
	if (!tegra_t210)
		reg_write_array((u32 *)DSI_BASE, _di_dsi_init_pads_t210b01, ARRAY_SIZE(_di_dsi_init_pads_t210b01));

	// Setup init seq packet lengths, timings and power on DSI.
	reg_write_array((u32 *)DSI_BASE, _di_dsi_init_config, ARRAY_SIZE(_di_dsi_init_config));
	usleep(10000);

	// Enable LCD Reset.
	gpio_write(GPIO_PORT_V, GPIO_PIN_2, GPIO_HIGH);
	usleep(60000);

	// Setup DSI device takeover timeout.
	DSI(_DSIREG(DSI_BTA_TIMING)) = _nx_aula ? 0x40103 : 0x50204;

	// Get Display ID.
	_display_id = 0xCCCCCC;
	for (u32 i = 0; i < 3; i++)
	{
		if (!display_dsi_read(MIPI_DCS_GET_DISPLAY_ID, 3, &_display_id))
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
	if (_nx_aula && _display_id == 0xCCCC)
		_display_id = PANEL_SAM_AMS699VC01;

	// Initialize display panel.
	switch (_display_id)
	{
	case PANEL_SAM_AMS699VC01:
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 180000);
		// Set color mode to basic (natural). Stock is Saturated (0x00). (Reset value is 0x20).
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE_PARAM,
							  MIPI_DCS_PRIV_SM_SET_COLOR_MODE | (DCS_SM_COLOR_MODE_BASIC << 8), 0);
		// Enable backlight and smooth PWM.
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE_PARAM,
							  MIPI_DCS_SET_CONTROL_DISPLAY | ((DCS_CONTROL_DISPLAY_BRIGHTNESS_CTRL | DCS_CONTROL_DISPLAY_DIMMING_CTRL) << 8), 0);

		// Unlock Level 2 registers.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x539;      // MIPI_DSI_DCS_LONG_WRITE: 5 bytes.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x5A5A5AE2; // MIPI_DCS_PRIV_SM_SET_REGS_LOCK: Unlock Level 2 registers.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x5A;
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;

		// Set registers offset and set PWM transition to 6 frames (100ms).
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE_PARAM, MIPI_DCS_PRIV_SM_SET_REG_OFFSET | (7 << 8), 0);
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE_PARAM, MIPI_DCS_PRIV_SM_SET_ELVSS      | (6 << 8), 0);

		// Relock Level 2 registers.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x539;      // MIPI_DSI_DCS_LONG_WRITE: 5 bytes.
		DSI(_DSIREG(DSI_WR_DATA)) = 0xA55A5AE2; // MIPI_DCS_PRIV_SM_SET_REGS_LOCK: Lock Level 2 registers.
		DSI(_DSIREG(DSI_WR_DATA)) = 0xA5;
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;

		// Set backlight to 0%.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x339;    // MIPI_DSI_DCS_LONG_WRITE: 3 bytes.
		DSI(_DSIREG(DSI_WR_DATA)) = 0x000051; // MIPI_DCS_SET_BRIGHTNESS 0000: 0%. FF07: 100%.
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		_dsi_bl = 0;
		break;

	case PANEL_JDI_XXX062M:
		reg_write_array((u32 *)DSI_BASE, _di_dsi_panel_init_config_jdi, ARRAY_SIZE(_di_dsi_panel_init_config_jdi));
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
	case PANEL_SHP_LQ055T1SW10:
	default: // Allow spare part displays to work.
		_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_EXIT_SLEEP_MODE, 120000);
		break;
	}

	// Unblank display.
	_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_SET_DISPLAY_ON, 20000);

	// Switch to DSI HS mode.
	// DIVM: 1, DIVN: 24, DIVP: 1. PLLD_OUT: 468.0 MHz, PLLD_OUT0 (DSI-BCLK): 234.0 MHz. (PCLK: 78 MHz)
	clock_enable_plld(1, 24, false, tegra_t210);

	// Set HS PHY timing and finalize DSI packet sequence configuration.
	reg_write_array((u32 *)DSI_BASE, _di_dsi_seq_pkt_video_non_burst_no_eot_config, ARRAY_SIZE(_di_dsi_seq_pkt_video_non_burst_no_eot_config));

	// Set 1-by-1 pixel/clock and pixel clock to 234 / 3 = 78 MHz. For 60 Hz refresh rate.
	DISPLAY_A(_DIREG(DC_DISP_DISP_CLOCK_CONTROL)) = PIXEL_CLK_DIVIDER_PCD1 | SHIFT_CLK_DIVIDER(4); // 4: div3.

	// Set DSI mode to HOST.
	reg_write_array((u32 *)DSI_BASE, _di_dsi_host_mode_config, ARRAY_SIZE(_di_dsi_host_mode_config));
	usleep(10000);

	/*
	 * Calibrate display communication pads.
	 * When switching to the 16ff pad brick, the clock lane termination control
	 * is separated from data lane termination. This change of the mipi cal
	 * brings in a bug that the DSI pad clock termination code can't be loaded
	 * in one time calibration on T210B01. Trigger calibration twice.
	 */
	reg_write_array((u32 *)MIPI_CAL_BASE, _di_mipi_pad_cal_config, ARRAY_SIZE(_di_mipi_pad_cal_config));
	for (u32 i = 0; i < 2; i++)
	{
		// Set MIPI bias pad config.
		MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG2)) = 0x10010;
		MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_BIAS_PAD_CFG1)) = tegra_t210 ? 0x300 : 0;

		// Set pad trimmers and set MIPI DSI cal offsets.
		if (tegra_t210)
		{
			reg_write_array((u32 *)DSI_BASE,      _di_dsi_pad_cal_config_t210,          ARRAY_SIZE(_di_dsi_pad_cal_config_t210));
			reg_write_array((u32 *)MIPI_CAL_BASE, _di_mipi_dsi_cal_prod_config_t210,    ARRAY_SIZE(_di_mipi_dsi_cal_prod_config_t210));
		}
		else
		{
			reg_write_array((u32 *)DSI_BASE,      _di_dsi_pad_cal_config_t210b01,       ARRAY_SIZE(_di_dsi_pad_cal_config_t210b01));
			reg_write_array((u32 *)MIPI_CAL_BASE, _di_mipi_dsi_cal_prod_config_t210b01, ARRAY_SIZE(_di_mipi_dsi_cal_prod_config_t210b01));
		}

		// Reset all unused MIPI cal offsets.
		reg_write_array((u32 *)MIPI_CAL_BASE, _di_mipi_dsi_cal_unused_config, ARRAY_SIZE(_di_mipi_dsi_cal_unused_config));

		// Set Prescale/filter and start calibration.
		MIPI_CAL(_DSIREG(MIPI_CAL_MIPI_CAL_CTRL)) = 0x2A000001;
	}
	usleep(10000);

	// Enable video display controller.
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_dc_video_enable_config, ARRAY_SIZE(_di_dc_video_enable_config));
}

void display_backlight_pwm_init()
{
	if (_display_id == PANEL_SAM_AMS699VC01)
		return;

	// Enable PWM clock.
	clock_enable_pwm();

	// Enable PWM and set it to 25KHz PFM. 29.5KHz is stock.
	PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN;

	PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = (PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) & ~PINMUX_FUNC_MASK) | 1; // Set PWM0 mode.
	usleep(2);

	gpio_config(GPIO_PORT_V, GPIO_PIN_0, GPIO_MODE_SPIO); // Backlight power mode.
}

void display_backlight(bool enable)
{
	// Backlight PWM GPIO.
	gpio_write(GPIO_PORT_V, GPIO_PIN_0, enable ? GPIO_HIGH : GPIO_LOW);
}

static void _display_dsi_backlight_brightness(u32 duty)
{
	if (_dsi_bl == duty)
		return;

	// Convert duty to candela.
	u32 candela = duty * PANEL_SM_BL_CANDELA_MAX / 255;

	u16 bl_ctrl = byte_swap_16((u16)candela);
	display_dsi_vblank_write(MIPI_DCS_SET_BRIGHTNESS, 2, &bl_ctrl);

	// Wait for backlight to completely turn off. 6 frames.
	if (!duty)
		usleep(100000);

	_dsi_bl = duty;
}

static void _display_pwm_backlight_brightness(u32 duty, u32 step_delay)
{
	u32 old_value = (PWM(PWM_CONTROLLER_PWM_CSR_0) >> 16) & 0xFF;
	if (duty == old_value)
		return;

	if (old_value < duty)
	{
		for (u32 i = old_value; i < duty + 1; i++)
		{
			PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN | (i << 16);
			usleep(step_delay);
		}
	}
	else
	{
		for (u32 i = old_value; i > duty; i--)
		{
			PWM(PWM_CONTROLLER_PWM_CSR_0) = PWM_CSR_EN | (i << 16);
			usleep(step_delay);
		}
	}
	if (!duty)
		PWM(PWM_CONTROLLER_PWM_CSR_0) = 0;
}

void display_backlight_brightness(u32 brightness, u32 step_delay)
{
	if (brightness > 255)
		brightness = 255;

	if (_display_id != PANEL_SAM_AMS699VC01)
		_display_pwm_backlight_brightness(brightness, step_delay);
	else
		_display_dsi_backlight_brightness(brightness);
}

u32 display_get_backlight_brightness()
{
	if (_display_id != PANEL_SAM_AMS699VC01)
		return ((PWM(PWM_CONTROLLER_PWM_CSR_0) >> 16) & 0xFF);
	else
		return _dsi_bl;
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

	// Wait for 5 frames (HOST1X_CH0_SYNC_SYNCPT_9).
	// Not here. Wait for 1 frame manually.
	usleep(20000);

	// Propagate changes to all register buffers and disable host cmd packets during video.
	DISPLAY_A(_DIREG(DC_CMD_STATE_ACCESS)) = READ_MUX_ACTIVE | WRITE_MUX_ACTIVE;
	DSI(_DSIREG(DSI_VIDEO_MODE_CONTROL)) = 0;

	// De-initialize video controller.
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_dc_video_disable_config, ARRAY_SIZE(_di_dc_video_disable_config));

	// Set DISP1 clock source, parent clock and DSI/PCLK to command mode.
	// T210:    DIVM: 1, DIVN: 20, DIVP: 3. PLLD_OUT: 100.0 MHz, PLLD_OUT0 (DSI-BCLK): 50.0 MHz. (PCLK: 16.66 MHz)
	// T210B01: DIVM: 1, DIVN: 20, DIVP: 3. PLLD_OUT:  97.8 MHz, PLLD_OUT0 (DSI-BCLK): 48.9 MHz. (PCLK: 16.30 MHz)
	clock_enable_plld(3, 20, true, hw_get_chip_id() == GP_HIDREV_MAJOR_T210);

	// Set timings for lowpower clocks.
	reg_write_array((u32 *)DSI_BASE, _di_dsi_timing_deinit_config, ARRAY_SIZE(_di_dsi_timing_deinit_config));

	if (_display_id != PANEL_SAM_AMS699VC01)
		usleep(10000);

	// De-initialize display panel.
	switch (_display_id)
	{
	case PANEL_JDI_XXX062M:
		reg_write_array((u32 *)DSI_BASE, _di_dsi_panel_deinit_config_jdi, ARRAY_SIZE(_di_dsi_panel_deinit_config_jdi));
		break;

	case PANEL_AUO_A062TAN01:
		reg_write_array((u32 *)DSI_BASE, _di_dsi_panel_deinit_config_auo, ARRAY_SIZE(_di_dsi_panel_deinit_config_auo));
		usleep(5000);
		break;

	case PANEL_INL_2J055IA_27A:
	case PANEL_AUO_A055TAN01:
	case PANEL_SHP_LQ055T1SW10:
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
		else // PANEL_SHP_LQ055T1SW10.
			DSI(_DSIREG(DSI_WR_DATA)) = 0x731348B1; // MIPI_DCS_PRIV_SET_POWER_CONTROL. (Not deep standby, BT3 / XDK, VRH gamma volt adj 51 / x40).
		if (_display_id == PANEL_INL_2J055IA_27A || _display_id == PANEL_AUO_A055TAN01)
		{
			// (NVRH gamma volt adj 9, Amplifier current small / x30, FS0 freq Fosc/80 / FS1 freq Fosc/32, Enter standby / PON / VCOMG).
			DSI(_DSIREG(DSI_WR_DATA)) = 0x71143209;
			DSI(_DSIREG(DSI_WR_DATA)) = 0x114D31;   // (Unknown).
		}
		else // PANEL_SHP_LQ055T1SW10.
		{
			// (NVRH gamma volt adj 9, Amplifier current small / x30, FS0 freq Fosc/80 / FS1 freq Fosc/48, Enter standby / PON / VCOMG).
			DSI(_DSIREG(DSI_WR_DATA)) = 0x71243209;
			DSI(_DSIREG(DSI_WR_DATA)) = 0x004C31;   // (Unknown).
		}
		DSI(_DSIREG(DSI_TRIGGER)) = DSI_TRIGGER_HOST;
		usleep(5000);
		break;

	case PANEL_INL_P062CCA_AZ1:
	case PANEL_SAM_AMS699VC01:
	default:
		break;
	}

	// Blank - powerdown.
	_display_dsi_send_cmd(MIPI_DSI_DCS_SHORT_WRITE, MIPI_DCS_ENTER_SLEEP_MODE,
		(_display_id == PANEL_SAM_AMS699VC01) ? 120000 : 50000);

skip_panel_deinit:
	// Disable LCD power pins.
	gpio_write(GPIO_PORT_V, GPIO_PIN_2, GPIO_LOW);     // LCD Reset disable.
	usleep(10000);

	if (!_nx_aula) // HOS uses panel id.
	{
		gpio_write(GPIO_PORT_I, GPIO_PIN_1, GPIO_LOW); // LCD AVDD -5.4V disable.
		gpio_write(GPIO_PORT_I, GPIO_PIN_0, GPIO_LOW); // LCD AVDD +5.4V disable.

		// Make sure LCD PWM backlight pin is in PWM0 mode.
		gpio_config(GPIO_PORT_V, GPIO_PIN_0, GPIO_MODE_SPIO); // Backlight PWM.
		PINMUX_AUX(PINMUX_AUX_LCD_BL_PWM) = PINMUX_TRISTATE | PINMUX_PULL_DOWN | 1; // Set PWM0 mode.
	}
	usleep(10000);

	// Disable Display Interface specific clocks.
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_SET) = BIT(CLK_H_MIPI_CAL) | BIT(CLK_H_DSI);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_CLR) = BIT(CLK_H_MIPI_CAL) | BIT(CLK_H_DSI);
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = BIT(CLK_L_DISP1);
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = BIT(CLK_L_DISP1);

	// Power down pads.
	DSI(_DSIREG(DSI_PAD_CONTROL_0)) = DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) |
									  DSI_PAD_CONTROL_VS1_PDIO_CLK   | DSI_PAD_CONTROL_VS1_PDIO(0xF);
	DSI(_DSIREG(DSI_POWER_CONTROL)) = 0;

	// Disable DSI AVDD.
	max7762x_regulator_enable(REGULATOR_LDO0, false);
}

void display_end() { _display_panel_and_hw_end(false); };

u16 display_get_decoded_panel_id()
{
	return _display_id;
}

void display_set_decoded_panel_id(u32 id)
{
	// Get Hardware type, as it's used in various DI functions.
	_nx_aula = fuse_read_hw_type() == FUSE_NX_HW_TYPE_AULA;

	// Decode Display ID.
	_display_id = ((id >> 8) & 0xFF00) | (id & 0xFF);

	if ((_display_id & 0xFF) == PANEL_JDI_XXX062M)
		_display_id = PANEL_JDI_XXX062M;

	// For Aula ensure that we have a compatible panel id.
	if (_nx_aula && _display_id == 0xCCCC)
		_display_id = PANEL_SAM_AMS699VC01;
}

void display_color_screen(u32 color)
{
	// Disable all windows.
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_win_one_color, ARRAY_SIZE(_di_win_one_color));

	// Configure display to show single color.
	DISPLAY_A(_DIREG(DC_DISP_BLEND_BACKGROUND_COLOR)) = color;

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | WIN_A_UPDATE |  WIN_B_UPDATE  | WIN_C_UPDATE  | WIN_D_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ | WIN_D_ACT_REQ;
	usleep(35000); // Wait 2 frames. No need on Aula.

	if (_display_id != PANEL_SAM_AMS699VC01)
		display_backlight(true);
	else
		display_backlight_brightness(150, 0);
}

u32 *display_init_window_a_pitch()
{
	// Sanitize framebuffer area.
	memset((u32 *)IPL_FB_ADDRESS, 0, IPL_FB_SZ);

	// This configures the framebuffer @ IPL_FB_ADDRESS with a resolution of 720x1280 (line stride 720).
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_winA_pitch, ARRAY_SIZE(_di_winA_pitch));
	//usleep(35000); // Wait 2 frames. No need on Aula.

	return (u32 *)DISPLAY_A(_DIREG(DC_WINBUF_START_ADDR));
}

u32 *display_init_window_a_pitch_vic()
{
	// This configures the framebuffer @ NYX_FB_ADDRESS with a resolution of 720x1280 (line stride 720).
	if (_display_id != PANEL_SAM_AMS699VC01)
		usleep(8000); // Wait half frame for PWM to apply.
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_winA_pitch_vic, ARRAY_SIZE(_di_winA_pitch_vic));
	if (_display_id != PANEL_SAM_AMS699VC01)
		usleep(35000); // Wait 2 frames.

	return (u32 *)DISPLAY_A(_DIREG(DC_WINBUF_START_ADDR));
}

u32 *display_init_window_a_pitch_inv()
{
	// This configures the framebuffer @ NYX_FB_ADDRESS with a resolution of 720x1280 (line stride 720).
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_winA_pitch_inv, ARRAY_SIZE(_di_winA_pitch_inv));
	usleep(35000); // Wait 2 frames. No need on Aula.

	return (u32 *)DISPLAY_A(_DIREG(DC_WINBUF_START_ADDR));
}

u32 *display_init_window_a_block()
{
	// This configures the framebuffer @ NYX_FB_ADDRESS with a resolution of 720x1280.
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_winA_block, ARRAY_SIZE(_di_winA_block));
	usleep(35000); // Wait 2 frames. No need on Aula.

	return (u32 *)DISPLAY_A(_DIREG(DC_WINBUF_START_ADDR));
}

u32 *display_init_window_d_console()
{
	// This configures the framebuffer @ LOG_FB_ADDRESS with a resolution of 1280x720 (line stride 720).
	reg_write_array((u32 *)DISPLAY_A_BASE, _di_winD_log, ARRAY_SIZE(_di_winD_log));

	return (u32 *)DISPLAY_A(_DIREG(DC_WINBUF_START_ADDR));
}

void display_window_disable(u32 window)
{
	// Select window C.
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = BIT(WINDOW_SELECT + window);

	// Disable window C.
	DISPLAY_A(_DIREG(DC_WIN_WIN_OPTIONS)) = 0;

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | BIT(WIN_UPDATE  + window);
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | BIT(WIN_ACT_REQ + window);
}

void display_set_framebuffer(u32 window, void *fb)
{
	// Select window.
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = BIT(WINDOW_SELECT + window);

	// Set new fb address.
	DISPLAY_A(_DIREG(DC_WINBUF_START_ADDR)) = (u32)fb;

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | BIT(WIN_UPDATE  + window);
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | BIT(WIN_ACT_REQ + window);
}

void display_move_framebuffer(u32 window, void *fb)
{
	// Select window.
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = BIT(WINDOW_SELECT + window);

	// Get current framebuffer address.
	const void *fb_curr = (void *)DISPLAY_A(_DIREG(DC_WINBUF_START_ADDR));
	u32 win_size = DISPLAY_A(_DIREG(DC_WIN_PRESCALED_SIZE));
	win_size = (win_size & 0x7FFF) * ((win_size >> 16) & 0x1FFF);

	// Copy fb over.
	memcpy(fb, fb_curr, win_size);

	// Set new fb address.
	DISPLAY_A(_DIREG(DC_WINBUF_START_ADDR)) = (u32)fb;

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | BIT(WIN_UPDATE  + window);
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | BIT(WIN_ACT_REQ + window);
}

void display_window_d_console_enable()
{
	// Only update active registers on vsync.
	DISPLAY_A(_DIREG(DC_CMD_REG_ACT_CONTROL)) = DISPLAY_A(_DIREG(DC_CMD_REG_ACT_CONTROL)) & ~WIN_D_ACT_HCNTR_SEL;

	// Select window D.
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = WINDOW_D_SELECT;

	// Enable and setup window D.
	DISPLAY_A(_DIREG(DC_WIN_WIN_OPTIONS))   = WIN_ENABLE;
	DISPLAY_A(_DIREG(DC_WIN_POSITION))      = 0xFF80; // X: -128.

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | WIN_D_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;

	// Pull-down effect.
	for (u32 i = 0xFF80; i < 0x10000; i++)
	{
		// Set window position.
		DISPLAY_A(_DIREG(DC_WIN_POSITION)) = i & 0xFFFF;

		// Arm and activate changes.
		DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | WIN_D_UPDATE;
		DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;
		usleep(1000);
	}

	DISPLAY_A(_DIREG(DC_WIN_POSITION)) = 0;

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | WIN_D_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;
}

void display_window_d_console_disable()
{
	// Select window D.
	DISPLAY_A(_DIREG(DC_CMD_DISPLAY_WINDOW_HEADER)) = WINDOW_D_SELECT;

	// Pull-up effect.
	for (u32 i = 0xFFFF; i > 0xFF7F; i--)
	{
		// Set window position.
		DISPLAY_A(_DIREG(DC_WIN_POSITION)) = i & 0xFFFF;

		// Arm and activate changes.
		DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | WIN_D_UPDATE;
		DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;
		usleep(500);
	}

	// Disable window D.
	DISPLAY_A(_DIREG(DC_WIN_POSITION))    = 0;
	DISPLAY_A(_DIREG(DC_WIN_WIN_OPTIONS)) = 0;

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | WIN_D_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | WIN_D_ACT_REQ;
}

void display_cursor_init(void *crs_fb, u32 size)
{
	// Setup cursor.
	DISPLAY_A(_DIREG(DC_DISP_CURSOR_START_ADDR))    = CURSOR_CLIPPING(CURSOR_CLIP_WIN_A) | size | ((u32)crs_fb >> 10);
	DISPLAY_A(_DIREG(DC_DISP_BLEND_CURSOR_CONTROL)) = CURSOR_BLEND_R8G8B8A8                    |
													  CURSOR_BLEND_DST_FACTOR(CURSOR_BLEND_K1) |
													  CURSOR_BLEND_SRC_FACTOR(CURSOR_BLEND_K1) | 0xFF;

	// Enable cursor window.
	DISPLAY_A(_DIREG(DC_DISP_DISP_WIN_OPTIONS)) |= CURSOR_ENABLE;

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | CURSOR_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | CURSOR_ACT_REQ;
}

void display_cursor_set_pos(u32 x, u32 y)
{
	// Set cursor position.
	DISPLAY_A(_DIREG(DC_DISP_CURSOR_POSITION)) = x | (y << 16);

	// Arm and activate changes.
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | CURSOR_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | CURSOR_ACT_REQ;
}

void display_cursor_deinit()
{
	DISPLAY_A(_DIREG(DC_DISP_BLEND_CURSOR_CONTROL)) = 0;
	DISPLAY_A(_DIREG(DC_DISP_DISP_WIN_OPTIONS)) &= ~CURSOR_ENABLE;

	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_UPDATE  | CURSOR_UPDATE;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = GENERAL_ACT_REQ | CURSOR_ACT_REQ;
}
