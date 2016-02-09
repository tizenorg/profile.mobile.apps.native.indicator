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
#include <runtime_info.h>
#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"earphone"
#define TIMER_INTERVAL	0.3

//#define _(str) gettext(str)

static int register_earphone_module(void *data);
static int unregister_earphone_module(void);
static int wake_up_cb(void *data);

icon_s earphone = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_earphone_module,
	.fini = unregister_earphone_module,
	.wake_up = wake_up_cb
};

static const char *icon_path[] = {
	"Earphone/B03_BT_Headset.png",
	NULL
};
static int updated_while_lcd_off = 0;
static int bShown = 0;



static void set_app_state(void* data)
{
	earphone.ad = data;
}

static void show_image_icon(void)
{
	if(bShown == 1)
		return;

	earphone.img_obj.data = icon_path[0];
	icon_show(&earphone);

	bShown = 1;
}

static void hide_image_icon(void)
{
	icon_hide(&earphone);

	bShown = 0;
}

void check_jack_port(void *data)
{
	bool is_jack_connected;
	bool is_tv_out_connected;
	int ret;

	retif(data == NULL, , "Invalid parameter!");

	if(icon_get_update_flag()==0) {
		updated_while_lcd_off = 1;
		return;
	}
	updated_while_lcd_off = 0;
	ret = runtime_info_get_value_bool(RUNTIME_INFO_KEY_AUDIO_JACK_CONNECTED, &is_jack_connected);
	retm_if(ret != RUNTIME_INFO_ERROR_NONE, "runtime_info_get_value_bool failed[%s]", get_error_message(ret));

	ret = runtime_info_get_value_bool(RUNTIME_INFO_KEY_TV_OUT_CONNECTED, &is_tv_out_connected);
	retm_if(ret != RUNTIME_INFO_ERROR_NONE, "runtime_info_get_value_bool failed[%s]", get_error_message(ret));

	if (is_jack_connected || is_tv_out_connected) {
		DBG("Earphone connected");
		show_image_icon();
	}
	else
		hide_image_icon();
}

void indicator_earphone_change_cb(runtime_info_key_e key, void *data)
{
	check_jack_port(data);
}

static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off == 0)
		return OK;

	check_jack_port(data);
	return OK;
}

static int register_earphone_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = runtime_info_set_changed_cb(RUNTIME_INFO_KEY_AUDIO_JACK_CONNECTED, indicator_earphone_change_cb, data);
	retvm_if(ret != RUNTIME_INFO_ERROR_NONE, FAIL, "runtime_info_set_changed_cb failed[%s]", get_error_message(ret));

	ret = runtime_info_set_changed_cb(RUNTIME_INFO_KEY_TV_OUT_CONNECTED, indicator_earphone_change_cb, data);
	retvm_if(ret != RUNTIME_INFO_ERROR_NONE, FAIL, "runtime_info_set_changed_cb failed[%s]", get_error_message(ret));

	check_jack_port(data);

	return ret;
}

static int unregister_earphone_module(void)
{
	int ret;

	ret = runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_AUDIO_JACK_CONNECTED);
	retvm_if(ret != RUNTIME_INFO_ERROR_NONE, FAIL, "runtime_info_unset_changed_cb failed[%s]", get_error_message(ret));

	ret = runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_TV_OUT_CONNECTED);
	retvm_if(ret != RUNTIME_INFO_ERROR_NONE, FAIL, "runtime_info_unset_changed_cb failed[%s]", get_error_message(ret));

	return ret;
}
