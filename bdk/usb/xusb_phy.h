/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _XUSB_PHY_H_
#define _XUSB_PHY_H_

#include <utils/types.h>

typedef enum {
	XUSB_PHY_NOT_ATTEMPTED,
	XUSB_PHY_PLLE_SEQUENCER_ACTIVE,
	XUSB_PHY_PLLE_ALREADY_ENABLED,
	XUSB_PHY_UPHY_SEQUENCER_ACTIVE,
	XUSB_PHY_POWER_FAILED,
	XUSB_PHY_UNSUPPORTED_OSC,
	XUSB_PHY_PLLE_LOCK_FAILED,
	XUSB_PHY_CAL_START_FAILED,
	XUSB_PHY_CAL_STOP_FAILED,
	XUSB_PHY_UPHY_LOCK_FAILED,
	XUSB_PHY_RCAL_START_FAILED,
	XUSB_PHY_RCAL_STOP_FAILED,
	XUSB_PHY_READY
} xusb_phy_result_t;

bool xusb_phy_superspeed_init(void);
void xusb_phy_superspeed_end(void);
xusb_phy_result_t xusb_phy_superspeed_result(void);
const char *xusb_phy_superspeed_status(void);

#endif
