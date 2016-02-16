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
//#include <Ecore_X.h>
#include <utils_i18n.h>
#include <system_settings.h>
#include <device/battery.h>

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "indicator_gui.h"
#include "icon.h"
#include "util.h"
#include "modules.h"
#include "box.h"
#include "log.h"

#define SYSTEM_RESUME		"system_wakeup"

#define TIME_FONT_SIZE_24		ELM_SCALE_SIZE(30)
#define TIME_FONT_SIZE_12		ELM_SCALE_SIZE(30)
#define AMPM_FONT_SIZE		ELM_SCALE_SIZE(29)

#define TIME_FONT_COLOR		200, 200, 200, 255
#define AMPM_FONT_COLOR		200, 200, 200, 255
#define LABEL_STRING		"<font_size=%d>%s" \
	"</font_size>"
#define LABEL_STRING_FONT		"%s</font>"

#define BATTERY_TIMER_INTERVAL		3
#define BATTERY_TIMER_INTERVAL_CHARGING	30

#define CLOCK_STR_LEN 128

enum {
	INDICATOR_CLOCK_MODE_12H = 0,
	INDICATOR_CLOCK_MODE_24H,
	INDICATOR_CLOCK_MODE_MAX
};

static system_settings_key_e clock_callback_array[] = {
	SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
	SYSTEM_SETTINGS_KEY_TIME_CHANGED,
	SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY,
	SYSTEM_SETTINGS_KEY_LOCALE_TIMEZONE,
};

int clock_mode = INDICATOR_CLOCK_MODE_12H;
int clock_hour = 0;
static const char *colon = ":";
static const char *ratio = "&#x2236;";

static int apm_length = 0;
static int apm_position = 0;
extern Ecore_Timer *clock_timer;

static i18n_udatepg_h _last_generator;
static char *_last_locale = NULL;

static int register_clock_module(void *data);
static int unregister_clock_module(void);
static int language_changed_cb(void *data);
static int region_changed_cb(void *data);
static int wake_up_cb(void *data);
#ifdef _SUPPORT_SCREEN_READER
static int register_clock_tts(void *data,int win_type);
#endif

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED8
#define MODULE_NAME		"clock"

static void indicator_get_apm_by_region(char* output, void* data);
static void indicator_get_time_by_region(char* output, void* data);

#ifdef _SUPPORT_SCREEN_READER
static void ICU_set_timezone(const char *timezone);
#endif

icon_s sysclock = {
	.type = INDICATOR_TXT_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.init = register_clock_module,
	.fini = unregister_clock_module,
	.region_changed = region_changed_cb,
	.lang_changed = language_changed_cb,
	.wake_up = wake_up_cb,
};



void cal_delete_last_generator(void)
{
	if (_last_locale) {
		free(_last_locale);
		_last_locale = NULL;
	}
	if (_last_generator) {
		i18n_udatepg_destroy(_last_generator);
		_last_generator = NULL;
	}
}



static i18n_udatepg_h __cal_get_pattern_generator(const char *locale, int *status)
{
	if (!_last_generator || !_last_locale || strcmp(locale, _last_locale)) {

		cal_delete_last_generator();

		_last_locale = strdup(locale);

		int ret = i18n_udatepg_create(locale, &_last_generator);
		if (ret != I18N_ERROR_NONE) {
			_E("i18n_udatepg_create failed %d", ret);
			_last_generator = NULL;
		}
	}
	return _last_generator;
}



static void set_app_state(void* data)
{
	sysclock.ad = data;
}



static void indicator_clock_changed_cb(void *data)
{
	char time_str[CLOCK_STR_LEN] = {0,};
	char time_buf[CLOCK_STR_LEN] = {0,};
	char ampm_buf[CLOCK_STR_LEN] = {0,};
	char ampm_str[CLOCK_STR_LEN] = {0,};
	char buf[CLOCK_STR_LEN] = {0,};
	char result[CLOCK_STR_LEN] = {0,};
	char icu_apm[CLOCK_STR_LEN] = {0,};

	struct tm *ts = NULL;
	time_t ctime;
	struct appdata *ad = NULL;
	int len;
	int font_size;
	int ampm_size = AMPM_FONT_SIZE;

	ret_if(!data);

	ad = (struct appdata *)data;

	if (icon_get_update_flag() == 0) return;

	/* Set time */
	ctime = time(NULL);
	ts = localtime(&ctime);
	if (ts == NULL) {
		_E("Fail to get localtime !");
		return;
	}

	if (clock_timer != NULL) {
		ecore_timer_del(clock_timer);
		clock_timer = NULL;
	}

	memset(time_str, 0x00, sizeof(time_str));
	memset(ampm_str, 0x00, sizeof(ampm_str));
	memset(time_buf, 0x00, sizeof(time_buf));
	memset(ampm_buf, 0x00, sizeof(ampm_buf));
	memset(buf, 0x00, sizeof(buf));

	clock_timer = ecore_timer_add(60 - ts->tm_sec, (void *)indicator_clock_changed_cb, data);
	if(!clock_timer) {
		_E("Fail to add timer !");
	}

	indicator_get_apm_by_region(icu_apm,data);
	indicator_get_time_by_region(time_buf,data);


	if (clock_mode == INDICATOR_CLOCK_MODE_12H) {
		char bf1[32] = { 0, };
		int hour;
		static int pre_hour = 0;

		if (apm_length>=4) {
			if (ts->tm_hour >= 0 && ts->tm_hour < 12) {
				snprintf(ampm_buf, sizeof(ampm_buf),"%s","AM");
			} else {
				snprintf(ampm_buf, sizeof(ampm_buf),"%s","PM");
			}
		} else {
			snprintf(ampm_buf, sizeof(ampm_buf),"%s",icu_apm);
		}

		strftime(bf1, sizeof(bf1), "%l", ts);
		hour = atoi(bf1);
		strftime(bf1, sizeof(bf1), ":%M", ts);

		font_size = TIME_FONT_SIZE_12;
		clock_hour = hour;

		if ((pre_hour<10 && hour>=10)||(pre_hour>=10 && hour<10)) {
			box_update_display(&(ad->win));
		}

		pre_hour = hour;
	} else {
		font_size = TIME_FONT_SIZE_24;
	}

	snprintf(time_str, sizeof(time_str), LABEL_STRING, font_size, time_buf);
	snprintf(ampm_str, sizeof(ampm_str), LABEL_STRING, ampm_size, ampm_buf);

	if (clock_mode == INDICATOR_CLOCK_MODE_12H) {
		if (apm_position == 0) {
			len = snprintf(buf, sizeof(buf), "%s %s", ampm_str, time_str);
		} else {
			len = snprintf(buf, sizeof(buf), "%s %s", time_str, ampm_str);
		}
	} else {
		len = snprintf(buf, sizeof(buf), "%s", time_str);
	}

	snprintf(result, sizeof(result), LABEL_STRING_FONT, buf);
	if (len < 0) {
		_E("Unexpected ERROR!");
		return;
	}

	_D("[CLOCK MODULE] Timer Status : %d Time: %s", clock_timer, result);
	util_part_text_emit(data, "elm.text.clock", result);

	return;
}



static void clock_format_changed(void *data)
{
	struct appdata *ad = NULL;
	bool mode_24 = 0;
	int ret = -1;
	i18n_timezone_h timezone;

	ret_if(!data);

	ad = (struct appdata *)data;

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &mode_24);
	retm_if(ret != SYSTEM_SETTINGS_ERROR_NONE, "Error getting time format value");

	/* Check Time format. If timeformat have invalid value, Set to 12H */
	if (mode_24)
	{
		if(clock_mode == INDICATOR_CLOCK_MODE_12H)
		{
			clock_mode = INDICATOR_CLOCK_MODE_24H;
			box_update_display(&(ad->win));
		}
	}
	else
	{
		if(clock_mode==INDICATOR_CLOCK_MODE_24H)
		{
			clock_mode = INDICATOR_CLOCK_MODE_12H;
			box_update_display(&(ad->win));
		}
	}

	char *timezone_str = util_get_timezone_str();

	ret = i18n_timezone_create(&timezone, timezone_str);
	if (ret != I18N_ERROR_NONE) {
		_E("Unable to create timzone handle for %s: %d", timezone_str, ret);
		free(timezone_str);
		return;
	}

	ret = i18n_timezone_set_default(timezone);
	if (ret != I18N_ERROR_NONE) {
		_E("Unable to set default timzone: %d", ret);
		i18n_timezone_destroy(timezone);
		free(timezone_str);
		return;
	}

	indicator_clock_changed_cb(data);
	i18n_timezone_destroy(timezone);
	free(timezone_str);
}



static int language_changed_cb(void *data)
{
	char *pa_lang;
	int ret = -1;

	ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &pa_lang);
	retv_if(ret != SYSTEM_SETTINGS_ERROR_NONE, FAIL);

	DBG("language_changed_cb %s",pa_lang);
	indicator_clock_changed_cb(data);

	free(pa_lang);

	return OK;
}



static int region_changed_cb(void *data)
{
	clock_format_changed(data);
	return OK;
}



static int wake_up_cb(void *data)
{
	indicator_clock_changed_cb(data);

	return OK;
}



static void time_format_changed(system_settings_key_e key, void *data)
{
	DBG("time format changed");
	clock_format_changed(data);
}



static int register_clock_module(void *data)
{
	int r = 0;
	int ret = -1;
	int i;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	for(i = 0; i < ARRAY_SIZE(clock_callback_array); ++i) {

		ret = util_system_settings_set_changed_cb(clock_callback_array[i], time_format_changed, data);

		if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
			r = r | ret;
		}
	}

	clock_format_changed(data);

	return r;
}



static int unregister_clock_module(void)
{
	int i;
	for(i = 0; i < ARRAY_SIZE(clock_callback_array); ++i)
		util_system_settings_unset_changed_cb(clock_callback_array[i], time_format_changed);


	if (clock_timer != NULL) {
		ecore_timer_del(clock_timer);
		clock_timer = NULL;
	}

	cal_delete_last_generator();

	return 0;
}



static inline char *_extend_heap(char *buffer, int *sz, int incsz)
{
	char *tmp;

	*sz += incsz;
	tmp = realloc(buffer, *sz);
	if (!tmp) {
		ERR("Heap");
		return NULL;
	}

	return tmp;
}



static char *_string_replacer(const char *src, const char *pattern, const char *replace)
{
	const char *ptr;
	char *tmp = NULL;
	char *ret = NULL;
	int idx = 0;
	int out_idx = 0;
	int out_sz = 0;
	enum {
		STATE_START,
		STATE_FIND,
		STATE_CHECK,
		STATE_END,
	} state;

	if (!src || !pattern)
		return NULL;

	out_sz = strlen(src);
	ret = strdup(src);
	if (!ret) {
		ERR("Heap");
		return NULL;
	}

	out_idx = 0;
	for (state = STATE_START, ptr = src; state != STATE_END; ptr++) {
		switch (state) {
		case STATE_START:
			if (*ptr == '\0') {
				state = STATE_END;
			} else if (!isblank(*ptr)) {
				state = STATE_FIND;
				ptr--;
			}
			break;
		case STATE_FIND:
			if (*ptr == '\0') {
				state = STATE_END;
			} else if (*ptr == *pattern) {
				state = STATE_CHECK;
				ptr--;
				idx = 0;
			} else {
				ret[out_idx] = *ptr;
				out_idx++;
				if (out_idx == out_sz) {
					tmp = _extend_heap(ret, &out_sz, strlen(replace) + 1);
					if (!tmp) {
						free(ret);
						return NULL;
					}
					ret = tmp;
				}
			}
			break;
		case STATE_CHECK:
			if (!pattern[idx]) {
				/*!
     * If there is no space for copying the replacement,
     * Extend size of the return buffer.
     */
				if (out_sz - out_idx < strlen(replace) + 1) {
					tmp = _extend_heap(ret, &out_sz, strlen(replace) + 1);
					if (!tmp) {
						free(ret);
						return NULL;
					}
					ret = tmp;
				}

				strcpy(ret + out_idx, replace);
				out_idx += strlen(replace);

				state = STATE_FIND;
				ptr--;
			} else if (*ptr != pattern[idx]) {
				ptr -= idx;

				/* Copy the first matched character */
				ret[out_idx] = *ptr;
				out_idx++;
				if (out_idx == out_sz) {
					tmp = _extend_heap(ret, &out_sz, strlen(replace) + 1);
					if (!tmp) {
						free(ret);
						return NULL;
					}

					ret = tmp;
				}

				state = STATE_FIND;
			} else {
				idx++;
			}
			break;
		default:
			break;
		}
	}

	ret[out_idx] = '\0';
	return ret;
}



void indicator_get_apm_by_region(char* output,void *data)
{
	int ret = -1;
	char *locale = NULL;
	retif(data == NULL, , "Data parameter is NULL");
	retif(output == NULL, , "output parameter is NULL");

	i18n_uchar u_custom_skeleton[CLOCK_STR_LEN] = { 0, };
	i18n_uchar u_timezone[64] = {0,};
	i18n_uchar u_best_pattern[CLOCK_STR_LEN] = { 0, };
	i18n_uchar u_formatted[CLOCK_STR_LEN] = { 0, };

	i18n_udate_format_h formatter;
	int32_t best_pattern_len, formatted_len;

	char s_best_pattern[CLOCK_STR_LEN] = { 0, };
	char s_formatted[CLOCK_STR_LEN] = { 0, };

	int status = 0;

	i18n_udatepg_h pattern_generator = NULL;

	ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &locale);
	ret_if(ret != SYSTEM_SETTINGS_ERROR_NONE);

	DBG("Locale: %s", locale);

	retm_if(locale == NULL, "[Error] get value of fail.");

	i18n_ustring_copy_ua_n(u_custom_skeleton, "hhmm", ARRAY_SIZE(u_custom_skeleton));

	pattern_generator = __cal_get_pattern_generator (locale, &status);
	if (pattern_generator == NULL) {
		free(locale);
		return ;
	}

	ret = i18n_udatepg_get_best_pattern(pattern_generator, u_custom_skeleton, i18n_ustring_get_length(u_custom_skeleton),
			u_best_pattern, (int32_t)ARRAY_SIZE(u_best_pattern), &best_pattern_len);
	if (ret != I18N_ERROR_NONE) {
		_E("i18n_udatepg_get_best_pattern failed: %d", ret);
		free(locale);
		return;
	}

	i18n_ustring_copy_au(s_best_pattern, u_best_pattern);
	i18n_ustring_copy_ua(u_best_pattern, "a");

	char *timezone_id = util_get_timezone_str();
	DBG("TimeZone is %s", timezone_id);

	if (s_best_pattern[0] == 'a') {
		apm_position = 0;
	}
	else {
		apm_position = 1;
	}

	i18n_udate date;
	ret = i18n_ucalendar_get_now(&date);
	if (ret != I18N_ERROR_NONE) {
		ERR("i18n_ucalendar_get_now failed: %d", ret);
		free(locale);
		free(timezone_id);
		return;
	}
	if (timezone_id) {
		i18n_ustring_copy_ua_n(u_timezone, timezone_id, ARRAY_SIZE(u_timezone));
	}

	ret = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, locale, timezone_id ? u_timezone : NULL, -1,
			u_best_pattern, -1, &formatter);
	if (ret != I18N_ERROR_NONE) {
		free(locale);
		free(timezone_id);
		return;
	}

	free(locale);
	free(timezone_id);

	ret = i18n_udate_format_date(formatter, date, u_formatted, ARRAY_SIZE(s_formatted), NULL, &formatted_len);
	if (ret != I18N_ERROR_NONE) {
		i18n_udate_destroy(formatter);
		return;
	}

	i18n_udate_destroy(formatter);

	i18n_ustring_copy_au(s_formatted, u_formatted);
	apm_length = i18n_ustring_get_length(u_formatted);

	if (strlen(s_formatted) < CLOCK_STR_LEN) {
		strncpy(output, s_formatted, strlen(s_formatted));
	}
	else {
		strncpy(output, s_formatted, CLOCK_STR_LEN - 1);
	}

	return;
}



void indicator_get_time_by_region(char* output,void *data)
{
	int ret = -1;
	char *locale;
	retif(data == NULL, , "Data parameter is NULL");
	retif(output == NULL, , "output parameter is NULL");

	i18n_uchar u_custom_skeleton[CLOCK_STR_LEN] = { 0, };
	i18n_uchar u_timezone[64] = {0,};
	i18n_uchar u_best_pattern[CLOCK_STR_LEN] = { 0, };
	i18n_uchar u_formatted[CLOCK_STR_LEN] = { 0, };

	int status = 0;
	i18n_udate_format_h formatter = NULL;

	char s_best_pattern[CLOCK_STR_LEN] = { 0, };
	char s_formatted[CLOCK_STR_LEN] = { 0, };
	char *s_convert_formatted = NULL;

	char s_time_skeleton[20] = {0,};
	i18n_udatepg_h pattern_generator = NULL;

	int32_t best_pattern_len, formatted_len;

	if (clock_mode == INDICATOR_CLOCK_MODE_12H) {
		strcpy(s_time_skeleton, "hm");
	}
	else {
		strcpy(s_time_skeleton, "Hm");
	}

	ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &locale);
	retm_if(ret != SYSTEM_SETTINGS_ERROR_NONE, "Cannot get LOCALE_COUNTRY string");
	DBG("Locale: %s", locale);

	if (locale == NULL) {
		ERR("[Error] get value of fail.");
		return;
	}

	i18n_ustring_copy_ua_n(u_custom_skeleton, s_time_skeleton, ARRAY_SIZE(u_custom_skeleton));

	pattern_generator = __cal_get_pattern_generator (locale, &status);
	if (pattern_generator == NULL) {
		free(locale);
		return;
	}

	ret = i18n_udatepg_get_best_pattern(pattern_generator, u_custom_skeleton, i18n_ustring_get_length(u_custom_skeleton),
				u_best_pattern, ARRAY_SIZE(u_best_pattern), &best_pattern_len);
	if (ret != I18N_ERROR_NONE) {
		_E("i18n_udatepg_get_best_pattern failed: %d", ret);
		free(locale);
		return;
	}

	char a_best_pattern[64] = {0,};
	i18n_ustring_copy_au(a_best_pattern, u_best_pattern);

	char *a_best_pattern_fixed = strtok(a_best_pattern, "a");
	a_best_pattern_fixed = strtok(a_best_pattern_fixed, " ");
	if (a_best_pattern_fixed) {
		i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_fixed);
	}

	i18n_ustring_copy_au(s_best_pattern, u_best_pattern);

	DBG("BestPattern is %s", s_best_pattern);

	i18n_udate date;
	ret = i18n_ucalendar_get_now(&date);
	if (ret != I18N_ERROR_NONE) {
		ERR("i18n_ucalendar_get_now failed: %d", ret);
		free(locale);
		return;
	}

	char* timezone_id = util_get_timezone_str();
	DBG("TimeZone is %s", timezone_id);

	if (timezone_id) {
		i18n_ustring_copy_ua_n(u_timezone, timezone_id, ARRAY_SIZE(u_timezone));
	}

	ret = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, locale, timezone_id ? u_timezone : NULL, -1,
			u_best_pattern, -1, &formatter);
	if (ret != I18N_ERROR_NONE) {
		free(locale);
		free(timezone_id);
		return;
	}

	free(timezone_id);

	ret = i18n_udate_format_date(formatter, date, u_formatted, ARRAY_SIZE(s_formatted), NULL, &formatted_len);
	if (ret != I18N_ERROR_NONE) {
		free(locale);
		i18n_udate_destroy(formatter);
		return;
	}

	i18n_udate_destroy(formatter);

	i18n_ustring_copy_au(s_formatted, u_formatted);
	DBG("DATE & TIME is %s %s %d %s", locale, s_formatted, i18n_ustring_get_length(u_formatted), s_best_pattern);

	free(locale);

	DBG("24H :: Before change %s", s_formatted);
	s_convert_formatted = _string_replacer(s_formatted, colon, ratio);
	DBG("24H :: After change %s", s_convert_formatted);

	if (!s_convert_formatted) {
		DBG("_string_replacer return NULL");
		return;
	}

	if (strlen(s_convert_formatted) < CLOCK_STR_LEN) {
		strncpy(output, s_convert_formatted, strlen(s_convert_formatted));
	}
	else {
		strncpy(output, s_convert_formatted, CLOCK_STR_LEN - 1);
	}

	free(s_convert_formatted);

	return;
}


#ifdef _SUPPORT_SCREEN_READER
static void ICU_set_timezone(const char *timezone)
{
	i18n_timezone_h tmz;

	if (timezone == NULL) {
		ERR("TIMEZONE is NULL");
		return;
	}

	int ret = i18n_timezone_create(&tmz, timezone);
	if (ret != I18N_ERROR_NONE) {
		ERR("Unable to create timezone handle from %s: %d", timezone, ret);
		return;
	}

	ret = i18n_timezone_set_default(tmz);
	if (ret != I18N_ERROR_NONE) {
		ERR("Unable to set default timezone to %s: %d", timezone, ret);
	}

	i18n_timezone_destroy(tmz);
}
#endif



#ifdef _SUPPORT_SCREEN_READER
static char *_access_info_cb(void *data, Evas_Object *obj)
{
	Evas_Object *item = data;
	char *tmp = NULL;
	char time_str[32];
	char time_buf[128], ampm_buf[128];
	char buf[CLOCK_STR_LEN];
	char buf1[CLOCK_STR_LEN];
	int ret = 0;
	int battery_percentage = 0;
	int hour = 0;
	int minute = 0;
	char strHour[128] = { 0, };
	char strMin[128] = { 0, };


	struct tm *ts = NULL;
	time_t ctime;
	int len;

	retif(data == NULL,NULL, "Invalid parameter!");
	char *timezone = util_get_timezone_str();
	ICU_set_timezone(timezone);
	if(timezone!=NULL)
		free(timezone);

	/* Set time */
	ctime = time(NULL);
	ts = localtime(&ctime);
	if (ts == NULL)
		return NULL;

	memset(time_str, 0x00, sizeof(time_str));
	memset(time_buf, 0x00, sizeof(time_buf));
	memset(ampm_buf, 0x00, sizeof(ampm_buf));
	memset(buf, 0x00, sizeof(buf));
	memset(buf1, 0x00, sizeof(buf1));

	if (clock_mode == INDICATOR_CLOCK_MODE_12H) {
		char bf1[32] = { 0, };

		if (ts->tm_hour >= 0 && ts->tm_hour < 12)
			strncpy(ampm_buf, _("IDS_IDLE_OPT_AM_ABB"),sizeof(ampm_buf)-1);
		else
			strncpy(ampm_buf, _("IDS_IDLE_OPT_PM_ABB"),sizeof(ampm_buf)-1);

		strftime(bf1, sizeof(bf1), "%l", ts);
		hour = atoi(bf1);
		strftime(bf1, sizeof(bf1), "%M", ts);
		minute = atoi(bf1);
	}
	else{
		char bf1[32] = { 0, };

		strftime(bf1, sizeof(bf1), "%H", ts);
		hour = atoi(bf1);
		strftime(bf1, sizeof(bf1), "%M", ts);
		minute = atoi(bf1);
	}

	if(hour ==1)
	{
		strncpy(strHour, _("IDS_COM_BODY_1_HOUR"),sizeof(strHour));
	}
	else
	{
		snprintf(strHour, sizeof(strHour), _("IDS_COM_POP_PD_HOURS"),hour);
	}

	if(minute ==1)
	{
		strncpy(strMin, _("IDS_COM_BODY_1_MINUTE"),sizeof(strMin));
	}
	else
	{
		snprintf(strMin, sizeof(strMin), _("IDS_COM_BODY_PD_MINUTES"),minute);
	}

	if(clock_mode == INDICATOR_CLOCK_MODE_12H)
		snprintf(time_str, sizeof(time_str), "%s, %s, %s", strHour, strMin,ampm_buf);
	else
		snprintf(time_str, sizeof(time_str), "%s, %s", strHour, strMin);


	ret = device_battery_get_percent(&battery_percentage);
	if (ret != DEVICE_ERROR_NONE)
	{
		return NULL;
	}

	snprintf(buf1, sizeof(buf1), _("IDS_IDLE_BODY_PD_PERCENT_OF_BATTERY_POWER_REMAINING"), battery_percentage);

	snprintf(buf, sizeof(buf), "%s, %s, %s", time_str, buf1, _("IDS_IDLE_BODY_STATUS_BAR_ITEM"));

	DBG("buf: %s", buf);
	tmp = strdup(buf);
	if (!tmp) return NULL;
	return tmp;
}



static int register_clock_tts(void *data,int win_type)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	Evas_Object *to = NULL;
	Evas_Object *ao = NULL;
	struct appdata *ad = data;

	to = (Evas_Object *) edje_object_part_object_get(elm_layout_edje_get(ad->win[win_type].layout), "elm.rect.clock.access");
	ao = util_access_object_register(to, ad->win[win_type].layout);
	util_access_object_info_cb_set(ao,ELM_ACCESS_INFO,_access_info_cb,data);
	return 0;
}
#endif

