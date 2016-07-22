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
#include <tethering.h>

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

typedef struct {
	tethering_h handle;
	bool bt_enabled;
	bool bt_client_connected;
	bool wifi_enabled;
	bool wifi_client_connected;
	bool usb_enabled;
	bool usb_client_connected;
} tethering_data_t;

enum {
	TETHERING_MULTI_ON_CONNECTED = 0,
	TETHERING_MULTI_ON_NOT_CONNECTED,
	TETHERING_BT_ON_CONNECTED,
	TETHERING_BT_ON_NOT_CONNECTED,
	TETHERING_USB_ON_CONNECTED,
	TETHERING_USB_ON_NOT_CONNECTED,
	TETHERING_WIFI_ON_CONNECTED,
	TETHERING_WIFI_ON_NOT_CONNECTED,
	TETHERING_MAX,
};

static const char *icon_path[TETHERING_MAX] = {
	[TETHERING_MULTI_ON_CONNECTED] 		= "tethering/B03_All_connected.png",
	[TETHERING_MULTI_ON_NOT_CONNECTED] 	= "tethering/B03_All_no_connected.png",
	[TETHERING_BT_ON_CONNECTED] 		= "tethering/B03_BT_connected.png",
	[TETHERING_BT_ON_NOT_CONNECTED] 	= "tethering/B03_BT_no_connected.png",
	[TETHERING_USB_ON_CONNECTED] 		= "tethering/B03_USB_connected.png",
	[TETHERING_USB_ON_NOT_CONNECTED] 	= "tethering/B03_USB_no_connected.png",
	[TETHERING_WIFI_ON_CONNECTED] 		= "tethering/B03_Wi_Fi_connected.png",
	[TETHERING_WIFI_ON_NOT_CONNECTED] 	= "tethering/B03_Wi_Fi_no_connected.png",
};

static int updated_while_lcd_off = 0;
static int prevIndex = -1;
static tethering_data_t tet_data;


static void set_app_state(void *data)
{
	mobile_hotspot.ad = data;
}

static void show_image_icon(int type)
{
	if (prevIndex == type)
		return;

	mobile_hotspot.img_obj.data = icon_path[type];
	icon_show(&mobile_hotspot);

	prevIndex = type;
}

static void hide_image_icon(void)
{
	icon_hide(&mobile_hotspot);

	prevIndex = -1;
}

static void _mobile_hotspot_view_update(tethering_data_t *data)
{
	retm_if(data == NULL, "Invalid parameter!");

	if (icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	/* How many tethering methods are on */
	int sum = (data->usb_enabled ? 1 : 0) + (data->wifi_enabled ? 1 : 0) + (data->bt_enabled ? 1 : 0);

	if (sum == 0)
		hide_image_icon();

	if (sum >= 2) {
		if (data->usb_client_connected || data->wifi_client_connected || data->bt_client_connected)
			show_image_icon(TETHERING_MULTI_ON_CONNECTED);
		else
			show_image_icon(TETHERING_MULTI_ON_NOT_CONNECTED);
		return;
	}

	if (data->bt_enabled) {
		if (data->bt_client_connected)
			show_image_icon(TETHERING_BT_ON_CONNECTED);
		else
			show_image_icon(TETHERING_BT_ON_NOT_CONNECTED);
	}

	if (data->usb_enabled) {
		if (data->usb_client_connected)
			show_image_icon(TETHERING_USB_ON_CONNECTED);
		else
			show_image_icon(TETHERING_USB_ON_NOT_CONNECTED);
	}

	if (data->wifi_enabled) {
		if (data->wifi_client_connected)
			show_image_icon(TETHERING_WIFI_ON_CONNECTED);
		else
			show_image_icon(TETHERING_WIFI_ON_NOT_CONNECTED);
	}
}

static int wake_up_cb(void *data)
{
	if (updated_while_lcd_off == 0)
		return OK;

	_mobile_hotspot_view_update(&tet_data);
	return OK;
}

static bool _teter_client_cb(tethering_client_h client, void *user_data)
{
	tethering_data_t *td = user_data;
	tethering_type_e type;

	int ret = tethering_client_get_tethering_type(client, &type);
	retvm_if(ret != TETHERING_ERROR_NONE, false, "tethering_client_get_tethering_type failed: %d", get_error_message(ret));

	switch (type) {
	case TETHERING_TYPE_BT:
		td->bt_client_connected = true;
		break;
	case TETHERING_TYPE_WIFI:
		td->wifi_client_connected = true;
		break;
	case TETHERING_TYPE_USB:
		td->usb_client_connected = true;
		break;
	case TETHERING_TYPE_ALL:
		td->bt_client_connected = true;
		td->wifi_client_connected = true;
		td->usb_client_connected = true;
		break;
	default:
		break;
	}
	return true;
}

static int _tethering_data_reload(tethering_data_t *data)
{
	data->bt_enabled = tethering_is_enabled(data->handle, TETHERING_TYPE_BT);
	data->wifi_enabled = tethering_is_enabled(data->handle, TETHERING_TYPE_WIFI);
	data->usb_enabled = tethering_is_enabled(data->handle, TETHERING_TYPE_USB);

	data->bt_client_connected = data->wifi_client_connected = data->usb_client_connected = false;

	if (!data->bt_enabled && !data->wifi_enabled && !data->usb_enabled)
		return 0;

	int ret = tethering_foreach_connected_clients(data->handle, TETHERING_TYPE_ALL, _teter_client_cb, data);
	retvm_if(ret != TETHERING_ERROR_NONE, FAIL, "tethering_foreach_connected_clients failed: %d", get_error_message(ret));

	return 0;
}

static void _tethering_data_connection_state_changed(tethering_client_h client, bool opened, void *user_data)
{
	_tethering_data_reload(user_data);
	_mobile_hotspot_view_update(user_data);
}

static void _tethering_data_enabled_changed(tethering_error_e result, tethering_type_e type, bool is_requested, void *user_data)
{
	_tethering_data_reload(user_data);
	_mobile_hotspot_view_update(user_data);
}

static void _tethering_data_disabled(tethering_error_e result, tethering_type_e type, tethering_disabled_cause_e cause, void *user_data)
{
	_tethering_data_reload(user_data);
	_mobile_hotspot_view_update(user_data);
}

static int _tethering_data_init(tethering_data_t *data)
{
	int ret = tethering_create(&data->handle);
	retvm_if(ret != TETHERING_ERROR_NONE, FAIL, "tethering_create failed: %d", get_error_message(ret));

	ret = tethering_set_connection_state_changed_cb(data->handle, TETHERING_TYPE_ALL, _tethering_data_connection_state_changed, data);
	if (ret != TETHERING_ERROR_NONE) {
		tethering_destroy(data->handle);
		_E("tethering_set_connection_state_changed_cb failed: %d", get_error_message(ret));
		return FAIL;
	}

	ret = tethering_set_enabled_cb(data->handle, TETHERING_TYPE_ALL, _tethering_data_enabled_changed, data);
	if (ret != TETHERING_ERROR_NONE) {
		tethering_destroy(data->handle);
		_E("tethering_set_enabled_cb failed: %d", get_error_message(ret));
		return FAIL;
	}

	ret = tethering_set_disabled_cb(data->handle, TETHERING_TYPE_ALL, _tethering_data_disabled, data);
	if (ret != TETHERING_ERROR_NONE) {
		tethering_destroy(data->handle);
		_E("tethering_set_disabled_cb failed: %d", get_error_message(ret));
		return FAIL;
	}

	return _tethering_data_reload(data);
}

static void _tethering_data_shutdown(tethering_data_t *data)
{
	tethering_destroy(data->handle);
}

static int register_mobile_hotspot_module(void *data)
{
	int ret;

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = _tethering_data_init(&tet_data);
	retvm_if(ret != 0, FAIL, "_tethering_data_init failed");

	_mobile_hotspot_view_update(&tet_data);

	return OK;
}

static int unregister_mobile_hotspot_module(void)
{
	_tethering_data_shutdown(&tet_data);

	return OK;
}
