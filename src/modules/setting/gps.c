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
#include <runtime_info.h>
#include <device/display.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_3
#define MODULE_NAME		"gps"
#define TIMER_INTERVAL	0.3

static int register_gps_module(void *data);
static int unregister_gps_module(void);
static int _wake_up_cb(void *data);

icon_s gps = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_gps_module,
	.fini = unregister_gps_module,
	.wake_up = _wake_up_cb
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
	[LEVEL_GPS_SEARCHING] = "Bluetooth, NFC, GPS/B03_GPS_On.png",
};
static int updated_while_lcd_off = 0;
static int prevIndex = -1;



static void set_app_state(void* data)
{
	gps.ad = data;
}

static void show_image_icon(void *data, int index)
{
	if (index < LEVEL_MIN)
		index = LEVEL_MIN;
	else if (index >= LEVEL_MAX)
		index = LEVEL_GPS_SEARCHING;

	if(prevIndex == index)
		return;

	gps.img_obj.data = icon_path[index];
	icon_show(&gps);

	prevIndex = index;
}

static void hide_image_icon(void)
{
	icon_hide(&gps);

	prevIndex = -1;
}

static void icon_animation_set(enum indicator_icon_ani type)
{
	icon_ani_set(&gps,type);
}

static int indicator_gps_state_get(void)
{
	runtime_info_gps_status_e gps_status = 0;
	int status = 0;

	if (runtime_info_get_value_int(RUNTIME_INFO_KEY_GPS_STATUS, (int *)&gps_status) < 0)
	{
		_E("Error getting RUNTIME_INFO_KEY_GPS_STATUS value");
		return 0;
	}

	if(gps_status == RUNTIME_INFO_GPS_STATUS_CONNECTED)
		status = IND_POSITION_STATE_CONNECTED;
	else if(gps_status == RUNTIME_INFO_GPS_STATUS_SEARCHING)
		status = IND_POSITION_STATE_SEARCHING;
	else
		status = IND_POSITION_STATE_OFF;

	return status;
}

static void _gps_state_icon_set(int status, void *data)
{
	_D("GPS STATUS: %d", status);
	int ret;
	display_state_e display_state = DISPLAY_STATE_NORMAL;

	switch (status) {
	case IND_POSITION_STATE_OFF:
		hide_image_icon();
		break;

	case IND_POSITION_STATE_CONNECTED:
		show_image_icon(data, LEVEL_GPS_ON);
		icon_animation_set(ICON_ANI_NONE);
		break;

	case IND_POSITION_STATE_SEARCHING:
		ret = device_display_get_state(&display_state);
		retm_if(ret != DEVICE_ERROR_NONE, "device_display_get_state failed[%s]", get_error_message(ret));

		if(display_state == DISPLAY_STATE_SCREEN_OFF) {
			show_image_icon(data, LEVEL_GPS_SEARCHING);
			icon_animation_set(ICON_ANI_NONE);
		}
		else {
			show_image_icon(data, LEVEL_GPS_SEARCHING);
			icon_animation_set(ICON_ANI_BLINK);
		}
		break;

	default:
		hide_image_icon();
		_E("Invalid value!");
		break;
	}

	return;
}

void _gps_status_changed_cb(runtime_info_key_e key, void *data)
{
	ret_if(!data);

	if (icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	_gps_state_icon_set(indicator_gps_state_get(), data);

	return;
}

static int _wake_up_cb(void *data)
{
	if (updated_while_lcd_off == 0 && gps.obj_exist == EINA_FALSE)
		return OK;

	if (updated_while_lcd_off == 0 && gps.obj_exist == EINA_TRUE) {
		int gps_status = 0;
		runtime_info_get_value_int(RUNTIME_INFO_KEY_GPS_STATUS, &gps_status);

		switch (gps_status) {
		case RUNTIME_INFO_GPS_STATUS_SEARCHING:
			icon_animation_set(ICON_ANI_BLINK);
			break;
		case RUNTIME_INFO_GPS_STATUS_DISABLED:
		case RUNTIME_INFO_GPS_STATUS_CONNECTED:
		default:
			break;
		}

		return OK;
	}

	_gps_status_changed_cb(RUNTIME_INFO_KEY_GPS_STATUS, data);
	return OK;
}

static int register_gps_module(void *data)
{
	int ret;

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = runtime_info_set_changed_cb(RUNTIME_INFO_KEY_GPS_STATUS, _gps_status_changed_cb, data);

	_gps_state_icon_set(indicator_gps_state_get(), data);

	return ret;
}

static int unregister_gps_module(void)
{
	int ret;

	ret = runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_GPS_STATUS);

	return ret;
}
