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
#include "common.h"
#include "indicator.h"
#include "indicator_ui.h"
#include "modules.h"
#include "indicator_icon_util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_1
#define MODULE_NAME		"silent"

static int register_silent_module(void *data);
static int unregister_silent_module(void);

Indicator_Icon_Object silent[INDICATOR_WIN_MAX] = {
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
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_silent_module,
	.fini = unregister_silent_module
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
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_silent_module,
	.fini = unregister_silent_module
}
};

enum {
	PROFILE_SOUND_VIBRATION,
	PROFILE_MUTE,
	PROFILE_VIBRATION,
	PROFILE_NUM,
};

static const char *icon_path[PROFILE_NUM] = {
	[PROFILE_SOUND_VIBRATION] = "Profile/B03_Profile_Sound&Vibration.png",
	[PROFILE_MUTE] = "Profile/B03_Profile_Mute.png",
	[PROFILE_VIBRATION] = "Profile/B03_Profile_Vibration.png",
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		silent[i].ad = data;
	}
}

static void show_image_icon(int index)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		silent[i].img_obj.data = icon_path[index];
		indicator_util_icon_show(&silent[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&silent[i]);
	}
}

static void indicator_silent_change_cb(keynode_t *node, void *data)
{
	int sound_status = 0;
	int vib_status = 0;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &sound_status);
	ret =
	    vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &vib_status);

	if (ret == OK)
	{
		INFO("CURRENT Sound Status: %d vib_status: %d", sound_status,
		     vib_status);

		if (sound_status == TRUE && vib_status == TRUE)
		{
			show_image_icon(PROFILE_SOUND_VIBRATION);
		}
		else if(sound_status == FALSE && vib_status==FALSE)
		{
			/* Mute Mode */
			show_image_icon(PROFILE_MUTE);
		}
		else if(sound_status == FALSE && vib_status==TRUE)
		{
			/* Vibration Only Mode */
			show_image_icon(PROFILE_VIBRATION);
		}
		else
		{
			hide_image_icon();
		}
	}
	else
	{
		ERR("Failed to get current profile!");
	}

	return;
}

static int register_silent_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL,
				       indicator_silent_change_cb, data);
	if (ret != OK)
		ERR("Fail: register VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
				       indicator_silent_change_cb, data);
	if (ret != OK)
		ERR("Fail: register VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL");

	indicator_silent_change_cb(NULL, data);

	return ret;
}

static int unregister_silent_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL,
				       indicator_silent_change_cb);
	if (ret != OK)
		ERR("Fail: ignore VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
				       indicator_silent_change_cb);
	if (ret != OK)
		ERR("Fail: ignore VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL");
	return OK;
}
