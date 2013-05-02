/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>
#include <runtime_info.h>
#include <Ecore_X.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>

#include "common.h"
#include "indicator.h"
#include "indicator_ui.h"
#include "indicator_gui.h"
#include "indicator_icon_util.h"
#include "indicator_util.h"
#include "modules.h"

#define SYSTEM_RESUME				"system_wakeup"

#define TIME_FONT_SIZE_24	34
#define TIME_FONT_SIZE_12	30
#define TIME_FONT_SIZE_BATTERY	32
#define TIME_FONT_COLOR		243, 243, 243, 255

#define AMPM_FONT_SIZE		24
#define AMPM_FONT_COLOR		243, 243, 243, 255
#define LABEL_STRING		"<font_size=%d>%s" \
				"</font_size></font>"

#define BATTERY_TIMER_INTERVAL		3
#define BATTERY_TIMER_INTERVAL_CHARGING	30

#define CLOCK_STR_LEN 256

enum {
	INDICATOR_CLOCK_MODE_12H = 0,
	INDICATOR_CLOCK_MODE_24H,
	INDICATOR_CLOCK_MODE_MAX
};

static int notifd;
static int clock_mode = INDICATOR_CLOCK_MODE_12H;
static int apm_length = 0;
static int apm_position = 0;
static Ecore_Timer *timer = NULL;
static Ecore_Timer *battery_timer = NULL;
static Ecore_Timer *battery_charging_timer = NULL;
static int battery_charging = 0;
static int battery_charging_first = 0;

static int register_clock_module(void *data);
static int unregister_clock_module(void);
static int language_changed_cb(void *data);
static int region_changed_cb(void *data);
static int wake_up_cb(void *data);

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED6
#define MODULE_NAME		"clock"

static void indicator_get_time_by_region(char* output, void* data);
static void ICU_set_timezone(const char *timezone);
static void indicator_clock_display_battery_percentage(void *data,int win_type );

Indicator_Icon_Object sysclock[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_TXT_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.init = register_clock_module,
	.fini = unregister_clock_module,
	.lang_changed = NULL,
	.region_changed = region_changed_cb,
	.lang_changed = language_changed_cb,
	.wake_up = wake_up_cb
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_TXT_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.init = register_clock_module,
	.fini = unregister_clock_module,
	.lang_changed = NULL,
	.region_changed = region_changed_cb,
	.lang_changed = language_changed_cb,
	.wake_up = wake_up_cb
}
};

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		sysclock[i].ad = data;
	}
}

static void indicator_clock_changed_cb(void *data)
{
	char time_str[32];
	char time_buf[128], ampm_buf[128];
	char buf[CLOCK_STR_LEN];
	char icu_apm[CLOCK_STR_LEN] = {0,};

	struct tm *ts = NULL;
	time_t ctime;
	int len;
	int font_size;

	retif(data == NULL, , "Invalid parameter!");

	if(indicator_util_get_update_flag()==0)
	{
		DBG("need to update");
		return;
	}

	if (battery_timer != NULL || battery_charging_timer != NULL)
	{
		DBG("battery is displaying. ignore clock callback");
		return;
	}

	ctime = time(NULL);
	ts = localtime(&ctime);
	if (ts == NULL)
		return;

	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
	}

	memset(time_str, 0x00, sizeof(time_str));
	memset(time_buf, 0x00, sizeof(time_buf));
	memset(ampm_buf, 0x00, sizeof(ampm_buf));
	memset(buf, 0x00, sizeof(buf));

	timer =
	    ecore_timer_add(60 - ts->tm_sec, (void *)indicator_clock_changed_cb,
			    data);

	indicator_get_time_by_region(icu_apm,data);

	if (clock_mode == INDICATOR_CLOCK_MODE_12H) {
		char bf1[32] = { 0, };
		int hour;

		if(apm_length>0 && apm_length<=4)
		{
			snprintf(ampm_buf, sizeof(ampm_buf),LABEL_STRING, AMPM_FONT_SIZE,icu_apm);
		}
		else
		{
			if (ts->tm_hour >= 0 && ts->tm_hour < 12)
				snprintf(ampm_buf, sizeof(ampm_buf),
					 LABEL_STRING, AMPM_FONT_SIZE,
					 "AM");
			else
				snprintf(ampm_buf, sizeof(ampm_buf),
					 LABEL_STRING, AMPM_FONT_SIZE,
					 "PM");
		}

	        strftime(bf1, sizeof(bf1), "%l", ts);
	        hour = atoi(bf1);
        	strftime(bf1, sizeof(bf1), ":%M", ts);

	        snprintf(time_str, sizeof(time_str), "%d%s", hour, bf1);
		font_size = TIME_FONT_SIZE_12;
		indicator_signal_emit(data,"indicator.clock.ampm","indicator.prog");
	}
	else{
		font_size = TIME_FONT_SIZE_24;
		strftime(time_str, sizeof(time_str), "%H:%M", ts);
		indicator_signal_emit(data,"indicator.clock.default","indicator.prog");
	}

	snprintf(time_buf, sizeof(time_buf), LABEL_STRING, font_size, time_str);

	if(apm_position == 0)
		len = snprintf(buf, sizeof(buf), "%s%s", ampm_buf, time_buf);
	else
		len = snprintf(buf, sizeof(buf), "%s%s", time_buf, ampm_buf);

	if (len < 0) {
		ERR("Unexpected ERROR!");
		return;
	}

	INFO("[CLOCK MODULE] Timer Status : %d Time: %s", timer, buf);

	indicator_part_text_emit(data,"elm.text.clock", buf);

	return;
}

static void indicator_clock_format_changed_cb(keynode_t *node, void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	int r = -1;

	bool is_24hour_enabled = false;

	INFO("[Enter] indicator_clock_format_changed_cb");

	r = runtime_info_get_value_bool(
			RUNTIME_INFO_KEY_24HOUR_CLOCK_FORMAT_ENABLED, &is_24hour_enabled);

	if( r==RUNTIME_INFO_ERROR_NONE&&is_24hour_enabled==true)
	{
		clock_mode = INDICATOR_CLOCK_MODE_24H;
	}
	else
	{
		clock_mode = INDICATOR_CLOCK_MODE_12H;
	}

	char *timezone = vconf_get_str(VCONFKEY_SETAPPL_TIMEZONE_ID);
	ICU_set_timezone(timezone);
	indicator_clock_changed_cb(data);
	free(timezone);
}

static void indicator_clock_pm_state_change_cb(keynode_t *node, void *data)
{
	int status = 0;

	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_PM_STATE, &status);

	switch(status)
	{
		case VCONFKEY_PM_STATE_LCDOFF:
			if (timer != NULL) {
				ecore_timer_del(timer);
				timer = NULL;
			}

			if (battery_timer != NULL) {
				ecore_timer_del(battery_timer);
				battery_timer = NULL;
			}

			if (battery_charging_timer != NULL) {
				ecore_timer_del(battery_charging_timer);
				battery_charging_timer = NULL;
			}
			break;
		default:
			break;
	}

}

static void indicator_clock_battery_disp_changed_cb(keynode_t *node, void *data)
{
	int status = 0;

	vconf_get_int(VCONFKEY_BATTERY_DISP_STATE,&status);

	DBG("indicator_clock_battery_disp_changed_cb(%d)",status);

	if(status==2)
	{
		indicator_clock_display_battery_percentage(data,0);
		indicator_clock_display_battery_percentage(data,1);
	}
	else
	{
		indicator_clock_display_battery_percentage(data,status);
	}
}

static void indicator_clock_charging_now_cb(keynode_t *node, void *data)
{
	int status = 0;
	int lock_state = 0;

	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock_state);

	vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &status);

	battery_charging = status;

	DBG("indicator_clock_charging_now_cb(%d)",status);

	if(lock_state==VCONFKEY_IDLE_LOCK)
	{
		DBG("indicator_clock_charging_now_cb:lock_state(%d)",lock_state);
		return;
	}

	if(battery_charging_first == 1&&status==1)
	{
		DBG("indicator_clock_charging_now_cb : ignore(%d)",status);
	}

	if(status==1)
	{
		battery_charging_first = 1;
		indicator_clock_display_battery_percentage(data,0);
	}
}

static void indicator_clock_battery_capacity_cb(keynode_t *node, void *data)
{
	retif(data == NULL, , "Invalid parameter!");

	if(battery_charging==1&&battery_charging_timer!=NULL)
	{
		DBG("indicator_clock_battery_capacity_cb:battery_charging(%d)",battery_charging);
		indicator_clock_display_battery_percentage(data,0);
	}
}


static void indicator_clock_usb_cb(keynode_t *node, void *data)
{
	int status = 0;

	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS, &status);

	DBG("indicator_clock_usb_cb(%d)",status);

	if(status==VCONFKEY_SYSMAN_USB_DISCONNECTED)
	{
		battery_charging_first = 0;
		if (battery_charging_timer != NULL)
		{
			ecore_timer_del(battery_charging_timer);
			battery_charging_timer = NULL;
		}
		indicator_clock_changed_cb(data);
	}
}

static void indicator_clock_battery_display_cb(void *data)
{
	INFO("indicator_clock_battery_charging_stop_cb");

	if (battery_timer != NULL) {
		ecore_timer_del(battery_timer);
		battery_timer = NULL;
	}

	indicator_clock_changed_cb(data);
}

static void indicator_clock_battery_charging_stop_cb(void *data)
{

	INFO("indicator_clock_battery_charging_stop_cb");

	if (battery_charging_timer != NULL) {
		ecore_timer_del(battery_charging_timer);
		battery_charging_timer = NULL;
	}

	indicator_clock_changed_cb(data);
}

static void indicator_clock_lock_state_cb(keynode_t *node, void *data)
{
	int status = 0;

	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &status);

	DBG("indicator_clock_lock_state_cb(%d)",status);

	if(status==VCONFKEY_IDLE_UNLOCK && battery_charging==1)
	{
		battery_charging_first = 1;
		indicator_clock_display_battery_percentage(data,0);
	}

}
static void indicator_clock_battery_precentage_setting_cb(keynode_t *node, void *data)
{
	int ret = 0;
	int status = 0;

	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_bool(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, &status);
	if (ret != OK)
	{
		ERR("Fail to get [%s: %d]",VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, ret);
		return;
	}
	if(status==0)
	{
		if (battery_charging_timer != NULL) {
			ecore_timer_del(battery_charging_timer);
			battery_charging_timer = NULL;
		}
		if (battery_timer != NULL) {
			ecore_timer_del(battery_timer);
			battery_timer = NULL;
		}
		indicator_clock_changed_cb(data);
	}
}

static void indicator_clock_display_battery_percentage(void *data,int win_type )
{
	int ret = FAIL;
	int status = 0;
	int battery_capa = 0;
	char buf[256] = {0,};
	char temp[256] = {0,};
	struct appdata *ad = (struct appdata *)data;


	if(battery_charging_timer!=NULL)
	{
		INFO("30sec timer alive");
		return;
	}

	ret = vconf_get_bool(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, &status);
	if (ret != OK)
		ERR("Fail to get [%s: %d]",
			VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, ret);

	if(status)
	{
		ret = vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, &battery_capa);
		if (ret != OK)
		{
			ERR("Fail to get [VCONFKEY_SYSMAN_BATTERY_CAPACITY:%d]", ret);
			return;
		}
		if (battery_capa < 0)
		{
			INFO("Invalid Battery Capacity: %d", battery_capa);
			return;
		}

		INFO("Battery Capacity: %d", battery_capa);

		if (battery_capa > 100)
			battery_capa = 100;

		snprintf(temp, sizeof(temp), "%d%%",battery_capa);

		snprintf(buf, sizeof(buf), LABEL_STRING, TIME_FONT_SIZE_BATTERY, temp);

		INFO("indicator_clock_display_battery_percentage %s", buf);

		indicator_part_text_emit_by_win(&(ad->win[win_type]),"elm.text.clock", buf);

		if(battery_charging == 1)
		{

			battery_charging_timer =  ecore_timer_add(BATTERY_TIMER_INTERVAL_CHARGING, (void *)indicator_clock_battery_charging_stop_cb,data);
		}
		else
		{
			if (battery_timer != NULL) {
				ecore_timer_del(battery_timer);
				battery_timer = NULL;
			}

			battery_timer =  ecore_timer_add(BATTERY_TIMER_INTERVAL, (void *)indicator_clock_battery_display_cb,data);
		}
	}

}


static int language_changed_cb(void *data)
{
	DBG("language_changed_cb");
	indicator_clock_changed_cb(data);
	return OK;
}

static int region_changed_cb(void *data)
{
	DBG("region_changed_cb");
	indicator_clock_format_changed_cb(NULL, data);
	return OK;
}

static int wake_up_cb(void *data)
{
	int status = 0;

	INFO("CLOCK wake_up_cb");

	retif(data == NULL, FAIL, "Invalid parameter!");

	vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &status);

	DBG("wake_up_cb(%d)",status);

	if(status==VCONFKEY_IDLE_UNLOCK && battery_charging==1)
	{
		indicator_clock_display_battery_percentage(data,0);
	}
	else
	{
		indicator_clock_changed_cb(data);
	}
	return OK;
}

static int register_clock_module(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_SYSTEM_TIME_CHANGED,
				       indicator_clock_format_changed_cb, data);
	if (ret != OK) {
		ERR("Fail: register VCONFKEY_SYSTEM_TIME_CHANGED");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_REGIONFORMAT_TIME1224,
				       indicator_clock_format_changed_cb, data);
	if (ret != OK) {
		ERR("Fail: register VCONFKEY_REGIONFORMAT_TIME1224");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_TIMEZONE_INT,
				       indicator_clock_format_changed_cb, data);
	if (ret != OK) {
		ERR("Fail: register VCONFKEY_SETAPPL_TIMEZONE_INT");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE, indicator_clock_pm_state_change_cb, (void *)data);

	if (ret != OK) {
		ERR("Fail: register VCONFKEY_PM_STATE");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_BATTERY_DISP_STATE,
				       indicator_clock_battery_disp_changed_cb, data);
	if (ret != OK) {
		ERR("Fail: register VCONFKEY_SETAPPL_TIMEZONE_INT");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CAPACITY,
				       indicator_clock_battery_capacity_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW,
				       indicator_clock_charging_now_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_STATUS,
				       indicator_clock_usb_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}


	ret = vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE,
				       indicator_clock_lock_state_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL,
					indicator_clock_battery_precentage_setting_cb, data);
	if (ret != OK) {
		ERR("Failed to register callback!");
		r = r | ret;
	}

	indicator_clock_format_changed_cb(NULL, data);

	return r;
}

static int unregister_clock_module(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_SYSTEM_TIME_CHANGED,
					       indicator_clock_format_changed_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_SYSTEM_TIME_CHANGED");

	ret = vconf_ignore_key_changed(VCONFKEY_REGIONFORMAT_TIME1224,
				       indicator_clock_format_changed_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_REGIONFORMAT_TIME1224");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_TIMEZONE_INT,
				       indicator_clock_format_changed_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_SETAPPL_TIMEZONE_INT");

	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE,
					       indicator_clock_battery_disp_changed_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_PM_STATE");

	ret = vconf_ignore_key_changed(VCONFKEY_BATTERY_DISP_STATE,
					       indicator_clock_pm_state_change_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_BATTERY_DISP_STATE");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CAPACITY,
					       indicator_clock_battery_capacity_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW,
					       indicator_clock_charging_now_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW");

	ret = vconf_ignore_key_changed(VCONFKEY_SYSMAN_USB_STATUS,
					       indicator_clock_usb_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW");


	ret = vconf_ignore_key_changed(VCONFKEY_IDLE_LOCK_STATE,
					       indicator_clock_lock_state_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW");

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL,
					       indicator_clock_battery_precentage_setting_cb);
	if (ret != OK)
		ERR("Fail: unregister VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW");

	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
	}

	if (battery_timer != NULL) {
		ecore_timer_del(battery_timer);
		battery_timer = NULL;
	}

	if (battery_charging_timer != NULL) {
		ecore_timer_del(battery_charging_timer);
		battery_charging_timer = NULL;
	}

	return OK;
}

void indicator_get_time_by_region(char* output,void *data)
{
	retif(data == NULL, , "Data parameter is NULL");
	retif(output == NULL, , "output parameter is NULL");


	UChar customSkeleton[CLOCK_STR_LEN] = { 0, };
	UErrorCode status = U_ZERO_ERROR;
	UDateFormat *formatter = NULL;

	UChar bestPattern[CLOCK_STR_LEN] = { 0, };
	UChar formatted[CLOCK_STR_LEN] = { 0, };

	char bestPatternString[CLOCK_STR_LEN] = { 0, };
	char formattedString[CLOCK_STR_LEN] = { 0, };

	UDateTimePatternGenerator *pattern_generator = NULL;

	char *time_skeleton = "hhmm";

	char *locale = vconf_get_str(VCONFKEY_REGIONFORMAT);
	if (locale == NULL) {
		DBG("[Error] get value of VCONFKEY_REGIONFORMAT fail.");
	}

	u_uastrncpy(customSkeleton, time_skeleton, strlen(time_skeleton));

	pattern_generator = udatpg_open(locale, &status);

	int32_t bestPatternCapacity = (int32_t) (sizeof(bestPattern) / sizeof((bestPattern)[0]));
	(void)udatpg_getBestPattern(pattern_generator, customSkeleton,
				    u_strlen(customSkeleton), bestPattern,
				    bestPatternCapacity, &status);

	u_austrcpy(bestPatternString, bestPattern);
	u_uastrcpy(bestPattern,"a");

	if(bestPatternString[0] == 'a')
	{
		apm_position = 0;
	}
	else
	{
		apm_position = 1;
	}

	UDate date = ucal_getNow();
	formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, bestPattern, -1, &status);
	int32_t formattedCapacity = (int32_t) (sizeof(formatted) / sizeof((formatted)[0]));
	(void)udat_format(formatter, date, formatted, formattedCapacity, NULL, &status);
	u_austrcpy(formattedString, formatted);

	DBG("DATE & TIME is %s %s %d %s", locale, formattedString, u_strlen(formatted), bestPatternString);

	apm_length = u_strlen(formatted);

	udatpg_close(pattern_generator);

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

	DBG("ICU_set_timezone = %s ", timezone);
	UErrorCode ec = U_ZERO_ERROR;
	UChar *str = uastrcpy(timezone);

	ucal_setDefaultTimeZone(str, &ec);
	if (U_SUCCESS(ec)) {
		DBG("ucal_setDefaultTimeZone() SUCCESS ");
	} else {
		DBG("ucal_setDefaultTimeZone() FAILED : %s ",
			      u_errorName(ec));
	}
	free(str);
}


