/*
 * Copyright (c) 2018 naehrwert
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

#ifndef _PINMUX_H_
#define _PINMUX_H_

#include <utils/types.h>

/*! APB MISC registers. */
#define APB_MISC_GP_SDMMC1_CLK_LPBK_CONTROL 0x8D4
#define APB_MISC_GP_SDMMC3_CLK_LPBK_CONTROL 0x8D8
#define APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL   0xA98
#define APB_MISC_GP_VGPIO_GPIO_MUX_SEL      0xB74

/*! Pinmux registers. */
#define PINMUX_AUX_SDMMC1_CLK      0x00
#define PINMUX_AUX_SDMMC1_CMD      0x04
#define PINMUX_AUX_SDMMC1_DAT3     0x08
#define PINMUX_AUX_SDMMC1_DAT2     0x0C
#define PINMUX_AUX_SDMMC1_DAT1     0x10
#define PINMUX_AUX_SDMMC1_DAT0     0x14
#define PINMUX_AUX_SDMMC3_CLK      0x1C
#define PINMUX_AUX_SDMMC3_CMD      0x20
#define PINMUX_AUX_SDMMC3_DAT0     0x24
#define PINMUX_AUX_SDMMC3_DAT1     0x28
#define PINMUX_AUX_SDMMC3_DAT2     0x2C
#define PINMUX_AUX_SDMMC3_DAT3     0x30
#define PINMUX_AUX_SATA_LED_ACTIVE 0x4C
#define PINMUX_AUX_DMIC3_CLK       0xB4
#define PINMUX_AUX_DMIC3_DAT       0xB8
#define PINMUX_AUX_CAM_I2C_SCL     0xD4
#define PINMUX_AUX_CAM_I2C_SDA     0xD8
#define PINMUX_AUX_UART2_TX        0xF4
#define PINMUX_AUX_UART3_TX        0x104
#define PINMUX_AUX_DAP4_DIN        0x148
#define PINMUX_AUX_DAP4_SCLK       0x150
#define PINMUX_AUX_GPIO_X1_AUD     0x18C
#define PINMUX_AUX_GPIO_X3_AUD     0x190
#define PINMUX_AUX_SPDIF_IN        0x1A4
#define PINMUX_AUX_USB_VBUS_EN0    0x1A8
#define PINMUX_AUX_USB_VBUS_EN1    0x1AC
#define PINMUX_AUX_WIFI_EN         0x1B4
#define PINMUX_AUX_WIFI_RST        0x1B8
#define PINMUX_AUX_AP_WAKE_NFC     0x1CC
#define PINMUX_AUX_NFC_EN          0x1D0
#define PINMUX_AUX_NFC_INT         0x1D4
#define PINMUX_AUX_CAM1_PWDN       0x1EC
#define PINMUX_AUX_CAM2_PWDN       0x1F0
#define PINMUX_AUX_LCD_BL_PWM      0x1FC
#define PINMUX_AUX_LCD_BL_EN       0x200
#define PINMUX_AUX_LCD_RST         0x204
#define PINMUX_AUX_LCD_GPIO1       0x208
#define PINMUX_AUX_LCD_GPIO2       0x20C
#define PINMUX_AUX_TOUCH_INT       0x220
#define PINMUX_AUX_MOTION_INT      0x224
#define PINMUX_AUX_BUTTON_HOME     0x240
#define PINMUX_AUX_GPIO_PE6        0x248
#define PINMUX_AUX_GPIO_PH6        0x250
#define PINMUX_AUX_GPIO_PK3        0x260
#define PINMUX_AUX_GPIO_PZ1        0x280
/* Only in T210B01 */
#define PINMUX_AUX_SDMMC2_DAT0     0x294
#define PINMUX_AUX_SDMMC2_DAT1     0x298
#define PINMUX_AUX_SDMMC2_DAT2     0x29C
#define PINMUX_AUX_SDMMC2_DAT3     0x2A0
#define PINMUX_AUX_SDMMC2_DAT4     0x2A4
#define PINMUX_AUX_SDMMC2_DAT5     0x2A8
#define PINMUX_AUX_SDMMC2_DAT6     0x2AC
#define PINMUX_AUX_SDMMC2_DAT7     0x2B0
#define PINMUX_AUX_SDMMC2_CLK      0x2B4
#define PINMUX_AUX_SDMMC2_CMD      0x2BC

/*! 0:UART-A, 1:UART-B, 3:UART-C, 3:UART-D */
#define PINMUX_AUX_UARTX_TX(x)  (0xE4 + 0x10 * (x))
#define PINMUX_AUX_UARTX_RX(x)  (0xE8 + 0x10 * (x))
#define PINMUX_AUX_UARTX_RTS(x) (0xEC + 0x10 * (x))
#define PINMUX_AUX_UARTX_CTS(x) (0xF0 + 0x10 * (x))
/*! 0:GEN1, 1:GEN2, 2:GEN3, 3:CAM, 4:PWR */
#define PINMUX_AUX_X_I2C_SCL(x) (0xBC + 8 * (x))
#define PINMUX_AUX_X_I2C_SDA(x) (0xC0 + 8 * (x))

#define PINMUX_FUNC_MASK    (3 << 0)

#define PINMUX_PULL_MASK    (3 << 2)
#define PINMUX_PULL_NONE    (0 << 2)
#define PINMUX_PULL_DOWN    (1 << 2)
#define PINMUX_PULL_UP      (2 << 2)

#define PINMUX_TRISTATE     BIT(4)
#define PINMUX_PARKED       BIT(5)
#define PINMUX_INPUT_ENABLE BIT(6)
#define PINMUX_LOCK         BIT(7)
#define PINMUX_LPDR         BIT(8)
#define PINMUX_HSM          BIT(9)

#define PINMUX_IO_HV        BIT(10)
#define PINMUX_OPEN_DRAIN   BIT(11)
#define PINMUX_SCHMT        BIT(12)

#define PINMUX_DRIVE_MASK   (3 << 13)
#define PINMUX_DRIVE_1X     (0 << 13)
#define PINMUX_DRIVE_2X     (1 << 13)
#define PINMUX_DRIVE_3X     (2 << 13)
#define PINMUX_DRIVE_4X     (3 << 13)

void pinmux_config_uart(u32 idx);
void pinmux_config_i2c(u32 idx);

#endif
