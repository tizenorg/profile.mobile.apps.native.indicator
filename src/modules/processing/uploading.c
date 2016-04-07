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
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_1
#define MODULE_NAME		"uploading"
#define TIMER_INTERVAL	0.3
#define ICON_NUM 7

static int register_uploading_module(void *data);
static int unregister_uploading_module(void);
static int wake_up_cb(void *data);

icon_s uploading = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_uploading_module,
	.fini = unregister_uploading_module,
	.wake_up = wake_up_cb
};

static const char *icon_path[] = {
	"Processing/B03_Processing_upload_ani_00.png",
	"Processing/B03_Processing_upload_ani_01.png",
	"Processing/B03_Processing_upload_ani_02.png",
	"Processing/B03_Processing_upload_ani_03.png",
	"Processing/B03_Processing_upload_ani_04.png",
	"Processing/B03_Processing_upload_ani_05.png",
	"Processing/B03_Processing_upload_ani_06.png",
	NULL
};

static Ecore_Timer *timer;
static int icon_index = 0;
static int updated_while_lcd_off = 0;

static void set_app_state(void* data)
{
	uploading.ad = data;
}

static void show_image_icon(void* data, int index)
{
	uploading.img_obj.data = icon_path[index];
	icon_show(&uploading);
}

static void hide_image_icon(void)
{
	icon_hide(&uploading);
}

static Eina_Bool show_uploading_icon_cb(void* data)
{

	show_image_icon(data,icon_index);
	icon_index = (++icon_index % ICON_NUM) ? icon_index : 0;

	return ECORE_CALLBACK_RENEW;
}

static void show_uploading_icon(void* data)
{
	if(timer==NULL)
	{
		timer = ecore_timer_add(TIMER_INTERVAL,	show_uploading_icon_cb, data);
	}
	else
	{
		_E("show_uploading_icon!, timer");
	}
}

static void hide_uploading_icon(void)
{
	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
		icon_index = 0;
	}

	hide_image_icon();
}


static void indicator_uploading_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret;

	retm_if(data == NULL, "Invalid parameter!");

	if(icon_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_int(VCONFKEY_WIFI_DIRECT_SENDING_STATE, &status);

	retm_if(ret != 0, "Failed to get VCONFKEY_WIFI_DIRECT_SENDING_STATE value");

	if (status == 1) {
		show_uploading_icon(data);

	} else {
		hide_uploading_icon();
	}
}

static void indicator_uploading_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;

	retm_if(data == NULL, "Invalid parameter!");

	vconf_get_int(VCONFKEY_PM_STATE, &status);

	if(status == VCONFKEY_PM_STATE_LCDOFF)
	{
		if (timer != NULL) {
			ecore_timer_del(timer);
			timer = NULL;
		}
	}
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0 && uploading.obj_exist == EINA_FALSE)
	{
		return OK;
	}

	indicator_uploading_change_cb(NULL, data);
	return OK;
}

static int register_uploading_module(void *data)
{
	int ret = 0;

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_WIFI_DIRECT_SENDING_STATE,
										indicator_uploading_change_cb, data);
	retvm_if(ret != 0, FAIL, "vconf_notify_key_changed failed[%d]", ret);

	ret = ret | vconf_notify_key_changed(VCONFKEY_PM_STATE,
										indicator_uploading_pm_state_change_cb, data);
	if(ret != 0) {
		_E("vconf_notify_key_changed failed[%d]", ret);
		unregister_uploading_module();
		return FAIL;
	}

	indicator_uploading_change_cb(NULL, data);

	return ret;
}

static int unregister_uploading_module(void)
{
	vconf_ignore_key_changed(VCONFKEY_WIFI_DIRECT_SENDING_STATE,
										indicator_uploading_change_cb);

	vconf_ignore_key_changed(VCONFKEY_PM_STATE,
										indicator_uploading_pm_state_change_cb);

	return OK;
}
