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
#include <vconf.h>

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
	.img_obj = {0, 0, BATTERY_ICON_WIDTH, BATTERY_ICON_HEIGHT},
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
	.img_obj = {0, 0, 7, 10},
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
	.img_obj = {0, 0, 7, 10},
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
	BATTERY_LEVEL_MIN = 0,
	BATTERY_LEVEL_0 = BATTERY_LEVEL_MIN,
	BATTERY_LEVEL_1,
	BATTERY_LEVEL_2,
	BATTERY_LEVEL_3,
	BATTERY_LEVEL_4,
	BATTERY_LEVEL_5,
	BATTERY_LEVEL_6,
	BATTERY_LEVEL_7,
	BATTERY_LEVEL_8,
	BATTERY_LEVEL_9,
	BATTERY_LEVEL_10,
	BATTERY_LEVEL_MAX = BATTERY_LEVEL_10,
	BATTERY_LEVEL_COUNT
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


static const char *icon_path[BATTERY_LEVEL_COUNT] = {
	[BATTERY_LEVEL_0] = "Power/B03_stat_sys_battery_0.png",
	[BATTERY_LEVEL_1] = "Power/B03_stat_sys_battery_10.png",
	[BATTERY_LEVEL_2] = "Power/B03_stat_sys_battery_20.png",
	[BATTERY_LEVEL_3] = "Power/B03_stat_sys_battery_30.png",
	[BATTERY_LEVEL_4] = "Power/B03_stat_sys_battery_40.png",
	[BATTERY_LEVEL_5] = "Power/B03_stat_sys_battery_50.png",
	[BATTERY_LEVEL_6] = "Power/B03_stat_sys_battery_60.png",
	[BATTERY_LEVEL_7] = "Power/B03_stat_sys_battery_70.png",
	[BATTERY_LEVEL_8] = "Power/B03_stat_sys_battery_80.png",
	[BATTERY_LEVEL_9] = "Power/B03_stat_sys_battery_90.png",
	[BATTERY_LEVEL_10] = "Power/B03_stat_sys_battery_100.png"
};

static const char *charging_icon_path[BATTERY_LEVEL_COUNT] = {
	[BATTERY_LEVEL_0] = "Power/B03_stat_sys_battery_charge_anim0.png",
	[BATTERY_LEVEL_1] = "Power/B03_stat_sys_battery_charge_anim10.png",
	[BATTERY_LEVEL_2] = "Power/B03_stat_sys_battery_charge_anim20.png",
	[BATTERY_LEVEL_3] = "Power/B03_stat_sys_battery_charge_anim30.png",
	[BATTERY_LEVEL_4] = "Power/B03_stat_sys_battery_charge_anim40.png",
	[BATTERY_LEVEL_5] = "Power/B03_stat_sys_battery_charge_anim50.png",
	[BATTERY_LEVEL_6] = "Power/B03_stat_sys_battery_charge_anim60.png",
	[BATTERY_LEVEL_7] = "Power/B03_stat_sys_battery_charge_anim70.png",
	[BATTERY_LEVEL_8] = "Power/B03_stat_sys_battery_charge_anim80.png",
	[BATTERY_LEVEL_9] = "Power/B03_stat_sys_battery_charge_anim90.png",
	[BATTERY_LEVEL_10] = "Power/B03_stat_sys_battery_charge_anim100.png"
};

static const char *percentage_battery_digit_icon_path[BATTERY_LEVEL_COUNT] = {
	[BATTERY_LEVEL_0]  = "Power/battery_text/B03_stat_sys_battery_num_0.png",
	[BATTERY_LEVEL_1]  = "Power/battery_text/B03_stat_sys_battery_num_1.png",
	[BATTERY_LEVEL_2]  = "Power/battery_text/B03_stat_sys_battery_num_2.png",
	[BATTERY_LEVEL_3]  = "Power/battery_text/B03_stat_sys_battery_num_3.png",
	[BATTERY_LEVEL_4]  = "Power/battery_text/B03_stat_sys_battery_num_4.png",
	[BATTERY_LEVEL_5]  = "Power/battery_text/B03_stat_sys_battery_num_5.png",
	[BATTERY_LEVEL_6]  = "Power/battery_text/B03_stat_sys_battery_num_6.png",
	[BATTERY_LEVEL_7]  = "Power/battery_text/B03_stat_sys_battery_num_7.png",
	[BATTERY_LEVEL_8]  = "Power/battery_text/B03_stat_sys_battery_num_8.png",
	[BATTERY_LEVEL_9]  = "Power/battery_text/B03_stat_sys_battery_num_9.png",
	[BATTERY_LEVEL_10] = "Power/battery_text/B03_stat_sys_battery_num_100.png"
};

static const char *percentage_battery_icon_path[BATTERY_LEVEL_COUNT] = {
	[BATTERY_LEVEL_0] = "Power/B03_stat_sys_battery_percent_0.png",
	[BATTERY_LEVEL_1] = "Power/B03_stat_sys_battery_percent_10.png",
	[BATTERY_LEVEL_2] = "Power/B03_stat_sys_battery_percent_20.png",
	[BATTERY_LEVEL_3] = "Power/B03_stat_sys_battery_percent_30.png",
	[BATTERY_LEVEL_4] = "Power/B03_stat_sys_battery_percent_40.png",
	[BATTERY_LEVEL_5] = "Power/B03_stat_sys_battery_percent_50.png",
	[BATTERY_LEVEL_6] = "Power/B03_stat_sys_battery_percent_60.png",
	[BATTERY_LEVEL_7] = "Power/B03_stat_sys_battery_percent_70.png",
	[BATTERY_LEVEL_8] = "Power/B03_stat_sys_battery_percent_80.png",
	[BATTERY_LEVEL_9] = "Power/B03_stat_sys_battery_percent_90.png",
	[BATTERY_LEVEL_10] = "Power/B03_stat_sys_battery_percent_100.png"
};

static const char *percentage_battery_charging_icon_path[BATTERY_LEVEL_COUNT] = {
	[BATTERY_LEVEL_0] = "Power/B03_stat_sys_battery_percent_charge_anim0.png",
	[BATTERY_LEVEL_1] = "Power/B03_stat_sys_battery_percent_charge_anim10.png",
	[BATTERY_LEVEL_2] = "Power/B03_stat_sys_battery_percent_charge_anim20.png",
	[BATTERY_LEVEL_3] = "Power/B03_stat_sys_battery_percent_charge_anim30.png",
	[BATTERY_LEVEL_4] = "Power/B03_stat_sys_battery_percent_charge_anim40.png",
	[BATTERY_LEVEL_5] = "Power/B03_stat_sys_battery_percent_charge_anim50.png",
	[BATTERY_LEVEL_6] = "Power/B03_stat_sys_battery_percent_charge_anim60.png",
	[BATTERY_LEVEL_7] = "Power/B03_stat_sys_battery_percent_charge_anim70.png",
	[BATTERY_LEVEL_8] = "Power/B03_stat_sys_battery_percent_charge_anim80.png",
	[BATTERY_LEVEL_9] = "Power/B03_stat_sys_battery_percent_charge_anim90.png",
	[BATTERY_LEVEL_10] = "Power/B03_stat_sys_battery_percent_charge_anim100.png"
};

struct battery_level_info {
	int current_level;
	int current_percentage;
};

static struct battery_level_info _level;
static Eina_Bool battery_charging = EINA_FALSE;
static int battery_percentage = -1;
static Eina_Bool is_battery_percentage_shown = EINA_FALSE;

static void set_app_state(void *data)
{
	battery.ad = data;
	digit.ad = data;
	digit_additional.ad = data;
}

static int __battery_percentage_to_level(int percentage)
{
	int level = 0;

	if (percentage >= 100)
		level = BATTERY_LEVEL_MAX;
	else if (percentage >= 91)
		level = BATTERY_LEVEL_10;
	else if (percentage >= 81)
		level = BATTERY_LEVEL_9;
	else if (percentage >= 71)
		level = BATTERY_LEVEL_8;
	else if (percentage >= 61)
		level = BATTERY_LEVEL_7;
	else if (percentage >= 51)
		level = BATTERY_LEVEL_6;
	else if (percentage >= 41)
		level = BATTERY_LEVEL_5;
	else if (percentage >= 31)
		level = BATTERY_LEVEL_4;
	else if (percentage >= 21)
		level = BATTERY_LEVEL_3;
	else if (percentage >= 11)
		level = BATTERY_LEVEL_2;
	else if (percentage >= 1)
		level = BATTERY_LEVEL_1;
	else
		level = BATTERY_LEVEL_MIN;

	return level;
}


static void show_battery_icon(int level)
{

	if (is_battery_percentage_shown) {

		if (battery_charging == EINA_TRUE)
			battery.img_obj.data = percentage_battery_charging_icon_path[level];
		else
			battery.img_obj.data = percentage_battery_icon_path[level];

	} else {

		if (battery_charging == EINA_TRUE) {
			battery.img_obj.data = charging_icon_path[level];
		} else {
			battery.img_obj.data = icon_path[level];
		}
	}

	icon_show(&battery);

}

static void show_digits()
{
	_D("Show digits: %d", battery_percentage);

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
	_D("Hide digits");

	icon_hide(&digit);
	icon_hide(&digit_additional);
}


static void indicator_battery_level_init(void)
{
	_level.current_level = -1;
	_level.current_percentage = -1;
}

static void indicator_battery_update_state(void *data)
{
	show_battery_icon(_level.current_level);
}

static void indicator_battery_resize_percengate(void *data)
{
	if (!is_battery_percentage_shown) {
		hide_digits();
		return;
	}

	show_digits();
}

static void indicator_battery_update_display(void *data)
{
	int ret;
	int level = 0;

	retm_if(data == NULL, "Invalid parameter!");

	if (icon_get_update_flag() == 0)
		return;

	ret = device_battery_get_percent(&battery_percentage);
	if (ret != DEVICE_ERROR_NONE)
		return;

	if (battery_percentage < 0) {
		_E("Invalid Battery Capacity in percents: %d", battery_percentage);
		return;
	}

	if (battery_percentage > 100)
		battery_percentage = 100;

	_resize_battery_digits_icons_box();

	_level.current_percentage = battery_percentage;

	_D("Battery capacity percentage: %d", battery_percentage);

	/* Check Battery Level */
	level = __battery_percentage_to_level(battery_percentage);

	_level.current_level = level;

	indicator_battery_resize_percengate(data);
	indicator_battery_update_state(data);

}


static void indicator_battery_check_charging(void *data)
{
	bool status = 0;
	int ret;

	ret = device_battery_is_charging(&status);

	if (ret != DEVICE_ERROR_NONE) {
		_E("Fail to get battery charging status");
		return;

	} else {
		_D("Battery charge Status: %d", status);
	}

	battery_charging = (Eina_Bool)status;

	indicator_battery_update_display(data);
}


static void indicator_battery_charging_cb(device_callback_e type, void *value, void *data)
{
	indicator_battery_check_charging(data);
}


static void indicator_battery_change_cb(device_callback_e type, void *value, void *data)
{
	indicator_battery_update_display(data);
}


static void indicator_battery_batt_percentage_cb(device_callback_e type, void *value, void *data)
{
	struct appdata *ad = NULL;

	int status = 0;

	ret_if(!data);

	ad = (struct appdata *)data;

	vconf_get_bool(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, &status);

	if (status == 1) {
		is_battery_percentage_shown = EINA_TRUE;
		indicator_battery_update_display(data);
		show_digits();

	} else {
		//remove battery percentage
		is_battery_percentage_shown = EINA_FALSE;
		indicator_battery_update_display(data);
		hide_digits();
	}

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

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

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

	return r;
}


static void _resize_battery_digits_icons_box()
{
	if (battery_percentage < 10) {
		util_signal_emit(digit.ad, "indicator.battery.percentage.one.digit.show", "indicator.prog");
	} else if (battery_percentage < 100) {
		util_signal_emit(digit.ad, "indicator.battery.percentage.two.digits.show", "indicator.prog");
	} else {
		util_signal_emit(digit.ad, "indicator.battery.percentage.full.show", "indicator.prog");
	}
}
