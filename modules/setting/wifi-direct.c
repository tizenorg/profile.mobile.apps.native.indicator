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
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED8
#define MODULE_NAME		"wifi_direct"

static int register_wifi_direct_module(void *data);
static int unregister_wifi_direct_module(void);
static int wake_up_cb(void *data);
static void show_icon(void *data, int index);
static void hide_icon(void);

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif

icon_s wifi_direct = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_wifi_direct_module,
	.fini = unregister_wifi_direct_module,
	.wake_up = wake_up_cb,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

static int updated_while_lcd_off = 0;
static int prevIndex = -1;
Ecore_Timer *timer_wifi = NULL;

enum {
	WIFI_DIRECT_CONNECTED = 0,
	WIFI_DIRECT_MAX,
};

#define WIFI_D_ICON_CONNECTED \
	"Bluetooth, NFC, GPS/B03_Wi-fi_direct-On_connected.png"

static const char *icon_path[WIFI_DIRECT_MAX] = {
	[WIFI_DIRECT_CONNECTED] = WIFI_D_ICON_CONNECTED
};



static void set_app_state(void* data)
{
	wifi_direct.ad = data;
}



static void show_icon(void *data, int index)
{
	if (index < WIFI_DIRECT_CONNECTED || index >= WIFI_DIRECT_MAX)
		index = WIFI_DIRECT_CONNECTED;

	if(prevIndex == index) {
		return;
	}

	wifi_direct.img_obj.data = icon_path[index];
	icon_show(&wifi_direct);

	prevIndex = index;
	util_signal_emit(wifi_direct.ad,"indicator.wifidirect.show","indicator.prog");
}

static void hide_icon(void)
{
	icon_hide(&wifi_direct);

	prevIndex = -1;
	util_signal_emit(wifi_direct.ad,"indicator.wifidirect.hide","indicator.prog");
}

static void indicator_wifi_direct_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	if(icon_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_int(VCONFKEY_WIFI_DIRECT_STATE, &status);

	if (ret == OK) {
		DBG("wifi_direct STATUS: %d", status);

		switch (status) {
		case VCONFKEY_WIFI_DIRECT_GROUP_OWNER:
		case VCONFKEY_WIFI_DIRECT_CONNECTED:
			show_icon(data, WIFI_DIRECT_CONNECTED);
			break;
		default:
			hide_icon();
			break;
		}
		return;
	}

	hide_icon();
	return;
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		return OK;
	}

	indicator_wifi_direct_change_cb(NULL, data);
	return OK;
}

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	char *tmp = NULL;
	char buf[256] = {0,};
	int status = 0;

	vconf_get_int(VCONFKEY_WIFI_DIRECT_STATE, &status);

	switch (status)
	{
	case VCONFKEY_WIFI_DIRECT_GROUP_OWNER:
	case VCONFKEY_WIFI_DIRECT_CONNECTED:
		snprintf(buf, sizeof(buf), "%s, %s", _("Wi-Fi direct On and Connected"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
		break;
	default:
		break;
	}

	tmp = strdup(buf);
	if (!tmp) return NULL;
	return tmp;
}
#endif

static int register_wifi_direct_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_WIFI_DIRECT_STATE,
					indicator_wifi_direct_change_cb, data);

	indicator_wifi_direct_change_cb(NULL, data);

	return ret;
}

static int unregister_wifi_direct_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_WIFI_DIRECT_STATE,
					indicator_wifi_direct_change_cb);

	return ret;
}
