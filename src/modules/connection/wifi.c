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
#include <wifi.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "util.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED6
#define MODULE_NAME		"wifi"

static int register_wifi_module(void *data);
static int unregister_wifi_module(void);
static int wake_up_cb(void *data);

icon_s wifi = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_wifi_module,
	.fini = unregister_wifi_module,
	.wake_up = wake_up_cb
};

enum {
	LEVEL_WIFI_01 = 0,
	LEVEL_WIFI_02,
	LEVEL_WIFI_03,
	LEVEL_WIFI_04,
	LEVEL_WIFI_MAX
};

static const char *icon_path[LEVEL_WIFI_MAX] = {
	[LEVEL_WIFI_01] = "Connection/B03_data_downloading_Wifi_01.png",
	[LEVEL_WIFI_02] = "Connection/B03_data_downloading_Wifi_02.png",
	[LEVEL_WIFI_03] = "Connection/B03_data_downloading_Wifi_03.png",
	[LEVEL_WIFI_04] = "Connection/B03_data_downloading_Wifi_04.png",
};

static int transfer_state = -1;
static int updated_while_lcd_off = 0;
static int prevIndex = -1;
static wifi_rssi_level_e rssi_level;


static void set_app_state(void *data)
{
	wifi.ad = data;
}

static void show_image_icon(void *data, int index)
{
	if (prevIndex == index) {
		return;
	}

	wifi.img_obj.data = icon_path[index];
	icon_show(&wifi);

	prevIndex = index;
	util_signal_emit(wifi.ad, "indicator.wifi.show", "indicator.prog");
}

static void hide_image_icon(void)
{
	transfer_state = -1;

	icon_hide(&wifi);

	prevIndex = -1;
	util_signal_emit(wifi.ad, "indicator.wifi.hide", "indicator.prog");
	util_signal_emit(wifi.ad, "indicator.wifi.updown.hide", "indicator.prog");
}

static void show_wifi_transfer_icon(void *data)
{
	int type = -1;
	int status = 0;

	if (vconf_get_int(VCONFKEY_WIFI_TRANSFER_STATE, &status) < 0) {
		_E("Error getting VCONFKEY_WIFI_TRANSFER_STATE value");
		return;
	}

	switch (status) {
	case VCONFKEY_WIFI_TRANSFER_STATE_TXRX:
		type = TRANSFER_UPDOWN;
		break;
	case VCONFKEY_WIFI_TRANSFER_STATE_TX:
		type = TRANSFER_UP;
		break;
	case VCONFKEY_WIFI_TRANSFER_STATE_RX:
		type = TRANSFER_DOWN;
		break;
	case VCONFKEY_WIFI_TRANSFER_STATE_NONE:
		type = TRANSFER_NONE;
		break;
	default:
		break;
	}

	if (transfer_state == type) {
		_D("same transfer state");
		return;
	}

	transfer_state = type;
	switch (type) {
		case TRANSFER_NONE:
			util_signal_emit(wifi.ad, "indicator.wifi.updown.none", "indicator.prog");
			break;
		case TRANSFER_DOWN:
			util_signal_emit(wifi.ad, "indicator.wifi.updown.download", "indicator.prog");
			break;
		case TRANSFER_UP:
			util_signal_emit(wifi.ad, "indicator.wifi.updown.upload", "indicator.prog");
			break;
		case TRANSFER_UPDOWN:
			util_signal_emit(wifi.ad, "indicator.wifi.updown.updownload", "indicator.prog");
			break;
		default:
			break;
	}
}

static int _rssi_level_to_strength(wifi_rssi_level_e level)
{
	switch (level) {
		case WIFI_RSSI_LEVEL_0:
			return LEVEL_WIFI_01;
		case WIFI_RSSI_LEVEL_2:
		case WIFI_RSSI_LEVEL_1:
			return LEVEL_WIFI_02;
		case WIFI_RSSI_LEVEL_3:
			return LEVEL_WIFI_03;
		case WIFI_RSSI_LEVEL_4:
			return LEVEL_WIFI_04;
		default:
			return WIFI_RSSI_LEVEL_0;
	}
}

static void _wifi_view_update(void *data)
{
	bool activated;
	wifi_connection_state_e state;
	int ret;

	ret_if(!data);

	if (icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = wifi_is_activated(&activated);
	retm_if(ret != WIFI_ERROR_NONE, "wifi_is_activated failed: %s", get_error_message(ret));
	_D("wifi_state : %d", activated);

	if (!activated) {
		hide_image_icon();
		return;
	}

	ret = wifi_get_connection_state(&state);
	retm_if(ret != WIFI_ERROR_NONE, "wifi_get_connection_state failed: %s", get_error_message(ret));
	if (state != WIFI_CONNECTION_STATE_CONNECTED) {
		hide_image_icon();
		return;
	}

	show_wifi_transfer_icon(data);
	show_image_icon(data, _rssi_level_to_strength(rssi_level));

	return;
}

static void _wifi_rssi_level_changed(wifi_rssi_level_e level, void *data)
{
	rssi_level = level;
	_wifi_view_update(data);
	return;
}

static void _wifi_connection_state_changed(wifi_connection_state_e state, wifi_ap_h ap, void *data)
{
	_wifi_view_update(data);
	return;
}

static void _wifi_device_state_changed(wifi_device_state_e state, void *data)
{
	_wifi_view_update(data);
	return;
}

static int wake_up_cb(void *data)
{
	if (updated_while_lcd_off == 0) {
		return OK;
	}

	_wifi_view_update(data);
	return OK;
}

static void _wifi_changed_cb(keynode_t *node, void *user_data)
{
	_wifi_view_update(user_data);
}

static int register_wifi_module(void *data)
{
	retv_if(!data, 0);

	set_app_state(data);

	int ret = util_wifi_initialize();
	retvm_if(ret != WIFI_ERROR_NONE, FAIL, "util_wifi_initialize failed : %s", get_error_message(ret));

	ret = util_wifi_set_device_state_changed_cb(_wifi_device_state_changed, data);
	if (ret != WIFI_ERROR_NONE) {
		_E("util_wifi_set_device_state_changed_cb failed: %s", get_error_message(ret));
		unregister_wifi_module();
		return FAIL;
	}

	ret = util_wifi_set_connection_state_changed_cb(_wifi_connection_state_changed, data);
	if (ret != 0) {
		_E("util_wifi_set_connection_state_changed_cb failed");
		unregister_wifi_module();
		return FAIL;
	}

	ret = wifi_set_rssi_level_changed_cb(_wifi_rssi_level_changed, data);
	if (ret != WIFI_ERROR_NONE) {
		_E("wifi_set_rssi_level_changed_cb failed: %s", get_error_message(ret));
		unregister_wifi_module();
		return FAIL;
	}

	ret = vconf_notify_key_changed(VCONFKEY_WIFI_TRANSFER_STATE, _wifi_changed_cb, data);
	if (ret != WIFI_ERROR_NONE) {
		_E("vconf_notify_key_changed failed: %s", get_error_message(ret));
		unregister_wifi_module();
		return FAIL;
	}

	_wifi_view_update(data);

	return OK;
}

static int unregister_wifi_module(void)
{
	util_wifi_unset_device_state_changed_cb(_wifi_device_state_changed);
	util_wifi_unset_connection_state_changed_cb(_wifi_connection_state_changed);
	wifi_unset_rssi_level_changed_cb();
	vconf_ignore_key_changed(VCONFKEY_WIFI_TRANSFER_STATE, _wifi_changed_cb);


	int ret = util_wifi_deinitialize();
	if (ret != WIFI_ERROR_NONE) {
		_E("util_wifi_deinitialize failed : %s", get_error_message(ret));
	}

	return OK;
}
