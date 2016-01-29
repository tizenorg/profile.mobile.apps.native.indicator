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

#include <tapi_common.h>
#include <TelNetwork.h>
#include <TelSim.h>
#include <ITapiSim.h>
#include <TelCall.h>
#include <ITapiCall.h>
#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>
#include <ITapiNetwork.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "indicator_gui.h"
#include "util.h"
#include "connection/connection.h"
#include "processing/call_divert.h"
#include "log.h"

#define VCONFKEY_TELEPHONY_PREFERRED_VOICE_SUBSCRIPTION	"db/telephony/dualsim/preferred_voice_subscription"

#define RSSI1_ICON_PRIORITY		INDICATOR_PRIORITY_FIXED2
#define RSSI2_ICON_PRIORITY		INDICATOR_PRIORITY_FIXED3
#define SIMCARD_ICON_PRIORITY	INDICATOR_PRIORITY_FIXED4

#define MODULE_NAME			"RSSI"

#define ICON_NOSIM		_("IDS_COM_BODY_NO_SIM")
#define ICON_SEARCH		_("IDS_COM_BODY_SEARCHING")
#define ICON_NOSVC		_("IDS_CALL_POP_NOSERVICE")

#define TAPI_HANDLE_MAX  2

static int register_rssi_module(void *data);
static int unregister_rssi_module(void);
static int language_changed_cb(void *data);
static int wake_up_cb(void *data);
static void _on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data);
static void _flight_mode(keynode_t *key, void *data);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif

TapiHandle *tapi_handle[TAPI_HANDLE_MAX+1] = {0, };

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

enum {
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
};

static int registered = 0;
static int updated_while_lcd_off = 0;
#ifdef _SUPPORT_SCREEN_READER
#endif

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
	_on_noti(NULL, NULL, NULL, data);

	return INDICATOR_ERROR_NONE;
}

static int wake_up_cb(void *data)
{
	if (!updated_while_lcd_off) return INDICATOR_ERROR_NONE;

	_on_noti(NULL, NULL, NULL, data);

	return INDICATOR_ERROR_NONE;
}

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	Evas_Object *item = data;
	char *tmp = NULL;
	char buf[256] = {0,};
	char buf1[256] = {0,};
	int status = 0;

	vconf_get_int(VCONFKEY_TELEPHONY_RSSI, &status);
	snprintf(buf1, sizeof(buf1), _("IDS_IDLE_BODY_PD_OUT_OF_4_BARS_OF_SIGNAL_STRENGTH"), status);

	snprintf(buf, sizeof(buf), "%s, %s", buf1,_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
	_D("buf: %s", buf);

	tmp = strdup(buf);
	retv_if(!tmp, NULL);

	return tmp;
}
#endif

static void _on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int sim1_service = 0;
	int status = 0;
	int ret = 0;
	int val = 0;
	struct appdata *ad = (struct appdata *)user_data;
	TelSimCardStatus_t sim_status_sim1 = 0x00;

	if (!icon_get_update_flag()) {
		updated_while_lcd_off = 1;
		_D("need to update %d", updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &status);
	if (ret == OK && status == TRUE) {
		_D("Flight mode");
		_show_image_icon(user_data, LEVEL_FLIGHT);
		return;
	}

	sim_status_sim1 = ad->tel_info.sim_status;

	if (sim_status_sim1 != TAPI_SIM_STATUS_CARD_NOT_PRESENT) {

		val = ad->tel_info.network_service_type;
		_D("Network service type : %d", val);

		switch (val) {
		case TAPI_NETWORK_SERVICE_TYPE_NO_SERVICE:
			_D("Service type : NO_SERVICE");
			_show_image_icon(user_data, LEVEL_NOSVC);
			break;
		case TAPI_NETWORK_SERVICE_TYPE_EMERGENCY:
			_D("Service type : EMERGENCY");
			val = ad->tel_info.signal_level;
			if (ret != OK) {
				_E("Can not get signal strength level");
			}
			_show_image_icon(user_data, LEVEL_RSSI_SIM1_0+val);
			break;
		case TAPI_NETWORK_SERVICE_TYPE_SEARCH:
			_D("Service type : SEARCH");
			_show_image_icon(user_data, LEVEL_SEARCH);
			break;
		default:
			_D("Service type : UNKNOWN (%d)", val);
			sim1_service = 1;
			break;
		}

		if (sim1_service) {
			val = ad->tel_info.signal_level;
			_D("get Sim1 signal strength level: %d", val);

			int roaming = 0;
			roaming = ad->tel_info.roaming_status;

			if (roaming) {
					_show_image_icon(user_data, LEVEL_RSSI_ROAMING_0+val);
			} else {
					_show_image_icon(user_data, LEVEL_RSSI_SIM1_0+val);
			}
		}
	}

	if (sim_status_sim1 == TAPI_SIM_STATUS_CARD_NOT_PRESENT) {
		val = ad->tel_info.signal_level;
		_D("No sim card : Signal-strength level : %d", val);
		if (ret != OK) {
			_E("Can not get signal strength level");
		}
		_show_image_icon(user_data, LEVEL_RSSI_SIM1_0+val);
	}
}

static void _flight_mode(keynode_t *key, void *data)
{
	_on_noti(NULL, NULL, NULL, data);
}

static void _init_tel_info(void* data)
{
	int ret = 0;
	struct appdata *ad = (struct appdata *)data;
	int i = 0;
	int val = 0;
	TelSimCardStatus_t sim_status;

	ret_if(!ad);

	/* FIXME : modem_num? */
//	for (i = 0; i < 2; i++) {
		if (!tapi_handle[i]) return;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_ROAMING_STATUS, &val);
		if (ret != OK) {
			_E("Can not get %s",TAPI_PROP_NETWORK_ROAMING_STATUS);
		}
		ad->tel_info.roaming_status = val;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL, &val);
		if (ret != OK) {
			_E("Can not get %s",TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL);
		}
		ad->tel_info.signal_level = val;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_SERVICE_TYPE, &val);
		if (ret != OK) {
			_E("Can not get %s",TAPI_PROP_NETWORK_SERVICE_TYPE);
		}
		ad->tel_info.network_service_type = val;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_PS_TYPE, &val);
		if (ret != OK) {
			_E("Can not get %s",TAPI_PROP_NETWORK_SERVICE_TYPE);
		}
		ad->tel_info.network_ps_type = val;

		TelCallPreferredVoiceSubs_t preferred_sub = TAPI_CALL_PREFERRED_VOICE_SUBS_UNKNOWN;
		ret = tel_get_call_preferred_voice_subscription(tapi_handle[i], &preferred_sub);
		if (ret != OK) {
			_E("Can not get %s",TAPI_NOTI_CALL_PREFERRED_VOICE_SUBSCRIPTION);
		}
		ad->tel_info.prefered_voice = preferred_sub;
#ifdef DEVICE_BUILD
		TelNetworkDefaultDataSubs_t preferred_data = TAPI_NETWORK_DEFAULT_DATA_SUBS_UNKNOWN;
		ret = tel_get_network_default_data_subscription(tapi_handle[i], &preferred_data);
		if (ret != OK) {
			_E("Can not get %s",TAPI_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION);
		}
		ad->tel_info.prefered_data = preferred_data;

		TelNetworkDefaultSubs_t default_subs = TAPI_NETWORK_DEFAULT_SUBS_UNKNOWN;
		ret = tel_get_network_default_subscription(tapi_handle[i], &default_subs);
		if (ret != OK) {
			_E("Can not get %s",TAPI_NOTI_NETWORK_DEFAULT_SUBSCRIPTION);
		}
		ad->tel_info.default_network = default_subs;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_SIM_CALL_FORWARD_STATE, &val);
		if (ret != OK) {
			_E("Can not get %s",TAPI_PROP_SIM_CALL_FORWARD_STATE);
		}
		ad->tel_info.call_forward = val;
#endif

		int changed = 0;
		ret = tel_get_sim_init_info(tapi_handle[i], &sim_status, &changed);
		if (ret != OK) {
			_E("Can not get sim init info");
		}
		ad->tel_info.sim_status = sim_status;
//	}
}

static void _signal_strength_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *sig_level = data;
	struct appdata *ad = (struct appdata *)user_data;

	ret_if(!ad);

	_D("Signal level = %d", *sig_level);

	ad->tel_info.signal_level= *sig_level;

	_on_noti(handle_obj, noti_id, data, user_data);
}

static void _sim_status_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *sim_status = data;
	struct appdata *ad = (struct appdata *)user_data;

	ret_if(!ad);

	_D("Sim status = %d ", *sim_status);

	ad->tel_info.sim_status= *sim_status;

	_on_noti(handle_obj, noti_id, data, user_data);
	connection_icon_on_noti(handle_obj, noti_id, data, user_data);
}

static void _network_service_type_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *service_type = data;
	struct appdata *ad = (struct appdata *)user_data;

	ret_if(!ad);

	_D ("Network service type = %d", *service_type);

	ad->tel_info.network_service_type= *service_type;

	_on_noti(handle_obj, noti_id, data, user_data);
	connection_icon_on_noti(handle_obj, noti_id, data, user_data);
}

static void _network_ps_type_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *ps_type = data;
	struct appdata *ad = (struct appdata *)user_data;

	ret_if(!ad);

	_D ("Network ps type = %d", *ps_type);

	ad->tel_info.network_ps_type= *ps_type;

	connection_icon_on_noti(handle_obj, noti_id, data, user_data);
}

static void _roaming_status_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *roaming_status = data;
	struct appdata *ad = (struct appdata *)user_data;

	ret_if(!ad);

	_D ("Roaming status = %d", *roaming_status);

	ad->tel_info.roaming_status= *roaming_status;

	_on_noti(handle_obj, noti_id, data, user_data);
}
/* FIXME : remove? */
static void _preferred_data_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	struct tel_noti_network_default_data_subs *noti = data;
	struct appdata *ad = (struct appdata *)user_data;

	ret_if(!ad);

	_D ("Preferred data = %d", noti->default_subs);

	//Data preferred calback comes from only the sim handle which changed
	//SIM1->SIM2 , callback comes only for SIM2 handle
	ad->tel_info.prefered_data = noti->default_subs;
	//data prefered is not controllered

	connection_icon_on_noti(handle_obj, noti_id, data, user_data);
}

static void _default_network_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	struct tel_noti_network_default_subs *noti = data;
	struct appdata *ad = (struct appdata *)user_data;

	ret_if(!ad);

	_D ("Default network = %d", noti->default_subs);

	ad->tel_info.default_network = noti->default_subs;

	_on_noti(handle_obj, noti_id, data, user_data);
}

static void _call_forward_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *call_forward = data;
	struct appdata *ad = (struct appdata *)user_data;

	ret_if(!ad);

	_D ("Call forward = %d", *call_forward);

	ad->tel_info.call_forward = *call_forward;

	call_forward_on_noti(handle_obj, noti_id,data, user_data);
}

/* Initialize TAPI */
static void _init_tel(void *data)
{
	char **cp_list = NULL;
	unsigned int modem_num = 0;

	_D("Initialize TAPI");

	if (registered) {
		_E("TAPI Already registered");
		return;
	}

	cp_list = tel_get_cp_name_list();
	ret_if(!cp_list);

	while (cp_list[modem_num]) {
		tapi_handle[modem_num] = tel_init(cp_list[modem_num]);
		if (!tapi_handle[modem_num]) {
			_E("tapi_handle[%d] is NULL", modem_num);
		} else {
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL, _signal_strength_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_NOTI_SIM_STATUS, _sim_status_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_NETWORK_SERVICE_TYPE, _network_service_type_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_NETWORK_PS_TYPE, _network_ps_type_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_NETWORK_ROAMING_STATUS, _roaming_status_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION, _preferred_data_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_NOTI_NETWORK_DEFAULT_SUBSCRIPTION, _default_network_cb, data);
#ifdef DEVICE_BUILD
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_SIM_CALL_FORWARD_STATE , _call_forward_cb, data);
#endif
		}
		modem_num++;
	}
	_D("modem num: %d", modem_num);

	tapi_handle[modem_num] = NULL;
	g_strfreev(cp_list);
	_init_tel_info(data);
	_on_noti(NULL, NULL, NULL, data);

	registered = 1;
}

/* De-initialize TAPI */
static void _deinit_tel()
{
	_D("De-initialize TAPI");
	unsigned int i = 0;

	while (tapi_handle[i]) {
		/* De-initialize TAPI handle */
		if(tapi_handle[i])
		{
			tel_deregister_noti_event(tapi_handle[i], TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL);
			tel_deregister_noti_event(tapi_handle[i], TAPI_NOTI_SIM_STATUS);
			tel_deregister_noti_event(tapi_handle[i], TAPI_PROP_NETWORK_SERVICE_TYPE);
			tel_deregister_noti_event(tapi_handle[i], TAPI_PROP_NETWORK_ROAMING_STATUS);
			tel_deregister_noti_event(tapi_handle[i], TAPI_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION);
			tel_deregister_noti_event(tapi_handle[i], TAPI_NOTI_NETWORK_DEFAULT_SUBSCRIPTION);
#ifdef DEVICE_BUILD
			tel_deregister_noti_event(tapi_handle[i], TAPI_PROP_SIM_CALL_FORWARD_STATE);
#endif
		}
		tel_deinit(tapi_handle[i]);
		tapi_handle[i] = NULL;

		/* Move to next handle */
		i++;
	}
	registered = 0;
}

static void _tel_ready_cb(keynode_t *key, void *data)
{
	gboolean status = FALSE;

	status = vconf_keynode_get_bool(key);
	if (status == TRUE) {    /* Telephony State - READY */
		_init_tel(data);
	}
	else { /* Telephony State – NOT READY */
		/* De-initialization is optional here (ONLY if required) */
		_deinit_tel();
	}
}

static int register_rssi_module(void *data)
{
	gboolean state = FALSE;
	int ret;

	retv_if(!data, 0);

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode, data);
	if (ret != OK) _E("Failed to register callback for VCONFKEY_TELEPHONY_FLIGHT_MODE");

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_READY, &state);
	if (ret != OK) {
		_E("Failed to get value for VCONFKEY_TELEPHONY_READY");
		return ret;
	}

	if(state) {
		_D("Telephony ready");
		_init_tel(data);
	} else {
		_D("Telephony not ready");
		vconf_notify_key_changed(VCONFKEY_TELEPHONY_READY, _tel_ready_cb, data);
	}

	return ret;
}

static int unregister_rssi_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_READY, _tel_ready_cb);
	if (ret != OK) _E("Failed to unregister callback for VCONFKEY_TELEPHONY_READY");

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode);
	if (ret != OK) _E("Failed to unregister callback for VCONFKEY_TELEPHONY_FLIGHT_MODE");

	_deinit_tel();

	return ret;
}
/* End of file */
