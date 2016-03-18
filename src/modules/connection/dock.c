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

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"dock"
#define TIMER_INTERVAL	0.3

static int register_dock_module(void *data);
static int unregister_dock_module(void);
static int bShown = 0;

icon_s dock = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_dock_module,
	.fini = unregister_dock_module
};

static const char *icon_path[] = {
	"Desk dock/B03_desk_dock.png",
	NULL
};



static void set_app_state(void* data)
{
	dock.ad = data;
}



static void show_image_icon(void)
{
	if(bShown == 1)
	{
		return;
	}

	dock.img_obj.data = icon_path[0];
	icon_show(&dock);

	bShown = 1;
}



static void hide_image_icon(void)
{
	icon_hide(&dock);

	bShown = 0;
}



static void indicator_dock_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	/* First, check dock status */
	ret = vconf_get_int(VCONFKEY_SYSMAN_CRADLE_STATUS, &status);
	if (ret == OK) {
		if (status > 0) {
			_D("dock Status: %d", status);
			show_image_icon();
		}
		else
		{
			hide_image_icon();
		}
	}

	return;
}



static int register_dock_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_CRADLE_STATUS,
				       indicator_dock_change_cb, data);
	if (ret != OK)
	{
		r = ret;
	}

	indicator_dock_change_cb(NULL, data);

	return r;
}



static int unregister_dock_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_CRADLE_STATUS,
				       indicator_dock_change_cb);

	return ret;
}
