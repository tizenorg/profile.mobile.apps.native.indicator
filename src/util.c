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



#include <vconf.h>
#include <app.h>
#include <app_common.h>
#include <Eina.h>
#include <system_settings.h>

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "indicator_gui.h"
#include "util.h"
#include "icon.h"
#include "box.h"
#include "log.h"

#define APP_CONTROL_OPERATION_POPUP_SEARCH "http://samsung.com/appcontrol/operation/search"

#define DEFAULT_DIR	ICONDIR
#define DIR_PREFIX	"Theme_%02d_"
#define LABEL_STRING	"<color=#%02x%02x%02x%02x>%s</color>"

#define MSG_RESERVED "reserved://indicator/"

#define MSG_NORMAL_STATUS_ICON		MSG_RESERVED"icons/notify_message"
#define MSG_FAILED_STATUS_ICON		MSG_RESERVED"icons/notify_message_failed"
#define MSG_DELIVER_REPORT_STATUS_ICON	MSG_RESERVED"icons/delivery_report_message"
#define MSG_READ_REPORT_STATUS_ICON	MSG_RESERVED"icons/read_report_message"
#define MSG_VOICE_MSG_STATUS_ICON	MSG_RESERVED"icons/notify_voicemail"
#define MSG_SIM_FULL_STATUS_ICON	MSG_RESERVED"icons/sim_card_full"


typedef struct {
	wifi_connection_state_changed_cb cb;
	void *data;
} wifi_handler_t;

typedef struct {
	wifi_device_state_changed_cb cb;
	void *data;
} wifi_device_handler_t;

typedef struct {
	system_settings_key_e key;
	system_settings_changed_cb cb;
	void *data;
} system_settings_handler_t;

typedef struct {
	runtime_info_key_e key;
	runtime_info_changed_cb cb;
	void *data;
} runtime_info_handler_t;

typedef struct {
	char *path;
	char *icon;
} reserved_t;

static Eina_List *wifi_callbacks;
static Eina_List *wifi_device_callbacks;
static Eina_List *ss_callbacks;
static Eina_List *ri_callbacks;
static int wifi_init_cnt = 0;

const reserved_t reserved_paths[] = {
	{MSG_NORMAL_STATUS_ICON, "/Notify/B03_notify_message.png"},
	{MSG_FAILED_STATUS_ICON, "/Notify/B03_notify_message_failed.png"},
	{MSG_DELIVER_REPORT_STATUS_ICON, "/Event/B03_event_delivery_report_message.png"},
	{MSG_READ_REPORT_STATUS_ICON, "/Event/B03_event_read_report_message.png"},
	{MSG_VOICE_MSG_STATUS_ICON, "/Event/B03_Event_voicemail.png"},
	{MSG_SIM_FULL_STATUS_ICON, "/SIM card full/B03_sim_card_full.png"}
};


char *util_set_label_text_color(const char *txt)
{
	Eina_Strbuf *temp_buf = NULL;
	Eina_Bool buf_result = EINA_FALSE;
	char *ret_str = NULL;

	retvm_if(txt == NULL, NULL, "Invalid parameter!");

	temp_buf = eina_strbuf_new();
	buf_result = eina_strbuf_append_printf(temp_buf,
				LABEL_STRING, FONT_COLOR, txt);

	if (buf_result == EINA_FALSE)
		_D("Failed to make label string!");
	else
		ret_str = eina_strbuf_string_steal(temp_buf);

	eina_strbuf_free(temp_buf);
	return ret_str;
}

void util_bg_color_rgba_set(Evas_Object *layout, char r, char g, char b, char a)
{
	Edje_Message_Int_Set *msg;

	ret_if(!layout);

	msg = malloc(sizeof(*msg) + 3 * sizeof(int));

	msg->count = 4;
	msg->val[0] = r;
	msg->val[1] = g;
	msg->val[2] = b;
	msg->val[3] = a;

	edje_object_message_send(elm_layout_edje_get(layout), EDJE_MESSAGE_INT_SET, 1, msg);
	free(msg);
}

void util_bg_color_default_set(Evas_Object *layout)
{
	ret_if(!layout);

	elm_layout_signal_emit(layout, "bg.color.default", "indicator.prog");
}

void util_bg_call_color_set(Evas_Object *layout, bg_color_e color)
{
	ret_if(!layout);

	switch (color) {
		case BG_COLOR_CALL_INCOMING:
			elm_layout_signal_emit(layout, "bg.color.call.incoming", "indicator.prog");
			break;
		case BG_COLOR_CALL_END:
			elm_layout_signal_emit(layout, "bg.color.call.end", "indicator.prog");
			break;
		case BG_COLOR_CALL_ON_HOLD:
			elm_layout_signal_emit(layout, "bg.color.call.onhold", "indicator.prog");
			break;
		default:
			util_bg_color_default_set(layout);
			break;
	}
}

const char *util_get_icon_dir(void)
{
	return util_get_res_file_path(DEFAULT_DIR);
}



void util_signal_emit(void* data, const char *emission, const char *source)
{
	struct appdata *ad = NULL;
	Evas_Object *edje = NULL;

	ret_if(!data);

	ad = (struct appdata *)data;

	edje = elm_layout_edje_get(ad->win.layout);
	ret_if(!edje);
	edje_object_signal_emit(edje, emission, source);
}



void util_part_text_emit(void* data, const char *part, const char *text)
{
	struct appdata *ad = (struct appdata *)data;
	retm_if(data == NULL, "Invalid parameter!");
	Evas_Object *edje;

	retm_if(ad->win.layout == NULL, "Invalid parameter!");
	edje = elm_layout_edje_get(ad->win.layout);
	edje_object_part_text_set(edje, part, text);
}



void util_signal_emit_by_win(void* data, const char *emission, const char *source)
{
	win_info *win = NULL;
	Evas_Object *edje = NULL;

	ret_if(!data);

	win = (win_info*)data;
	ret_if(!win->layout);

	_D("emission %s", emission);

	edje = elm_layout_edje_get(win->layout);
	edje_object_signal_emit(edje, emission, source);
}



void util_part_text_emit_by_win(void* data, const char *part, const char *text)
{
	win_info *win = (win_info*)data;
	retm_if(data == NULL, "Invalid parameter!");
	Evas_Object *edje;

	retm_if(win->layout == NULL, "Invalid parameter!");
	edje = elm_layout_edje_get(win->layout);
	edje_object_part_text_set(edje, part, text);
}



void util_part_content_img_set(void *data, const char *part, const char *img_path)
{
	struct appdata *ad = (struct appdata *)data;
	retm_if(data == NULL, "Invalid parameter!");
	Evas_Object *icon = NULL;
	char path[PATH_MAX];

	retm_if(ad->win.layout == NULL, "Invalid parameter!");

	icon = elm_image_add(ad->win.layout);
	retm_if(!icon, "Cannot create elm icon object!");

	if (strncmp(img_path, "/", 1) != 0)
	{
		snprintf(path, sizeof(path), "%s/%s", util_get_icon_dir(), img_path);
	}
	else
	{
		strncpy(path, img_path, sizeof(path)-1);
	}

	if (!ecore_file_exists(path))
	{
		_E("icon file does not exist!!: %s",path);
		return;
	}
	elm_image_file_set(icon, path, NULL);
	evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_part_content_set(ad->win.layout, part, icon);
}



void util_send_status_message_start(void *data, double duration)
{
	Ecore_Evas *ee_port;
	win_info* win = (win_info*)data;
	retm_if(data == NULL, "Invalid parameter!");
	struct appdata *ad = win->data;
	Indicator_Data_Animation msg = {0,};

	msg.xwin = ad->active_indi_win;
	msg.duration = duration;

	_D("status start %x, %f",ad->active_indi_win,duration);
	ee_port = ecore_evas_ecore_evas_get(evas_object_evas_get(win->win));
	ecore_evas_msg_send(ee_port, MSG_DOMAIN_CONTROL_INDICATOR, MSG_ID_INDICATOR_ANI_START, &(msg), sizeof(Indicator_Data_Animation));

}



void util_launch_search(void* data)
{
	int lock_state = SYSTEM_SETTINGS_LOCK_STATE_UNLOCK;
	app_control_h service;

	int ret = system_settings_get_value_int(SYSTEM_SETTINGS_KEY_LOCK_STATE, &lock_state);
	retm_if(ret != SYSTEM_SETTINGS_ERROR_NONE, "system_settings_get_value_int failed %s", get_error_message(ret));

	/* In Lock Screen, home button don't have to do */
	if (lock_state == SYSTEM_SETTINGS_LOCK_STATE_LOCK) {
		return;
	}

	if (util_check_system_status() == FAIL) {
		_D("util_check_system_status failed");
		return;
	}

	app_control_create(&service);
	app_control_set_operation(service, APP_CONTROL_OPERATION_MAIN);
	app_control_set_app_id(service, SEARCH_PKG_NAME);

	ret = app_control_send_launch_request(service, NULL, NULL);

	if(ret != APP_CONTROL_ERROR_NONE) {
		_E("Cannot launch app");
	}

	app_control_destroy(service);
}



int util_check_system_status(void)
{
	int ret, value = -1;

	ret = vconf_get_int(VCONFKEY_PWLOCK_STATE, &value);
	if (ret == OK &&
	    (value == VCONFKEY_PWLOCK_BOOTING_LOCK ||
	     value == VCONFKEY_PWLOCK_RUNNING_LOCK))
		return FAIL;

	return OK;
}



#ifdef _SUPPORT_SCREEN_READER
Evas_Object *util_access_object_register(Evas_Object *object, Evas_Object *layout)
{
	if ((object == NULL) || (layout == NULL)) {
		_E("Access object doesn't exist!!! %x %x",object,layout);
		return NULL;
	}

	return elm_access_object_register(object, layout);
}



void util_access_object_unregister(Evas_Object *object)
{
	if (object == NULL) {
		_E("Access object doesn't exist!!! %x",object);
		return NULL;
	}

	elm_access_object_unregister(object);
}



void util_access_object_info_set(Evas_Object *object, int info_type, char *info_text)
{
	if ((object == NULL) || (info_text == NULL)) {
		_E("Access info set fails %x, %x!!!",object,info_text);
		return;
	}

	elm_access_info_set(object, info_type, (const char *)info_text);
}



void util_access_object_activate_cb_set(Evas_Object *object, Elm_Access_Activate_Cb activate_cb, void *cb_data)
{
	if ((object == NULL) || (activate_cb == NULL)) {
		_E("Access activated cb set fails %x %x!!!",object,activate_cb);
		return;
	}

	elm_access_activate_cb_set(object, activate_cb, cb_data);
}



void util_access_object_info_cb_set(Evas_Object *object, int type, Elm_Access_Info_Cb info_cb, void *cb_data)
{
	if ((object == NULL) || (info_cb == NULL)) {
		_E("Access info cb set fails  %x %x!!!",object,info_cb);
		return;
	}

	elm_access_info_cb_set(object, type, info_cb, cb_data);
}



void util_icon_access_register(icon_s *icon)
{

	if(icon == NULL)
	{
		_E("ICON NULL");
		return;
	}

	if(icon->tts_enable == EINA_TRUE && icon->ao==NULL)
	{
		Evas_Object *to = NULL;

		to = (Evas_Object *) edje_object_part_object_get(elm_layout_edje_get(icon->img_obj.obj), "elm.rect.icon.access");
		icon->ao = util_access_object_register(to, icon->img_obj.obj);

		if(icon->access_cb!=NULL)
		{
			util_access_object_info_cb_set(icon->ao,ELM_ACCESS_INFO,icon->access_cb,icon->ad);
		}
	}
}



void util_icon_access_unregister(icon_s *icon)
{
	if(icon == NULL)
	{
		_E("ICON NULL");
		return;
	}

	if(icon->tts_enable == EINA_TRUE&&icon->ao!=NULL)
	{
		util_access_object_unregister(icon->ao);
		icon->ao = NULL;
	}
}
#endif /* _SUPPORT_SCREEN_READER */



static char* _get_timezone_from_vconf(void)
{
	char *szTimezone = NULL;
	szTimezone = vconf_get_str(VCONFKEY_SETAPPL_TIMEZONE_ID);
	if(szTimezone == NULL)
	{
		_E("Cannot get time zone.");
		return strdup("N/A");
	}

	return szTimezone;
}



char* util_get_timezone_str(void)
{
	return _get_timezone_from_vconf();
}



Eina_Bool util_win_prop_angle_get(Ecore_X_Window win, int *req)
{
	Eina_Bool res = EINA_FALSE;
#if 0
	int ret, count;
	int angle[2] = {-1, -1};
	unsigned char* prop_data = NULL;
	ret = ecore_x_window_prop_property_get(win, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
	if (ret <= 0) {
		if (prop_data) free(prop_data);
		return res;
	}

	if (ret && prop_data) {
		memcpy (&angle, prop_data, sizeof (int)*count);
		if (count == 2) res = EINA_TRUE;
	}

	if (prop_data) free(prop_data);
	*req  = angle[0]; //current angle

	if (angle[0] == -1 && angle[1] == -1) res = EINA_FALSE;
#endif

	return res;
}


#if 0
int util_get_block_width(void* data, const char* part)
{
	Evas_Object * eo = NULL;
	int geo_dx = 0;
	int geo_dy = 0;
	retvm_if(data == NULL, -1, "Invalid parameter!");
	retvm_if(part == NULL, -1, "Invalid parameter!");

	win_info* win = (win_info*)data;

	eo = (Evas_Object *) edje_object_part_object_get(elm_layout_edje_get(win->layout), part);

	evas_object_geometry_get(eo, NULL, NULL, &geo_dx, &geo_dy);

	return geo_dx;
}



int util_get_string_width(void* data, const char* part)
{
	Evas_Object * eo = NULL;
	int text_dx = 0;
	int text_dy = 0;
	retvm_if(data == NULL, -1, "Invalid parameter!");
	retvm_if(part == NULL, -1, "Invalid parameter!");

	win_info* win = (win_info*)data;

	eo = (Evas_Object *) edje_object_part_object_get(elm_layout_edje_get(win->layout), part);

	evas_object_textblock_size_formatted_get(eo, &text_dx, &text_dy);

	return text_dx;
}
#endif


int util_check_noti_ani(const char *path)
{
	retv_if(!path, 0);
	if (!strcmp(path,"reserved://indicator/ani/downloading")
		|| !strcmp(path,"reserved://indicator/ani/uploading")) {
		return 1;
	}
	return 0;
}

char *util_get_real_path(char *special_path)
{
	retv_if(!special_path, 0);

	char *real_path = NULL;

	int i;
	for (i = 0; i < ARRAY_SIZE(reserved_paths); ++i) {
		if(!strcmp(special_path, reserved_paths[i].path)) {

			real_path = calloc(
					strlen(util_get_icon_dir())
					+ strlen(reserved_paths[i].icon)
					+ 1,
					sizeof(char));

			if(!real_path)
				return NULL;

			snprintf(real_path,
					strlen(util_get_icon_dir()) + strlen(reserved_paths[i].icon) + 1,
					"%s%s",	util_get_icon_dir(), reserved_paths[i].icon);

			return real_path;
		}
	}

	return NULL;
}

void util_start_noti_ani(icon_s *icon)
{
	retm_if(icon == NULL, "Invalid parameter!");

	if(util_check_noti_ani(icon->img_obj.data))
	{
		_D("%s",icon->name);
		if(!strcmp(icon->img_obj.data,"reserved://indicator/ani/downloading"))
		{
			icon_ani_set(icon,ICON_ANI_DOWNLOADING);
		}
		else
		{
			icon_ani_set(icon,ICON_ANI_UPLOADING);
		}
	}
}



void util_stop_noti_ani(icon_s *icon)
{
	retm_if(icon == NULL, "Invalid parameter!");

	if(util_check_noti_ani(icon->img_obj.data))
	{
		Evas_Object *img_edje;
		img_edje = elm_layout_edje_get(icon->img_obj.obj);
		_D("%s",icon->name);
		if(!strcmp(icon->img_obj.data,"reserved://indicator/ani/downloading"))
		{
			edje_object_signal_emit(img_edje, "indicator.ani.downloading.stop","elm.swallow.icon");
		}
		else
		{
			edje_object_signal_emit(img_edje, "indicator.ani.uploading.stop","elm.swallow.icon");
		}
	}
}



void util_char_replace(char *text, char s, char t)
{
	retm_if(text == NULL, "invalid argument");

	int i = 0, text_len = 0;

	text_len = strlen(text);

	for (i = 0; i < text_len; i++) {
		if (*(text + i) == s) {
			*(text + i) = t;
		}
	}
}



static bool _is_empty_str(const char *str)
{
	if (NULL == str || '\0' == str[0])
		return true;
	return false;
}



char *util_safe_str(const char *str, const char *str_search)
{
	if (_is_empty_str(str))
		return NULL;

	return strstr(str, str_search);
}



int util_dynamic_state_get(void)
{
	int val = 0;
	//vconf_get_bool(VCONFKEY_SETAPPL_DYNAMIC_STATUS_BAR, &val);
	return val;
}



Ecore_File_Monitor *util_file_monitor_add(const char* file_path, Ecore_File_Monitor_Cb callback_func, void *data)
{
	Ecore_File_Monitor *pFileMonitor = NULL;
	pFileMonitor = ecore_file_monitor_add(file_path, callback_func, data);
	if(pFileMonitor == NULL) _D("ecore_file_monitor_add return NULL !!");

	return pFileMonitor;
}



void util_file_monitor_remove(Ecore_File_Monitor* pFileMonitor)
{
	if(pFileMonitor == NULL) return;

	ecore_file_monitor_del(pFileMonitor);
	pFileMonitor = NULL;
}

const char *util_get_file_path(enum app_subdir dir, const char *relative)
{
	static char buf[PATH_MAX];
	char *prefix;

	switch (dir) {
	case APP_DIR_DATA:
		prefix = app_get_data_path();
		break;
	case APP_DIR_CACHE:
		prefix = app_get_cache_path();
		break;
	case APP_DIR_RESOURCE:
		prefix = app_get_resource_path();
		break;
	case APP_DIR_SHARED_RESOURCE:
		prefix = app_get_shared_resource_path();
		break;
	case APP_DIR_SHARED_TRUSTED:
		prefix = app_get_shared_trusted_path();
		break;
	case APP_DIR_EXTERNAL_DATA:
		prefix = app_get_external_data_path();
		break;
	case APP_DIR_EXTERNAL_CACHE:
		prefix = app_get_external_cache_path();
		break;
	default:
		_E("Not handled directory type.");
		return NULL;
	}
	if (prefix == NULL)
		return NULL;

	size_t res = eina_file_path_join(buf, sizeof(buf), prefix, relative);
	free(prefix);
	if (res > sizeof(buf)) {
		_E("Path exceeded PATH_MAX");
		return NULL;
	}

	return &buf[0];
}

int util_wifi_initialize(void)
{
	_D("util_wifi_initialize");

	wifi_init_cnt++;

	int ret = wifi_initialize();
	if (ret == WIFI_ERROR_INVALID_OPERATION) {
		_W("WiFi already initialized");
		return OK;
	}
	retv_if(ret != WIFI_ERROR_NONE, ret);

	return OK;
}

int util_wifi_deinitialize(void)
{
	_D("util_wifi_deinitialize");

	wifi_init_cnt--;

	if (wifi_init_cnt == 0) {
		int ret = wifi_deinitialize();
		retv_if(ret != WIFI_ERROR_NONE, ret);
	}
	return OK;
}

static void _wifi_state_cb(wifi_connection_state_e state, wifi_ap_h ap, void *user_data)
{
	Eina_List *l;
	wifi_handler_t *hdl;

	EINA_LIST_FOREACH(wifi_callbacks, l, hdl) {
		if (hdl->cb) hdl->cb(state, ap, hdl->data);
	}
}

int util_wifi_set_connection_state_changed_cb(wifi_connection_state_changed_cb cb, void *data)
{
	wifi_handler_t *hdl = malloc(sizeof(wifi_handler_t));
	if (!hdl) {
		return -1;
	}

	if (!wifi_callbacks) {
		int err = wifi_set_connection_state_changed_cb(_wifi_state_cb, NULL);
		if (err != WIFI_ERROR_NONE) {
			free(hdl);
			return -1;
		}
	}

	hdl->cb = cb;
	hdl->data = data;
	wifi_callbacks = eina_list_append(wifi_callbacks, hdl);

	return 0;
}

void util_wifi_unset_connection_state_changed_cb(wifi_connection_state_changed_cb cb)
{
	Eina_List *l, *l2;
	wifi_handler_t *hdl;

	EINA_LIST_FOREACH_SAFE(wifi_callbacks, l, l2, hdl) {
		if (hdl->cb == cb) {
			wifi_callbacks = eina_list_remove_list(wifi_callbacks, l);
			free(hdl);
		}
	}
	if (!wifi_callbacks)
		wifi_unset_connection_state_changed_cb();
}

/** WIFI DEVICE STATE CB **/

static void _wifi_device_state_cb(wifi_device_state_e state, void *user_data)
{
	Eina_List *l;
	wifi_device_handler_t *hdl;

	EINA_LIST_FOREACH(wifi_device_callbacks, l, hdl) {
		if (hdl->cb) hdl->cb(state, hdl->data);
	}
}

int util_wifi_set_device_state_changed_cb(wifi_device_state_changed_cb cb, void *data)
{
	wifi_device_handler_t *hdl = malloc(sizeof(wifi_device_handler_t));
	if (!hdl) {
		_D("malloc failed");
		return WIFI_ERROR_OUT_OF_MEMORY;
	}

	if (!wifi_device_callbacks) {
		int err = wifi_set_device_state_changed_cb(_wifi_device_state_cb, NULL);
		if (err != WIFI_ERROR_NONE) {
			free(hdl);

			_D("wifi_set_device_state_changed_cb failed[%d]:%s", err, get_error_message(err));
			return err;
		}
	}

	hdl->cb = cb;
	hdl->data = data;
	wifi_device_callbacks = eina_list_append(wifi_device_callbacks, hdl);

	return 0;
}

void util_wifi_unset_device_state_changed_cb(wifi_device_state_changed_cb cb)
{
	Eina_List *l, *l2;
	wifi_device_handler_t *hdl;

	EINA_LIST_FOREACH_SAFE(wifi_device_callbacks, l, l2, hdl) {
		if (hdl->cb == cb) {
			wifi_device_callbacks = eina_list_remove_list(wifi_device_callbacks, l);
			free(hdl);
		}
	}
	if (!wifi_device_callbacks)
		wifi_unset_device_state_changed_cb();
}

/** SYSTEM SETTINGS CB **/

static void _system_settings_cb(system_settings_key_e key, void *data)
{
	Eina_List *l;
	system_settings_handler_t *hdl;

	EINA_LIST_FOREACH(ss_callbacks, l, hdl) {
		if (hdl->key == key) {
			if (hdl->cb) hdl->cb(key, hdl->data);
		}
	}
}

int util_system_settings_set_changed_cb(system_settings_key_e key, system_settings_changed_cb cb, void *data)
{
	system_settings_handler_t *handler = malloc(sizeof(system_settings_handler_t));
	if (!handler) {
		return -1;
	}

	system_settings_unset_changed_cb(key);
	int err = system_settings_set_changed_cb(key, _system_settings_cb, NULL);
	if (err != SYSTEM_SETTINGS_ERROR_NONE) {
		_E("system_settings_set_changed_cb failed[%d]: %s", err, get_error_message(err));
		free(handler);
		return -1;
	}

	handler->key = key;
	handler->cb = cb;
	handler->data = data;

	ss_callbacks = eina_list_append(ss_callbacks, handler);

	return 0;
}

void util_system_settings_unset_changed_cb(system_settings_key_e key, system_settings_changed_cb cb)
{
	Eina_List *l, *l2;
	system_settings_handler_t *hdl;
	Eina_Bool del_key_cb = EINA_TRUE;

	EINA_LIST_FOREACH_SAFE(ss_callbacks, l, l2, hdl) {
		if (hdl->key == key) {
			if (hdl->cb == cb) {
				ss_callbacks = eina_list_remove_list(ss_callbacks, l);
				free(hdl);
			}
			else {
				del_key_cb = EINA_FALSE;
			}
		}
	}
	if (del_key_cb)
		system_settings_unset_changed_cb(key);
}

static void _runtime_info_cb(runtime_info_key_e key, void *data)
{
	Eina_List *l;
	runtime_info_handler_t *hdl;

	EINA_LIST_FOREACH(ri_callbacks, l, hdl) {
		if (hdl->key == key) {
			if (hdl->cb) hdl->cb(key, hdl->data);
		}
	}
}

int util_runtime_info_set_changed_cb(runtime_info_key_e key, runtime_info_changed_cb cb, void *data)
{
	runtime_info_handler_t *handler = malloc(sizeof(runtime_info_handler_t));
	if (!handler) {
		return -1;
	}

	runtime_info_unset_changed_cb(key);
	int err = runtime_info_set_changed_cb(key, _runtime_info_cb, NULL);
	if (err != RUNTIME_INFO_ERROR_NONE) {
		_E("runtime_info_set_changed_cb failed: %s", get_error_message(err));
		free(handler);
		return -1;
	}

	handler->key = key;
	handler->cb = cb;
	handler->data = data;

	ri_callbacks = eina_list_append(ri_callbacks, handler);

	return 0;
}

void util_runtime_info_unset_changed_cb(runtime_info_key_e key, runtime_info_changed_cb cb)
{
	Eina_List *l, *l2;
	runtime_info_handler_t *hdl;
	Eina_Bool del_key_cb = EINA_TRUE;

	EINA_LIST_FOREACH_SAFE(ri_callbacks, l, l2, hdl) {
		if (hdl->key == key) {
			if (hdl->cb == cb) {
				ri_callbacks = eina_list_remove_list(ri_callbacks, l);
				free(hdl);
			}
			else {
				del_key_cb = EINA_FALSE;
			}
		}
	}
	if (del_key_cb)
		runtime_info_unset_changed_cb(key);

}

/* End of file */
