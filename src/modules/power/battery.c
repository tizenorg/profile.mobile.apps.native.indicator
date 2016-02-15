/*
 *  Indicator
 *
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <system_settings.h>
#include <device/display.h>
#include <device/battery.h>
#include <device/callback.h>
#include <runtime_info.h>
#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "indicator_gui.h"
#include "util.h"
#include "box.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED9
#define MODULE_NAME		"battery"
#define MODULE_NAME_DIGIT	"battery"
#define MODULE_NAME_DIGIT2	"battery"
#define TIMER_INTERVAL	0.7


static int register_battery_module(void *data);
static int unregister_battery_module(void);
static int wake_up_cb(void *data);
static void _resize_battery_digits_icons_box(void);


icon_s battery = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.area = INDICATOR_ICON_AREA_FIXED,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,0,BATTERY_ICON_WIDTH,BATTERY_ICON_HEIGHT},
	.obj_exist = EINA_FALSE,
	.init = register_battery_module,
	.fini = unregister_battery_module,
	.wake_up = wake_up_cb,
	.digit_area = -1
};

icon_s digit = {
	.type = INDICATOR_DIGIT_ICON,
	.name = MODULE_NAME_DIGIT,
	.priority = ICON_PRIORITY,
	.area = INDICATOR_ICON_AREA_FIXED,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,0,7,10},
	.obj_exist = EINA_FALSE,
	.wake_up = wake_up_cb,
	.digit_area = DIGIT_UNITY
};

icon_s digit_additional = {
	.type = INDICATOR_DIGIT_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.area = INDICATOR_ICON_AREA_FIXED,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,0,7,10},
	.obj_exist = EINA_FALSE,
	.wake_up = wake_up_cb,
	.digit_area = DIGIT_DOZENS_UNITY
};

enum {
	INDICATOR_CLOCK_MODE_12H = 0,
	INDICATOR_CLOCK_MODE_24H,
	INDICATOR_CLOCK_MODE_MAX
};


enum {
	BATTERY_ICON_WIDTH_12H,
	BATTERY_ICON_WIDTH_24H,
	BATTERY_ICON_WIDTH_NUM
};

enum {
	BATTERY_LEVEL_8,
	BATTERY_LEVEL_20,
};

enum {
	LEVEL_MIN = 0,
	LEVEL_0 = LEVEL_MIN,	/* 0% */
	LEVEL_1,		/* 1  ~   5% */
	LEVEL_2,		/* 6  ~  10% */
	LEVEL_3,		/* 11 ~  15% */
	LEVEL_4,		/* 16 ~  20% */
	LEVEL_5,		/* 21 ~  25 % */
	LEVEL_6,		/* 25 ~  30 % */
	LEVEL_7,		/* 31 ~  35 % */
	LEVEL_8,		/* 36 ~  40 % */
	LEVEL_9,		/* 41 ~  45 % */
	LEVEL_10,		/* 46 ~  50 % */
	LEVEL_11,		/* 51 ~  55 % */
	LEVEL_12,		/* 56 ~  60 % */
	LEVEL_13,		/* 61 ~  65 % */
	LEVEL_14,		/* 66 ~  70 % */
	LEVEL_15,		/* 71 ~  75 % */
	LEVEL_16,		/* 76 ~  80 % */
	LEVEL_17,		/* 81 ~  85 % */
	LEVEL_18,		/* 86 ~  90 % */
	LEVEL_19,		/* 91 ~  95 % */
	LEVEL_20,		/* 96 ~  100 % */
	LEVEL_MAX = LEVEL_20,
	LEVEL_NUM,
	LEVEL_FULL
};

enum {
	LEVEL_PERCENTAGE_MIN = 0,
	LEVEL_PERCENTAGE_0 = LEVEL_PERCENTAGE_MIN,	/* 0% */
	LEVEL_PERCENTAGE_1,		/* 1  ~  10% */
	LEVEL_PERCENTAGE_2,		/* 11 ~  20% */
	LEVEL_PERCENTAGE_3,		/* 21 ~  30% */
	LEVEL_PERCENTAGE_4,		/* 31 ~  40% */
	LEVEL_PERCENTAGE_5,		/* 41 ~  50 % */
	LEVEL_PERCENTAGE_6,		/* 51 ~  60 % */
	LEVEL_PERCENTAGE_7,		/* 61 ~  70 % */
	LEVEL_PERCENTAGE_8,		/* 71 ~  80 % */
	LEVEL_PERCENTAGE_9,		/* 81 ~  90 % */
	LEVEL_PERCENTAGE_10,	/* 91 ~  100 % */
	LEVEL_PERCENTAGE_MAX = LEVEL_PERCENTAGE_10,
	LEVEL_PERCENTAGE_NUM
};

enum {
	BATTERY_PERCENT_1_DIGIT = 0,
	BATTERY_PERCENT_2_DIGITS,
	BATTERY_PERCENT_DIGITS
};

enum {
	BATTERY_ETC_STATE_FULL,
	BATTERY_ETC_STATE_MAX,
};

static int batt_full = 0;

static const char *icon_path[LEVEL_NUM] = {
	[LEVEL_0] = "Power/B03_stat_sys_battery_4.png",
	[LEVEL_1] = "Power/B03_stat_sys_battery_5.png",
	[LEVEL_2] = "Power/B03_stat_sys_battery_10.png",
	[LEVEL_3] = "Power/B03_stat_sys_battery_15.png",
	[LEVEL_4] = "Power/B03_stat_sys_battery_20.png",
	[LEVEL_5] = "Power/B03_stat_sys_battery_25.png",
	[LEVEL_6] = "Power/B03_stat_sys_battery_30.png",
	[LEVEL_7] = "Power/B03_stat_sys_battery_35.png",
	[LEVEL_8] = "Power/B03_stat_sys_battery_40.png",
	[LEVEL_9] = "Power/B03_stat_sys_battery_45.png",
	[LEVEL_10] = "Power/B03_stat_sys_battery_50.png",
	[LEVEL_11] = "Power/B03_stat_sys_battery_55.png",
	[LEVEL_12] = "Power/B03_stat_sys_battery_60.png",
	[LEVEL_13] = "Power/B03_stat_sys_battery_65.png",
	[LEVEL_14] = "Power/B03_stat_sys_battery_70.png",
	[LEVEL_15] = "Power/B03_stat_sys_battery_75.png",
	[LEVEL_16] = "Power/B03_stat_sys_battery_80.png",
	[LEVEL_17] = "Power/B03_stat_sys_battery_85.png",
	[LEVEL_18] = "Power/B03_stat_sys_battery_90.png",
	[LEVEL_19] = "Power/B03_stat_sys_battery_95.png",
	[LEVEL_20] = "Power/B03_stat_sys_battery_100.png"
};

static const char *charging_icon_path[LEVEL_NUM] = {
	[LEVEL_0] = "Power/B03_stat_sys_battery_charge_anim4.png",
	[LEVEL_1] = "Power/B03_stat_sys_battery_charge_anim5.png",
	[LEVEL_2] = "Power/B03_stat_sys_battery_charge_anim10.png",
	[LEVEL_3] = "Power/B03_stat_sys_battery_charge_anim15.png",
	[LEVEL_4] = "Power/B03_stat_sys_battery_charge_anim20.png",
	[LEVEL_5] = "Power/B03_stat_sys_battery_charge_anim25.png",
	[LEVEL_6] = "Power/B03_stat_sys_battery_charge_anim30.png",
	[LEVEL_7] = "Power/B03_stat_sys_battery_charge_anim35.png",
	[LEVEL_8] = "Power/B03_stat_sys_battery_charge_anim40.png",
	[LEVEL_9] = "Power/B03_stat_sys_battery_charge_anim45.png",
	[LEVEL_10] = "Power/B03_stat_sys_battery_charge_anim50.png",
	[LEVEL_11] = "Power/B03_stat_sys_battery_charge_anim55.png",
	[LEVEL_12] = "Power/B03_stat_sys_battery_charge_anim60.png",
	[LEVEL_13] = "Power/B03_stat_sys_battery_charge_anim65.png",
	[LEVEL_14] = "Power/B03_stat_sys_battery_charge_anim70.png",
	[LEVEL_15] = "Power/B03_stat_sys_battery_charge_anim75.png",
	[LEVEL_16] = "Power/B03_stat_sys_battery_charge_anim80.png",
	[LEVEL_17] = "Power/B03_stat_sys_battery_charge_anim85.png",
	[LEVEL_18] = "Power/B03_stat_sys_battery_charge_anim90.png",
	[LEVEL_19] = "Power/B03_stat_sys_battery_charge_anim95.png",
	[LEVEL_20] = "Power/B03_stat_sys_battery_charge_anim100.png"
};
#if 0
static const char *percentage_bg_icon_path[BATTERY_PERCENT_DIGITS] = {
	[BATTERY_PERCENT_1_DIGIT] = "Power/battery_text/B03_stat_sys_battery_bg_1.png",
	[BATTERY_PERCENT_2_DIGITS] = "Power/battery_text/B03_stat_sys_battery_bg_2.png"
};
#endif
static const char *percentage_battery_digit_icon_path[11] = {
	[0]  = "Power/battery_text/B03_stat_sys_battery_num_0.png",
	[1]  = "Power/battery_text/B03_stat_sys_battery_num_1.png",
	[2]  = "Power/battery_text/B03_stat_sys_battery_num_2.png",
	[3]  = "Power/battery_text/B03_stat_sys_battery_num_3.png",
	[4]  = "Power/battery_text/B03_stat_sys_battery_num_4.png",
	[5]  = "Power/battery_text/B03_stat_sys_battery_num_5.png",
	[6]  = "Power/battery_text/B03_stat_sys_battery_num_6.png",
	[7]  = "Power/battery_text/B03_stat_sys_battery_num_7.png",
	[8]  = "Power/battery_text/B03_stat_sys_battery_num_8.png",
	[9]  = "Power/battery_text/B03_stat_sys_battery_num_9.png",
	[10] = "Power/battery_text/B03_stat_sys_battery_num_100.png"
};

static const char *percentage_battery_icon_path[LEVEL_PERCENTAGE_NUM] = {
	[LEVEL_PERCENTAGE_0] = "Power/B03_stat_sys_battery_percent_0.png",
	[LEVEL_PERCENTAGE_1] = "Power/B03_stat_sys_battery_percent_10.png",
	[LEVEL_PERCENTAGE_2] = "Power/B03_stat_sys_battery_percent_20.png",
	[LEVEL_PERCENTAGE_3] = "Power/B03_stat_sys_battery_percent_30.png",
	[LEVEL_PERCENTAGE_4] = "Power/B03_stat_sys_battery_percent_40.png",
	[LEVEL_PERCENTAGE_5] = "Power/B03_stat_sys_battery_percent_50.png",
	[LEVEL_PERCENTAGE_6] = "Power/B03_stat_sys_battery_percent_60.png",
	[LEVEL_PERCENTAGE_7] = "Power/B03_stat_sys_battery_percent_70.png",
	[LEVEL_PERCENTAGE_8] = "Power/B03_stat_sys_battery_percent_80.png",
	[LEVEL_PERCENTAGE_9] = "Power/B03_stat_sys_battery_percent_90.png",
	[LEVEL_PERCENTAGE_10] = "Power/B03_stat_sys_battery_percent_100.png"
};

static const char *percentage_battery_charging_icon_path[LEVEL_PERCENTAGE_NUM] = {
	[LEVEL_PERCENTAGE_0] = "Power/B03_stat_sys_battery_percent_charge_anim0.png",
	[LEVEL_PERCENTAGE_1] = "Power/B03_stat_sys_battery_percent_charge_anim10.png",
	[LEVEL_PERCENTAGE_2] = "Power/B03_stat_sys_battery_percent_charge_anim20.png",
	[LEVEL_PERCENTAGE_3] = "Power/B03_stat_sys_battery_percent_charge_anim30.png",
	[LEVEL_PERCENTAGE_4] = "Power/B03_stat_sys_battery_percent_charge_anim40.png",
	[LEVEL_PERCENTAGE_5] = "Power/B03_stat_sys_battery_percent_charge_anim50.png",
	[LEVEL_PERCENTAGE_6] = "Power/B03_stat_sys_battery_percent_charge_anim60.png",
	[LEVEL_PERCENTAGE_7] = "Power/B03_stat_sys_battery_percent_charge_anim70.png",
	[LEVEL_PERCENTAGE_8] = "Power/B03_stat_sys_battery_percent_charge_anim80.png",
	[LEVEL_PERCENTAGE_9] = "Power/B03_stat_sys_battery_percent_charge_anim90.png",
	[LEVEL_PERCENTAGE_10] = "Power/B03_stat_sys_battery_percent_charge_anim100.png"
};

enum {
	FUEL_GAUGE_LV_MIN = 0,
	FUEL_GAUGE_LV_0 = FUEL_GAUGE_LV_MIN,
	FUEL_GAUGE_LV_1,
	FUEL_GAUGE_LV_2,
	FUEL_GAUGE_LV_3,
	FUEL_GAUGE_LV_4,
	FUEL_GAUGE_LV_5,
	FUEL_GAUGE_LV_6,
	FUEL_GAUGE_LV_7,
	FUEL_GAUGE_LV_8,
	FUEL_GAUGE_LV_9,
	FUEL_GAUGE_LV_10,
	FUEL_GAUGE_LV_11,
	FUEL_GAUGE_LV_12,
	FUEL_GAUGE_LV_13,
	FUEL_GAUGE_LV_14,
	FUEL_GAUGE_LV_15,
	FUEL_GAUGE_LV_16,
	FUEL_GAUGE_LV_17,
	FUEL_GAUGE_LV_18,
	FUEL_GAUGE_LV_19,
	FUEL_GAUGE_LV_20,
	FUEL_GAUGE_LV_MAX = FUEL_GAUGE_LV_20,
	FUEL_GAUGE_LV_NUM,
};

struct battery_level_info {
	int current_level;
	int current_percentage;
	int min_level;
	int max_level;
};

static struct battery_level_info _level;
static Ecore_Timer *timer;
static int battery_level_type = BATTERY_LEVEL_8;
static int battery_charging = EINA_FALSE;
static int aniIndex = -1;
static int prev_mode = -1;
static int prev_level = -1;
static int prev_full = -1;
static int battery_percentage = -1;
static Eina_Bool is_battery_percentage_shown = EINA_FALSE;

static void set_app_state(void* data)
{
	battery.ad = data;
	digit.ad = data;
	digit_additional.ad = data;
}

static void delete_timer(void)
{
	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
	}
}

static int __battery_percentage_to_level(int percentage)
{
	int level = 0;

	if(is_battery_percentage_shown)
	{
		if (battery_level_type == BATTERY_LEVEL_20) {
			if (percentage >= 100)
				level = FUEL_GAUGE_LV_MAX;
			else if (percentage < 3)
				level = FUEL_GAUGE_LV_0;
			else
				level = (int)((percentage + 2) / 5);
		}
		else
		{
			if (percentage >= 91)
				level = LEVEL_PERCENTAGE_10;
			else if (percentage >= 81)
				level = LEVEL_PERCENTAGE_9;
			else if (percentage >= 71)
				level = LEVEL_PERCENTAGE_8;
			else if (percentage >= 61)
				level = LEVEL_PERCENTAGE_7;
			else if (percentage >= 51)
				level = LEVEL_PERCENTAGE_6;
			else if (percentage >= 41)
				level = LEVEL_PERCENTAGE_5;
			else if (percentage >= 31)
				level = LEVEL_PERCENTAGE_4;
			else if (percentage >= 21)
				level = LEVEL_PERCENTAGE_3;
			else if (percentage >= 11)
				level = LEVEL_PERCENTAGE_2;
			else if (percentage >= 1)
				level = LEVEL_PERCENTAGE_1;
			else
				level = LEVEL_PERCENTAGE_0;
		}
	}
	else
	{
		if (battery_level_type == BATTERY_LEVEL_20) {
			if (percentage >= 100)
				level = FUEL_GAUGE_LV_MAX;
			else if (percentage < 3)
				level = FUEL_GAUGE_LV_0;
			else
				level = (int)((percentage + 2) / 5);
		} else {
			if (percentage >= 96)
				level = LEVEL_20;
			else if (percentage >= 91)
				level = LEVEL_19;
			else if (percentage >= 86)
				level = LEVEL_18;
			else if (percentage >= 81)
				level = LEVEL_17;
			else if (percentage >= 76)
				level = LEVEL_16;
			else if (percentage >= 71)
				level = LEVEL_15;
			else if (percentage >= 66)
				level = LEVEL_14;
			else if (percentage >= 61)
				level = LEVEL_13;
			else if (percentage >= 56)
				level = LEVEL_12;
			else if (percentage >= 51)
				level = LEVEL_11;
			else if (percentage >= 46)
				level = LEVEL_10;
			else if (percentage >= 41)
				level = LEVEL_9;
			else if (percentage >= 36)
				level = LEVEL_8;
			else if (percentage >= 31)
				level = LEVEL_7;
			else if (percentage >= 26)
				level = LEVEL_6;
			else if (percentage >= 21)
				level = LEVEL_5;
			else if (percentage >= 16)
				level = LEVEL_4;
			else if (percentage >= 11)
				level = LEVEL_3;
			else if (percentage >= 6)
				level = LEVEL_2;
			else if (percentage >= 1)
				level = LEVEL_1;
			else
				level = LEVEL_0;
		}
	}
	return level;
}


#if 0
static void icon_animation_set(enum indicator_icon_ani type)
{
	icon_ani_set(&battery, type);
}
#endif


static void show_battery_icon(int mode, int level)
{
	if (is_battery_percentage_shown) {
		if (batt_full == 1) {
			battery.img_obj.data = percentage_battery_icon_path[LEVEL_PERCENTAGE_MAX];
		} else if (battery_charging==EINA_TRUE) {
			battery.img_obj.data = percentage_battery_charging_icon_path[level];
		} else {
			battery.img_obj.data = percentage_battery_icon_path[level];
		}
		icon_show(&battery);
	} else {
		if(batt_full == 1) {
			battery.img_obj.data = icon_path[LEVEL_MAX];
		} else if(battery_charging==EINA_TRUE) {
			battery.img_obj.data = charging_icon_path[level];
		} else {
			battery.img_obj.data = icon_path[level];
		}
		icon_show(&battery);
	}

	prev_full = batt_full;
	prev_mode = mode;
	prev_level = level;
}

static void show_digits()
{
	DBG("Show digits: %d", battery_percentage);

	if (battery_percentage < 10) {
		digit.img_obj.data = percentage_battery_digit_icon_path[battery_percentage];
		digit.digit_area = DIGIT_UNITY;
		digit.img_obj.width = 7;
		icon_show(&digit);
		icon_hide(&digit_additional);

	} else if (battery_percentage < 100) {
		digit.img_obj.data = percentage_battery_digit_icon_path[battery_percentage/10];
		digit.digit_area = DIGIT_DOZENS;
		digit.img_obj.width = 7;
		digit_additional.img_obj.data = percentage_battery_digit_icon_path[battery_percentage%10];
		digit_additional.digit_area = DIGIT_DOZENS_UNITY;
		digit_additional.img_obj.width = 7;
		icon_show(&digit);
		icon_show(&digit_additional);
	} else {
		digit.img_obj.data = percentage_battery_digit_icon_path[10];
		digit.digit_area = DIGIT_HUNDREDS;
		digit.img_obj.width = 17;
		icon_show(&digit);
		icon_hide(&digit_additional);
	}
}



static void hide_digits()
{
	DBG("Hide digits");

	icon_hide(&digit);
	icon_hide(&digit_additional);
}



static void indicator_battery_level_init(void)
{
	/* Currently, kernel not support level 6, So We use only level 20 */
	battery_level_type = BATTERY_LEVEL_8;
	_level.min_level = LEVEL_MIN;
	_level.current_level = -1;
	_level.current_percentage = -1;
	_level.max_level = LEVEL_MAX;
}


#if 0
static Eina_Bool indicator_battery_charging_ani_cb(void *data)
{

	retif(data == NULL, FAIL, "Invalid parameter!");

	if (_level.current_level == _level.max_level) {
		aniIndex = _level.max_level;
		show_battery_icon(battery_charging,aniIndex);
		timer = NULL;
		return TIMER_STOP;
	}

	if (aniIndex >= _level.max_level || aniIndex < 0)
		aniIndex = _level.current_level;
	else
		aniIndex++;

	show_battery_icon(battery_charging,aniIndex);

	return TIMER_CONTINUE;
}
#endif


static int indicator_change_battery_image_level(void *data, int level)
{
	retif(data == NULL, FAIL, "Invalid parameter!");

	if(is_battery_percentage_shown)
	{
		if (battery_level_type == BATTERY_LEVEL_20) {
			if (level < FUEL_GAUGE_LV_MIN)
				level = FUEL_GAUGE_LV_MIN;
			else if (level > FUEL_GAUGE_LV_MAX)
				level = FUEL_GAUGE_LV_MAX;
		}
		else {
			if (level < LEVEL_PERCENTAGE_MIN)
				level = LEVEL_PERCENTAGE_MIN;
		}
	}
	else
	{
		if (battery_level_type == BATTERY_LEVEL_20) {
			if (level < FUEL_GAUGE_LV_MIN)
				level = FUEL_GAUGE_LV_MIN;
			else if (level > FUEL_GAUGE_LV_MAX)
				level = FUEL_GAUGE_LV_MAX;
		} else {
			if (level < LEVEL_MIN)
				level = LEVEL_MIN;
		}
	}

	/* Set arg for display image only or text with image */
	show_battery_icon(battery_charging,level);
	return OK;
}

static void indicator_bttery_update_by_charging_state(void *data)
{
	aniIndex = -1;
	indicator_change_battery_image_level(data,
						  _level.current_level);
}

static void indicator_battery_resize_percengate(void* data)
{
	if(!is_battery_percentage_shown)
	{
		hide_digits();
		return;
	}

	show_digits();
}

static void indicator_battery_update_display(void *data)
{
	int ret;
	int level = 0;

	retif(data == NULL, , "Invalid parameter!");

	if(icon_get_update_flag()==0)
	{
		return;
	}

	ret = device_battery_get_percent(&battery_percentage);
	if (ret != DEVICE_ERROR_NONE)
	{
		return;
	}

	if (battery_percentage < 0)
	{
		ERR("Invalid Battery Capacity in percents: %d", battery_percentage);
		return;
	}

	if (battery_percentage > 100)
		battery_percentage = 100;

	_resize_battery_digits_icons_box();

	_level.current_percentage = battery_percentage;

	DBG("Battery capacity percentage: %d", battery_percentage);

	/* Check Battery Level */
	level = __battery_percentage_to_level(battery_percentage);
	if (level == _level.current_level)
	{
	}
	else {
		_level.current_level = level;
	}

	indicator_battery_resize_percengate(data);
	indicator_bttery_update_by_charging_state(data);

}


static void indicator_battery_check_charging(void *data)
{
	bool status = 0;
	int ret;

	ret = device_battery_is_charging(&status);

	if (ret != DEVICE_ERROR_NONE)
	{
		ERR("Fail to get battery charging status");
		return;
	} else {
		DBG("Battery charge Status: %d", status);
	}

	battery_charging = (int)status;

	indicator_battery_update_display(data);
}

static void indicator_battery_charging_cb(device_callback_e type, void *value, void *data)
{
	indicator_battery_check_charging(data);
}

static void indicator_battery_change_cb(device_callback_e type, void *value, void *data)
{
	device_battery_level_e battery_level;
	int ret = -1;

	ret = device_battery_get_level_status(&battery_level);
	if(ret != DEVICE_ERROR_NONE)
	{
		return;
	}

	if(battery_level == DEVICE_BATTERY_LEVEL_FULL)
		batt_full = 1;
	else
		batt_full = 0;

	indicator_battery_update_display(data);
}

static void indicator_battery_pm_state_change_cb(device_callback_e type, void *value, void *data)
{
	display_state_e display_state;

	retif(data == NULL, , "Invalid parameter!");

	display_state = device_display_get_state(&display_state);

	if(display_state == DISPLAY_STATE_SCREEN_OFF)
	{
		delete_timer();
	}
}

static void indicator_battery_batt_percentage_cb(device_callback_e type, void *value, void *data)
{
	struct appdata* ad = NULL;

	ret_if(!data);

	ad = (struct appdata*)data;

	//remove battery percentage
	is_battery_percentage_shown = EINA_FALSE;
	_level.max_level = LEVEL_MAX;
	indicator_battery_update_display(data);
	hide_digits();
	box_update_display(&(ad->win));
}

static int wake_up_cb(void *data)
{
	indicator_battery_update_display(data);

	return OK;
}

static int register_battery_module(void *data)
{
	int r = 0;
	int ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	/* DO NOT change order of below fuctions */
	set_app_state(data);
	indicator_battery_level_init();

	ret = device_add_callback(DEVICE_CALLBACK_BATTERY_CAPACITY,
					indicator_battery_change_cb, data);
	if (ret != DEVICE_ERROR_NONE) {
		r = ret;
	}

	ret = device_add_callback(DEVICE_CALLBACK_BATTERY_LEVEL,
					indicator_battery_change_cb, data);
	if (ret != DEVICE_ERROR_NONE) {
		r = r | ret;
	}

	ret = device_add_callback(DEVICE_CALLBACK_BATTERY_CHARGING,
					indicator_battery_charging_cb, data);
	if (ret != DEVICE_ERROR_NONE) {
		r = r | ret;
	}

	ret = device_add_callback(DEVICE_CALLBACK_DISPLAY_STATE,
					indicator_battery_pm_state_change_cb, data);
	if (ret != DEVICE_ERROR_NONE) {
		r = r | ret;
	}

	indicator_battery_batt_percentage_cb(DEVICE_CALLBACK_BATTERY_CAPACITY, NULL, data);
	indicator_battery_check_charging(data);

	return r;
}

static int unregister_battery_module(void)
{
	int r = 0;
	int ret = -1;

	ret = device_remove_callback(DEVICE_CALLBACK_BATTERY_CAPACITY, indicator_battery_change_cb);
	if (ret != DEVICE_ERROR_NONE) {
		r = ret;
	}

	ret = device_remove_callback(DEVICE_CALLBACK_BATTERY_LEVEL, indicator_battery_change_cb);
	if (ret != DEVICE_ERROR_NONE) {
		r = r | ret;
	}

	ret = device_remove_callback(DEVICE_CALLBACK_BATTERY_CHARGING, indicator_battery_charging_cb);
	if (ret != DEVICE_ERROR_NONE) {
		r = r | ret;
	}

	ret = device_remove_callback(DEVICE_CALLBACK_DISPLAY_STATE, indicator_battery_pm_state_change_cb);
	if (ret != DEVICE_ERROR_NONE) {
		r = r | ret;
	}

	delete_timer();

	return r;
}

static void _resize_battery_digits_icons_box()
{
	if (battery_percentage < 10) {
		util_signal_emit(digit.ad, "indicator.battery.percentage.one.digit.show", "indicator.prog");
	} else if(battery_percentage < 100) {
		util_signal_emit(digit.ad, "indicator.battery.percentage.two.digits.show", "indicator.prog");
	} else {
		util_signal_emit(digit.ad, "indicator.battery.percentage.full.show", "indicator.prog");
	}
}
