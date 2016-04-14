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
#include <system_settings.h>
#include <runtime_info.h>

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_6
#define MODULE_NAME		"silent"

static int register_silent_module(void *data);
static int unregister_silent_module(void);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif


icon_s silent = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_silent_module,
	.fini = unregister_silent_module,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

enum {
	PROFILE_MUTE,
	PROFILE_VIBRATION,
	PROFILE_NUM,
};

static const char *icon_path[PROFILE_NUM] = {
	[PROFILE_MUTE] = "Profile/B03_Profile_Mute.png",
	[PROFILE_VIBRATION] = "Profile/B03_Profile_Vibration.png",
};

static int prevIndex = -1;

static void set_app_state(void* data)
{
	silent.ad = data;
}

static void show_image_icon(int index)
{
	if(prevIndex == index)
	{
		return;
	}

	silent.img_obj.data = icon_path[index];
	icon_show(&silent);

	prevIndex = index;
}

static void hide_image_icon(void)
{
	if(prevIndex == -1)
	{
		_D("ALREADY HIDE");
		return;
	}

	icon_hide(&silent);

	prevIndex = -1;
}

static bool _get_sound_profile(bool *silent_mode, bool *vib_status)
{
	int ret;

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_SOUND_SILENT_MODE, silent_mode);
	retvm_if(ret != SYSTEM_SETTINGS_ERROR_NONE, false, "Failed to get silent mode status.");

	ret = runtime_info_get_value_bool(RUNTIME_INFO_KEY_VIBRATION_ENABLED, vib_status);
	retvm_if(ret != RUNTIME_INFO_ERROR_NONE, false, "Failed to get vibration status.");

	_D("CURRENT Sound Status: %d, Vibration Status: %d", (int)(*silent_mode), (int)(*vib_status));

	return true;
}

static void _set_sound_profile(bool silent_mode, bool vib_status)
{
	if (silent_mode && !vib_status) {
		/* Mute Mode */
		show_image_icon(PROFILE_MUTE);
	} else if (!silent_mode && vib_status) {
		/* Vibration Only Mode */
		show_image_icon(PROFILE_VIBRATION);
	} else {
		hide_image_icon();
	}
}

static void _display_sound_profile(void)
{
	bool silent_mode = false;
	bool vib_status = false;

	if (_get_sound_profile(&silent_mode, &vib_status))
		_set_sound_profile(silent_mode, vib_status);
}

static void _system_setting_silent_change_cb(system_settings_key_e key, void *user_data)
{
	_display_sound_profile();
}

static void _runtime_info_vibration_change_cb(runtime_info_key_e key, void *user_data)
{
	_display_sound_profile();
}

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	char *tmp = NULL;
	char buf[256] = {0,};
	bool silent_mode = false;
	bool vib_status = false;

	if (!_get_sound_profile(&silent_mode, &vib_status))
		return NULL;

	if (silent_mode && !vib_status) {
		/* Mute Mode */
		snprintf(buf, sizeof(buf), "%s, %s, %s", _("IDS_IDLE_BODY_MUTE"),_("Sound profile"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
	} else if (!silent_mode && vib_status) {
		/* Vibration Only Mode */
		snprintf(buf, sizeof(buf), "%s, %s, %s", _("IDS_IDLE_BODY_VIBRATE"),_("Sound profile"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
	} else {
		//do nothing;
	}

	tmp = strdup(buf);
	if (!tmp)
		return NULL;

	return tmp;
}
#endif

static int register_silent_module(void *data)
{
	int ret;

	retv_if(!data, 0);

	set_app_state(data);

	ret = util_system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_SOUND_SILENT_MODE, _system_setting_silent_change_cb, data);
	retvm_if(ret != SYSTEM_SETTINGS_ERROR_NONE, ret, "Failed to set silent mode change callback function.");

	ret = util_runtime_info_set_changed_cb(RUNTIME_INFO_KEY_VIBRATION_ENABLED, _runtime_info_vibration_change_cb, data);
	if (ret != RUNTIME_INFO_ERROR_NONE)
		util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_SOUND_SILENT_MODE);

	retvm_if(ret != RUNTIME_INFO_ERROR_NONE, ret, "Failed to set vibration change callback function.");

	/* It is sufficient to call only one callback function responsible for sound profile display,
	 * as both functions:
	 * 		- _system_setting_silent_change_cb()
	 * 		- _runtime_info_vibration_change_cb()
	 * does the same but in different context.
	 */
	_system_setting_silent_change_cb(SYSTEM_SETTINGS_KEY_SOUND_SILENT_MODE, data);

	return ret;
}

static int unregister_silent_module(void)
{
	int ret;

	ret = util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_SOUND_SILENT_MODE);
	if (ret != SYSTEM_SETTINGS_ERROR_NONE)
		_E("Failed to unset silent mode change callback function.");

	ret = util_runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_VIBRATION_ENABLED);
	if (ret != RUNTIME_INFO_ERROR_NONE)
		_E("Failed to unset vibration change callback function.");

	return ret;
}
/* End of file */
