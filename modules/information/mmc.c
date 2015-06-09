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
#include "main.h"
#include "modules.h"
#include "icon.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_SYSTEM_3
#define MODULE_NAME		"mmc"
#define TIMER_INTERVAL	0.3

static int register_mmc_module(void *data);
static int unregister_mmc_module(void);
static int wake_up_cb(void *data);

icon_s mmc = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_SYSTEM,
	.init = register_mmc_module,
	.fini = unregister_mmc_module,
	.wake_up = wake_up_cb
};

static const char *icon_path[] = {
	"Storage/B03_storage_t_flash.png",
	NULL
};
static int updated_while_lcd_off = 0;
static int bShown = 0;



static void set_app_state(void* data)
{
	mmc.ad = data;
}



static void show_image_icon(void)
{
	if(bShown == 1)
	{
		return;
	}

	mmc.img_obj.data = icon_path[0];
	icon_show(&mmc);

	bShown = 1;
}



static void hide_image_icon(void)
{
	icon_hide(&mmc);

	bShown = 0;
}



static void indicator_mmc_change_cb(keynode_t *node, void *data)
{
	int status = 0, mmc_status = 0;
	int ret;

	retif(data == NULL, , "Invalid parameter!");
	DBG("indicator_mmc_change_cb");
	if(icon_get_update_flag()==0)
	{
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;

	ret = vconf_get_int(VCONFKEY_FILEMANAGER_MMC_STATUS, &status);
	if (ret != OK) {
		return;
	}

	switch (status) {
	case VCONFKEY_FILEMANAGER_MMC_LOADING:
		DBG("MMC loading");
		ret = vconf_get_int(VCONFKEY_SYSMAN_MMC_STATUS, &mmc_status);
		if (ret != OK) {
			return;
		}
		if(mmc_status == VCONFKEY_SYSMAN_MMC_MOUNTED)
		{
			DBG("Mounting");
			show_image_icon();
		}
		else
		{
			DBG("Unmounting");
		}
		break;
	default:
		hide_image_icon();
		break;
	}
}



static void indicator_mmc_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	retif(data == NULL, , "Invalid parameter!");

	if (vconf_get_int(VCONFKEY_PM_STATE, &status) < 0)
	{
		ERR("Error getting VCONFKEY_PM_STATE value");
		return;
	}

	if(status == VCONFKEY_PM_STATE_LCDOFF)
	{
		int sos_status = 0;
		if (vconf_get_int(VCONFKEY_FILEMANAGER_MMC_STATUS, &sos_status) < 0)
		{
			ERR("Error getting VCONFKEY_FILEMANAGER_MMC_STATUS value");
			return;
		}

		switch (sos_status) {
		case VCONFKEY_FILEMANAGER_MMC_LOADING:
			break;
		default:
			break;
		}
	}
}



static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0 )
	{
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

	ret = vconf_notify_key_changed(VCONFKEY_FILEMANAGER_MMC_STATUS,
					indicator_mmc_change_cb, data);

	ret = ret | vconf_notify_key_changed(VCONFKEY_PM_STATE,
						indicator_mmc_pm_state_change_cb, data);

	indicator_mmc_change_cb(NULL, data);

	return ret;
}



static int unregister_mmc_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_FILEMANAGER_MMC_STATUS,
					indicator_mmc_change_cb);

	ret = ret | vconf_ignore_key_changed(VCONFKEY_PM_STATE,
						indicator_mmc_pm_state_change_cb);

	return ret;
}
