/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>
#include <runtime_info.h>
#include "common.h"
#include "indicator.h"
#include "indicator_ui.h"
#include "modules.h"
#include "indicator_icon_util.h"
#include "indicator_gui.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED4
#define MODULE_NAME		"battery"
#define TIMER_INTERVAL	0.7
#define BATTERY_TEXTWIDTH	62
#define BATTERY_VALUE_FONT_SIZE	20
#define BATTERY_PERCENT_FONT_SIZE	20
#define BATTERY_PERCENT_FONT_STYLE "Bold"

static int register_battery_module(void *data);
static int unregister_battery_module(void);
static int wake_up_cb(void *data);

Indicator_Icon_Object battery[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.area = INDICATOR_ICON_AREA_FIXED,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,0,BATTERY_ICON_WIDTH,BATTERY_ICON_HEIGHT},
	.obj_exist = EINA_FALSE,
	.init = register_battery_module,
	.fini = unregister_battery_module,
	.wake_up = wake_up_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.area = INDICATOR_ICON_AREA_FIXED,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,0,BATTERY_ICON_WIDTH,BATTERY_ICON_HEIGHT},
	.obj_exist = EINA_FALSE,
	.init = register_battery_module,
	.fini = unregister_battery_module,
	.wake_up = wake_up_cb
}
};

enum {
	INDICATOR_CLOCK_MODE_12H = 0,
	INDICATOR_CLOCK_MODE_24H,
	INDICATOR_CLOCK_MODE_MAX
};

static int clock_mode = INDICATOR_CLOCK_MODE_12H;

enum {
	BATTERY_ICON_WIDTH_12H,
	BATTERY_ICON_WIDTH_24H,
	BATTERY_ICON_WIDTH_NUM
};

enum {
	BATTERY_LEVEL_6,
	BATTERY_LEVEL_20,
};

enum {
	LEVEL_MIN = 0,
	LEVEL_0 = LEVEL_MIN,
	LEVEL_1,
	LEVEL_2,
	LEVEL_3,
	LEVEL_4,
	LEVEL_5,
	LEVEL_6,
	LEVEL_MAX = LEVEL_6,
	LEVEL_NUM,
};

static const char *icon_path[LEVEL_NUM] = {
	[LEVEL_0] = "Power/battery_6/B03_Power_battery_00.png",
	[LEVEL_1] = "Power/battery_6/B03_Power_battery_01.png",
	[LEVEL_2] = "Power/battery_6/B03_Power_battery_02.png",
	[LEVEL_3] = "Power/battery_6/B03_Power_battery_03.png",
	[LEVEL_4] = "Power/battery_6/B03_Power_battery_04.png",
	[LEVEL_5] = "Power/battery_6/B03_Power_battery_05.png",
	[LEVEL_6] = "Power/battery_6/B03_Power_battery_06.png",
};

static const char *charging_icon_path[LEVEL_NUM] = {
	[LEVEL_0] = "Power/battery_6/B03_Power_connected_00.png",
	[LEVEL_1] = "Power/battery_6/B03_Power_connected_01.png",
	[LEVEL_2] = "Power/battery_6/B03_Power_connected_02.png",
	[LEVEL_3] = "Power/battery_6/B03_Power_connected_03.png",
	[LEVEL_4] = "Power/battery_6/B03_Power_connected_04.png",
	[LEVEL_5] = "Power/battery_6/B03_Power_connected_05.png",
	[LEVEL_6] = "Power/battery_6/B03_Power_connected_06.png",
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

static const char *fuel_guage_icon_path[BATTERY_ICON_WIDTH_NUM][FUEL_GAUGE_LV_NUM] = {
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_0] = "Power/12H/B03_battery_animation_12h_00.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_1] = "Power/12H/B03_battery_animation_12h_01.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_2] = "Power/12H/B03_battery_animation_12h_02.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_3] = "Power/12H/B03_battery_animation_12h_03.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_4] = "Power/12H/B03_battery_animation_12h_04.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_5] = "Power/12H/B03_battery_animation_12h_05.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_6] = "Power/12H/B03_battery_animation_12h_06.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_7] = "Power/12H/B03_battery_animation_12h_07.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_8] = "Power/12H/B03_battery_animation_12h_08.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_9] = "Power/12H/B03_battery_animation_12h_09.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_10] = "Power/12H/B03_battery_animation_12h_10.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_11] = "Power/12H/B03_battery_animation_12h_11.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_12] = "Power/12H/B03_battery_animation_12h_12.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_13] = "Power/12H/B03_battery_animation_12h_13.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_14] = "Power/12H/B03_battery_animation_12h_14.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_15] = "Power/12H/B03_battery_animation_12h_15.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_16] = "Power/12H/B03_battery_animation_12h_16.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_17] = "Power/12H/B03_battery_animation_12h_17.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_18] = "Power/12H/B03_battery_animation_12h_18.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_19] = "Power/12H/B03_battery_animation_12h_19.png",
	[BATTERY_ICON_WIDTH_12H][FUEL_GAUGE_LV_20] = "Power/12H/B03_battery_animation_12h_20.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_0] = "Power/24H/B03_battery_animation_24h_00.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_1] = "Power/24H/B03_battery_animation_24h_01.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_2] = "Power/24H/B03_battery_animation_24h_02.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_3] = "Power/24H/B03_battery_animation_24h_03.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_4] = "Power/24H/B03_battery_animation_24h_04.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_5] = "Power/24H/B03_battery_animation_24h_05.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_6] = "Power/24H/B03_battery_animation_24h_06.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_7] = "Power/24H/B03_battery_animation_24h_07.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_8] = "Power/24H/B03_battery_animation_24h_08.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_9] = "Power/24H/B03_battery_animation_24h_09.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_10] = "Power/24H/B03_battery_animation_24h_10.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_11] = "Power/24H/B03_battery_animation_24h_11.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_12] = "Power/24H/B03_battery_animation_24h_12.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_13] = "Power/24H/B03_battery_animation_24h_13.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_14] = "Power/24H/B03_battery_animation_24h_14.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_15] = "Power/24H/B03_battery_animation_24h_15.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_16] = "Power/24H/B03_battery_animation_24h_16.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_17] = "Power/24H/B03_battery_animation_24h_17.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_18] = "Power/24H/B03_battery_animation_24h_18.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_19] = "Power/24H/B03_battery_animation_24h_19.png",
	[BATTERY_ICON_WIDTH_24H][FUEL_GAUGE_LV_20] = "Power/24H/B03_battery_animation_24h_20.png",
};

struct battery_level_info {
	int current_level;
	int current_capa;
	int min_level;
	int max_level;
};

static struct battery_level_info _level;
static Ecore_Timer *timer;
static int battery_level_type = BATTERY_LEVEL_20;
static Eina_Bool battery_percentage_on = EINA_FALSE;
static Eina_Bool battery_charging = EINA_FALSE;
static int aniIndex = -1;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		battery[i].ad = data;
	}
}

static void delete_timer(void)
{
	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
	}
}

static int __battery_capa_to_level(int capacity)
{
	int level = 0;

	if (battery_level_type == BATTERY_LEVEL_20) {
		if (capacity >= 100)
			level = FUEL_GAUGE_LV_MAX;
		else if (capacity < 3)
			level = FUEL_GAUGE_LV_0;
		else
			level = (int)((capacity + 2) / 5);
	} else {
		if (capacity > 100)
			level = LEVEL_MAX;
		else if (capacity >= 80)
			level = LEVEL_6;
		else if (capacity >= 60)
			level = LEVEL_5;
		else if (capacity >= 40)
			level = LEVEL_4;
		else if (capacity >= 25)
			level = LEVEL_3;
		else if (capacity >= 15)
			level = LEVEL_2;
		else if (capacity >= 5)
			level = LEVEL_1;
		else
			level = LEVEL_0;
	}

	return level;
}

static void show_battery_icon(void)
{
	int i = 0;
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_show(&battery[i]);
	}
}

static void hide_battery_icon(void)
{
	int i = 0;
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&battery[i]);
	}
}

static void icon_animation_set(enum indicator_icon_ani type)
{
	int i = 0;
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_animation_set(&battery[i],type);
	}
}

static void indicator_battery_get_time_format( void)
{
	int r = -1;

	bool is_24hour_enabled = false;

	r = runtime_info_get_value_bool(
			RUNTIME_INFO_KEY_24HOUR_CLOCK_FORMAT_ENABLED, &is_24hour_enabled);

	if( r==RUNTIME_INFO_ERROR_NONE&&is_24hour_enabled==true)
	{
		clock_mode = INDICATOR_CLOCK_MODE_24H;
	}
	else
	{
		clock_mode = INDICATOR_CLOCK_MODE_12H;
	}

}

static void indicator_battery_level_init(void)
{
	battery_level_type = BATTERY_LEVEL_20;
	_level.min_level = FUEL_GAUGE_LV_MIN;
	_level.current_level = -1;
	_level.current_capa = -1;
	_level.max_level = FUEL_GAUGE_LV_MAX;
	indicator_battery_get_time_format();
}

static void indicator_battery_text_set(void *data, int value, Indicator_Icon_Object *icon)
{
	Eina_Strbuf *temp_buf = NULL;
	Eina_Strbuf *percent_buf = NULL;
	char *temp_str = NULL;

	retif(data == NULL, , "Invalid parameter!");

	icon->type = INDICATOR_TXT_WITH_IMG_ICON;
	temp_buf = eina_strbuf_new();
	percent_buf = eina_strbuf_new();

	eina_strbuf_append_printf(temp_buf, "%d", value);
	temp_str = eina_strbuf_string_steal(temp_buf);
	eina_strbuf_append_printf(percent_buf, "%s",
				  indicator_util_icon_label_set
				  (temp_str, NULL, NULL,
				   BATTERY_VALUE_FONT_SIZE,
				   data));
	free(temp_str);

	eina_strbuf_append_printf(temp_buf, "%%");
	temp_str = eina_strbuf_string_steal(temp_buf);
	eina_strbuf_append_printf(percent_buf, "%s",
				  indicator_util_icon_label_set
				  (temp_str, NULL,
				   BATTERY_PERCENT_FONT_STYLE,
				   BATTERY_PERCENT_FONT_SIZE,
				   data));
	free(temp_str);

	if (icon->txt_obj.data != NULL)
		free(icon->txt_obj.data);

	icon->txt_obj.data = eina_strbuf_string_steal(percent_buf);

	if (temp_buf != NULL)
		eina_strbuf_free(temp_buf);

	if (percent_buf != NULL)
		eina_strbuf_free(percent_buf);

}

static Eina_Bool indicator_battery_charging_ani_cb(void *data)
{

	retif(data == NULL, FAIL, "Invalid parameter!");

	if (_level.current_level == _level.max_level) {
		aniIndex = _level.max_level;
		battery[0].img_obj.data = fuel_guage_icon_path[clock_mode][aniIndex];
		battery[1].img_obj.data = fuel_guage_icon_path[clock_mode][aniIndex];
		show_battery_icon();
		timer = NULL;
		return TIMER_STOP;
	}

	if (aniIndex >= _level.max_level || aniIndex < 0)
		aniIndex = _level.current_level;
	else
		aniIndex++;

	battery[0].img_obj.data = fuel_guage_icon_path[clock_mode][aniIndex];
	battery[1].img_obj.data = fuel_guage_icon_path[clock_mode][aniIndex];
	show_battery_icon();

	return TIMER_CONTINUE;
}

static int indicator_change_battery_image_level(void *data, int level)
{
	retif(data == NULL, FAIL, "Invalid parameter!");

	if (battery_level_type == BATTERY_LEVEL_20) {
		if (level < FUEL_GAUGE_LV_MIN)
			level = FUEL_GAUGE_LV_MIN;
		else if (level > FUEL_GAUGE_LV_MAX)
			level = FUEL_GAUGE_LV_MAX;
	} else {
		if (level < LEVEL_MIN)
			level = LEVEL_MIN;
		else if (level > LEVEL_MAX)
			level = LEVEL_MAX;
	}

	DBG("level = %d",level);
	battery[0].img_obj.data = fuel_guage_icon_path[clock_mode][level];
	battery[1].img_obj.data = fuel_guage_icon_path[clock_mode][level];
	show_battery_icon();
	return OK;
}

static void indicator_battery_check_percentage_option(void *data)
{
	int ret = FAIL;
	int status = 0;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_bool(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, &status);
	if (ret != OK)
		ERR("Fail to get [%s: %d]",
			VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, ret);

	int i = 0;
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		battery_percentage_on = EINA_FALSE;
		battery[i].img_obj.width = BATTERY_ICON_WIDTH;
		battery[i].type = INDICATOR_IMG_ICON;
	}
}

static void indicator_bttery_update_by_charging_state(void *data)
{
	if (battery_charging == EINA_TRUE) {
		if (!timer) {
			icon_animation_set(ICON_ANI_NONE);
			timer = ecore_timer_add(TIMER_INTERVAL,
					indicator_battery_charging_ani_cb,
					data);
		}
	} else {
		aniIndex = -1;
		delete_timer();
		icon_animation_set(ICON_ANI_NONE);
		indicator_change_battery_image_level(data,
				_level.current_level);
	}
}

static void indicator_battery_update_display(void *data)
{
	int battery_capa = 0;
	int ret;
	int level = 0;

	retif(data == NULL, , "Invalid parameter!");

	if(indicator_util_get_update_flag()==0)
	{
		DBG("need to update");
		return;
	}

	ret = vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, &battery_capa);
	if (ret != OK)
	{
		ERR("Fail to get [VCONFKEY_SYSMAN_BATTERY_CAPACITY:%d]", ret);
		return;
	}

	if (battery_capa < 0)
	{
		INFO("Invalid Battery Capacity: %d", battery_capa);
		return;
	}

	INFO("Battery Capacity: %d", battery_capa);

	if (battery_capa > 100)
		battery_capa = 100;

	_level.current_capa = battery_capa;

	indicator_battery_check_percentage_option(data);

	level = __battery_capa_to_level(battery_capa);
	if (level == _level.current_level)
		DBG("battery level is not changed");
	else {
		_level.current_level = level;
	}
	indicator_bttery_update_by_charging_state(data);

}


static void indicator_battery_check_charging(void *data)
{
	int ret = -1;
	int status = 0;

	ret = vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &status);
	if (ret != OK)
		ERR("Fail to get [VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW:%d]", ret);

	INFO("Battery charge Status: %d", status);

	battery_charging = status;

	indicator_battery_update_display(data);
}

static void indicator_battery_charging_cb(keynode_t *node, void *data)
{
	indicator_battery_check_charging(data);
}

static void indicator_battery_percentage_option_cb(keynode_t *node, void *data)
{

}

static void indicator_battery_change_cb(keynode_t *node, void *data)
{
	indicator_battery_update_display(data);
}

static void indicator_battery_clock_format_changed_cb(keynode_t *node, void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	INFO("[Enter] indicator_battery_clock_format_changed_cb");

	indicator_battery_get_time_format();

	indicator_battery_update_display(data);
}

static void indicator_battery_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_PM_STATE, &status);

	if(status == VCONFKEY_PM_STATE_LCDOFF)
	{
		delete_timer();
	}
}

static int wake_up_cb(void *data)
{
	INFO("BATTERY wake_up_cb");
	indicator_battery_clock_format_changed_cb(NULL, data);
	return OK;
}

static int register_battery_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);
	indicator_battery_level_init();

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CAPACITY,
				       indicator_battery_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW,
				       indicator_battery_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW,
				       indicator_battery_charging_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL,
			       indicator_battery_percentage_option_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_REGIONFORMAT_TIME1224,
					       indicator_battery_clock_format_changed_cb, data);
	if (ret != OK) {
		ERR("Fail: register VCONFKEY_REGIONFORMAT_TIME1224");
		r = r | ret;
	}
	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE,
					       indicator_battery_pm_state_change_cb, data);
	if (ret != OK) {
		ERR("Fail: register VCONFKEY_PM_STATE");
		r = r | ret;
	}

	indicator_battery_update_display(data);

	return r;
}

static int unregister_battery_module(void)
{
	int ret;
	int i = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CAPACITY,
				       indicator_battery_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW,
				       indicator_battery_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW,
				       indicator_battery_charging_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL,
				       indicator_battery_percentage_option_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE,
				       indicator_battery_pm_state_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	delete_timer();

	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		if (battery[i].txt_obj.data != NULL)
			free(battery[i].txt_obj.data);
	}

	return OK;
}
