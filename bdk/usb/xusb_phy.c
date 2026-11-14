/*
 * Tegra210/Tegra210B01 USB3 port 0 PHY initialization.
 *
 * Adapted from NVIDIA's GPLv2 Linux Tegra XUSB padctl and PLLE drivers:
 * drivers/phy/tegra/xusb-tegra210.c (including Tegra210B01 calibration),
 * drivers/clk/tegra/clk-pll.c.
 * Copyright (c) 2014-2020 NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2015 Google, Inc.
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <usb/xusb_phy.h>
#include <usb/usb_t210.h>
#include <power/max7762x.h>
#include <power/max77620.h>
#include <soc/clock.h>
#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/t210.h>
#include <soc/timer.h>

#define USB3_PAD_MUX       0x028
#define UPHY_PLL_CTL1      0x360
#define UPHY_PLL_CTL2      0x364
#define UPHY_PLL_CTL4      0x36C
#define UPHY_PLL_CTL5      0x370
#define UPHY_PLL_CTL8      0x37C
#define UPHY_PLL_CTL10     0x384
#define UPHY_LANE_CTL8(n)  (0x47C + (n) * 0x40)
#define USB3_ECTL1         0xA60
#define USB3_ECTL2         0xA64
#define USB3_ECTL3         0xA68
#define USB3_ECTL4         0xA6C
#define USB3_ECTL6         0xA74

#define PLL_IDDQ           BIT(0)
#define PLL_SLEEP_MASK     (3 << 1)
#define PLL_ENABLE         BIT(3)
#define PLL_PWR_OVRD       BIT(4)
#define PLL_LOCK           BIT(15)
#define PLL_CAL_EN         BIT(0)
#define PLL_CAL_DONE       BIT(1)
#define PLL_CAL_OVRD       BIT(2)
#define PLL_RCAL_EN        BIT(12)
#define PLL_RCAL_CLK_EN    BIT(13)
#define PLL_RCAL_OVRD      BIT(15)
#define PLL_RCAL_DONE      BIT(31)
#define SS_CLAMP           BIT(0)
#define SS_CLAMP_EARLY     BIT(1)
#define SS_VCORE_DOWN      BIT(2)
#define AUX_CLAMP          BIT(29)
#define AUX_CLAMP_EARLY    BIT(30)
#define AUX_VCORE_DOWN     BIT(31)
#define PLLE_SS_CTRL       0x068
#define PLLE_ENABLE       BIT(31)
#define PLLE_LOCK          BIT(11)
#define PLLE_IDDQ          BIT(13)
#define PLLE_IDDQ_SWCTL    BIT(14)
#define PLLE_HW_SEQ        BIT(24)
#define XUSBIO_PLL_CFG0    0x51C
#define PLLP_HSIO_GATE     BIT(29)
#define PLLP_XUSB_GATE     BIT(28)
#define UPHY_MGMT_DIV_MASK 0xFF
#define UPHY_MGMT_GATE     BIT(8)
#define UPHY_MGMT_DIV_102M 6 // Fractional divider: 408 MHz / (1 + 6/2).

static bool phy_owned;
static xusb_phy_result_t phy_result;
static u32 phy_lane;
static u32 saved_mux, saved_map, saved_elpg;
static u32 saved_plle_base, saved_plle_misc, saved_plle_aux, saved_plle_ss;
static u8 saved_ldo1, saved_ldo7;
static u32 saved_uphy_reset;
static u32 saved_uphy_mgmt, saved_pllp_gates;
static u32 saved_pad_protection[4];

xusb_phy_result_t xusb_phy_superspeed_result(void)
{
	return phy_result;
}

const char *xusb_phy_superspeed_status(void)
{
	static const char * const messages[] = {
		"PHY not attempted",
		"skipped: PLLE sequencer active",
		"skipped: PLLE already enabled",
		"skipped: UPHY sequencer active",
		"regulator setup failed",
		"unsupported oscillator",
		"PLLE lock timeout",
		"PLL calibration start timeout",
		"PLL calibration stop timeout",
		"UPHY PLL lock timeout",
		"resistor calibration start timeout",
		"resistor calibration stop timeout",
		"PHY ready; link negotiated USB 2"
	};
	return messages[phy_result];
}

static bool _pad_wait(u32 reg, u32 mask, u32 expected)
{
	for (u32 i = 0; i < 10000; i++)
	{
		if ((XUSB_PADCTL(reg) & mask) == expected)
			return true;
		usleep(10);
	}
	return false;
}

static bool _plle_enable(void)
{
	// Switch's PLL reference is the 38.4 MHz oscillator. Do not reprogram
	// clocks inherited from a running hardware sequencer or another client.
	if (CLOCK(CLK_RST_CONTROLLER_PLLE_AUX) & PLLE_HW_SEQ)
		return false;
	if (CLOCK(CLK_RST_CONTROLLER_PLLE_BASE) & PLLE_ENABLE)
		return false;
	phy_result = XUSB_PHY_UNSUPPORTED_OSC;
	if (clock_get_osc_freq() != 38400)
		return false;
	phy_result = XUSB_PHY_PLLE_LOCK_FAILED;

	CLOCK(CLK_RST_CONTROLLER_PLLE_AUX) &= ~(BIT(28) | BIT(2)); // pll_ref parent.
	CLOCK(CLK_RST_CONTROLLER_PLLE_AUX) |= BIT(4) | BIT(6); // Software control.
	CLOCK(CLK_RST_CONTROLLER_PLLE_BASE) &= ~BIT(30); // Disable lock override.
	CLOCK(CLK_RST_CONTROLLER_PLLE_MISC) =
		(CLOCK(CLK_RST_CONTROLLER_PLLE_MISC) & ~(PLLE_IDDQ | (3 << 4) | (2 << 2))) |
		BIT(9) | PLLE_IDDQ_SWCTL | BIT(8);
	usleep(5);
	CLOCK(PLLE_SS_CTRL) |= BIT(10) | BIT(11) | BIT(12);
	// M=2, N=125, CML=14: 38.4 MHz reference -> 100 MHz PLLE output.
	CLOCK(CLK_RST_CONTROLLER_PLLE_BASE) =
		(CLOCK(CLK_RST_CONTROLLER_PLLE_BASE) & ~((0x1F << 24) | 0xFFFF)) |
		(14 << 24) | (125 << 8) | 2;
	usleep(1);
	CLOCK(CLK_RST_CONTROLLER_PLLE_BASE) |= PLLE_ENABLE;
	bool locked = false;
	for (u32 i = 0; i < 10000; i++)
	{
		if (CLOCK(CLK_RST_CONTROLLER_PLLE_MISC) & PLLE_LOCK)
		{
			locked = true;
			break;
		}
		usleep(10);
	}
	if (!locked)
		return false;
	CLOCK(PLLE_SS_CTRL) = (CLOCK(PLLE_SS_CTRL) &
		~(BIT(14) | BIT(15) | 0x1FF | (0xFF << 16) | (0x3F << 24))) |
		0x21 | BIT(16) | (0x23 << 24);
	CLOCK(PLLE_SS_CTRL) &= ~(BIT(12) | BIT(10));
	usleep(1);
	CLOCK(PLLE_SS_CTRL) &= ~BIT(11);
	usleep(1);
	return true;
}

static void _mariko_calibration(void)
{
	static const u32 lane_data[] = {
		0x00010002, 0x00040032, 0x00070022, 0x00352587,
		0x00490FC7, 0x00520001, 0x00533C0F, 0x0056C00F,
		0x005DFF07, 0x005E141A, 0x00970080
	};
	static const u32 pll_data[] = { 0x00020000, 0x00037051, 0x00250130, 0x001E0017 };
	for (u32 i = 0; i < ARRAY_SIZE(lane_data); i++)
		XUSB_PADCTL(UPHY_LANE_CTL8(phy_lane)) = lane_data[i] | BIT(27) | BIT(24);
	for (u32 i = 0; i < ARRAY_SIZE(pll_data); i++)
		XUSB_PADCTL(UPHY_PLL_CTL10) = pll_data[i] | BIT(27) | BIT(24);
}

bool xusb_phy_superspeed_init(void)
{
	if (phy_owned)
		return true;
	bool erista = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;
	phy_lane = erista ? 6 : 5; // Switch Linux board lane mapping, USB3 port 0.
	// Avoid disturbing a PHY inherited from an OS. USB2 remains available.
	phy_result = XUSB_PHY_PLLE_SEQUENCER_ACTIVE;
	if (CLOCK(CLK_RST_CONTROLLER_PLLE_AUX) & PLLE_HW_SEQ)
		return false;
	phy_result = XUSB_PHY_PLLE_ALREADY_ENABLED;
	if (CLOCK(CLK_RST_CONTROLLER_PLLE_BASE) & PLLE_ENABLE)
		return false;
	phy_result = XUSB_PHY_UPHY_SEQUENCER_ACTIVE;
	if (CLOCK(XUSBIO_PLL_CFG0) & BIT(24))
		return false;

	saved_mux = XUSB_PADCTL(USB3_PAD_MUX);
	saved_map = XUSB_PADCTL(XUSB_PADCTL_SS_PORT_MAP);
	saved_elpg = XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1);
	saved_plle_base = CLOCK(CLK_RST_CONTROLLER_PLLE_BASE);
	saved_plle_misc = CLOCK(CLK_RST_CONTROLLER_PLLE_MISC);
	saved_plle_aux = CLOCK(CLK_RST_CONTROLLER_PLLE_AUX);
	saved_plle_ss = CLOCK(PLLE_SS_CTRL);
	saved_ldo1 = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_LDO1_CFG);
	saved_ldo7 = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_LDO7_CFG);
	saved_uphy_reset = CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_Y) & BIT(CLK_Y_PEX_USB_UPHY);
	if (!erista)
	{
		saved_uphy_mgmt = CLOCK(CLK_RST_CONTROLLER_PEX_SATA_USB_RX_BYP);
		saved_pllp_gates = CLOCK(CLK_RST_CONTROLLER_PLLP_MISC1) & (PLLP_HSIO_GATE | PLLP_XUSB_GATE);
	}
	for (u32 port = 0; port < 4; port++)
		saved_pad_protection[port] = XUSB_PADCTL(0x84 + port * 0x40);
	phy_owned = true;
	phy_result = XUSB_PHY_POWER_FAILED;

	// LDO1 supplies dvddio_pex on both revisions; Erista additionally uses
	// LDO7 for avdd_pll_uerefe. LDO8 is shared with other board devices.
	if (max7762x_regulator_set_voltage(REGULATOR_LDO1, 1050000) ||
		max7762x_regulator_enable(REGULATOR_LDO1, true))
		goto fail;
	if (erista && (max7762x_regulator_set_voltage(REGULATOR_LDO7, 1050000) ||
		max7762x_regulator_enable(REGULATOR_LDO7, true)))
		goto fail;
	if (!_plle_enable())
		goto fail;

	if (!erista)
	{
		// NVIDIA's Tegra210B01 PCIe pad probe enables "uphy_mgmt"
		// before releasing the PHY reset. Calibration needs this 102 MHz
		// clock in addition to the PLLE reference used by the analog PLL.
		CLOCK(CLK_RST_CONTROLLER_PLLP_MISC1) |= PLLP_HSIO_GATE | PLLP_XUSB_GATE;
		CLOCK(CLK_RST_CONTROLLER_PEX_SATA_USB_RX_BYP) =
			(saved_uphy_mgmt & ~(UPHY_MGMT_DIV_MASK | UPHY_MGMT_GATE)) | UPHY_MGMT_DIV_102M;
		usleep(1);
		CLOCK(CLK_RST_CONTROLLER_PEX_SATA_USB_RX_BYP) |= UPHY_MGMT_GATE;
		usleep(2);
	}
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_CLR) = BIT(CLK_Y_PEX_USB_UPHY);
	usleep(2);
	if (erista)
	{
		XUSB_PADCTL(UPHY_PLL_CTL2) = (XUSB_PADCTL(UPHY_PLL_CTL2) & ~0xFFFFFF0) | (0x136 << 4);
		XUSB_PADCTL(UPHY_PLL_CTL5) = (XUSB_PADCTL(UPHY_PLL_CTL5) & ~(0xFF << 16)) | (0x2A << 16);
	}
	else
		_mariko_calibration();
	XUSB_PADCTL(UPHY_PLL_CTL1) |= PLL_PWR_OVRD;
	XUSB_PADCTL(UPHY_PLL_CTL2) |= PLL_CAL_OVRD;
	XUSB_PADCTL(UPHY_PLL_CTL8) |= PLL_RCAL_OVRD;
	XUSB_PADCTL(UPHY_PLL_CTL4) = (XUSB_PADCTL(UPHY_PLL_CTL4) & ~((3 << 12) | (0xF << 4))) | (2 << 12) | BIT(15);
	XUSB_PADCTL(UPHY_PLL_CTL1) = (XUSB_PADCTL(UPHY_PLL_CTL1) &
		~((3 << 16) | (0xFF << 20) | PLL_IDDQ | PLL_SLEEP_MASK)) | (0x19 << 20);
	usleep(10);
	XUSB_PADCTL(UPHY_PLL_CTL4) |= BIT(8);
	XUSB_PADCTL(UPHY_PLL_CTL2) |= PLL_CAL_EN;
	phy_result = XUSB_PHY_CAL_START_FAILED;
	if (!_pad_wait(UPHY_PLL_CTL2, PLL_CAL_DONE, PLL_CAL_DONE))
		goto fail;
	XUSB_PADCTL(UPHY_PLL_CTL2) &= ~PLL_CAL_EN;
	phy_result = XUSB_PHY_CAL_STOP_FAILED;
	if (!_pad_wait(UPHY_PLL_CTL2, PLL_CAL_DONE, 0))
		goto fail;
	XUSB_PADCTL(UPHY_PLL_CTL1) |= PLL_ENABLE;
	phy_result = XUSB_PHY_UPHY_LOCK_FAILED;
	if (!_pad_wait(UPHY_PLL_CTL1, PLL_LOCK, PLL_LOCK))
		goto fail;
	XUSB_PADCTL(UPHY_PLL_CTL8) |= PLL_RCAL_EN | PLL_RCAL_CLK_EN;
	phy_result = XUSB_PHY_RCAL_START_FAILED;
	if (!_pad_wait(UPHY_PLL_CTL8, PLL_RCAL_DONE, PLL_RCAL_DONE))
		goto fail;
	XUSB_PADCTL(UPHY_PLL_CTL8) &= ~PLL_RCAL_EN;
	phy_result = XUSB_PHY_RCAL_STOP_FAILED;
	if (!_pad_wait(UPHY_PLL_CTL8, PLL_RCAL_DONE, 0))
		goto fail;
	XUSB_PADCTL(UPHY_PLL_CTL8) &= ~PLL_RCAL_CLK_EN;
	// Keep the PLL under software control for the bounded UMS session.
	// This permits explicit shutdown instead of handing it to the OS sequencer.
	u32 shift = 12 + phy_lane * 2;
	XUSB_PADCTL(USB3_PAD_MUX) = (XUSB_PADCTL(USB3_PAD_MUX) & ~(3 << shift)) |
		BIT(shift) | BIT(1 + phy_lane);
	XUSB_PADCTL(XUSB_PADCTL_SS_PORT_MAP) &= ~0x1F; // External SS0 -> USB2 port 0.
	XUSB_PADCTL(USB3_ECTL1) = (XUSB_PADCTL(USB3_ECTL1) & ~(3 << 16)) | (2 << 16);
	XUSB_PADCTL(USB3_ECTL2) = (XUSB_PADCTL(USB3_ECTL2) & ~0xFFFF) | 0xFC;
	XUSB_PADCTL(USB3_ECTL3) = 0xC0077F1F;
	XUSB_PADCTL(USB3_ECTL4) = (XUSB_PADCTL(USB3_ECTL4) & ~(0xFFFFu << 16)) | (0x1C7 << 16);
	XUSB_PADCTL(USB3_ECTL6) = 0xFCF01368;
	if (!erista)
	{
		// Tegra210B01 pad protection workaround from the NVIDIA driver.
		for (u32 port = 0; port < 4; port++)
			XUSB_PADCTL(0x84 + port * 0x40) |= BIT(6);
	}
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) &= ~AUX_CLAMP;
	usleep(100);
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) &= ~AUX_CLAMP_EARLY;
	usleep(100);
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) &= ~AUX_VCORE_DOWN;
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) &= ~SS_VCORE_DOWN;
	usleep(100);
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) &= ~SS_CLAMP_EARLY;
	usleep(100);
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) &= ~SS_CLAMP;
	phy_result = XUSB_PHY_READY;
	return true;

fail:
	xusb_phy_superspeed_end();
	return false;
}

void xusb_phy_superspeed_end(void)
{
	if (!phy_owned)
		return;
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) |= SS_CLAMP_EARLY;
	usleep(100);
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) |= SS_CLAMP;
	usleep(250);
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) |= SS_VCORE_DOWN;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_SET) = BIT(CLK_Y_PEX_USB_UPHY);
	CLOCK(CLK_RST_CONTROLLER_PLLE_BASE) &= ~PLLE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLE_MISC) |= PLLE_IDDQ_SWCTL | PLLE_IDDQ;
	usleep(1);
	XUSB_PADCTL(USB3_PAD_MUX) = saved_mux;
	XUSB_PADCTL(XUSB_PADCTL_SS_PORT_MAP) = saved_map;
	XUSB_PADCTL(XUSB_PADCTL_ELPG_PROGRAM_1) = saved_elpg;
	CLOCK(CLK_RST_CONTROLLER_PLLE_BASE) = saved_plle_base;
	CLOCK(CLK_RST_CONTROLLER_PLLE_MISC) = saved_plle_misc;
	CLOCK(CLK_RST_CONTROLLER_PLLE_AUX) = saved_plle_aux;
	CLOCK(PLLE_SS_CTRL) = saved_plle_ss;
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_LDO1_CFG, saved_ldo1);
	if (phy_lane == 6)
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_LDO7_CFG, saved_ldo7);
	else
	{
		for (u32 port = 0; port < 4; port++)
			XUSB_PADCTL(0x84 + port * 0x40) = saved_pad_protection[port];
		// Restore the management clock while the UPHY remains in reset.
		CLOCK(CLK_RST_CONTROLLER_PEX_SATA_USB_RX_BYP) = saved_uphy_mgmt;
		CLOCK(CLK_RST_CONTROLLER_PLLP_MISC1) =
			(CLOCK(CLK_RST_CONTROLLER_PLLP_MISC1) & ~(PLLP_HSIO_GATE | PLLP_XUSB_GATE)) | saved_pllp_gates;
	}
	if (!saved_uphy_reset)
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_Y_CLR) = BIT(CLK_Y_PEX_USB_UPHY);
	phy_owned = false;
}
