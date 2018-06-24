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

#include "tui.h"
#include "btn.h"
#include "max17050.h"

#ifdef MENU_LOGO_ENABLE
extern u8 *Kc_MENU_LOGO;
#define X_MENU_LOGO       158
#define Y_MENU_LOGO        76
#define X_POS_MENU_LOGO   538
#define Y_POS_MENU_LOGO  1180
#endif //MENU_LOGO_ENABLE

void tui_pbar(gfx_con_t *con, int x, int y, u32 val, u32 fgcol, u32 bgcol)
{
	u32 cx, cy;

	gfx_con_getpos(con, &cx, &cy);

	gfx_con_setpos(con, x, y);

	gfx_printf(con, "%k[%3d%%]%k", fgcol, val, 0xFFCCCCCC);

	x += 7 * 8;
	
	for (int i = 0; i < 6; i++)
	{
		gfx_line(con->gfx_ctxt, x, y + i + 1, x + 3 * val, y + i + 1, fgcol);
		gfx_line(con->gfx_ctxt, x + 3 * val, y + i + 1, x + 3 * 100, y + i + 1, bgcol);
	}

	gfx_con_setpos(con, cx, cy);
}

void *tui_do_menu(gfx_con_t *con, menu_t *menu)
{
	int idx = 0, prev_idx = 0, cnt = 0x7FFFFFFF;
	u32 battPercent = 0;
	int battVoltCurr = 0;

	gfx_clear_grey(con->gfx_ctxt, 0x1B);
#ifdef MENU_LOGO_ENABLE
	gfx_set_rect_rgb(con->gfx_ctxt, Kc_MENU_LOGO,
		X_MENU_LOGO, Y_MENU_LOGO, X_POS_MENU_LOGO, Y_POS_MENU_LOGO);
#endif //MENU_LOGO_ENABLE

	while (1)
	{
		gfx_con_setcol(con, 0xFFCCCCCC, 1, 0xFF1B1B1B);
		gfx_con_setpos(con, menu->x, menu->y);
		gfx_printf(con, "[%s]\n\n", menu->caption);

		// Skip caption or seperator lines selection.
		while (menu->ents[idx].type == MENT_CAPTION ||
			menu->ents[idx].type == MENT_CHGLINE)
		{
			if (prev_idx <= idx || (!idx && prev_idx == cnt - 1))
			{
				idx++;
				if (idx > (cnt - 1))
				{
					idx = 0;
					prev_idx = 0;
				}
			}
			else
			{
				idx--;
				if (idx < 0)
				{
					idx = cnt - 1;
					prev_idx = cnt;
				}
			}
		}
		prev_idx = idx;

		// Draw the menu.
		for (cnt = 0; menu->ents[cnt].type != MENT_END; cnt++)
		{
			if (cnt == idx)
				gfx_con_setcol(con, 0xFF1B1B1B, 1, 0xFFCCCCCC);
			else
				gfx_con_setcol(con, 0xFFCCCCCC, 1, 0xFF1B1B1B);
			if (menu->ents[cnt].type == MENT_CAPTION)
				gfx_printf(con, "%k %s", menu->ents[cnt].color, menu->ents[cnt].caption);
			else if (menu->ents[cnt].type != MENT_CHGLINE)
				gfx_printf(con, " %s", menu->ents[cnt].caption);
			if(menu->ents[cnt].type == MENT_MENU)
				gfx_printf(con, "%k...", 0xFF0099EE);
			gfx_printf(con, " \n");
		}
		gfx_con_setcol(con, 0xFFCCCCCC, 1, 0xFF1B1B1B);
		gfx_putc(con, '\n');

		// Print help and battery status.
		gfx_con_getpos(con, &con->savedx,  &con->savedy);
		gfx_con_setpos(con, 0,  74 * 16);
		gfx_printf(con, "%k VOL: Move up/down\n PWR: Select option\n\n", 0xFF555555);
			
		max17050_get_property(MAX17050_RepSOC, (int *)&battPercent);
		gfx_printf(con, " %d.%d%%", (battPercent >> 8) & 0xFF, (battPercent & 0xFF) / 26);
		max17050_get_property(MAX17050_VCELL, &battVoltCurr);
		gfx_printf(con, " (%d mV)         ", battVoltCurr);
		max17050_get_property(MAX17050_AvgCurrent, &battVoltCurr);
		if (battVoltCurr > 0)
			gfx_printf(con, "\n %kCharging:%k %d mA         %k\n",
				0xFF008000, 0xFF555555, battVoltCurr / 1000, 0xFFCCCCCC);
		else
			gfx_printf(con, "\n %kDischarging:%k -%d mA     %k\n",
				0xFF800000, 0xFF555555, (~battVoltCurr) / 1000, 0xFFCCCCCC);
		gfx_con_setpos(con, con->savedx,  con->savedy);

		// Wait for user command.
		u32 btn = btn_wait();

		if (btn & BTN_VOL_DOWN && idx < (cnt - 1))
			idx++;
		else if (btn & BTN_VOL_DOWN && idx == (cnt - 1))
			idx = 0;
		if (btn & BTN_VOL_UP && idx > 0)
			idx--;
		else if (btn & BTN_VOL_UP && idx == 0)
			idx = cnt - 1;
		if (btn & BTN_POWER)
		{
			ment_t *ent = &menu->ents[idx];
			switch (ent->type)
			{
			case MENT_HANDLER:
				ent->handler(ent->data);
				break;
			case MENT_MENU:
				return tui_do_menu(con, ent->menu);
				break;
			case MENT_CHOICE:
				return ent->data;
				break;
			case MENT_BACK:
				return NULL;
				break;
			default:
				break;
			}
			gfx_clear_grey(con->gfx_ctxt, 0x1B);
#ifdef MENU_LOGO_ENABLE
			gfx_set_rect_rgb(con->gfx_ctxt, Kc_MENU_LOGO,
				X_MENU_LOGO, Y_MENU_LOGO, X_POS_MENU_LOGO, Y_POS_MENU_LOGO);
#endif //MENU_LOGO_ENABLE
		}
	}

	return NULL;
}
