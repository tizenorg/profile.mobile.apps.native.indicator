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
#include <system_settings.h>
#include <runtime_info.h>
#include <network/wifi.h>

/**
 * @file util.h
 */

typedef enum {
	INDICATOR_ERROR_NONE = 0,
	INDICATOR_ERROR_FAIL = -1,
	INDICATOR_ERROR_DB_FAILED = -2,
	INDICATOR_ERROR_OUT_OF_MEMORY = -3,
	INDICATOR_ERROR_INVALID_PARAMETER = -4,
	INDICATOR_ERROR_NO_DATA = -5,
} indicator_error_e;

typedef enum {
	BG_COLOR_DEFAULT = 0,
	BG_COLOR_CALL_INCOMING,
	BG_COLOR_CALL_END,
	BG_COLOR_CALL_ON_HOLD,
} bg_color_e;

typedef struct _Indicator_Data_Animation Indicator_Data_Animation;

struct _Indicator_Data_Animation
{
   Ecore_X_Window xwin;
   double         duration;
};


/**
 * @brief Sets label text color.
 *
 * @param[in] txt the text to append color info
 *
 * @return txt with color info appended, or NULL if failure
 */
extern char *util_set_label_text_color(const char *txt);

/**
 * @brief Sets indicator background color to default.
 *
 * @param[in] layout layout of the indicator
 *
 * @see util_bg_color_rgba_set()
 * @see util_bg_call_color_set()
 */
extern void util_bg_color_default_set(Evas_Object *layout);

/**
 * @brief Sets indicator background color globally.
 *
 * @remarks The background color is set until it is changed using the same function,
 * util_bg_call_color_set() or util_bg_color_default_set()
 *
 * @param[in] layout layout of the indicator
 * @param[in] r red component of the color
 * @param[in] g green component of the color
 * @param[in] b blue component of the color
 * @param[in] a alpha component of the color
 *
 * @see util_bg_color_default_set()
 * @see util_bg_call_color_set()
 */
extern void util_bg_color_rgba_set(Evas_Object *layout, char r, char g, char b, char a);

/**
 * @brief Sets indicator background color globally for call app purposes.
 *
 * @remarks The background color is set until it is changed using the same function,
 * util_bg_call_color_set() or util_bg_color_default_set()
 *
 * @param[in] layout layout of the indicator
 * @param[in] color specifies particular bg state accordingly to call app state
 *
 * @see util_bg_color_default_set()
 * @see util_bg_color_rgba_set()
 */
extern void util_bg_call_color_set(Evas_Object *layout, bg_color_e color);

/**
 * @brief Gets icons directory.
 *
 * @return icons directory, or NULL if failure
 */
extern const char *util_get_icon_dir(void);

/**
 * @brief Emits specified signal.
 *
 * @param[in] data the app data
 * @paran[in] emission signal to emit
 * @param[in] source signal source
 *
 */
extern void util_signal_emit(void* data, const char *emission, const char *source);

/**
 * @brief Emits text to specified part.
 *
 * @param[in] data the app data
 * @paran[in] part part name
 * @param[in] text text to emit
 */
extern void util_part_text_emit(void* data, const char *part, const char *text);

/**
 * @brief Emits text to specified part using win info structure to get layout.
 *
 * @param[in] data win info
 * @paran[in] emission signal to emit
 * @param[in] source signal source
 */
extern void util_signal_emit_by_win(void* data, const char *emission, const char *source);

/**
 * @brief Emits text to specified part using win info structure to get layout.
 *
 * @param[in] data win info
 * @paran[in] part part name
 * @param[in] text text to emit
 */
extern void util_part_text_emit_by_win(void* data, const char *part, const char *text);

/**
 * @brief Sets image to part content.
 *
 * @param[in] data the app data
 * @paran[in] part part name
 * @param[in] img_path path to image to set
 */
extern void util_part_content_img_set(void *data, const char *part, const char *img_path);

/**
 * @brief Launches search.
 *
 * @param[in] data the app data
 */
extern void util_launch_search(void* data);

/**
 * @brief Checks system status.
 *
 * @return 0 if PWLOCK is set to VCONFKEY_PWLOCK_BOOTING_UNLOCK or VCONFKEY_PWLOCK_RUNNING_UNLOCK, -1 otherwise
 *
 * @see #VCONFKEY_PWLOCK_BOOTING_UNLOCK
 * @see #VCONFKEY_PWLOCK_BOOTING_LOCK
 * @see	#VCONFKEY_PWLOCK_RUNNING_UNLOCK
 * @see #VCONFKEY_PWLOCK_RUNNING_LOCK
 */
extern int util_check_system_status(void);

/**
 * @brief Gets timezone from vconf.
 *
 * @param[in/out] timezone id or "N/A" on failure
 */
extern void util_get_timezone_str(char **timezone);

/**
 * @brief Gets window angle property.
 *
 * @remarks will be removed
 */
extern Eina_Bool util_win_prop_angle_get(Ecore_X_Window win, int *curr);

/**
 * @brief Checks if path contains special string that indicates animation icon.
 *
 * @return 1 if contains, 0 if not
 */
extern int util_check_noti_ani(const char* path);

/**
 * @brief Checks if @a path is a one of special reserved strings and returns absolute path to real file related to the string.
 *
 * @param path a path to examine
 *
 * @return absolute file path on success or NULL in case of failure
 */
extern char *util_get_real_path(char *path);

/**
 * @brief Checks if @a path is a one of special reserved strings.
 *
 * @param path a path to examine
 *
 * @return true if @a path is a special string, false otherwise.
 */
bool util_reserved_path_check(char *path);

/**
 * @brief Starts animation of specified notification icon.
 *
 * @param icon icon to animate
 */
extern void util_start_noti_ani(icon_s *icon);

/**
 * @brief Stops animation of specified notification icon.
 *
 * @param icon icon to stop animation
 */
extern void util_stop_noti_ani(icon_s *icon);

/**
 * @brief Sends status message.
 *
 * @param data win info
 * @param duration duration of the message
 */
extern void util_send_status_message_start(void *data, double duration);

/**
 * @brief Replaces char in string.
 *
 * @param to_replace char to replace
 * @param replacer char that will replace @a to_replace char
 */
extern void util_char_replace(char *text, char to_replace, char replacer);

/**
 * @brief Gets dynamic state
 *
 * @return state
 */
extern int util_dynamic_state_get(void);

/**
 * @brief Add monitor to file.
 *
 * @param file_path path to file to monitor
 * @param callback_func callback function to be invoked when file state will change
 * @param data data that will be passed to callback function
 *
 * @return pointer to monitor handler or NULL in case of failure
 */
extern Ecore_File_Monitor *util_file_monitor_add(const char *file_path, Ecore_File_Monitor_Cb callback_func, void *data);

/**
 * @brief Removes monitor to file.
 *
 * @param file_monitor file monitor handler
 */
extern void util_file_monitor_remove(Ecore_File_Monitor *file_monitor);

/**
 * @brief Gets substring that starts with specified @str_search string.
 *
 * @param substring or NULL if substring not found
 */
extern char *util_safe_str(const char *str, const char *str_search);

/**
 * @brief Checks if @a str starts with @a prefix
 *
 * @param[in] prefix prefix candidate
 * @param[in] str string to examine
 *
 * @return true if @a prefix is a prefix of @a str, false otherwise
 */
bool util_string_prefix_check(const char *prefix, const char *str);

#ifdef _SUPPORT_SCREEN_READER
extern Evas_Object *util_access_object_register(Evas_Object *object, Evas_Object *layout);
extern void util_access_object_unregister(Evas_Object *object);
extern void util_access_object_info_set(Evas_Object *object, int info_type, char *info_text);
extern void util_icon_access_register(icon_s *icon);
extern void util_icon_access_unregister(icon_s *icon);
#endif /** _SUPPORT_SCREEN_READER */

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/**
 * @brief Application sub-directories type.
 */
enum app_subdir {
	APP_DIR_DATA,
	APP_DIR_CACHE,
	APP_DIR_RESOURCE,
	APP_DIR_SHARED_RESOURCE,
	APP_DIR_SHARED_TRUSTED,
	APP_DIR_EXTERNAL_DATA,
	APP_DIR_EXTERNAL_CACHE,
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
#define util_get_shared_res_file_path(x) util_get_file_path(APP_DIR_SHARED_RESOURCE, (x))
#define util_get_trusted_file_path(x) util_get_file_path(APP_DIR_SHARED_TRUSTED, (x))
#define util_get_external_data_file_path(x) util_get_file_path(APP_DIR_EXTERNAL_DATA, (x))
#define util_get_external_cache_file_path(x) util_get_file_path(APP_DIR_EXTERNAL_CACHE, (x))

/**
 * @brief Initializes WiFi using wifi_initialize API
 * @remarks If WiFi is already initialized, #WIFI_ERROR_NONE will be returned.
 * @return 0 on success, other value on failure
 */
int util_wifi_initialize(void);

/**
 * @brief Deinitializes WiFi using wifi_deinitialize API
 * @remarks Only last call of that function is effective to avoid unwanted deinitialization.
 * @return 0 on success, other value on failure
 */
int util_wifi_deinitialize(void);

/**
 * @brief Allows to set multiple callbacks using wifi_set_connection_state_changed_cb API
 *
 * @param cb callback
 * @param data user_data passed to callback function.
 * @return 0 on success, other value on failure
 */
int util_wifi_set_connection_state_changed_cb(wifi_connection_state_changed_cb cb, void *data);

/**
 * @brief Unregisters callback set with util_wifi_set_connection_state_changed_cb.
 *
 * @param cb callback
 */
void util_wifi_unset_connection_state_changed_cb(wifi_connection_state_changed_cb cb);

/**
 * @brief Allows to set multiple callbacks using wifi_set_device_state_changed_cb API
 *
 * @param cb callback
 * @param data user_data passed to callback function.
 * @return 0 on success, other value on failure
 */
int util_wifi_set_device_state_changed_cb(wifi_device_state_changed_cb cb, void *data);

/**
 * @brief Unregisters callback set with util_wifi_set_device_state_changed_cb.
 *
 * @param cb callback
 */
void util_wifi_unset_device_state_changed_cb(wifi_device_state_changed_cb cb);


/**
 * @brief Allows to register multiple callbacks using system_settings_changed_cb API.
 *
 * @param key key to monitor.
 * @param cb callback.
 * @param data user_data passed to callback function.
 *
 * @return 0 on success, other value on failure.
 */
int util_system_settings_set_changed_cb(system_settings_key_e key, system_settings_changed_cb cb, void *data);

/**
 * @brief Unregisters callback set with util_system_settings_set_changed_cb.
 *
 * @param key key to stop monitor.
 * @param cb callback
 */
void util_system_settings_unset_changed_cb(system_settings_key_e key, system_settings_changed_cb cb);

/**
 * @brief Allows to register multiple callbacks using runtime_info_set_changed_cb API.
 *
 * @param key key to monitor.
 * @param cb callback.
 * @param data user data passed to callback function.
 *
 * @return 0 on success, other value on failure.
 */
int util_runtime_info_set_changed_cb(runtime_info_key_e key, runtime_info_changed_cb cb, void *data);

/**
 * @brief Unregisters callback set with util_runtime_info_set_changed_cb.
 *
 * @param key key to stop monitor.
 * @param cb callback.
 */
void util_runtime_info_unset_changed_cb(runtime_info_key_e key, runtime_info_changed_cb cb);

#endif /* __INDICATOR_UTIL_H__ */
