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

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED3
#define MODULE_NAME		"ROAMING"

static int register_roaming_module(void *data);
static int unregister_roaming_module(void);

icon_s roaming = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.init = register_roaming_module,
	.fini = unregister_roaming_module
};

static const char *icon_path[] = {
	"RSSI/B03_Roaming.png",
	NULL
};

static void set_app_state(void* data)
{
	roaming.ad = data;
}

static void show_image_icon(void)
{
	roaming.img_obj.data = icon_path[0];
	icon_show(&roaming);
}

static void hide_image_icon(void)
{
	icon_hide(&roaming);
}

static void indicator_roaming_change_cb(keynode_t *node, void *data)
{
	int status = 0;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	/* First, check NOSIM mode */
	ret = vconf_get_int(VCONFKEY_TELEPHONY_SIM_SLOT, &status);
	if (ret == OK && status != VCONFKEY_TELEPHONY_SIM_INSERTED) {
		DBG("ROAMING Status: No SIM Mode");
		hide_image_icon();
		return;
	}

	/* Second, check Roaming mode */
	ret = vconf_get_int(VCONFKEY_TELEPHONY_SVC_ROAM, &status);
	DBG("ROAMING Status: %d", status);
	if (ret == OK) {
		if (status == VCONFKEY_TELEPHONY_SVC_ROAM_ON) {
			show_image_icon();
			return;
		} else {
			hide_image_icon();

			return;
		}
	}

	ERR("Failed to get roaming status!");
	return;
}

static int register_roaming_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM,
				       indicator_roaming_change_cb, data);
	if (ret != OK) {
		r = ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				       indicator_roaming_change_cb, data);
	if (ret != OK) {
		r = r | ret;
	}

	indicator_roaming_change_cb(NULL, data);

	return r;
}

static int unregister_roaming_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM,
				       indicator_roaming_change_cb);

	ret = ret | vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
				       indicator_roaming_change_cb);

	return ret;
}
