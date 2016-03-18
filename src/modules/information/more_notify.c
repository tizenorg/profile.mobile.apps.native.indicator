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
#include <app_preference.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_MIN
#define MODULE_NAME		"more_notify"

static int register_more_notify_module(void *data);
static int unregister_more_notify_module(void);
static int wake_up_cb(void *data);

static int updated_while_lcd_off = 0;

static bool bShow = 0;

icon_s more_notify = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_more_notify_module,
	.fini = unregister_more_notify_module,
	.wake_up = wake_up_cb
};

enum {
	MUSIC_PLAY,
	MUSIC_PAUSED,
};

static char *icon_path[] = {
	"Notify/B03_notify_more.png",
	NULL
};


static void set_app_state(void* data)
{
	more_notify.ad = data;
}


static void show_image_icon_by_win(win_info* win)
{
	more_notify.img_obj.data = icon_path[0];
	icon_show(&more_notify);
}


static void hide_image_icon_by_win(win_info* win)
{
	icon_hide(&more_notify);
}



static void _handle_more_notify_icon(win_info* win, bool val)
{
	retm_if(win == NULL, "Invalid parameter!");

	if (bShow == val)
		return;

	bShow = val;

	_D("val %d", val);

	if (val) {
		show_image_icon_by_win(win);
		_D("_handle_more_notify_show");
	} else {
		hide_image_icon_by_win(win);
		_D("_handle_more_notify_hide");
	}
}


static void indicator_more_notify_change_cb(const char *key, void *data)
{
	struct appdata *ad = (struct appdata *)(more_notify.ad);
	bool val = 0;

	retm_if(data == NULL, "Invalid parameter!");

	_D("indicator_more_notify_change_cb");
	win_info* win = NULL;

	if (strcmp(key, INDICATOR_MORE_NOTI) == 0) {
		win = &(ad->win);
	} else {
		_E("invalid value!");
		return;
	}

	int err = preference_get_boolean(key, &val);
	retm_if(err != PREFERENCE_ERROR_NONE, "preference_get_boolean failed: %s", get_error_message(err));

	_handle_more_notify_icon(win, val);

	return;
}


static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0)
		return OK;

	indicator_more_notify_change_cb(INDICATOR_MORE_NOTI, data);
	return OK;
}


static int register_more_notify_module(void *data)
{
	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	int ret = preference_set_changed_cb(INDICATOR_MORE_NOTI, indicator_more_notify_change_cb, data);
	retvm_if(ret != PREFERENCE_ERROR_NONE, FAIL, "preference_set_changed_cb failed: %s", get_error_message(ret));

	return OK;
}


static int unregister_more_notify_module(void)
{
	int ret = preference_unset_changed_cb(INDICATOR_MORE_NOTI);
	retvm_if(ret != PREFERENCE_ERROR_NONE, FAIL, "preference_set_changed_cb failed: %s", get_error_message(ret));

	return OK;
}
