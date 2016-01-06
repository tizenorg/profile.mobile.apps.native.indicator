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
//#include <Ecore_X.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>
#include <system_settings.h>

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

int clock_mode = INDICATOR_CLOCK_MODE_12H;
int clock_hour = 0;
static const char *colon = ":";
static const char *ratio = "&#x2236;";

static int apm_length = 0;
static int apm_position = 0;
extern Ecore_Timer *clock_timer;

static UDateTimePatternGenerator *_last_generator = NULL;
static char *_last_locale = NULL;
static int battery_charging = 0;

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

static void ICU_set_timezone(const char *timezone);

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
	.lang_changed = NULL,
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
		udatpg_close(_last_generator);
		_last_generator = NULL;
	}
}



static UDateTimePatternGenerator *__cal_get_pattern_generator(const char *locale, UErrorCode *status)
{
	if (!_last_generator || !_last_locale || strcmp(locale, _last_locale)) {

		cal_delete_last_generator();

		_last_locale = strdup(locale);

		_last_generator = udatpg_open(locale, status);

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
		const char *region = NULL;

		int bRegioncheck = 0;
		char *lang1 = "it_IT";

		region = vconf_get_str(VCONFKEY_REGIONFORMAT);
		ret_if(!region);

		if (strncmp(region,lang1,strlen(lang1)) == 0) bRegioncheck = 1;

		if (apm_length>=4 || bRegioncheck==1) {
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



static void _clock_format_changed_cb(keynode_t *node, void *data)
{
	struct appdata *ad = NULL;
	int mode_24 = 0;

	ret_if(!data);

	ad = (struct appdata *)data;

	if (vconf_get_int(VCONFKEY_REGIONFORMAT_TIME1224,&mode_24) < 0)
	{
		ERR("Error getting VCONFKEY_REGIONFORMAT_TIME1224 value");
		return;
	}

	/* Check Time format. If timeformat have invalid value, Set to 12H */
	if( mode_24==VCONFKEY_TIME_FORMAT_24)
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

	char *timezone = util_get_timezone_str();
	ICU_set_timezone(timezone);
	indicator_clock_changed_cb(data);
	if(timezone!=NULL)
		free(timezone);
}



static void indicator_clock_charging_now_cb(keynode_t *node, void *data)
{
	int status = 0;

	retif(data == NULL, , "Invalid parameter!");


	vconf_get_int(VCONFKEY_SYSMAN_CHARGER_STATUS, &status);

	battery_charging = status;
}



static int language_changed_cb(void *data)
{
	const char *pa_lang = vconf_get_str(VCONFKEY_LANGSET);
	DBG("language_changed_cb %s",pa_lang);
	indicator_clock_changed_cb(data);
	return OK;
}



static int region_changed_cb(void *data)
{
	_clock_format_changed_cb(NULL, data);
	return OK;
}



static int wake_up_cb(void *data)
{
	indicator_clock_changed_cb(data);

	return OK;
}



/*static void _time_changed(system_settings_key_e key, void *data)
{
	DBG("_time_changed");
	_clock_format_changed_cb(NULL,data);
}*/



static void regionformat_changed(keynode_t *node, void *data)
{
	DBG("regionformat_changed");
	_clock_format_changed_cb(NULL,data);
}



static void timezone_int_changed(keynode_t *node, void *data)
{
	DBG("timezone_int_changed");
	_clock_format_changed_cb(NULL,data);
}



static void timezone_id_changed(keynode_t *node, void *data)
{
	char *szTimezone = NULL;
	szTimezone = vconf_get_str(VCONFKEY_SETAPPL_TIMEZONE_ID);

	DBG("timezone_id_changed %s",szTimezone);
	_clock_format_changed_cb(NULL,data);
}



static int register_clock_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	/*ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_TIME_CHANGED, _time_changed, data);
	if (ret != OK) {
		r = r | ret;
	}*/

	ret = vconf_notify_key_changed(VCONFKEY_REGIONFORMAT_TIME1224, regionformat_changed, data);
	if (ret != OK) {
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_TIMEZONE_INT, timezone_int_changed, data);
	if (ret != OK) {
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_TIMEZONE_ID, timezone_id_changed, data);
	if (ret != OK) {
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_REGIONFORMAT, regionformat_changed, data);
	if (ret != OK) {
		r = r | ret;
	}
	_clock_format_changed_cb(NULL, data);
	indicator_clock_charging_now_cb(NULL,data);

	return r;
}



static int unregister_clock_module(void)
{
	int ret;

	//ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_TIME_CHANGED);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_REGIONFORMAT_TIME1224, regionformat_changed);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_SETAPPL_TIMEZONE_INT, timezone_int_changed);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_SETAPPL_TIMEZONE_ID, timezone_id_changed);
	ret = ret | vconf_ignore_key_changed(VCONFKEY_REGIONFORMAT, regionformat_changed);

	if (clock_timer != NULL) {
		ecore_timer_del(clock_timer);
		clock_timer = NULL;
	}

	cal_delete_last_generator();

	return ret;
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
	retif(data == NULL, , "Data parameter is NULL");
	retif(output == NULL, , "output parameter is NULL");


	UChar customSkeleton[CLOCK_STR_LEN] = { 0, };
	UChar u_timezone[64] = {0,};

	UErrorCode status = U_ZERO_ERROR;
	UDateFormat *formatter = NULL;

	UChar bestPattern[CLOCK_STR_LEN] = { 0, };
	UChar formatted[CLOCK_STR_LEN] = { 0, };

	char bestPatternString[CLOCK_STR_LEN] = { 0, };
	char formattedString[CLOCK_STR_LEN] = { 0, };

	UDateTimePatternGenerator *pattern_generator = NULL;

	char *time_skeleton = "hhmm";

	char* timezone_id = NULL;
	timezone_id = util_get_timezone_str();

	char *locale = vconf_get_str(VCONFKEY_REGIONFORMAT);
	if(locale == NULL)
	{
		ERR("[Error] get value of fail.");
		free(timezone_id);
		return;
	}

	/* Remove ".UTF-8" in locale */
	char locale_tmp[32] = {0,};
	strncpy(locale_tmp, locale, sizeof(locale_tmp)-1);
	char *p = util_safe_str(locale_tmp, ".UTF-8");
	if (p) {
		*p = 0;
	}

	u_uastrncpy(customSkeleton, time_skeleton, strlen(time_skeleton));

	pattern_generator = __cal_get_pattern_generator (locale_tmp, &status);
	if (pattern_generator == NULL) {
		return ;
	}

	int32_t bestPatternCapacity = (int32_t) (sizeof(bestPattern) / sizeof((bestPattern)[0]));
	(void)udatpg_getBestPattern(pattern_generator, customSkeleton,
				    u_strlen(customSkeleton), bestPattern,
				    bestPatternCapacity, &status);

	u_austrcpy(bestPatternString, bestPattern);
	u_uastrcpy(bestPattern,"a");

	DBG("TimeZone is %s", timezone_id);

	if(bestPatternString[0] == 'a')
	{
		apm_position = 0;
	}
	else
	{
		apm_position = 1;
	}

	UDate date = ucal_getNow();
	if(timezone_id)
	{
		u_uastrncpy(u_timezone, timezone_id, sizeof(u_timezone));
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale_tmp, u_timezone, -1, bestPattern, -1, &status);
	}
	else
	{
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale_tmp, NULL, -1, bestPattern, -1, &status);
	}
	if (formatter == NULL) {
		return ;
	}

	int32_t formattedCapacity = (int32_t) (sizeof(formatted) / sizeof((formatted)[0]));
	(void)udat_format(formatter, date, formatted, formattedCapacity, NULL, &status);
	u_austrcpy(formattedString, formatted);

	apm_length = u_strlen(formatted);

	udat_close(formatter);

	if(strlen(formattedString)<CLOCK_STR_LEN)
	{
		strncpy(output,formattedString,strlen(formattedString));
	}
	else
	{
		strncpy(output,formattedString,CLOCK_STR_LEN-1);
	}

	return;
}



void indicator_get_time_by_region(char* output,void *data)
{
	retif(data == NULL, , "Data parameter is NULL");
	retif(output == NULL, , "output parameter is NULL");

	UChar customSkeleton[CLOCK_STR_LEN] = { 0, };
	UChar u_timezone[64] = {0,};

	UErrorCode status = U_ZERO_ERROR;
	UDateFormat *formatter = NULL;

	UChar bestPattern[CLOCK_STR_LEN] = { 0, };
	UChar formatted[CLOCK_STR_LEN] = { 0, };

	char bestPatternString[CLOCK_STR_LEN] = { 0, };
	char formattedString[CLOCK_STR_LEN] = { 0, };
	char* convertFormattedString = NULL;

	char time_skeleton[20] = {0,};
	UDateTimePatternGenerator *pattern_generator = NULL;

	if(clock_mode == INDICATOR_CLOCK_MODE_12H)
	{
		strcpy(time_skeleton,"hm");
	}
	else
	{
		strcpy(time_skeleton,"Hm");
	}
	char* timezone_id = NULL;
	timezone_id = util_get_timezone_str();

	char *locale = vconf_get_str(VCONFKEY_REGIONFORMAT);
	if(locale == NULL)
	{
		ERR("[Error] get value of fail.");
		free(timezone_id);
		return;
	}

	/* Remove ".UTF-8" in locale */
	char locale_tmp[32] = {0,};
	strncpy(locale_tmp, locale, sizeof(locale_tmp)-1);
	char *p = util_safe_str(locale_tmp, ".UTF-8");
	if (p) {
		*p = 0;
	}

	u_uastrncpy(customSkeleton, time_skeleton, strlen(time_skeleton));

	pattern_generator = __cal_get_pattern_generator (locale_tmp, &status);
	if (pattern_generator == NULL) {
		return ;
	}

	int32_t bestPatternCapacity = (int32_t) (sizeof(bestPattern) / sizeof((bestPattern)[0]));
	(void)udatpg_getBestPattern(pattern_generator, customSkeleton,
				    u_strlen(customSkeleton), bestPattern,
				    bestPatternCapacity, &status);

	char a_best_pattern[64] = {0,};
	u_austrcpy(a_best_pattern, bestPattern);
	char *a_best_pattern_fixed = strtok(a_best_pattern, "a");
	a_best_pattern_fixed = strtok(a_best_pattern_fixed, " ");
	if(a_best_pattern_fixed)
	{
		u_uastrcpy(bestPattern, a_best_pattern_fixed);
	}

	u_austrcpy(bestPatternString, bestPattern);

	DBG("BestPattern is %s", bestPatternString);
	DBG("TimeZone is %s", timezone_id);

	UDate date = ucal_getNow();
	if(timezone_id)
	{
		u_uastrncpy(u_timezone, timezone_id, sizeof(u_timezone));
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale_tmp, u_timezone, -1, bestPattern, -1, &status);
	}
	else
	{
		formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale_tmp, NULL, -1, bestPattern, -1, &status);
	}

	if (formatter == NULL) {
		return;
	}
	int32_t formattedCapacity = (int32_t) (sizeof(formatted) / sizeof((formatted)[0]));
	(void)udat_format(formatter, date, formatted, formattedCapacity, NULL, &status);
	u_austrcpy(formattedString, formatted);
	DBG("DATE & TIME is %s %s %d %s", locale_tmp, formattedString, u_strlen(formatted), bestPatternString);

	DBG("24H :: Before change %s", formattedString);
	convertFormattedString = _string_replacer(formattedString, colon, ratio);
	DBG("24H :: After change %s", convertFormattedString);

	if(convertFormattedString == NULL)
	{
		DBG("_string_replacer return NULL");
		udat_close(formatter);
		return;
	}

	udat_close(formatter);

	if(strlen(convertFormattedString)<CLOCK_STR_LEN)
	{
		strncpy(output,convertFormattedString,strlen(convertFormattedString));
	}
	else
	{
		strncpy(output,convertFormattedString,CLOCK_STR_LEN-1);
	}

	if(convertFormattedString != NULL)
	{
		free(convertFormattedString);
		convertFormattedString = NULL;
	}

	return;
}



static UChar *uastrcpy(const char *chars)
{
	int len = 0;
	UChar *str = NULL;
	len = strlen(chars);
	str = (UChar *) malloc(sizeof(UChar) *(len + 1));
	if (!str)
		return NULL;
	u_uastrcpy(str, chars);
	return str;
}



static void ICU_set_timezone(const char *timezone)
{
	if(timezone == NULL)
	{
		ERR("TIMEZONE is NULL");
		return;
	}

	UErrorCode ec = U_ZERO_ERROR;
	UChar *str = uastrcpy(timezone);

	ucal_setDefaultTimeZone(str, &ec);
	if (U_SUCCESS(ec)) {
	} else {
		DBG("ucal_setDefaultTimeZone() FAILED : %s ",
			      u_errorName(ec));
	}
	free(str);
}



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
	int battery_capa = 0;
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


	ret = vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, &battery_capa);
	if (ret != OK)
	{
		return NULL;
	}
	if (battery_capa < 0)
	{
		return NULL;
	}

	if (battery_capa > 100)
		battery_capa = 100;
	snprintf(buf1, sizeof(buf1), _("IDS_IDLE_BODY_PD_PERCENT_OF_BATTERY_POWER_REMAINING"), battery_capa);

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

