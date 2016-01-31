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
#include "main.h"
#include "modules.h"
#include "icon.h"
#include <sys/statvfs.h>
#include <app_event.h>

static event_handler_h handler;

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"lowmem"

static int register_lowmem_module(void *data);
static int unregister_lowmem_module(void);
static int wake_up_cb(void *data);
void check_storage();
void get_internal_storage_status(double *total, double *avail);

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
	{
		return;
	}

	lowmem.img_obj.data = icon_path[0];
	icon_show(&lowmem);

	bShown = 1;
}

static void hide_image_icon(void)
{
	icon_hide(&lowmem);

	bShown = 0;
}



static void indicator_lowmem_pm_state_change_cb(keynode_t *node, void *data)
{
}



static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0 && lowmem.obj_exist == EINA_FALSE)
	{
		return OK;
	}

	return OK;
}



static void on_changed_receive(const char *event_name, bundle *event_data, void *user_data)
{
	char *val = NULL;

	if (!event_name || strcmp(event_name, SYSTEM_EVENT_LOW_MEMORY)) {
		DBG("Invalid event: %s", event_name);
		return;
	}

	DBG("lowmem signal Received");

	int ret = bundle_get_str(event_data, EVENT_KEY_LOW_MEMORY, &val);
	if (ret != BUNDLE_ERROR_NONE) {
		ERR("bundle_get_str failed for %s: %d", EVENT_KEY_LOW_MEMORY, ret);
		return;
	}

	if (!val) {
		ERR("Empty bundle value for %s", EVENT_KEY_LOW_MEMORY);
		return;
	}

	if (strcmp(val, EVENT_VAL_MEMORY_NORMAL)) {
		hide_image_icon();
	}
	else if (strcmp(val, EVENT_VAL_MEMORY_HARD_WARNING)) {
		show_image_icon();
	}
	else if (strcmp(val, EVENT_VAL_MEMORY_SOFT_WARNING)) {
		show_image_icon();
	}
	else {
		ERR("Unrecognized %s value %s", EVENT_KEY_LOW_MEMORY, val);
	}
}



static void event_cleaner(void)
{
	if (!handler) {
		DBG("already unregistered");
		return;
	}

	event_remove_event_handler(handler);
	handler = NULL;
}



static int event_listener(void)
{
	if (handler) {
		DBG("alreay exist");
		return -1;
	}

	int ret = event_add_event_handler(SYSTEM_EVENT_LOW_MEMORY, on_changed_receive, NULL, &handler);
	if (ret != EVENT_ERROR_NONE) {
		ERR("event_add_event_handler failed on %s: %d", SYSTEM_EVENT_LOW_MEMORY, ret);
		return -1;
	}
	return 0;
}



static int register_lowmem_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE,
						indicator_lowmem_pm_state_change_cb, data);

	check_storage();
	event_listener();

	return ret;
}



static int unregister_lowmem_module(void)
{
	int ret;


	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE,
						indicator_lowmem_pm_state_change_cb);

	event_cleaner();

	return ret;
}



void check_storage()
{
	double total = 0.0;
	double available = 0.0;
	double percentage = 0.0;
	get_internal_storage_status(&total, &available);
	percentage = (available/total) * 100.0;
	DBG("check_storage : Total : %lf, Available : %lf Percentage : %lf", total, available, percentage);
	if(percentage <= 5.0)
	{
		show_image_icon();
	}
}



void get_internal_storage_status(double *total, double *avail)
{
	int ret;
	double tmp_total;
	struct statvfs s;
	const double sz_32G = 32. * 1073741824;
	const double sz_16G = 16. * 1073741824;
	const double sz_8G = 8. * 1073741824;

	retif(total == NULL, , "Invalid parameter!");
	retif(avail == NULL, , "Invalid parameter!");

	ret = statvfs("/opt/usr", &s);
	if (0 == ret)
	{
		tmp_total = (double)s.f_frsize * s.f_blocks;
		*avail = (double)s.f_bsize * s.f_bavail;

		if (sz_16G < tmp_total)
			*total = sz_32G;
		else if (sz_8G < tmp_total)
			*total = sz_16G;
		else
			*total = sz_8G;
	}
}
