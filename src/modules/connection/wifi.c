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
#define TIMER_INTERVAL	0.3

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
static Eina_Bool wifi_transferring = EINA_FALSE;
static int updated_while_lcd_off = 0;
static int prevIndex = -1;



static void set_app_state(void* data)
{
	wifi.ad = data;
}

static void show_image_icon(void *data, int index)
{
	if(prevIndex == index)
	{
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

static void show_wifi_transfer_icon(void* data)
{
	int type = -1;
	int status = 0;

	if (vconf_get_int(VCONFKEY_WIFI_TRANSFER_STATE, &status) < 0)
	{
		ERR("Error getting VCONFKEY_WIFI_TRANSFER_STATE value");
		return;
	}

	switch(status)
	{
	case VCONFKEY_WIFI_TRANSFER_STATE_TXRX://TX/RX BOTH
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

	if(transfer_state==type)
	{
		DBG("same transfer state");
		return;
	}

	transfer_state = type;
	switch (type)
	{
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

static void _wifi_changed_cb(keynode_t *node, void *data)
{
	bool wifi_state = FALSE;
	int status, strength;
	int ret;

	ret_if(!data);

	if (icon_get_update_flag()==0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_int(VCONFKEY_WIFI_STRENGTH, &strength);
	if (ret == OK) {
		if (strength < VCONFKEY_WIFI_STRENGTH_MIN) {
			strength = VCONFKEY_WIFI_STRENGTH_MIN;
		}
		else if (strength > VCONFKEY_WIFI_STRENGTH_MAX) {
			strength = VCONFKEY_WIFI_STRENGTH_MAX;
		}
	} else {
		strength = VCONFKEY_WIFI_STRENGTH_MAX;
	}

	if (strength <= 0) strength = 1;

	/* Second, check wifi status */
	ret = wifi_is_activated(&wifi_state);
	_D("wifi_state : %d", wifi_state);
	if(ret != WIFI_ERROR_NONE) {
		_E("wifi_is_activated error. ret is [%d]", ret);
	}

	ret = wifi_get_connection_state(&status);
	if (ret == WIFI_ERROR_NONE) {
		DBG("CONNECTION WiFi Status: %d", status);
		switch(status) {
		case WIFI_CONNECTION_STATE_CONNECTED:
			show_wifi_transfer_icon(data);
			show_image_icon(data, strength-1);
			break;
		default: //WIFI_CONNECTION_STATE_DISCONNECTED
			hide_image_icon();
			break;
		}
	}

	return;
}

static void _wifi_device_state_changed_cb(wifi_device_state_e state, void *user_data)
{
	bool wifi_state = FALSE;
	int ret, strength;

	ret_if(!user_data);

	if (icon_get_update_flag()==0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_int(VCONFKEY_WIFI_STRENGTH, &strength);
	if (ret == OK) {
		if (strength < VCONFKEY_WIFI_STRENGTH_MIN) {
			strength = VCONFKEY_WIFI_STRENGTH_MIN;
		}
		else if (strength > VCONFKEY_WIFI_STRENGTH_MAX) {
			strength = VCONFKEY_WIFI_STRENGTH_MAX;
		}
	} else {
		strength = VCONFKEY_WIFI_STRENGTH_MAX;
	}

	if (strength <= 0) strength = 1;

	ret = wifi_is_activated(&wifi_state);
	_D("wifi_state : %d", wifi_state);
	if(ret != WIFI_ERROR_NONE) {
		_E("wifi_is_activated error. ret is [%d]", ret);
	}

	if (wifi_state) {
		show_wifi_transfer_icon(user_data);
		show_image_icon(user_data, strength-1);
	} else {
		hide_image_icon();
	}

	return;
}

static void _wifi_connection_state_changed_cb(wifi_connection_state_e state, wifi_ap_h ap, void *user_data)
{
	ret_if(!user_data);

	_wifi_changed_cb(NULL, user_data);

	return;
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		return OK;
	}

	_wifi_changed_cb(NULL, data);
	return OK;
}

static bool _wifi_init(void)
{
	int ret = -1;
	ret = wifi_initialize();
	if (ret != WIFI_ERROR_NONE) {
		_E("wifi_initialize is fail : %d", ret);
		return FALSE;
	}

	return TRUE;
}

static void _wifi_fini(void)
{
	int ret = -1;
	ret = wifi_deinitialize();
	if (ret != WIFI_ERROR_NONE) {
		_E("wifi_deinitialize is fail : %d", ret);
	}
}

static int register_wifi_module(void *data)
{
	int r = 0, ret = -1;

	retv_if(!data, 0);

	set_app_state(data);
	_wifi_init();

	ret = wifi_set_device_state_changed_cb(_wifi_device_state_changed_cb, data);
	if (ret != WIFI_ERROR_NONE) r = ret;

	ret = wifi_set_connection_state_changed_cb(_wifi_connection_state_changed_cb, data);
	if (ret != WIFI_ERROR_NONE) r = ret;

	ret = vconf_notify_key_changed(VCONFKEY_WIFI_STRENGTH, _wifi_changed_cb, data);
	if (ret != OK) r = r | ret;

	ret = vconf_notify_key_changed(VCONFKEY_WIFI_TRANSFER_STATE, _wifi_changed_cb, data);
	if (ret != OK) r = r | ret;

	_wifi_changed_cb(NULL, data);

	return r;
}

static int unregister_wifi_module(void)
{
	int ret;

	ret = wifi_unset_device_state_changed_cb();
	ret = wifi_unset_connection_state_changed_cb();
	ret = ret | vconf_ignore_key_changed(VCONFKEY_WIFI_STRENGTH, _wifi_changed_cb);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_WIFI_TRANSFER_STATE, _wifi_changed_cb);

	if (wifi_transferring == EINA_TRUE) {
		wifi_transferring  = EINA_FALSE;
	}

	_wifi_fini();

	return ret;
}
/* End of file */
