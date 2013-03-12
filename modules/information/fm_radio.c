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
#include "indicator_icon_util.h"
#include "modules.h"
#include "indicator_ui.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_1
#define MODULE_NAME		"FM_Radio"

static int register_fm_radio_module(void *data);
static int unregister_fm_radio_module(void);

Indicator_Icon_Object fm_radio[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_fm_radio_module,
	.fini = unregister_fm_radio_module
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_fm_radio_module,
	.fini = unregister_fm_radio_module
}
};

static char *icon_path[] = {
	"Background playing/B03_Backgroundplaying_FMradio.png",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		fm_radio[i].ad = data;
	}
}

static void show_image_icon(void *data)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		fm_radio[i].img_obj.data = icon_path[0];
		indicator_util_icon_show(&fm_radio[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&fm_radio[i]);
	}
}

static void indicator_fm_radio_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_RADIO_STATE, &status);
	if (ret == OK) {
		INFO("FM_RADIO state: %d", status);
		if (status == VCONFKEY_RADIO_PLAY)
			show_image_icon(data);
		else
			hide_image_icon();
	}
	return;
}

static int register_fm_radio_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_RADIO_STATE,
				       indicator_fm_radio_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback!");
	indicator_fm_radio_change_cb(NULL, data);

	return ret;
}

static int unregister_fm_radio_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_RADIO_STATE,
				       indicator_fm_radio_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

//	hide_image_icon();

	return OK;
}
