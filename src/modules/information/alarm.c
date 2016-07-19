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
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_5
#define MODULE_NAME		"alarm"

static int bShown = 0;
static int register_alarm_module(void *data);
static int unregister_alarm_module(void);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif

icon_s useralarm = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_alarm_module,
	.fini = unregister_alarm_module,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb,
#endif
};

static char *icon_path[] = {
	"Alarm/B03_Alarm.png",
	NULL
};



static void set_app_state(void* data)
{
	useralarm.ad = data;
}



static void show_image_icon(void *data)
{
	if(bShown == 1)
	{
		return;
	}

	useralarm.img_obj.data = icon_path[0];
	icon_show(&useralarm);

	bShown = 1;
}



static void hide_image_icon(void)
{
	icon_hide(&useralarm);

	bShown = 0;
}



static void indicator_alarm_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret;

	retm_if(data == NULL, "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_ALARM_STATE, &status);
	if (ret == OK) {
		if (status > 0) {
			_D("ALARM COUNT: %d", status);
			show_image_icon(data);
			return;
		}
		_D("ALARM COUNT: %d", status);
		hide_image_icon();
		return;
	}
	_E("Failed to get alarm count!");
	return;
}



#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	char *tmp = NULL;
	char buf[256] = {0,};

	int status = 0;

	vconf_get_int(VCONFKEY_ALARM_STATE, &status);

	if(status>0)
	{
		snprintf(buf, sizeof(buf), "%s, %s, %s", _("IDS_COM_BODY_ALARM"),_("IDS_IDLE_BODY_ICON"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
	}

	tmp = strdup(buf);
	if (!tmp) return NULL;
	return tmp;
}
#endif



static int register_alarm_module(void *data)
{
	int ret = -1;

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_ALARM_STATE, indicator_alarm_change_cb, data);

	retvm_if(ret != 0, FAIL, "vconf_notify_key_changed failed[%d]", ret);

	indicator_alarm_change_cb(NULL, data);

	return ret;
}



static int unregister_alarm_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_ALARM_STATE, indicator_alarm_change_cb);

	return ret;
}
