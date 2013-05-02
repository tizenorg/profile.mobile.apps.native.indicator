/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
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

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED2
#define MODULE_NAME		"wifi"
#define TIMER_INTERVAL	0.3

static int register_wifi_module(void *data);
static int unregister_wifi_module(void);
static int wake_up_cb(void *data);

Indicator_Icon_Object wifi[INDICATOR_WIN_MAX] = {
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
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_wifi_module,
	.fini = unregister_wifi_module,
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
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_wifi_module,
	.fini = unregister_wifi_module,
	.wake_up = wake_up_cb
}
};

enum {
	LEVEL_WIFI_01 = 0,
	LEVEL_WIFI = LEVEL_WIFI_01,
	LEVEL_WIFI_02,
	LEVEL_WIFI_03,
	LEVEL_WIFI_04,
	LEVEL_WIFI_MAX
};

static const char *icon_path[LEVEL_WIFI_MAX] = {
	[LEVEL_WIFI_01] = "Connection/B03_connection_Wifi_01.png",
	[LEVEL_WIFI_02] = "Connection/B03_connection_Wifi_02.png",
	[LEVEL_WIFI_03] = "Connection/B03_connection_Wifi_03.png",
	[LEVEL_WIFI_04] = "Connection/B03_connection_Wifi_04.png",
};

static Eina_Bool wifi_transferring = EINA_FALSE;
static int updated_while_lcd_off = 0;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		wifi[i].ad = data;
	}
}

static void show_image_icon(void *data, int index)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		wifi[i].img_obj.data = icon_path[index];
		indicator_util_icon_show(&wifi[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&wifi[i]);
	}
}


static void indicator_wifi_change_cb(keynode_t *node, void *data)
{
	int status, strength;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	if(indicator_util_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		DBG("need to update %d",updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_int(VCONFKEY_WIFI_STRENGTH, &strength);
	if (ret == OK) {
		INFO("CONNECTION WiFi Strength: %d", strength);
		if (strength < VCONFKEY_WIFI_STRENGTH_MIN) {
			strength = VCONFKEY_WIFI_STRENGTH_MIN;
		} else if (strength > VCONFKEY_WIFI_STRENGTH_MAX) {
			strength = VCONFKEY_WIFI_STRENGTH_MAX;
		}
	} else
		strength = VCONFKEY_WIFI_STRENGTH_MAX;

	ret = vconf_get_int(VCONFKEY_WIFI_STATE, &status);
	if (ret == OK) {
		INFO("CONNECTION WiFi Status: %d", status);
		if (status != VCONFKEY_WIFI_TRANSFER) {
			if ( wifi_transferring == EINA_TRUE ) {
				wifi_transferring = EINA_FALSE;
			}
		}
		if (status == VCONFKEY_WIFI_UNCONNECTED) {
			hide_image_icon();
			return;
		} else if (status == VCONFKEY_WIFI_CONNECTED) {
			if (strength <= 0)
				strength = 1;

			show_image_icon(data,LEVEL_WIFI + strength-1);
			return;
		} else if (status == VCONFKEY_WIFI_TRANSFER) {
			if (wifi_transferring != EINA_TRUE) {
				wifi_transferring = EINA_TRUE;
			}
			return;
		}
	}
	hide_image_icon();
	return;
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		DBG("ICON WAS NOT UPDATED");
		return OK;
	}

	indicator_wifi_change_cb(NULL, data);
	return OK;
}

static int register_wifi_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_WIFI_STATE,
				       indicator_wifi_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_WIFI_STRENGTH,
				       indicator_wifi_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}
	indicator_wifi_change_cb(NULL, data);

	return r;
}

static int unregister_wifi_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_WIFI_STATE,
				       indicator_wifi_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_WIFI_STRENGTH,
				       indicator_wifi_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	if (wifi_transferring == EINA_TRUE) {
		wifi_transferring  = EINA_FALSE;
	}

	return OK;
}
