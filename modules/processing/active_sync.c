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
#include "common.h"
#include "indicator.h"
#include "indicator_ui.h"
#include "modules.h"
#include "indicator_icon_util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"active_sync"

static int register_active_sync_module(void *data);
static int unregister_active_sync_module(void);
static int hib_enter_active_sync_module(void);
static int hib_leave_active_sync_module(void *data);

Indicator_Icon_Object active_sync[INDICATOR_WIN_MAX] = {
{
	.type = INDICATOR_IMG_ICON,
	.win_type = INDICATOR_WIN_PORT,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_active_sync_module,
	.fini = unregister_active_sync_module,
	.hib_enter = hib_enter_active_sync_module,
	.hib_leave = hib_leave_active_sync_module
},
{
	.type = INDICATOR_IMG_ICON,
	.win_type = INDICATOR_WIN_LAND,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_active_sync_module,
	.fini = unregister_active_sync_module,
	.hib_enter = hib_enter_active_sync_module,
	.hib_leave = hib_leave_active_sync_module
}
};

static const char *icon_path[] = {
	"Processing/B03_Processing_Syncing.png",
	"Processing/B03_Processing_Syncerror.png",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		active_sync[i].ad = data;
	}

}

static void show_image_icon(void* data)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		active_sync[i].img_obj.data = icon_path[0];
		indicator_util_icon_show(&active_sync[i]);
		indicator_util_icon_animation_set(&active_sync[i],
							  ICON_ANI_ROTATE);
	}
}

static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&active_sync[i]);
	}
}

static void indicator_active_sync_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = 0;

	retif(data == NULL, , "Invalid parameter!");

	if (ret == FAIL) {
		ERR("Failed to get VCONFKEY_SYNC_STATE!");
		return;
	}

	if (status == TRUE) {
		INFO("Active sync is set");
		show_image_icon(data);

	} else {
		INFO("Active sync is unset");
		hide_image_icon();
	}
}

static int register_active_sync_module(void *data)
{
	int ret = 0;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	if (ret != OK)
		ERR("Failed to register callback!");

	indicator_active_sync_change_cb(NULL, data);

	return ret;
}

static int unregister_active_sync_module(void)
{
	int ret = 0;

	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}

static int hib_enter_active_sync_module(void)
{
	int ret = 0;

	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}

static int hib_leave_active_sync_module(void *data)
{
	int ret = 0;

	retif(data == NULL, FAIL, "Invalid parameter!");

	retif(ret != OK, FAIL, "Failed to register callback!");

	indicator_active_sync_change_cb(NULL, data);
	return OK;
}
