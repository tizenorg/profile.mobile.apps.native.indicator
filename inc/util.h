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



#ifndef __INDICATOR_UTIL_H__
#define __INDICATOR_UTIL_H__
#include <Ecore.h>
#include <wifi.h>

typedef enum {
	INDICATOR_ERROR_NONE = 0,
	INDICATOR_ERROR_FAIL = -1,
	INDICATOR_ERROR_DB_FAILED = -2,
	INDICATOR_ERROR_OUT_OF_MEMORY = -3,
	INDICATOR_ERROR_INVALID_PARAMETER = -4,
	INDICATOR_ERROR_NO_DATA = -5,
} indicator_error_e;

typedef struct _Indicator_Data_Animation Indicator_Data_Animation;

struct _Indicator_Data_Animation
{
   Ecore_X_Window xwin;
   double         duration;
};

extern char *util_set_label_text_color(const char *txt);
extern const char *util_get_icon_dir(void);
extern void util_signal_emit(void* data, const char *emission, const char *source);
extern void util_part_text_emit(void* data, const char *part, const char *text);
extern void util_signal_emit_by_win(void* data, const char *emission, const char *source);
extern void util_part_text_emit_by_win(void* data, const char *part, const char *text);
extern void util_battery_percentage_part_content_set(void* data, const char *part, const char *img_path);
extern void util_launch_search(void* data);
extern int util_check_system_status(void);
extern char* util_get_timezone_str(void);
extern Eina_Bool util_win_prop_angle_get(Ecore_X_Window win, int *curr);
extern int util_is_orf(void);
extern int util_check_noti_ani(const char* path);
extern void util_start_noti_ani(void* data);
extern void util_stop_noti_ani(void* data);
extern void util_send_status_message_start(void* data,double duration);
extern void util_char_replace(char *text, char s, char t);
extern int util_dynamic_state_get(void);

extern Ecore_File_Monitor* util_file_monitor_add(const char* file_path, Ecore_File_Monitor_Cb callback_func, void *ad);
extern void util_file_monitor_remove(Ecore_File_Monitor* pFileMonitor);
extern char *util_safe_str(const char *str, const char *strSearch);

#ifdef _SUPPORT_SCREEN_READER
extern Evas_Object *util_access_object_register(Evas_Object *object, Evas_Object *layout);
extern void util_access_object_unregister(Evas_Object *object);
extern void util_access_object_info_set(Evas_Object *object, int info_type, char *info_text);
extern void util_icon_access_register(icon_s *icon);
extern void util_icon_access_unregister(icon_s *icon);
#endif /* _SUPPORT_SCREEN_READER */

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/*
 * @brief Application sub-directories type.
 */
enum app_subdir {
	APP_DIR_DATA,
	APP_DIR_CACHE,
	APP_DIR_RESOURCE,
	APP_DIR_SHARED_DATA,
	APP_DIR_SHARED_RESOURCE,
	APP_DIR_SHARED_TRUSTED,
	APP_DIR_EXTERNAL_DATA,
	APP_DIR_EXTERNAL_CACHE,
	APP_DIR_EXTERNAL_SHARED_DATA,
};

/**
 * @brief Returns absolute path to resource file located in applications directory.
 *
 * @param subdir type of subdirectory
 * @param relative path of resource in application's subdir.
 *        eg. for DATA_DIR subdir and relative "database.db" => "/home/owner/apps/org.tizen.homescreen-efl/data/database.db"
 * @return absolute path string.
 */
const char *util_get_file_path(enum app_subdir dir, const char *relative);

/**
 * @brief Convinience macros
 */
#define util_get_data_file_path(x) util_get_file_path(APP_DIR_DATA, (x))
#define util_get_cache_file_path(x) util_get_file_path(APP_DIR_CACHE, (x))
#define util_get_res_file_path(x) util_get_file_path(APP_DIR_RESOURCE, (x))
#define util_get_shared_data_file_path(x) util_get_file_path(APP_DIR_SHARED_DATA, (x))
#define util_get_shared_res_file_path(x) util_get_file_path(APP_DIR_SHARED_RESOURCE, (x))
#define util_get_trusted_file_path(x) util_get_file_path(APP_DIR_SHARED_TRUSTED, (x))
#define util_get_external_data_file_path(x) util_get_file_path(APP_DIR_EXTERNAL_DATA, (x))
#define util_get_external_cache_file_path(x) util_get_file_path(APP_DIR_EXTERNAL_CACHE, (x))
#define util_get_external_shared_data_file_path(x) util_get_file_path(APP_DIR_EXTERNAL_SHARED_DATA, (x))

/**
 * @brief Allows to set multiple callbacks using wifi_set_connection_state_changed_cb API
 *
 * @param cb callback
 * @param data user_data passed to callback function.
 * @return 0 on success, other value on failure
 */
int util_wifi_set_connection_state_changed_cb(wifi_connection_state_changed_cb, void *data);

/**
 * @brief Unregisters callback set with util_wifi_set_connection_state_changed_cb.
 *
 * @param cb callback
 */
void util_wifi_unset_connection_state_changed_cb(wifi_connection_state_changed_cb);

#endif /* __INDICATOR_UTIL_H__ */
