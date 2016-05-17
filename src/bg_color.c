/*
 *  Indicator
 *
 * Copyright (c) 2000 - 2016 Samsung Electronics Co., Ltd. All rights reserved.
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

#include "indicator.h"
#include "log.h"
#include "util.h"
#include "main.h"

#include <message_port.h>
#include <bundle.h>

#define KEY_INDICATOR_BG "indicator/bg/color"

/*
 * Values for KEY_INDICATOR_BG key
 */
#define VALUE_INDICATOR_BG_RGB "indicator/bg/color/rgb"
#define KEY_VALUE_INDICATOR_BG_CALL "indicator/bg/color/call"

/*
 * Keys that are set if KEY_INDICATOR_BG is paired with VALUE_INDICATOR_BG_RGB
 */
#define KEY_R "bg/r"
#define KEY_G "bg/g"
#define KEY_B "bg/b"
#define KEY_A "bg/a"

/*
 * Values for KEY_VALUE_INDICATOR_BG_CALL key
 */
#define VALUE_CALL_INCOMING_CALL "call/during_call"
#define VALUE_CALL_ON_HOLD "call/on_hold"
#define VALUE_CALL_END_CALL "call/end_call"
#define VALUE_CALL_IDLE "call/idle"


static int port_id;


static void set_bg_for_call(void *data, bundle *message)
{
	_D("set_bg_for_call");

	struct appdata *ad;

	ret_if(!data);
	ad = data;

	char *call_state;
	int ret;

	ret = bundle_get_str(message, KEY_VALUE_INDICATOR_BG_CALL, &call_state);
	retm_if(ret != BUNDLE_ERROR_NONE, "bundle_get_type failed[%d]:%s", ret, get_error_message(ret));

	if (!strcmp(call_state, VALUE_CALL_INCOMING_CALL)) {
		util_bg_call_color_set(ad->win.layout, BG_COLOR_CALL_INCOMING);
	} else if (!strcmp(call_state, VALUE_CALL_ON_HOLD)) {
		util_bg_call_color_set(ad->win.layout, BG_COLOR_CALL_ON_HOLD);
	} else if (!strcmp(call_state, VALUE_CALL_END_CALL)) {
		util_bg_call_color_set(ad->win.layout, BG_COLOR_CALL_END);
	} else if (!strcmp(call_state, VALUE_CALL_IDLE)) {
		util_bg_color_default_set(ad->win.layout);
	} else {
		_E("Invalid value!");
	}
}


static void validate_and_set_bg_rgba(int rgba_r, int rgba_g, int rgba_b, int rgba_a)
{
	_D("validate_and_set_bg");

	retm_if(rgba_r < 0 || rgba_r > 255, "R component of RGBA color scheme is invalid");
	retm_if(rgba_g < 0 || rgba_g > 255, "G component of RGBA color scheme is invalid");
	retm_if(rgba_b < 0 || rgba_b > 255, "B component of RGBA color scheme is invalid");
	retm_if(rgba_a < 0 || rgba_a > 255, "A component of RGBA color scheme is invalid");
}


static void set_bg_rgba(void *data, bundle *message)
{
	_D("set_bg_rgba");

	void *rgba_r;
	void *rgba_g;
	void *rgba_b;
	void *rgba_a;
	int ret;
	size_t size;
	struct appdata *ad;

	ret_if(!data);
	ad = data;

	ret = bundle_get_byte(message, KEY_R, &rgba_r, &size);
	retm_if(ret != BUNDLE_ERROR_NONE, "bundle_get_byte failed[%d]:%s", ret, get_error_message(ret));
	ret = bundle_get_byte(message, KEY_G, &rgba_g, &size);
	retm_if(ret != BUNDLE_ERROR_NONE, "bundle_get_byte failed[%d]:%s", ret, get_error_message(ret));
	ret = bundle_get_byte(message, KEY_B, &rgba_b, &size);
	retm_if(ret != BUNDLE_ERROR_NONE, "bundle_get_byte failed[%d]:%s", ret, get_error_message(ret));
	ret = bundle_get_byte(message, KEY_A, &rgba_a, &size);
	retm_if(ret != BUNDLE_ERROR_NONE, "bundle_get_byte failed[%d]:%s", ret, get_error_message(ret));

	validate_and_set_bg_rgba(*(int *)rgba_r, *(int *)rgba_g, *(int *)rgba_b, *(int *)rgba_a);

	util_bg_color_rgba_set(ad->win.layout, *(int *)rgba_r, *(int *)rgba_g, *(int *)rgba_b, *(int *)rgba_a);
}


static void message_port_cb(int trusted_local_port_id, const char *remote_app_id,
		const char *remote_port, bool trusted_remote_port, bundle *message, void *data)
{
	_D("message_port_cb");

	int ret;
	char *value;

	ret = bundle_get_str(message, KEY_INDICATOR_BG, &value);
	retm_if(ret != BUNDLE_ERROR_NONE, "bundle_get_type failed[%d]:%s", ret, get_error_message(ret));

	if (!strcmp(value, KEY_VALUE_INDICATOR_BG_CALL))
		set_bg_for_call(data, message);

	else if (!strcmp(value, VALUE_INDICATOR_BG_RGB))
		set_bg_rgba(data, message);
}


void message_port_register(void *data)
{
	_D("message_port_register");

	char *port_name = "indicator/bg/color";

	port_id = message_port_register_trusted_local_port(port_name, message_port_cb, data);
	retm_if(port_id < MESSAGE_PORT_ERROR_NONE,
			"message_port_register_trusted_local_port failed[%d]:%s", port_id, get_error_message(port_id));
}


void message_port_unregister(void)
{
	_D("message_port_unregister");
	int ret;

	ret = message_port_unregister_trusted_local_port(port_id);
	retm_if(ret != MESSAGE_PORT_ERROR_NONE, "message_port_unregister_trusted_local_port failed[%d]:%s", ret, get_error_message(ret));

}
