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
#include "common.h"
#include "indicator.h"
#include "indicator_icon_util.h"
#include "modules.h"
#include "indicator_ui.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"gps"
#define TIMER_INTERVAL	0.3

static int register_gps_module(void *data);
static int unregister_gps_module(void);
static int hib_enter_gps_module(void);
static int hib_leave_gps_module(void *data);
static int wake_up_cb(void *data);

Indicator_Icon_Object gps[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_gps_module,
	.fini = unregister_gps_module,
	.hib_enter = hib_enter_gps_module,
	.hib_leave = hib_leave_gps_module,
	.wake_up = wake_up_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_gps_module,
	.fini = unregister_gps_module,
	.hib_enter = hib_enter_gps_module,
	.hib_leave = hib_leave_gps_module,
	.wake_up = wake_up_cb
}
};

enum {
	LEVEL_MIN = 0,
	LEVEL_GPS_ON = LEVEL_MIN,
	LEVEL_GPS_SEARCHING,
	LEVEL_MAX
};

enum {
	IND_POSITION_STATE_OFF = 0,
	IND_POSITION_STATE_SEARCHING,
	IND_POSITION_STATE_CONNECTED
};

static const char *icon_path[LEVEL_MAX] = {
	[LEVEL_GPS_ON] = "Bluetooth, NFC, GPS/B03_GPS_On.png",
	[LEVEL_GPS_SEARCHING] = "Bluetooth, NFC, GPS/B03_GPS_Searching.png",
};
static int updated_while_lcd_off = 0;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		gps[i].ad = data;
	}
}

static void show_image_icon(void *data, int index)
{
	int i = 0;

	if (index < LEVEL_MIN)
		index = LEVEL_MIN;
	else if (index >= LEVEL_MAX)
		index = LEVEL_GPS_SEARCHING;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		gps[i].img_obj.data = icon_path[index];
		indicator_util_icon_show(&gps[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&gps[i]);
	}
}

static void icon_animation_set(enum indicator_icon_ani type)
{
	int i = 0;
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_animation_set(&gps[i],type);
	}
}

static int indicator_gps_state_get(void)
{
	int ret = 0;
	int gps_status = 0;
	int status = 0;

	ret = vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps_status);
	if (ret < 0)
		ERR("fail to get [%s]", VCONFKEY_LOCATION_GPS_STATE);

	INFO("GPS STATUS: %d", gps_status);

	if(gps_status == VCONFKEY_LOCATION_GPS_CONNECTED)
	{
		status = IND_POSITION_STATE_CONNECTED;
	}
	else if(gps_status == VCONFKEY_LOCATION_GPS_SEARCHING)
	{
		status = IND_POSITION_STATE_SEARCHING;
	}
	else
	{
		status = IND_POSITION_STATE_OFF;
	}

	return status;
}

static void indicator_gps_state_icon_set(int status, void *data)
{
	INFO("GPS STATUS: %d", status);

	switch (status) {
	case IND_POSITION_STATE_OFF:
		hide_image_icon();
		break;
	case IND_POSITION_STATE_CONNECTED:
		show_image_icon(data, LEVEL_GPS_ON);
		icon_animation_set(ICON_ANI_NONE);
		break;
	case IND_POSITION_STATE_SEARCHING:
		show_image_icon(data, LEVEL_GPS_SEARCHING);
		icon_animation_set(ICON_ANI_BLINK);
		break;
	default:
		hide_image_icon();
		ERR("Invalid value!");
		break;
	}

	return;
}

static void indicator_gps_change_cb(keynode_t *node, void *data)
{

	retif(data == NULL, , "Invalid parameter!");

	if(indicator_util_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		DBG("need to update %d",updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	indicator_gps_state_icon_set(indicator_gps_state_get(), data);

	return;
}

static void indicator_gps_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = 0;
	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_PM_STATE, &status);

	if(status == VCONFKEY_PM_STATE_LCDOFF)
	{
		int gps_status = 0;
		ret = vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps_status);
		if (ret < 0)
			ERR("fail to get [%s]", VCONFKEY_LOCATION_GPS_STATE);

		INFO("GPS STATUS: %d", gps_status);
		switch (gps_status) {
		case IND_POSITION_STATE_SEARCHING:
			show_image_icon(data, LEVEL_GPS_SEARCHING);
			icon_animation_set(ICON_ANI_NONE);
			break;
		case IND_POSITION_STATE_OFF:
		case IND_POSITION_STATE_CONNECTED:
		default:
			break;
		}
	}
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0 && gps[0].obj_exist==EINA_FALSE)
	{
		DBG("ICON WAS NOT UPDATED");
		return OK;
	}

	indicator_gps_change_cb(NULL, data);
	return OK;
}

static int register_gps_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_LOCATION_GPS_STATE,
				       indicator_gps_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback! : VCONFKEY_LOCATION_GPS_STATE");

	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE,
				       indicator_gps_pm_state_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback! : VCONFKEY_LOCATION_GPS_STATE");

	indicator_gps_state_icon_set(indicator_gps_state_get(), data);

	return ret;
}

static int unregister_gps_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_LOCATION_GPS_STATE,
				       indicator_gps_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE,
				       indicator_gps_pm_state_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}

static int hib_enter_gps_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_LOCATION_GPS_STATE,
				       indicator_gps_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}

static int hib_leave_gps_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	ret = vconf_notify_key_changed(VCONFKEY_LOCATION_GPS_STATE,
				       indicator_gps_change_cb, data);
	retif(ret != OK, FAIL, "Failed to register callback!");

	indicator_gps_state_icon_set(indicator_gps_state_get(), data);

	return OK;
}

