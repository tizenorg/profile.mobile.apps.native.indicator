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
//#include <Ecore_X.h>
#include <bluetooth.h>
//#include <bluetooth_extention.h>

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
static void show_call_icon( void *data);

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
	.minictrl_control = NULL //mctrl_monitor_cb
};

static int bt_state = 0;

static const char *icon_path[] = {
	"Call/B03_Call_Duringcall.png",
	"Call/B03_Call_bluetooth.png",
	NULL
};

static void set_app_state(void* data)
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

static void __bt_ag_sco_state_changed_cb(int result, bool connected, const char *remote_address, bt_audio_profile_type_e type, void *user_data)
{
	int status = 0;

	if (connected) bt_state=1;
	else bt_state=0;

	vconf_get_int(VCONFKEY_CALL_STATE, &status);

	if (status != VCONFKEY_CALL_OFF) show_call_icon(user_data);
}

static void register_bt_state( void *data)
{
	int error = -1;

	error = bt_initialize();
	if (error != BT_ERROR_NONE) _E("bt_initialize return [%d]", error);

	error = bt_audio_initialize();
	if (error != BT_ERROR_NONE) _E("bt_audio_initialize return [%d]", error);

	error = bt_audio_set_connection_state_changed_cb(__bt_ag_sco_state_changed_cb, data);   // callback µî·Ï
	if (error != BT_ERROR_NONE) _E("bt_ag_set_sco_state_changed_cb return [%d]", error);

}

static void unregister_bt_state( void )
{
	int error = -1;

	error = bt_audio_unset_connection_state_changed_cb();
	if (error != BT_ERROR_NONE) _E("bt_ag_unset_sco_state_changed_cb return [%d]", error);

	error = bt_audio_deinitialize();
	if (error != BT_ERROR_NONE) _E("bt_audio_deinitialize return [%d]", error);

	error = bt_deinitialize();
	if (error != BT_ERROR_NONE) _E("bt_audio_deinitialize return [%d]", error);
}

static void show_call_icon( void *data)
{
	int status = 0;
	int ret = 0;

	ret_if(!data);

	ret = vconf_get_int(VCONFKEY_CALL_STATE, &status);
	if (ret != OK) {
		_E("Failed to get VCONFKEY_CALL_STATE!");
		return;
	}

	switch (status) {
	case VCONFKEY_CALL_VOICE_CONNECTING:
	case VCONFKEY_CALL_VIDEO_CONNECTING:
		show_image_icon(data);
		icon_animation_set(ICON_ANI_BLINK);
		break;
	case VCONFKEY_CALL_VOICE_ACTIVE:
	case VCONFKEY_CALL_VIDEO_ACTIVE:
		show_image_icon(data);
		icon_animation_set(ICON_ANI_NONE);
		break;
	case VCONFKEY_CALL_OFF:
		hide_image_icon();
		break;
	default:
		_E("Invalid value %d", status);
		break;
	}
}

static void indicator_call_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = 0;

	ret_if(!data);

	ret = vconf_get_int(VCONFKEY_CALL_STATE, &status);
	if (ret != OK) {
		_E("Failed to get VCONFKEY_CALL_STATE!");
		return;
	}

	switch (status) {
	case VCONFKEY_CALL_VOICE_CONNECTING:
	case VCONFKEY_CALL_VIDEO_CONNECTING:
		show_image_icon(data);
		icon_animation_set(ICON_ANI_BLINK);
		break;
	case VCONFKEY_CALL_VOICE_ACTIVE:
	case VCONFKEY_CALL_VIDEO_ACTIVE:
		show_image_icon(data);
		icon_animation_set(ICON_ANI_NONE);
		break;
	case VCONFKEY_CALL_OFF:
		hide_image_icon();
		break;
	default:
		_E("Invalid value %d", status);
		break;
	}
}

static int register_call_module(void *data)
{
	int ret = 0;

	retv_if(!data, 0);

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_CALL_STATE, indicator_call_change_cb, data);
	register_bt_state(data);
	if (ret != OK) _E("Failed to register VCONFKEY_CALL_STATE callback!");

	return ret;
}

static int unregister_call_module(void)
{
	int ret = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_CALL_STATE, indicator_call_change_cb);
	unregister_bt_state();
	if (ret != OK) _E("Failed to register VCONFKEY_CALL_STATE callback!");

	return ret;
}
/* End of file */
