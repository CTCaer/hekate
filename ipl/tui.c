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

	gfx_clear(con->gfx_ctxt, 0xFF000000);

	while (1)
	{
		gfx_con_setcol(con, 0xFFFFFFFF, 1, 0xFF000000);
		gfx_con_setpos(con, menu->x, menu->y);
		gfx_printf(con, "[%s]\n\n", menu->caption);

		for (cnt = 0; menu->ents[cnt].type != MENT_END; cnt++)
		{
			if (cnt == idx)
				gfx_con_setcol(con, 0xFF000000, 1, 0xFFCCCCCC);
			else
				gfx_con_setcol(con, 0xFFFFFFFF, 1, 0xFF000000);
			con->x += 8;
			gfx_printf(con, "%s", menu->ents[cnt].caption);
			if(menu->ents[cnt].type == MENT_MENU)
				gfx_printf(con, "%k...", 0xFFEE9900);
			gfx_putc(con, '\n');
		}

		gfx_con_setcol(con, 0xFFFFFFFF, 1, 0xFF000000);
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
			}
			gfx_clear(con->gfx_ctxt, 0xFF000000);
		}
	}

	return NULL;
}
