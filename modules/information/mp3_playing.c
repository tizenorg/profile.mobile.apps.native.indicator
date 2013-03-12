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
#define MODULE_NAME		"MP3_PLAY"
#define MINICONTROL_NAME	"[musicplayer-mini]"

static int register_mp3_play_module(void *data);
static int unregister_mp3_play_module(void);
static void mctrl_monitor_cb(int action, const char *name, void *data);

Indicator_Icon_Object mp3_play[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_mp3_play_module,
	.fini = unregister_mp3_play_module,
	.minictrl_control = mctrl_monitor_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_mp3_play_module,
	.fini = unregister_mp3_play_module,
	.minictrl_control = mctrl_monitor_cb
}
};

enum {
	MUSIC_PLAY,
	MUSIC_PAUSED,
};

static char *icon_path[] = {
	"Background playing/B03_Backgroundplaying_MP3playing.png",
	"Background playing/B03_Backgroundplaying_Music_paused.png",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		mp3_play[i].ad = data;
	}
}

static void show_image_icon(void *data, int status)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		mp3_play[i].img_obj.data = icon_path[status];
		indicator_util_icon_show(&mp3_play[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&mp3_play[i]);
	}
}

static void show_mp_icon(void* data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_MUSIC_STATE, &status);
	if (ret == OK) {
		INFO("MUSIC state: %d", status);
		switch (status) {
		case VCONFKEY_MUSIC_PLAY:
			show_image_icon(data, MUSIC_PLAY);
			break;
		case VCONFKEY_MUSIC_PAUSE:
			show_image_icon(data, MUSIC_PAUSED);
			break;
		default:
			break;
		}
	}
}


static void indicator_mp3_play_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	DBG("indicator_mp3_play_change_cb");

	show_mp_icon(data);

	return;
}

static void mctrl_monitor_cb(int action, const char *name, void *data)
{
	retif(!data, , "data is NULL");
	retif(!name, , "name is NULL");

	if(strncmp(name,MINICONTROL_NAME,strlen(MINICONTROL_NAME))!=0)
	{
		ERR("_mctrl_monitor_cb:no mp %s",name);
		return;
	}

	DBG("_mctrl_monitor_cb:%s %d",name,action);

	switch (action) {
	case MINICONTROL_ACTION_START:
		vconf_notify_key_changed(VCONFKEY_MUSIC_STATE, indicator_mp3_play_change_cb, data);
		show_mp_icon(data);
		break;
	case MINICONTROL_ACTION_STOP:
		hide_image_icon();
		vconf_ignore_key_changed(VCONFKEY_MUSIC_STATE, indicator_mp3_play_change_cb);
		break;
	default:
		break;
	}
}

static int register_mp3_play_module(void *data)
{

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	return OK;
}

static int unregister_mp3_play_module(void)
{
	return OK;
}
