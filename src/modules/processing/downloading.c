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
#define MODULE_NAME		"downloading"
#define TIMER_INTERVAL	0.3
#define ICON_NUM 7

static int register_downloading_module(void *data);
static int unregister_downloading_module(void);
static int wake_up_cb(void *data);

icon_s downloading = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_downloading_module,
	.fini = unregister_downloading_module,
	.wake_up = wake_up_cb
};

static const char *icon_path[] = {
	"Processing/B03_Processing_download_ani_00.png",
	"Processing/B03_Processing_download_ani_01.png",
	"Processing/B03_Processing_download_ani_02.png",
	"Processing/B03_Processing_download_ani_03.png",
	"Processing/B03_Processing_download_ani_04.png",
	"Processing/B03_Processing_download_ani_05.png",
	"Processing/B03_Processing_download_ani_06.png",
	NULL
};

static Ecore_Timer *timer;
static int icon_index = 0;
static int updated_while_lcd_off = 0;

static void set_app_state(void *data)
{
	downloading.ad = data;
}

static void show_image_icon(void *data, int index)
{
	downloading.img_obj.data = icon_path[index];
	icon_show(&downloading);
}

static void hide_image_icon(void)
{
	icon_hide(&downloading);
}

static Eina_Bool show_downloading_icon_cb(void *data)
{

	show_image_icon(data, icon_index);
	icon_index++;
	icon_index = (icon_index % ICON_NUM) ? icon_index : 0;

	return ECORE_CALLBACK_RENEW;
}

static void show_downloading_icon(void *data)
{
	if (timer == NULL)
		timer = ecore_timer_add(TIMER_INTERVAL,	show_downloading_icon_cb, data);
	else
		_E("show_downloading_icon!, timer");
}

static void hide_downloading_icon(void)
{
	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
		icon_index = 0;
	}

	hide_image_icon();
}


static void indicator_downloading_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = 0;

	retm_if(data == NULL, "Invalid parameter!");

	if (icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_int(VCONFKEY_WIFI_DIRECT_RECEIVING_STATE, &status);
	retm_if(ret != 0,"vconf_get_int failed");

	if (status == 1)
		show_downloading_icon(data);
	else
		hide_downloading_icon();
}

static void indicator_downloading_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;

	retm_if(data == NULL, "Invalid parameter!");

	if (vconf_get_int(VCONFKEY_PM_STATE, &status) < 0) {
		_E("Error getting VCONFKEY_PM_STATE value");

		if (timer != NULL) {
			ecore_timer_del(timer);
			timer = NULL;
		}
		return;
	}

	if (status == VCONFKEY_PM_STATE_LCDOFF)
		if (timer != NULL) {
			ecore_timer_del(timer);
			timer = NULL;
		}
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0 && downloading.obj_exist == EINA_FALSE)
		return OK;

	indicator_downloading_change_cb(NULL, data);
	return OK;
}

static int register_downloading_module(void *data)
{
	int ret = 0;
	int ret2 = 0;
	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_WIFI_DIRECT_RECEIVING_STATE,
										indicator_downloading_change_cb, data);
	retvm_if(ret != 0, FAIL, "vconf_notify_key_changed failed");

	ret2 = vconf_notify_key_changed(VCONFKEY_PM_STATE,
										indicator_downloading_pm_state_change_cb, data);
	if(ret2 != 0) _E("vconf_notify_key_changed failed");

	indicator_downloading_change_cb(NULL, data);

	return OK;
}

static int unregister_downloading_module(void)
{
	int ret = 0;

	ret |= vconf_ignore_key_changed(VCONFKEY_WIFI_DIRECT_RECEIVING_STATE,
										indicator_downloading_change_cb);

	ret |= vconf_ignore_key_changed(VCONFKEY_PM_STATE,
										indicator_downloading_pm_state_change_cb);
	return ret;
}
