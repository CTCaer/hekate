/*{
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

#include "tui.h"
#include "btn.h"

void tui_pbar(gfx_con_t *con, int x, int y, u32 val)
{
	u32 cx, cy;

	gfx_con_getpos(con, &cx, &cy);

	gfx_con_setpos(con, x, y);

	gfx_printf(con, "[%3d%%]", val);

	x += 7 * 8;
	
	for (int i = 0; i < 6; i++)
	{
		gfx_line(con->gfx_ctxt, x, y + i + 1, x + 3 * val, y + i + 1, 0xFFFFFFFF);
		gfx_line(con->gfx_ctxt, x + 3 * val, y + i + 1, x + 3 * 100, y + i + 1, 0xFF888888);
	}

	gfx_con_setpos(con, cx, cy);
}

void *tui_do_menu(gfx_con_t *con, menu_t *menu)
{
	int idx = 0, cnt;
	int prev_idx = 0;

	gfx_clear(con->gfx_ctxt, 0xFF1B1B1B);

	while (1)
	{
		gfx_con_setcol(con, 0xFFCCCCCC, 1, 0xFF1B1B1B);
		gfx_con_setpos(con, menu->x, menu->y);
		gfx_printf(con, "[%s]\n\n", menu->caption);

		// Skip caption or seperator lines selection
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

		// Draw the menu
		for (cnt = 0; menu->ents[cnt].type != MENT_END; cnt++)
		{
			if (cnt == idx)
				gfx_con_setcol(con, 0xFF1B1B1B, 1, 0xFFCCCCCC);
			else
				gfx_con_setcol(con, 0xFFCCCCCC, 1, 0xFF1B1B1B);
			if (cnt != idx && menu->ents[cnt].type == MENT_CAPTION)
				gfx_printf(con, "%k %s", menu->ents[cnt].color, menu->ents[cnt].caption);
			else
				gfx_printf(con, " %s", menu->ents[cnt].caption);
			if(menu->ents[cnt].type == MENT_MENU)
				gfx_printf(con, "%k...", 0xFFEE9900);
			gfx_printf(con, " \n");
		}
		gfx_con_setcol(con, 0xFFCCCCCC, 1, 0xFF1B1B1B);
		gfx_putc(con, '\n');

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
			gfx_clear(con->gfx_ctxt, 0xFF1B1B1B);
		}
	}

	return NULL;
}
