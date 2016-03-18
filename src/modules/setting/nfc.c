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
#include <nfc.h>
#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "util.h"
#include "log.h"

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

	if(prevIndex == index) {
		_D("same icon");
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

static void indicator_nfc_change_cb(bool activated, void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if(icon_get_update_flag()==0) {
		updated_while_lcd_off = 1;
		_D("need to update %d",updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	INFO("NFC STATUS is %s", (activated)? "activated" : "not activated");

	if (activated) {
		/* Show NFC Icon */
		show_image_icon(data, NFC_ON);
		return;
	}
	hide_image_icon();
	return;
}

static int wake_up_cb(void *data)
{
	int ret;
	bool status;

	if(updated_while_lcd_off==0)
		return OK;

	status = nfc_manager_is_activated();
	ret = get_last_result();
	retvm_if(ret != NFC_ERROR_NONE, ret, "nfc_manager_is_activated failed[%s]!", get_error_message(ret));

	indicator_nfc_change_cb(status, data);
	return OK;
}

static int register_nfc_module(void *data)
{
	int ret;
	bool status;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = nfc_manager_set_activation_changed_cb(indicator_nfc_change_cb, data);
	retif(ret != NFC_ERROR_NONE, FAIL, "Failed to register callback!");

	status = nfc_manager_is_activated();
	ret = get_last_result();
	retvm_if(ret != NFC_ERROR_NONE, ret, "nfc_manager_is_activated failed[%s]!", get_error_message(ret));

	indicator_nfc_change_cb(status, data);

	return OK;
}

static int unregister_nfc_module(void)
{
	nfc_manager_unset_activation_changed_cb();

	if (get_last_result() != NFC_ERROR_NONE)
		_E("Failed to unregister callback!");

	return OK;
}
