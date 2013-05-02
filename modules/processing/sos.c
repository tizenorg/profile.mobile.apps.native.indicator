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
#include "common.h"
#include "indicator.h"
#include "indicator_ui.h"
#include "modules.h"
#include "indicator_icon_util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"sos"

static int register_sos_module(void *data);
static int unregister_sos_module(void);
static int wake_up_cb(void *data);


Indicator_Icon_Object sos[INDICATOR_WIN_MAX] = {
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
	.init = register_sos_module,
	.fini = unregister_sos_module,
	.wake_up = wake_up_cb
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
	.init = register_sos_module,
	.fini = unregister_sos_module,
	.wake_up = wake_up_cb
}
};

static const char *icon_path[] = {
	"Call/B03_Call_SOSmessge_active.png",
	NULL
};
static int updated_while_lcd_off = 0;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		sos[i].ad = data;
	}
}

static void show_image_icon(void *data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		sos[i].img_obj.data = icon_path[0];
		indicator_util_icon_show(&sos[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&sos[i]);
	}
}

static void icon_animation_set(enum indicator_icon_ani type)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_animation_set(&sos[i],type);
	}
}

static void indicator_sos_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = 0;
	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_PM_STATE, &status);

	if(status == VCONFKEY_PM_STATE_LCDOFF)
	{
		int sos_status = 0;
		ret = vconf_get_int(VCONFKEY_MESSAGE_SOS_STATE, &sos_status);
		if (ret < 0)
			ERR("fail to get [%s]", VCONFKEY_MESSAGE_SOS_STATE);

		INFO("SOS STATUS: %d", sos_status);
		switch (sos_status) {
		case VCONFKEY_MESSAGE_SOS_STANDBY:
			icon_animation_set(ICON_ANI_NONE);
			break;
		default:
			break;
		}
	}
}

static void indicator_sos_change_cb(keynode_t *node, void *data)
{
	int send_option = 0;
	int sos_state = VCONFKEY_MESSAGE_SOS_IDLE;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	if(indicator_util_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		DBG("need to update %d",updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_bool(VCONFKEY_MESSAGE_SOS_SEND_OPTION, &send_option);
	if (ret == FAIL)
		ERR("Failed to get VCONFKEY_MESSAGE_SOS_SEND_OPTION!");

	INFO("sos send option = %d", send_option);

	if (!send_option) {
		hide_image_icon();
		return;
	}

	show_image_icon(data);

	ret = vconf_get_int(VCONFKEY_MESSAGE_SOS_STATE, &sos_state);
	if (ret == FAIL)
		ERR("Failed to get VCONFKEY_MESSAGE_SOS_SEND_OPTION!");

	if (sos_state == VCONFKEY_MESSAGE_SOS_STANDBY)
		icon_animation_set(ICON_ANI_BLINK);
	else
		icon_animation_set(ICON_ANI_NONE);
}
static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0&&sos[0].obj_exist==EINA_FALSE)
	{
		DBG("ICON WAS NOT UPDATED");
		return OK;
	}

	indicator_sos_change_cb(NULL, data);
	return OK;
}

static int register_sos_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_MESSAGE_SOS_SEND_OPTION,
				       indicator_sos_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback! [%s]",
				VCONFKEY_MESSAGE_SOS_SEND_OPTION);

	ret = vconf_notify_key_changed(VCONFKEY_MESSAGE_SOS_STATE,
				       indicator_sos_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback! [%s]",
				VCONFKEY_MESSAGE_SOS_STATE);

	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE,
					       indicator_sos_pm_state_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback! : VCONFKEY_PM_STATE");

	indicator_sos_change_cb(NULL, data);

	return ret;
}

static int unregister_sos_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_MESSAGE_SOS_SEND_OPTION,
				       indicator_sos_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!  [%s]",
				VCONFKEY_MESSAGE_SOS_SEND_OPTION);

	ret = vconf_ignore_key_changed(VCONFKEY_MESSAGE_SOS_STATE,
				       indicator_sos_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!  [%s]",
				VCONFKEY_MESSAGE_SOS_STATE);

	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE,
					       indicator_sos_pm_state_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}

