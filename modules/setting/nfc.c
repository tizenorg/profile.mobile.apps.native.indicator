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
#include "indicator_icon_util.h"
#include "modules.h"
#include "indicator_ui.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_4
#define MODULE_NAME		"nfc"

static int register_nfc_module(void *data);
static int unregister_nfc_module(void);
static int wake_up_cb(void *data);


Indicator_Icon_Object nfc[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_nfc_module,
	.fini = unregister_nfc_module,
	.wake_up = wake_up_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_nfc_module,
	.fini = unregister_nfc_module,
	.wake_up = wake_up_cb
}
};

enum {
	NFC_ON = 0,
	NFC_NUM,
};


static const char *icon_path[NFC_NUM] = {
	[NFC_ON] = "Bluetooth, NFC, GPS/B03_NFC_On.png",
};
static int updated_while_lcd_off = 0;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		nfc[i].ad = data;
	}
}

static void show_image_icon(void *data, int index)
{
	int i = 0;

	if (index < NFC_ON || index >= NFC_NUM)
		index = NFC_ON;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		nfc[i].img_obj.data = icon_path[index];
		indicator_util_icon_show(&nfc[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&nfc[i]);
	}
}

static void indicator_nfc_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	if(indicator_util_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		DBG("need to update %d",updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_bool(VCONFKEY_NFC_STATE, &status);
	if (ret == OK) {
		INFO("NFC STATUS: %d", status);

		if (status == 1) {
			show_image_icon(data, NFC_ON);
			return;
		}
	}

	hide_image_icon();
	return;
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		DBG("ICON WAS NOT UPDATED");
		return OK;
	}

	indicator_nfc_change_cb(NULL, data);
	return OK;
}

static int register_nfc_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_NFC_STATE,
				       indicator_nfc_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback!");

	indicator_nfc_change_cb(NULL, data);

	return ret;
}

static int unregister_nfc_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_NFC_STATE,
				       indicator_nfc_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}
