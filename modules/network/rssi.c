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
#include "indicator_gui.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED3
#define MODULE_NAME		"RSSI"

#define ICON_NOSIM		_("IDS_COM_BODY_NO_SIM")
#define ICON_SEARCH		_("IDS_COM_BODY_SEARCHING")
#define ICON_NOSVC		_("IDS_CALL_POP_NOSERVICE")

static int register_rssi_module(void *data);
static int unregister_rssi_module(void);
static int hib_enter_rssi_module(void);
static int hib_leave_rssi_module(void *data);
static int language_changed_cb(void *data);
static int wake_up_cb(void *data);

Indicator_Icon_Object rssi[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_rssi_module,
	.fini = unregister_rssi_module,
	.hib_enter = hib_enter_rssi_module,
	.hib_leave = hib_leave_rssi_module,
	.lang_changed = language_changed_cb,
	.wake_up = wake_up_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.init = register_rssi_module,
	.fini = unregister_rssi_module,
	.hib_enter = hib_enter_rssi_module,
	.hib_leave = hib_leave_rssi_module,
	.lang_changed = language_changed_cb,
	.wake_up = wake_up_cb
}
};

enum {
	LEVEL_RSSI_MIN = 0,
	LEVEL_RSSI_0 = LEVEL_RSSI_MIN,
	LEVEL_RSSI_1,
	LEVEL_RSSI_2,
	LEVEL_RSSI_3,
	LEVEL_RSSI_4,
	LEVEL_RSSI_MAX = LEVEL_RSSI_4,
	LEVEL_FLIGHT,
	LEVEL_NOSIM,
	LEVEL_SEARCH,
	LEVEL_NOSVC,
	LEVEL_MAX,
};

static int bRoaming = 0;
static int updated_while_lcd_off = 0;

static const char *icon_path[LEVEL_MAX] = {
	[LEVEL_RSSI_0] = "RSSI/B03_RSSI_Sim_00.png",
	[LEVEL_RSSI_1] = "RSSI/B03_RSSI_Sim_01.png",
	[LEVEL_RSSI_2] = "RSSI/B03_RSSI_Sim_02.png",
	[LEVEL_RSSI_3] = "RSSI/B03_RSSI_Sim_03.png",
	[LEVEL_RSSI_4] = "RSSI/B03_RSSI_Sim_04.png",
	[LEVEL_FLIGHT] = "RSSI/B03_RSSI_Flightmode.png",
	[LEVEL_NOSIM] = "RSSI/B03_RSSI_NoSim.png",
	[LEVEL_SEARCH] = "RSSI/B03_RSSI_Searching.png",
	[LEVEL_NOSVC] = "RSSI/B03_RSSI_NoService.png",
};

static const char *roaming_icon_path[LEVEL_MAX] = {
	[LEVEL_RSSI_0] = "RSSI/B03_RSSI_roaming_00.png",
	[LEVEL_RSSI_1] = "RSSI/B03_RSSI_roaming_01.png",
	[LEVEL_RSSI_2] = "RSSI/B03_RSSI_roaming_02.png",
	[LEVEL_RSSI_3] = "RSSI/B03_RSSI_roaming_03.png",
	[LEVEL_RSSI_4] = "RSSI/B03_RSSI_roaming_04.png",
	[LEVEL_FLIGHT] = "RSSI/B03_RSSI_Flightmode.png",
	[LEVEL_NOSIM] = "RSSI/B03_RSSI_NoSim.png",
	[LEVEL_SEARCH] = "RSSI/B03_RSSI_Searching.png",
	[LEVEL_NOSVC] = "RSSI/B03_RSSI_NoService.png",
};


static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		rssi[i].ad = data;
	}
}

static int level_check(int *level)
{
	if (*level < LEVEL_RSSI_MIN) {
		*level = LEVEL_RSSI_MIN;
		return -1;
	} else if (*level > LEVEL_RSSI_MAX) {
		*level = LEVEL_RSSI_MAX;
		return 1;
	}
	return 0;
}

static void show_image_icon(void *data, int index)
{
	int i = 0;

	if (index < LEVEL_RSSI_MIN)
		index = LEVEL_RSSI_MIN;
	else if (index >= LEVEL_MAX)
		index = LEVEL_NOSVC;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		rssi[i].img_obj.width = DEFAULT_ICON_WIDTH;

		if(bRoaming == 1)
		{
			rssi[i].img_obj.data = roaming_icon_path[index];
		}
		else
		{
			rssi[i].img_obj.data = icon_path[index];
		}

		indicator_util_icon_show(&rssi[i]);
	}
}

static void hide_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&rssi[i]);
	}
}

static void indicator_rssi_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	if(indicator_util_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		DBG("need to update %d",updated_while_lcd_off);
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &status);
	if (ret == OK && status == TRUE) {
		INFO("RSSI Status: Flight Mode");
		show_image_icon(data, LEVEL_FLIGHT);
		return;
	}

	ret = vconf_get_int(VCONFKEY_TELEPHONY_SIM_SLOT, &status);
	if (ret == OK && status != VCONFKEY_TELEPHONY_SIM_INSERTED) {
		INFO("RSSI Status: No SIM Mode");
		show_image_icon(data, LEVEL_NOSIM);
		return;
	}

	ret = vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &status);
	if (ret == OK) {
		if (status == VCONFKEY_TELEPHONY_SVCTYPE_NOSVC) {
			INFO("RSSI Status: No Service");
			show_image_icon(data, LEVEL_NOSVC);
			return;
		}
		if (status == VCONFKEY_TELEPHONY_SVCTYPE_SEARCH) {
			INFO("RSSI Status: Searching Service");
			show_image_icon(data, LEVEL_SEARCH);
			return;
		}
	}

	ret = vconf_get_int(VCONFKEY_TELEPHONY_SVC_ROAM, &status);
	if (ret == OK) {
		if (status == VCONFKEY_TELEPHONY_SVC_ROAM_ON) {
			INFO("ROAMING Status: %d", status);
			bRoaming = 1;
		} else {
			bRoaming = 0;
		}
	}

	ret = vconf_get_int(VCONFKEY_TELEPHONY_RSSI, &status);
	if (ret == OK) {
		INFO("RSSI Level: %d", status);
		level_check(&status);
		show_image_icon(data, status);
		return;
	}


	ERR("Failed to get rssi status! Set as No Service.");
	show_image_icon(data, LEVEL_NOSVC);
	return;
}

static int language_changed_cb(void *data)
{
	indicator_rssi_change_cb(NULL, data);
	return OK;
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0)
	{
		DBG("ICON WAS NOT UPDATED");
		return OK;
	}

	indicator_rssi_change_cb(NULL, data);
	return OK;
}


static int register_rssi_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_RSSI,
				       indicator_rssi_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				       indicator_rssi_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVCTYPE,
				       indicator_rssi_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE,
				       indicator_rssi_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM,
				       indicator_rssi_change_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = ret;
	}

	indicator_rssi_change_cb(NULL, data);

	return r;
}

static int unregister_rssi_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_RSSI,
				       indicator_rssi_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				       indicator_rssi_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SVCTYPE,
				       indicator_rssi_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE,
				       indicator_rssi_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM,
				       indicator_rssi_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}

static int hib_enter_rssi_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE,
				       indicator_rssi_change_cb);
	if (ret != OK) {
		ERR("Failed to unregister callback!");
	}

	return OK;
}

static int hib_leave_rssi_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE,
				       indicator_rssi_change_cb, data);
	retif(ret != OK, FAIL, "Failed to register callback!");

	indicator_rssi_change_cb(NULL, data);
	return OK;
}
