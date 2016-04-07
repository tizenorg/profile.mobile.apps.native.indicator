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

#include "common.h"
#include "modules.h"
#include "indicator.h"
#include "main.h"
#include "util.h"
#include "icon.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_MINICTRL2
#define MODULE_NAME		"MP3_PLAY"
#define MINICONTROL_NAME	"[musicplayer-mini]"
#define MUSIC_STATUS_FILE_PATH	"MusicPlayStatus.ini"
#define MAX_NAM_LEN 640
#define MP_APP_ID "org.tizen.music-player-lite"

static int register_mp3_play_module(void *data);
static int unregister_mp3_play_module(void);
static int wake_up_cb(void *data);

static int updated_while_lcd_off = 0;
static Ecore_File_Monitor *pFileMonitor = NULL;

icon_s mp3_play = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_MINICTRL,
	.init = register_mp3_play_module,
	.fini = unregister_mp3_play_module,
	.wake_up = wake_up_cb
};

enum {
	MUSIC_PLAY,
	MUSIC_PAUSED,
};

static char *icon_path[] = {
	"Background playing/B03_Backgroundplaying_music_playing.png",
	"Background playing/B03_Backgroundplaying_music_paused.png",
	NULL
};

static int prevIndex = -1;


static void set_app_state(void* data)
{
	mp3_play.ad = data;
}


static void show_image_icon(void *data, int status)
{
	if (prevIndex == status)
		return;

	mp3_play.img_obj.data = icon_path[status];
	icon_show(&mp3_play);

	prevIndex = status;
}


static void hide_image_icon(void)
{
	icon_hide(&mp3_play);

	prevIndex = -1;
}


static void show_mp_icon(void* data)
{
	FILE* fp = fopen(util_get_data_file_path(MUSIC_STATUS_FILE_PATH), "r");
	char line[MAX_NAM_LEN+1];

	retm_if(data == NULL, "Invalid parameter!");

	retm_if(fp == NULL, "Invalid file path !!");

	if(icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		_D("need to update %d",updated_while_lcd_off);
		fclose(fp);
		return;
	}

	updated_while_lcd_off = 0;

	if(fgets(line, MAX_NAM_LEN, fp)) {
		if(strstr(line, "play")) {
			_D("Music state : PLAY");
			show_image_icon(data, MUSIC_PLAY);
		}
		else if(strstr(line, "pause")) {
			_D("Music state : PAUSED");
			show_image_icon(data, MUSIC_PAUSED);
		}
		else if(strstr(line, "stop") || strstr(line, "off")) {
			_D("Music state : STOP or OFF");
			hide_image_icon();
		}
	}
	retm_if(fclose(fp), "File close error!");

}


void hide_mp_icon(void)
{
	hide_image_icon();
}


static void indicator_mp3_play_change_cb(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char* path)
{
	retm_if(data == NULL, "Invalid parameter!");
	_D("indicator_mp3_play_change_cb");

	show_mp_icon(data);

	return;
}


static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0)
		return OK;

	indicator_mp3_play_change_cb(data, pFileMonitor, (Ecore_File_Event)NULL, util_get_data_file_path(MUSIC_STATUS_FILE_PATH));
	return OK;
}

static int register_mp3_play_module(void *data)
{
	_D("Music file monitor added !!");
	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ECORE_FILE_MONITOR_DELIF(pFileMonitor);
	pFileMonitor = util_file_monitor_add(util_get_data_file_path(MUSIC_STATUS_FILE_PATH),
			(Ecore_File_Monitor_Cb)indicator_mp3_play_change_cb, data);

	retvm_if(pFileMonitor == NULL, FAIL, "util_file_monitor_add return NULL!!");

	return OK;
}


static int unregister_mp3_play_module(void)
{
	_D("Music file monitor removed !!");
	retvm_if(pFileMonitor == NULL, FAIL, "File Monitor do not exist !");

	util_file_monitor_remove(pFileMonitor);
	pFileMonitor = NULL;

	return OK;
}
