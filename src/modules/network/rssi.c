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

#include <telephony.h>
#include <stdio.h>
#include <stdlib.h>
#include <system_settings.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "indicator_gui.h"
#include "util.h"
#include "log.h"

#define RSSI1_ICON_PRIORITY		INDICATOR_PRIORITY_FIXED2
#define RSSI2_ICON_PRIORITY		INDICATOR_PRIORITY_FIXED3
#define SIMCARD_ICON_PRIORITY	INDICATOR_PRIORITY_FIXED4

#define MODULE_NAME			"RSSI"

#define ICON_NOSIM		_("IDS_COM_BODY_NO_SIM")
#define ICON_SEARCH		_("IDS_COM_BODY_SEARCHING")
#define ICON_NOSVC		_("IDS_CALL_POP_NOSERVICE")

static int register_rssi_module(void *data);
static int unregister_rssi_module(void);
static int language_changed_cb(void *data);
static int wake_up_cb(void *data);
static void _view_update(void *user_data);
static void _flight_mode(system_settings_key_e key, void *data);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif

static telephony_handle_list_s tel_list;

icon_s rssi = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = RSSI1_ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_rssi_module,
	.fini = unregister_rssi_module,
	.lang_changed = language_changed_cb,
	.wake_up = wake_up_cb,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

typedef enum {
	LEVEL_RSSI_MIN = 0,
	LEVEL_FLIGHT,
	LEVEL_NOSIM,
	LEVEL_SEARCH,
	LEVEL_NOSVC,
	LEVEL_LIMITED,
	LEVEL_RSSI_SIM1_0,
	LEVEL_RSSI_SIM1_1,
	LEVEL_RSSI_SIM1_2,
	LEVEL_RSSI_SIM1_3,
	LEVEL_RSSI_SIM1_4,
	LEVEL_RSSI_ROAMING_0,
	LEVEL_RSSI_ROAMING_1,
	LEVEL_RSSI_ROAMING_2,
	LEVEL_RSSI_ROAMING_3,
	LEVEL_RSSI_ROAMING_4,
	LEVEL_MAX
} rssi_icon_e;

static int updated_while_lcd_off = 0;

static const char *icon_path[LEVEL_MAX] = {
	[LEVEL_FLIGHT] = "RSSI/B03_RSSI_Flightmode.png",
	[LEVEL_NOSIM] = "RSSI/B03_RSSI_NoSim.png",
	[LEVEL_SEARCH] = "RSSI/B03_RSSI_Searching.png",
	[LEVEL_NOSVC] = "RSSI/B03_RSSI_NoService.png",
	[LEVEL_LIMITED] = "RSSI/B03_Network_LimitedService.png",
	[LEVEL_RSSI_SIM1_0] = "RSSI/B03_RSSI_Sim_00.png",
	[LEVEL_RSSI_SIM1_1] = "RSSI/B03_RSSI_Sim_01.png",
	[LEVEL_RSSI_SIM1_2] = "RSSI/B03_RSSI_Sim_02.png",
	[LEVEL_RSSI_SIM1_3] = "RSSI/B03_RSSI_Sim_03.png",
	[LEVEL_RSSI_SIM1_4] = "RSSI/B03_RSSI_Sim_04.png",
	[LEVEL_RSSI_ROAMING_0] = "RSSI/B03_RSSI_roaming_00.png",
	[LEVEL_RSSI_ROAMING_1] = "RSSI/B03_RSSI_roaming_01.png",
	[LEVEL_RSSI_ROAMING_2] = "RSSI/B03_RSSI_roaming_02.png",
	[LEVEL_RSSI_ROAMING_3] = "RSSI/B03_RSSI_roaming_03.png",
	[LEVEL_RSSI_ROAMING_4] = "RSSI/B03_RSSI_roaming_04.png",
};

static void set_app_state(void* data)
{
	rssi.ad = data;
}

static void _show_image_icon(void *data, int index)
{
	if (index < LEVEL_RSSI_MIN) {
		index = LEVEL_RSSI_MIN;
	} else if (index >= LEVEL_MAX) {
		index = LEVEL_NOSVC;
	}

	rssi.img_obj.width = DEFAULT_ICON_WIDTH;
	rssi.img_obj.data = icon_path[index];

	icon_show(&rssi);

	util_signal_emit(rssi.ad, "indicator.rssi1.show", "indicator.prog");
}

static int language_changed_cb(void *data)
{
	_view_update(data);

	return INDICATOR_ERROR_NONE;
}

static int wake_up_cb(void *data)
{
	if (!updated_while_lcd_off) return INDICATOR_ERROR_NONE;

	_view_update(data);

	return INDICATOR_ERROR_NONE;
}

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	char buf[256] = {0,};
	telephony_network_rssi_e level;

	if (tel_list.count <= 0) {
		return NULL;
	}

	int err = telephony_network_get_rssi(tel_list.handle[0], &level);
	retvm_if(err != TELEPHONY_ERROR_NONE, NULL, "telephony_network_get_rssi failed: %s", get_error_message(err));

	snprintf(buf, sizeof(buf), _("IDS_IDLE_BODY_PD_OUT_OF_4_BARS_OF_SIGNAL_STRENGTH"), level);

	_D("buf: %s", buf);

	return strdup(buf);
}
#endif

static rssi_icon_e icon_enum_get(bool roaming_enabled, telephony_network_rssi_e rssi)
{
	switch (rssi) {
		case TELEPHONY_NETWORK_RSSI_0:
			return roaming_enabled ? LEVEL_RSSI_ROAMING_0 : LEVEL_RSSI_SIM1_0;
		case TELEPHONY_NETWORK_RSSI_1:
		case TELEPHONY_NETWORK_RSSI_2:
			return roaming_enabled ? LEVEL_RSSI_ROAMING_1 : LEVEL_RSSI_SIM1_1;
		case TELEPHONY_NETWORK_RSSI_3:
			return roaming_enabled ? LEVEL_RSSI_ROAMING_2 : LEVEL_RSSI_SIM1_2;
		case TELEPHONY_NETWORK_RSSI_4:
		case TELEPHONY_NETWORK_RSSI_5:
			return roaming_enabled ? LEVEL_RSSI_ROAMING_3 : LEVEL_RSSI_SIM1_3;
		case TELEPHONY_NETWORK_RSSI_6:
			return roaming_enabled ? LEVEL_RSSI_ROAMING_4 : LEVEL_RSSI_SIM1_4;
		default:
			_E("Unhandled rssi level");
			return LEVEL_RSSI_MIN;
	}
}

static void _rssi_icon_update(telephony_h handle, void *user_data)
{
	telephony_network_rssi_e signal;
	bool roaming;

	int ret = telephony_network_get_rssi(handle, &signal);
	retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_network_get_rssi failed %s", get_error_message(ret));

	_D("SIM1 signal strength level: %d", signal);

	ret = telephony_network_get_roaming_status(handle, &roaming);
	retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_network_get_roaming_status failed %s", get_error_message(ret));

	_show_image_icon(user_data, icon_enum_get(roaming, signal));
}

static void _view_update(void *user_data)
{
	bool status;
	int ret = 0;
	telephony_sim_state_e status_sim1;

	if (!icon_get_update_flag()) {
		updated_while_lcd_off = 1;
		_D("need to update %d", updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, &status);
	retm_if(ret != SYSTEM_SETTINGS_ERROR_NONE, "system_settings_get_value_bool failed: %s", get_error_message(ret));

	if (status) {
		_D("Flight mode");
		_show_image_icon(user_data, LEVEL_FLIGHT);
		return;
	}

	ret = telephony_sim_get_state(tel_list.handle[0], &status_sim1);
	retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_sim_get_state failed: %s", get_error_message(ret));

	if (status_sim1 != TELEPHONY_SIM_STATE_UNAVAILABLE) {
		telephony_network_service_state_e service_state;

		ret = telephony_network_get_service_state(tel_list.handle[0], &service_state);
		retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_network_get_service_state failed %s", get_error_message(ret));

		switch (service_state) {
		case TELEPHONY_NETWORK_SERVICE_STATE_OUT_OF_SERVICE:
			_D("Service type : NO_SERVICE");
			_show_image_icon(user_data, LEVEL_NOSVC);
			break;
		case TELEPHONY_NETWORK_SERVICE_STATE_EMERGENCY_ONLY:
			_D("Service type : EMERGENCY");
			_rssi_icon_update(tel_list.handle[0], user_data);
			break;
		case TELEPHONY_NETWORK_SERVICE_STATE_IN_SERVICE:
			_D("Service type : IN SERVICE");
			_rssi_icon_update(tel_list.handle[0], user_data);
			break;
		default:
			_D("Unhandled service state %d", service_state);
			break;
		}
	} else {
		_D("No SIM");
		_show_image_icon(user_data, LEVEL_NOSIM);
	}
}

static void _flight_mode(system_settings_key_e key, void *data)
{
	_view_update(data);
}

static void _status_changed_cb(telephony_h handle, telephony_noti_e noti_id, void *data, void *user_data)
{
	_D("Telephony handle status chagned %d", noti_id);
	_view_update(user_data);
}

/* Initialize TAPI */
static void _init_tel(void *data)
{
	int ret, i, j;

	_D("Initialize telephony...");

	telephony_noti_e events[] = { TELEPHONY_NOTI_NETWORK_SIGNALSTRENGTH_LEVEL, TELEPHONY_NOTI_SIM_STATUS, TELEPHONY_NOTI_NETWORK_SERVICE_STATE,
		TELEPHONY_NOTI_NETWORK_PS_TYPE, TELEPHONY_NOTI_NETWORK_ROAMING_STATUS, TELEPHONY_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION,
		TELEPHONY_NOTI_NETWORK_DEFAULT_SUBSCRIPTION};

	for (i = 0; i < tel_list.count; i++) {
		for (j = 0; j < ARRAY_SIZE(events); j++) {
			ret = telephony_set_noti_cb(tel_list.handle[i], events[j], _status_changed_cb, data);
			retm_if(ret != TELEPHONY_ERROR_NONE, "telephony_set_noti_cb failed for event %d: %s", events[j], get_error_message(ret));
		}
	}

	_view_update(data);

}

/* De-initialize TAPI */
static void _deinit_tel()
{
	_D("De-initialize TAPI");
	telephony_deinit(&tel_list);
}

static void _tel_ready_cb(telephony_state_e state, void *user_data)
{
	if (state == TELEPHONY_STATE_READY) {
		_init_tel(user_data);
		_view_update(user_data);
	}
}

static int register_rssi_module(void *data)
{
	telephony_state_e state;
	int ret;

	retv_if(!data, 0);

	set_app_state(data);

	ret = util_system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode, data);
	retvm_if(ret, FAIL, "util_system_settings_set_changed_cb failed: %s", get_error_message(ret));

	ret = telephony_init(&tel_list);
	retvm_if(ret != TELEPHONY_ERROR_NONE, FAIL, "telephony_init failed: %s", get_error_message(ret));

	ret = telephony_set_state_changed_cb(_tel_ready_cb, data);
	if (ret != TELEPHONY_ERROR_NONE) {
		_E("telephony_set_state_changed_cb failed %s", get_error_message(ret));
		util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode);
		return ret;
	}

	ret = telephony_get_state(&state);
	if (ret != TELEPHONY_ERROR_NONE) {
		_E("telephony_get_state failed %s", get_error_message(ret));
		util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode);
		telephony_unset_state_changed_cb(_tel_ready_cb);
		return ret;
	}

	if (state == TELEPHONY_STATE_READY) {
		_D("Telephony ready");
		_init_tel(data);
	} else if (state == TELEPHONY_STATE_NOT_READY)
		_D("Telephony not ready");

	return ret;
}

static int unregister_rssi_module(void)
{
	int ret;

	ret = telephony_unset_state_changed_cb(_tel_ready_cb);
	if (ret != TELEPHONY_ERROR_NONE) _E("telephony_unset_state_changed_cb %s", get_error_message(ret));

	util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode);

	_deinit_tel();

	return ret;
}
/* End of file */
