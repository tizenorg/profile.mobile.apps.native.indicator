/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
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
#include "indicator_ui.h"
#include "modules.h"
#include "indicator_icon_util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"mmc"
#define TIMER_INTERVAL	0.3

static int register_mmc_module(void *data);
static int unregister_mmc_module(void);
static int wake_up_cb(void *data);

Indicator_Icon_Object mmc[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_mmc_module,
	.fini = unregister_mmc_module,
	.wake_up = wake_up_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_mmc_module,
	.fini = unregister_mmc_module,
	.wake_up = wake_up_cb
}

};

static const char *icon_path[] = {
	"Background playing/B03_Memorycard.png",
	NULL
};
static int updated_while_lcd_off = 0;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		mmc[i].ad = data;
	}
}

static void show_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		mmc[i].img_obj.data = icon_path[0];
		indicator_util_icon_show(&mmc[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&mmc[i]);
	}
}

static void icon_animation_set(enum indicator_icon_ani type)
{
	int i = 0;
	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_animation_set(&mmc[i], type);
	}
}

static void indicator_mmc_change_cb(keynode_t *node, void *data)
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

	ret = vconf_get_int(VCONFKEY_FILEMANAGER_DB_STATUS, &status);
	if (ret == FAIL) {
		ERR("Failed to get VCONFKEY_MMC_STATE!");
		return;
	}

	switch (status) {
	case VCONFKEY_FILEMANAGER_DB_UPDATING:
		INFO("MMC loading");
		show_image_icon();
		icon_animation_set(ICON_ANI_BLINK);
		break;

	case VCONFKEY_FILEMANAGER_DB_UPDATED:
	default:
		hide_image_icon();
		break;
	}
}

static void indicator_mmc_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret = 0;
	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_PM_STATE, &status);

	if(status == VCONFKEY_PM_STATE_LCDOFF)
	{
		int sos_status = 0;
		ret = vconf_get_int(VCONFKEY_FILEMANAGER_DB_STATUS, &sos_status);
		if (ret < 0)
			ERR("fail to get [%s]", VCONFKEY_FILEMANAGER_DB_STATUS);

		INFO("mmc STATUS: %d", sos_status);
		switch (sos_status) {
		case VCONFKEY_FILEMANAGER_DB_UPDATING:
			icon_animation_set(ICON_ANI_NONE);
			break;
		default:
			break;
		}
	}
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0 && mmc[0].obj_exist == EINA_FALSE)
	{
		DBG("ICON WAS NOT UPDATED");
		return OK;
	}

	indicator_mmc_change_cb(NULL, data);
	return OK;
}

static int register_mmc_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_FILEMANAGER_DB_STATUS,
				       indicator_mmc_change_cb, data);
	if (ret != OK)
		ERR("Failed to register mmcback!");

	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE,
					       indicator_mmc_pm_state_change_cb, data);
	if (ret != OK)
		ERR("Failed to register callback! : VCONFKEY_PM_STATE");


	indicator_mmc_change_cb(NULL, data);

	return ret;
}

static int unregister_mmc_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_FILEMANAGER_DB_STATUS,
				       indicator_mmc_change_cb);
	if (ret != OK)
		ERR("Failed to unregister mmcback!");


	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE,
					       indicator_mmc_pm_state_change_cb);
	if (ret != OK)
		ERR("Failed to unregister callback!");

	return OK;
}
