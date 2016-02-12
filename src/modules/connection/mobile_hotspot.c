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
#include <vconf.h>
#include <system_settings.h>
#include <runtime_info.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "log.h"
#include "util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"mobile_hotspot"

static int register_mobile_hotspot_module(void *data);
static int unregister_mobile_hotspot_module(void);
static int wake_up_cb(void *data);

icon_s mobile_hotspot = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_mobile_hotspot_module,
	.fini = unregister_mobile_hotspot_module,
	.wake_up = wake_up_cb
};

enum {
	TETHERING_ALL_ON_CONNECTED = 0,
	TETHERING_ALL_ON_NOT_CONNECTED,
	TETHERING_BT_ON_CONNECTED,
	TETHERING_BT_ON_NOT_CONNECTED,
	TETHERING_USB_ON_CONNECTED,
	TETHERING_USB_ON_NOT_CONNECTED,
	TETHERING_WIFI_ON_CONNECTED,
	TETHERING_WIFI_ON_NOT_CONNECTED,
	TETHERING_MAX,
};

static const char *icon_path[TETHERING_MAX] = {
	[TETHERING_ALL_ON_CONNECTED] 		= "tethering/B03_All_connected.png",
	[TETHERING_ALL_ON_NOT_CONNECTED] 	= "tethering/B03_All_no_connected.png",
	[TETHERING_BT_ON_CONNECTED] 		= "tethering/B03_BT_connected.png",
	[TETHERING_BT_ON_NOT_CONNECTED] 	= "tethering/B03_BT_no_connected.png",
	[TETHERING_USB_ON_CONNECTED] 		= "tethering/B03_USB_connected.png",
	[TETHERING_USB_ON_NOT_CONNECTED] 	= "tethering/B03_USB_no_connected.png",
	[TETHERING_WIFI_ON_CONNECTED] 		= "tethering/B03_Wi_Fi_connected.png",
	[TETHERING_WIFI_ON_NOT_CONNECTED] 	= "tethering/B03_Wi_Fi_no_connected.png",
};
static int updated_while_lcd_off = 0;
static int prevIndex = -1;



static void set_app_state(void* data)
{
	mobile_hotspot.ad = data;
}



static void show_image_icon(int type)
{
	if(prevIndex == type)
	{
		return;
	}

	mobile_hotspot.img_obj.data = icon_path[type];
	icon_show(&mobile_hotspot);

	prevIndex = type;
}



static void hide_image_icon(void)
{
	icon_hide(&mobile_hotspot);

	prevIndex = -1;
}



static void indicator_mobile_hotspot_change_cb(void *user_data)
{
	int ret;
	bool usb_enabled, wifi_enabled, bt_enabled;

	retm_if(user_data == NULL, "Invalid parameter!");

	if (icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = runtime_info_get_value_bool(RUNTIME_INFO_KEY_USB_TETHERING_ENABLED, &usb_enabled);
	retm_if(ret != RUNTIME_INFO_ERROR_NONE, "runtime_info_get_value_bool failed");

	ret = runtime_info_get_value_bool(RUNTIME_INFO_KEY_WIFI_HOTSPOT_ENABLED, &wifi_enabled);
	retm_if(ret != RUNTIME_INFO_ERROR_NONE, "runtime_info_get_value_bool failed");

	ret = runtime_info_get_value_bool(RUNTIME_INFO_KEY_BLUETOOTH_TETHERING_ENABLED, &bt_enabled);
	retm_if(ret != RUNTIME_INFO_ERROR_NONE, "runtime_info_get_value_bool failed");

	/* How many tethering methods are on */
	int sum = (usb_enabled ? 1 : 0) + (wifi_enabled ? 1 : 0) + (bt_enabled ? 1 : 0);

	if (sum == 0) {
		hide_image_icon();
	}

	if (sum >= 2) {
		show_image_icon(TETHERING_ALL_ON_CONNECTED);
	}

	if (bt_enabled) {
		show_image_icon(TETHERING_BT_ON_CONNECTED);
	}

	if (usb_enabled) {
		show_image_icon(TETHERING_USB_ON_CONNECTED);
	}

	if (wifi_enabled) {
		show_image_icon(TETHERING_WIFI_ON_CONNECTED);
	}
}


static void _runtime_info_key_update_cb(runtime_info_key_e key, void *user_data)
{
	indicator_mobile_hotspot_change_cb(user_data);
}


static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		return OK;
	}

	indicator_mobile_hotspot_change_cb(data);
	return OK;
}



static int register_mobile_hotspot_module(void *data)
{
	int ret;

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = util_runtime_info_set_changed_cb(RUNTIME_INFO_KEY_WIFI_HOTSPOT_ENABLED, _runtime_info_key_update_cb, data);
	retvm_if(ret != 0, FAIL, "util_runtime_info_set_changed_cb failed.");

	ret = util_runtime_info_set_changed_cb(RUNTIME_INFO_KEY_BLUETOOTH_TETHERING_ENABLED, _runtime_info_key_update_cb, data);
	if (ret != 0) {
		util_runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_WIFI_HOTSPOT_ENABLED, _runtime_info_key_update_cb);
		ERR("util_runtime_info_set_changed_cb failed");
		return FAIL;
	}

	ret = util_runtime_info_set_changed_cb(RUNTIME_INFO_KEY_USB_TETHERING_ENABLED, _runtime_info_key_update_cb, data);
	if (ret != 0) {
		util_runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_WIFI_HOTSPOT_ENABLED, _runtime_info_key_update_cb);
		util_runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_BLUETOOTH_TETHERING_ENABLED, _runtime_info_key_update_cb);
		ERR("util_runtime_info_set_changed_cb failed");
		return FAIL;
	}

	indicator_mobile_hotspot_change_cb(data);

	return OK;
}

static int unregister_mobile_hotspot_module(void)
{
	util_runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_WIFI_HOTSPOT_ENABLED, _runtime_info_key_update_cb);
	util_runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_BLUETOOTH_ENABLED, _runtime_info_key_update_cb);
	util_runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_USB_TETHERING_ENABLED, _runtime_info_key_update_cb);

	return OK;
}
