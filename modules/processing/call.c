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
#include <Ecore_X.h>
#include <minicontrol-monitor.h>
#include "common.h"
#include "indicator.h"
#include "indicator_ui.h"
#include "modules.h"
#include "indicator_icon_util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_1
#define MODULE_NAME		"call"
#define MINICONTROL_VOICE_NAME	"[voicecall-quickpanel]"
#define MINICONTROL_VIDEO_NAME	"[videocall-quickpanel]"

static int register_call_module(void *data);
static int unregister_call_module(void);
static int mctrl_monitor_cb(int action, const char *name, void *data);

Indicator_Icon_Object call[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_call_module,
	.fini = unregister_call_module,
	.minictrl_control = mctrl_monitor_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_call_module,
	.fini = unregister_call_module,
	.minictrl_control = mctrl_monitor_cb
}
};

static const char *icon_path[] = {
	"Call/B03_Call_Duringcall.png",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		call[i].ad = data;
	}
}

static void show_image_icon(void *data)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		call[i].img_obj.data = icon_path[0];
		indicator_util_icon_show(&call[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&call[i]);
	}
}

static void icon_animation_set(enum indicator_icon_ani type)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_animation_set(&call[i], type);
	}
}

static void show_call_icon( void *data)
{
	int status = 0;
	int ret = 0;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_CALL_STATE, &status);
	if (ret == FAIL) {
		ERR("Failed to get VCONFKEY_CALL_STATE!");
		return;
	}

	INFO("Call state = %d", status);
	switch (status) {
	case VCONFKEY_CALL_VOICE_CONNECTING:
	case VCONFKEY_CALL_VIDEO_CONNECTING:
		show_image_icon(data);
		icon_animation_set(ICON_ANI_BLINK);
		break;
	case VCONFKEY_CALL_VOICE_ACTIVE:
	case VCONFKEY_CALL_VIDEO_ACTIVE:
		show_image_icon(data);
		icon_animation_set(ICON_ANI_NONE);
		break;
	case VCONFKEY_CALL_OFF:
		INFO("Call off");
		hide_image_icon();
		break;
	default:
		ERR("Invalid value %d", status);
		break;
	}
}

static void indicator_call_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = 0;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_CALL_STATE, &status);
	if (ret == FAIL) {
		ERR("Failed to get VCONFKEY_CALL_STATE!");
		return;
	}
	switch (status) {
	case VCONFKEY_CALL_VOICE_CONNECTING:
	case VCONFKEY_CALL_VIDEO_CONNECTING:
		show_image_icon(data);
		icon_animation_set(ICON_ANI_BLINK);
		break;
	case VCONFKEY_CALL_VOICE_ACTIVE:
	case VCONFKEY_CALL_VIDEO_ACTIVE:
		hide_image_icon();
		break;
	case VCONFKEY_CALL_OFF:
		INFO("Call off");
		hide_image_icon();
		break;
	default:
		ERR("Invalid value %d", status);
		break;
	}

}

static int mctrl_monitor_cb(int action, const char *name, void *data)
{
	int ret = 0;
	int status = 0;

	retif(!data, FAIL, "data is NULL");
	retif(!name, FAIL, "name is NULL");

	if(strncmp(name,MINICONTROL_VOICE_NAME,strlen(MINICONTROL_VOICE_NAME))!=0
		&&strncmp(name,MINICONTROL_VIDEO_NAME,strlen(MINICONTROL_VIDEO_NAME))!=0)
	{
		ERR("_mctrl_monitor_cb: no call%s",name);
		return FAIL;
	}

	DBG("_mctrl_monitor_cb:%s %d",name,action);

	switch (action) {
	case MINICONTROL_ACTION_START:
		show_call_icon(data);
		break;
	case MINICONTROL_ACTION_STOP:
		ret = vconf_get_int(VCONFKEY_CALL_STATE, &status);
		if (ret == FAIL) {
			ERR("Failed to get VCONFKEY_CALL_STATE!");
			return FAIL;
		}
		INFO("Call state = %d", status);
		switch (status) {
		case VCONFKEY_CALL_VOICE_CONNECTING:
		case VCONFKEY_CALL_VIDEO_CONNECTING:
			break;
		case VCONFKEY_CALL_VOICE_ACTIVE:
		case VCONFKEY_CALL_VIDEO_ACTIVE:
		case VCONFKEY_CALL_OFF:
			INFO("Call off");
			icon_animation_set(ICON_ANI_NONE);
			hide_image_icon();
			break;
		default:
			ERR("Invalid value %d", status);
			break;
		}
		break;
	default:
		break;
	}
	return OK;
}

static int register_call_module(void *data)
{
	int ret = 0;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_CALL_STATE, indicator_call_change_cb, data);

	if (ret != OK)
		ERR("Failed to register callback!");

	return OK;
}

static int unregister_call_module(void)
{
	int ret = 0;

	ret = vconf_ignore_key_changed(VCONFKEY_CALL_STATE, indicator_call_change_cb);

	if (ret != OK)
		ERR("Failed to register callback!");

	return OK;
}
