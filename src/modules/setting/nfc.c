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
#include "util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"nfc"

static int register_nfc_module(void *data);
static int unregister_nfc_module(void);
static int wake_up_cb(void *data);


icon_s nfc = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_nfc_module,
	.fini = unregister_nfc_module,
	.wake_up = wake_up_cb
};

enum {
	NFC_ON = 0,
	NFC_NUM,
};


static const char *icon_path[NFC_NUM] = {
	[NFC_ON] = "Bluetooth, NFC, GPS/B03_NFC_On.png",
};
static int updated_while_lcd_off = 0;
static int prevIndex = -1;



static void set_app_state(void* data)
{
	nfc.ad = data;
}



static void show_image_icon(void *data, int index)
{
	if (index < NFC_ON || index >= NFC_NUM)
		index = NFC_ON;

	if(prevIndex == index)
	{
		DBG("same icon");
		return;
	}

	nfc.img_obj.data = icon_path[index];
	icon_show(&nfc);

	prevIndex = index;
}

static void hide_image_icon(void)
{
	icon_hide(&nfc);

	prevIndex = -1;
}

static void indicator_nfc_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	if(util_is_orf()==0)
	{
		DBG("does not support in this bin");
		return;
	}

	if(icon_get_update_flag()==0)
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
			/* Show NFC Icon */
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
