/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
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
#define TIMER_INTERVAL	0.5
#define SYNC_ICON_NUM 4

static int register_active_sync_module(void *data);
static int unregister_active_sync_module(void);
static int wake_up_cb(void *data);

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
	.wake_up = wake_up_cb
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
	.wake_up = wake_up_cb
}
};

static const char *icon_path[] = {
	"Processing/B03_Processing_Syncing_01.png",
	"Processing/B03_Processing_Syncing_02.png",
	"Processing/B03_Processing_Syncing_03.png",
	"Processing/B03_Processing_Syncing_04.png",
	NULL
};

static Ecore_Timer *timer;
static int icon_index = 0;
static int updated_while_lcd_off = 0;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		active_sync[i].ad = data;
	}

}

static void show_image_icon(void* data, int index)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		active_sync[i].img_obj.data = icon_path[index];
		indicator_util_icon_show(&active_sync[i]);
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

static Eina_Bool show_sync_icon_cb(void* data)
{
	DBG("show_sync_icon_cb!, %d",icon_index);
	show_image_icon(data,icon_index);
	icon_index = (++icon_index % SYNC_ICON_NUM) ? icon_index : 0;

	return ECORE_CALLBACK_RENEW;
}

static void show_sync_icon(void* data)
{
	if(timer==NULL)
	{
		timer = ecore_timer_add(TIMER_INTERVAL,	show_sync_icon_cb, data);
	}
	else
	{
		ERR("show_sync_icon!, timer");
	}
}

static void hide_sync_icon(void)
{
	DBG("hide_sync_icon!, %d",icon_index);
	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
		icon_index = 0;
	}

	hide_image_icon();
}


static void indicator_active_sync_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = 0;
	int result = 0;

	retif(data == NULL, , "Invalid parameter!");

}

static void indicator_active_sync_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	retif(data == NULL, , "Invalid parameter!");

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
	if(updated_while_lcd_off==0&&active_sync[0].obj_exist==EINA_FALSE)
	{
		DBG("ICON WAS NOT UPDATED");
		return OK;
	}

	indicator_active_sync_change_cb(NULL, data);
	return OK;
}

static int register_active_sync_module(void *data)
{
	int ret = 0;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	return ret;
}

static int unregister_active_sync_module(void)
{
	int ret = 0;

	return OK;
}
