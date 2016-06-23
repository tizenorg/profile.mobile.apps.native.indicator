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
#include <storage.h>
#include <vconf.h>

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_3
#define MODULE_NAME		"mmc"

static int register_ext_storage_module(void *data);
static int unregister_ext_storage_module(void);
static int wake_up_cb(void *data);

icon_s ext_storage = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_ext_storage_module,
	.fini = unregister_ext_storage_module,
	.wake_up = wake_up_cb
};

typedef enum {
	EXT_STORAGE_STATE_REMOVED = 0x01,
	EXT_STORAGE_STATE_MOUNTED = 0x02,
	EXT_STORAGE_STATE_MOUNTED_READ_ONLY = 0x04,
	EXT_STORAGE_STATE_UNMOUNTABLE = 0x08,
} ext_storage_state_e;

static const char *icon_path[] = {
	"Storage/B03_storage_t_flash.png",
	NULL
};

static int updated_while_lcd_off = 0;
static int bShown = 0;


static void set_app_state(void *data)
{
	ext_storage.ad = data;
}

static void show_image_icon(void)
{
	if (bShown == 1)
		return;

	ext_storage.img_obj.data = icon_path[0];
	icon_show(&ext_storage);

	bShown = 1;
}

static void hide_image_icon(void)
{
	icon_hide(&ext_storage);

	bShown = 0;
}

static void _ext_storage_cb(keynode_t *node, void *user_data)
{
	_D("indicator_mmc_change_cb");

	int storage_status;

	if(icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}

	updated_while_lcd_off = 0;

	vconf_get_int(VCONFKEY_SYSMAN_MMC_STATUS, &storage_status);

	switch (storage_status) {
		case VCONFKEY_SYSMAN_MMC_MOUNTED:
			show_image_icon();
			break;

		case VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED:
		case VCONFKEY_SYSMAN_MMC_REMOVED:
			hide_image_icon();
			break;

		default:
			_E("Invalid state:%d", storage_status);
			break;
	}
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0)
		return OK;

	_ext_storage_cb(NULL, data);

	return OK;
}

static int register_ext_storage_module(void *data)
{
	int ret;
	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_MMC_STATUS, _ext_storage_cb, data);
	retvm_if(ret == FAIL, FAIL, "vconf_notify_key_changed failed!");

	_ext_storage_cb(NULL, data);

	return OK;
}

static int unregister_ext_storage_module(void)
{
	vconf_ignore_key_changed(VCONFKEY_SYSMAN_MMC_STATUS, _ext_storage_cb);

	return OK;
}
