/*
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

/*********************
 *      INCLUDES
 *********************/
#include "lv_theme.h"

#if USE_LV_THEME_HEKATE

/*********************
 *      DEFINES
 *********************/
#define DEF_RADIUS             4
#define COLOR_SHADOW_LIGHT     LV_COLOR_HEX(0xAAAAAA)
#define COLOR_SHADOW_DARK      LV_COLOR_HEX(0x1F1F1F)
#define COLOR_HOS_TURQUOISE    (lv_color_hsv_to_rgb(_hue, 100, 100)) // 0x00FFC9
#define COLOR_HOS_TEAL_LIGHTER (lv_color_hsv_to_rgb(_hue, 100,  93)) // 0x00EDBA
#define COLOR_HOS_TEAL_LIGHT   (lv_color_hsv_to_rgb(_hue, 100,  72)) // 0x00B78F
#define COLOR_HOS_TEAL         (lv_color_hsv_to_rgb(_hue, 100,  64)) // 0x00A273
#define COLOR_HOS_ORANGE       LV_COLOR_HEX(0xFF5500)
#define COLOR_HOS_BG_DARKER    LV_COLOR_HEX(0x1B1B1B)
#define COLOR_HOS_BG_DARK      LV_COLOR_HEX(0x222222)
#define COLOR_HOS_BG           LV_COLOR_HEX(0x2D2D2D)
#define COLOR_HOS_BG_LIGHT     LV_COLOR_HEX(0x3D3D3D)
#define COLOR_HOS_LIGHT_BORDER LV_COLOR_HEX(0x4D4D4D)
#define COLOR_HOS_TXT_WHITE    LV_COLOR_HEX(0xFBFBFB)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_theme_t theme;
static lv_style_t def;

/*Static style definitions*/
static lv_style_t sb;

/*Saved input parameters*/
static uint16_t _hue;
static lv_font_t * _font;

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void basic_init(void)
{
	static lv_style_t bg, panel;

	lv_style_copy(&def, &lv_style_plain); // Initialize the default style.
	def.text.font = _font;
	def.body.radius = DEF_RADIUS;
	def.text.color = COLOR_HOS_TXT_WHITE;
	//def.image.color = COLOR_HOS_TXT_WHITE; //Needed if symbol image.
	//def.image.opa = LV_OPA_COVER;

	lv_style_copy(&bg, &def);
	bg.body.main_color = COLOR_HOS_BG;
	//bg.body.main_color = LV_COLOR_BLACK;
	bg.body.grad_color = bg.body.main_color;
	bg.body.radius = 0;
	bg.body.empty = 1;

	lv_style_copy(&panel, &def);
	panel.body.radius = DEF_RADIUS;
	panel.body.main_color = COLOR_HOS_BG;
	panel.body.grad_color = COLOR_HOS_BG;
	panel.body.border.width = 1;
	panel.body.border.color = COLOR_HOS_LIGHT_BORDER;
	panel.body.border.opa = LV_OPA_COVER;
	panel.body.shadow.color = COLOR_SHADOW_LIGHT;
	panel.body.shadow.type = LV_SHADOW_BOTTOM;
	panel.body.shadow.width = 4;
	panel.body.padding.hor = LV_DPI / 8;
	panel.body.padding.ver = LV_DPI / 8;
	panel.body.padding.inner = LV_DPI / 12;
	//panel.text.color = COLOR_HOS_TXT_WHITE;

	lv_style_copy(&sb, &def);
	sb.body.main_color = LV_COLOR_BLACK;
	sb.body.grad_color = LV_COLOR_BLACK;
	sb.body.opa = LV_OPA_40;
	sb.body.padding.hor = LV_DPI / 25;

	theme.bg = &bg;
	theme.panel = &panel;
}

static void cont_init(void)
{
#if USE_LV_CONT != 0
	static lv_style_t cont;
	lv_style_copy(&cont, theme.panel);
	cont.body.shadow.width = 0;
	cont.body.border.width = 0;

	theme.cont = &cont;
#endif
}

static void btn_init(void)
{
#if USE_LV_BTN != 0
	static lv_style_t rel, pr, tgl_rel, tgl_pr, ina;

	lv_style_copy(&rel, &def);
	rel.body.main_color = COLOR_HOS_BG_LIGHT;
	rel.body.grad_color = rel.body.main_color;
	rel.body.radius = 6;
	rel.body.padding.hor = LV_DPI / 3;
	rel.body.padding.ver = LV_DPI / 6;
	rel.body.padding.inner = LV_DPI / 10;
	rel.body.shadow.color = COLOR_SHADOW_DARK;
	rel.body.shadow.type = LV_SHADOW_BOTTOM;
	rel.body.shadow.width = 6;
	rel.body.border.width = 0;
	rel.body.border.color = COLOR_HOS_BG_LIGHT;
	rel.body.border.part = LV_BORDER_FULL;
	//rel.text.color = COLOR_HOS_TXT_WHITE;

	lv_style_copy(&pr, &rel);
	pr.body.main_color = LV_COLOR_HEX(0x505050);
	pr.body.grad_color = pr.body.main_color;
	pr.body.shadow.width = 0;
	pr.body.border.color = COLOR_HOS_TEAL_LIGHTER;
	pr.text.color = COLOR_HOS_TURQUOISE;
	pr.body.border.width = 4;

	lv_style_copy(&tgl_rel, &rel);
	tgl_rel.body.border.color = COLOR_HOS_TEAL_LIGHTER;
	tgl_rel.body.border.width = 4;

	lv_style_copy(&tgl_pr, &tgl_rel);
	tgl_pr.body.main_color = LV_COLOR_HEX(0x505050);
	tgl_pr.body.grad_color = tgl_pr.body.main_color;
	tgl_pr.text.color = COLOR_HOS_TURQUOISE;
	tgl_pr.body.shadow.width = 0;

	lv_style_copy(&ina, &rel);
	ina.body.main_color = COLOR_HOS_BG_DARK;
	ina.body.grad_color = ina.body.main_color;
	//ina.body.shadow.width = 0;
	ina.text.color = LV_COLOR_HEX(0x888888);
	ina.body.border.width = 4;

	theme.btn.rel = &rel;
	theme.btn.pr = &pr;
	theme.btn.tgl_rel = &tgl_rel;
	theme.btn.tgl_pr =  &tgl_pr;
	theme.btn.ina =  &ina;
#endif
}


static void label_init(void)
{
#if USE_LV_LABEL != 0
	static lv_style_t prim, sec, hint;

	lv_style_copy(&prim, &def);
	prim.text.font = _font;
	prim.text.color = COLOR_HOS_TXT_WHITE;

	lv_style_copy(&sec, &prim);
	sec.text.color = COLOR_HOS_ORANGE;

	lv_style_copy(&hint, &prim);
	hint.text.color = LV_COLOR_HEX(0xCCCCCC);

	theme.label.prim = &prim;
	theme.label.sec = &sec;
	theme.label.hint = &hint;
#endif
}

static void img_init(void)
{
#if USE_LV_IMG != 0
	static lv_style_t img_light, img_dark;
	lv_style_copy(&img_light, &def);
	img_light.image.color = LV_COLOR_WHITE;
	img_light.image.intense = LV_OPA_80;

	lv_style_copy(&img_dark, &def);
	img_dark.image.color = COLOR_HOS_BG_DARKER;
	img_dark.image.intense = LV_OPA_80;


	theme.img.light = &def;
	theme.img.dark = &def;
#endif
}

static void line_init(void)
{
#if USE_LV_LINE != 0
	static lv_style_t line;
	lv_style_copy(&line, &def);
	line.line.color = LV_COLOR_HEX(0x656565);
	theme.line.decor = &line;
#endif
}

static void led_init(void)
{
#if USE_LV_LED != 0
	static lv_style_t led;
	lv_style_copy(&led, &def);
	led.body.shadow.width = LV_DPI / 10;
	led.body.radius = LV_RADIUS_CIRCLE;
	led.body.border.width = LV_DPI / 30;
	led.body.border.opa = LV_OPA_30;
	led.body.main_color = lv_color_hsv_to_rgb(_hue, 100, 100);
	led.body.grad_color = lv_color_hsv_to_rgb(_hue, 100, 100);
	led.body.border.color = lv_color_hsv_to_rgb(_hue, 60, 60);
	led.body.shadow.color = lv_color_hsv_to_rgb(_hue, 100, 100);

	theme.led = &led;
#endif
}

static void bar_init(void)
{
#if USE_LV_BAR
	static lv_style_t bar_bg, bar_indic;

	lv_style_copy(&bar_bg, &def);
	bar_bg.body.main_color = COLOR_HOS_LIGHT_BORDER;
	bar_bg.body.grad_color = bar_bg.body.main_color;
	bar_bg.body.radius = 3;
	bar_bg.body.border.width = 0;
	bar_bg.body.padding.hor = LV_DPI / 12;
	bar_bg.body.padding.ver = LV_DPI / 12;

	lv_style_copy(&bar_indic, &bar_bg);
	bar_indic.body.main_color = COLOR_HOS_TURQUOISE;
	bar_indic.body.grad_color = bar_indic.body.main_color;
	bar_indic.body.padding.hor = 0;
	bar_indic.body.padding.ver = 0;

	theme.bar.bg = &bar_bg;
	theme.bar.indic = &bar_indic;
#endif
}

static void slider_init(void)
{
#if USE_LV_SLIDER != 0
	static lv_style_t knob;
	static lv_style_t slide_bar;

	lv_style_copy(&knob, &def);
	knob.body.radius = LV_RADIUS_CIRCLE;
	knob.body.border.width = 0;
	knob.body.main_color = theme.bar.indic->body.main_color;
	knob.body.grad_color = knob.body.main_color;

	lv_style_copy(&slide_bar, theme.bar.indic);
	slide_bar.body.main_color = COLOR_HOS_TEAL_LIGHT;
	slide_bar.body.grad_color = slide_bar.body.main_color;

	theme.slider.bg = theme.bar.bg;
	theme.slider.indic = &slide_bar;
	theme.slider.knob = &knob;
#endif
}

static void sw_init(void)
{
#if USE_LV_SW != 0
	static lv_style_t sw_bg, sw_indic, sw_knob_off, sw_knob_on;
	lv_style_copy(&sw_bg, theme.slider.bg);
	sw_bg.body.radius = LV_RADIUS_CIRCLE;

	lv_style_copy(&sw_indic, theme.slider.bg);
	sw_indic.body.radius = LV_RADIUS_CIRCLE;

	lv_style_copy(&sw_knob_on, theme.slider.knob);

	lv_style_copy(&sw_knob_off, &sw_knob_on);
	sw_knob_off.body.main_color = LV_COLOR_HEX(0xDADADA);
	sw_knob_off.body.grad_color = sw_knob_off.body.main_color;
	sw_knob_off.body.border.width = 1;
	sw_knob_off.body.border.color = LV_COLOR_HEX(0x999999);
	sw_knob_off.body.border.opa = LV_OPA_COVER;

	theme.sw.bg = &sw_bg;
	theme.sw.indic = &sw_indic;
	theme.sw.knob_off = &sw_knob_off;
	theme.sw.knob_on = &sw_knob_on;
#endif
}


static void lmeter_init(void)
{
#if USE_LV_LMETER != 0
	static lv_style_t lmeter;
	lv_style_copy(&lmeter, &def);
	lmeter.body.main_color = lv_color_hsv_to_rgb(_hue, 75, 90);
	lmeter.body.grad_color = lmeter.body.main_color;
	lmeter.body.padding.hor = LV_DPI / 10; // Scale line length.
	lmeter.line.color = LV_COLOR_HEX(0x999999);
	lmeter.line.width = 2;

	theme.lmeter = &lmeter;
#endif
}

static void gauge_init(void)
{
#if USE_LV_GAUGE != 0

	static lv_style_t gauge;
	lv_style_copy(&gauge, &def);
	gauge.body.main_color = lv_color_hsv_to_rgb(_hue, 10, 60);
	gauge.body.grad_color = gauge.body.main_color;
	gauge.body.padding.hor = LV_DPI / 16; // Scale line length.
	gauge.body.padding.inner = LV_DPI / 8;
	gauge.body.border.color = LV_COLOR_HEX(0x999999);
	gauge.text.color = LV_COLOR_HEX(0xDDDDDD);
	gauge.line.width = 3;
	gauge.line.color = lv_color_hsv_to_rgb(_hue, 95, 70);

	theme.gauge = &gauge;
#endif
}

static void arc_init(void)
{
#if USE_LV_ARC != 0

	static lv_style_t arc;
	lv_style_copy(&arc, &def);
	arc.line.width = 10;
	arc.line.color = lv_color_hsv_to_rgb(_hue, 90, 90);

	/*For prelaoder*/
	arc.body.border.width = 10;
	arc.body.border.color = lv_color_hsv_to_rgb(_hue, 30, 90);
	arc.body.padding.hor = 0;
	arc.body.padding.ver = 0;

	theme.arc = &arc;
#endif
}

static void preload_init(void)
{
#if USE_LV_PRELOAD != 0

	theme.preload = theme.arc;
#endif
}

static void chart_init(void)
{
#if USE_LV_CHART
	theme.chart = theme.panel;
#endif
}

static void calendar_init(void)
{
#if USE_LV_CALENDAR
	static lv_style_t ina_days;
	lv_style_copy(&ina_days, &def);
	ina_days.text.color = lv_color_hsv_to_rgb(_hue, 0, 70);

	static lv_style_t high_days;
	lv_style_copy(&high_days, &def);
	high_days.text.color = lv_color_hsv_to_rgb(_hue, 80, 90);

	static lv_style_t week_box;
	lv_style_copy(&week_box, &def);
	week_box.body.main_color = lv_color_hsv_to_rgb(_hue, 40, 100);
	week_box.body.grad_color = lv_color_hsv_to_rgb(_hue, 40, 100);
	week_box.body.padding.ver = LV_DPI / 20;
	week_box.body.padding.hor = theme.panel->body.padding.hor;
	week_box.body.border.color = theme.panel->body.border.color;
	week_box.body.border.width = theme.panel->body.border.width;
	week_box.body.border.part = LV_BORDER_LEFT | LV_BORDER_RIGHT;
	week_box.body.radius = 0;

	static lv_style_t today_box;
	lv_style_copy(&today_box, &def);
	today_box.body.main_color = LV_COLOR_WHITE;
	today_box.body.grad_color = LV_COLOR_WHITE;
	today_box.body.padding.ver = LV_DPI / 20;
	today_box.body.radius = 0;

	theme.calendar.bg = theme.panel;
	theme.calendar.header = &lv_style_transp;
	theme.calendar.inactive_days = &ina_days;
	theme.calendar.highlighted_days = &high_days;
	theme.calendar.week_box = &week_box;
	theme.calendar.today_box = &today_box;
#endif
}

static void cb_init(void)
{
#if USE_LV_CB != 0
	static lv_style_t rel, pr, tgl_rel, tgl_pr, ina;
	lv_style_copy(&rel, theme.panel);
	rel.body.shadow.type = LV_SHADOW_FULL;
	rel.body.shadow.width = 3;

	lv_style_copy(&pr, &rel);
	pr.body.main_color = LV_COLOR_HEX(0xCCCCCC);
	pr.body.grad_color = pr.body.main_color;
	pr.body.shadow.width = 3;

	lv_style_copy(&tgl_rel, &rel);
	tgl_rel.body.main_color = lv_color_hsv_to_rgb(_hue, 75, 85);
	tgl_rel.body.grad_color = tgl_rel.body.main_color;
	tgl_rel.body.shadow.width = 0;

	lv_style_copy(&tgl_pr, &tgl_rel);
	tgl_pr.body.main_color = lv_color_hsv_to_rgb(_hue, 75, 65);
	tgl_pr.body.grad_color = tgl_pr.body.main_color;

	lv_style_copy(&ina, theme.btn.ina);

	theme.cb.bg = &lv_style_transp;
	theme.cb.box.rel = &rel;
	theme.cb.box.pr = &pr;
	theme.cb.box.tgl_rel = &tgl_rel;
	theme.cb.box.tgl_pr = &tgl_pr;
	theme.cb.box.ina = &ina;
#endif
}


static void btnm_init(void)
{
#if USE_LV_BTNM
	static lv_style_t bg, rel, pr, tgl_rel, tgl_pr, ina;

	lv_style_copy(&bg, theme.panel);
	bg.body.padding.hor = 0;
	bg.body.padding.ver = 0;
	bg.body.padding.inner = 0;
	bg.text.color = LV_COLOR_HEX(0x555555);

	lv_style_copy(&rel, theme.panel);
	rel.body.border.part = LV_BORDER_FULL | LV_BORDER_INTERNAL;
	rel.body.border.width = 1;
	rel.body.border.color = LV_COLOR_HEX(0xBBBBBB);
	rel.body.empty = 1;
	rel.body.shadow.width = 0;

	lv_style_copy(&pr, &rel);
	pr.glass = 0;
	pr.body.main_color = LV_COLOR_HEX(0xDDDDDD);
	pr.body.grad_color = pr.body.main_color;
	pr.body.border.width = 0;
	pr.body.empty = 0;

	lv_style_copy(&tgl_rel, &pr);
	tgl_rel.body.main_color = lv_color_hsv_to_rgb(_hue, 90, 70);
	tgl_rel.body.grad_color = tgl_rel.body.main_color;
	tgl_rel.text.color = lv_color_hsv_to_rgb(_hue, 5, 95);

	lv_style_copy(&tgl_pr, &tgl_rel);
	tgl_pr.body.main_color = lv_color_hsv_to_rgb(_hue, 95, 65);
	tgl_pr.body.grad_color = tgl_pr.body.main_color;
	tgl_pr.body.border.width = 0;

	lv_style_copy(&ina, theme.btn.ina);

	theme.btnm.bg = &bg;
	theme.btnm.btn.rel = &rel;
	theme.btnm.btn.pr = &pr;
	theme.btnm.btn.tgl_rel = &tgl_rel;
	theme.btnm.btn.tgl_pr = &tgl_pr;
	theme.btnm.btn.ina = &ina;
#endif
}

static void kb_init(void)
{
#if USE_LV_KB

	static lv_style_t bg, rel;

	lv_style_copy(&bg, theme.btnm.bg);
	bg.text.color = LV_COLOR_HEX(0xCCCCCC);
	bg.body.border.width = 0;
	bg.body.radius = 0;
	bg.body.shadow.color = COLOR_SHADOW_DARK;
	bg.body.shadow.type = LV_SHADOW_BOTTOM;
	bg.body.shadow.width = 4;

	lv_style_copy(&rel, &lv_style_transp);
	rel.text.font = _font;

	theme.kb.bg = &bg;
	theme.kb.btn.rel = &rel;
	theme.kb.btn.pr = theme.btnm.btn.pr;
	theme.kb.btn.tgl_rel = theme.btnm.btn.tgl_rel;
	theme.kb.btn.tgl_pr = theme.btnm.btn.tgl_pr;
	theme.kb.btn.ina = theme.btnm.btn.ina;
#endif

}

static void mbox_init(void)
{
#if USE_LV_MBOX
	static lv_style_t bg;

	lv_style_copy(&bg, theme.panel);
	bg.body.main_color = LV_COLOR_HEX(0x464646);
	bg.body.grad_color = bg.body.main_color;
	bg.body.shadow.color = COLOR_HOS_BG;
	bg.body.shadow.type = LV_SHADOW_FULL;
	bg.body.shadow.width = 8;

	bg.body.padding.hor = LV_DPI * 3 / 6;
	bg.body.padding.ver = LV_DPI / 4;
	bg.body.padding.inner = LV_DPI / 3;

	theme.mbox.bg = &bg;
	theme.mbox.btn.bg = &lv_style_transp;
	theme.mbox.btn.rel = theme.btn.rel;
	theme.mbox.btn.pr = theme.btn.pr;
#endif
}

static void page_init(void)
{
#if USE_LV_PAGE
	theme.page.bg = theme.panel;
	theme.page.scrl = &lv_style_transp;
	theme.page.sb = &sb;
#endif
}

static void ta_init(void)
{
#if USE_LV_TA
	static lv_style_t panel, oneline;

	lv_style_copy(&panel, theme.panel);
	panel.body.border.width = 0;
	panel.body.shadow.color = COLOR_SHADOW_DARK;
	panel.body.shadow.type = LV_SHADOW_FULL;
	panel.body.shadow.width = 3;

	lv_style_copy(&oneline, &def);
	oneline.body.empty = 1;
	oneline.body.radius = 0;
	oneline.body.border.part = LV_BORDER_BOTTOM;
	oneline.body.border.width = 3;
	oneline.body.border.color = LV_COLOR_HEX(0x555555);
	oneline.body.border.opa = LV_OPA_COVER;
	oneline.text.color = LV_COLOR_HEX(0x888888);

	theme.ta.area = &panel;
	theme.ta.oneline = &oneline;
	theme.ta.cursor = NULL; // Let library to calculate the cursor's style.
	theme.ta.sb = &sb;
#endif
}

static void spinbox_init(void)
{
#if USE_LV_SPINBOX
	theme.spinbox.bg= theme.panel;
	theme.spinbox.cursor = theme.ta.cursor;
	theme.spinbox.sb = theme.ta.sb;
#endif
}

static void list_init(void)
{
#if USE_LV_LIST != 0

	static lv_style_t list_bg, rel, pr, tgl_rel, tgl_pr, ina;

	lv_style_copy(&list_bg, theme.panel);
	list_bg.body.padding.hor = 0;
	list_bg.body.padding.ver = 0;
	list_bg.body.padding.inner = 0;
	list_bg.body.shadow.width = 0;

	lv_style_copy(&rel, &lv_style_transp);
	rel.body.padding.hor = LV_DPI / 8;
	rel.body.padding.ver = LV_DPI / 6;
	rel.body.radius = 0;
	rel.body.border.color = LV_COLOR_HEX(0x444444);
	rel.body.border.width = 1;
	rel.body.border.part = LV_BORDER_BOTTOM;

	lv_style_copy(&pr, &rel);
	pr.glass = 0;
	pr.body.main_color = LV_COLOR_HEX(0x505050);
	pr.body.grad_color = pr.body.main_color;
	//pr.body.border.width = 1;
	pr.body.empty = 0;
	//pr.body.radius = 0;
   // pr.text.font = _font;

	lv_style_copy(&tgl_rel, &pr);
	tgl_rel.body.main_color = COLOR_HOS_BG_LIGHT;
	tgl_rel.body.grad_color = tgl_rel.body.main_color;
	//tgl_rel.text.color = lv_color_hsv_to_rgb(_hue, 5, 95);
	tgl_rel.text.color = COLOR_HOS_TEAL_LIGHTER;

	lv_style_copy(&tgl_pr, &tgl_rel);
	tgl_pr.body.main_color = LV_COLOR_HEX(0x505050);
	tgl_pr.body.grad_color = tgl_pr.body.main_color;
	tgl_pr.body.border.width = 0;

	lv_style_copy(&ina, &pr);
	ina.body.main_color = COLOR_HOS_BG_DARK;
	ina.body.grad_color = ina.body.main_color;

	theme.list.sb = &sb;
	theme.list.bg = &list_bg;
	theme.list.scrl = &lv_style_transp_tight;
	theme.list.btn.rel = &rel;
	theme.list.btn.pr = &pr;
	theme.list.btn.tgl_rel = &tgl_rel;
	theme.list.btn.tgl_pr = &tgl_pr;
	theme.list.btn.ina = &ina;
#endif
}

static void ddlist_init(void)
{
#if USE_LV_DDLIST != 0
	static lv_style_t bg, sel;
	lv_style_copy(&bg, theme.panel);
	bg.body.padding.hor = LV_DPI / 6;
	//bg.body.padding.ver = LV_DPI / 6;
	bg.body.radius = 0;
	bg.body.shadow.width = 0;
	bg.body.border.width = 0;
	bg.text.line_space = LV_DPI / 8;
	bg.text.color = COLOR_HOS_TURQUOISE;

	lv_style_copy(&sel, &bg);
	sel.body.main_color = COLOR_HOS_BG_LIGHT;
	sel.body.grad_color = sel.body.main_color;

	theme.ddlist.bg = &bg;
	theme.ddlist.bgo = &bg;
	theme.ddlist.pr = &sel;
	theme.ddlist.sel = &sel;
	theme.ddlist.sb = &sb;
#endif
}

static void roller_init(void)
{
#if USE_LV_ROLLER != 0
	static lv_style_t roller_bg, roller_sel;

	lv_style_copy(&roller_bg, &lv_style_transp);
	roller_bg.body.padding.hor = LV_DPI / 6;
	roller_bg.body.padding.ver = LV_DPI / 6;
	roller_bg.text.line_space = LV_DPI / 8;
	roller_bg.text.font = _font;
	roller_bg.glass = 0;
	roller_bg.text.color = LV_COLOR_HEX(0x444444);

	lv_style_copy(&roller_sel, &roller_bg);
	roller_sel.text.color = COLOR_HOS_TURQUOISE;

	theme.roller.bg = &roller_bg;
	theme.roller.sel = &roller_sel;
#endif
}

static void tabview_init(void)
{
#if USE_LV_TABVIEW != 0
	static lv_style_t indic, btn_bg, rel, pr, tgl_rel, tgl_pr;

	lv_style_copy(&indic, &def);
	indic.body.main_color = COLOR_HOS_TURQUOISE;
	indic.body.grad_color = indic.body.main_color;
	indic.body.radius = 0;
	indic.body.border.width = 0;
	indic.body.padding.inner = LV_DPI / 20;
	indic.body.opa = LV_OPA_0;

	lv_style_copy(&btn_bg, &def);
	btn_bg.body.main_color = COLOR_HOS_BG;
	btn_bg.body.grad_color = btn_bg.body.main_color;
	btn_bg.body.radius = 0;
	btn_bg.body.empty = 1;
	btn_bg.body.border.width = 0;
	btn_bg.body.border.color = LV_COLOR_HEX(0xDDDDDD);
	btn_bg.body.border.part = LV_BORDER_BOTTOM;
	btn_bg.body.border.opa = LV_OPA_COVER;
	btn_bg.body.shadow.width = 0;
	btn_bg.body.shadow.color = COLOR_SHADOW_LIGHT;
	btn_bg.body.shadow.type = LV_SHADOW_BOTTOM;
	btn_bg.body.padding.inner = 0;
	btn_bg.body.padding.hor = 0;
	btn_bg.body.padding.ver = 0;
	btn_bg.text.color = COLOR_HOS_TXT_WHITE;

	lv_style_copy(&rel, &lv_style_transp);
	rel.body.padding.ver = LV_DPI * 4 / 23;
	rel.text.font = _font;

	lv_style_copy(&pr, &def);
	pr.body.main_color = COLOR_HOS_BG_LIGHT;
	pr.body.grad_color = pr.body.main_color;
	pr.body.border.width = 0;
	pr.body.empty = 0;
	pr.body.radius = 0;
	pr.body.border.color = LV_COLOR_HEX(0x888888);
	pr.body.border.part = LV_BORDER_BOTTOM;
	pr.body.border.opa = LV_OPA_COVER;
	pr.text.color = COLOR_HOS_TURQUOISE;

	lv_style_copy(&tgl_rel, &lv_style_transp);
	tgl_rel.glass = 0;
	tgl_rel.text.font = _font;
	tgl_rel.text.color = COLOR_HOS_TURQUOISE;

	lv_style_copy(&tgl_pr, &def);
	tgl_pr.body.main_color = COLOR_HOS_BG_LIGHT;
	tgl_pr.body.grad_color = tgl_pr.body.main_color;
	tgl_pr.body.border.width = 0;
	tgl_pr.body.empty = 0;
	tgl_pr.body.radius = 0;
	tgl_pr.text.color = COLOR_HOS_TURQUOISE;

	theme.tabview.bg = theme.bg;
	theme.tabview.indic = &indic;
	theme.tabview.btn.bg = &btn_bg;
	theme.tabview.btn.rel = &rel;
	theme.tabview.btn.pr = &pr;
	theme.tabview.btn.tgl_rel = &tgl_rel;
	theme.tabview.btn.tgl_pr = &tgl_pr;
#endif
}

static void tileview_init(void)
{
#if USE_LV_TILEVIEW != 0
	theme.tileview.bg = &lv_style_transp_tight;
	theme.tileview.scrl = &lv_style_transp_tight;
	theme.tileview.sb = theme.page.sb;
#endif
}

static void table_init(void)
{
#if USE_LV_TABLE != 0
	static lv_style_t cell;
	lv_style_copy(&cell, theme.panel);
	cell.body.radius = 0;
	cell.body.border.width = 1;
	cell.body.padding.hor = LV_DPI / 12;
	cell.body.padding.ver = LV_DPI / 12;

	theme.table.bg = &lv_style_transp_tight;
	theme.table.cell = &cell;
#endif
}

static void win_init(void)
{
#if USE_LV_WIN != 0
	static lv_style_t header, rel, pr;

	lv_style_copy(&header, &def);
	header.body.main_color = COLOR_HOS_BG;
	header.body.grad_color = header.body.main_color;
	header.body.radius = 0;
	header.body.border.width = 0;
	header.body.border.color = LV_COLOR_HEX(0xDDDDDD);
	header.body.border.part = LV_BORDER_BOTTOM;
	header.body.border.opa = LV_OPA_COVER;
	header.body.shadow.width = 0;
	header.body.shadow.color = COLOR_SHADOW_LIGHT;
	header.body.shadow.type = LV_SHADOW_BOTTOM;
	header.body.padding.inner = 0;
	header.body.padding.hor = 8;
	header.body.padding.ver = 0;
	//header.text.color = COLOR_HOS_TXT_WHITE;

	lv_style_copy(&rel, theme.btn.rel);
	rel.body.radius = 0;
	rel.body.opa = LV_OPA_0;
	rel.body.border.width = 0;

	lv_style_copy(&pr, theme.btn.pr);
	pr.body.radius = 0;
	pr.body.border.width = 0;

	theme.win.bg = theme.panel;
	theme.win.sb = &sb;
	theme.win.header = &header;
	theme.win.content.bg = &lv_style_transp;
	theme.win.content.scrl = &lv_style_transp;
	theme.win.btn.rel = &rel;
	theme.win.btn.pr = &pr;
#endif
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/



/**
 * Initialize the hekate theme
 * @param hue [0..360] hue value from HSV color space to define the theme's base color
 * @param font pointer to a font (NULL to use the default)
 * @return pointer to the initialized theme
 */
lv_theme_t * lv_theme_hekate_init(uint16_t hue, lv_font_t * font)
{
	if(font == NULL) font = LV_FONT_DEFAULT;

	_hue = hue;
	_font = font;

	/*For backward compatibility initialize all theme elements with a default style */
	uint16_t i;
	lv_style_t ** style_p = (lv_style_t **) &theme;
	for(i = 0; i < sizeof(lv_theme_t) / sizeof(lv_style_t *); i++) {
		*style_p = &def;
		style_p++;
	}

	basic_init();
	cont_init();
	btn_init();
	label_init();
	img_init();
	line_init();
	led_init();
	bar_init();
	slider_init();
	sw_init();
	lmeter_init();
	gauge_init();
	chart_init();
	arc_init();
	preload_init();
	calendar_init();
	cb_init();
	btnm_init();
	kb_init();
	mbox_init();
	page_init();
	ta_init();
	spinbox_init();
	list_init();
	ddlist_init();
	roller_init();
	tabview_init();
	tileview_init();
	table_init();
	win_init();

	return &theme;
}

/**
 * Get a pointer to the theme
 * @return pointer to the theme
 */
lv_theme_t * lv_theme_get_hekate(void)
{
	return &theme;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif

