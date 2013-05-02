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
#include "indicator_gui.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED5
#define MODULE_NAME		"home"

static int register_home_module(void *data);
static int unregister_home_module(void);

Indicator_Icon_Object home[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,0,APPTRAY_ICON_WIDTH,APPTRAY_ICON_HEIGHT},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.exist_in_view = EINA_FALSE,
	.init = register_home_module,
	.fini = unregister_home_module,
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,0,APPTRAY_ICON_WIDTH,APPTRAY_ICON_HEIGHT},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.exist_in_view = EINA_FALSE,
	.init = register_home_module,
	.fini = unregister_home_module,
}

};

static const char *icon_path[] = {
	"App tray/B03_app_tray.PNG",
	"App tray/B03_app_tray_press.PNG",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		home[i].ad = data;
	}

}
static void show_image_icon(int index)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		home[i].img_obj.data = icon_path[index];
		indicator_util_icon_show(&home[i]);
	}
}
static void change_home_icon_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = -1;

	retif(data == NULL, , "Invalid parameter!");

}
static int register_home_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	show_image_icon(0);
	return 0;
}

static int unregister_home_module(void)
{

	return 0;
}
