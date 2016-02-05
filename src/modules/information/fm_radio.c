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

// TODO No possibility to be notified about radio status for now.

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_MINICTRL2
#define MODULE_NAME		"FM_Radio"

static int register_fm_radio_module(void *data);
static int unregister_fm_radio_module(void);

icon_s fm_radio = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_MINICTRL,
	.init = register_fm_radio_module,
	.fini = unregister_fm_radio_module
};

static char *icon_path[] = {
	"Background playing/B03_Backgroundplaying_FMradio.png",
	NULL
};



static void set_app_state(void* data)
{
	fm_radio.ad = data;
}



static void show_image_icon(void *data)
{
	fm_radio.img_obj.data = icon_path[0];
	icon_show(&fm_radio);
}



static void hide_image_icon(void)
{
	icon_hide(&fm_radio);
}



static void indicator_fm_radio_change_cb(void *data)
{
	int status = 0;
	int ret = -1;

	DBG("indicator_fm_radio_change_cb called!");
	retif(data == NULL, , "Invalid parameter!");

	if (ret == OK) {
		INFO("FM_RADIO state: %d", status);
		if (status == 1)
			show_image_icon(data);
		else
			hide_image_icon();
	}
	else
	{
		DBG("Fail to get vconfkey (ret:%d)", ret);
	}
	return;
}



void hide_fm_radio_icon(void)
{
	hide_image_icon();
}



static int register_fm_radio_module(void *data)
{
	int ret = -1;

	DBG("register_fm_radio_module called!");
	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	indicator_fm_radio_change_cb(data);

	return ret;
}



static int unregister_fm_radio_module(void)
{
	DBG("unregister_fm_radio_module called!");

	return OK;
}
