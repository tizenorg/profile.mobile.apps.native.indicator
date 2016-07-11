/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <Elementary.h>
#include <vconf.h>
#include <notification_internal.h>
#include <notification_setting.h>
#include <notification_setting_internal.h>
#include <notification.h>
#include <app_manager.h>
#include <efl_util.h>

#include "common.h"
#include "main.h"
#include "util.h"
#include "log.h"
#include "indicator.h"
#include "indicator_gui.h"
#include "ticker.h"

#define TICKERNOTI_DURATION	3.0

#define DEFAULT_ICON ICONDIR		"/quickpanel_icon_default.png"

#define PATH_DOWNLOAD "reserved://quickpanel/ani/downloading"
#define PATH_UPLOAD "reserved://quickpanel/ani/uploading"
#define PATH_INSTALL "reserved://quickpanel/ani/install"

#define TICKER_EDJ EDJDIR"ticker.edj"
#define DEFAULT_EDJ EDJDIR"ticker_default.edj"

static ticker_info_s *_ticker_noti_create(notification_h noti);
static void _ticker_noti_destroy(ticker_info_s **ticker_info);
static Evas_Object *_ticker_icon_create(ticker_info_s *ticker_info);
static Evas_Object *_ticker_textblock_create(ticker_info_s *ticker_info);
static void _ticker_view_destroy(void);
static int _ticker_view_create(void);
static Evas_Object *_ticker_window_create(struct appdata *ad);
static void _ticker_window_destroy(void);

/* Using this macro to emphasize that some portion like stacking and
rotation handling are implemented for X based platform */
#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#endif

static struct {
	Evas_Object *win;
	Evas_Object *layout;
	Evas_Object *scroller;

	ticker_info_s *current;
	ticker_info_s *pending;

	Ecore_Timer *timer;

	struct appdata *ad;
} ticker;

static int _is_text_correct(const char *text)
{
	if (text != NULL) {
		if (strlen(text) > 0) {
			if (strcmp(text, "") != 0) {
				return 1;
			}
		}
	}
	return 0;
}

static int _is_security_lockscreen_launched(void)
{
	int lock_state = 0;
	int lock_type = SETTING_SCREEN_LOCK_TYPE_NONE;

	int ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock_state);
	retvm_if(ret, 0, "%s", get_error_message(ret));
	ret = vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &lock_type);
	retvm_if(ret, 0, "%s", get_error_message(ret));

	if (lock_state == VCONFKEY_IDLE_LOCK &&
		(lock_type != SETTING_SCREEN_LOCK_TYPE_NONE || lock_type != SETTING_SCREEN_LOCK_TYPE_SWIPE))
			return 1;

	return 0;
}

static bool _is_do_not_disturb(notification_h noti)
{
	if (!noti)
		return false;

	notification_system_setting_h system_setting;
	bool do_not_disturb = true;

	notification_setting_h noti_setting;
	char *pkgname = NULL;
	bool do_not_disturb_exception = false;

	int ret = notification_system_setting_load_system_setting(&system_setting);
	retvm_if(ret != NOTIFICATION_ERROR_NONE || system_setting == NULL, true,
			"notification_system_setting_load_system_setting failed: %s", get_error_message(ret));

	ret = notification_system_setting_get_do_not_disturb(system_setting, &do_not_disturb);
	if (ret != NOTIFICATION_ERROR_NONE)
			_E("notification_system_setting_get_do_not_disturb failed: %s", get_error_message(ret));

	notification_system_setting_free_system_setting(system_setting);


	ret = notification_get_pkgname(noti, &pkgname);
	retvm_if(ret != NOTIFICATION_ERROR_NONE || !pkgname, true,
			"notification_get_pkgname failed: %s", get_error_message(ret));

	ret = notification_setting_get_setting_by_package_name(pkgname, &noti_setting);
	retvm_if(ret != NOTIFICATION_ERROR_NONE || noti_setting == NULL, true,
			"notification_setting_get_setting_by_package_name failed: %s", get_error_message(ret));

	ret = notification_setting_get_do_not_disturb_except(noti_setting, &do_not_disturb_exception);
	if (ret != NOTIFICATION_ERROR_NONE) {
		do_not_disturb_exception = false;
		_E("notification_setting_get_do_not_disturb_except failed: %s", get_error_message(ret));
	}

	notification_setting_free_notification(noti_setting);

	_D("do_not_disturb = %d, do_not_disturb_exception = %d", do_not_disturb, do_not_disturb_exception);

	return !do_not_disturb || do_not_disturb_exception ? false : true;

}

static bool _notification_accept(notification_h noti)
{
	if (!noti)
		return false;

	int applist = 0;

	int ret = notification_get_display_applist(noti, &applist);
	retvm_if(ret != NOTIFICATION_ERROR_NONE, false, "notification_get_display_applist failed: %s", get_error_message(ret));

	return applist & NOTIFICATION_DISPLAY_APP_TICKER ? true : false;
}

static void _request_to_delete_noti(notification_h noti)
{
	ret_if(!noti);

	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	int ret = notification_get_display_applist(noti, &applist);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_get_display_applist failed: %s", get_error_message(ret));

	if (applist == NOTIFICATION_DISPLAY_APP_TICKER) {
		ret = notification_delete(noti);
		if (ret != NOTIFICATION_ERROR_NONE)
			_E("Could not delete notification");
	} else {
		ret = notification_set_display_applist(noti, applist & ~NOTIFICATION_DISPLAY_APP_TICKER);
		retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_set_display_applist failed: %s", ret, get_error_message(ret));

		ret = notification_update(noti);
		retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_update failed: %s", get_error_message(ret));
	}

}

static indicator_animated_icon_type _animated_icon_type_get(const char *path)
{
	retv_if(path == NULL, INDICATOR_ANIMATED_ICON_NONE);

	if (strncasecmp(path, PATH_DOWNLOAD, MIN(strlen(PATH_DOWNLOAD), strlen(path))) == 0) {
		return INDICATOR_ANIMATED_ICON_DOWNLOAD;
	} else if (strncasecmp(path, PATH_UPLOAD, MIN(strlen(PATH_UPLOAD), strlen(path))) == 0) {
		return INDICATOR_ANIMATED_ICON_UPLOAD;
	} else if (strncasecmp(path, PATH_INSTALL, MIN(strlen(PATH_INSTALL), strlen(path))) == 0) {
		return INDICATOR_ANIMATED_ICON_INSTALL;
	}

	return INDICATOR_ANIMATED_ICON_NONE;
}

static Evas_Object *_animated_icon_get(Evas_Object *parent, const char *path)
{
	indicator_animated_icon_type type = INDICATOR_ANIMATED_ICON_NONE;
	const char *layout_icon = NULL;
	Evas_Object *layout = NULL;

	retv_if(!parent, NULL);
	retv_if(!path, NULL);

	type = _animated_icon_type_get(path);

	if (type == INDICATOR_ANIMATED_ICON_DOWNLOAD) {
		layout_icon = "quickpanel/animated_icon_download";
	} else if (type == INDICATOR_ANIMATED_ICON_UPLOAD) {
		layout_icon = "quickpanel/animated_icon_upload";
	} else if (type == INDICATOR_ANIMATED_ICON_INSTALL) {
		layout_icon = "quickpanel/animated_icon_install";
	} else {
		return NULL;
	}

	layout = elm_layout_add(parent);
	if (layout) {
		elm_layout_file_set(layout, DEFAULT_EDJ, layout_icon);
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(layout);
	}
	return layout;
}

static char *_pkginfo_get_icon(const char *pkgid)
{
	char *icon_path = NULL;
	app_info_h app_info;

	retvm_if(pkgid == NULL, NULL, invalid parameter);

	int ret = app_info_create(pkgid, &app_info);
	retvm_if(ret != APP_MANAGER_ERROR_NONE, NULL, "app_info_create failed: %s", get_error_message(ret));

	ret = app_info_get_icon(app_info, &icon_path);
	if (ret != APP_MANAGER_ERROR_NONE) {
		app_info_destroy(app_info);
		_E("app_info_get_icon failed: %s", get_error_message(ret));
		return NULL;
	}

	app_info_destroy(app_info);

	return icon_path;
}

static char *_noti_get_text(notification_h noti, notification_text_type_e text_type)
{
	char *text = NULL;

	int ret = notification_get_text(noti, text_type, &text);
	retv_if(ret != NOTIFICATION_ERROR_NONE || !text, NULL);

	return elm_entry_utf8_to_markup(text);
}

static void _strbuf_add(Eina_Strbuf *str_buf, char *text, const char *delimiter)
{
	if (text != NULL) {
		if (strlen(text) > 0) {
			if (delimiter != NULL) {
				eina_strbuf_append(str_buf, delimiter);
			}
			eina_strbuf_append(str_buf, text);
		}
	}
}

static int _ticker_icon_update(ticker_info_s *ticker_info)
{
	Evas_Object *current_icon = elm_object_part_content_unset(ticker.layout, "icon");
	if (current_icon)
		evas_object_del(current_icon);

	Evas_Object *icon = _ticker_icon_create(ticker_info);
	if (!icon)
		return -1;

	ticker_info->icon = icon;
	elm_object_part_content_set(ticker.layout, "icon", icon);

	return 0;
}

static int _ticker_textblock_update(ticker_info_s *ticker_info)
{
	retvm_if(!ticker.win || !ticker.layout, -1,
			"Tickers window or layout doesn't exist");

	Evas_Object *current_textblock = elm_object_content_unset(ticker.scroller);
	if (current_textblock)
		evas_object_del(current_textblock);

	Evas_Object *textblock = _ticker_textblock_create(ticker_info);
	retvm_if(!textblock, -1, "_ticker_textblock_create failed");

	ticker_info->textblock = textblock;

	evas_object_show(ticker_info->textblock);
	elm_object_content_set(ticker.scroller, textblock);

	return 0;
}

static int _ticker_textblock_show_next_line(void)
{
	Evas_Object *textblock_layout = NULL;
	Evas_Object *textblock = NULL;
	Evas_Object *scroller = NULL;
	ticker_info_s *ticker_info = ticker.current;
	int ch = 0;

	textblock_layout = elm_layout_edje_get(ticker_info->textblock);
	retvm_if(!textblock_layout, -1, "textblock_layout == NULL");

	textblock = (Evas_Object *)edje_object_part_object_get(textblock_layout, "elm.text");
	retvm_if(!textblock, -1, "textblock == NULL");

	_D("Show textblock line: %d", ticker_info->current_line);

	if (!evas_object_textblock_line_number_geometry_get(textblock, ticker_info->current_line,
				NULL, NULL, NULL, &ch)) {
		_E("Textblock line number exceeded");
		return -1;
	}

	scroller = elm_object_part_content_get(ticker.layout, "text_rect");
	retvm_if(!scroller, -1, "scroller == NULL");

	elm_scroller_page_size_set(scroller, 0, ch);
	elm_scroller_page_bring_in(scroller, 0, ticker_info->current_line++);

	return 0;
}

static int _ticker_view_update(ticker_info_s *ticker_info)
{
	retvm_if(_ticker_textblock_update(ticker_info), -1,
			"_ticker_textblock_update failed");
	retvm_if(_ticker_icon_update(ticker_info), -1,
			"_ticker_icon_update failed");

	return 0;
}

static Eina_Bool _ticker_update(void *data)
{
	_D("Ticker update timeout");

	if (ticker.pending) {
		_ticker_noti_destroy(&ticker.current);
		ticker.current = ticker.pending;
		ticker.pending = NULL;
	}

	if (!ticker.current->textblock)
		goto_if(_ticker_view_update(ticker.current), EXIT);

	if (_ticker_textblock_show_next_line())
		goto EXIT;

	return ECORE_CALLBACK_RENEW;

EXIT:
	_E("Ticker view update failed");

	_ticker_noti_destroy(&ticker.current);
	_ticker_view_destroy();

	return ECORE_CALLBACK_CANCEL;
}

static char *_ticker_get_layout_text(notification_h noti, notification_ly_type_e option)
{
	char *title_utf8 = NULL;
	char *content_utf8 = NULL;
	char *info1_utf8 = NULL;
	char *info1_sub_utf8 = NULL;
	char *info2_utf8 = NULL;
	char *info2_sub_utf8 = NULL;
	Eina_Strbuf *buffer = NULL;
	char *result = NULL;

	retv_if(!noti, NULL);

	buffer = eina_strbuf_new();
	retvm_if(!buffer, NULL, "eina_strbuf_new failed");

	title_utf8 = _noti_get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE);
	content_utf8 = _noti_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT);
	info1_utf8 = _noti_get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_1);
	info1_sub_utf8 = _noti_get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1);
	info2_utf8 = _noti_get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_2);
	info2_sub_utf8 = _noti_get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2);

	//FIXME find difference between NOTIFICATION_LY_NOTI_EVENT_SINGLE display and other
	if (_is_text_correct(title_utf8) && (_is_text_correct(content_utf8))) {
		_strbuf_add(buffer, title_utf8, NULL);
		_strbuf_add(buffer, content_utf8, "<br/>");
	} else if (_is_text_correct(info1_utf8) && (_is_text_correct(content_utf8))) {
		_strbuf_add(buffer, content_utf8, NULL);
		_strbuf_add(buffer, info1_utf8, "<br/>");
		_strbuf_add(buffer, info1_sub_utf8, " ");
	} else if (_is_text_correct(info1_utf8) && _is_text_correct(info2_utf8)) {
		_strbuf_add(buffer, info1_utf8, NULL);
		_strbuf_add(buffer, info1_sub_utf8, " ");
		_strbuf_add(buffer, info2_utf8, "<br/>");
		_strbuf_add(buffer, info2_sub_utf8, " ");
	} else if (_is_text_correct(title_utf8)) {
		_strbuf_add(buffer, title_utf8, NULL);
	} else if (_is_text_correct(content_utf8)) {
		_strbuf_add(buffer, content_utf8, NULL);
	}

	result = strdup(eina_strbuf_string_get(buffer));

	eina_strbuf_free(buffer);

	if (title_utf8) {
		free(title_utf8);
	}
	if (content_utf8) {
		free(content_utf8);
	}
	if (info1_utf8) {
		free(info1_utf8);
	}
	if (info1_sub_utf8) {
		free(info1_sub_utf8);
	}
	if (info2_utf8) {
		free(info2_utf8);
	}
	if (info2_sub_utf8) {
		free(info2_sub_utf8);
	}

	return result;
}

static char *_ticker_get_text(notification_h noti)
{
	notification_ly_type_e noti_layout;

	retv_if(!noti, NULL);
	int ret = notification_get_layout(noti, &noti_layout);
	retvm_if(ret != NOTIFICATION_ERROR_NONE, NULL,
			"notification_get_layout failed: %s", get_error_message(ret));

	//TODO: What is the purpose of the notification layout - for now guide says nothing
	//	about that
	return _ticker_get_layout_text(noti, noti_layout);
}

static void _ticker_layout_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_ticker_view_destroy();
}

static Evas_Object *_ticker_icon_create(ticker_info_s *ticker_info)
{
	char *icon_path = NULL;
	char *icon_default = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *layout = ticker.layout;
	notification_h noti = ticker_info->noti;

	retv_if(!layout, NULL);
	retv_if(!noti, NULL);

	int ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path);
	if (ret != NOTIFICATION_ERROR_NONE || !icon_path) {
		_E("notification_get_image failed: %s", get_error_message(ret));
		return NULL;
	}

	icon = _animated_icon_get(layout, icon_path);
	if (icon == NULL) {
		icon = elm_image_add(layout);
		if (icon_path == NULL
				|| (elm_image_file_set(icon, icon_path, NULL) == EINA_FALSE)) {
			icon_default = _pkginfo_get_icon(ticker.current->pkgname);
			if (icon_default != NULL) {
				elm_image_file_set(icon, icon_default, NULL);
				elm_image_resizable_set(icon, EINA_TRUE, EINA_TRUE);
				free(icon_default);
			} else {
				elm_image_file_set(icon, DEFAULT_ICON, NULL);
				elm_image_resizable_set(icon, EINA_TRUE, EINA_TRUE);
			}
		}
	}

	evas_object_show(icon);

	return icon;
}

static Evas_Object *_ticker_textblock_create(ticker_info_s *ticker_info)
{
	Evas_Object *textblock = elm_layout_add(ticker.win);
	char *ticker_text = NULL;

	if (!elm_layout_file_set(textblock, util_get_res_file_path(TICKER_EDJ),
				"quickpanel/tickernoti/text")) {
		_E("elm_layout_file_set failed");
		evas_object_del(textblock);
		return NULL;
	}

	evas_object_size_hint_align_set(textblock, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(textblock, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (!ticker_info->noti) {
		_E("!ticker_info->noti");
		evas_object_del(textblock);
		return NULL;
	}

	ticker_text = _ticker_get_text(ticker_info->noti);
	if (!ticker_text) {
		_E("!ticker_text");
		evas_object_del(textblock);
		return NULL;
	}

	elm_object_part_text_set(textblock, "elm.text", ticker_text);

	free(ticker_text);

	return textblock;
}

static Evas_Object *_ticker_scroller_create(Evas_Object *layout)
{
	Evas_Object *scroller = elm_scroller_add(layout);

	evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_scroller_gravity_set(scroller, 0, 0);

	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_scroller_movement_block_set(scroller,
			ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL | ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
	elm_scroller_page_scroll_limit_set(scroller, 1, 1);

	elm_object_part_content_set(layout, "text_rect", scroller);

	return scroller;
}

static Evas_Object *_ticker_layout_create(Evas_Object *win)
{
	if (!win)
		return NULL;

	Evas_Object *layout = elm_layout_add(win);

	if (!elm_layout_file_set(layout, util_get_res_file_path(TICKER_EDJ),
				"quickpanel/tickernoti/normal"))
		_E("elm_layout_file_set failed");

	evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_DOWN, _ticker_layout_mouse_down_cb, NULL);

	elm_win_resize_object_add(win, layout);

	return layout;
}

static void _ticker_win_rotation_cb(void *data, Evas_Object *obj, void *event)
{
	int angle = 0;
	Evas_Object *win = obj;

	angle = elm_win_active_win_orientation_get(win);
	angle %= 360;

	if (angle < 0)
		angle += 360;

	_D("port.w = %d", ticker.ad->win.port_w);
	_D("land.w = %d", ticker.ad->win.land_w);
	_D("angle = %d", angle);

	switch (angle) {
	case 0:
	case 180:
		evas_object_resize(win, ticker.ad->win.port_w, INDICATOR_HEIGHT);
		evas_object_move(win, 0, 0);
		break;
	case 90:
		evas_object_resize(win, ticker.ad->win.land_w, INDICATOR_HEIGHT);
		evas_object_move(win, 0, 0);
		break;
	case 270:
		evas_object_resize(win, ticker.ad->win.land_w, INDICATOR_HEIGHT);
		evas_object_move(win, ticker.ad->win.port_w - INDICATOR_HEIGHT, 0);
		break;
	default:
		_E("Undefined window rotation angle");
	}
}

static Evas_Object *_ticker_window_create(struct appdata *ad)
{
	Evas_Object *win = NULL;

	_D("A window is created for ticker notifications");

	win = elm_win_add(NULL, "ticker_win", ELM_WIN_NOTIFICATION);
	retv_if(!win, NULL);

	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_title_set(win, "ticker_win");
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);
	efl_util_set_notification_window_level(win, EFL_UTIL_NOTIFICATION_LEVEL_HIGH);
	elm_win_prop_focus_skip_set(win, EINA_TRUE);
	elm_win_role_set(win, "notification-normal");

	elm_win_aux_hint_add(win, "wm.policy.win.user.geometry", "1");
	evas_object_resize(win, ad->win.w, INDICATOR_HEIGHT);
	evas_object_move(win, 0, 0);

	if (elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, rots, 4);
	}

	evas_object_smart_callback_add(win, "wm,rotation,changed", _ticker_win_rotation_cb, ad);

	_ticker_win_rotation_cb(NULL, ticker.win, NULL);

	return win;
}

static void _ticker_window_destroy(void)
{
	evas_object_smart_callback_del(ticker.win, "wm,rotation,changed", _ticker_win_rotation_cb);
	evas_object_del(ticker.win);
}

static int _ticker_view_create(void)
{
	if (!ticker.win)
		ticker.win = _ticker_window_create(ticker.ad);
	if (!ticker.layout)
		ticker.layout = _ticker_layout_create(ticker.win);
	if (!ticker.scroller)
		ticker.scroller = _ticker_scroller_create(ticker.layout);

	evas_object_show(ticker.layout);
	evas_object_show(ticker.scroller);
	evas_object_show(ticker.win);

	if (ticker.ad)
		util_signal_emit_by_win(&ticker.ad->win, "message.show.noeffect", "indicator.prog");

	if (ticker.win && ticker.layout && ticker.scroller)
		return 0;
	else {
		_ticker_view_destroy();
		return -1;
	}
}

static void _ticker_view_destroy(void)
{
	if (ticker.win) {
		_ticker_window_destroy();

		ticker.win = NULL;
		ticker.layout = NULL;
		ticker.scroller = NULL;
	}

	_ticker_noti_destroy(&ticker.current);
	ticker.current = NULL;
	_ticker_noti_destroy(&ticker.pending);
	ticker.pending = NULL;

	if (ticker.timer) {
		ecore_timer_del(ticker.timer);
		ticker.timer = NULL;
	}

	if (ticker.ad)
		util_signal_emit_by_win(&ticker.ad->win, "message.hide", "indicator.prog");
}

static ticker_info_s *_ticker_noti_create(notification_h noti)
{
	retv_if(!noti, NULL);
	char *pkgname = NULL;

	ticker_info_s *ticker_info = calloc(1, sizeof(ticker_info_s));
	retvm_if(!ticker_info, NULL,
			"ticker_info calloc failed");

	int ret = notification_clone(noti, &ticker_info->noti);
	if (ret != NOTIFICATION_ERROR_NONE || !ticker_info->noti) {
		_E("notification_clone failed: %s", get_error_message(ret));
		free(ticker_info);
		return NULL;
	}

	ret = notification_get_pkgname(ticker_info->noti, &pkgname);
	if (ret != NOTIFICATION_ERROR_NONE || !pkgname) {
		_E("notification_get_pkgname failed: %s", get_error_message(ret));
		free(ticker_info);
		return NULL;
	}

	ticker_info->pkgname = strdup(pkgname);
	ticker_info->current_line = 0;
	ticker_info->textblock = NULL;


	return ticker_info;
}

static void _ticker_noti_destroy(ticker_info_s **ticker_info)
{
	ret_if(!*ticker_info);

	if ((*ticker_info)->noti) {
		_request_to_delete_noti((*ticker_info)->noti);
		notification_free((*ticker_info)->noti);
		(*ticker_info)->noti = NULL;
	}
	if ((*ticker_info)->textblock)
		evas_object_del((*ticker_info)->textblock);
	if ((*ticker_info)->icon)
		evas_object_del((*ticker_info)->icon);

	if (!ticker.pending && !ticker.current)
		_ticker_view_destroy();

	free((*ticker_info)->pkgname);
	free(*ticker_info);

	*ticker_info = NULL;
}

static void _ticker_noti_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	ret_if(!op_list);
	ret_if(!op_list->noti);

	if (!_notification_accept(op_list->noti)) {
		_E("Notification rejected");
		return;
	}

	if (op_list->type != NOTIFICATION_OP_INSERT
			&& op_list->type != NOTIFICATION_OP_UPDATE) {
		_E("Wrong operation type");
		return;
	}

	if (_is_security_lockscreen_launched()) {
		_E("Lockscreen launched, ticker dismiss");
		_request_to_delete_noti(op_list->noti);
		return;
	}

	if (_is_do_not_disturb(op_list->noti)) {
		_E("Do not disturb");
		_request_to_delete_noti(op_list->noti);
		return;
	}

	ticker_info_s *ticker_info = _ticker_noti_create(op_list->noti);
	retm_if(!ticker_info, "_ticker_noti_create failed");

	retm_if(_ticker_view_create(), "_ticker_view_create failed");

	if (!ticker.current)
		ticker.current = ticker_info;
	else {
		if (ticker.pending)
			_ticker_noti_destroy(&ticker.pending);
		ticker.pending = ticker_info;
	}

	if (!ticker.timer) {
		ticker.timer = ecore_timer_add(TICKERNOTI_DURATION, _ticker_update, NULL);
		_ticker_update(NULL);
	}

	return;
}

int ticker_init(struct appdata *ad)
{
	ticker.ad = ad;

	int ret = notification_register_detailed_changed_cb(_ticker_noti_detailed_changed_cb, ad);
	if (ret != NOTIFICATION_ERROR_NONE) {
		_E("notification_register_detailed_changed_cb failed: %s", get_error_message(ret));
		return INDICATOR_ERROR_FAIL;
	}

	return INDICATOR_ERROR_NONE;
}

int ticker_fini(struct appdata *ad)
{
	retv_if(!ad, 0);

	_ticker_view_destroy();

	return INDICATOR_ERROR_NONE;
}
/* End of file */
