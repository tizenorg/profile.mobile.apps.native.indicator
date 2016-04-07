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
#include <system_settings.h>
#include <telephony.h>
#include <vconf.h>

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "log.h"
#include "util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_2
#define MODULE_NAME		"call_divert"

static int register_call_divert_module(void *data);
static int unregister_call_divert_module(void);
static void _on_noti(void *user_data);
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

static telephony_handle_list_s list;
static const char *icon_path = "Call divert/B03_Call_divert_default.png";


static void set_app_state(void *data)
{
	call_divert.ad = data;
}

static void _show_image_icon()
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
	int ret;

	ret = vconf_get_int(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE, &status);
	retv_if(ret != 0, NULL);

	switch (status) {
	case VCONFKEY_TELEPHONY_CALL_FORWARD_ON:
		snprintf(buf, sizeof(buf), "%s, %s, %s", _("IDS_CST_BODY_CALL_FORWARDING"),
				_("IDS_IDLE_BODY_ICON"), _("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
		break;
	default:
		break;
	}

	return strdup(buf);
}
#endif


static void _on_noti(void *user_data)
{
	int ret = 0;
	telephony_state_e state;
	int call_divert_state = VCONFKEY_TELEPHONY_CALL_FORWARD_ON;

	ret_if(!user_data);

	bool flight_mode;
	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, &flight_mode);
	ret_if(ret != SYSTEM_SETTINGS_ERROR_NONE);

	if (flight_mode == true) {
		_D("Flight Mode");
		_hide_image_icon();
		return;
	}

	ret = telephony_get_state(&state);
	ret_if(ret != TELEPHONY_ERROR_NONE);

	if (state == TELEPHONY_STATE_NOT_READY) {
		_D("Telephony not ready");
		_hide_image_icon();
		return;
	}

	ret = vconf_get_int(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE, &call_divert_state);
	ret_if(ret != 0);

	if (call_divert_state == VCONFKEY_TELEPHONY_CALL_FORWARD_ON) {
		_D("Show call divert icon");
		_show_image_icon();
	} else {
		_D("Hide call divert icon");
		_hide_image_icon();
	}

	return;
}


static void _tel_ready_cb(telephony_state_e state, void *user_data)
{
	_on_noti(user_data);
}


static void _flight_mode(system_settings_key_e key, void *user_data)
{
	_on_noti(user_data);
}


static void _vconf_indicator_call_forward(keynode_t *node, void *user_data)
{
	_on_noti(user_data);
}


static void free_resources()
{
	util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode);
	telephony_deinit(&list);
	telephony_unset_state_changed_cb(_tel_ready_cb);
}


static int register_call_divert_module(void *data)
{
	int ret;
	telephony_state_e state;

	retv_if(!data, 0);

	set_app_state(data);

	ret = util_system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode, data);
	retvm_if(ret != SYSTEM_SETTINGS_ERROR_NONE, FAIL,
			"util_system_settings_set_changed_cb failed[%s]", get_error_message(ret));

	ret = telephony_init(&list);
	if (ret != TELEPHONY_ERROR_NONE) {
		_E("telephony_init failed[%s]", get_error_message(ret));
		util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode);
		return FAIL;
	}

	ret = telephony_set_state_changed_cb(_tel_ready_cb, data);
	if (ret != TELEPHONY_ERROR_NONE) {
		_E("telephony_set_state_changed_cb failed[%s]", get_error_message(ret));
		free_resources();
		return FAIL;
	}

	ret = telephony_get_state(&state);
	if (ret != TELEPHONY_ERROR_NONE) {
		_E("telephony_get_state failed[%s]", get_error_message(ret));
		free_resources();
		return FAIL;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE, _vconf_indicator_call_forward, data);
	if (ret != OK) {
		_E("vconf_notify_key_changed failed[%s]", get_error_message(ret));
		free_resources();
		return FAIL;
	}

	_tel_ready_cb(state, data);

	return ret;
}


static int unregister_call_divert_module(void)
{
	int ret = 0;

	util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, _flight_mode);

	ret = ret | telephony_unset_state_changed_cb(_tel_ready_cb);

	ret = ret | telephony_deinit(&list);

	ret = ret | vconf_ignore_key_changed(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE, _vconf_indicator_call_forward);

	return ret;
}
/* End of file */
