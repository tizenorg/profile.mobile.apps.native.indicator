/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>
#include <minicontrol-monitor.h>
#include "common.h"
#include "indicator.h"
#include "indicator_icon_util.h"
#include "modules.h"
#include "indicator_ui.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_1
#define MODULE_NAME		"VOICE_RECORDER"
#define MINICONTROL_NAME	"[voicerecorder_mini_controller]"


static int register_voice_recorder_module(void *data);
static int unregister_voice_recorder_module(void);
static void mctrl_monitor_cb(int action, const char *name, void *data);

Indicator_Icon_Object voice_recorder[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_voice_recorder_module,
	.fini = unregister_voice_recorder_module,
	.minictrl_control = mctrl_monitor_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_voice_recorder_module,
	.fini = unregister_voice_recorder_module,
	.minictrl_control = mctrl_monitor_cb
}

};

enum {
	VOICE_RECORDER_RECORDING,
	VOICE_RECORDER_PAUSED,
	VOICE_RECORDER_READY
};

static char *icon_path[] = {
	"Background playing/B03_Backgroundplaying_voicerecorder_Recording.png",
	"Background playing/B03_Backgroundplaying_voicerecorder_paused.png",
	"Background playing/B03_Backgroundplaying_Voicerecording.png",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		voice_recorder[i].ad = data;
	}
}

static void show_image_icon(void *data, int status)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		voice_recorder[i].img_obj.data = icon_path[status];
		indicator_util_icon_show(&voice_recorder[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&voice_recorder[i]);
	}
}

static void show_voicerecoder_icon(void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_VOICERECORDER_STATE, &status);
	if (ret == OK) {
		INFO("VOICE RECORDER state: %d", status);
		switch (status) {
		case VCONFKEY_VOICERECORDER_RECORDING:
			show_image_icon(data, VOICE_RECORDER_RECORDING);
			break;
		case VCONFKEY_VOICERECORDER_PAUSED:
			show_image_icon(data, VOICE_RECORDER_PAUSED);
			break;
		case VCONFKEY_VOICERECORDER_READY:
			show_image_icon(data, VOICE_RECORDER_READY);
			break;
		default:
			break;
		}
	}
	return;
}


static void indicator_voice_recorder_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	show_voicerecoder_icon(data);
	return;
}

static void mctrl_monitor_cb(int action, const char *name, void *data)
{
	retif(!data, , "data is NULL");
	retif(!name, , "name is NULL");

	if(strncmp(name,MINICONTROL_NAME,strlen(MINICONTROL_NAME))!=0)
	{
		ERR("_mctrl_monitor_cb: no VR %s",name);
		return;
	}

	DBG("_mctrl_monitor_cb:%s %d",name,action);

	switch (action) {
	case MINICONTROL_ACTION_START:
		vconf_notify_key_changed(VCONFKEY_VOICERECORDER_STATE, indicator_voice_recorder_change_cb, data);
		show_voicerecoder_icon(data);
		break;
	case MINICONTROL_ACTION_STOP:
		hide_image_icon();
		vconf_ignore_key_changed(VCONFKEY_VOICERECORDER_STATE, indicator_voice_recorder_change_cb);
		break;
	default:
		break;
	}
}

static int register_voice_recorder_module(void *data)
{

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	return OK;
}

static int unregister_voice_recorder_module(void)
{

	return OK;
}
