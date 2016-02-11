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

typedef struct {
	wifi_connection_state_changed_cb cb;
	void *data;
} wifi_handler_t;

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

static Eina_List *wifi_callbacks;
static Eina_List *ss_callbacks;
static Eina_List *ri_callbacks;

char *util_set_label_text_color(const char *txt)
{
	Eina_Strbuf *temp_buf = NULL;
	Eina_Bool buf_result = EINA_FALSE;
	char *ret_str = NULL;

	retif(txt == NULL, NULL, "Invalid parameter!");

	temp_buf = eina_strbuf_new();
	buf_result = eina_strbuf_append_printf(temp_buf,
				LABEL_STRING, FONT_COLOR, txt);

	if (buf_result == EINA_FALSE)
		DBG("Failed to make label string!");
	else
		ret_str = eina_strbuf_string_steal(temp_buf);

	eina_strbuf_free(temp_buf);
	return ret_str;
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

	char *filter1 = "indicator.connection.updown";
	char *filter2 = "indicator.wifi.updown";
	if (strncmp(filter1, emission, strlen(filter1)) != 0
			&& strncmp(filter2, emission, strlen(filter2)) != 0) {
		SECURE_DBG("emission %s",emission);
	}

	edje = elm_layout_edje_get(ad->win.layout);
	ret_if(!edje);
	edje_object_signal_emit(edje, emission, source);
}



void util_part_text_emit(void* data, const char *part, const char *text)
{
	struct appdata *ad = (struct appdata *)data;
	retif(data == NULL, , "Invalid parameter!");
	Evas_Object *edje;

	retif(ad->win.layout == NULL, , "Invalid parameter!");
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
	retif(data == NULL, , "Invalid parameter!");
	Evas_Object *edje;

	retif(win->layout == NULL, , "Invalid parameter!");
	edje = elm_layout_edje_get(win->layout);
	edje_object_part_text_set(edje, part, text);
}



void util_battery_percentage_part_content_set(void* data, const char *part, const char *img_path)
{
	struct appdata *ad = (struct appdata *)data;
	retif(data == NULL, , "Invalid parameter!");
	Evas_Object *icon = NULL;
	char path[PATH_MAX];

	retif(ad->win.layout == NULL, , "Invalid parameter!");

	icon = elm_image_add(ad->win.layout);
	retif(!icon, , "Cannot create elm icon object!");

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
		ERR("icon file does not exist!!: %s",path);
		return;
	}
	elm_image_file_set(icon, path, NULL);
	evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_part_content_set(ad->win.layout, part, icon);
}



void util_send_status_message_start(void* data,double duration)
{
	Ecore_Evas *ee_port;
	win_info* win = (win_info*)data;
	retif(data == NULL, , "Invalid parameter!");
	struct appdata *ad = win->data;
	Indicator_Data_Animation msg = {0,};

	msg.xwin = ad->active_indi_win;
	msg.duration = duration;

	DBG("status start %x, %f",ad->active_indi_win,duration);
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
		DBG("util_check_system_status failed");
		return;
	}

	app_control_create(&service);
	app_control_set_operation(service, APP_CONTROL_OPERATION_MAIN);
	app_control_set_app_id(service, SEARCH_PKG_NAME);

	ret = app_control_send_launch_request(service, NULL, NULL);

	if(ret != APP_CONTROL_ERROR_NONE) {
		ERR("Cannot launch app");
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
		ERR("Access object doesn't exist!!! %x %x",object,layout);
		return NULL;
	}

	return elm_access_object_register(object, layout);
}



void util_access_object_unregister(Evas_Object *object)
{
	if (object == NULL) {
		ERR("Access object doesn't exist!!! %x",object);
		return NULL;
	}

	elm_access_object_unregister(object);
}



void util_access_object_info_set(Evas_Object *object, int info_type, char *info_text)
{
	if ((object == NULL) || (info_text == NULL)) {
		ERR("Access info set fails %x, %x!!!",object,info_text);
		return;
	}

	elm_access_info_set(object, info_type, (const char *)info_text);
}



void util_access_object_activate_cb_set(Evas_Object *object, Elm_Access_Activate_Cb activate_cb, void *cb_data)
{
	if ((object == NULL) || (activate_cb == NULL)) {
		ERR("Access activated cb set fails %x %x!!!",object,activate_cb);
		return;
	}

	elm_access_activate_cb_set(object, activate_cb, cb_data);
}



void util_access_object_info_cb_set(Evas_Object *object, int type, Elm_Access_Info_Cb info_cb, void *cb_data)
{
	if ((object == NULL) || (info_cb == NULL)) {
		ERR("Access info cb set fails  %x %x!!!",object,info_cb);
		return;
	}

	elm_access_info_cb_set(object, type, info_cb, cb_data);
}



void util_icon_access_register(icon_s *icon)
{

	if(icon == NULL)
	{
		ERR("ICON NULL");
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
		ERR("ICON NULL");
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
		ERR("Cannot get time zone.");
		return strdup("N/A");
	}

	return szTimezone;
}



char* util_get_timezone_str(void)
{
	enum { BUFFERSIZE = 1024 };
	char buf[BUFFERSIZE];
	ssize_t len = readlink("/opt/etc/localtime", buf, sizeof(buf)-1);

	if (len != -1)
	{
		buf[len] = '\0';
	}
	else
	{
		ERR("NO TIMEZONEINFO");
		return _get_timezone_from_vconf();
	}
	return (char*)strdup(buf+20);	// Asia/Seoul
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
	retif(data == NULL,-1, "Invalid parameter!");
	retif(part == NULL,-1, "Invalid parameter!");

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
	retif(data == NULL,-1, "Invalid parameter!");
	retif(part == NULL,-1, "Invalid parameter!");

	win_info* win = (win_info*)data;

	eo = (Evas_Object *) edje_object_part_object_get(elm_layout_edje_get(win->layout), part);

	evas_object_textblock_size_formatted_get(eo, &text_dx, &text_dy);

	return text_dx;
}
#endif


int util_is_orf(void)
{
	return 1;
}



int util_check_noti_ani(const char* path)
{
	retv_if(!path, 0);
	if (!strcmp(path,"reserved://indicator/ani/downloading")
		|| !strcmp(path,"reserved://indicator/ani/uploading")) {
		return 1;
	}
	return 0;
}



void util_start_noti_ani(void* data)
{
	icon_s *icon = (icon_s *)data;
	retif(data == NULL, , "Invalid parameter!");

	if(util_check_noti_ani(icon->img_obj.data))
	{
		DBG("%s",icon->name);
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



void util_stop_noti_ani(void* data)
{
	icon_s *icon = (icon_s *)data;
	retif(data == NULL, , "Invalid parameter!");

	if(util_check_noti_ani(icon->img_obj.data))
	{
		Evas_Object *img_edje;
		img_edje = elm_layout_edje_get(icon->img_obj.obj);
		DBG("%s",icon->name);
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
	retif(text == NULL, , "invalid argument");

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



char *util_safe_str(const char *str, const char *strSearch)
{
	if (_is_empty_str(str))
		return NULL;

	return strstr(str, strSearch);
}



int util_dynamic_state_get(void)
{
	int val = 0;
	//vconf_get_bool(VCONFKEY_SETAPPL_DYNAMIC_STATUS_BAR, &val);
	return val;
}



Ecore_File_Monitor* util_file_monitor_add(const char* file_path, Ecore_File_Monitor_Cb callback_func, void *ad)
{
	SECURE_DBG("File path : %s", file_path);
	Ecore_File_Monitor* pFileMonitor = NULL;
	pFileMonitor = ecore_file_monitor_add(file_path, callback_func, ad);
	if(pFileMonitor == NULL) SECURE_DBG("ecore_file_monitor_add return NULL !!");

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
	case APP_DIR_SHARED_DATA:
		prefix = app_get_shared_data_path();
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
	case APP_DIR_EXTERNAL_SHARED_DATA:
		prefix = app_get_external_shared_data_path();
		break;
	default:
		LOGE("Not handled directory type.");
		return NULL;
	}
	size_t res = eina_file_path_join(buf, sizeof(buf), prefix, relative);
	free(prefix);
	if (res > sizeof(buf)) {
		LOGE("Path exceeded PATH_MAX");
		return NULL;
	}

	return &buf[0];
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
	wifi_callbacks = eina_list_append(wifi_callbacks, cb);

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
		ERR("system_settings_set_changed_cb failed: %s", get_error_message(err));
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
		ERR("runtime_info_set_changed_cb failed: %s", get_error_message(err));
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
