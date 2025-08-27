/*
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

#include <stdlib.h>

#include <bdk.h>

#include "gui.h"
#include "gui_emummc_tools.h"
#include "gui_tools.h"
#include "gui_info.h"
#include "gui_options.h"
#include <libs/lvgl/lv_themes/lv_theme_hekate.h>
#include <libs/lvgl/lvgl.h>
#include "../gfx/logos-gui.h"

#include "../config.h"
#include <libs/fatfs/ff.h>

extern volatile boot_cfg_t *b_cfg;
extern volatile nyx_storage_t *nyx_str;

extern lv_res_t launch_payload(lv_obj_t *list);

static bool disp_init_done = false;
static bool do_auto_reload = false;

lv_style_t hint_small_style;
lv_style_t hint_small_style_white;
lv_style_t monospace_text;

lv_obj_t *payload_list;
lv_obj_t *autorcm_btn;
lv_obj_t *close_btn;

lv_img_dsc_t *icon_switch;
lv_img_dsc_t *icon_payload;
lv_img_dsc_t *icon_lakka;

lv_img_dsc_t *hekate_bg;

lv_style_t btn_transp_rel, btn_transp_pr, btn_transp_tgl_rel, btn_transp_tgl_pr;
lv_style_t ddlist_transp_bg, ddlist_transp_sel;
lv_style_t tabview_btn_pr, tabview_btn_tgl_pr;

lv_style_t mbox_darken;

char *text_color;

typedef struct _jc_lv_driver_t
{
	lv_indev_t *indev_jc;
	lv_indev_t *indev_touch;
// LV_INDEV_READ_PERIOD * JC_CAL_MAX_STEPS = 264 ms.
#define JC_CAL_MAX_STEPS 8
	u32 calibration_step;
	u16 cx_max;
	u16 cx_min;
	u16 cy_max;
	u16 cy_min;
	s16 pos_x;
	s16 pos_y;
	s16 pos_last_x;
	s16 pos_last_y;
	lv_obj_t *cursor;
	u32 cursor_timeout;
	bool cursor_hidden;
	u32 console_timeout;
} jc_lv_driver_t;

static jc_lv_driver_t jc_drv_ctx;

gui_status_bar_ctx status_bar;

static void _nyx_disp_init()
{
	vic_surface_t vic_sfc;
	vic_sfc.src_buf  = NYX_FB2_ADDRESS;
	vic_sfc.dst_buf  = NYX_FB_ADDRESS;
	vic_sfc.width    = 1280;
	vic_sfc.height   = 720;
	vic_sfc.pix_fmt  = VIC_PIX_FORMAT_X8R8G8B8;
	vic_sfc.rotation = VIC_ROTATION_270;

	// Set hardware rotation via VIC.
	vic_init();
	vic_set_surface(&vic_sfc);

	// Turn off backlight to hide the transition.
	display_backlight_brightness(0, 1000);

	// Rotate and copy the first frame.
	vic_compose();

	// Switch to new window configuration.
	display_init_window_a_pitch_vic();

	// Enable logging on window D.
	display_init_window_d_console();

	// Switch back the backlight.
	display_backlight_brightness(h_cfg.backlight - 20, 1000);
}

static void _save_log_to_bmp(char *fname)
{
	u32 *fb_ptr = (u32 *)LOG_FB_ADDRESS;

	// Check if there's log written.
	bool log_changed = false;
	for (u32 i = 0; i < 0xCD000; i++)
	{
		if (fb_ptr[i] != 0)
		{
			log_changed = true;
			break;
		}
	}

	if (!log_changed)
		return;

	const u32 file_size = LOG_FB_SZ + 0x36;
	u8 *bitmap = malloc(file_size);

	// Reconstruct FB for bottom-top, landscape bmp. Rotation: 656x1280 -> 1280x656.
	u32 *fb = malloc(LOG_FB_SZ);
	for (int x = 1279; x > - 1; x--)
	{
		for (int y = 655; y > -1; y--)
			fb[y * 1280 + x] = *fb_ptr++;
	}

	manual_system_maintenance(true);

	memcpy(bitmap + 0x36, fb, LOG_FB_SZ);

	typedef struct _bmp_t
	{
		u16 magic;
		u32 size;
		u32 rsvd;
		u32 data_off;
		u32 hdr_size;
		u32 width;
		u32 height;
		u16 planes;
		u16 pxl_bits;
		u32 comp;
		u32 img_size;
		u32 res_h;
		u32 res_v;
		u64 rsvd2;
	} __attribute__((packed)) bmp_t;

	bmp_t *bmp = (bmp_t *)bitmap;

	bmp->magic    = 0x4D42;
	bmp->size     = file_size;
	bmp->rsvd     = 0;
	bmp->data_off = 0x36;
	bmp->hdr_size = 40;
	bmp->width    = 1280;
	bmp->height   = 656;
	bmp->planes   = 1;
	bmp->pxl_bits = 32;
	bmp->comp     = 0;
	bmp->img_size = LOG_FB_SZ;
	bmp->res_h    = 2834;
	bmp->res_v    = 2834;
	bmp->rsvd2    = 0;

	char path[0x80];
	strcpy(path, "bootloader/screenshots");
	s_printf(path + strlen(path), "/nyx%s_log.bmp", fname);
	sd_save_to_file(bitmap, file_size, path);

	free(bitmap);
	free(fb);
}

static void _save_fb_to_bmp()
{
	// Disallow screenshots if less than 2s passed.
	static u32 timer = 0;
	if (get_tmr_ms() < timer)
		return;

	if (do_auto_reload)
		goto exit;

	// Invalidate data.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, false);

	const u32 file_size = NYX_FB_SZ + 0x36;
	u8 *bitmap = malloc(file_size);
	u32 *fb = malloc(NYX_FB_SZ);
	u32 *fb_ptr = (u32 *)NYX_FB2_ADDRESS;
	u32 line_bytes = 1280 * sizeof(u32);

	// Reconstruct FB for bottom-top, landscape bmp. No rotation.
	for (int y = 719; y > -1; y--)
	{
		memcpy(&fb[y * 1280], fb_ptr, line_bytes);
		fb_ptr += line_bytes / sizeof(u32);
	}

	// Create notification box.
	lv_obj_t * mbox = lv_mbox_create(lv_layer_top(), NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_mbox_set_text(mbox, SYMBOL_CAMERA"  #FFDD00 Saving screenshot...#");
	lv_obj_set_width(mbox, LV_DPI * 4);
	lv_obj_set_top(mbox, true);
	lv_obj_align(mbox, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

	// Capture effect.
	display_backlight_brightness(255, 100);
	msleep(150);
	display_backlight_brightness(h_cfg.backlight - 20, 100);

	manual_system_maintenance(true);

	memcpy(bitmap + 0x36, fb, NYX_FB_SZ);

	typedef struct _bmp_t
	{
		u16 magic;
		u32 size;
		u32 rsvd;
		u32 data_off;
		u32 hdr_size;
		u32 width;
		u32 height;
		u16 planes;
		u16 pxl_bits;
		u32 comp;
		u32 img_size;
		u32 res_h;
		u32 res_v;
		u64 rsvd2;
	} __attribute__((packed)) bmp_t;

	bmp_t *bmp = (bmp_t *)bitmap;

	bmp->magic    = 0x4D42;
	bmp->size     = file_size;
	bmp->rsvd     = 0;
	bmp->data_off = 0x36;
	bmp->hdr_size = 40;
	bmp->width    = 1280;
	bmp->height   = 720;
	bmp->planes   = 1;
	bmp->pxl_bits = 32;
	bmp->comp     = 0;
	bmp->img_size = NYX_FB_SZ;
	bmp->res_h    = 2834;
	bmp->res_v    = 2834;
	bmp->rsvd2    = 0;

	sd_mount();

	char path[0x80];

	strcpy(path, "bootloader");
	f_mkdir(path);
	strcat(path, "/screenshots");
	f_mkdir(path);

	// Create date/time name.
	char fname[32];
	rtc_time_t time;
	max77620_rtc_get_time_adjusted(&time);
	s_printf(fname, "%04d%02d%02d_%02d%02d%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
	s_printf(path + strlen(path), "/nyx%s.bmp", fname);

	// Save screenshot and log.
	int res = sd_save_to_file(bitmap, file_size, path);
	if (!res)
		_save_log_to_bmp(fname);

	sd_unmount();

	free(bitmap);
	free(fb);

	if (!res)
		lv_mbox_set_text(mbox, SYMBOL_CAMERA"  #96FF00 Screenshot saved!#");
	else
		lv_mbox_set_text(mbox, SYMBOL_WARNING"  #FFDD00 Screenshot failed!#");
	manual_system_maintenance(true);
	lv_mbox_start_auto_close(mbox, 4000);

exit:
	// Set timer to 2s.
	timer = get_tmr_ms() + 2000;
}

static void _disp_fb_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_p)
{
	// Draw to intermediate non-rotated framebuffer.
	gfx_set_rect_pitch((u32 *)NYX_FB2_ADDRESS, (u32 *)color_p, 1280, x1, y1, x2, y2);

	// Rotate and copy to visible framebuffer.
	if (disp_init_done)
		vic_compose();

	// Check if display init was done. If it's the first big draw, init.
	if (!disp_init_done && ((x2 - x1 + 1) > 600))
	{
		disp_init_done = true;
		_nyx_disp_init();
	}

	lv_flush_ready();
}

static touch_event touchpad;
static bool touch_enabled;
static bool console_enabled = false;

static bool _fts_touch_read(lv_indev_data_t *data)
{
	if (touch_enabled)
		touch_poll(&touchpad);
	else
		return false;

	// Take a screenshot if 3 fingers.
	if (touchpad.fingers > 2)
	{
		_save_fb_to_bmp();

		data->state = LV_INDEV_STATE_REL;
		return false;
	}

	if (console_enabled)
	{
		// Print input debugging in console.
		gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy, &gfx_con.savedcol);
		gfx_con_setpos(32, 638, GFX_COL_AUTO);
		gfx_con.fntsz = 8;
		gfx_printf("x: %4d, y: %4d | z: %3d | ", touchpad.x, touchpad.y, touchpad.z);
		gfx_printf("1: %02X, 2: %02X, 3: %02X, ", touchpad.raw[1], touchpad.raw[2], touchpad.raw[3]);
		gfx_printf("4: %02X, 5: %02X, 6: %02X, 7: %02X",
			touchpad.raw[4], touchpad.raw[5], touchpad.raw[6], touchpad.raw[7]);
		gfx_con_setpos(gfx_con.savedx, gfx_con.savedy, gfx_con.savedcol);
		gfx_con.fntsz = 16;

		return false;
	}

	// Always set touch points.
	data->point.x = touchpad.x;
	data->point.y = touchpad.y;

	// Decide touch enable.
	switch (touchpad.type & STMFTS_MASK_EVENT_ID)
	{
	case STMFTS_EV_MULTI_TOUCH_ENTER:
	case STMFTS_EV_MULTI_TOUCH_MOTION:
		data->state = LV_INDEV_STATE_PR;
		break;
	case STMFTS_EV_MULTI_TOUCH_LEAVE:
		data->state = LV_INDEV_STATE_REL;
		break;
	case STMFTS_EV_NO_EVENT:
	default:
		if (touchpad.touch)
			data->state = LV_INDEV_STATE_PR;
		else
			data->state = LV_INDEV_STATE_REL;
		break;
	}

	return false; // No buffering so no more data read.
}

static bool _jc_virt_mouse_read(lv_indev_data_t *data)
{
	// Poll Joy-Con.
	jc_gamepad_rpt_t *jc_pad = joycon_poll();

	if (!jc_pad)
	{
		data->state = LV_INDEV_STATE_REL;
		return false;
	}

	// Take a screenshot if Capture button is pressed.
	if (jc_pad->cap)
	{
		_save_fb_to_bmp();

		data->state = LV_INDEV_STATE_REL;
		return false;
	}

	// Calibrate left stick.
	if (jc_drv_ctx.calibration_step != JC_CAL_MAX_STEPS)
	{
		if (n_cfg.jc_force_right)
		{
			if (jc_pad->conn_r
				&& jc_pad->rstick_x > 0x400 && jc_pad->rstick_y > 0x400
				&& jc_pad->rstick_x < 0xC00 && jc_pad->rstick_y < 0xC00)
			{
				jc_drv_ctx.calibration_step++;
				jc_drv_ctx.cx_max = jc_pad->rstick_x + 0x96;
				jc_drv_ctx.cx_min = jc_pad->rstick_x - 0x96;
				jc_drv_ctx.cy_max = jc_pad->rstick_y + 0x96;
				jc_drv_ctx.cy_min = jc_pad->rstick_y - 0x96;
				jc_drv_ctx.cursor_timeout = 0;
			}
		}
		else if (jc_pad->conn_l
			     && jc_pad->lstick_x > 0x400 && jc_pad->lstick_y > 0x400
			     && jc_pad->lstick_x < 0xC00 && jc_pad->lstick_y < 0xC00)
		{
			jc_drv_ctx.calibration_step++;
			jc_drv_ctx.cx_max = jc_pad->lstick_x + 0x96;
			jc_drv_ctx.cx_min = jc_pad->lstick_x - 0x96;
			jc_drv_ctx.cy_max = jc_pad->lstick_y + 0x96;
			jc_drv_ctx.cy_min = jc_pad->lstick_y - 0x96;
			jc_drv_ctx.cursor_timeout = 0;
		}

		if (jc_drv_ctx.calibration_step != JC_CAL_MAX_STEPS)
		{
			data->state = LV_INDEV_STATE_REL;
			return false;
		}
	}

	// Re-calibrate on disconnection.
	if (n_cfg.jc_force_right && !jc_pad->conn_r)
		jc_drv_ctx.calibration_step = 0;
	else if (!n_cfg.jc_force_right && !jc_pad->conn_l)
		jc_drv_ctx.calibration_step = 0;

	// Set button presses.
	if (jc_pad->a || jc_pad->zl || jc_pad->zr)
		data->state = LV_INDEV_STATE_PR;
	else
		data->state = LV_INDEV_STATE_REL;

	// Enable console.
	if (jc_pad->plus || jc_pad->minus)
	{
		if (((u32)get_tmr_ms() - jc_drv_ctx.console_timeout) > 1000)
		{
			if (!console_enabled)
			{
				display_window_d_console_enable();
				console_enabled = true;
				gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy, &gfx_con.savedcol);
				gfx_con_setpos(964, 630, GFX_COL_AUTO);
				gfx_printf("Press -/+ to close");
				gfx_con_setpos(gfx_con.savedx, gfx_con.savedy, gfx_con.savedcol);
			}
			else
			{
				display_window_d_console_disable();
				console_enabled = false;
			}

			jc_drv_ctx.console_timeout = get_tmr_ms();
		}

		data->state = LV_INDEV_STATE_REL;
		return false;
	}

	if (console_enabled)
	{
		// Print input debugging in console.
		gfx_con_getpos(&gfx_con.savedx, &gfx_con.savedy, &gfx_con.savedcol);
		gfx_con_setpos(32, 630, GFX_COL_AUTO);
		gfx_con.fntsz = 8;
		gfx_printf("x: %4X, y: %4X | rx: %4X, ry: %4X | b: %06X | bt: %d %d",
			jc_pad->lstick_x, jc_pad->lstick_y, jc_pad->rstick_x, jc_pad->rstick_y,
			jc_pad->buttons, jc_pad->batt_info_l, jc_pad->batt_info_r);
		gfx_con_setpos(gfx_con.savedx, gfx_con.savedy, gfx_con.savedcol);
		gfx_con.fntsz = 16;

		data->state = LV_INDEV_STATE_REL;
		return false;
	}

	// Calculate new cursor position.
	if (!n_cfg.jc_force_right)
	{
		// Left stick X.
		if (jc_pad->lstick_x <= jc_drv_ctx.cx_max && jc_pad->lstick_x >= jc_drv_ctx.cx_min)
			jc_drv_ctx.pos_x += 0;
		else if (jc_pad->lstick_x > jc_drv_ctx.cx_max)
			jc_drv_ctx.pos_x += ((jc_pad->lstick_x - jc_drv_ctx.cx_max) / 30);
		else
			jc_drv_ctx.pos_x -= ((jc_drv_ctx.cx_min - jc_pad->lstick_x) / 30);

		// Left stick Y.
		if (jc_pad->lstick_y <= jc_drv_ctx.cy_max && jc_pad->lstick_y >= jc_drv_ctx.cy_min)
			jc_drv_ctx.pos_y += 0;
		else if (jc_pad->lstick_y > jc_drv_ctx.cy_max)
		{
			s16 val = (jc_pad->lstick_y - jc_drv_ctx.cy_max) / 30;
			// Hoag has inverted Y axis.
			if (jc_pad->sio_mode)
				val *= -1;
			jc_drv_ctx.pos_y -= val;
		}
		else
		{
			s16 val = (jc_drv_ctx.cy_min - jc_pad->lstick_y) / 30;
			// Hoag has inverted Y axis.
			if (jc_pad->sio_mode)
				val *= -1;
			jc_drv_ctx.pos_y += val;
		}
	}
	else
	{
		// Right stick X.
		if (jc_pad->rstick_x <= jc_drv_ctx.cx_max && jc_pad->rstick_x >= jc_drv_ctx.cx_min)
			jc_drv_ctx.pos_x += 0;
		else if (jc_pad->rstick_x > jc_drv_ctx.cx_max)
			jc_drv_ctx.pos_x += ((jc_pad->rstick_x - jc_drv_ctx.cx_max) / 30);
		else
			jc_drv_ctx.pos_x -= ((jc_drv_ctx.cx_min - jc_pad->rstick_x) / 30);

		// Right stick Y.
		if (jc_pad->rstick_y <= jc_drv_ctx.cy_max && jc_pad->rstick_y >= jc_drv_ctx.cy_min)
			jc_drv_ctx.pos_y += 0;
		else if (jc_pad->rstick_y > jc_drv_ctx.cy_max)
		{
			s16 val = (jc_pad->rstick_y - jc_drv_ctx.cy_max) / 30;
			// Hoag has inverted Y axis.
			if (jc_pad->sio_mode)
				val *= -1;
			jc_drv_ctx.pos_y -= val;
		}
		else
		{
			s16 val = (jc_drv_ctx.cy_min - jc_pad->rstick_y) / 30;
			// Hoag has inverted Y axis.
			if (jc_pad->sio_mode)
				val *= -1;
			jc_drv_ctx.pos_y += val;
		}
	}

	// Ensure value inside screen limits.
	if (jc_drv_ctx.pos_x < 0)
		jc_drv_ctx.pos_x = 0;
	else if (jc_drv_ctx.pos_x > 1279)
		jc_drv_ctx.pos_x = 1279;

	if (jc_drv_ctx.pos_y < 0)
		jc_drv_ctx.pos_y = 0;
	else if (jc_drv_ctx.pos_y > 719)
		jc_drv_ctx.pos_y = 719;

	// Set cursor position.
	data->point.x = jc_drv_ctx.pos_x;
	data->point.y = jc_drv_ctx.pos_y;

	// Auto hide cursor.
	if (jc_drv_ctx.pos_x != jc_drv_ctx.pos_last_x || jc_drv_ctx.pos_y != jc_drv_ctx.pos_last_y)
	{
		jc_drv_ctx.pos_last_x = jc_drv_ctx.pos_x;
		jc_drv_ctx.pos_last_y = jc_drv_ctx.pos_y;

		jc_drv_ctx.cursor_hidden = false;
		jc_drv_ctx.cursor_timeout = get_tmr_ms();
		lv_indev_set_cursor(jc_drv_ctx.indev_jc, jc_drv_ctx.cursor);

		// Un hide cursor.
		lv_obj_set_opa_scale_enable(jc_drv_ctx.cursor, false);
	}
	else
	{
		if (!jc_drv_ctx.cursor_hidden)
		{
			if (((u32)get_tmr_ms() - jc_drv_ctx.cursor_timeout) > 3000)
			{
				// Remove cursor and hide it.
				lv_indev_set_cursor(jc_drv_ctx.indev_jc, NULL);
				lv_obj_set_opa_scale_enable(jc_drv_ctx.cursor, true);
				lv_obj_set_opa_scale(jc_drv_ctx.cursor, LV_OPA_TRANSP);

				jc_drv_ctx.cursor_hidden = true;
			}
		}
		else
			data->state = LV_INDEV_STATE_REL; // Ensure that no clicks are allowed.
	}

	if (jc_pad->b && close_btn)
	{
		lv_action_t close_btn_action = lv_btn_get_action(close_btn, LV_BTN_ACTION_CLICK);
		close_btn_action(close_btn);
		close_btn = NULL;
	}

	return false; // No buffering so no more data read.
}

typedef struct _system_maintenance_tasks_t
{
	union
	{
		lv_task_t *tasks[2];
		struct
		{
			lv_task_t *status_bar;
			lv_task_t *dram_periodic_comp;
		} task;
	};
} system_maintenance_tasks_t;

static system_maintenance_tasks_t system_tasks;

void manual_system_maintenance(bool refresh)
{
	for (u32 task_idx = 0; task_idx < (sizeof(system_maintenance_tasks_t) / sizeof(lv_task_t *)); task_idx++)
	{
		lv_task_t *task = system_tasks.tasks[task_idx];
		if (task && (lv_tick_elaps(task->last_run) >= task->period))
		{
			task->last_run = lv_tick_get();
			task->task(task->param);
		}
	}
	if (refresh)
		lv_refr_now();
}

lv_img_dsc_t *bmp_to_lvimg_obj(const char *path)
{
	u32 fsize;
	u8 *bitmap = sd_file_read(path, &fsize);
	if (!bitmap)
		return NULL;

	struct _bmp_data
	{
		u32 size;
		u32 size_x;
		u32 size_y;
		u32 offset;
	};

	struct _bmp_data bmpData;

	// Get values manually to avoid unaligned access.
	bmpData.size = bitmap[2] | bitmap[3] << 8 |
		bitmap[4] << 16 | bitmap[5] << 24;
	bmpData.offset = bitmap[10] | bitmap[11] << 8 |
		bitmap[12] << 16 | bitmap[13] << 24;
	bmpData.size_x = bitmap[18] | bitmap[19] << 8 |
		bitmap[20] << 16 | bitmap[21] << 24;
	bmpData.size_y = bitmap[22] | bitmap[23] << 8 |
		bitmap[24] << 16 | bitmap[25] << 24;
	// Sanity check.
	if (bitmap[0] == 'B' &&
		bitmap[1] == 'M' &&
		bitmap[28] == 32 && // Only 32 bit BMPs allowed.
		bmpData.size <= fsize)
	{
		// Check if non-default Bottom-Top.
		bool flipped = false;
		if (bmpData.size_y & 0x80000000)
		{
			bmpData.size_y = ~(bmpData.size_y) + 1;
			flipped = true;
		}

		lv_img_dsc_t *img_desc = (lv_img_dsc_t *)bitmap;
		u32 offset_copy = ALIGN((u32)bitmap + sizeof(lv_img_dsc_t), 0x10);

		img_desc->header.always_zero = 0;
		img_desc->header.w = bmpData.size_x;
		img_desc->header.h = bmpData.size_y;
		img_desc->header.cf = (bitmap[28] == 32) ? LV_IMG_CF_TRUE_COLOR_ALPHA : LV_IMG_CF_TRUE_COLOR; // Only LV_IMG_CF_TRUE_COLOR_ALPHA is actually allowed.
		img_desc->data_size = bmpData.size - bmpData.offset;
		img_desc->data = (u8 *)offset_copy;

		u32 *tmp = malloc(bmpData.size);
		u32 *tmp2 = (u32 *)offset_copy;

		// Copy the unaligned data to an aligned buffer.
		memcpy((u8 *)tmp, bitmap + bmpData.offset, img_desc->data_size);
		u32 j = 0;

		if (!flipped)
		{
			for (u32 y = 0; y < bmpData.size_y; y++)
			{
				for (u32 x = 0; x < bmpData.size_x; x++)
					tmp2[j++] = tmp[(bmpData.size_y - 1 - y ) * bmpData.size_x + x];
			}
		}
		else
		{
			for (u32 y = 0; y < bmpData.size_y; y++)
			{
				for (u32 x = 0; x < bmpData.size_x; x++)
					tmp2[j++] = tmp[y * bmpData.size_x + x];
			}
		}

		free(tmp);
	}
	else
	{
		free(bitmap);
		return NULL;
	}

	return (lv_img_dsc_t *)bitmap;
}

lv_res_t nyx_generic_onoff_toggle(lv_obj_t *btn)
{
	lv_obj_t *label_btn = lv_obj_get_child(btn, NULL);
	lv_obj_t *label_btn2 = lv_obj_get_child(btn, label_btn);

	char label_text[64];
	if (!label_btn2)
	{
		strcpy(label_text, lv_label_get_text(label_btn));
		label_text[strlen(label_text) - 15] = 0;

		if (!(lv_btn_get_state(btn) & LV_BTN_STATE_TGL_REL))
		{
			strcat(label_text, "#D0D0D0    OFF#");
			lv_label_set_text(label_btn, label_text);
		}
		else
		{
			s_printf(label_text, "%s%s%s", label_text, text_color, "    ON #");
			lv_label_set_text(label_btn, label_text);
		}
	}
	else
	{
		if (!(lv_btn_get_state(btn) & LV_BTN_STATE_TGL_REL))
			lv_label_set_text(label_btn, "#D0D0D0 OFF#");
		else
		{
			s_printf(label_text, "%s%s", text_color, " ON #");
			lv_label_set_text(label_btn, label_text);
		}
	}

	return LV_RES_OK;
}

lv_res_t mbox_action(lv_obj_t *btns, const char *txt)
{
	lv_obj_t *mbox = lv_mbox_get_from_btn(btns);
	lv_obj_t *dark_bg = lv_obj_get_parent(mbox);

	lv_obj_del(dark_bg); // Deletes children also (mbox).

	return LV_RES_INV;
}

bool nyx_emmc_check_battery_enough()
{
	if (h_cfg.devmode)
		return true;

	int batt_volt = 0;

	max17050_get_property(MAX17050_VCELL, &batt_volt);

	if (batt_volt && batt_volt < 3650)
	{
		lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
		lv_obj_set_style(dark_bg, &mbox_darken);
		lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

		static const char * mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
		lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
		lv_mbox_set_recolor_text(mbox, true);

		lv_mbox_set_text(mbox,
			"#FF8000 Battery Check#\n\n"
			"#FFDD00 Battery is not enough to carry on#\n"
			"#FFDD00 with selected operation!#\n\n"
			"Charge to at least #C7EA46 3650 mV#, and try again!");

		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
		lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_set_top(mbox, true);

		return false;
	}

	return true;
}

static void _nyx_sd_card_issues_warning(void *param)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);

	lv_mbox_set_text(mbox,
		"#FF8000 SD Card Issues Warning#\n\n"
		"#FFDD00 The SD Card is initialized in 1-bit mode!#\n"
		"#FFDD00 This might mean detached or broken connector!#\n\n"
		"You might want to check\n#C7EA46 Console Info# -> #C7EA46 microSD#");

	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);
}

void nyx_window_toggle_buttons(lv_obj_t *win, bool disable)
{
	lv_win_ext_t * ext = lv_obj_get_ext_attr(win);
	lv_obj_t * hbtn;

	hbtn = lv_obj_get_child_back(ext->header, NULL);
	hbtn = lv_obj_get_child_back(ext->header, hbtn); // Skip the title.

	if (disable)
	{
		while (hbtn != NULL)
		{
			lv_obj_set_opa_scale(hbtn, LV_OPA_40);
			lv_obj_set_opa_scale_enable(hbtn, true);
			lv_obj_set_click(hbtn, false);
			hbtn = lv_obj_get_child_back(ext->header, hbtn);
		}
	}
	else
	{
		while (hbtn != NULL)
		{
			lv_obj_set_opa_scale(hbtn, LV_OPA_COVER);
			lv_obj_set_click(hbtn, true);
			hbtn = lv_obj_get_child_back(ext->header, hbtn);
		}
	}
}

lv_res_t nyx_win_close_action_custom(lv_obj_t * btn)
{
	close_btn = NULL;

	return lv_win_close_action(btn);
}

lv_obj_t *nyx_create_standard_window(const char *win_title)
{
	static lv_style_t win_bg_style;

	lv_style_copy(&win_bg_style, &lv_style_plain);
	win_bg_style.body.main_color = lv_theme_get_current()->bg->body.main_color;
	win_bg_style.body.grad_color = win_bg_style.body.main_color;

	lv_obj_t *win = lv_win_create(lv_scr_act(), NULL);
	lv_win_set_title(win, win_title);
	lv_win_set_style(win, LV_WIN_STYLE_BG, &win_bg_style);
	lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);

	close_btn = lv_win_add_btn(win, NULL, SYMBOL_CLOSE" Close", nyx_win_close_action_custom);

	return win;
}

lv_obj_t *nyx_create_window_custom_close_btn(const char *win_title, lv_action_t rel_action)
{
	static lv_style_t win_bg_style;

	lv_style_copy(&win_bg_style, &lv_style_plain);
	win_bg_style.body.main_color = lv_theme_get_current()->bg->body.main_color;
	win_bg_style.body.grad_color = win_bg_style.body.main_color;

	lv_obj_t *win = lv_win_create(lv_scr_act(), NULL);
	lv_win_set_title(win, win_title);
	lv_win_set_style(win, LV_WIN_STYLE_BG, &win_bg_style);
	lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);

	close_btn = lv_win_add_btn(win, NULL, SYMBOL_CLOSE" Close", rel_action);

	return win;
}

static bool launch_logs_enable = false;

static void _launch_hos(u8 autoboot, u8 autoboot_list)
{
	b_cfg->boot_cfg = BOOT_CFG_AUTOBOOT_EN;
	if (launch_logs_enable)
		b_cfg->boot_cfg |= BOOT_CFG_FROM_LAUNCH;
	b_cfg->autoboot = autoboot;
	b_cfg->autoboot_list = autoboot_list;

	void (*main_ptr)() = (void *)nyx_str->hekate;

	sd_end();

	hw_deinit(false, 0);

	(*main_ptr)();
}

void reload_nyx(lv_obj_t *obj, bool force)
{
	if (!force)
	{
		sd_mount();

		// Check that Nyx still exists.
		if (f_stat("bootloader/sys/nyx.bin", NULL))
		{
			sd_unmount();

			// Remove lvgl object in case of being invoked from a window.
			if (obj)
				lv_obj_del(obj);

			do_auto_reload = false;

			return;
		}
	}

	b_cfg->boot_cfg = BOOT_CFG_AUTOBOOT_EN;
	b_cfg->autoboot = 0;
	b_cfg->autoboot_list = 0;
	b_cfg->extra_cfg = 0;

	void (*main_ptr)() = (void *)nyx_str->hekate;

	sd_end();

	hw_deinit(false, 0);

	(*main_ptr)();
}

static lv_res_t reload_action(lv_obj_t *btns, const char *txt)
{
	if (!lv_btnm_get_pressed(btns))
		reload_nyx(NULL, false);

	return mbox_action(btns, txt);
}

static lv_res_t _removed_sd_action(lv_obj_t *btns, const char *txt)
{
	u32 btnidx = lv_btnm_get_pressed(btns);

	switch (btnidx)
	{
	case 0:
		if (h_cfg.rcm_patched)
			power_set_state(POWER_OFF_REBOOT);
		else
			power_set_state(REBOOT_RCM);
		break;
	case 1:
		power_set_state(POWER_OFF_RESET);
		break;
	case 2:
		sd_end();
		do_auto_reload = false;
		break;
	}

	return mbox_action(btns, txt);
}

static void _check_sd_card_removed(void *params)
{
	static lv_obj_t *dark_bg = NULL;

	// The following checks if SDMMC_1 is initialized.
	// If yes and card was removed, shows a message box,
	// that will reload Nyx, when the card is inserted again.
	if (!do_auto_reload && sd_get_card_removed())
	{
		dark_bg = lv_obj_create(lv_scr_act(), NULL);
		lv_obj_set_style(dark_bg, &mbox_darken);
		lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

		static const char * mbox_btn_map[] = { "\221Reboot (RCM)", "\221Power Off", "\221Do not reload", "" };
		static const char * mbox_btn_map_rcm_patched[] = { "\221Reboot", "\221Power Off", "\221Do not reload", "" };
		lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
		lv_mbox_set_recolor_text(mbox, true);
		lv_obj_set_width(mbox, LV_HOR_RES * 6 / 9);

		lv_mbox_set_text(mbox, "\n#FF8000 SD card was removed!#\n\n#96FF00 Nyx will reload after inserting it.#\n\nReminder that you can use UMS instead of removing it.\n");
		lv_mbox_add_btns(mbox, h_cfg.rcm_patched ? mbox_btn_map_rcm_patched : mbox_btn_map, _removed_sd_action);

		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_set_top(mbox, true);

		do_auto_reload = true;
	}

	// If in reload state and card was inserted, reload nyx.
	if (do_auto_reload && !sd_get_card_removed())
		reload_nyx(dark_bg, false);
}

lv_task_t *task_emmc_errors;
static void _nyx_emmc_issues_warning(void *params)
{
	if (emmc_get_mode() < EMMC_MMC_HS400)
	{
		// Remove task.
		lv_task_del(task_emmc_errors);

		lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
		lv_obj_set_style(dark_bg, &mbox_darken);
		lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

		static const char * mbox_btn_map[] = { "\251", "\222OK", "\251", "" };
		lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
		lv_mbox_set_recolor_text(mbox, true);

		lv_mbox_set_text(mbox,
			"#FF8000 eMMC Issues Warning#\n\n"
			"#FFDD00 Your eMMC is initialized in a slower mode!#\n"
			"#FFDD00 This might mean hardware issues!#\n\n"
			"You might want to check\n#C7EA46 Console Info# -> #C7EA46 eMMC#");

		lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);
		lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_set_top(mbox, true);
	}
}

static lv_res_t _reboot_action(lv_obj_t *btns, const char *txt)
{
	u32 btnidx = lv_btnm_get_pressed(btns);

	switch (btnidx)
	{
	case 0:
		power_set_state(REBOOT_BYPASS_FUSES);
		break;
	case 1:
		if (h_cfg.rcm_patched)
			power_set_state(POWER_OFF_REBOOT);
		else
			power_set_state(REBOOT_RCM);
		break;
	}

	return mbox_action(btns, txt);
}

static lv_res_t _poweroff_action(lv_obj_t *btns, const char *txt)
{
	if (!lv_btnm_get_pressed(btns))
		power_set_state(POWER_OFF_RESET);

	return mbox_action(btns, txt);
}

static lv_res_t _create_mbox_reload(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\221Reload", "\221Cancel", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES * 4 / 10);

	lv_mbox_set_text(mbox, "#FF8000 Do you really want#\n#FF8000 to reload hekate & Nyx?#\n\n"
		"This also checks\n#96FF00 bootloader/update.bin#\nfor hekate updates");

	lv_mbox_add_btns(mbox, mbox_btn_map, reload_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_reboot(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\221OFW", "\221RCM", "\221Cancel", "" };
	static const char * mbox_btn_map_autorcm[] = { "\261OFW", "\221RCM", "\221Cancel", "" };
	static const char * mbox_btn_map_patched[] = { "\221OFW", "\221Normal", "\221Cancel", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 2);

	lv_mbox_set_text(mbox, "#FF8000 Choose where to reboot:#");

	if (h_cfg.rcm_patched)
		lv_mbox_add_btns(mbox, mbox_btn_map_patched, _reboot_action);
	else
		lv_mbox_add_btns(mbox, !h_cfg.autorcm_enabled ? mbox_btn_map : mbox_btn_map_autorcm, _reboot_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _create_mbox_poweroff(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\221Power Off", "\221Cancel", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES * 4 / 10);

	lv_mbox_set_text(mbox, "#FF8000 Do you really want#\n#FF8000 to power off?#");

	lv_mbox_add_btns(mbox, mbox_btn_map, _poweroff_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

void nyx_create_onoff_button(lv_theme_t *th, lv_obj_t *parent, lv_obj_t *btn, const char *btn_name, lv_action_t action, bool transparent)
{
	// Create buttons that are flat and text, plus On/Off switch.
	static lv_style_t btn_onoff_rel_hos_style, btn_onoff_pr_hos_style;
	lv_style_copy(&btn_onoff_rel_hos_style, th->btn.rel);
	btn_onoff_rel_hos_style.body.shadow.width = 0;
	btn_onoff_rel_hos_style.body.border.width = 0;
	btn_onoff_rel_hos_style.body.padding.hor = 0;
	btn_onoff_rel_hos_style.body.radius = 0;
	btn_onoff_rel_hos_style.body.empty = 1;

	lv_style_copy(&btn_onoff_pr_hos_style, &btn_onoff_rel_hos_style);
	if (transparent)
	{
		btn_onoff_pr_hos_style.body.main_color = LV_COLOR_HEX(0xFFFFFF);
		btn_onoff_pr_hos_style.body.opa = 35;
	}
	else
		btn_onoff_pr_hos_style.body.main_color = LV_COLOR_HEX(theme_bg_color ? (theme_bg_color + 0x101010) : 0x2D2D2D);
	btn_onoff_pr_hos_style.body.grad_color = btn_onoff_pr_hos_style.body.main_color;
	btn_onoff_pr_hos_style.text.color = th->btn.pr->text.color;
	btn_onoff_pr_hos_style.body.empty = 0;

	lv_obj_t *label_btn = lv_label_create(btn, NULL);
	lv_obj_t *label_btnsw = NULL;

	lv_label_set_recolor(label_btn, true);
	label_btnsw = lv_label_create(btn, NULL);
	lv_label_set_recolor(label_btnsw, true);
	lv_btn_set_layout(btn, LV_LAYOUT_OFF);

	lv_btn_set_style(btn, LV_BTN_STYLE_REL, &btn_onoff_rel_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_PR, &btn_onoff_pr_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_TGL_REL, &btn_onoff_rel_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_TGL_PR, &btn_onoff_pr_hos_style);

	lv_btn_set_fit(btn, false, true);
	lv_obj_set_width(btn, lv_obj_get_width(parent));
	lv_btn_set_toggle(btn, true);

	lv_label_set_text(label_btn, btn_name);

	lv_label_set_text(label_btnsw, "#D0D0D0 OFF#");
	lv_obj_align(label_btn, btn, LV_ALIGN_IN_LEFT_MID, LV_DPI / 4, 0);
	lv_obj_align(label_btnsw, btn, LV_ALIGN_IN_RIGHT_MID, -LV_DPI / 4, -LV_DPI / 10);

	if (action)
		lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, action);
}

static void _create_text_button(lv_theme_t *th, lv_obj_t *parent, lv_obj_t *btn, const char *btn_name, lv_action_t action)
{
	// Create buttons that are flat and only have a text label.
	static lv_style_t btn_onoff_rel_hos_style, btn_onoff_pr_hos_style;
	lv_style_copy(&btn_onoff_rel_hos_style, th->btn.rel);
	btn_onoff_rel_hos_style.body.shadow.width = 0;
	btn_onoff_rel_hos_style.body.border.width = 0;
	btn_onoff_rel_hos_style.body.radius = 0;
	btn_onoff_rel_hos_style.body.padding.hor = LV_DPI / 4;
	btn_onoff_rel_hos_style.body.empty = 1;

	lv_style_copy(&btn_onoff_pr_hos_style, &btn_onoff_rel_hos_style);
	if (hekate_bg)
	{
		btn_onoff_pr_hos_style.body.main_color = LV_COLOR_HEX(0xFFFFFF);
		btn_onoff_pr_hos_style.body.opa = 35;
	}
	else
		btn_onoff_pr_hos_style.body.main_color = LV_COLOR_HEX(theme_bg_color ? (theme_bg_color + 0x101010) : 0x2D2D2D);
	btn_onoff_pr_hos_style.body.grad_color = btn_onoff_pr_hos_style.body.main_color;
	btn_onoff_pr_hos_style.text.color = th->btn.pr->text.color;
	btn_onoff_pr_hos_style.body.empty = 0;

	lv_obj_t *label_btn = lv_label_create(btn, NULL);

	lv_label_set_recolor(label_btn, true);

	lv_btn_set_style(btn, LV_BTN_STYLE_REL, &btn_onoff_rel_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_PR, &btn_onoff_pr_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_TGL_REL, &btn_onoff_rel_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_TGL_PR, &btn_onoff_pr_hos_style);

	lv_btn_set_fit(btn, true, true);

	lv_label_set_text(label_btn, btn_name);

	if (action)
		lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, action);
}

static void _create_tab_about(lv_theme_t * th, lv_obj_t * parent)
{
	lv_obj_t * lbl_credits = lv_label_create(parent, NULL);

	lv_obj_align(lbl_credits, NULL, LV_ALIGN_IN_TOP_LEFT, LV_DPI / 2, LV_DPI / 2);
	lv_label_set_style(lbl_credits, &monospace_text);
	lv_label_set_recolor(lbl_credits, true);
	lv_label_set_static_text(lbl_credits,
		"#C7EA46 hekate# (c) 2018,      #C7EA46 naehrwert#, #C7EA46 st4rk#\n"
		"       (c) 2018-2025, #C7EA46 CTCaer#\n"
		"\n"
		"#C7EA46 Nyx#    (c) 2019-2025, #C7EA46 CTCaer#\n"
		"\n"
		"Thanks to: #00CCFF derrek, nedwill, plutoo, #\n"
		"           #00CCFF shuffle2, smea, thexyz, yellows8 #\n"
		"\n"
		"Greetings to: fincs, hexkyz, SciresM,\n"
		"              Shiny Quagsire, WinterMute\n"
		"\n"
		"Open source and free packages used:                    \n" // Label width alignment padding.
		" - Littlev Graphics Library,\n"
		"   Copyright (c) 2016-2018, Gabor Kiss-Vamosi\n\n"
		" - FatFs R0.13c,\n"
		"   Copyright (c) 2006-2018, ChaN\n"
		"   Copyright (c) 2018-2022, CTCaer\n\n"
		" - bcl-1.2.0,\n"
		"   Copyright (c) 2003-2006, Marcus Geelnard\n\n"
		" - blz,\n"
		"   Copyright (c) 2018, SciresM\n\n"
		" - elfload,\n"
		"   Copyright (c) 2014, Owen Shepherd\n"
		"   Copyright (c) 2018, M4xw"
	);

	lv_obj_t * lbl_octopus = lv_label_create(parent, NULL);
	lv_obj_align(lbl_octopus, lbl_credits, LV_ALIGN_OUT_RIGHT_TOP, -LV_DPI / 10, 0);
	lv_label_set_style(lbl_octopus, &monospace_text);
	lv_label_set_recolor(lbl_octopus, true);

	lv_label_set_static_text(lbl_octopus,
		"\n#00CCFF                          ___#\n"
		"#00CCFF                       .-'   `'.#\n"
		"#00CCFF                      /         \\#\n"
		"#00CCFF                      |         ;#\n"
		"#00CCFF                      |         |           ___.--,#\n"
		"#00CCFF             _.._     |0) = (0) |    _.---'`__.-( (_.#\n"
		"#00CCFF      __.--'`_.. '.__.\\    '--. \\_.-' ,.--'`     `\"\"`#\n"
		"#00CCFF     ( ,.--'`   ',__ /./;   ;, '.__.'`    __#\n"
		"#00CCFF     _`) )  .---.__.' / |   |\\   \\__..--\"\"  \"\"\"--.,_#\n"
		"#00CCFF    `---' .'.''-._.-'`_./  /\\ '.  \\ _.--''````'''--._`-.__.'#\n"
		"#00CCFF          | |  .' _.-' |  |  \\  \\  '.               `----`#\n"
		"#00CCFF           \\ \\/ .'     \\  \\   '. '-._)#\n"
		"#00CCFF            \\/ /        \\  \\    `=.__`'-.#\n"
		"#00CCFF            / /\\         `) )    / / `\"\".`\\#\n"
		"#00CCFF      , _.-'.'\\ \\        / /    ( (     / /#\n"
		"#00CCFF       `--'`   ) )    .-'.'      '.'.  | (#\n"
		"#00CCFF              (/`    ( (`          ) )  '-;    ##00FFCC [switchbrew]#\n"
		"#00CCFF               `      '-;         (-'#"
	);

	lv_obj_t *hekate_img = lv_img_create(parent, NULL);
	lv_img_set_src(hekate_img, &hekate_logo);
	lv_obj_align(hekate_img, lbl_octopus, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 2 / 3);

	lv_obj_t *ctcaer_img = lv_img_create(parent, NULL);
	lv_img_set_src(ctcaer_img, &ctcaer_logo);
	lv_obj_align(ctcaer_img, lbl_octopus, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, LV_DPI * 2 / 3);

	char version[32];
	s_printf(version, "Nyx %s%d.%d.%d%c", NYX_VER_RL ? "v" : "", NYX_VER_MJ, NYX_VER_MN, NYX_VER_HF, NYX_VER_RL > 'A' ? NYX_VER_RL : 0);
	lv_obj_t * lbl_ver = lv_label_create(parent, NULL);
	lv_obj_align(lbl_ver, ctcaer_img, LV_ALIGN_OUT_BOTTOM_RIGHT, -LV_DPI / 20, LV_DPI / 4);
	lv_label_set_style(lbl_ver, &monospace_text);
	lv_label_set_text(lbl_ver, version);
}

static void _update_status_bar(void *params)
{
	static char *label = NULL;

	u16 soc_temp = 0;
	u32 batt_percent = 0;
	int charge_status = 0;
	int batt_volt = 0;
	int batt_curr = 0;
	rtc_time_t time;

	// Get sensor data.
	max77620_rtc_get_time_adjusted(&time);
	soc_temp = tmp451_get_soc_temp(false);
	bq24193_get_property(BQ24193_ChargeStatus, &charge_status);
	max17050_get_property(MAX17050_RepSOC, (int *)&batt_percent);
	max17050_get_property(MAX17050_VCELL, &batt_volt);
	max17050_get_property(MAX17050_Current, &batt_curr);

	// Enable fan if more than 41 oC.
	u32 soc_temp_dec = soc_temp >> 8;
	fan_set_from_temp(soc_temp_dec);

	if (!label)
		label = (char *)malloc(512);

	// Set time and SoC temperature.
	s_printf(label, "%02d:%02d "SYMBOL_DOT" "SYMBOL_TEMPERATURE" %02d.%d",
		time.hour, time.min, soc_temp_dec, (soc_temp & 0xFF) / 10);

	lv_label_set_text(status_bar.time_temp, label);

	lv_obj_realign(status_bar.temp_symbol);
	lv_obj_realign(status_bar.temp_degrees);

	// Set battery percent and charging symbol.
	s_printf(label, " "SYMBOL_DOT" %d.%d%% ", (batt_percent >> 8) & 0xFF, (batt_percent & 0xFF) / 26);

	u8 batt_level = (batt_percent >> 8) & 0xFF;
	if (batt_level > 80)
		strcat(label, SYMBOL_BATTERY_FULL);
	else if (batt_level > 60)
		strcat(label, SYMBOL_BATTERY_3);
	else if (batt_level > 40)
		strcat(label, SYMBOL_BATTERY_2);
	else if (batt_level > 15)
		strcat(label, SYMBOL_BATTERY_1);
	else
		strcat(label, "#FF3C28 "SYMBOL_BATTERY_EMPTY"#");

	// Set charging symbol.
	if (charge_status)
		strcat(label, " #FFDD00 "SYMBOL_CHARGE"#");

	lv_label_set_text(status_bar.battery, label);
	lv_obj_realign(status_bar.battery);

	// Set battery current draw and voltage.
	s_printf(label, "#%s%d", batt_curr >= 0 ? "96FF00 +" : "FF3C28 ", batt_curr / 1000);

	bool voltage_empty = batt_volt < 3200;
	s_printf(label + strlen(label), " mA# (%s%d mV%s)",
		voltage_empty ? "#FF8000 " : "", batt_volt,  voltage_empty ? " "SYMBOL_WARNING"#" : "");

	lv_label_set_text(status_bar.battery_more, label);
	lv_obj_realign(status_bar.battery_more);
}

static lv_res_t _create_mbox_payloads(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\251", "\222Cancel", "\251", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES * 5 / 9);

	lv_mbox_set_text(mbox, "Select a payload to launch:");

	// Create a list with all found payloads.
	//! TODO: SHould that be tabs with buttons? + Icon support?
	lv_obj_t *list = lv_list_create(mbox, NULL);
	payload_list = list;
	lv_obj_set_size(list, LV_HOR_RES * 3 / 7, LV_VER_RES * 3 / 7);
	lv_list_set_single_mode(list, true);

	if (!sd_mount())
	{
		lv_mbox_set_text(mbox, "#FFDD00 Failed to init SD!#");

		goto out_end;
	}

	dirlist_t *filelist = dirlist("bootloader/payloads", NULL, 0);
	sd_unmount();

	u32 i = 0;
	if (filelist)
	{
		while (true)
		{
			if (!filelist->name[i])
				break;
			lv_list_add(list, NULL, filelist->name[i], launch_payload);
			i++;
		}
		free(filelist);
	}

out_end:
	lv_mbox_add_btns(mbox, mbox_btn_map, mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}
typedef struct _launch_menu_entries_t
{
	lv_obj_t *btn[20];
	lv_obj_t *label[20];
} launch_menu_entries_t;

static launch_menu_entries_t launch_ctxt;
static lv_obj_t *launch_bg = NULL;
static bool launch_bg_done = false;

static lv_res_t _launch_more_cfg_action(lv_obj_t *btn)
{
	lv_btn_ext_t *ext = lv_obj_get_ext_attr(btn);

	_launch_hos(ext->idx, 1);

	return LV_RES_OK;
}

static lv_res_t _win_launch_close_action(lv_obj_t * btn)
{
	// Cleanup icons.
	for (u32 i = 0; i < (n_cfg.entries_5_col ? 10 : 8); i++)
	{
		lv_obj_t *btns = launch_ctxt.btn[i];
		lv_btn_ext_t *ext = lv_obj_get_ext_attr(btns);
		if (ext->idx)
		{
			// This gets latest object, which is the button overlay. So iterate 2 times.
			lv_obj_t * img = lv_obj_get_child(btns, NULL);
			img = lv_obj_get_child(btns, img);

			lv_img_dsc_t *src = (lv_img_dsc_t *)lv_img_get_src(img);

			// Avoid freeing base icons.
			if ((src != icon_switch) && (src != icon_payload))
				free(src);
		}
	}

	lv_obj_t * win = lv_win_get_from_btn(btn);

	lv_obj_del(win);

	if (n_cfg.home_screen && !launch_bg_done && hekate_bg)
	{
		lv_obj_set_opa_scale_enable(launch_bg, true);
		lv_obj_set_opa_scale(launch_bg, LV_OPA_TRANSP);
		//if (launch_bg)
		//	lv_obj_del(launch_bg); //! TODO: Find why it hangs.
		launch_bg_done = true;
	}

	close_btn = NULL;

	return LV_RES_INV;
}

static lv_obj_t *create_window_launch(const char *win_title)
{
	static lv_style_t win_bg_style, win_header;

	lv_style_copy(&win_bg_style, &lv_style_plain);
	win_bg_style.body.main_color = lv_theme_get_current()->bg->body.main_color;
	win_bg_style.body.grad_color = win_bg_style.body.main_color;

	if (n_cfg.home_screen && !launch_bg_done && hekate_bg)
	{
		lv_obj_t *img = lv_img_create(lv_scr_act(), NULL);
		lv_img_set_src(img, hekate_bg);

		launch_bg = img;
	}

	lv_obj_t *win = lv_win_create(lv_scr_act(), NULL);
	lv_win_set_title(win, win_title);

	lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);

	if (n_cfg.home_screen && !launch_bg_done && hekate_bg)
	{
		lv_style_copy(&win_header, lv_theme_get_current()->win.header);
		win_header.body.opa = LV_OPA_TRANSP;

		win_bg_style.body.opa = LV_OPA_TRANSP;
		lv_win_set_style(win, LV_WIN_STYLE_HEADER, &win_header);
	}

	lv_win_set_style(win, LV_WIN_STYLE_BG, &win_bg_style);

	close_btn = lv_win_add_btn(win, NULL, SYMBOL_CLOSE" Close", _win_launch_close_action);

	return win;
}

static lv_res_t _launch_action(lv_obj_t *btn)
{
	lv_btn_ext_t *ext = lv_obj_get_ext_attr(btn);

	_launch_hos(ext->idx, 0);

	return LV_RES_OK;
}

static lv_res_t logs_onoff_toggle(lv_obj_t *btn)
{
	launch_logs_enable = !launch_logs_enable;

	lv_obj_t *label_btn = lv_obj_get_child(btn, NULL);

	char label_text[64];
	strcpy(label_text, lv_label_get_text(label_btn));
	label_text[strlen(label_text) - 12] = 0;

	if (!launch_logs_enable)
	{
		strcat(label_text, "#D0D0D0 OFF#");
		lv_label_set_text(label_btn, label_text);
	}
	else
	{
		s_printf(label_text, "%s%s%s", label_text, text_color, " ON #");
		lv_label_set_text(label_btn, label_text);
	}

	return LV_RES_OK;
}

typedef struct _launch_button_pos_t
{
	u16 btn_x;
	u16 btn_y;
	u16 lbl_x;
	u16 lbl_y;
} launch_button_pos_t;

static const launch_button_pos_t launch_button_pos8[8] = {
	// First row.
	{  19,  36,   0, 245 },
	{ 340,  36, 321, 245 },
	{ 661,  36, 642, 245 },
	{ 982,  36, 963, 245 },
	// Second row.
	{  19, 313,   0, 522 },
	{ 340, 313, 321, 522 },
	{ 661, 313, 642, 522 },
	{ 982, 313, 963, 522 }
};

static const launch_button_pos_t launch_button_pos10[10] = {
	// First row.
	{ 19, 36,  0,  245},
	{260, 36, 241, 245},
	{501, 36, 482, 245},
	{742, 36, 723, 245},
	{983, 36, 964, 245},
	// Second row.
	{ 19, 313,   0, 522},
	{260, 313, 241, 522},
	{501, 313, 482, 522},
	{742, 313, 723, 522},
	{983, 313, 964, 522}
};

static lv_res_t _create_window_home_launch(lv_obj_t *btn)
{
	const u32 max_entries = n_cfg.entries_5_col ? 10 : 8;
	const launch_button_pos_t *launch_button_pos = n_cfg.entries_5_col ? launch_button_pos10 : launch_button_pos8;

	char *icon_path;

	static lv_style_t btn_home_noborder_rel;
	lv_style_copy(&btn_home_noborder_rel, lv_theme_get_current()->btn.rel);
	btn_home_noborder_rel.body.opa = LV_OPA_0;
	btn_home_noborder_rel.body.border.width = 4;
	btn_home_noborder_rel.body.border.opa = LV_OPA_0;

	static lv_style_t btn_home_transp_rel;
	lv_style_copy(&btn_home_transp_rel, lv_theme_get_current()->btn.rel);
	btn_home_transp_rel.body.opa = LV_OPA_0;
	btn_home_transp_rel.body.border.width = 4;

	static lv_style_t btn_home_transp_pr;
	lv_style_copy(&btn_home_transp_pr, lv_theme_get_current()->btn.pr);
	btn_home_transp_pr.body.main_color = LV_COLOR_HEX(0xFFFFFF);
	btn_home_transp_pr.body.grad_color = btn_home_transp_pr.body.main_color;
	btn_home_transp_pr.body.opa = LV_OPA_30;

	static lv_style_t btn_label_home_transp;
	lv_style_copy(&btn_label_home_transp, lv_theme_get_current()->cont);
	btn_label_home_transp.body.opa = LV_OPA_TRANSP;

	lv_obj_t *win;

	bool more_cfg = false;
	bool combined_cfg = false;
	if (btn)
	{
		if (strcmp(lv_label_get_text(lv_obj_get_child(btn, NULL)) + 8,"Launch#"))
			more_cfg = true;
	}
	else
	{
		switch (n_cfg.home_screen)
		{
		case 1: // All configs.
			combined_cfg = true;
			break;
		case 3: // More configs
			more_cfg = true;
			break;
		}
	}

	if (!btn)
		win = create_window_launch(SYMBOL_GPS" hekate - Launch");
	else if (!more_cfg)
		win = create_window_launch(SYMBOL_GPS" Launch");
	else
		win = create_window_launch(SYMBOL_GPS" More Configurations");

	lv_win_add_btn(win, NULL, SYMBOL_LIST" Logs #D0D0D0 OFF#", logs_onoff_toggle);
	launch_logs_enable = false;

	lv_cont_set_fit(lv_page_get_scrl(lv_win_get_content(win)), false, false);
	lv_page_set_scrl_height(lv_win_get_content(win), 572);

	lv_btn_ext_t * ext;
	lv_obj_t *btn_boot_entry;
	lv_obj_t *boot_entry_lbl_cont;
	lv_obj_t *boot_entry_label;
	bool no_boot_entries = false;

	// Create CFW buttons.
	// Buttons are 200 x 200 with 4 pixel borders.
	// Icons must be <= 192 x 192.
	// Create first Button.
	btn_boot_entry = lv_btn_create(win, NULL);
	launch_ctxt.btn[0] = btn_boot_entry;
	lv_obj_set_size(btn_boot_entry, 200, 200);
	lv_obj_set_pos(btn_boot_entry, launch_button_pos[0].btn_x, launch_button_pos[0].btn_y);
	lv_obj_set_opa_scale(btn_boot_entry, LV_OPA_0);
	lv_obj_set_opa_scale_enable(btn_boot_entry, true);
	lv_btn_set_layout(btn_boot_entry, LV_LAYOUT_OFF);

	boot_entry_lbl_cont = lv_cont_create(win, NULL);
	boot_entry_label = lv_label_create(boot_entry_lbl_cont, NULL);
	lv_obj_set_style(boot_entry_label, &hint_small_style_white);
	lv_label_set_text(boot_entry_label, "");
	launch_ctxt.label[0] = boot_entry_label;

	lv_cont_set_fit(boot_entry_lbl_cont, false, false);
	lv_cont_set_layout(boot_entry_lbl_cont, LV_LAYOUT_CENTER);
	lv_obj_set_size(boot_entry_lbl_cont, 238, 20);
	lv_obj_set_pos(boot_entry_lbl_cont, launch_button_pos[0].lbl_x, launch_button_pos[0].lbl_y);
	lv_obj_set_style(boot_entry_lbl_cont, &btn_label_home_transp);

	// Create the rest of the buttons.
	for (u32 btn_idx = 1; btn_idx < (n_cfg.entries_5_col ? 10 : 8); btn_idx++)
	{
		btn_boot_entry = lv_btn_create(win, btn_boot_entry);
		launch_ctxt.btn[btn_idx] = btn_boot_entry;
		lv_obj_set_pos(btn_boot_entry, launch_button_pos[btn_idx].btn_x, launch_button_pos[btn_idx].btn_y);

		boot_entry_lbl_cont = lv_cont_create(win, boot_entry_lbl_cont);
		boot_entry_label = lv_label_create(boot_entry_lbl_cont, boot_entry_label);
		lv_obj_set_pos(boot_entry_lbl_cont, launch_button_pos[btn_idx].lbl_x, launch_button_pos[btn_idx].lbl_y);
		launch_ctxt.label[btn_idx] = boot_entry_label;
	}

	// Create colorized icon style based on its parent style.
	static lv_style_t img_style;
	lv_style_copy(&img_style, &lv_style_plain);
	img_style.image.color = lv_color_hsv_to_rgb(n_cfg.theme_color, 100, 100);
	img_style.image.intense = LV_OPA_COVER;

	// Parse ini boot entries and set buttons/icons.
	char *tmp_path = malloc(1024);
	u32 curr_btn_idx = 0; // Active buttons.
	LIST_INIT(ini_sections);

	if (!sd_mount())
		goto failed_sd_mount;

	// Check if we use custom system icons.
	bool icon_sw_custom = !f_stat("bootloader/res/icon_switch_custom.bmp", NULL);
	bool icon_pl_custom = !f_stat("bootloader/res/icon_payload_custom.bmp", NULL);

	// Choose what to parse.
	bool ini_parse_success = false;
	if (!more_cfg)
		ini_parse_success = ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false);
	else
		ini_parse_success = ini_parse(&ini_sections, "bootloader/ini", true);

	if (combined_cfg && !ini_parse_success)
	{
ini_parsing:
		list_init(&ini_sections);
		ini_parse_success = ini_parse(&ini_sections, "bootloader/ini", true);
		more_cfg = true;
	}

	if (!ini_parse_success)
		goto ini_parse_failed;

	// Iterate to all boot entries and load icons.
	u32 entry_idx = 1;
	LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
	{
		if (!strcmp(ini_sec->name, "config") || (ini_sec->type != INI_CHOICE))
			continue;

		icon_path = NULL;
		bool payload = false;
		bool img_colorize = false;
		bool img_noborder = false;
		lv_img_dsc_t *bmp = NULL;
		lv_obj_t *img = NULL;

		// Check for icons.
		LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
		{
			if (!strcmp("icon", kv->key))
				icon_path = kv->val;
			else if (!strcmp("payload", kv->key))
				payload = true;
		}

		// If icon not found, check res folder for section_name.bmp.
		// If not, use defaults.
		if (!icon_path)
		{
			s_printf(tmp_path, "bootloader/res/%s.bmp", ini_sec->name);
			bmp = bmp_to_lvimg_obj(tmp_path);
			if (!bmp)
			{
				s_printf(tmp_path, "bootloader/res/%s_hue_nobox.bmp", ini_sec->name);
				bmp = bmp_to_lvimg_obj(tmp_path);
				if (bmp)
				{
					img_noborder = true;
					img_colorize = true;
				}
				if (!bmp)
				{
					s_printf(tmp_path, "bootloader/res/%s_hue.bmp", ini_sec->name);
					bmp = bmp_to_lvimg_obj(tmp_path);
					if (bmp)
						img_colorize = true;
				}
				if (!bmp)
				{
					s_printf(tmp_path, "bootloader/res/%s_nobox.bmp", ini_sec->name);
					bmp = bmp_to_lvimg_obj(tmp_path);
					if (bmp)
						img_noborder = true;
				}
			}

			if (!bmp && payload)
			{
				bmp = icon_payload;

				if (!icon_pl_custom)
					img_colorize = true;
			}
		}
		else
		{
			bmp = bmp_to_lvimg_obj(icon_path);

			// Check if both colorization and border are enabled.
			if (bmp && strlen(icon_path) > 14 && !memcmp(icon_path + strlen(icon_path) - 14, "_hue_nobox", 10))
			{
				img_colorize = true;
				img_noborder = true;
			}
			else
			{
				// Check if colorization is enabled.
				if (bmp && strlen(icon_path) > 8 && !memcmp(icon_path + strlen(icon_path) - 8, "_hue", 4))
					img_colorize = true;

				// Check if no border is enabled.
				if (bmp && strlen(icon_path) > 10 && !memcmp(icon_path + strlen(icon_path) - 10, "_nobox", 6))
					img_noborder = true;
			}
		}

		// Enable button.
		lv_obj_set_opa_scale(launch_ctxt.btn[curr_btn_idx], LV_OPA_COVER);

		// Default to switch logo if no icon found at all.
		if (!bmp)
		{
			bmp = icon_switch;

			if (!icon_sw_custom)
				img_colorize = true;
		}

		//Set icon.
		if (bmp)
		{
			img = lv_img_create(launch_ctxt.btn[curr_btn_idx], NULL);

			if (img_colorize)
				lv_img_set_style(img, &img_style);

			lv_img_set_src(img, bmp);
		}

		// Add button mask/radius and align icon.
		lv_obj_t *btns = lv_btn_create(launch_ctxt.btn[curr_btn_idx], NULL);
		u32 btn_width = 200;
		u32 btn_height = 200;
		if (img_noborder)
		{
			btn_width = bmp->header.w + 4;
			btn_height = bmp->header.h + 4;

			if (btn_width > 200)
				btn_width = 200;
			if (btn_height > 200)
				btn_height = 200;

			lv_btn_set_style(launch_ctxt.btn[curr_btn_idx], LV_BTN_STYLE_REL, &btn_home_noborder_rel);
			lv_btn_set_style(launch_ctxt.btn[curr_btn_idx], LV_BTN_STYLE_PR, &btn_home_noborder_rel);
		}
		lv_obj_set_size(btns, btn_width, btn_height);
		lv_btn_set_style(btns, LV_BTN_STYLE_REL, img_noborder ? &btn_home_noborder_rel : &btn_home_transp_rel);
		lv_btn_set_style(btns, LV_BTN_STYLE_PR, &btn_home_transp_pr);
		if (img)
			lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
		if (img_noborder)
			lv_obj_align(btns, NULL, LV_ALIGN_CENTER, 0, 0);

		// Set autoboot index.
		ext = lv_obj_get_ext_attr(btns);
		ext->idx = entry_idx;
		ext = lv_obj_get_ext_attr(launch_ctxt.btn[curr_btn_idx]); // Redundancy.
		ext->idx = entry_idx;

		// Set action.
		if (!more_cfg)
			lv_btn_set_action(btns, LV_BTN_ACTION_CLICK, _launch_action);
		else
			lv_btn_set_action(btns, LV_BTN_ACTION_CLICK, _launch_more_cfg_action);

		// Set button's label text.
		lv_label_set_text(launch_ctxt.label[curr_btn_idx], ini_sec->name);
		lv_obj_set_opa_scale(launch_ctxt.label[curr_btn_idx], LV_OPA_COVER);

		// Set rolling text if name is big.
		if (strlen(ini_sec->name) > 22)
			lv_label_set_long_mode(launch_ctxt.label[curr_btn_idx], LV_LABEL_LONG_ROLL);

		entry_idx++;
		curr_btn_idx++;

		// Check if we exceed max buttons.
		if (curr_btn_idx >= max_entries)
			break;
	}

	ini_free(&ini_sections);

ini_parse_failed:
	// Reiterate the loop with more cfgs if combined.
	if (combined_cfg && (curr_btn_idx < (n_cfg.entries_5_col ? 10 : 8)) && !more_cfg)
		goto ini_parsing;

failed_sd_mount:
	if (curr_btn_idx < 1)
		no_boot_entries = true;

	sd_unmount();

	free(tmp_path);

	// No boot entries found.
	if (no_boot_entries)
	{
		lv_obj_t *label_error = lv_label_create(win, NULL);
		lv_label_set_recolor(label_error, true);
		if (!more_cfg)
		{
			lv_label_set_static_text(label_error,
				"#FFDD00 No main boot entries found...#\n"
				"Check that #96FF00 bootloader/hekate_ipl.ini# has boot entries\n"
				"or use #C7EA46 More configs# button for more boot entries.");
		}
		else
		{
			lv_label_set_static_text(label_error,
				"#FFDD00 No .ini or boot entries found...#\n"
				"Check that a .ini file exists in #96FF00 bootloader/ini/#\n"
				"and that it contains at least one entry.");
		}

		lv_obj_set_pos(label_error, 19, 0);
	}

	return LV_RES_OK;
}

static void _create_tab_home(lv_theme_t *th, lv_obj_t *parent)
{
	lv_page_set_scrl_layout(parent, LV_LAYOUT_OFF);
	lv_page_set_scrl_fit(parent, false, false);
	lv_page_set_scrl_height(parent, 592);

	char btn_colored_text[64];

	// Set brand label.
	lv_obj_t *label_brand = lv_label_create(parent, NULL);
	lv_label_set_recolor(label_brand, true);
	s_printf(btn_colored_text, "%s%s", text_color, " hekate#");
	lv_label_set_text(label_brand, btn_colored_text);
	lv_obj_set_pos(label_brand, 50, 48);

	// Set tagline label.
	lv_obj_t *label_tagline = lv_label_create(parent, NULL);
	lv_obj_set_style(label_tagline, &hint_small_style_white);
	lv_label_set_static_text(label_tagline, "THE ALL IN ONE BOOTLOADER FOR ALL YOUR NEEDS");
	lv_obj_set_pos(label_tagline, 50, 82);

	static lv_style_t icons;
	lv_style_copy(&icons, th->label.prim);
	icons.text.font = &hekate_symbol_120;

	// Launch button.
	lv_obj_t *btn_launch = lv_btn_create(parent, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn_launch, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn_launch, LV_BTN_STYLE_PR, &btn_transp_pr);
	}
	lv_obj_t *label_btn = lv_label_create(btn_launch, NULL);
	lv_label_set_recolor(label_btn, true);
	lv_obj_set_style(label_btn, &icons);
	s_printf(btn_colored_text, "%s%s", text_color, " "SYMBOL_DOT"#");
	lv_label_set_text(label_btn, btn_colored_text);
	lv_btn_set_action(btn_launch, LV_BTN_ACTION_CLICK, _create_window_home_launch);
	lv_obj_set_size(btn_launch, 256, 256);
	lv_obj_set_pos(btn_launch, 50, 160);
	lv_btn_set_layout(btn_launch, LV_LAYOUT_OFF);
	lv_obj_align(label_btn, NULL, LV_ALIGN_CENTER, 0, -28);
	lv_obj_t *label_btn2 = lv_label_create(btn_launch, NULL);
	lv_label_set_recolor(label_btn2, true);
	s_printf(btn_colored_text, "%s%s", text_color, " Launch#");
	lv_label_set_text(label_btn2, btn_colored_text);
	lv_obj_align(label_btn2, NULL, LV_ALIGN_IN_TOP_MID, 0, 174);

	// More Configs button.
	lv_obj_t *btn_more_cfg = lv_btn_create(parent, btn_launch);
	label_btn = lv_label_create(btn_more_cfg, label_btn);
	s_printf(btn_colored_text, "%s%s", text_color, " "SYMBOL_CLOCK"#");
	lv_label_set_text(label_btn, btn_colored_text);
	lv_btn_set_action(btn_more_cfg, LV_BTN_ACTION_CLICK, _create_window_home_launch);
	lv_btn_set_layout(btn_more_cfg, LV_LAYOUT_OFF);
	lv_obj_align(label_btn, NULL, LV_ALIGN_CENTER, 0, -28);
	label_btn2 = lv_label_create(btn_more_cfg, label_btn2);
	s_printf(btn_colored_text, "%s%s", text_color, " More Configs#");
	lv_label_set_text(label_btn2, btn_colored_text);
	lv_obj_set_pos(btn_more_cfg, 341, 160);
	lv_obj_align(label_btn2, NULL, LV_ALIGN_IN_TOP_MID, 0, 174);

	// Quick Launch button.
	// lv_obj_t *btn_quick_launch = lv_btn_create(parent, NULL);
	// lv_obj_t *label_quick_launch = lv_label_create(btn_quick_launch, NULL);
	// lv_label_set_static_text(label_quick_launch, SYMBOL_EDIT" Quick Launch");
	// lv_obj_set_width(btn_quick_launch, 256);
	// lv_obj_set_pos(btn_quick_launch, 343, 448);
	// lv_btn_set_action(btn_quick_launch, LV_BTN_ACTION_CLICK, NULL);

	lv_obj_t *btn_nyx_options = lv_btn_create(parent, NULL);
	_create_text_button(th, NULL, btn_nyx_options, SYMBOL_SETTINGS" Nyx Settings", NULL);
	//lv_obj_set_width(btn_nyx_options, 256);
	lv_btn_set_action(btn_nyx_options, LV_BTN_ACTION_CLICK, create_win_nyx_options);
	lv_obj_align(btn_nyx_options, NULL, LV_ALIGN_IN_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI / 12);

	// Payloads button.
	lv_obj_t *btn_payloads = lv_btn_create(parent, btn_launch);
	label_btn = lv_label_create(btn_payloads, label_btn);
	s_printf(btn_colored_text, "%s%s", text_color, " "SYMBOL_OK"#");
	lv_label_set_text(label_btn, btn_colored_text);
	lv_btn_set_action(btn_payloads, LV_BTN_ACTION_CLICK, _create_mbox_payloads);
	lv_btn_set_layout(btn_payloads, LV_LAYOUT_OFF);
	lv_obj_align(label_btn, NULL, LV_ALIGN_CENTER, 0, -28);
	label_btn2 = lv_label_create(btn_payloads, label_btn2);
	s_printf(btn_colored_text, "%s%s", text_color, " Payloads#");
	lv_label_set_text(label_btn2, btn_colored_text);
	lv_obj_set_pos(btn_payloads, 632, 160);
	lv_obj_align(label_btn2, NULL, LV_ALIGN_IN_TOP_MID, 0, 174);

	lv_obj_t *line_sep = lv_line_create(parent, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, {0, LV_DPI * 3} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, th->line.decor);
	lv_obj_align(line_sep, btn_payloads, LV_ALIGN_OUT_RIGHT_MID, 35, 0);

	// emuMMC manage button.
	lv_obj_t *btn_emummc = lv_btn_create(parent, btn_launch);
	label_btn = lv_label_create(btn_emummc, label_btn);
	s_printf(btn_colored_text, "%s%s", text_color, " "SYMBOL_LIST"#");
	lv_label_set_text(label_btn, btn_colored_text);
	lv_btn_set_action(btn_emummc, LV_BTN_ACTION_CLICK, create_win_emummc_tools);
	lv_btn_set_layout(btn_emummc, LV_LAYOUT_OFF);
	lv_obj_align(label_btn, NULL, LV_ALIGN_CENTER, 0, -28);
	lv_obj_set_pos(btn_emummc, 959, 160);
	label_btn2 = lv_label_create(btn_emummc, label_btn2);
	s_printf(btn_colored_text, "%s%s", text_color, " emuMMC#");
	lv_label_set_text(label_btn2, btn_colored_text);
	lv_obj_align(label_btn2, NULL, LV_ALIGN_IN_TOP_MID, 0, 174);

	// Create bottom right power buttons.
	lv_obj_t *btn_reboot = lv_btn_create(parent, NULL);
	lv_obj_t *btn_power_off = lv_btn_create(parent, NULL);
	lv_obj_t *btn_reload = lv_btn_create(parent, NULL);

	_create_text_button(th, NULL, btn_power_off, SYMBOL_POWER" Power Off", _create_mbox_poweroff);
	lv_obj_align(btn_power_off, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -LV_DPI / 4, -LV_DPI / 12);

	_create_text_button(th, NULL, btn_reboot, SYMBOL_REBOOT" Reboot", _create_mbox_reboot);
	lv_obj_align(btn_reboot, btn_power_off, LV_ALIGN_OUT_LEFT_MID, 0, 0);

	_create_text_button(th, NULL, btn_reload, SYMBOL_REFRESH" Reload", _create_mbox_reload);
	lv_obj_align(btn_reload, btn_reboot, LV_ALIGN_OUT_LEFT_MID, 0, 0);
}

static lv_res_t _save_options_action(lv_obj_t *btn)
{
	static const char * mbox_btn_map[] = {"\251", "\222OK!", "\251", ""};
	lv_obj_t * mbox = lv_mbox_create(lv_scr_act(), NULL);
	lv_mbox_set_recolor_text(mbox, true);

	int res = 0;

	if (sd_mount())
		res = !create_config_entry();

	if (res)
		lv_mbox_set_text(mbox, "#FF8000 hekate Configuration#\n\n#96FF00 The configuration was saved to sd card!#");
	else
		lv_mbox_set_text(mbox, "#FF8000 hekate Configuration#\n\n#FFDD00 Failed to save the configuration#\n#FFDD00 to sd card!#");
	lv_mbox_add_btns(mbox, mbox_btn_map, NULL);
	lv_obj_set_top(mbox, true);

	nyx_options_clear_ini_changes_made();

	sd_unmount();

	return LV_RES_OK;
}

static void _create_status_bar(lv_theme_t * th)
{
	static lv_obj_t *status_bar_bg;
	status_bar_bg = lv_cont_create(lv_layer_top(), NULL);
	status_bar.bar_bg = status_bar_bg;

	static lv_style_t status_bar_style;
	lv_style_copy(&status_bar_style, &lv_style_plain_color);
	status_bar_style.body.opa = LV_OPA_0;
	status_bar_style.body.shadow.width = 0;

	lv_obj_set_style(status_bar_bg, &status_bar_style);
	lv_obj_set_size(status_bar_bg, LV_HOR_RES, LV_DPI * 9 / 14);
	lv_obj_align(status_bar_bg, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

	// Battery percentages.
	lv_obj_t *lbl_battery = lv_label_create(status_bar_bg, NULL);
	lv_label_set_recolor(lbl_battery, true);
	lv_label_set_text(lbl_battery, " "SYMBOL_DOT" 00.0% "SYMBOL_BATTERY_1" #FFDD00 "SYMBOL_CHARGE"#");
	lv_obj_align(lbl_battery, NULL, LV_ALIGN_IN_RIGHT_MID, -LV_DPI * 6 / 11, 0);
	status_bar.battery = lbl_battery;

	// Amperages, voltages.
	lbl_battery = lv_label_create(status_bar_bg, lbl_battery);
	lv_obj_set_style(lbl_battery, &hint_small_style_white);
	lv_label_set_text(lbl_battery, "#96FF00 +0 mA# (0 mV)");
	lv_obj_align(lbl_battery, status_bar.battery, LV_ALIGN_OUT_LEFT_MID, -LV_DPI / 25, -1);
	status_bar.battery_more = lbl_battery;

	lv_obj_t *lbl_left = lv_label_create(status_bar_bg, NULL);
	lv_label_set_text(lbl_left, SYMBOL_CLOCK" ");
	lv_obj_align(lbl_left, NULL, LV_ALIGN_IN_LEFT_MID, LV_DPI * 6 / 11, 0);

	// Time, temperature.
	lv_obj_t *lbl_time_temp = lv_label_create(status_bar_bg, NULL);
	lv_label_set_text(lbl_time_temp, "00:00 "SYMBOL_DOT" "SYMBOL_TEMPERATURE" 00.0");
	lv_obj_align(lbl_time_temp, lbl_left, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	status_bar.time_temp = lbl_time_temp;

	lbl_left = lv_label_create(status_bar_bg, NULL);
	lv_label_set_text(lbl_left, " "SYMBOL_DOT);
	lv_obj_align(lbl_left, lbl_time_temp, LV_ALIGN_OUT_RIGHT_MID, 0, -LV_DPI / 14);
	status_bar.temp_symbol = lbl_left;

	lv_obj_t *lbl_degrees = lv_label_create(status_bar_bg, NULL);
	lv_label_set_text(lbl_degrees, "C");
	lv_obj_align(lbl_degrees, lbl_left, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 50, LV_DPI / 14);
	status_bar.temp_degrees = lbl_degrees;

	// Middle button.
	//! TODO: Utilize it for more.
	lv_obj_t *btn_mid = lv_btn_create(status_bar_bg, NULL);
	lv_obj_t *lbl_mid = lv_label_create(btn_mid, NULL);
	lv_label_set_static_text(lbl_mid, "Save Options");
	lv_obj_set_size(btn_mid, LV_DPI * 5 / 2, LV_DPI / 2);
	lv_obj_align(btn_mid, NULL, LV_ALIGN_CENTER, 0, 0);
	status_bar.mid = btn_mid;
	lv_obj_set_opa_scale(btn_mid, LV_OPA_0);
	lv_obj_set_opa_scale_enable(btn_mid, true);
	lv_obj_set_click(btn_mid, false);
	lv_btn_set_action(btn_mid, LV_BTN_ACTION_CLICK, _save_options_action);
}

static lv_res_t _create_mbox_save_changes_action(lv_obj_t *btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	mbox_action(btns, txt);

	if (!btn_idx)
		_save_options_action(NULL);

	return LV_RES_INV;
}

void nyx_check_ini_changes()
{
	if (nyx_options_get_ini_changes_made())
	{
		lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
		lv_obj_set_style(dark_bg, &mbox_darken);
		lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

		static const char * mbox_btn_map[] = { "\222Save", "\222Cancel", "" };
		lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
		lv_mbox_set_recolor_text(mbox, true);

		lv_mbox_set_text(mbox,
			"#FF8000 Main configuration#\n\n"
			"You changed the configuration!\n\n"
			"Do you want to save it?");

		lv_mbox_add_btns(mbox, mbox_btn_map, _create_mbox_save_changes_action);
		lv_obj_set_width(mbox, LV_HOR_RES / 9 * 5);
		lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
		lv_obj_set_top(mbox, true);

		nyx_options_clear_ini_changes_made();
	}
}

static lv_res_t _show_hide_save_button(lv_obj_t *tv, uint16_t tab_idx)
{
	if (tab_idx == 4) // Options.
	{
		lv_btn_set_action(status_bar.mid, LV_BTN_ACTION_CLICK, _save_options_action);
		lv_obj_set_opa_scale(status_bar.mid, LV_OPA_COVER);
		lv_obj_set_click(status_bar.mid, true);
	}
	else
	{
		lv_obj_set_opa_scale(status_bar.mid, LV_OPA_0);
		lv_obj_set_click(status_bar.mid, false);
	}

	nyx_check_ini_changes();

	return LV_RES_OK;
}

static void _nyx_set_default_styles(lv_theme_t * th)
{
	lv_style_copy(&mbox_darken, &lv_style_plain);
	mbox_darken.body.main_color = LV_COLOR_BLACK;
	mbox_darken.body.grad_color = mbox_darken.body.main_color;
	mbox_darken.body.opa = LV_OPA_30;

	lv_style_copy(&hint_small_style, th->label.hint);
	hint_small_style.text.letter_space = 1;
	hint_small_style.text.font = &interui_20;

	lv_style_copy(&hint_small_style_white, th->label.prim);
	hint_small_style_white.text.letter_space = 1;
	hint_small_style_white.text.font = &interui_20;

	lv_style_copy(&monospace_text, &lv_style_plain);
	monospace_text.body.main_color = LV_COLOR_HEX(0x1B1B1B);
	monospace_text.body.grad_color = LV_COLOR_HEX(0x1B1B1B);
	monospace_text.body.border.color = LV_COLOR_HEX(0x1B1B1B);
	monospace_text.body.border.width = 0;
	monospace_text.body.opa = LV_OPA_TRANSP;
	monospace_text.text.color = LV_COLOR_HEX(0xD8D8D8);
	monospace_text.text.font = &ubuntu_mono;
	monospace_text.text.letter_space = 0;
	monospace_text.text.line_space = 0;

	lv_style_copy(&btn_transp_rel, th->btn.rel);
	btn_transp_rel.body.main_color = LV_COLOR_HEX(0x444444);
	btn_transp_rel.body.grad_color = btn_transp_rel.body.main_color;
	btn_transp_rel.body.opa = LV_OPA_50;

	lv_style_copy(&btn_transp_pr, th->btn.pr);
	btn_transp_pr.body.main_color = LV_COLOR_HEX(0x888888);
	btn_transp_pr.body.grad_color = btn_transp_pr.body.main_color;
	btn_transp_pr.body.opa = LV_OPA_50;

	lv_style_copy(&btn_transp_tgl_rel, th->btn.tgl_rel);
	btn_transp_tgl_rel.body.main_color = LV_COLOR_HEX(0x444444);
	btn_transp_tgl_rel.body.grad_color = btn_transp_tgl_rel.body.main_color;
	btn_transp_tgl_rel.body.opa = LV_OPA_50;

	lv_style_copy(&btn_transp_tgl_pr, th->btn.tgl_pr);
	btn_transp_tgl_pr.body.main_color = LV_COLOR_HEX(0x888888);
	btn_transp_tgl_pr.body.grad_color = btn_transp_tgl_pr.body.main_color;
	btn_transp_tgl_pr.body.opa = LV_OPA_50;

	lv_style_copy(&ddlist_transp_bg, th->ddlist.bg);
	ddlist_transp_bg.body.main_color = LV_COLOR_HEX(0x2D2D2D);
	ddlist_transp_bg.body.grad_color = ddlist_transp_bg.body.main_color;
	ddlist_transp_bg.body.opa = 180;

	lv_style_copy(&ddlist_transp_sel, th->ddlist.sel);
	ddlist_transp_sel.body.main_color = LV_COLOR_HEX(0x4D4D4D);
	ddlist_transp_sel.body.grad_color = ddlist_transp_sel.body.main_color;
	ddlist_transp_sel.body.opa = 180;

	lv_style_copy(&tabview_btn_pr, th->tabview.btn.pr);
	tabview_btn_pr.body.main_color = LV_COLOR_HEX(0xFFFFFF);
	tabview_btn_pr.body.grad_color = tabview_btn_pr.body.main_color;
	tabview_btn_pr.body.opa = 35;

	lv_style_copy(&tabview_btn_tgl_pr, th->tabview.btn.tgl_pr);
	tabview_btn_tgl_pr.body.main_color = LV_COLOR_HEX(0xFFFFFF);
	tabview_btn_tgl_pr.body.grad_color = tabview_btn_tgl_pr.body.main_color;
	tabview_btn_tgl_pr.body.opa = 35;

	lv_color_t tmp_color = lv_color_hsv_to_rgb(n_cfg.theme_color, 100, 100);
	text_color = malloc(32);
	s_printf(text_color, "#%06X", (u32)(tmp_color.full & 0xFFFFFF));
}

lv_task_t *task_bpmp_clock;
void first_time_bpmp_clock(void *param)
{
	// Remove task.
	lv_task_del(task_bpmp_clock);

	// Max clock seems fine. Save it.
	n_cfg.bpmp_clock = 1;
	create_nyx_config_entry(false);
}

static void _nyx_main_menu(lv_theme_t * th)
{
	static lv_style_t no_padding;
	lv_style_copy(&no_padding, &lv_style_transp);
	no_padding.body.padding.hor = 0;

	// Initialize global styles.
	_nyx_set_default_styles(th);

	// Create screen container.
	lv_obj_t *scr = lv_cont_create(NULL, NULL);
	lv_scr_load(scr);
	lv_cont_set_style(scr, th->bg);

	// Create base background and add a custom one if exists.
	lv_obj_t *cnr = lv_cont_create(scr, NULL);
	static lv_style_t base_bg_style;
	lv_style_copy(&base_bg_style, &lv_style_plain_color);
	base_bg_style.body.main_color = th->bg->body.main_color;
	base_bg_style.body.grad_color = base_bg_style.body.main_color;
	lv_cont_set_style(cnr, &base_bg_style);
	lv_obj_set_size(cnr, LV_HOR_RES, LV_VER_RES);

	if (hekate_bg)
	{
		lv_obj_t *img = lv_img_create(cnr, NULL);
		lv_img_set_src(img, hekate_bg);
	}

	// Add tabview page to screen.
	lv_obj_t *tv = lv_tabview_create(scr, NULL);
	if (hekate_bg)
	{
		lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_PR, &tabview_btn_pr);
		lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_TGL_PR, &tabview_btn_tgl_pr);
	}
	lv_tabview_set_sliding(tv, false);
	lv_obj_set_size(tv, LV_HOR_RES, LV_VER_RES);

	// Add all tabs content.
	char version[32];
	char rel = (nyx_str->version >> 24) & 0xFF;
	s_printf(version, "hekate %s%d.%d.%d%c",
			 rel ? "v" : "", nyx_str->version & 0xFF, (nyx_str->version >> 8) & 0xFF, (nyx_str->version >> 16) & 0xFF, rel > 'A' ? rel : 0);
	lv_obj_t *tab_about = lv_tabview_add_tab(tv, version);

	lv_obj_t *tab_home = lv_tabview_add_tab(tv, SYMBOL_HOME" Home");

	lv_obj_t *tab_tools = lv_tabview_add_tab(tv, SYMBOL_TOOLS" Tools");
	lv_page_set_style(tab_tools, LV_PAGE_STYLE_BG, &no_padding);
	lv_page_set_style(tab_tools, LV_PAGE_STYLE_SCRL, &no_padding);

	lv_obj_t *tab_info = lv_tabview_add_tab(tv, SYMBOL_INFO" Console Info");
	lv_page_set_style(tab_info, LV_PAGE_STYLE_BG, &no_padding);
	lv_page_set_style(tab_info, LV_PAGE_STYLE_SCRL, &no_padding);

	lv_obj_t *tab_options = lv_tabview_add_tab(tv, SYMBOL_SETTINGS" Options");

	_create_tab_about(th, tab_about);
	_create_tab_home(th, tab_home);
	create_tab_tools(th, tab_tools);
	create_tab_info(th, tab_info);
	create_tab_options(th, tab_options);

	lv_tabview_set_tab_act(tv, 1, false);

	// Create status bar.
	_create_status_bar(th);

	// Create tasks.
	system_tasks.task.dram_periodic_comp = lv_task_create(minerva_periodic_training, EMC_PERIODIC_TRAIN_MS, LV_TASK_PRIO_HIGHEST, NULL);
	lv_task_ready(system_tasks.task.dram_periodic_comp);

	system_tasks.task.status_bar = lv_task_create(_update_status_bar, 5000, LV_TASK_PRIO_LOW, NULL);
	lv_task_ready(system_tasks.task.status_bar);

	lv_task_create(_check_sd_card_removed, 2000, LV_TASK_PRIO_LOWEST, NULL);

	task_emmc_errors = lv_task_create(_nyx_emmc_issues_warning, 2000, LV_TASK_PRIO_LOWEST, NULL);
	lv_task_ready(task_emmc_errors);

	// Create top level global line separators.
	lv_obj_t *line = lv_cont_create(lv_layer_top(), NULL);

	static lv_style_t line_style;
	lv_style_copy(&line_style, &lv_style_plain_color);

	line_style.body.main_color = LV_COLOR_HEX(0xDDDDDD); // 0x505050
	line_style.body.grad_color = line_style.body.main_color;
	line_style.body.shadow.width = 0;

	lv_cont_set_style(line, &line_style);
	lv_obj_set_size(line, LV_HOR_RES - LV_DPI * 3 / 5, 1);
	lv_obj_set_pos(line, LV_DPI * 3 / 10, 63);

	lv_obj_set_top(line, true);

	line = lv_cont_create(lv_layer_top(), line);
	lv_obj_set_pos(line, LV_DPI * 3 / 10, 656);
	lv_obj_set_top(line, true);

	// Option save button.
	lv_tabview_set_tab_load_action(tv, _show_hide_save_button);

	// Check if Nyx was launched with a function set.
	if (nyx_str->cfg & NYX_CFG_UMS)
	{
		nyx_str->cfg &= ~(NYX_CFG_UMS);
		lv_task_t *task_run_ums = lv_task_create(nyx_run_ums, LV_TASK_ONESHOT, LV_TASK_PRIO_LOWEST, (void *)&nyx_str->cfg);
		lv_task_once(task_run_ums);
	}
	else if (n_cfg.home_screen)
		_create_window_home_launch(NULL);

	if (!n_cfg.timeoffset)
	{
		lv_task_t *task_run_clock = lv_task_create(first_time_clock_edit, LV_TASK_ONESHOT, LV_TASK_PRIO_MID, NULL);
		lv_task_once(task_run_clock);
	}

	if (!n_cfg.bpmp_clock)
		task_bpmp_clock = lv_task_create(first_time_bpmp_clock, 10000, LV_TASK_PRIO_LOWEST, NULL);
}

void nyx_load_and_run()
{
	memset(&system_tasks, 0, sizeof(system_maintenance_tasks_t));

	lv_init();
	gfx_con.fillbg = 1;

	// Initialize framebuffer drawing functions.
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.disp_flush = _disp_fb_flush;
	lv_disp_drv_register(&disp_drv);

	// Initialize Joy-Con.
	if (!n_cfg.jc_disable)
	{
		lv_task_t *task_jc_init_hw = lv_task_create(jc_init_hw, LV_TASK_ONESHOT, LV_TASK_PRIO_LOWEST, NULL);
		lv_task_once(task_jc_init_hw);
	}
	lv_indev_drv_t indev_drv_jc;
	lv_indev_drv_init(&indev_drv_jc);
	indev_drv_jc.type = LV_INDEV_TYPE_POINTER;
	indev_drv_jc.read = _jc_virt_mouse_read;
	memset(&jc_drv_ctx, 0, sizeof(jc_lv_driver_t));
	jc_drv_ctx.indev_jc = lv_indev_drv_register(&indev_drv_jc);
	close_btn = NULL;

	// Initialize touch.
	touch_enabled = touch_power_on();
	lv_indev_drv_t indev_drv_touch;
	lv_indev_drv_init(&indev_drv_touch);
	indev_drv_touch.type = LV_INDEV_TYPE_POINTER;
	indev_drv_touch.read = _fts_touch_read;
	jc_drv_ctx.indev_touch = lv_indev_drv_register(&indev_drv_touch);
	touchpad.touch = false;

	// Initialize temperature sensor.
	tmp451_init();

	// Set hekate theme based on chosen hue.
	lv_theme_t *th = lv_theme_hekate_init(n_cfg.theme_bg, n_cfg.theme_color, NULL);
	lv_theme_set_current(th);

	// Create main menu
	_nyx_main_menu(th);

	jc_drv_ctx.cursor = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_src(jc_drv_ctx.cursor, &touch_cursor);
	lv_obj_set_opa_scale(jc_drv_ctx.cursor, LV_OPA_TRANSP);
	lv_obj_set_opa_scale_enable(jc_drv_ctx.cursor, true);

	// Check if sd card issues.
	if (sd_get_mode() == SD_1BIT_HS25)
	{
		lv_task_t *task_run_sd_errors = lv_task_create(_nyx_sd_card_issues_warning, LV_TASK_ONESHOT, LV_TASK_PRIO_LOWEST, NULL);
		lv_task_once(task_run_sd_errors);
	}

	// Gui loop.
	if (h_cfg.t210b01)
	{
		// Minerva not supported on T210B01 yet. Slight power saving via spinlock.
		while (true)
		{
			lv_task_handler();
			usleep(400);
		}
	}
	else
	{
		// Alternate DRAM frequencies. Saves 280 mW.
		while (true)
		{
			minerva_change_freq(FREQ_1600);  // Takes 295 us.

			lv_task_handler();

			minerva_change_freq(FREQ_800);   // Takes 80 us.
		}
	}
}
