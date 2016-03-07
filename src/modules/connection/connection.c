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
#include <wifi.h>
#include <telephony.h>
#include <system_settings.h>
#include <runtime_info.h>
#include <vconf.h>
#include <app_event.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "util.h"
#include "box.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED5
#define MODULE_NAME		"connection"

static int register_conn_module(void *data);
static int unregister_conn_module(void);
static int wake_up_cb(void *data);
static void __deinit_tel(void);

static int transfer_state = -1;
static int isBTIconShowing = 0;
static telephony_handle_list_s tel_list;
static int updated_while_lcd_off = 0;
static int prevIndex = -1;
static event_handler_h event;


icon_s conn = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_conn_module,
	.fini = unregister_conn_module,
	.wake_up = wake_up_cb
};

typedef enum {
	LEVEL_MIN = 0,
	LEVEL_2G = LEVEL_MIN,
	LEVEL_EDGE,
	LEVEL_3G,
	LEVEL_H,
	LEVEL_H_PLUS,
	LEVEL_3G_PLUS,
	LEVEL_LTE,
	LEVEL_4G,
	LEVEL_BT_TETHERING,
	LEVEL_LAST
} icon_e;

static const char *icon_path[LEVEL_LAST] = {
	[LEVEL_2G] = "Connection/B03_connection_G.png",
	[LEVEL_EDGE] = "Connection/B03_connection_E.png",
	[LEVEL_3G] = "Connection/B03_connection_3G.png",
	[LEVEL_H] = "Connection/B03_connection_H.png",
	[LEVEL_H_PLUS] = "Connection/B03_connection_H+.png",
	[LEVEL_3G_PLUS] = "Connection/B03_connection_3G+.png",
	[LEVEL_LTE] = "Connection/B03_connection_LTE.png",
	[LEVEL_4G] = "Connection/B03_connection_4G.png",
	[LEVEL_BT_TETHERING] = "Bluetooth, NFC, GPS/B03_BT_tethering_network.png"
};

static void set_app_state(void *data)
{
	conn.ad = data;
}

static void show_connection_transfer_icon(void *data)
{
	int state = 0;
	int ret = 0;

	ret = vconf_get_int(VCONFKEY_PACKET_STATE, &state);

	if (ret == OK) {

		if (transfer_state == state) {
			_D("same transfer state");
			return;
		}

		_D("type %d",state);
		transfer_state = state;

		switch (state) {
		case VCONFKEY_PACKET_RX:
			util_signal_emit(conn.ad, "indicator.connection.updown.download", "indicator.prog");
			break;
		case VCONFKEY_PACKET_TX:
			util_signal_emit(conn.ad, "indicator.connection.updown.upload", "indicator.prog");
			break;
		case VCONFKEY_PACKET_RXTX:
			util_signal_emit(conn.ad, "indicator.connection.updown.updownload", "indicator.prog");
			break;
		case VCONFKEY_PACKET_NORMAL:
			util_signal_emit(conn.ad, "indicator.connection.updown.none", "indicator.prog");
			break;
		default:
			break;
		}
	}
}

static void show_image_icon(int type)
{
	if (prevIndex == type)
		return;

	conn.img_obj.data = icon_path[type];
	icon_show(&conn);

	prevIndex = type;
	util_signal_emit(conn.ad, "indicator.connection.show", "indicator.prog");
}

static void hide_image_icon(void)
{
	transfer_state = -1;

	icon_hide(&conn);

	util_signal_emit(conn.ad, "indicator.connection.hide", "indicator.prog");
	util_signal_emit(conn.ad, "indicator.connection.updown.hide", "indicator.prog");

	prevIndex = -1;
}

static icon_e _icon_level_for_network_type(telephony_network_type_e net_type)
{
	_D("network_type:%d", net_type);

	switch (net_type) {
	case TELEPHONY_NETWORK_TYPE_GSM:
	case TELEPHONY_NETWORK_TYPE_GPRS:
		return LEVEL_2G;
	case TELEPHONY_NETWORK_TYPE_EDGE:
		return LEVEL_EDGE;
	case TELEPHONY_NETWORK_TYPE_UMTS:
		return LEVEL_3G;
	case TELEPHONY_NETWORK_TYPE_HSDPA:
		return LEVEL_H;
	case TELEPHONY_NETWORK_TYPE_LTE:
		return LEVEL_LTE;
	default:
		return LEVEL_LAST;
	}
}

static icon_e _icon_level_for_ps_network_type(telephony_network_ps_type_e net_type)
{
	_D("network_ps_type:%d", net_type);

	switch (net_type) {
	case TELEPHONY_NETWORK_PS_TYPE_HSDPA:
	case TELEPHONY_NETWORK_PS_TYPE_HSUPA:
	case TELEPHONY_NETWORK_PS_TYPE_HSPA:
		return LEVEL_H;
	case TELEPHONY_NETWORK_PS_TYPE_HSPAP:
		return LEVEL_H_PLUS;
	default:
		return LEVEL_LAST;
	}
}

static void _view_icon_update_ps_network(telephony_network_ps_type_e ps_type, void *data)
{
	icon_e icon = _icon_level_for_ps_network_type(ps_type);

	if (icon != LEVEL_LAST) {
		show_image_icon(icon);
		show_connection_transfer_icon(data);
	} else
		hide_image_icon();
}

static void _view_icon_update_network(telephony_network_service_state_e state,
		telephony_network_type_e network_type, void *data)
{
	icon_e icon;

	switch (state) {
		case TELEPHONY_NETWORK_SERVICE_STATE_IN_SERVICE:
			icon = _icon_level_for_network_type(network_type);
			if (icon != LEVEL_LAST) {
				show_image_icon(icon);
				show_connection_transfer_icon(data);
			} else
				hide_image_icon();
			break;
		case TELEPHONY_NETWORK_SERVICE_STATE_OUT_OF_SERVICE:
		case TELEPHONY_NETWORK_SERVICE_STATE_EMERGENCY_ONLY:
		default:
			hide_image_icon();
			break;
	}
}

static void _view_icon_update(telephony_h handle, void *data)
{
	telephony_network_ps_type_e ps_type;
	telephony_network_service_state_e service_state;
	telephony_network_type_e network_type;

	retm_if(data == NULL, "Invalid parameter!");

	int ret = telephony_network_get_ps_type(handle, &ps_type);
	retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_network_get_ps_type failed %s",
			get_error_message(ret));

	ret = telephony_network_get_type(handle, &network_type);
	retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_network_get_type failed %s",
			get_error_message(ret));

	ret = telephony_network_get_service_state(handle, &service_state);
	retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_network_get_service_state failed %s",
			get_error_message(ret));

	if (ps_type != TELEPHONY_NETWORK_PS_TYPE_UNKNOWN)
		_view_icon_update_ps_network(ps_type, data);
	else
		_view_icon_update_network(service_state, network_type, data);
}

static void on_noti(telephony_h handle, const char *noti_id, void *data, void *user_data)
{
	telephony_network_default_data_subs_e default_subscription;
	wifi_connection_state_e state;

	int ret = 0;
	bool val;
	retm_if(user_data == NULL, "invalid parameter!!");

	ret = wifi_get_connection_state(&state);
	_D("WIFI Status : %d", state);
	retm_if(ret != WIFI_ERROR_NONE, "wifi_get_connection_state failed: %s",
			get_error_message(ret));

	if (state == WIFI_CONNECTION_STATE_CONNECTED) {
		_D("WIFI connected, so hide connection icon");
		hide_image_icon();
		return;
	}

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, &val);
	retm_if(ret != SYSTEM_SETTINGS_ERROR_NONE, "system_settings_get_value_bool failed: %s",
			get_error_message(ret));

	if (val) {
		_D("FLIGHT MODE ON");
		hide_image_icon();
		return;
	}

	ret = runtime_info_get_value_bool(RUNTIME_INFO_KEY_BLUETOOTH_TETHERING_ENABLED, &val);
	retm_if(ret != RUNTIME_INFO_ERROR_NONE, "runtime_info_get_value_bool failed: %s",
			get_error_message(ret));

	if (val) {
		_D("bluetooth tethering on");
		isBTIconShowing = 1;
		show_image_icon(LEVEL_BT_TETHERING);
		util_signal_emit(conn.ad, "indicator.connection.updown.hide", "indicator.prog");
		return;
	} else {
		_D("bluetooth tethering off");
		if (isBTIconShowing == 1) {
			isBTIconShowing = 0;
			hide_image_icon();
		}
	}

	ret = telephony_network_get_default_data_subscription(handle, &default_subscription);
	retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_network_get_default_data_subscription failed %s",
			get_error_message(ret));

	switch (default_subscription) {
		case TELEPHONY_NETWORK_DEFAULT_DATA_SUBS_SIM1:
			_view_icon_update(handle, user_data);
			break;
		case TELEPHONY_NETWORK_DEFAULT_DATA_SUBS_SIM2:
		case TELEPHONY_NETWORK_DEFAULT_DATA_SUBS_UNKNOWN:
		default:
			hide_image_icon();
			break;
	}
}

static int wake_up_cb(void *data)
{
	if (updated_while_lcd_off == 0)
		return OK;

	on_noti(tel_list.handle[0], NULL, NULL, data);
	return OK;
}


static void _update_status_ri(runtime_info_key_e key, void *user_data)
{
	on_noti(tel_list.handle[0], NULL, NULL, user_data);
}

static void _wifi_status_changed_cb(wifi_connection_state_e state, wifi_ap_h ap, void *user_data)
{
	int status = 0;
	int ret = 0;

	ret = wifi_get_connection_state(&status);
	if (ret == WIFI_ERROR_NONE) {
		INFO("[CB] WIFI Status: %d", status);
		if (status == WIFI_CONNECTION_STATE_CONNECTED) {
			_D("[CB] WIFI connected, so hide connection icon");
			hide_image_icon();
		} else
			on_noti(tel_list.handle[0], NULL, NULL, user_data);
	}
}

static void _flight_mode(system_settings_key_e key, void *user_data)
{
	on_noti(tel_list.handle[0], NULL, NULL, user_data);
}

static void _update_status(telephony_h handle, telephony_noti_e noti_id, void *data, void *user_data)
{
	on_noti(tel_list.handle[0], NULL, NULL, data);
}

/* Initialize TAPI */
static int __init_tel(void *data)
{
	_D("__init_tel");
	int ret, i;

	if (!tel_list.count) {
		_D("Not SIM handle returned by telephony_init");
		__deinit_tel();
		return FAIL;
	}

	telephony_noti_e events[] = { TELEPHONY_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION,
									TELEPHONY_NOTI_NETWORK_PS_TYPE,
									TELEPHONY_NOTI_NETWORK_SERVICE_STATE };

	for (i = 0; i < ARRAY_SIZE(events); i++) {
		/* Currently handle only first SIM */
		ret = telephony_set_noti_cb(tel_list.handle[0], events[i], _update_status, data);
		if (ret != TELEPHONY_ERROR_NONE) {
			_E("telephony_set_noti_cb failed: %s", get_error_message(ret));
			__deinit_tel();
			return FAIL;
		}
	}

	ret = util_wifi_set_connection_state_changed_cb(_wifi_status_changed_cb, data);
	if (ret != 0) {
		_E("util_wifi_set_connection_state_changed_cb");
		__deinit_tel();
		return FAIL;
	}

	ret = util_system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE,
			_flight_mode, data);

	if (ret != 0) {
		_E("util_system_settings_set_changed_cb failed");
		__deinit_tel();
		return FAIL;
	}

	ret = util_runtime_info_set_changed_cb(RUNTIME_INFO_KEY_BLUETOOTH_TETHERING_ENABLED,
			_update_status_ri, data);

	if (ret != 0) {
		_E("util_runtime_info_set_changed_cb failed");
		__deinit_tel();
		return FAIL;
	}

	on_noti(tel_list.handle[0], NULL, NULL, data);

	return OK;
}

/* De-initialize telephony */
static void __deinit_tel()
{
	_D("__deinit_tel");

	util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode);
	util_wifi_unset_connection_state_changed_cb(_wifi_status_changed_cb);
	util_runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_BLUETOOTH_TETHERING_ENABLED, _update_status_ri);

	if (tel_list.count)
		telephony_deinit(&tel_list);
	tel_list.count = 0;

	hide_image_icon();
}

static void tel_ready_cb(telephony_state_e state, void *user_data)
{
	if (state == TELEPHONY_STATE_READY)
		__init_tel(user_data);
	else if (state == TELEPHONY_STATE_NOT_READY)
		__deinit_tel();
}

static void data_event_cb(const char *event_name, bundle *event_data, void *user_data)
{
	char *value;
	int ret;
	ret = bundle_get_str(event_data, EVENT_KEY_MOBILE_DATA_STATE, &value);
	if (ret != BUNDLE_ERROR_NONE) {
		_E("bundle_get_str failed: %s", get_error_message(ret));
		hide_image_icon();
		return;
	}
	_D("bundle value:%s", value);

	if (!strcmp(value, "off"))
		hide_image_icon();
	else
		on_noti(tel_list.handle[0], NULL, NULL, user_data);
}

static int register_conn_module(void *data)
{
	int ret;
	telephony_state_e state;

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = telephony_get_state(&state);
	retvm_if(ret != TELEPHONY_ERROR_NONE, FAIL, "telephony_get_state failed: %s",
			get_error_message(ret));

	ret = telephony_init(&tel_list);
	retvm_if(ret != TELEPHONY_ERROR_NONE, FAIL, "telephony_init failed: %s",
			get_error_message(ret));

	if (state == TELEPHONY_STATE_READY) {
		_D("Telephony ready");
		retv_if(__init_tel(data) != OK, FAIL);
	} else if (state == TELEPHONY_STATE_NOT_READY)
		_D("Telephony not ready");

	ret = telephony_set_state_changed_cb(tel_ready_cb, data);
	if (ret != TELEPHONY_ERROR_NONE) {
		_E("telephony_set_state_changed_cb failed: %s", get_error_message(ret));
		__deinit_tel();
		return FAIL;
	}

	ret = event_add_event_handler(SYSTEM_EVENT_MOBILE_DATA_STATE, data_event_cb, data, &event);
	if (ret != EVENT_ERROR_NONE) {
		_E("event_add_event_handler failed: %s", get_error_message(ret));
		__deinit_tel();
		return FAIL;
	}

	return OK;
}

static int unregister_conn_module(void)
{
	telephony_unset_state_changed_cb(tel_ready_cb);
	__deinit_tel();
	event_remove_event_handler(event);

	return OK;
}
