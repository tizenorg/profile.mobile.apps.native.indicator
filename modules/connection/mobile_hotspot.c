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

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"mobile_hotspot"

static int register_mobile_hotspot_module(void *data);
static int unregister_mobile_hotspot_module(void);
static int wake_up_cb(void *data);

icon_s mobile_hotspot = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_mobile_hotspot_module,
	.fini = unregister_mobile_hotspot_module,
	.wake_up = wake_up_cb
};

enum {
	TETHERING_ALL_ON_CONNECTED = 0,
	TETHERING_ALL_ON_NOT_CONNECTED,
	TETHERING_BT_ON_CONNECTED,
	TETHERING_BT_ON_NOT_CONNECTED,
	TETHERING_USB_ON_CONNECTED,
	TETHERING_USB_ON_NOT_CONNECTED,
	TETHERING_WIFI_ON_CONNECTED,
	TETHERING_WIFI_ON_NOT_CONNECTED,
	TETHERING_MAX,
};

static const char *icon_path[TETHERING_MAX] = {
	[TETHERING_ALL_ON_CONNECTED] 		= "tethering/B03_All_connected.png",
	[TETHERING_ALL_ON_NOT_CONNECTED] 	= "tethering/B03_All_no_connected.png",
	[TETHERING_BT_ON_CONNECTED] 		= "tethering/B03_BT_connected.png",
	[TETHERING_BT_ON_NOT_CONNECTED] 	= "tethering/B03_BT_no_connected.png",
	[TETHERING_USB_ON_CONNECTED] 		= "tethering/B03_USB_connected.png",
	[TETHERING_USB_ON_NOT_CONNECTED] 	= "tethering/B03_USB_no_connected.png",
	[TETHERING_WIFI_ON_CONNECTED] 		= "tethering/B03_Wi_Fi_connected.png",
	[TETHERING_WIFI_ON_NOT_CONNECTED] 	= "tethering/B03_Wi_Fi_no_connected.png",
};
static int updated_while_lcd_off = 0;
static int prevIndex = -1;



static void set_app_state(void* data)
{
	mobile_hotspot.ad = data;
}



static void show_image_icon(int type)
{
	if(prevIndex == type)
	{
		return;
	}

	mobile_hotspot.img_obj.data = icon_path[type];
	icon_show(&mobile_hotspot);

	prevIndex = type;
}



static void hide_image_icon(void)
{
	icon_hide(&mobile_hotspot);

	prevIndex = -1;
}



static void indicator_mobile_hotspot_change_cb(keynode_t *node, void *data)
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

	ret = vconf_get_int(VCONFKEY_MOBILE_HOTSPOT_MODE, &status);
	if (ret == OK)
	{
		DBG("mobile_hotspot status: %d", status);
		if (status != VCONFKEY_MOBILE_HOTSPOT_MODE_NONE)
		{
			int connected_device = 0;
			int on_device_cnt = 0;

			int bBT = 0;
			int bWifi = 0;
			int bUSB = 0;

			vconf_get_int(VCONFKEY_MOBILE_HOTSPOT_CONNECTED_DEVICE,&connected_device);

			if(status&VCONFKEY_MOBILE_HOTSPOT_MODE_BT)
				bBT = 1;
			if(status&VCONFKEY_MOBILE_HOTSPOT_MODE_WIFI)
				bWifi = 1;
			if(status&VCONFKEY_MOBILE_HOTSPOT_MODE_USB)
				bUSB = 1;

			on_device_cnt = bBT+bWifi+bUSB;
			DBG("STSTUS %d %d %d , %d",bBT,bWifi,bUSB,connected_device);
			if(on_device_cnt==0)
			{
				hide_image_icon();
				return;
			}
			else if(on_device_cnt>=2)
			{
				if(connected_device>=1)
				{
					show_image_icon(TETHERING_ALL_ON_CONNECTED);
				}
				else
				{
					show_image_icon(TETHERING_ALL_ON_NOT_CONNECTED);
				}
			}
			else
			{
				if(bBT==1)
				{
					if(connected_device>0)
					{
						show_image_icon(TETHERING_BT_ON_CONNECTED);
					}
					else
					{
						show_image_icon(TETHERING_BT_ON_NOT_CONNECTED);
					}
				}

				if(bUSB==1)
				{
					if(connected_device>0)
					{
						show_image_icon(TETHERING_USB_ON_CONNECTED);
					}
					else
					{
						show_image_icon(TETHERING_USB_ON_NOT_CONNECTED);
					}
				}

				if(bWifi==1)
				{
					if(connected_device>0)
					{
						show_image_icon(TETHERING_WIFI_ON_CONNECTED);
					}
					else
					{
						show_image_icon(TETHERING_WIFI_ON_NOT_CONNECTED);
					}
				}
			}

		}
		else
		{
			hide_image_icon();
			return;
		}
	}

}



static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		return OK;
	}

	indicator_mobile_hotspot_change_cb(NULL, data);
	return OK;
}



static int register_mobile_hotspot_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE,
			       indicator_mobile_hotspot_change_cb, data);
	if (ret != OK) {
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_CONNECTED_DEVICE,
			       indicator_mobile_hotspot_change_cb, data);
	if (ret != OK) {
		r = r | ret;
	}

	indicator_mobile_hotspot_change_cb(NULL, data);

	return r;
}



static int unregister_mobile_hotspot_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE,
				       indicator_mobile_hotspot_change_cb);

	ret = ret | vconf_ignore_key_changed(VCONFKEY_MOBILE_HOTSPOT_CONNECTED_DEVICE,
				       indicator_mobile_hotspot_change_cb);

	return ret;
}
