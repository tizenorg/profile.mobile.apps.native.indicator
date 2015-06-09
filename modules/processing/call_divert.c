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


#include <tapi_common.h>
#include <TelNetwork.h>
#include <TelSim.h>
#include <ITapiSim.h>
#include <TelCall.h>
#include <ITapiCall.h>
#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>
#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_2
#define MODULE_NAME		"call_divert"
#define MODULE_NAME_SIM2		"call_divert2"

#define TAPI_HANDLE_MAX  2

static int register_call_divert_module(void *data);
static int unregister_call_divert_module(void);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif

static void on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data);

icon_s call_divert = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_call_divert_module,
	.fini = unregister_call_divert_module,
	.area = INDICATOR_ICON_AREA_SYSTEM,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

icon_s call_divert2 = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME_SIM2,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

/*FIXME*/
enum {
	SIM_ICON_SIM1 = 0,
	SIM_ICON_SIM2,
	SIM_ICON_CALL,
	SIM_ICON_SMS,
	SIM_ICON_MMS,
	SIM_ICON_INTERNET,
	SIM_ICON_HOME,
	SIM_ICON_OFFICE,
	SIM_ICON_HEART,
	SIM_ICON_MAX
};

static const char *icon_path[SIM_ICON_MAX] = {
	[SIM_ICON_SIM1] = "Processing/B03_Call_divert_Sim_1.png",
	[SIM_ICON_SIM2] = "Processing/B03_Call_divert_Sim_2.png",
	[SIM_ICON_CALL] = "Processing/B03_Call_divert_Sim_phone.png",
	[SIM_ICON_SMS] = "Processing/B03_Call_divert_Sim_SMS.png",
	[SIM_ICON_MMS] = "Processing/B03_Call_divert_Sim_MMS.png",
	[SIM_ICON_INTERNET] = "Processing/B03_Call_divert_Sim_global.png",
	[SIM_ICON_HOME] = "Processing/B03_Call_divert_Sim_home.png",
	[SIM_ICON_OFFICE] = "Processing/B03_Call_divert_Sim_office.png",
	[SIM_ICON_HEART] = "Processing/B03_Call_divert_Sim_heart.png",
};

enum {
	SIM_1 = 1,
	SIM_2
};



static void set_app_state(void* data)
{
	call_divert.ad = data;
	call_divert2.ad = data;
}



static void show_image_icon(void *data, int icon)
{
	call_divert.img_obj.data = icon_path[icon];
	icon_show(&call_divert);
}



static void hide_image_icon(void)
{
	icon_hide(&call_divert);
}



static void show_sim2_image_icon(void *data, int icon)
{
	call_divert2.img_obj.data = icon_path[icon];
	icon_show(&call_divert2);
}



static void hide_sim2_image_icon(void)
{
	icon_hide(&call_divert2);
}



#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	char *tmp = NULL;
	char buf[256] = {0,};
	int status = 0;

	switch (status)
	{
	case VCONFKEY_TELEPHONY_CALL_FORWARD_ON:
		snprintf(buf, sizeof(buf), "%s, %s, %s",_("IDS_CST_BODY_CALL_FORWARDING"),_("IDS_IDLE_BODY_ICON"),_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));
		break;
	default:
		break;
	}

	tmp = strdup(buf);
	if (!tmp) return NULL;
	return tmp;
}
#endif

static int _get_sim_icon(int sim_number)
{
	int status = 0;
//	int ret = 0;

	if(sim_number == SIM_1)
	{
	/*	ret = vconf_get_int(VCONFKEY_SETAPPL_SIM1_ICON, &status);
		if (ret == OK)
		{
			LOGD("Sim 1 icon: %d", status);
		}*/
	}
	else
	{
	/*	ret = vconf_get_int(VCONFKEY_SETAPPL_SIM2_ICON, &status);
		if (ret == OK)
		{
			LOGD("Sim 2 icon: %d", status);
		}*/
	}

	return status;
}

static void on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	DBG("on_noti");
	int status = 0;
	int ret = 0;
	int ret_for_sim1 = 0;
	int ret_for_sim2 = 0;
	int icon = 0;
	retif(user_data == NULL, , "invalid parameter!!");

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &status);
	if (ret == OK && status == TRUE) {
		DBG("Flight Mode");
		hide_sim2_image_icon();
		hide_image_icon();
		return;
	}

	if(ret_for_sim1 == VCONFKEY_TELEPHONY_CALL_FORWARD_ON)
	{
		DBG("Show call_farward 1 icon.");
		icon = _get_sim_icon(SIM_1);
		show_image_icon(data, icon);
	}
	else    // VCONFKEY_TELEPHONY_CALL_FORWARD_OFF
	{
		DBG("Hide call_farward 1 icon.");
		hide_image_icon();
	}

	if(ret_for_sim2 == VCONFKEY_TELEPHONY_CALL_FORWARD_ON)
	{
		DBG("Show call_farward 2 icon.");
		icon = _get_sim_icon(SIM_2);
		show_sim2_image_icon(data, icon);
	}
	else    // VCONFKEY_TELEPHONY_CALL_FORWARD_OFF
	{
		DBG("Hide call_farward 2 icon.");
		hide_sim2_image_icon();
	}
}

void call_forward_on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	DBG("");
	on_noti(NULL, NULL, NULL, user_data);

}

/* Initialize TAPI */
static void __init_tel(void *data)
{
	on_noti(NULL, NULL, NULL, data);
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

static void _flight_mode(keynode_t *key, void *data)
{
	on_noti(NULL, NULL, NULL, data);
}
#if 0
static void _sim_icon_update(keynode_t *key, void *data)
{
	on_noti(NULL, NULL, NULL, data);
}
#endif
static int register_call_divert_module(void *data)
{
	int ret = 0;

	retif(data == NULL, FAIL, "Invalid parameter!");

	gboolean state = FALSE;

	set_app_state(data);
	vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode, data);

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

	return ret;
}

static int unregister_call_divert_module(void)
{
	int ret = 0;

	__deinit_tel();

	vconf_ignore_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode);
	//vconf_ignore_key_changed(VCONFKEY_SETAPPL_SIM1_ICON, _sim_icon_update);
	//vconf_ignore_key_changed(VCONFKEY_SETAPPL_SIM2_ICON, _sim_icon_update);

	return ret;
}
