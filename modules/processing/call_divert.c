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

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_2
#define MODULE_NAME		"call_divert"

#define TAPI_HANDLE_MAX  2

static int register_call_divert_module(void *data);
static int unregister_call_divert_module(void);
static void _on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif

icon_s call_divert = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_call_divert_module,
	.fini = unregister_call_divert_module,
	.area = INDICATOR_ICON_AREA_SYSTEM,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

static const char *icon_path = "call_devert/B03_Call_divert_default.png";

static void set_app_state(void* data)
{
	call_divert.ad = data;
}

static void _show_image_icon(void *data)
{
	call_divert.img_obj.data = icon_path;
	icon_show(&call_divert);
}

static void _hide_image_icon(void)
{
	icon_hide(&call_divert);
}

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	char *tmp = NULL;
	char buf[256] = {0,};
	int status = 0;

	switch (status) {
	case VCONFKEY_TELEPHONY_CALL_FORWARD_ON:
		snprintf(buf, sizeof(buf), "%s, %s, %s",_("IDS_CST_BODY_CALL_FORWARDING"),_("IDS_IDLE_BODY_ICON"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
		break;
	default:
		break;
	}

	tmp = strdup(buf);
	if (!tmp) return NULL;

	return tmp;
}
#endif

static void _on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int status = 0;
	int ret = 0;
	int call_divert_state = 0;

	ret_if(!user_data);

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &status);
	if (ret == OK && status == TRUE) {
		_D("Flight Mode");
		_hide_image_icon();
		return;
	}

	/*
	 *\note
	 * Call forward function like call divert.
	 */
	/* FIXME */
	if (call_divert_state == VCONFKEY_TELEPHONY_CALL_FORWARD_ON) {
		_D("Show call divert icon");
		_show_image_icon(data);
	} else { /* VCONFKEY_TELEPHONY_CALL_FORWARD_OFF */
		_D("Hide call divert icon");
		_hide_image_icon();
	}

	return;
}

void call_forward_on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	_D("");
	_on_noti(NULL, NULL, NULL, user_data);

}

/* Initialize TAPI */
static void _init_tel(void *data)
{
	_on_noti(NULL, NULL, NULL, data);
}

/* De-initialize TAPI */
static void _deinit_tel()
{
	_D("");
}

static void _tel_ready_cb(keynode_t *key, void *data)
{
	gboolean status = FALSE;

	status = vconf_keynode_get_bool(key);
	if (status == TRUE) { /* Telephony State - READY */
		_init_tel(data);
	} else { /* Telephony State – NOT READY */
		/*
		 *\note
		 * De-initialization is optional here. (ONLY if required)
		 */
		_deinit_tel();
	}
}

static void _flight_mode(keynode_t *key, void *data)
{
	_on_noti(NULL, NULL, NULL, data);
}

static void _sim_icon_update(keynode_t *key, void *data)
{
	_on_noti(NULL, NULL, NULL, data);
}

static int register_call_divert_module(void *data)
{
	gboolean state = FALSE;
	int ret;

	retv_if(!data, 0);

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode, data);
	if (ret != OK) {
		_E("Failed to register callback for VCONFKEY_TELEPHONY_FLIGHT_MODE");
	}

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_READY, &state);
	if (ret != OK) {
		_E("Failed to get value for VCONFKEY_TELEPHONY_READY");
		return ret;
	}

	if (state) {
		_D("Telephony ready");
		_init_tel(data);
	} else {
		_D("Telephony not ready");
		vconf_notify_key_changed(VCONFKEY_TELEPHONY_READY, _tel_ready_cb, data);
	}

	return ret;
}

static int unregister_call_divert_module(void)
{
	int ret = 0;

	_deinit_tel();

	vconf_ignore_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode);

	return ret;
}
/* End of file */
