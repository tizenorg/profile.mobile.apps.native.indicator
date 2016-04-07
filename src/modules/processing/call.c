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
#include <bluetooth.h>
#include <telephony.h>

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "util.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_MINICTRL3
#define MODULE_NAME		"call"
#define MINICONTROL_VOICE_NAME	"[voicecall-quickpanel]"
#define MINICONTROL_VIDEO_NAME	"[videocall-quickpanel]"

static int register_call_module(void *data);
static int unregister_call_module(void);
static void indicator_call_change_cb(telephony_h handle, telephony_noti_e noti_id, void *data, void *user_data);
static int check_calls_status(int handle_no, void *data);

enum {
	CALL_UI_STATUS_NONE = 0,
	CALL_UI_STATUS_INCOM,
	CALL_UI_STATUS_OUTGOING,
	CALL_UI_STATUS_ACTIVE,
	CALL_UI_STATUS_HOLD,
	CALL_UI_STATUS_END,
};

icon_s call = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_MINICTRL,
	.init = register_call_module,
	.fini = unregister_call_module,
	.minictrl_control = NULL /* mctrl_monitor_cb */
};

static int bt_state = 0;
static telephony_noti_e call_state;
static telephony_handle_list_s list;

static const char *icon_path[] = {
	"Call/B03_Call_Duringcall.png",
	"Call/B03_Call_bluetooth.png",
	NULL
};

static void set_app_state(void *data)
{
	call.ad = data;
}

static void show_image_icon(void *data)
{
	if (bt_state == 1) {
		call.img_obj.data = icon_path[1];
	} else {
		call.img_obj.data = icon_path[0];
	}
	icon_show(&call);
}

static void hide_image_icon(void)
{
	icon_hide(&call);
}

static void icon_animation_set(enum indicator_icon_ani type)
{
	icon_ani_set(&call, type);
}

static void __bt_ag_sco_state_changed_cb(int result, bool connected,
		const char *remote_address, bt_audio_profile_type_e type, void *user_data)
{
	if (connected)
		bt_state = 1;
	else
		bt_state = 0;

	for (int j = 0; j < list.count; j++)
		check_calls_status(j, user_data);
}

static void register_bt_state(void *data)
{
	int error = -1;

	error = bt_initialize();
	if (error != BT_ERROR_NONE) _E("bt_initialize return [%d]", error);

	error = bt_audio_initialize();
	if (error != BT_ERROR_NONE) _E("bt_audio_initialize return [%d]", error);

	error = bt_audio_set_connection_state_changed_cb(__bt_ag_sco_state_changed_cb, data);
	if (error != BT_ERROR_NONE) _E("bt_ag_set_sco_state_changed_cb return [%d]", error);
}

static void unregister_bt_state(void)
{
	int error = -1;

	error = bt_audio_unset_connection_state_changed_cb();
	if (error != BT_ERROR_NONE) _E("bt_ag_unset_sco_state_changed_cb return [%d]", error);

	error = bt_audio_deinitialize();
	if (error != BT_ERROR_NONE) _E("bt_audio_deinitialize return [%d]", error);

	error = bt_deinitialize();
	if (error != BT_ERROR_NONE) _E("bt_audio_deinitialize return [%d]", error);
}

static telephony_noti_e convert_call_status(telephony_call_status_e call_status)
{
	switch (call_status) {
	case TELEPHONY_CALL_STATUS_IDLE:
		return TELEPHONY_NOTI_VOICE_CALL_STATUS_IDLE;
	case TELEPHONY_CALL_STATUS_ACTIVE:
		return TELEPHONY_NOTI_VOICE_CALL_STATUS_ACTIVE;
	case TELEPHONY_CALL_STATUS_HELD:
		return TELEPHONY_NOTI_VOICE_CALL_STATUS_HELD;
	case TELEPHONY_CALL_STATUS_DIALING:
		return TELEPHONY_NOTI_VOICE_CALL_STATUS_DIALING;
	case TELEPHONY_CALL_STATUS_ALERTING:
		return TELEPHONY_NOTI_VOICE_CALL_STATUS_ALERTING;
	case TELEPHONY_CALL_STATUS_INCOMING:
		return TELEPHONY_NOTI_VOICE_CALL_STATUS_INCOMING;
	}
}

static void indicator_call_change_cb(telephony_h handle, telephony_noti_e noti_id, void *data, void *user_data)
{
	int status = 0;

	ret_if(!user_data);

	call_state = noti_id;

	switch (noti_id) {
	case TELEPHONY_NOTI_VOICE_CALL_STATUS_DIALING:
	case TELEPHONY_NOTI_VIDEO_CALL_STATUS_DIALING:
	case TELEPHONY_NOTI_VOICE_CALL_STATUS_INCOMING:
	case TELEPHONY_NOTI_VIDEO_CALL_STATUS_INCOMING:
	case TELEPHONY_NOTI_VOICE_CALL_STATUS_ALERTING:
	case TELEPHONY_NOTI_VIDEO_CALL_STATUS_ALERTING:
		show_image_icon(user_data);
		icon_animation_set(ICON_ANI_BLINK);
		break;
	case TELEPHONY_NOTI_VOICE_CALL_STATUS_ACTIVE:
	case TELEPHONY_NOTI_VIDEO_CALL_STATUS_ACTIVE:
	case TELEPHONY_NOTI_VOICE_CALL_STATUS_HELD:
		show_image_icon(user_data);
		icon_animation_set(ICON_ANI_NONE);
		break;
	case TELEPHONY_NOTI_VOICE_CALL_STATUS_IDLE:
	case TELEPHONY_NOTI_VIDEO_CALL_STATUS_IDLE:
		hide_image_icon();
		break;
	default:
		_E("Invalid value %d", status);
		hide_image_icon();
		break;
	}
}

static int check_calls_status(int handle_no, void *data)
{
	int ret = OK;
	int ret_val = OK;

	telephony_call_h *call_list;
	unsigned int call_cnt = 0;

	if ((handle_no < 0) || (handle_no >= list.count)) {
		_E("Invalid handle number: %d", handle_no);
		return FAIL;
	}

	ret = telephony_call_get_call_list(list.handle[handle_no], &call_cnt, &call_list);
	if (ret != TELEPHONY_ERROR_NONE) {
		_E("telephony_call_get_call_list failed : %s", get_error_message(ret));
		return FAIL;
	}

	if (call_cnt == 0) {
		hide_image_icon();
		_D("No calls available");
		ret_val = FAIL;
	} else {
		for (int i = 0; i < call_cnt; i++) {
			telephony_call_status_e status;

			ret = telephony_call_get_status(call_list[i], &status);

			if (ret != TELEPHONY_ERROR_NONE) {
				_E("telephony_call_get_status failed : %s", get_error_message(ret));

				ret_val = FAIL;
				continue;
			}
			indicator_call_change_cb(NULL, convert_call_status(status), NULL, data);
		}
	}

	telephony_call_release_call_list(call_cnt, &call_list);

	return ret_val;
}

static int register_call_module(void *data)
{
	int ret = OK;
	int ret_val = OK;
	retv_if(!data, FAIL);

	set_app_state(data);

	ret = telephony_init(&list);
	retvm_if(ret != TELEPHONY_ERROR_NONE, FAIL, "telephony_init failed : %s", get_error_message(ret));

	for (int j = 0; j < list.count; j++) {
		for (int i = TELEPHONY_NOTI_VOICE_CALL_STATUS_IDLE; i <= TELEPHONY_NOTI_VIDEO_CALL_STATUS_INCOMING; i++) {
			ret = telephony_set_noti_cb(list.handle[j], i, indicator_call_change_cb, data);
			if (ret != TELEPHONY_ERROR_NONE) {
				_E("telephony_set_noti_cb failed : %s", get_error_message(ret));
				_E("i: %d", i);
				ret_val = FAIL;
			}
		}
		ret = check_calls_status(j, data);
		ret_val = (ret != OK) ? FAIL : ret_val;
	}

	register_bt_state(data);

	return ret_val;
}

static int unregister_call_module(void)
{
	int ret = OK;
	int ret_val = OK;

	for (int j = 0; j < list.count; j++) {
		for (int i = TELEPHONY_NOTI_VOICE_CALL_STATUS_IDLE; i <= TELEPHONY_NOTI_VIDEO_CALL_STATUS_INCOMING; i++) {
			ret = telephony_unset_noti_cb(list.handle[j], i);
			if (ret != TELEPHONY_ERROR_NONE) {
				_E("telephony_unset_noti_cb failed : %s", get_error_message(ret));
				_E("i: %d", i);
				ret_val = FAIL;
			}
		}
	}

	ret = telephony_deinit(&list);
	if (ret != TELEPHONY_ERROR_NONE) {
		_E("telephony_deinit failed : %s", get_error_message(ret));
		ret_val = FAIL;
	}

	unregister_bt_state();

	return ret_val;
}
/* End of file */
