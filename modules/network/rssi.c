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
#include <ITapiNetwork.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "indicator_gui.h"
#include "util.h"
#include "connection/connection.h"
#include "processing/call_divert.h"

#define VCONFKEY_TELEPHONY_PREFERRED_VOICE_SUBSCRIPTION	"db/telephony/dualsim/preferred_voice_subscription"

#define RSSI1_ICON_PRIORITY		INDICATOR_PRIORITY_FIXED2
#define RSSI2_ICON_PRIORITY		INDICATOR_PRIORITY_FIXED3
#define SIMCARD_ICON_PRIORITY	INDICATOR_PRIORITY_FIXED4

#define MODULE_NAME			"RSSI"
#define MODULE_NAME_SIM2		"RSSI2"
#define MODULE_NAME_DUAL_SIM	"RSSI_DS"

#define ICON_NOSIM		_("IDS_COM_BODY_NO_SIM")
#define ICON_SEARCH		_("IDS_COM_BODY_SEARCHING")
#define ICON_NOSVC		_("IDS_CALL_POP_NOSERVICE")

#define TAPI_HANDLE_MAX  2

static int register_rssi_module(void *data);
static int unregister_rssi_module(void);
static int language_changed_cb(void *data);
static int wake_up_cb(void *data);
static void on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data);
static int _get_sim_icon(int sim_number);
static void _flight_mode(keynode_t *key, void *data);
#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj);
#endif

TapiHandle *tapi_handle[TAPI_HANDLE_MAX+1] = {0, };

static int registered = 0;


icon_s rssi_ds = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME_DUAL_SIM,
	.priority = SIMCARD_ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

icon_s rssi = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = RSSI1_ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_rssi_module,
	.fini = unregister_rssi_module,
	.lang_changed = language_changed_cb,
	.wake_up = wake_up_cb,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

icon_s rssi2 = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME_SIM2,
	.priority = RSSI2_ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
#ifdef _SUPPORT_SCREEN_READER
	.tts_enable = EINA_TRUE,
	.access_cb = access_info_cb
#endif
};

enum {
	SIM_1 = 1,
	SIM_2
};

enum {
	LEVEL_RSSI_MIN = 0,
	LEVEL_FLIGHT,
	LEVEL_NOSIM,
	LEVEL_SEARCH,
	LEVEL_NOSVC,
	LEVEL_LIMITED,
	LEVEL_RSSI_SIM1_0,
	LEVEL_RSSI_SIM1_1,
	LEVEL_RSSI_SIM1_2,
	LEVEL_RSSI_SIM1_3,
	LEVEL_RSSI_SIM1_4,
	LEVEL_RSSI_SIM2_0,
	LEVEL_RSSI_SIM2_1,
	LEVEL_RSSI_SIM2_2,
	LEVEL_RSSI_SIM2_3,
	LEVEL_RSSI_SIM2_4,
	LEVEL_RSSI_ROAMING_0,
	LEVEL_RSSI_ROAMING_1,
	LEVEL_RSSI_ROAMING_2,
	LEVEL_RSSI_ROAMING_3,
	LEVEL_RSSI_ROAMING_4,
	LEVEL_RSSI_ROAMING2_0,
	LEVEL_RSSI_ROAMING2_1,
	LEVEL_RSSI_ROAMING2_2,
	LEVEL_RSSI_ROAMING2_3,
	LEVEL_RSSI_ROAMING2_4,
	LEVEL_SIM_ICON_SIM1,
	LEVEL_SIM_ICON_SIM2,
	LEVEL_SIM_ICON_CALL,
	LEVEL_SIM_ICON_SMS,
	LEVEL_SIM_ICON_MMS,
	LEVEL_SIM_ICON_INTERNET,
	LEVEL_SIM_ICON_HOME,
	LEVEL_SIM_ICON_OFFICE,
	LEVEL_SIM_ICON_HEART,
	LEVEL_MAX
};

static int bRoaming = 0;
static int updated_while_lcd_off = 0;
static int prevIndex = -1;
static int prevRoam = -1;
#ifdef _SUPPORT_SCREEN_READER
static int bRssiShown = 0;
#endif
static const char *icon_path[LEVEL_MAX] = {
	[LEVEL_FLIGHT] = "RSSI/B03_RSSI_Flightmode.png",
	[LEVEL_NOSIM] = "RSSI/B03_RSSI_NoSim.png",
	[LEVEL_SEARCH] = "RSSI/B03_RSSI_Searching.png",
	[LEVEL_NOSVC] = "RSSI/B03_RSSI_NoService.png",
	[LEVEL_LIMITED] = "RSSI/B03_Network_LimitedService.png",
	[LEVEL_RSSI_SIM1_0] = "RSSI/B03_RSSI_Sim_00.png",
	[LEVEL_RSSI_SIM1_1] = "RSSI/B03_RSSI_Sim_01.png",
	[LEVEL_RSSI_SIM1_2] = "RSSI/B03_RSSI_Sim_02.png",
	[LEVEL_RSSI_SIM1_3] = "RSSI/B03_RSSI_Sim_03.png",
	[LEVEL_RSSI_SIM1_4] = "RSSI/B03_RSSI_Sim_04.png",
	[LEVEL_RSSI_SIM2_0] = "RSSI/B03_RSSI_Dual_Sim_00.png",
	[LEVEL_RSSI_SIM2_1] = "RSSI/B03_RSSI_Dual_Sim_01.png",
	[LEVEL_RSSI_SIM2_2] = "RSSI/B03_RSSI_Dual_Sim_02.png",
	[LEVEL_RSSI_SIM2_3] = "RSSI/B03_RSSI_Dual_Sim_03.png",
	[LEVEL_RSSI_SIM2_4] = "RSSI/B03_RSSI_Dual_Sim_04.png",
	[LEVEL_RSSI_ROAMING_0] = "RSSI/B03_RSSI_roaming_00.png",
	[LEVEL_RSSI_ROAMING_1] = "RSSI/B03_RSSI_roaming_01.png",
	[LEVEL_RSSI_ROAMING_2] = "RSSI/B03_RSSI_roaming_02.png",
	[LEVEL_RSSI_ROAMING_3] = "RSSI/B03_RSSI_roaming_03.png",
	[LEVEL_RSSI_ROAMING_4] = "RSSI/B03_RSSI_roaming_04.png",
	[LEVEL_RSSI_ROAMING2_0] = "RSSI/B03_RSSI_Dual_Sim_roaming_00.png",
	[LEVEL_RSSI_ROAMING2_1] = "RSSI/B03_RSSI_Dual_Sim_roaming_01.png",
	[LEVEL_RSSI_ROAMING2_2] = "RSSI/B03_RSSI_Dual_Sim_roaming_02.png",
	[LEVEL_RSSI_ROAMING2_3] = "RSSI/B03_RSSI_Dual_Sim_roaming_03.png",
	[LEVEL_RSSI_ROAMING2_4] = "RSSI/B03_RSSI_Dual_Sim_roaming_04.png",
	[LEVEL_SIM_ICON_SIM1] = "Dual SIM/B03_Dual_Sim_00.png",
	[LEVEL_SIM_ICON_SIM2] = "Dual SIM/B03_Dual_Sim_01.png",
	[LEVEL_SIM_ICON_CALL] = "Dual SIM/B03_Dual_Sim_phone.png",
	[LEVEL_SIM_ICON_SMS] = "Dual SIM/B03_Dual_Sim_messages.png",
	[LEVEL_SIM_ICON_MMS] = "Dual SIM/B03_Dual_Sim_data.png",
	[LEVEL_SIM_ICON_INTERNET] = "Dual SIM/B03_Dual_Sim_global.png",
	[LEVEL_SIM_ICON_HOME] = "Dual SIM/B03_Dual_Sim_home.png",
	[LEVEL_SIM_ICON_OFFICE] = "Dual SIM/B03_Dual_Sim_office.png",
	[LEVEL_SIM_ICON_HEART] = "Dual SIM/B03_Dual_Sim_heart.png"
};

static void set_app_state(void* data)
{
	rssi_ds.ad = data;
	rssi.ad = data;
	rssi2.ad = data;
}

static void show_sim_ds_image_icon(void *data, int index)
{
	if (index < LEVEL_RSSI_MIN)
		index = LEVEL_RSSI_MIN;
	else if (index >= LEVEL_MAX)
		index = LEVEL_NOSVC;

	rssi_ds.img_obj.width = DEFAULT_ICON_WIDTH;

	rssi_ds.img_obj.data = icon_path[index];

	icon_show(&rssi_ds);

	prevRoam = bRoaming;
	prevIndex = index;
	isSimShowing = 1;   // It is for judgement to show S1 icon(Sound, Wi-fi direct, bluetooth) or not.
	util_signal_emit(rssi_ds.ad,"indicator.simicon.show","indicator.prog");
}

static void show_image_icon(void *data, int index)
{
	if (index < LEVEL_RSSI_MIN)
		index = LEVEL_RSSI_MIN;
	else if (index >= LEVEL_MAX)
		index = LEVEL_NOSVC;

	rssi.img_obj.width = DEFAULT_ICON_WIDTH;

	rssi.img_obj.data = icon_path[index];

	icon_show(&rssi);

	prevRoam = bRoaming;
	prevIndex = index;
	isRSSI1Showing = 1;
	util_signal_emit(rssi.ad,"indicator.rssi1.show","indicator.prog");
}

static void show_sim2_image_icon(void *data, int index)
{
	if (index < LEVEL_RSSI_MIN)
		index = LEVEL_RSSI_MIN;
	else if (index >= LEVEL_MAX)
		index = LEVEL_NOSVC;

	rssi2.img_obj.width = DEFAULT_ICON_WIDTH;

	rssi2.img_obj.data = icon_path[index];

	icon_show(&rssi2);

	prevRoam = bRoaming;
	prevIndex = index;
	isRSSI2Showing = 1;
	util_signal_emit(rssi2.ad,"indicator.rssi2.show","indicator.prog");
}

static void hide_sim_ds_image_icon(void)
{
	icon_hide(&rssi_ds);

	isSimShowing = 0;    // It is for judgement to show S1 icon(Sound, Wi-fi direct, bluetooth) or not.
	util_signal_emit(rssi_ds.ad,"indicator.simicon.hide","indicator.prog");
}

static void hide_sim_image_icon(void)
{
	icon_hide(&rssi);

	isRSSI1Showing = 0;
	util_signal_emit(rssi.ad,"indicator.rssi1.hide","indicator.prog");
}

static void hide_sim2_image_icon(void)
{
	icon_hide(&rssi2);

	isRSSI2Showing = 0;
	util_signal_emit(rssi2.ad,"indicator.rssi2.hide","indicator.prog");
}

static int language_changed_cb(void *data)
{
	on_noti(NULL, NULL, NULL, data);
	return OK;
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		return OK;
	}

	on_noti(NULL, NULL, NULL, data);
	return OK;
}

#ifdef _SUPPORT_SCREEN_READER
static char *access_info_cb(void *data, Evas_Object *obj)
{
	Evas_Object *item = data;
	char *tmp = NULL;
	char buf[256] = {0,};
	char buf1[256] = {0,};
	int status = 0;

	if(bRssiShown == 0)
	{
		ERR("No rssi level");
		return NULL;
	}

	vconf_get_int(VCONFKEY_TELEPHONY_RSSI, &status);
	snprintf(buf1, sizeof(buf1), _("IDS_IDLE_BODY_PD_OUT_OF_4_BARS_OF_SIGNAL_STRENGTH"), status);

	snprintf(buf, sizeof(buf), "%s, %s", buf1,_("IDS_IDLE_BODY_STATUS_BAR_ITEM"));

	INFO("buf: %s", buf);
	tmp = strdup(buf);
	if (!tmp) return NULL;
	return tmp;
}
#endif

static void on_noti(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	DBG("On noti funtion");
	int sim1_service = 0;
	int sim2_service = 0;
	int status = 0;
	int sim_slot_count = 0;
	int ret = 0;
	int val = 0;
	int preferred_subscription = 0;
	struct appdata *ad = (struct appdata *)user_data;

	if(icon_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		DBG("need to update %d",updated_while_lcd_off);
		return;
	}

	updated_while_lcd_off = 0;

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &status);
	if (ret == OK && status == TRUE) {
		DBG("RSSI Status: Flight Mode");
		show_image_icon(user_data, LEVEL_FLIGHT);
		hide_sim2_image_icon();
		hide_sim_ds_image_icon();
		return;
	}
	ret = vconf_get_int(VCONFKEY_TELEPHONY_SIM_SLOT_COUNT, &sim_slot_count);
	if (ret != OK)
		LOGE("Failed to get dual_sim vconf");
	DBG("Target's Sim Slot COUNT : %d", sim_slot_count);

	TelSimCardStatus_t sim_status_sim1 = 0x00;
	TelSimCardStatus_t sim_status_sim2 = 0x00;

	sim_status_sim1 = ad->tel_info[0].sim_status;
	if(sim_slot_count == 1)
	{
		DBG("Single Sim target");
		sim_status_sim2 = TAPI_SIM_STATUS_CARD_NOT_PRESENT;
	}
	else if(sim_slot_count == 2)
	{
		DBG("Dual Sim target");
		sim_status_sim2 = ad->tel_info[1].sim_status;
	}

	// Sim 2
	if(sim_status_sim2 != TAPI_SIM_STATUS_CARD_NOT_PRESENT)
	{
		DBG("Sim2 inserted");
		val = ad->tel_info[1].network_service_type;
		DBG("Get SIM2 service type : %d", val);
		switch(val)
		{
			case TAPI_NETWORK_SERVICE_TYPE_NO_SERVICE:
				DBG("Service type: NO_SERVICE");
				show_sim2_image_icon(user_data, LEVEL_NOSVC);
				break;
			case TAPI_NETWORK_SERVICE_TYPE_EMERGENCY:
				DBG("Service type: EMERGENCY : Signal-strength level : %d", val);
				val = ad->tel_info[1].signal_level;
				if(ret != OK)
				{
					ERR("Can not get signal strength level");
				}
				show_sim2_image_icon(user_data, LEVEL_RSSI_SIM1_0+val);
				break;
			case TAPI_NETWORK_SERVICE_TYPE_SEARCH:
				DBG("Service type: SEARCH");
				show_sim2_image_icon(user_data, LEVEL_SEARCH);
				break;
			case TAPI_NETWORK_SERVICE_TYPE_UNKNOWN: DBG("Service type : UNKNOWN");
			default:
				sim2_service = 1;
				break;
		}
		if(sim2_service)
		{
			val = ad->tel_info[1].signal_level;
			DBG("Get SIM2 signal strength level: %d", val);
			int roaming = 0;
			roaming = ad->tel_info[1].roaming_status;
			preferred_subscription = ad->tel_info[1].default_network;
			if(roaming)
			{
				if(preferred_subscription == TAPI_NETWORK_DEFAULT_SUBS_SIM2  && sim_status_sim1 == TAPI_SIM_STATUS_SIM_INIT_COMPLETED)
				{
					show_sim2_image_icon(user_data, LEVEL_RSSI_ROAMING2_0+val);
				}
				else
				{
					show_sim2_image_icon(user_data, LEVEL_RSSI_ROAMING_0+val);
				}
			}
			else
			{
				if(preferred_subscription == TAPI_NETWORK_DEFAULT_SUBS_SIM2  && sim_status_sim1 == TAPI_SIM_STATUS_SIM_INIT_COMPLETED)
				{
					show_sim2_image_icon(user_data, LEVEL_RSSI_SIM2_0+val);
				}
				else
				{
					show_sim2_image_icon(user_data, LEVEL_RSSI_SIM1_0+val);
				}
			}
		}
	}

	// SIM 1
	if(sim_status_sim1 != TAPI_SIM_STATUS_CARD_NOT_PRESENT)
	{
		DBG("Sim1 inserted");
		val = ad->tel_info[0].network_service_type;
		DBG("Get SIM1 service type : %d", val);
		switch(val)
		{
			case TAPI_NETWORK_SERVICE_TYPE_NO_SERVICE:
				DBG("Service type: NO_SERVICE");
				show_image_icon(user_data, LEVEL_NOSVC);
				break;
			case TAPI_NETWORK_SERVICE_TYPE_EMERGENCY:
				DBG("Service type: EMERGENCY : Signal-strength level : %d", val);
				val = ad->tel_info[0].signal_level;
				if(ret != OK)
				{
					ERR("Can not get signal strength level");
				}
				show_image_icon(user_data, LEVEL_RSSI_SIM1_0+val);
				break;
			case TAPI_NETWORK_SERVICE_TYPE_SEARCH:
				DBG("Service type: SEARCH");
				show_image_icon(user_data, LEVEL_SEARCH);
				break;
			case TAPI_NETWORK_SERVICE_TYPE_UNKNOWN: DBG("Service type : UNKNOWN");
			default:
				DBG("Service type : UNKNOWN (%d)", val);
				sim1_service = 1;
				break;
		}
		if(sim1_service)
		{
			val = ad->tel_info[0].signal_level;
			DBG("Get SIM1 signal strength level: %d", val);
			int roaming = 0;
			roaming = ad->tel_info[0].roaming_status;
			preferred_subscription = ad->tel_info[0].default_network;
			if(roaming)
			{
				if(preferred_subscription == TAPI_NETWORK_DEFAULT_SUBS_SIM1  && sim_status_sim2 == TAPI_SIM_STATUS_SIM_INIT_COMPLETED)
				{
					show_image_icon(user_data, LEVEL_RSSI_ROAMING2_0+val);
				}
				else
				{
					show_image_icon(user_data, LEVEL_RSSI_ROAMING_0+val);
				}
			}
			else
			{
				if(preferred_subscription == TAPI_NETWORK_DEFAULT_SUBS_SIM1 && sim_status_sim2 == TAPI_SIM_STATUS_SIM_INIT_COMPLETED)
				{
					show_image_icon(user_data, LEVEL_RSSI_SIM2_0+val);
				}
				else
				{
					show_image_icon(user_data, LEVEL_RSSI_SIM1_0+val);
				}
			}

		}
	}

	if(sim_status_sim1 == TAPI_SIM_STATUS_SIM_INIT_COMPLETED && sim_status_sim2 == TAPI_SIM_STATUS_SIM_INIT_COMPLETED)
	{
		DBG("Dual Sim : Check preferred network subscription");
		if(preferred_subscription == TAPI_NETWORK_DEFAULT_SUBS_SIM1)
		{
			DBG("Preferred voice subscription set to sim1 : %d", preferred_subscription);
			show_sim_ds_image_icon(user_data, _get_sim_icon(SIM_1));

		}
		else if(preferred_subscription == TAPI_NETWORK_DEFAULT_SUBS_SIM2)
		{
			DBG("Preferred voice subscription set to sim2 : %d", preferred_subscription);
			show_sim_ds_image_icon(user_data, _get_sim_icon(SIM_2));
		}
		else
		{
			hide_sim_ds_image_icon();
			ERR("Preferred voice subscription is: %d", preferred_subscription);
		}
	}
	else if(sim_status_sim1 == TAPI_SIM_STATUS_SIM_INIT_COMPLETED && sim_status_sim2 != TAPI_SIM_STATUS_SIM_INIT_COMPLETED)
	{
		DBG("Single Sim : Show Sim card 1 icon");
		hide_sim2_image_icon();
		show_sim_ds_image_icon(user_data, _get_sim_icon(SIM_1));
	}
	else if(sim_status_sim1 != TAPI_SIM_STATUS_SIM_INIT_COMPLETED && sim_status_sim2 == TAPI_SIM_STATUS_SIM_INIT_COMPLETED)
	{
		DBG("Single Sim : Show Sim card 2 icon");
		hide_sim_image_icon();
		show_sim_ds_image_icon(user_data, _get_sim_icon(SIM_2));
	}

	if(sim_status_sim1 == TAPI_SIM_STATUS_CARD_NOT_PRESENT &&
		sim_status_sim2 == TAPI_SIM_STATUS_CARD_NOT_PRESENT)
	{
		hide_sim_image_icon();
		hide_sim2_image_icon();
		hide_sim_ds_image_icon();
		val = ad->tel_info[0].signal_level;
		DBG("No sim card : Signal-strength level : %d", val);
		if(ret != OK)
		{
			ERR("Can not get signal strength level");
		}
		show_image_icon(user_data, LEVEL_RSSI_SIM1_0+val);
	}
	else if(sim_slot_count !=2 )
	{
		DBG("Single Sim target");
		hide_sim2_image_icon();
		hide_sim_ds_image_icon();
	}
}

static int _get_sim_icon(int sim_number)
{
	int status = 0;
	int ret = 0;

	if(sim_number == SIM_1)
	{
//		ret = vconf_get_int(VCONFKEY_SETAPPL_SIM1_ICON, &status);
		if (ret == OK)
		{
			LOGD("Sim 1 icon: %d", status);
		}
	}
	else
	{
//		ret = vconf_get_int(VCONFKEY_SETAPPL_SIM2_ICON, &status);
		if (ret == OK)
		{
			LOGD("Sim 2 icon: %d", status);
		}
	}

	return status+LEVEL_SIM_ICON_SIM1;
}

static void _sim_icon_update(keynode_t *key, void *data)
{
	on_noti(NULL, NULL, NULL, data);
}

static void _flight_mode(keynode_t *key, void *data)
{
	on_noti(NULL, NULL, NULL, data);
}

int rssi_get_sim_number(TapiHandle *handle_obj)
{
	DBG("tapi_handle[0](%x),tapi_handle[1](%x),current(%x)",tapi_handle[0],tapi_handle[1],handle_obj);

	int ret = -1;
	if(tapi_handle[0]!=NULL)
	{
		if(tapi_handle[0]==handle_obj)
			ret = 0;
	}

	if(tapi_handle[1]!=NULL)
	{
		if(tapi_handle[1]==handle_obj)
			ret = 1;
	}

	if(ret == -1)
	{
		ERR("no handle %x",handle_obj);
		ret = 0;
	}

	DBG("sim %d",ret);

	return ret;
}
static void init_tel_info(void* data)
{
	int ret = 0;
	struct appdata *ad = (struct appdata *)data;
	int i = 0;
	int val = 0;
	TelSimCardStatus_t sim_status;
	retif(data == NULL, , "invalid parameter !!");

	for(i=0;i<2;i++)
	{
		if(tapi_handle[i]==NULL)
		{
			DBG("i %d,NULL");
			continue;
		}
		DBG("tapi handle %x %d",tapi_handle[i],i);
		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_ROAMING_STATUS, &val);
		if(ret != OK)
		{
			ERR("Can not get %s",TAPI_PROP_NETWORK_ROAMING_STATUS);
		}
		DBG("TAPI_PROP_NETWORK_ROAMING_STATUS %d",val);
		ad->tel_info[i].roaming_status = val;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL, &val);
		if(ret != OK)
		{
			ERR("Can not get %s",TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL);
		}
		DBG("TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL %d",val);
		ad->tel_info[i].signal_level = val;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_SERVICE_TYPE, &val);
		if(ret != OK)
		{
			ERR("Can not get %s",TAPI_PROP_NETWORK_SERVICE_TYPE);
		}
		DBG("TAPI_PROP_NETWORK_SERVICE_TYPE %d",val);
		ad->tel_info[i].network_service_type = val;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_NETWORK_PS_TYPE, &val);
		if(ret != OK)
		{
			ERR("Can not get %s",TAPI_PROP_NETWORK_SERVICE_TYPE);
		}
		DBG("TAPI_PROP_NETWORK_PS_TYPE %d",val);
		ad->tel_info[i].network_ps_type = val;


		TelCallPreferredVoiceSubs_t preferred_sub = TAPI_CALL_PREFERRED_VOICE_SUBS_UNKNOWN;
		ret = tel_get_call_preferred_voice_subscription(tapi_handle[i], &preferred_sub);
		if(ret != OK)
		{
			ERR("Can not get %s",TAPI_NOTI_CALL_PREFERRED_VOICE_SUBSCRIPTION);
		}
		DBG("TAPI_NOTI_CALL_PREFERRED_VOICE_SUBSCRIPTION %d",preferred_sub);
		ad->tel_info[i].prefered_voice = preferred_sub;
#ifdef DEVICE_BUILD
		TelNetworkDefaultDataSubs_t preferred_data = TAPI_NETWORK_DEFAULT_DATA_SUBS_UNKNOWN;
		ret =  tel_get_network_default_data_subscription(tapi_handle[i], &preferred_data);
		if(ret != OK)
		{
			ERR("Can not get %s",TAPI_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION);
		}
		DBG("TAPI_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION %d",preferred_data);
		ad->tel_info[i].prefered_data = preferred_data;

		TelNetworkDefaultSubs_t default_subs = TAPI_NETWORK_DEFAULT_SUBS_UNKNOWN;
		ret =  tel_get_network_default_subscription(tapi_handle[i], &default_subs);
		if(ret != OK)
		{
			ERR("Can not get %s",TAPI_NOTI_NETWORK_DEFAULT_SUBSCRIPTION);
		}
		DBG("TAPI_NOTI_NETWORK_DEFAULT_SUBSCRIPTION %d",default_subs);

		ad->tel_info[i].default_network = default_subs;

		ret = tel_get_property_int(tapi_handle[i], TAPI_PROP_SIM_CALL_FORWARD_STATE, &val);
		if(ret != OK)
		{
			ERR("Can not get %s",TAPI_PROP_SIM_CALL_FORWARD_STATE);
		}
		ad->tel_info[i].call_forward = val;
		DBG("TAPI_PROP_SIM_CALL_FORWARD_STATE %d",val);
#endif // DEVICE_BUILD

		int changed = 0;
		ret = tel_get_sim_init_info(tapi_handle[i], &sim_status, &changed);
		if(ret != OK)
		{
			ERR("Can not get sim init info");
		}
		DBG("sim info %d",sim_status);
		ad->tel_info[i].sim_status = sim_status;
	}
}
static void signal_strength_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *sig_level = data;
	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	sim_number = rssi_get_sim_number(handle_obj);
	DBG ("sim %d",sim_number);
	DBG ("property(%s) receive !!", TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL);
	DBG (" - sig_level = %d", *sig_level);

	ad->tel_info[sim_number].signal_level= *sig_level;

	on_noti(handle_obj,noti_id,data,user_data);
}

static void sim_status_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *sim_status = data;

	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	sim_number = rssi_get_sim_number(handle_obj);
	ERR ("sim %d",sim_number);

	ERR ("property(%s) receive !!", TAPI_NOTI_SIM_STATUS);
	ERR (" - sim_status = %d", *sim_status);

	ad->tel_info[sim_number].sim_status= *sim_status;

	on_noti(handle_obj,noti_id,data,user_data);
	connection_icon_on_noti(handle_obj,noti_id,data,user_data);
}

static void network_service_type_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *service_type = data;

	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	sim_number = rssi_get_sim_number(handle_obj);
	DBG ("sim %d",sim_number);

	DBG ("property(%s) receive !!", TAPI_PROP_NETWORK_SERVICE_TYPE);
	DBG (" - service_type = %d", *service_type);

	ad->tel_info[sim_number].network_service_type= *service_type;

	on_noti(handle_obj,noti_id,data,user_data);
	connection_icon_on_noti(handle_obj,noti_id,data,user_data);
}

static void network_ps_type_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *ps_type = data;

	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	sim_number = rssi_get_sim_number(handle_obj);
	DBG ("sim %d",sim_number);

	DBG ("property(%s) receive !!", TAPI_PROP_NETWORK_PS_TYPE);
	DBG (" - ps_type = %d", *ps_type);

	ad->tel_info[sim_number].network_ps_type= *ps_type;

	connection_icon_on_noti(handle_obj,noti_id,data,user_data);
}

static void roaming_status_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *roaming_status = data;

	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	sim_number = rssi_get_sim_number(handle_obj);
	DBG ("sim %d",sim_number);

	DBG ("property(%s) receive !!", TAPI_PROP_NETWORK_ROAMING_STATUS);
	DBG (" - roaming_status = %d", *roaming_status);

	ad->tel_info[sim_number].roaming_status= *roaming_status;

	on_noti(handle_obj,noti_id,data,user_data);
}


#if 0
static void preferred_voice_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *prefered_voice = data;

	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	sim_number = rssi_get_sim_number(handle_obj);
	DBG ("sim %d",sim_number);

	DBG ("property(%s) receive !!", TAPI_NOTI_CALL_PREFERRED_VOICE_SUBSCRIPTION);
	DBG (" - preferred_voice = %d", *prefered_voice);

	ad->tel_info[sim_number].prefered_voice = *prefered_voice;

	on_noti(handle_obj,noti_id,data,user_data);
}
#endif


static void preferred_data_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	struct tel_noti_network_default_data_subs *noti = data;
	sim_number = rssi_get_sim_number(handle_obj);
	DBG ("sim %d",sim_number);

	DBG ("property(%s) receive !!", TAPI_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION);
	DBG (" - preferred_data = %d", noti->default_subs);

	//Data preferred calback comes from only the sim handle which changed
	//SIM1->SIM2 , callback comes only for SIM2 handle
	ad->tel_info[0].prefered_data = noti->default_subs;
	ad->tel_info[1].prefered_data = noti->default_subs;
	//data prefered is not controllered

	connection_icon_on_noti(handle_obj,noti_id,data,user_data);
}

static void default_network_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	struct tel_noti_network_default_subs *noti = data;
	sim_number = rssi_get_sim_number(handle_obj);
	DBG ("sim %d",sim_number);

	DBG ("property(%s) receive !!", TAPI_NOTI_NETWORK_DEFAULT_SUBSCRIPTION);
	DBG (" - default_network = %d", noti->default_subs);

	ad->tel_info[sim_number].default_network = noti->default_subs;

	on_noti(handle_obj,noti_id,data,user_data);
}

static void call_forward_cb(TapiHandle *handle_obj, const char *noti_id, void *data, void *user_data)
{
	int *call_forward = data;

	struct appdata *ad = (struct appdata *)user_data;
	retif(user_data == NULL, , "invalid parameter!!");
	int sim_number = 0;
	sim_number = rssi_get_sim_number(handle_obj);

	DBG ("sim %d",sim_number);
#ifdef DEVICE_BUILD
	DBG ("property(%s) receive !!", TAPI_PROP_SIM_CALL_FORWARD_STATE);
#endif // DEVICE_BUILD
	DBG (" - call_forward = %d", *call_forward);

	ad->tel_info[sim_number].call_forward = *call_forward;

	call_forward_on_noti(handle_obj,noti_id,data,user_data);
}


/* Initialize TAPI */
static void __init_tel(void *data)
{
	DBG("__init_tel");
	char **cp_list = NULL;
	unsigned int modem_num = 0;

	if(registered ==1)
	{
		ERR("already registered");
		return;
	}

	cp_list = tel_get_cp_name_list();
	retif(cp_list == NULL, , "tel_get_cp_name_list() return NULL !!");
	while (cp_list[modem_num]) {
		tapi_handle[modem_num] = tel_init(cp_list[modem_num]);
		if(!tapi_handle[modem_num])
		{
			ERR("tapi_handle[%d] is null !!", modem_num);
		}
		else
		{
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL, signal_strength_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_NOTI_SIM_STATUS, sim_status_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_NETWORK_SERVICE_TYPE, network_service_type_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_NETWORK_PS_TYPE, network_ps_type_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_NETWORK_ROAMING_STATUS, roaming_status_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION, preferred_data_cb, data);
			tel_register_noti_event(tapi_handle[modem_num], TAPI_NOTI_NETWORK_DEFAULT_SUBSCRIPTION, default_network_cb, data);
#ifdef DEVICE_BUILD
			tel_register_noti_event(tapi_handle[modem_num], TAPI_PROP_SIM_CALL_FORWARD_STATE , call_forward_cb, data);
#endif // DEVICE_BUILD
		}
		modem_num++;
	}
	DBG("Model num: %d", modem_num);
	tapi_handle[modem_num] = NULL;
	g_strfreev(cp_list);
	init_tel_info(data);
	on_noti(NULL, NULL, NULL, data);

	registered = 1;
}

/* De-initialize TAPI */
static void __deinit_tel()
{
	DBG("__deinit_tel");
	unsigned int i = 0;

	while (tapi_handle[i]) {
		/* De-initialize TAPI handle */
		if(tapi_handle[i])
		{
			tel_deregister_noti_event(tapi_handle[i], TAPI_PROP_NETWORK_SIGNALSTRENGTH_LEVEL);
			tel_deregister_noti_event(tapi_handle[i], TAPI_NOTI_SIM_STATUS);
			tel_deregister_noti_event(tapi_handle[i], TAPI_PROP_NETWORK_SERVICE_TYPE);
			tel_deregister_noti_event(tapi_handle[i], TAPI_PROP_NETWORK_ROAMING_STATUS);
			tel_deregister_noti_event(tapi_handle[i], TAPI_NOTI_NETWORK_DEFAULT_DATA_SUBSCRIPTION);
			tel_deregister_noti_event(tapi_handle[i], TAPI_NOTI_NETWORK_DEFAULT_SUBSCRIPTION);
#ifdef DEVICE_BUILD
			tel_deregister_noti_event(tapi_handle[i], TAPI_PROP_SIM_CALL_FORWARD_STATE);
#endif // DEVICE_BUILD
		}
		tel_deinit(tapi_handle[i]);
		tapi_handle[i] = NULL;

		/* Move to next handle */
		i++;
	}
	registered = 0;
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

static int register_rssi_module(void *data)
{
	int r = 0;

	gboolean state = FALSE;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	DBG("RSSI initialization");
	vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE, _flight_mode, data);
//	vconf_notify_key_changed(VCONFKEY_SETAPPL_SIM1_ICON, _sim_icon_update, data);
//	vconf_notify_key_changed(VCONFKEY_SETAPPL_SIM2_ICON, _sim_icon_update, data);

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

	r = OK;
	return r;
}

static int unregister_rssi_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_READY,
					   tel_ready_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE,
					   _flight_mode);
	if (ret != OK)
		ERR("Failed to unregister callback!");
#if 0
	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_SIM1_ICON, _sim_icon_update);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_SIM2_ICON, _sim_icon_update);
	if (ret != OK)
		ERR("Failed to unregister callback!");
#endif
	__deinit_tel();
	ret = OK;
	return ret;
}
