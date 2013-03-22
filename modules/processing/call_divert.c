/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
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

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_5
#define MODULE_NAME		"call_divert"

static int register_call_divert_module(void *data);
static int unregister_call_divert_module(void);
static int hib_enter_call_divert_module(void);
static int hib_leave_call_divert_module(void *data);

Indicator_Icon_Object call_divert[INDICATOR_WIN_MAX] = {
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
	.init = register_call_divert_module,
	.fini = unregister_call_divert_module,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.hib_enter = hib_enter_call_divert_module,
	.hib_leave = hib_leave_call_divert_module
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
	.init = register_call_divert_module,
	.fini = unregister_call_divert_module,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.hib_enter = hib_enter_call_divert_module,
	.hib_leave = hib_leave_call_divert_module
}

};

static const char *icon_path[] = {
	"Processing/B03_Processing_calldivert.png",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		call_divert[i].ad = data;
	}
}

static void show_image_icon(void *data)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		call_divert[i].img_obj.data = icon_path[0];
		indicator_util_icon_show(&call_divert[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&call_divert[i]);
	}
}

static void indicator_call_divert_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE, &status);
	if (ret == FAIL) {
		ERR("Failed to get VCONFKEY_TELEPHONY_CALL_FORWARD_STATE!");
		return;
	}

	if (status == VCONFKEY_TELEPHONY_CALL_FORWARD_ON) {
		INFO("Call diverting is set!");
		show_image_icon(data);
	} else {
		INFO("Call diverting is unset!");
		hide_image_icon();
	}
}

static int register_call_divert_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE,
				       indicator_call_divert_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback!");

	indicator_call_divert_change_cb(NULL, data);

	return ret;
}

static int unregister_call_divert_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE,
				       indicator_call_divert_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}

static int hib_enter_call_divert_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE,
				       indicator_call_divert_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}

static int hib_leave_call_divert_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_CALL_FORWARD_STATE,
				       indicator_call_divert_change_cb, data);
	retif(ret != OK, FAIL, "Failed to register callback!");

	indicator_call_divert_change_cb(NULL, data);
	return OK;
}
