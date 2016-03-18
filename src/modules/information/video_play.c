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
#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_1
#define MODULE_NAME		"VIDEO_PLAY"

static int register_video_play_module(void *data);
static int unregister_video_play_module(void);
static int wake_up_cb(void *data);

static int updated_while_lcd_off = 0;


#define VCONF_VIDEO_PLAY_PLAYSTATUS "memory/private/org.tizen.videos/extern_mode"

icon_s video_play = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_video_play_module,
	.fini = unregister_video_play_module,
	.wake_up = wake_up_cb
};

static char *icon_path[] = {
	"Notification/B03_video.png",
	NULL
};



static void set_app_state(void *data)
{
	video_play.ad = data;
}



static void show_image_icon(void *data)
{
	video_play.img_obj.data = icon_path[0];
	icon_show(&video_play);
}



static void hide_image_icon(void)
{
	icon_hide(&video_play);
}



static void show_video_icon(void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");


	if (icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_bool(VCONF_VIDEO_PLAY_PLAYSTATUS, &status);
	if (ret == OK) {
		_D("VIDEO PLAY state: %d", status);
		if (status == 1)
			show_image_icon(data);
		else
			hide_image_icon();
	}
}



static void indicator_video_play_change_cb(keynode_t *node, void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	show_video_icon(data);

	return;
}



static int wake_up_cb(void *data)
{
	if (updated_while_lcd_off == 0) {
		return OK;
	}

	indicator_video_play_change_cb(NULL, data);

	return OK;
}



static int register_video_play_module(void *data)
{

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	vconf_notify_key_changed(VCONF_VIDEO_PLAY_PLAYSTATUS, indicator_video_play_change_cb, data);

	show_video_icon(data);

	return OK;
}



static int unregister_video_play_module(void)
{
	vconf_ignore_key_changed(VCONF_VIDEO_PLAY_PLAYSTATUS, indicator_video_play_change_cb);

	return OK;
}
