/*
 *  indicator
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Junghyun Kim <jh1114.kim@samsung.com> Kangwon Lee <newton.lee@samsung.com>
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
#include "indicator_ui.h"
#include "modules.h"
#include "indicator_icon_util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"earphone"
#define TIMER_INTERVAL	0.3

static int register_earphone_module(void *data);
static int unregister_earphone_module(void);

Indicator_Icon_Object earphone[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_earphone_module,
	.fini = unregister_earphone_module
},
{
	.win_type = INDICATOR_WIN_LAND,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_earphone_module,
	.fini = unregister_earphone_module
}

};

static const char *icon_path[] = {
	"Earphone/B03_Earphone.png",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		earphone[i].ad = data;
	}
}

static void show_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		earphone[i].img_obj.data = icon_path[0];
		indicator_util_icon_show(&earphone[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&earphone[i]);
	}
}

static void indicator_earphone_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_SYSMAN_EARJACK, &status);
	if (ret == FAIL) {
		ERR("Failed to get VCONFKEY_MMC_STATE!");
		return;
	}

	switch (status) {
	case VCONFKEY_SYSMAN_EARJACK_3WIRE:
	case VCONFKEY_SYSMAN_EARJACK_4WIRE:
	case VCONFKEY_SYSMAN_EARJACK_TVOUT:
		INFO("Earphone connected");
		show_image_icon();
		break;

	default:
		hide_image_icon();
		break;
	}
}

static int register_earphone_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_EARJACK,
				       indicator_earphone_change_cb, data);
	if (ret != OK)
		ERR("Failed to register earphoneback!");

	indicator_earphone_change_cb(NULL, data);

	return ret;
}

static int unregister_earphone_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_EARJACK,
				       indicator_earphone_change_cb);
	if (ret != OK)
		ERR("Failed to unregister earphoneback!");

	return OK;
}
