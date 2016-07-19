/*
 *  Indicator
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved.
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

#include <notification_setting_internal.h>
#include "icon.h"
#include "log.h"
#include "common.h"

#define MODULE_NAME "do_not_disturb"
#define ICON_PRIORITY INDICATOR_PRIORITY_SYSTEM_2
#define DND_ICON_PATH "waiting for UI designer reply" /* TODO */

static int dnd_register_dnd_module(void *data);
static int dnd_unregister_dnd_module(void);

icon_s dnd = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.img_obj.data = DND_ICON_PATH,
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = dnd_register_dnd_module,
	.fini = dnd_unregister_dnd_module,
};


static void _dnd_image_icon_state_change(bool state)
{
	if (state)
		icon_show(&dnd);
	else
		icon_hide(&dnd);


}


static void _dnd_cb(void *user_data, int do_not_disturb)
{
	_dnd_image_icon_state_change(do_not_disturb);
}


static int dnd_enabled_check()
{
	notification_system_setting_h setting;
	int ret = NOTIFICATION_ERROR_NONE;

	bool dnd = false;

	ret = notification_system_setting_load_system_setting(&setting);
	retvm_if(ret != NOTIFICATION_ERROR_NONE, FAIL,
			"notification_system_setting_load_system_setting failed[%d]:%s",
			ret, get_error_message(ret));

	ret = notification_system_setting_get_do_not_disturb(setting, &dnd);
	if(ret != NOTIFICATION_ERROR_NONE) {
		_E("notification_system_setting_load_system_setting failed[%d]:%s",
				ret, get_error_message(ret));

		notification_system_setting_free_system_setting(setting);
	}

	_dnd_image_icon_state_change(dnd);

	notification_system_setting_free_system_setting(setting);

	return OK;
}


static int dnd_register_dnd_module(void *data)
{
	int ret;

	ret = notification_register_system_setting_dnd_changed_cb(_dnd_cb, data);
	retvm_if(ret != NOTIFICATION_ERROR_NONE, FAIL,
			"notification_register_system_setting_dnd_changed_cb failed[%d]:%s",
			ret, get_error_message(ret));

	ret = dnd_enabled_check();
	if (ret != NOTIFICATION_ERROR_NONE)
		_E("dnd_enabled_check failed[%d]:%s", ret, get_error_message(ret));

	return OK;
}


static int dnd_unregister_dnd_module(void)
{
	notification_unregister_system_setting_dnd_changed_cb(_dnd_cb);
	return OK;
}
