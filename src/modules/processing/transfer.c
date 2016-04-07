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
//#include <Ecore_X.h>
#include <wifi.h>

#include "common.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED8
#define MODULE_NAME	"transfer"
#define TIMER_INTERVAL	0.03

static int register_transfer_module(void *data);
static int unregister_transfer_module(void);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif

static int transfer_type = -1;

static const char *icon_path[TRANSFER_MAX] = {
	[TRANSFER_NONE] = "Connection/B03_conection_not_UPdownload.png",
	[TRANSFER_UP] = "Connection/B03_connection_UPload.png",
	[TRANSFER_DOWN] = "Connection/B03_connection_download.png",
	[TRANSFER_UPDOWN] = "Connection/B03_conection_UPdownload.png"
};

icon_s transfer = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_transfer_module,
	.fini = unregister_transfer_module,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb,
#endif
};

static int prevIndex = -1;

static void set_app_state(void* data)
{
	transfer.ad = data;
}

static void hide_image_icon(void)
{
	icon_hide(&transfer);

	prevIndex = -1;
	transfer_type = -1;
}

static void show_image_icon(void *data, int index)
{
	if(prevIndex == index)
	{
		return;
	}

	if(index == -1) {
		hide_image_icon();
	} else {
		transfer.img_obj.data = icon_path[index];
		icon_show(&transfer);

		util_signal_emit(transfer.ad,"indicator.connection.show","indicator.prog");
	}

	prevIndex = index;

}

void show_transfer_icon(void *data,int index,int type)
{
	transfer_type = type;
	show_image_icon(data,index);

}

void hide_transfer_icon(int type)
{
	if(transfer_type!=-1 && transfer_type!=type)
	{
		return;
	}

	hide_image_icon();
	util_signal_emit(transfer.ad,"indicator.connection.hide","indicator.prog");
	transfer_type = -1;
}

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	char *tmp = NULL;
	char buf[256] = {0,};
	int svc_type = 0;
	int status = 0;

	wifi_get_connection_state(&status);

	if (status == WIFI_CONNECTION_STATE_CONNECTED) {
		snprintf(buf, sizeof(buf), "%s, %s, %s", _("IDS_IDLE_BODY_WI_FI"),_("Data network"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
	} else {
		vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &svc_type);

		switch(svc_type)
		{
			case VCONFKEY_TELEPHONY_SVCTYPE_3G:
				snprintf(buf, sizeof(buf), "%s, %s, %s", _("IDS_IDLE_BODY_3G"),_("Data network"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
				break;
			case VCONFKEY_TELEPHONY_SVCTYPE_LTE:
				snprintf(buf, sizeof(buf), "%s, %s, %s", _("4G"),_("Data network"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
				break;
			default:
				break;
		}
	}
	tmp = strdup(buf);
	if (!tmp) return NULL;
	return tmp;
}
#endif
static int register_transfer_module(void *data)
{
	set_app_state(data);
	return OK;
}

static int unregister_transfer_module(void)
{
	return OK;
}
