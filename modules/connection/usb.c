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
#include "indicator_icon_util.h"
#include "modules.h"
#include "indicator_ui.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"usb"
#define TIMER_INTERVAL	0.3

static int register_usb_module(void *data);
static int unregister_usb_module(void);

Indicator_Icon_Object usb[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_usb_module,
	.fini = unregister_usb_module
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_usb_module,
	.fini = unregister_usb_module
}
};

static const char *icon_path[] = {
	"USB tethering/B03_USB.png",
	NULL
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		usb[i].ad = data;
	}
}

static void show_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		usb[i].img_obj.data = icon_path[0];
		indicator_util_icon_show(&usb[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&usb[i]);
	}
}

static void indicator_usb_change_cb(keynode_t *node, void *data)
{
	int status;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS, &status);
	if (ret == OK) {
		if (status >= VCONFKEY_SYSMAN_USB_CONNECTED) {
			INFO("indicator_usb_change_cb : CONNECTION USB Status: %d", status);
			show_image_icon();
			return;
		}
		else
		{
			ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
			if (ret == OK) {
				if (status >= VCONFKEY_SYSMAN_USB_HOST_CONNECTED) {
					INFO("indicator_usb_change_cb : Host USB Status: %d", status);
					show_image_icon();
					return;
				} else
					hide_image_icon();
			}
		}
	}

	return;
}

static int register_usb_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS,
				       indicator_usb_change_cb, data);
	if (ret != OK)
	{
		ERR("Failed to register callback(VCONFKEY_SYSMAN_USB_STATUS)!");
		r = ret;
	}


	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS,
				       indicator_usb_change_cb, data);
	if (ret != OK)
	{
		ERR("Failed to register callback(VCONFKEY_SYSMAN_USB_HOST_STATUS)!");
		r = r|ret;
	}


	indicator_usb_change_cb(NULL, data);

	return r;
}

static int unregister_usb_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_STATUS,
				       indicator_usb_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback(VCONFKEY_SYSMAN_USB_STATUS)!");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS,
				       indicator_usb_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback(VCONFKEY_SYSMAN_USB_HOST_STATUS)!");

	return OK;
}
