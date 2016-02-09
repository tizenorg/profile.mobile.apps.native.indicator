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
#include <app_event.h>
#include <storage.h>
#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "log.h"

static event_handler_h handler;

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"lowmem"

static int register_lowmem_module(void *data);
static int unregister_lowmem_module(void);
static int wake_up_cb(void *data);
static int check_storage();

icon_s lowmem = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_lowmem_module,
	.fini = unregister_lowmem_module,
	.wake_up = wake_up_cb
};

static const char *icon_path[] = {
	"Storage/B03_storage_memoryfull.png",
	NULL
};


static int updated_while_lcd_off = 0;
static int bShown = 0;


static void set_app_state(void* data)
{
	lowmem.ad = data;
}

static void show_image_icon(void)
{
	if(bShown == 1)
		return;

	lowmem.img_obj.data = icon_path[0];
	icon_show(&lowmem);

	bShown = 1;
}

static void hide_image_icon(void)
{
	icon_hide(&lowmem);

	bShown = 0;
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0 && lowmem.obj_exist == EINA_FALSE)
		return OK;

	return check_storage();
}

static void on_changed_receive_cb(const char *event_name, bundle *event_data, void *user_data)
{
	char *val = NULL;

	retm_if ((!event_name || strcmp(event_name, SYSTEM_EVENT_LOW_MEMORY)),"Invalid event: %s", event_name);

	_D("lowmem signal Received");

	int ret = bundle_get_str(event_data, EVENT_KEY_LOW_MEMORY, &val);
	retm_if (ret != BUNDLE_ERROR_NONE,"bundle_get_str failed for %s: %d", EVENT_KEY_LOW_MEMORY, ret);

	retm_if (!val, "Empty bundle value for %s", EVENT_KEY_LOW_MEMORY);

	if (strcmp(val, EVENT_VAL_MEMORY_NORMAL))
		hide_image_icon();
	else if (strcmp(val, EVENT_VAL_MEMORY_HARD_WARNING))
		show_image_icon();
	else if (strcmp(val, EVENT_VAL_MEMORY_SOFT_WARNING))
		show_image_icon();
	else
		ERR("Unrecognized %s value %s", EVENT_KEY_LOW_MEMORY, val);
}

static void event_cleaner(void)
{
	if (!handler) {
		_D("already unregistered");
		return;
	}

	event_remove_event_handler(handler);
	handler = NULL;
}

static int event_listener_add(void)
{
	if (handler) {
		DBG("alreay exist");
		return FAIL;
	}

	int ret = event_add_event_handler(SYSTEM_EVENT_LOW_MEMORY, on_changed_receive_cb, NULL, &handler);
	retvm_if(ret != EVENT_ERROR_NONE, FAIL, "event_add_event_handler failed on %s: %d", SYSTEM_EVENT_LOW_MEMORY, ret);

	return OK;
}

static int register_lowmem_module(void *data)
{
	int ret;

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = check_storage();
	retv_if(ret != OK, FAIL);

	ret = event_listener_add();
	retv_if(ret != OK, FAIL);

	return ret;
}

static int unregister_lowmem_module(void)
{
	event_cleaner();

	return OK;
}

bool storage_cb (int storage_id, storage_type_e type, storage_state_e state, const char *path, void *user_data)
{

	if (type == STORAGE_TYPE_INTERNAL) {
		int *s_id = (int *)user_data;
		*s_id = storage_id;
		return false;
	}
	return true;
}

static int check_storage()
{
	int ret;
	double percentage = 0.0;
	unsigned long long available_bytes;
	unsigned long long total_bytes;

	int internal_storage_id;
	ret = storage_foreach_device_supported (storage_cb, (void *)(&internal_storage_id));
		retvm_if(ret != STORAGE_ERROR_NONE, FAIL, "storage_foreach_device_supported failed[%s]", get_error_message(ret));

	storage_get_available_space(internal_storage_id, &available_bytes);
	retvm_if(ret != STORAGE_ERROR_NONE, FAIL, "storage_get_available_space failed[%s]", get_error_message(ret));

	storage_get_total_space(internal_storage_id, &total_bytes);
	retvm_if(ret != STORAGE_ERROR_NONE, FAIL, "storage_get_total_space failed[%s]", get_error_message(ret));

	double a_b = (double)available_bytes;
	double t_b = (double)total_bytes;
	percentage = (a_b/t_b) * 100.0;

	_D("check_storage : Total : %lf, Available : %lf Percentage : %lf", t_b, a_b, percentage);

	if(percentage <= 5.0)
		show_image_icon();

	return OK;
}
