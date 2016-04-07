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
#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_3
#define MODULE_NAME		"mmc"
#define TIMER_INTERVAL	0.3

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

int state = 0x00;

static const char *icon_path[] = {
	"Storage/B03_storage_t_flash.png",
	NULL
};
static int updated_while_lcd_off = 0;
static int bShown = 0;


static void set_app_state(void* data)
{
	ext_storage.ad = data;
}

static void show_image_icon(void)
{
	if(bShown == 1)
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

static void _ext_storage_cb (int storage_id, storage_state_e state, void *data)
{
	int *status = (int *)data;

	_D("indicator_mmc_change_cb");
	if(icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	switch (state) {
		case STORAGE_STATE_MOUNTED_READ_ONLY:
		case STORAGE_STATE_MOUNTED:
			*status = *status | EXT_STORAGE_STATE_MOUNTED;
			if (*status & EXT_STORAGE_STATE_REMOVED)
				*status = *status - EXT_STORAGE_STATE_REMOVED; // remove bit LSB
			break;

		case STORAGE_STATE_UNMOUNTABLE:
		case STORAGE_STATE_REMOVED:
			*status = *status | EXT_STORAGE_STATE_REMOVED;
			if (*status & EXT_STORAGE_STATE_MOUNTED)
				*status = *status - EXT_STORAGE_STATE_MOUNTED; // remove bit second LSB
			break;

		default:
			_D("Storage with ID:%d, Invalid state:%d", storage_id, state);
			break;
	}
}

static void change_indicator_state(int storage_state)
 {
	_D("Storage indicator icon set");

	if ((storage_state & EXT_STORAGE_STATE_MOUNTED) || (storage_state & EXT_STORAGE_STATE_MOUNTED_READ_ONLY))
		show_image_icon();

	else
		hide_image_icon();
 }

static bool _wake_up_storage_register_cb(int storage_id, storage_type_e type, storage_state_e state, const char *path, void *user_data)
{
	if (type == STORAGE_TYPE_EXTERNAL)
		_ext_storage_cb(storage_id, state, user_data);

	return true;
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0)
		return OK;

	int ret = storage_foreach_device_supported(_wake_up_storage_register_cb, (void *)&state);
	retvm_if(ret != STORAGE_ERROR_NONE, FAIL, "storage_foreach_device_supported failed[%s]", get_error_message(ret));

	change_indicator_state(state);

	return OK;
}

static bool _storage_register_cb(int storage_id, storage_type_e type, storage_state_e state, const char *path, void *user_data)
{
	if (type == STORAGE_TYPE_EXTERNAL) {
		_ext_storage_cb(storage_id, state, user_data);
		int ret = storage_set_state_changed_cb(storage_id, _ext_storage_cb, user_data);
		retvm_if(ret != STORAGE_ERROR_NONE, false, "storage_foreach_device_supported failed[%s]", get_error_message(ret));
	}

	return true;
}

static int register_ext_storage_module(void *data)
{
	int ret;
	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	state = EXT_STORAGE_STATE_REMOVED;

	ret = storage_foreach_device_supported (_storage_register_cb, (void *)&state);

	retvm_if(ret != STORAGE_ERROR_NONE, FAIL, "storage_foreach_device_supported failed[%s]", get_error_message(ret));

	change_indicator_state(state);

	return OK;
}

static bool _storage_unregister_cb(int storage_id, storage_type_e type, storage_state_e state, const char *path, void *user_data)
{
	int ret;
	if (type == STORAGE_TYPE_EXTERNAL) {
		ret = storage_unset_state_changed_cb(storage_id, _ext_storage_cb);
		if(ret != STORAGE_ERROR_NONE)
			_D("storage_unset_state_changed_cb failed[%s]", get_error_message(ret));
	}
	return true;
}

static int unregister_ext_storage_module(void)
{
	int ret;

	ret = storage_foreach_device_supported (_storage_unregister_cb, NULL);
	retvm_if(ret != STORAGE_ERROR_NONE, FAIL, "storage_foreach_device_supported failed[%s]", get_error_message(ret));

	return OK;
}
