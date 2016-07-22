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
#include <runtime_info.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"usb"
#define TIMER_INTERVAL	0.3

static int register_usb_module(void *data);
static int unregister_usb_module(void);

icon_s usb = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_usb_module,
	.fini = unregister_usb_module
};

static const char *icon_path[] = {
	"Connection/B03_USB.png",
	NULL
};

static int bShown = 0;


static void set_app_state(void *data)
{
	usb.ad = data;
}

static void show_image_icon(void)
{
	if (bShown == 1) return;

	usb.img_obj.data = icon_path[0];
	icon_show(&usb);

	bShown = 1;
}

static void hide_image_icon(void)
{
	icon_hide(&usb);

	bShown = 0;
}

static void _cradle_change_cb(keynode_t *node, void *data)
{
	int cradle = 0;

	vconf_get_int(VCONFKEY_SYSMAN_CRADLE_STATUS, &cradle);
	if (cradle > 0) {
		_D("cradle Status: %d", cradle);
		hide_image_icon();
		return;
	}
}

static void _usb_change_cb(keynode_t *node, void *data)
{
	bool usb_state;
	int status;
	int ret;
	int cradle = 0;
	int tethering = 0;

	ret_if(!data);

	vconf_get_int(VCONFKEY_SYSMAN_CRADLE_STATUS, &cradle);
	if (cradle > 0) {
		_D("cradle Status: %d", cradle);
		hide_image_icon();
		return;
	}

	vconf_get_int(VCONFKEY_MOBILE_HOTSPOT_MODE, &tethering);
	if (tethering & VCONFKEY_MOBILE_HOTSPOT_MODE_USB) {
		_D("tethering Status: %d", tethering);
		hide_image_icon();
		return;
	}
	/* First, check usb state */
	ret = runtime_info_get_value_bool(RUNTIME_INFO_KEY_USB_CONNECTED, &usb_state);
	if (ret == RUNTIME_INFO_ERROR_NONE) {
		if (usb_state) {
			show_image_icon();
			return;
		} else {
			/* Second, check usb Host status */
			ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
			if (ret == OK) {
				if (status >= VCONFKEY_SYSMAN_USB_HOST_CONNECTED) {
					_D("Host USB Status: %d", status);
					show_image_icon();
					return;
				} else {
					hide_image_icon();
				}
			}
		}
	}

	return;
}

static void _runtime_info_usb_change_cb(runtime_info_key_e key, void *data)
{
	ret_if(!data);

	_usb_change_cb(NULL, data);

	return;
}

static int register_usb_module(void *data)
{
	int ret = -1;

	retv_if(!data, 0);

	set_app_state(data);

	ret = runtime_info_set_changed_cb(RUNTIME_INFO_KEY_USB_CONNECTED, _runtime_info_usb_change_cb, data);
	ret = ret | vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS, _usb_change_cb, data);
	ret = ret | vconf_notify_key_changed(VCONFKEY_SYSMAN_CRADLE_STATUS, _cradle_change_cb, data);
	ret = ret | vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, _usb_change_cb, data);

	_usb_change_cb(NULL, data);

	return ret;
}

static int unregister_usb_module(void)
{
	int ret;

	ret = runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_USB_CONNECTED);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS, _usb_change_cb);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_SYSMAN_CRADLE_STATUS, _cradle_change_cb);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, _usb_change_cb);
	return ret;
}
/* End of file */
