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
#include <stdbool.h>
#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_MINICTRL1
#define MODULE_NAME		"VOICE_RECORDER"
#define MINICONTROL_NAME	"[voicerecorder_mini_controller]"

#define INDICATOR_REMOTE_PORT "voicerecorder_indicator_port"
#define INDICATOR_APP_ID  "org.tizen.indicator"
#define INDICATOR_BUNDLE_KEY "voicerecorder_indicator_data"
#define BG_NONE     "voicerecorder_none"
#define BG_RECORING_START  "bg_recording_start"
#define BG_RECORING_PAUSE  "bg_recording_pause"
#define BG_PLAYING_START  "bg_playing_start"
#define BG_PLAYING_PAUSE  "bg_playing_pause"

static int register_voice_recorder_module(void *data);
static int unregister_voice_recorder_module(void);
static int wake_up_cb(void *data);

static int updated_while_lcd_off = 0;
static int prevIndex = -1;
static int vr_state = -1;

icon_s voice_recorder = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_MINICTRL,
	.init = register_voice_recorder_module,
	.fini = unregister_voice_recorder_module,
	.minictrl_control = NULL,//mctrl_monitor_cb,
	.wake_up = wake_up_cb
};

enum {
	VOICE_RECORDER_RECORDING,
	VOICE_RECORDER_RECORDING_PAUSED,
	VOICE_RECORDER_PLAYING,
	VOICE_RECORDER_PLAYING_PAUSED
};

static char *icon_path[] = {
	"Background playing/B03_Backgroundplaying_Voicerecorder.png",
	"Background playing/B03_Backgroundplaying_Voicerecorder.png",
	"Background playing/B03_Backgroundplaying_voicerecorder_player_play.png",
	"Background playing/B03_Backgroundplaying_voicerecorder_player_pause.png",
	NULL
};



static void set_app_state(void* data)
{
	voice_recorder.ad = data;
}



static void show_image_icon(void *data, int status)
{
	if(prevIndex == status)
	{
		return;
	}

	voice_recorder.img_obj.data = icon_path[status];
	icon_show(&voice_recorder);

	prevIndex = status;
}



static void hide_image_icon(void)
{
	icon_hide(&voice_recorder);

	prevIndex = -1;
}



static void show_voicerecoder_icon(void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if(icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	_D("VOICE RECORDER state: %d", vr_state);
	switch (vr_state) {
	case VOICE_RECORDER_RECORDING:
		show_image_icon(data, VOICE_RECORDER_RECORDING);
		break;
	case VOICE_RECORDER_RECORDING_PAUSED:
		show_image_icon(data, VOICE_RECORDER_RECORDING_PAUSED);
		break;
	case VOICE_RECORDER_PLAYING:
		show_image_icon(data, VOICE_RECORDER_PLAYING);
		break;
	case VOICE_RECORDER_PLAYING_PAUSED:
		show_image_icon(data, VOICE_RECORDER_PLAYING_PAUSED);
		break;
	default:
		hide_image_icon();
		break;
	}
	return;
}


#if 0
static void indicator_voice_recorder_change_cb(keynode_t *node, void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	show_voicerecoder_icon(data);
	return;
}
#endif


static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		return OK;
	}

	show_voicerecoder_icon(voice_recorder.ad);

	return OK;
}



void hide_voice_recorder_icon(void)
{
	hide_image_icon();;
}



static int register_voice_recorder_module(void *data)
{

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);
	show_voicerecoder_icon(data);
	return OK;
}



static int unregister_voice_recorder_module(void)
{
	return OK;
}
