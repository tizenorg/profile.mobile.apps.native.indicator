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
#define MODULE_NAME		"mobile_hotspot"

static int register_mobile_hotspot_module(void *data);
static int unregister_mobile_hotspot_module(void);
static int wake_up_cb(void *data);

Indicator_Icon_Object mobile_hotspot[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_mobile_hotspot_module,
	.fini = unregister_mobile_hotspot_module,
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
	.init = register_mobile_hotspot_module,
	.fini = unregister_mobile_hotspot_module,
	.wake_up = wake_up_cb
}

};

enum {
	MOBILEAP_ON_NOT_CONNECTED = 0,
	MOBILEAP_CONNECTED_MIN = MOBILEAP_ON_NOT_CONNECTED,
	MOBILEAP_CONNECTED1,
	MOBILEAP_CONNECTED2,
	MOBILEAP_CONNECTED3,
	MOBILEAP_CONNECTED4,
	MOBILEAP_CONNECTED5,
	MOBILEAP_CONNECTED6,
	MOBILEAP_CONNECTED7,
	MOBILEAP_CONNECTED8,
	MOBILEAP_CONNECTED9,
	MOBILEAP_MAX,
};

static const char *icon_path[MOBILEAP_MAX] = {
	[MOBILEAP_ON_NOT_CONNECTED] =
			"Connection/B03_MobileAP_on&not connected.png",
	[MOBILEAP_CONNECTED1] = "Connection/B03_MobileAP_connected_01.png",
	[MOBILEAP_CONNECTED2] = "Connection/B03_MobileAP_connected_02.png",
	[MOBILEAP_CONNECTED3] = "Connection/B03_MobileAP_connected_03.png",
	[MOBILEAP_CONNECTED4] = "Connection/B03_MobileAP_connected_04.png",
	[MOBILEAP_CONNECTED5] = "Connection/B03_MobileAP_connected_05.png",
	[MOBILEAP_CONNECTED6] = "Connection/B03_MobileAP_connected_06.png",
	[MOBILEAP_CONNECTED7] = "Connection/B03_MobileAP_connected_07.png",
	[MOBILEAP_CONNECTED8] = "Connection/B03_MobileAP_connected_08.png",
	[MOBILEAP_CONNECTED9] = "Connection/B03_MobileAP_connected_09.png",
};
static int updated_while_lcd_off = 0;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		mobile_hotspot[i].ad = data;
	}
}

static void show_image_icon(int type)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		mobile_hotspot[i].img_obj.data = icon_path[type];
		indicator_util_icon_show(&mobile_hotspot[i]);
	}
}
static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&mobile_hotspot[i]);
	}
}

static void indicator_mobile_hotspot_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	if(indicator_util_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		DBG("need to update %d",updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_int(VCONFKEY_MOBILE_HOTSPOT_MODE, &status);
	if (ret == OK) {
		INFO("mobile_hotspot status: %d", status);
		if (status != VCONFKEY_MOBILE_HOTSPOT_MODE_NONE) {
			int icon_index = 0;
			int connected_device = 0;

			ret = vconf_get_int(
				VCONFKEY_MOBILE_HOTSPOT_CONNECTED_DEVICE,
				&connected_device);

			if (ret == OK)
			{
				icon_index = MOBILEAP_CONNECTED_MIN + connected_device;
				if (icon_index < MOBILEAP_ON_NOT_CONNECTED) {
					ERR("unable to handle %d connected devices ",
						connected_device);
					icon_index = MOBILEAP_ON_NOT_CONNECTED;
				} else if (icon_index >= MOBILEAP_MAX) {
					ERR("unable to handle %d connected devices ",
						connected_device);
					icon_index = MOBILEAP_CONNECTED9;
				}
				show_image_icon(icon_index);
			}
			else
			{
				show_image_icon(MOBILEAP_ON_NOT_CONNECTED);
			}
			return;
		} else {
			hide_image_icon();
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

	indicator_mobile_hotspot_change_cb(NULL, data);
	return OK;
}

static int register_mobile_hotspot_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE,
			       indicator_mobile_hotspot_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_CONNECTED_DEVICE,
			       indicator_mobile_hotspot_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	indicator_mobile_hotspot_change_cb(NULL, data);

	return r;
}

static int unregister_mobile_hotspot_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE,
				       indicator_mobile_hotspot_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_CONNECTED_DEVICE,
				       indicator_mobile_hotspot_change_cb);
	if (ret != OK)
		ERR("Failed to register callback!");

	return OK;
}
