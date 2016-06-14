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

#include <notification.h>
#include <notification_status_internal.h>

#include "main.h"
#include "common.h"
#include "util.h"
#include "tts.h"
#include "box.h"
#include "log.h"

#define MSG_TIMEOUT 3
#define STR_BUF_SIZE 256
#define QUEUE_TIMEOUT 1
#define QUEUE_TIMEOUT2 5

#define MESSAGE_LINE1 "message.text"
#define MESSAGE_LINE2 "message.text2"

typedef struct _str_buf {
	char *data;
	int index;
	double timer_val;
} MsgBuf;

static struct _s_info {
	Eina_List *toast_list;
	Evas_Object *toast_popup;
	struct appdata *app_data;
} s_info = {
	.toast_list = NULL,
	.toast_popup = NULL,
	.app_data = NULL,
};

static void _post_toast_message(char *message);


static void _popup_timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	char *msg = NULL;
	int count = 0;

	evas_object_del(s_info.toast_popup);
	s_info.toast_popup = NULL;
	evas_object_del((Evas_Object *)data); /* toast window */

	count = eina_list_count(s_info.toast_list);
	if (count == 1){
		msg = (char *)eina_list_nth(s_info.toast_list, 0);
		free(msg);

		eina_list_free(s_info.toast_list);
		s_info.toast_list = NULL;
	} else if (count > 1) {
		msg = (char *)eina_list_nth(s_info.toast_list, 0);
		s_info.toast_list = eina_list_remove(s_info.toast_list, msg);
		free(msg);

		_post_toast_message((char *)eina_list_nth(s_info.toast_list, 0));
	}
}

static void _post_toast_message(char *message)
{
	Evas_Object *toast_window;

	toast_window = elm_win_add(NULL, "toast", ELM_WIN_BASIC);
	elm_win_alpha_set(toast_window, EINA_TRUE);
	elm_win_title_set(toast_window, "toast");

	if (elm_win_wm_rotation_supported_get(toast_window)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(toast_window, (const int*)(&rots), 4);
	}
	elm_win_role_set(toast_window, "notification-normal");
	evas_object_show(toast_window);

	s_info.toast_popup = elm_popup_add(toast_window);
	elm_object_style_set(s_info.toast_popup, "toast");
	evas_object_size_hint_weight_set(s_info.toast_popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_text_set(s_info.toast_popup, message);

	if (eina_list_count(s_info.toast_list) != 1) {
		elm_popup_timeout_set(s_info.toast_popup, 1.0);
	} else {
		elm_popup_timeout_set(s_info.toast_popup, 3.0);
	}
	evas_object_smart_callback_add(s_info.toast_popup, "timeout", _popup_timeout_cb, (void *)toast_window);

	elm_win_prop_focus_skip_set(toast_window, EINA_TRUE);

	evas_object_show(s_info.toast_popup);

	return;
}

static void _post_toast_message_callback(const char *message, void *data)
{
	_D("");
	char *msg = NULL;
	char *msg_from_list = NULL;
	char *temp_msg = NULL;
	int count = 0;

	msg = (char *)calloc(strlen(message) + 1, sizeof(char));
	ret_if(!msg);

	strncpy(msg, message, strlen(message) + 1);

	count = eina_list_count(s_info.toast_list);
	if (count == 0) {
		s_info.toast_list = eina_list_append(s_info.toast_list, msg);
		_post_toast_message(msg);
	} else if (count == 1) {
		msg_from_list = (char *)eina_list_nth(s_info.toast_list, 0);
		if (!msg_from_list) {
			free(msg);
			return;
		}

		if (!strcmp(msg, msg_from_list)) {
			elm_popup_timeout_set(s_info.toast_popup, 3.0);
			free(msg);
		} else {
			s_info.toast_list = eina_list_append(s_info.toast_list, msg);
			elm_popup_timeout_set(s_info.toast_popup, 1.0);
		}
	} else if (count >= 2) {
		temp_msg = (char *)eina_list_nth(s_info.toast_list, count - 1);
		if (!temp_msg) {
			free(msg);
			return;
		}

		if (!strcmp(msg, temp_msg)) {
			free(msg);
			return;
		} else {
			s_info.toast_list = eina_list_append(s_info.toast_list, msg);
		}
	}
	else
		free(msg);

	return;
}

int indicator_toast_popup_init(void *data)
{
	_D("indicator_toast_popup_init");
	int ret = 0;

	ret = notification_status_monitor_message_cb_set(_post_toast_message_callback, data);
	retvm_if(ret != NOTIFICATION_ERROR_NONE, FAIL,
			"notification_status_monitor_message_cb_set failed[%d]: %s", ret, get_error_message(ret));
	s_info.app_data = data;

	_D("");
	return OK;
}

int indicator_toast_popup_fini(void)
{
	int ret = 0;

	ret = notification_status_monitor_message_cb_unset();

	return ret;
}
/* End of file */
