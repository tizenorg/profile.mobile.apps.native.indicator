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
#include <bluetooth.h>
#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED7
#define MODULE_NAME		"bluetooth"
#define TIMER_INTERVAL	0.5

Ecore_Timer *timer_bt = NULL;

static int register_bluetooth_module(void *data);
static int unregister_bluetooth_module(void);
static int wake_up_cb(void *data);
static void show_image_icon(void *data, int index);
static void hide_image_icon(void);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif


icon_s bluetooth = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_bluetooth_module,
	.fini = unregister_bluetooth_module,
	.wake_up = wake_up_cb,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

enum {
	LEVEL_MIN = 0,
	LEVEL_BT_NOT_CONNECTED = LEVEL_MIN,
	LEVEL_BT_CONNECTED,
	LEVEL_BT_HEADSET,
	LEVEL_MAX
};

static const char *icon_path[LEVEL_MAX] = {
	[LEVEL_BT_NOT_CONNECTED] =
		"Bluetooth, NFC, GPS/B03_BT_On&Connected.png",
	[LEVEL_BT_CONNECTED] = "Bluetooth, NFC, GPS/B03-4_BT_activated_on.png",
	[LEVEL_BT_HEADSET] =
		"Bluetooth, NFC, GPS/B03_BT_On&Connected&headset.png",
};

static int updated_while_lcd_off = 0;
static int prevIndex = -1;

static void set_app_state(void* data)
{
	bluetooth.ad = data;
}

static void show_image_icon(void *data, int index)
{
	if (index < LEVEL_MIN || index >= LEVEL_MAX)
		index = LEVEL_MIN;

	if(prevIndex == index)
	{
		return;
	}
	bluetooth.img_obj.data = icon_path[index];
	icon_show(&bluetooth);

	prevIndex = index;
	util_signal_emit(bluetooth.ad,"indicator.bluetooth.show","indicator.prog");
}

static void hide_image_icon(void)
{
	icon_hide(&bluetooth);

	prevIndex = -1;
	util_signal_emit(bluetooth.ad,"indicator.bluetooth.hide","indicator.prog");
}

#define NO_DEVICE			(0x00)
#define HEADSET_CONNECTED	(0x01)
#define DEVICE_CONNECTED	(0x02)
#define DATA_TRANSFER		(0x04)

static void show_bluetooth_icon(void *data, int status)
{
	if (status == NO_DEVICE) {
		show_image_icon(data, LEVEL_BT_NOT_CONNECTED);
		return;
	}

	if (status & HEADSET_CONNECTED) {
		show_image_icon(data, LEVEL_BT_HEADSET);
	} else if (status & DEVICE_CONNECTED) {
		show_image_icon(data, LEVEL_BT_CONNECTED);
	}
	return;
}

static void indicator_bluetooth_adapter_state_changed_cb(int result, bt_adapter_state_e adapter_state, void *user_data)
{
	DBG("BT STATUS: %d", adapter_state);
	if (adapter_state != BT_ADAPTER_ENABLED)    // If adapter_state is NULL. hide_image_icon().
	{
		DBG("BT is not enabled. So hide BT icon.");
		hide_image_icon();
	}
}

static void indicator_bluetooth_change_cb(keynode_t *node, void *data)
{
	DBG("indicator_bluetooth_change_cb");
	int dev = 0;
	int ret = 0;
	int result = NO_DEVICE;
	bt_adapter_state_e adapter_state = BT_ADAPTER_DISABLED;

	retif(data == NULL, , "Invalid parameter!");

	if (icon_get_update_flag() == 0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = bt_adapter_get_state(&adapter_state);
	retif(ret != BT_ERROR_NONE, , "bt_adapter_get_state failed");
	if (adapter_state != BT_ADAPTER_ENABLED) {  // If adapter_state is NULL. hide_image_icon().
		DBG("BT is not enabled. So don't need to update BT icon.");
		return;
	}

	ret = vconf_get_int(VCONFKEY_BT_DEVICE, &dev);
	if (ret == OK) {
		DBG("Show BT ICON (BT DEVICE: %d)", dev);

		if (dev == VCONFKEY_BT_DEVICE_NONE) {
			show_bluetooth_icon(data, NO_DEVICE);
			return;
		}
		if ((dev & VCONFKEY_BT_DEVICE_HEADSET_CONNECTED) ||
		    (dev & VCONFKEY_BT_DEVICE_A2DP_HEADSET_CONNECTED)) {
			result = (result | HEADSET_CONNECTED);
			DBG("BT_HEADSET_CONNECTED(%x)", result);
		}
		if ((dev & VCONFKEY_BT_DEVICE_SAP_CONNECTED)
		|| (dev & VCONFKEY_BT_DEVICE_PBAP_CONNECTED)
		|| (dev & VCONFKEY_BT_DEVICE_HID_CONNECTED)
		|| (dev & VCONFKEY_BT_DEVICE_PAN_CONNECTED)) {
			result = (result | DEVICE_CONNECTED);
			DBG("BT_DEVICE_CONNECTED(%x)", result);
		}
		show_bluetooth_icon(data, result);
	}
	return;
}



static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0 && bluetooth.obj_exist == EINA_FALSE) {
		return OK;
	}

	indicator_bluetooth_change_cb(NULL, data);
	return OK;
}

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	char *tmp = NULL;
	char buf[256] = {0,};

	switch(prevIndex)
	{
		case LEVEL_BT_NOT_CONNECTED:
			snprintf(buf, sizeof(buf), "%s, %s", _("IDS_IDLE_BODY_BLUETOOTH_ON"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
			break;
		case LEVEL_BT_CONNECTED:
			snprintf(buf, sizeof(buf), "%s, %s", _("Bluetooth On and Connected"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
			break;
		case LEVEL_BT_HEADSET:
			snprintf(buf, sizeof(buf), "%s, %s", _("Bluetooth On and Connected headset"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
			break;
	}

	tmp = strdup(buf);
	if (!tmp) return NULL;
	return tmp;
}
#endif

static int register_bluetooth_module(void *data)
{
	int r = 0, ret = -1;
	bt_adapter_state_e adapter_state = BT_ADAPTER_DISABLED;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	// Register bluetooth adapter state call-back.
	ret = bt_initialize();
	if(ret != BT_ERROR_NONE) ERR("bt_initialize failed");
	ret = bt_adapter_set_state_changed_cb(indicator_bluetooth_adapter_state_changed_cb, data);
	if(ret != BT_ERROR_NONE) ERR("bt_adapter_set_state_changed_cb failed");

	ret = vconf_notify_key_changed(VCONFKEY_BT_DEVICE,
					indicator_bluetooth_change_cb, data);
	if (ret != OK) {
		r = r | ret;
	}

	ret = bt_adapter_get_state(&adapter_state);
	retif(ret != BT_ERROR_NONE, -1, "bt_adapter_get_state failed");

	indicator_bluetooth_change_cb(NULL, data);
	indicator_bluetooth_adapter_state_changed_cb(0, adapter_state, data);

	return r;
}

static int unregister_bluetooth_module(void)
{
	int ret = 0;

	// Unregister bluetooth adapter state call-back.
	ret = bt_adapter_unset_state_changed_cb();
	if(ret != BT_ERROR_NONE) ERR("bt_adapter_unset_state_changed_cb failed");
	ret = bt_deinitialize();
	if(ret != BT_ERROR_NONE) ERR("bt_deinitialize failed");


	ret = ret | vconf_ignore_key_changed(VCONFKEY_BT_DEVICE,
					indicator_bluetooth_change_cb);

	return ret;
}
