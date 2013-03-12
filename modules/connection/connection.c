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

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED2
#define MODULE_NAME		"connection"
#define TIMER_INTERVAL	0.3

static int register_conn_module(void *data);
static int unregister_conn_module(void);
extern void show_trnsfr_icon(void *data);
extern void hide_trnsfr_icon(void);

Indicator_Icon_Object conn[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_conn_module,
	.fini = unregister_conn_module
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_conn_module,
	.fini = unregister_conn_module
}
};

enum {
	LEVEL_MIN = 0,
	LEVEL_2G = LEVEL_MIN,
	LEVEL_EDGE,
	LEVEL_3G,
	LEVEL_HS,
	LEVEL_3G_PLUS,
	LEVEL_CDMA,
	LEVEL_GPRS,
	LEVEL_EVDO,
	LEVEL_CONN,
	LEVEL_CONN03,
	LEVEL_CONN_1X,
	LEVEL_LTE,
	LEVEL_MAX
};

static const char *icon_path[LEVEL_MAX] = {
	[LEVEL_2G] = "Connection/B03_connection_GSM.png",
	[LEVEL_EDGE] = "Connection/B03_connection02.png",
	[LEVEL_3G] = "Connection/B03_connection_3G.png",
	[LEVEL_HS] = "Connection/B03_connection_Highspeed.png",
	[LEVEL_3G_PLUS] = "Connection/B03_connection_3G+.png",
	[LEVEL_CDMA] = "Connection/B03_connection_CDMA.png",
	[LEVEL_GPRS] = "Connection/B03_connection_GPRS.png",
	[LEVEL_EVDO] = "Connection/B03_connection_EVDO.png",
	[LEVEL_CONN] = "Connection/B03_connection.png",
	[LEVEL_CONN03] = "Connection/B03_connection03.png",
	[LEVEL_CONN_1X] = "Connection/B03_connection_1x.png",
	[LEVEL_LTE] = "Connection/B03_connection_LTE.png",
};

static Eina_Bool dnet_transferring = EINA_FALSE;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		conn[i].ad = data;
	}
}

static void show_image_icon(int type)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		conn[i].img_obj.data = icon_path[type];
		indicator_util_icon_show(&conn[i]);
	}
}
static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&conn[i]);
	}
}


static void indicator_conn_change_cb(keynode_t *node, void *data)
{
	int svc_type = VCONFKEY_TELEPHONY_SVCTYPE_NONE;
	int status = 0;
	int ret = 0;
	int ps_type = VCONFKEY_TELEPHONY_PSTYPE_NONE;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_int(VCONFKEY_WIFI_STATE, &status);
	if (ret == OK) {
		INFO("CONNECTION WiFi Status: %d", status);
		if ((status == VCONFKEY_WIFI_CONNECTED)) {
			indicator_util_icon_hide(&conn);
			if (dnet_transferring == EINA_TRUE) {
				hide_trnsfr_icon();
				dnet_transferring = EINA_FALSE;
			}
			return;
		}
	}

	ret = vconf_get_int(VCONFKEY_DNET_STATE, &status);
	if (ret == OK) {
		INFO("CONNECTION DNET Status: %d", status);
		if (status == VCONFKEY_DNET_TRANSFER) {
			if (dnet_transferring == EINA_FALSE) {
				show_trnsfr_icon(data);
				dnet_transferring = EINA_TRUE;
			}
		} else {
			if (dnet_transferring == EINA_TRUE) {
				hide_trnsfr_icon();
				dnet_transferring = EINA_FALSE;
			}
		}
	}

	ret = vconf_get_int(VCONFKEY_TELEPHONY_PSTYPE, &ps_type);
	if (ret == OK) {
		INFO("Telephony packet service type: %d", ps_type);

		switch (ps_type) {
		case VCONFKEY_TELEPHONY_PSTYPE_HSDPA:
		case VCONFKEY_TELEPHONY_PSTYPE_HSUPA:
		case VCONFKEY_TELEPHONY_PSTYPE_HSPA:
			show_image_icon(LEVEL_HS);
			return;
		case VCONFKEY_TELEPHONY_PSTYPE_NONE:
		default:
			break;
		}
	}

	ret = vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &svc_type);
	if (ret == OK) {
		INFO("Telephony service type: %d", svc_type);

		switch (svc_type) {
		case VCONFKEY_TELEPHONY_SVCTYPE_2G:
		case VCONFKEY_TELEPHONY_SVCTYPE_2_5G:
			show_image_icon(LEVEL_2G);
			break;
		case VCONFKEY_TELEPHONY_SVCTYPE_2_5G_EDGE:
			show_image_icon(LEVEL_EDGE);
			break;
		case VCONFKEY_TELEPHONY_SVCTYPE_3G:
			show_image_icon(LEVEL_3G);
			break;
		case VCONFKEY_TELEPHONY_SVCTYPE_HSDPA:
			show_image_icon(LEVEL_HS);
			break;
		case VCONFKEY_TELEPHONY_SVCTYPE_LTE:
			show_image_icon(LEVEL_LTE);
			break;

		default:
			hide_image_icon();
			break;
		}

		return;
	}

	hide_image_icon();

	return;
}

static int register_conn_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVCTYPE,
				       indicator_conn_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_DNET_STATE,
				       indicator_conn_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_PSTYPE,
				       indicator_conn_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	indicator_conn_change_cb(NULL, data);

	return r;
}

static int unregister_conn_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SVCTYPE,
				       indicator_conn_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_DNET_STATE,
				       indicator_conn_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	if (dnet_transferring == EINA_TRUE) {
		hide_trnsfr_icon();
		dnet_transferring = EINA_FALSE;
	}

	return OK;
}
