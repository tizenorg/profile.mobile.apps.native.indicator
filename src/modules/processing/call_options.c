/*
 *  Indicator
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved.
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

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "util.h"
#include "log.h"

#define MODULE_NAME_MUTE "call_options_mute"
#define ICON_PRIORITY_MUTE	INDICATOR_PRIORITY_MINICTRL2
#define ICON_PATH_MUTE "Call/B03_Call_Mute.png"

#define MODULE_NAME_SPEAKER "call_options_speaker"
#define ICON_PRIORITY_SPEAKER	INDICATOR_PRIORITY_MINICTRL3
#define ICON_PATH_SPEAKER "Call/B03_Call_Speaker_on.png"

icon_s call_options_mute = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME_MUTE,
	.priority = ICON_PRIORITY_MUTE,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_MINICTRL,
	.init = register_call_options_mute_module,
	.fini = unregister_call_options_mute_module,
	.minictrl_control = NULL, /* mctrl_monitor_cb */
	.img_obj.data = ICON_PATH_MUTE

};

icon_s call_options_speaker = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME_SPEAKER,
	.priority = ICON_PRIORITY_SPEAKER,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_MINICTRL,
	.init = register_call_options_speaker_module,
	.fini = unregister_call_options_speaker_module,
	.minictrl_control = NULL, /* mctrl_monitor_cb */
	.img_obj.data = ICON_PATH_SPEAKER

};

static cm_client_h cm_handle;
static int init_cnt = 0;

static void mute_status_cb (cm_mute_status_e mute_status, void *user_data)
{
	if(mute_status == CM_MUTE_STATUS_ON) {
		icon_show(&call_options_mute);
	}
	else if (mute_status == CM_MUTE_STATUS_ON) {
		icon_hide(&call_options_mute);
	}
}

static void audio_status_cb(cm_audio_state_type_e audio_state, void *user_data)
{
	if (audio_state == CM_AUDIO_STATE_SPEAKER_E)
		icon_show(&call_options_speaker);
	else
		icon_hide(&call_options_speaker);
}

static int call_manager_init(void)
{
	if (init_cnt == 0) {
		int ret = cm_init(&cm_handle);
		retmv_if(ret != CM_ERROR_NONE, FAIL, "cm_init failed[%d]: %s", ret, get_error_message(ret));
	}

	init_cnt++;

	return OK;
}

static int call_manager_deinit(void)
{
	if (init_cnt == 1) {
		int ret = cm_deinit(cm_handle);
		retmv_if(ret != CM_ERROR_NONE, FAIL, "cm_deinit failed[%d]: %s", ret, get_error_message(ret));
	}

	init_cnt--;

	return OK;
}

static int register_call_options_mute_module(void *data)
{
	int ret;

	call_manager_init();

	ret = cm_set_mute_status_cb(cm_handle, mute_status_cb, data);
	retvm_if(ret != CM_ERROR_NONE, FAIL, "cm_mute_status_cb failed[%d]: %s", ret, get_error_message(ret));

	return OK;
}

static int register_call_options_speaker_module(void *data)
{
	int ret;

	call_manager_init();

	ret = cm_set_audio_state_changed_cb(cm_handle, audio_status_cb, data);
	retvm_if(ret != CM_ERROR_NONE, FAIL, "cm_set_audio_state_changed_cb[%d]: %s", ret, get_error_message(ret));

	return OK;
}

static int unregister_call_options_mute_module(void *data)
{
	cm_unset_mute_status_cb(cm_handle);
	call_manager_deinit();

	return OK;
}

static int unregister_call_options_speaker_module(void *data)
{
	cm_unset_audio_state_changed_cb(cm_handle);
	call_manager_deinit();
	return OK;
}
