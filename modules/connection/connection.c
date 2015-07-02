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
#include <tapi_common.h>
#include <TelNetwork.h>
#include <TelSim.h>
#include <ITapiNetwork.h>
#include <TelCall.h>
#include <vconf.h>
#include <wifi.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "util.h"
#include "box.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED5
#define MODULE_NAME		"connection"
#define TIMER_INTERVAL	0.3
#define TAPI_HANDLE_MAX  2

static int register_conn_module(void *data);
static int unregister_conn_module(void);
static int wake_up_cb(void *data);
static int transfer_state = -1;
int isBTIconShowing = 0;
extern TapiHandle *tapi_handle[TAPI_HANDLE_MAX+1];
static int updated_while_lcd_off = 0;
static int prevIndex = -1;

icon_s conn = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_conn_module,
	.fini = unregister_conn_module,
	.wake_up = wake_up_cb
};

enum {
	LEVEL_MIN = 0,
	LEVEL_2G = LEVEL_MIN,
	LEVEL_EDGE,
	LEVEL_3G,
	LEVEL_H,
	LEVEL_H_PLUS,
	LEVEL_3G_PLUS,
	LEVEL_LTE,
	LEVEL_4G,
	LEVEL_BT_TETHERING,
	LEVEL_MAX
};

static const char *icon_path[LEVEL_MAX] = {
	[LEVEL_2G] = "Connection/B03_connection_G.png",
	[LEVEL_EDGE] = "Connection/B03_connection_E.png",
	[LEVEL_3G] = "Connection/B03_connection_3G.png",
	[LEVEL_H] = "Connection/B03_connection_H.png",
	[LEVEL_H_PLUS] = "Connection/B03_connection_H+.png",
	[LEVEL_3G_PLUS] = "Connection/B03_connection_3G+.png",
	[LEVEL_LTE] = "Connection/B03_connection_LTE.png",
	[LEVEL_4G] = "Connection/B03_connection_4G.png",
	[LEVEL_BT_TETHERING] = "Bluetooth, NFC, GPS/B03_BT_tethering_network.png"
};



static void set_app_state(void* data)
{
	conn.ad = data;
}



static void show_connection_transfer_icon(void* data)
{
	int state = 0;
	int ret = 0;
	int type = -1;

	ret = vconf_get_int(VCONFKEY_PACKET_STATE, &state);
	if (ret == OK) {
		switch (state) {
		case VCONFKEY_PACKET_RX:
			type = TRANSFER_DOWN;
			break;
		case VCONFKEY_PACKET_TX:
			type = TRANSFER_UP;
			break;
		case VCONFKEY_PACKET_RXTX:
			type = TRANSFER_UPDOWN;
			break;
		case VCONFKEY_PACKET_NORMAL:
			type = TRANSFER_NONE;
			break;
		default:
			type = -1;
			break;
		}
	}

	if(transfer_state==type)
	{
		DBG("same transfer state");
		return;
	}

	DBG("type %d",type);
	transfer_state = type;
	switch (type)
	{
		case TRANSFER_NONE:
			util_signal_emit(conn.ad,"indicator.connection.updown.none","indicator.prog");
			break;
		case TRANSFER_DOWN:
			util_signal_emit(conn.ad,"indicator.connection.updown.download","indicator.prog");
			break;
		case TRANSFER_UP:
			util_signal_emit(conn.ad,"indicator.connection.updown.upload","indicator.prog");
			break;
		case TRANSFER_UPDOWN:
			util_signal_emit(conn.ad,"indicator.connection.updown.updownload","indicator.prog");
			break;
		default:
			break;

	}
}



static void show_image_icon(int type)
{
	if(prevIndex == type)
	{
		return;
	}

	conn.img_obj.data = icon_path[type];
	icon_show(&conn);

	prevIndex = type;
	util_signal_emit(conn.ad,"indicator.connection.show","indicator.prog");
}



static void hide_image_icon(void)
{
	transfer_state = -1;

	icon_hide(&conn);

	util_signal_emit(conn.ad,"indicator.connection.hide","indicator.prog");
	util_signal_emit(conn.ad,"indicator.connection.updown.hide","indicator.prog");

	prevIndex = -1;
}



static void _show_proper_icon(int svc_type,int ps_type, void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if (ps_type != TAPI_NETWORK_PS_TYPE_UNKNOWN) {
		switch (ps_type) {
		case TAPI_NETWORK_PS_TYPE_HSDPA:
		case TAPI_NETWORK_PS_TYPE_HSUPA:
		case TAPI_NETWORK_PS_TYPE_HSPA:
			show_image_icon(LEVEL_H);
			show_connection_transfer_icon(data);
			break;
		case TAPI_NETWORK_PS_TYPE_HSPAP:
			show_image_icon(LEVEL_H_PLUS);
			show_connection_transfer_icon(data);
			break;
		default:
			hide_image_icon();
			break;
		}
	} else {
		switch (svc_type) {
		case TAPI_NETWORK_SERVICE_TYPE_UNKNOWN:
		case TAPI_NETWORK_SERVICE_TYPE_NO_SERVICE:
		case TAPI_NETWORK_SERVICE_TYPE_EMERGENCY:
		case TAPI_NETWORK_SERVICE_TYPE_SEARCH:
			hide_image_icon();
			break;
		case TAPI_NETWORK_SERVICE_TYPE_2G:
		case TAPI_NETWORK_SERVICE_TYPE_2_5G:
			show_image_icon(LEVEL_2G);
			show_connection_transfer_icon(data);
			break;
		case TAPI_NETWORK_SERVICE_TYPE_2_5G_EDGE:
			show_image_icon(LEVEL_EDGE);
			show_connection_transfer_icon(data);
			break;
		case TAPI_NETWORK_SERVICE_TYPE_3G:
			show_image_icon(LEVEL_3G);
			show_connection_transfer_icon(data);
			break;
		case TAPI_NETWORK_SERVICE_TYPE_HSDPA:
			show_image_icon(LEVEL_H_PLUS);
			show_connection_transfer_icon(data);
			break;
		case TAPI_NETWORK_SERVICE_TYPE_LTE:
			show_image_icon(LEVEL_LTE);
			show_connection_transfer_icon(data);
			break;
		default:
			hide_image_icon();
			break;
		}
	}
}



static void on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	DBG("On noti function");
	TelNetworkDefaultDataSubs_t default_subscription = TAPI_NETWORK_DEFAULT_DATA_SUBS_UNKNOWN;
	int ret = 0;
	int val = 0;
	int ps_type = 0;
	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");

	ret = wifi_get_connection_state(&val);
	DBG("WIFI Status : %d", val);
	if (ret == WIFI_ERROR_NONE && val == WIFI_CONNECTION_STATE_CONNECTED) {
		DBG("WIFI connected, so hide connection icon");
		hide_image_icon();
		return;
	}

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &val);
	if (ret == OK && val == TRUE) {
		DBG("FLIGHT MODE ON");
		hide_image_icon();
		return;
	}

	ret = vconf_get_int(VCONFKEY_NETWORK_STATUS, &val);
	if (ret == OK && val == VCONFKEY_NETWORK_BLUETOOTH) {
		DBG("bluetooth tethering On");
		isBTIconShowing = 1;
		show_image_icon(LEVEL_BT_TETHERING);
		util_signal_emit(conn.ad,"indicator.connection.updown.hide","indicator.prog");
		return;
	}
	else if (ret == OK && val != VCONFKEY_NETWORK_BLUETOOTH) {
		DBG("bluetooth tethering Off");
		if(isBTIconShowing == 1)
		{
			isBTIconShowing = 0;
			hide_image_icon();
		}
	}

	default_subscription = ad->tel_info.prefered_data;
	if(default_subscription == TAPI_NETWORK_DEFAULT_DATA_SUBS_UNKNOWN)
	{
		hide_image_icon();
	}
	else if(default_subscription == TAPI_NETWORK_DEFAULT_DATA_SUBS_SIM1)
	{
		int ret = 0;
		ret = vconf_get_int(VCONFKEY_DNET_STATE, &val);
		if (ret == OK) {
			if (val == VCONFKEY_DNET_OFF)
			{
				DBG("CONNECTION DNET Status: %d", val);
				hide_image_icon();
				return;
			}
		}

		val = ad->tel_info.network_service_type;
		ps_type = ad->tel_info.network_ps_type;

		DBG("TAPI_NETWORK_DEFAULT_DATA_SUBS_SIM1 %d",val);
		DBG("TAPI_NETWORK_DEFAULT_DATA_SUBS_SIM1 %d",ps_type);
		_show_proper_icon(val,ps_type, user_data);
	}
	/* FIXME : remove? */
#if 0
	else
	{
		int ret = 0;
//		ret = vconf_get_int(VCONFKEY_DNET_STATE2, &val);
		if (ret == OK) {
			if (val == VCONFKEY_DNET_OFF)
			{
				DBG("CONNECTION DNET Status: %d", val);
				hide_image_icon();
				return;
			}
		}

		val = ad->tel_info[1].network_service_type;
		ps_type = ad->tel_info[1].network_ps_type;

		DBG("TAPI_NETWORK_DEFAULT_DATA_SUBS_SIM2 %d",val);
		DBG("TAPI_NETWORK_DEFAULT_DATA_SUBS_SIM2 %d",ps_type);
		_show_proper_icon(val,ps_type, user_data);
	}
#endif
}



static void indicator_conn_change_cb(keynode_t *node, void *data)
{
	struct appdata* ad = NULL;
	int svc_type = VCONFKEY_TELEPHONY_SVCTYPE_NONE;
	int status = 0;
	int ret = 0;
	int ps_type = VCONFKEY_TELEPHONY_PSTYPE_NONE;

	ret_if(!data);

	ad = (struct appdata*)data;

	if(icon_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		DBG("need to update %d",updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	retif(data == NULL, , "Invalid parameter!");

	/* check wifi status */
	ret = wifi_get_connection_state(&status);
	if (ret == WIFI_ERROR_NONE) {
		INFO("CONNECTION WiFi Status: %d", status);

		if ((status == WIFI_CONNECTION_STATE_CONNECTED))
		{
			int mms_state = 0;
			vconf_get_int(VCONFKEY_DNET_STATE, &mms_state);
			box_update_display(&(ad->win));

			if(mms_state!=VCONFKEY_DNET_SECURE_CONNECTED)
			{
				hide_image_icon();
				return;
			}
		}
	}

	/* get dnet status */
	ret = vconf_get_int(VCONFKEY_DNET_STATE, &status);
	if (ret == OK) {
		if (status == VCONFKEY_DNET_OFF)
		{
			DBG("CONNECTION DNET Status: %d", status);
			hide_image_icon();
		}
		else
		{
			ret = vconf_get_int(VCONFKEY_TELEPHONY_PSTYPE, &ps_type);
			if (ret == OK)
			{
				INFO("Telephony packet service type: %d", ps_type);

				switch (ps_type)
				{

				case VCONFKEY_TELEPHONY_PSTYPE_HSDPA:
				case VCONFKEY_TELEPHONY_PSTYPE_HSUPA:
				case VCONFKEY_TELEPHONY_PSTYPE_HSPA:
					if(util_is_orf())
					{
						show_image_icon(LEVEL_3G_PLUS);
					}
					else
					{
						show_image_icon(LEVEL_H);
					}
					show_connection_transfer_icon(data);
					return;
				case VCONFKEY_TELEPHONY_PSTYPE_NONE:
				default:
					break;
				}
			}

			/* check service type */
			ret = vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &svc_type);
			if (ret == OK) {
				switch (svc_type) {
				case VCONFKEY_TELEPHONY_SVCTYPE_2G:
					/**< Network 2G. Show to LEVEL_2G icon */
				case VCONFKEY_TELEPHONY_SVCTYPE_2_5G:
					/**< Network 2.5G. Show to LEVEL_2G icon  */
					show_image_icon(LEVEL_2G);
					show_connection_transfer_icon(data);
					break;
				case VCONFKEY_TELEPHONY_SVCTYPE_2_5G_EDGE:
					/**< Network EDGE */
					show_image_icon(LEVEL_EDGE);
					show_connection_transfer_icon(data);
					break;
				case VCONFKEY_TELEPHONY_SVCTYPE_3G:
					/**< Network UMTS */
					show_image_icon(LEVEL_3G);
					show_connection_transfer_icon(data);
					break;
				case VCONFKEY_TELEPHONY_SVCTYPE_LTE:
					/**< Network LTE */
					show_image_icon(LEVEL_4G);
					show_connection_transfer_icon(data);
					break;

				default:
					hide_image_icon();
					break;
				}

				return;
			}
		}
	}

//	ret = vconf_get_int(VCONFKEY_DNET_STATE2, &status);
	if (ret == OK) {
		if (status == VCONFKEY_DNET_OFF)
		{
			DBG("CONNECTION DNET Status: %d", status);
			hide_image_icon();
		}
		else
		{

			ret = vconf_get_int(VCONFKEY_TELEPHONY_PSTYPE, &ps_type);
			if (ret == OK)
			{
				switch (ps_type)
				{

				case VCONFKEY_TELEPHONY_PSTYPE_HSDPA:
				case VCONFKEY_TELEPHONY_PSTYPE_HSUPA:
					if(util_is_orf())
					{
						show_image_icon(LEVEL_3G_PLUS);
					}
					else
					{
						show_image_icon(LEVEL_H);
					}
					show_connection_transfer_icon(data);
					return;
				case VCONFKEY_TELEPHONY_PSTYPE_HSPA:
					show_image_icon(LEVEL_H_PLUS);
					show_connection_transfer_icon(data);
					return;

				case VCONFKEY_TELEPHONY_PSTYPE_NONE:
				default:
					break;
				}
			}

			/* check service type */
			ret = vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &svc_type);
			if (ret == OK) {
				switch (svc_type) {
				case VCONFKEY_TELEPHONY_SVCTYPE_2G:
					/**< Network 2G. Show to LEVEL_2G icon */
				case VCONFKEY_TELEPHONY_SVCTYPE_2_5G:
					/**< Network 2.5G. Show to LEVEL_2G icon  */
					show_image_icon(LEVEL_2G);
					show_connection_transfer_icon(data);
					break;
				case VCONFKEY_TELEPHONY_SVCTYPE_2_5G_EDGE:
					/**< Network EDGE */
					show_image_icon(LEVEL_EDGE);
					show_connection_transfer_icon(data);
					break;
				case VCONFKEY_TELEPHONY_SVCTYPE_3G:
					/**< Network UMTS */
					show_image_icon(LEVEL_3G);
					show_connection_transfer_icon(data);
					break;
				case VCONFKEY_TELEPHONY_SVCTYPE_LTE:
					/**< Network LTE */
					show_image_icon(LEVEL_4G);
					show_connection_transfer_icon(data);
					break;

				default:
					hide_image_icon();
					break;
				}

				return;
			}
		}
	}
	hide_image_icon();

	return;
}



static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		return OK;
	}
	on_noti(tapi_handle[0], NULL, NULL, data);
	return OK;
}


#if 0
static void svc_type_callback(keynode_t *node, void *data)
{
	int type = 0;

	vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &type);
	DBG("svc_type_callback %d",type);
	indicator_conn_change_cb(node,data);
}
#endif


static void ps_type_callback(keynode_t *node, void *data)
{
	int type = 0;

	vconf_get_int(VCONFKEY_TELEPHONY_PSTYPE, &type);

	DBG("ps_type_callback %d",type);
	indicator_conn_change_cb(node,data);
}



static void dnet_state_callback(keynode_t *node, void *data)
{
	DBG("dnet_state_callback");
	on_noti(tapi_handle[0], NULL, NULL, data);
}



/*static void dnet2_state_callback(keynode_t *node, void *data)
{
	DBG("dnet_state_callback");
	on_noti(tapi_handle[1], NULL, NULL, data);
}*/



static void packet_state_callback(keynode_t *node, void *data)
{
	DBG("packet_state_callback");
	on_noti(tapi_handle[0], NULL, NULL, data);
}



static void _wifi_status_changed_cb(wifi_connection_state_e state, wifi_ap_h ap, void *user_data)
{
	int status = 0;
	int ret = 0;

	ret = wifi_get_connection_state(&status);
	if (ret == WIFI_ERROR_NONE)
	{
		INFO("[CB] WIFI Status: %d", status);
		if(status == WIFI_CONNECTION_STATE_CONNECTED)
		{
			DBG("[CB] WIFI connected, so hide connection icon");
			hide_image_icon();
		}
		else
		{
			on_noti(tapi_handle[0], NULL, NULL, user_data);
		}
	}
}



static void _flight_mode(keynode_t *key, void *data)
{
	on_noti(tapi_handle[0], NULL, NULL, data);
}



static void _bt_tethering(keynode_t *key, void *data)
{
	on_noti(tapi_handle[0], NULL, NULL, data);
}



void connection_icon_on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	DBG("");
	on_noti(handle_obj, NULL, NULL, user_data);
}



/* Initialize TAPI */
static void __init_tel(void *data)
{
	DBG("__init_tel");
	int ret = FAIL;

	ret = vconf_notify_key_changed(VCONFKEY_DNET_STATE, dnet_state_callback, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
	}

/*	ret = vconf_notify_key_changed(VCONFKEY_DNET_STATE2, dnet2_state_callback, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
	}*/

	ret = vconf_notify_key_changed(VCONFKEY_PACKET_STATE, packet_state_callback, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
	}

	ret = vconf_notify_key_changed(VCONFKEY_NETWORK_STATUS, _bt_tethering, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
	}

	on_noti(tapi_handle[0], NULL, NULL, data);
}



/* De-initialize TAPI */
static void __deinit_tel()
{
	DBG("__deinit_tel");
}



static void tel_ready_cb(keynode_t *key, void *data)
{
	gboolean status = FALSE;

	status = vconf_keynode_get_bool(key);
	if (status == TRUE) {    /* Telephony State - READY */
		__init_tel(data);
	}
	else {                   /* Telephony State – NOT READY */
		/* De-initialization is optional here (ONLY if required) */
		__deinit_tel();
	}
}



static int register_conn_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_PSTYPE,
					   ps_type_callback, data);
	if (ret != OK) {
		r = r | ret;
	}

	ret = wifi_set_connection_state_changed_cb(_wifi_status_changed_cb, data);
	if (ret != WIFI_ERROR_NONE) {
		ERR("Failed to register wifi_set_connection_state_changed_cb!");
		r = r | ret;
	}
	gboolean state = FALSE;

	vconf_get_bool(VCONFKEY_TELEPHONY_READY, &state);

	if(state)
	{
		DBG("Telephony ready");
		__init_tel(data);
	}
	else
	{
		DBG("Telephony not ready");
		vconf_notify_key_changed(VCONFKEY_TELEPHONY_READY, tel_ready_cb, data);
	}

	return r;
}



static int unregister_conn_module(void)
{
	int ret = -1;

	ret = ret | vconf_ignore_key_changed(VCONFKEY_TELEPHONY_PSTYPE, ps_type_callback);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_DNET_STATE, dnet_state_callback);
//	ret = ret | vconf_ignore_key_changed(VCONFKEY_DNET_STATE2, dnet_state_callback);
	ret = ret | wifi_unset_connection_state_changed_cb();
	ret = ret | vconf_ignore_key_changed(VCONFKEY_PACKET_STATE, packet_state_callback);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_TELEPHONY_READY, tel_ready_cb);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_NETWORK_STATUS, _bt_tethering);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	__deinit_tel();

	return ret;
}
