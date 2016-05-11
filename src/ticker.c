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
//#include <Ecore_X.h>
#include <vconf.h>
#include <app_control.h>
#include <notification_internal.h>
#include <notification_setting.h>
#include <notification_setting_internal.h>
#include <feedback.h>
#include <system_settings.h>
#include <player.h>
#include <sound_manager.h>
#include <metadata_extractor.h>
#include <notification.h>
#include <notification_text_domain.h>
#include <app_manager.h>
#include <runtime_info.h>
#include <efl_util.h>

#include "common.h"
#include "main.h"
#include "util.h"
#include "log.h"
#include "indicator.h"
#include "ticker.h"

#define _SPACE ' '
#define TICKERNOTI_DURATION	3
#define QP_TICKER_DETAIL_DURATION 6
#define QP_PLAY_DURATION_LIMIT 15
#define TICKER_MSG_LEN				1024
#define TICKER_PHONE_NUMBER_MAX_LEN 40

#define DEFAULT_ICON ICONDIR		"/quickpanel_icon_default.png"

#define FORMAT_1LINE "<font_size=29><color=#F4F4F4FF>%s</color></font_size>"
#define FORMAT_2LINE "<font_size=26><color=#BABABAFF>%s</color></font_size><br/><font_size=29><color=#F4F4F4FF>%s</color></font_size>"

#define PRIVATE_DATA_KEY_APPDATA "pdka"
#define PRIVATE_DATA_KEY_TICKERNOTI_EXECUTED "pdkte"
#define PRIVATE_DATA_KEY_ANI_ICON_TYPE "pdkait"
#define PRIVATE_DATA_KEY_ICON "pdki"
#define PRIVATE_DATA_KEY_BOX "pdkb"
#define PRIVATE_DATA_KEY_TICKER_INFO "pdkti"
#define PRIVATE_DATA_KEY_NOTI "pdkn"
#define PRIVATE_DATA_KEY_DATA "pdk_data"

#define PATH_DOWNLOAD "reserved://quickpanel/ani/downloading"
#define PATH_UPLOAD "reserved://quickpanel/ani/uploading"
#define PATH_INSTALL "reserved://quickpanel/ani/install"

static void _create_ticker_noti(notification_h noti, struct appdata *ad, ticker_info_s *ticker_info);
static void _destroy_ticker_noti(ticker_info_s *ticker_info);

/* Using this macro to emphasize that some portion like stacking and
rotation handling are implemented for X based platform */
#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#endif

struct Internal_Data {
	Evas_Object *content;
	Ecore_Event_Handler *rotation_event_handler;
	Evas_Coord w;
	Evas_Coord h;
	int angle;
};

static int _is_text_exist(const char *text)
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
	int ret = 0;
	int is_lock_launched = 0;

	if ((ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &is_lock_launched)) == 0) {
		if (is_lock_launched == VCONFKEY_IDLE_LOCK &&
				(ret = vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &is_lock_launched)) == 0) {
			if (is_lock_launched != SETTING_SCREEN_LOCK_TYPE_NONE && is_lock_launched != SETTING_SCREEN_LOCK_TYPE_SWIPE) {
				return 1;
			}
		}
	}

	return 0;
}

static notification_h _get_instant_latest_message_from_list(ticker_info_s *ticker_info)
{
	int count = 0;
	notification_h noti = NULL;

	count = eina_list_count(ticker_info->ticker_list);
	if (count > 1) {
		noti = eina_list_nth(ticker_info->ticker_list, count-1);
	}
	eina_list_free(ticker_info->ticker_list);
	ticker_info->ticker_list = NULL;

	return noti;
}

static void _request_to_delete_noti(notification_h noti)
{
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	ret_if(!noti);

	if (notification_get_display_applist(noti, &applist) != NOTIFICATION_ERROR_NONE) {
		_E("Failed to get display");
	}

	if (applist & (NOTIFICATION_DISPLAY_APP_TICKER | NOTIFICATION_DISPLAY_APP_INDICATOR)) {
		if (applist & ~(NOTIFICATION_DISPLAY_APP_TICKER | NOTIFICATION_DISPLAY_APP_INDICATOR)) {
			// Do not delete in this case
			_D("There is another subscriber: 0x%X (filtered: 0x%X)", applist, applist & ~(NOTIFICATION_DISPLAY_APP_TICKER | NOTIFICATION_DISPLAY_APP_INDICATOR));
		} else {
			char *pkgname = NULL;
			int priv_id = 0;
			int status;

			if (notification_get_pkgname(noti, &pkgname) != NOTIFICATION_ERROR_NONE) {
				_E("Failed to get pkgname");
				/**
				 * @note
				 * Even though we failed to get pkgname,
				 * the delete_by_priv_id will try to do something with caller's pkgname instead of noti's pkgname.
				 */
			}

			_D("Target pkgname: [%s]", pkgname);
			if (notification_get_id(noti, NULL, &priv_id) != NOTIFICATION_ERROR_NONE) {
				_E("Failed to get priv_id");
			}

			status = notification_delete_by_priv_id(pkgname, NOTIFICATION_TYPE_NONE, priv_id);
			_D("Delete notification: %d (status: %d)", priv_id, status);
		}
	}
}

static void _resize_textblock_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	ticker_info_s *ticker_info = data;
	Evas_Object *tb = obj;
	Evas_Textblock_Cursor *cur;

	ret_if(!ticker_info);
	ret_if(!tb);

	/* RESIZE */
	if ((ticker_info->cursor_pos > 0) && (ticker_info->current_page != 0)) {

		Evas_Textblock_Cursor *cur1, *cur2;
		Evas_Coord fw, fh;
		char *range_text = NULL;

		cur1 = evas_object_textblock_cursor_new(tb);
		cur2 = evas_object_textblock_cursor_new(tb);

		evas_textblock_cursor_pos_set(cur1, ticker_info->cursor_pos);
		evas_object_textblock_size_formatted_get(tb, &fw, &fh);

		if (evas_textblock_cursor_char_coord_set(cur2, fw, fh)) {
			_D("cur2 coord set success");
			range_text = evas_textblock_cursor_range_text_get(cur1, cur2, EVAS_TEXTBLOCK_TEXT_MARKUP);

			if (range_text) {
				_D("There is a range_text: %s", range_text);
				elm_object_part_text_set(ticker_info->textblock, "elm.text", range_text);
				elm_scroller_region_show(ticker_info->scroller, 0, 0, 0, 0);
				ticker_info->current_page = 0;
				ticker_info->cursor_pos = 0;
			}
		}

		evas_textblock_cursor_free(cur1);
		evas_textblock_cursor_free(cur2);
	}

	/* ADJUST */
	cur = evas_object_textblock_cursor_new(tb);

	if (evas_textblock_cursor_line_set(cur, 0)) {

		Evas_Coord cy, ch;
		Evas_Coord vh;

		evas_textblock_cursor_line_geometry_get(cur, NULL, &cy, NULL, &ch);
		evas_object_geometry_get(ticker_info->scroller, NULL, NULL, NULL, &vh);

		if (ch > vh) {
			elm_scroller_region_bring_in(ticker_info->scroller, 0, cy - ((ch - vh) / 2), 0, vh);
		} else {
			elm_scroller_region_bring_in(ticker_info->scroller, 0, cy + ((vh - ch) / 2), 0, vh);
		}
		ticker_info->cursor_pos = evas_textblock_cursor_pos_get(cur);
	} else {
		ticker_info->cursor_pos = -1;
	}

	evas_textblock_cursor_free(cur);
}

static Eina_Bool _timeout_cb(void *data)
{
	ticker_info_s *ticker_info = NULL;

	retv_if(!data, EINA_FALSE);

	_D("message is timeout");

	ticker_info = data;

	/* If count is 1, self */
	if (ticker_info->ticker_list && eina_list_count(ticker_info->ticker_list) > 1) {
		if (ticker_info->timer) {
			ecore_timer_del(ticker_info->timer);
			ticker_info->timer = NULL;
		}
		_destroy_ticker_noti(ticker_info);

		return ECORE_CALLBACK_CANCEL;
	}

	if (ticker_info->cursor_pos != -1) {
		const Evas_Object *tb;
		Evas_Textblock_Cursor *cur;
		Evas_Coord cy, ch;
		Evas_Coord vh;

		tb = edje_object_part_object_get(elm_layout_edje_get(ticker_info->textblock), "elm.text");
		cur = evas_object_textblock_cursor_new(tb);
		ticker_info->current_page++;

		if (evas_textblock_cursor_line_set(cur, ticker_info->current_page)) {
			evas_textblock_cursor_line_geometry_get(cur, NULL, &cy, NULL, &ch);
			evas_object_geometry_get(ticker_info->scroller, NULL, NULL, NULL, &vh);

			if (ch > vh) {
				elm_scroller_region_bring_in(ticker_info->scroller, 0, cy - ((ch - vh) / 2), 0, vh);
			} else {
				elm_scroller_region_bring_in(ticker_info->scroller, 0, cy + ((vh - ch) / 2), 0, vh);
			}
			ticker_info->cursor_pos = evas_textblock_cursor_pos_get(cur);
		} else {
			ticker_info->cursor_pos = -1;
		}

		evas_textblock_cursor_free(cur);

		if (ticker_info->cursor_pos != -1) {
			return ECORE_CALLBACK_RENEW;
		}
	}

	if (ticker_info->timer) {
		ticker_info->timer = NULL;
	}

	_destroy_ticker_noti(ticker_info);

	return ECORE_CALLBACK_CANCEL;
}

static indicator_animated_icon_type _animated_type_get(const char *path)
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

#define DEFAULT_EDJ EDJDIR"/ticker_default.edj"
/* FIXME : evas_object_del(icon), we have to unset PRIVATE_DATA_KEY_ANI_ICON_TYPE) */
static Evas_Object *_animated_icon_get(Evas_Object *parent, const char *path)
{
	indicator_animated_icon_type type = INDICATOR_ANIMATED_ICON_NONE;
	const char *layout_icon = NULL;
	Evas_Object *layout = NULL;

	retv_if(!parent, NULL);
	retv_if(!path, NULL);

	type = _animated_type_get(path);

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
		evas_object_data_set(layout, PRIVATE_DATA_KEY_ANI_ICON_TYPE, (void *)type);
		evas_object_show(layout);
	}
	return layout;
}

static char *_get_pkginfo_icon(const char *pkgid)
{
	int ret;
	char *icon_path = NULL;
	app_info_h app_info;

	retvm_if(pkgid == NULL, NULL, invalid parameter);

	ret = app_info_create(pkgid, &app_info);
	if (ret != APP_MANAGER_ERROR_NONE) {
		_E("app_info_create for %s failed %d", pkgid, ret);
		return NULL;
	}

	ret = app_info_get_icon(app_info, &icon_path);
	if (ret != APP_MANAGER_ERROR_NONE) {
		app_info_destroy(app_info);
		_E("app_info_get_icon failed %d", ret);
		return NULL;
	}
	app_info_destroy(app_info);

	return icon_path;
}

static Evas_Object *_ticker_create_icon(Evas_Object *parent, notification_h noti)
{
	char *pkgname = NULL;
	char *icon_path = NULL;
	char *icon_default = NULL;
	Evas_Object *icon = NULL;

	retv_if(!parent, NULL);
	retv_if(!noti, NULL);

	notification_get_pkgname(noti, &pkgname);
	if (NOTIFICATION_ERROR_NONE != notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path)) {
		_E("Cannot get image path");
		return NULL;
	}
	if (icon_path) {
		icon = _animated_icon_get(parent, icon_path);
		if (icon == NULL) {
			icon = elm_image_add(parent);
			if (icon_path == NULL
					|| (elm_image_file_set(icon, icon_path, NULL) == EINA_FALSE)) {
				icon_default = _get_pkginfo_icon(pkgname);
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
	}

	return icon;
}

static char *_get_text(notification_h noti, notification_text_type_e text_type)
{
	char *text = NULL;

	notification_get_text(noti, text_type, &text);
	if (text) {
		return elm_entry_utf8_to_markup(text);
	}

	return NULL;
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

static int _is_phone_number(const char *address)
{
	int digit_count = 0;
	int addr_len = 0;

	retv_if(!address, 0);

	addr_len = strlen(address);

	if (addr_len == 0) return 0;

	/*  length check phone address should be longer than 2 and shorter than 40 */
	if (addr_len > 2 && addr_len <= TICKER_PHONE_NUMBER_MAX_LEN) {
		const char *pszOneChar = address;

		while (*pszOneChar) {
			if (isdigit(*pszOneChar)) {
				digit_count++;
			}
			++pszOneChar;
		}
		pszOneChar = address;

		if (*pszOneChar == '+') ++pszOneChar;

		while (*pszOneChar) {
			if (!isdigit(*pszOneChar)
					&& (*pszOneChar != '*') && (*pszOneChar != '#')
					&& (*pszOneChar != ' ')
					&& !((*pszOneChar == '-') && digit_count >= 7)) {
				return 0;
			}
			++pszOneChar;
		}

		return 1;
	} else {
		_D("invalid address length [%d]", addr_len);
		return 0;
	}
}

static void _char_set(char *dst, char s, int index, int size)
{
	if (index < size) {
		*(dst + index) = s;
	}
}

static void _make_phone_number_tts(char *dst, const char *src, int size)
{
	int no_op = 0;
	int i = 0, j = 0, text_len = 0;

	ret_if(!dst);
	ret_if(!src);

	text_len = strlen(src);

	for (i = 0, j= 0; i < text_len; i++) {
		if (no_op == 1) {
			_char_set(dst, *(src + i), j++, size);
		} else {
			if (isdigit(*(src + i))) {
				if (i + 1 < text_len) {
					if (*(src + i + 1) == '-' || *(src + i + 1) == _SPACE) {
						_char_set(dst, *(src + i), j++, size);
					} else {
						_char_set(dst, *(src + i), j++, size);
						_char_set(dst, _SPACE, j++, size);
					}
				} else {
					_char_set(dst, *(src + i), j++, size);
					_char_set(dst, _SPACE, j++, size);
				}
			} else if (*(src + i) == '-') {
				no_op = 1;
				_char_set(dst, *(src + i), j++, size);
			} else {
				_char_set(dst, *(src + i), j++, size);
			}
		}
	}
}

static void _check_and_add_to_buffer(Eina_Strbuf *str_buf, char *text, int is_check_phonenumber)
{
	char buf_number[TICKER_PHONE_NUMBER_MAX_LEN * 2] = { 0, };

	if (text != NULL) {
		if (strlen(text) > 0) {
			if (_is_phone_number(text) && is_check_phonenumber) {
				_make_phone_number_tts(buf_number, text,
						(TICKER_PHONE_NUMBER_MAX_LEN * 2) - 1);
				eina_strbuf_append(str_buf, buf_number);
			} else {
				eina_strbuf_append(str_buf, text);
			}
			eina_strbuf_append_char(str_buf, '\n');
		}
	}
}

static char *_ticker_get_label_layout_default(notification_h noti, int is_screenreader, char **str_line1, char **str_line2)
{
	int len = 0;
	int num_line = 0;
	char *domain = NULL;
	char *dir = NULL;
	char *title_utf8 = NULL;
	char *content_utf8 = NULL;
	char *info1_utf8 = NULL;
	char *info1_sub_utf8 = NULL;
	char *info2_utf8 = NULL;
	char *info2_sub_utf8 = NULL;
	char *event_count_utf8 = NULL;
	const char *tmp = NULL;
	Eina_Strbuf *line1 = NULL;
	Eina_Strbuf *line2 = NULL;
	char buf[TICKER_MSG_LEN] = { 0, };

	retv_if(!noti, NULL);

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	title_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE);
	content_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT);
	info1_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_1);
	info1_sub_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1);
	info2_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_2);
	info2_sub_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2);

	if (is_screenreader == 0) {
		line1 = eina_strbuf_new();
		line2 = eina_strbuf_new();

		if (line1 != NULL && line2 != NULL) {
			if (_is_text_exist(title_utf8) && (_is_text_exist(content_utf8)
					|| _is_text_exist(event_count_utf8))) {
				_strbuf_add(line1, title_utf8, NULL);
				_strbuf_add(line2, content_utf8, NULL);
				if (_is_text_exist(content_utf8)) {
					_strbuf_add(line2, event_count_utf8, " ");
				} else {
					_strbuf_add(line2, event_count_utf8, "");
				}
				num_line = 2;
			} else if (_is_text_exist(info1_utf8) && (_is_text_exist(content_utf8)
					|| _is_text_exist(event_count_utf8))) {
				_strbuf_add(line1, content_utf8, NULL);
				_strbuf_add(line1, event_count_utf8, " ");
				_strbuf_add(line2, info1_utf8, NULL);
				_strbuf_add(line2, info1_sub_utf8, " ");
				num_line = 2;
			} else if (_is_text_exist(info1_utf8) && _is_text_exist(info2_utf8)) {
				_strbuf_add(line1, info1_utf8, NULL);
				_strbuf_add(line1, info1_sub_utf8, " ");
				_strbuf_add(line2, info2_utf8, NULL);
				_strbuf_add(line2, info2_sub_utf8, " ");
				num_line = 2;
			} else if (_is_text_exist(title_utf8)) {
				_strbuf_add(line1, title_utf8, NULL);
				num_line = 1;
			} else if (_is_text_exist(content_utf8)) {
				_strbuf_add(line1, content_utf8, NULL);
				num_line = 1;
			}

			if (num_line == 2) {
				tmp = eina_strbuf_string_get(line1);
				if (str_line1 != NULL && tmp != NULL) {
					*str_line1 = strdup(tmp);
				}

				tmp = eina_strbuf_string_get(line2);
				if (str_line2 != NULL && tmp != NULL) {
					*str_line2 = strdup(tmp);
				}
			} else {
				tmp = eina_strbuf_string_get(line1);
				if (str_line1 != NULL && tmp != NULL) {
					*str_line1 = strdup(tmp);
				}
			}

			eina_strbuf_free(line1);
			eina_strbuf_free(line2);
		} else {
			_E("failed to allocate string buffer");
		}
	} else {
		if (title_utf8 == NULL
				&& event_count_utf8 == NULL
				&& content_utf8 == NULL
				&& info1_utf8 == NULL
				&& info1_sub_utf8 == NULL
				&& info2_utf8 == NULL
				&& info2_sub_utf8 == NULL) {
			len = 0;
		} else {
			Eina_Strbuf *strbuf = eina_strbuf_new();
			if (strbuf != NULL) {
				eina_strbuf_append(strbuf, _("IDS_QP_BUTTON_NOTIFICATION"));
				eina_strbuf_append_char(strbuf, '\n');
				_check_and_add_to_buffer(strbuf, title_utf8, 1);
				_check_and_add_to_buffer(strbuf, event_count_utf8, 0);
				_check_and_add_to_buffer(strbuf, content_utf8, 1);
				_check_and_add_to_buffer(strbuf, info1_utf8, 1);
				_check_and_add_to_buffer(strbuf, info1_sub_utf8, 1);
				_check_and_add_to_buffer(strbuf, info2_utf8, 1);
				_check_and_add_to_buffer(strbuf, info2_sub_utf8, 1);

				if (eina_strbuf_length_get(strbuf) > 0) {
					len = snprintf(buf, sizeof(buf) - 1, "%s", eina_strbuf_string_get(strbuf));
				}
				eina_strbuf_free(strbuf);
			}
		}
	}

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
	if (len > 0) {
		return strdup(buf);
	}

	return NULL;
}

static char *_ticker_get_label_layout_single(notification_h noti, int is_screenreader, char **str_line1, char **str_line2)
{
	int num_line = 0;
	int len = 0;
	char *domain = NULL;
	char *dir = NULL;
	char *title_utf8 = NULL;
	char *content_utf8 = NULL;
	char *info1_utf8 = NULL;
	char *info1_sub_utf8 = NULL;
	char *info2_utf8 = NULL;
	char *info2_sub_utf8 = NULL;
	Eina_Strbuf *line1 = NULL;
	Eina_Strbuf *line2 = NULL;
	const char *tmp = NULL;
	char buf[TICKER_MSG_LEN] = { 0, };

	retv_if(!noti, NULL);

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	title_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE);
	content_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT);
	info1_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_1);
	info1_sub_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_1);
	info2_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_2);
	info2_sub_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_SUB_2);

	if (is_screenreader == 0) {
		line1 = eina_strbuf_new();
		line2 = eina_strbuf_new();

		if (line1 != NULL && line2 != NULL) {
			if (_is_text_exist(info1_utf8) && _is_text_exist(content_utf8)) {
				_strbuf_add(line1, content_utf8, NULL);
				_strbuf_add(line2, info1_utf8, NULL);
				_strbuf_add(line2, info1_sub_utf8, " ");
				num_line = 2;
			} else if (_is_text_exist(title_utf8) && _is_text_exist(content_utf8)) {
				_strbuf_add(line1, title_utf8, NULL);
				_strbuf_add(line2, content_utf8, NULL);
				num_line = 2;
			} else if (_is_text_exist(title_utf8)) {
				_strbuf_add(line1, title_utf8, NULL);
				num_line = 1;
			} else if (_is_text_exist(content_utf8)) {
				_strbuf_add(line1, content_utf8, NULL);
				num_line = 1;
			}

			if (num_line == 2) {
				tmp = eina_strbuf_string_get(line1);
				if (str_line1 != NULL && tmp != NULL) {
					*str_line1 = strdup(tmp);
				}

				tmp = eina_strbuf_string_get(line2);
				if (str_line2 != NULL && tmp != NULL) {
					*str_line2 = strdup(tmp);
				}
			} else {
				tmp = eina_strbuf_string_get(line1);
				if (str_line1 != NULL && tmp != NULL) {
					*str_line1 = strdup(tmp);
				}
			}

			eina_strbuf_free(line1);
			eina_strbuf_free(line2);
		} else {
			_E("failed to allocate string buffer");
		}
	} else {
		if (title_utf8 == NULL
				&& content_utf8 == NULL
				&& info1_utf8 == NULL
				&& info1_sub_utf8 == NULL) {
			len = 0;
		} else {
			Eina_Strbuf *strbuf = eina_strbuf_new();
			if (strbuf != NULL) {
				eina_strbuf_append(strbuf, _("IDS_QP_BUTTON_NOTIFICATION"));
				eina_strbuf_append_char(strbuf, '\n');
				if (info1_utf8 == NULL) {
					_check_and_add_to_buffer(strbuf, title_utf8, 1);
					_check_and_add_to_buffer(strbuf, content_utf8, 1);
				} else {
					if (content_utf8 == NULL) {
						_check_and_add_to_buffer(strbuf, title_utf8, 1);
					}
					_check_and_add_to_buffer(strbuf, content_utf8, 1);
					_check_and_add_to_buffer(strbuf, info1_utf8, 1);
					_check_and_add_to_buffer(strbuf, info1_sub_utf8, 1);
				}

				if (eina_strbuf_length_get(strbuf) > 0) {
					len = snprintf(buf, sizeof(buf) - 1, "%s", eina_strbuf_string_get(strbuf));
				}
				eina_strbuf_free(strbuf);
			}
		}
	}

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
	if (len > 0) {
		return strdup(buf);
	}

	return NULL;
}

static char *_ticker_get_text(notification_h noti, int is_screenreader, char **line1, char **line2)
{
	char *result = NULL;
	notification_ly_type_e layout;

	retv_if(!noti, NULL);

	notification_get_layout(noti, &layout);

	if (layout == NOTIFICATION_LY_NOTI_EVENT_SINGLE) {
		result = _ticker_get_label_layout_single(noti, is_screenreader, line1, line2);
	} else {
		result = _ticker_get_label_layout_default(noti, is_screenreader, line1, line2);
	}

	return result;
}

static void _noti_hide_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	ticker_info_s *ticker_info = NULL;

	ret_if(!data);

	ticker_info = data;

	if (ticker_info->timer) {
		ecore_timer_del(ticker_info->timer);
		ticker_info->timer = NULL;
	}

	_destroy_ticker_noti(ticker_info);
}

static void _mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_noti_hide_cb(data, NULL, NULL, NULL);
}

static void _content_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	Evas_Coord h;
	struct Internal_Data *wd = evas_object_data_get(data, PRIVATE_DATA_KEY_DATA);

	ret_if(!wd);

	evas_object_size_hint_min_get(obj, NULL, &h);
	if (h > 0) {
		wd->h = h;
		evas_object_size_hint_min_set(obj, wd->w, wd->h);
		evas_object_size_hint_min_set(data, wd->w, wd->h);
	}
}

static void _sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
	struct Internal_Data *wd = evas_object_data_get(obj, PRIVATE_DATA_KEY_DATA);
	Evas_Object *sub = event_info;

	ret_if(!wd);

	if (sub == wd->content) {
		evas_object_event_callback_del(wd->content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _content_changed_size_hints);
		wd->content = NULL;
	}
}

static void _destroy_ticker_window(Evas_Object *window)
{
	struct Internal_Data *wd = NULL;

	ret_if(!window);

	evas_object_data_del(window, PRIVATE_DATA_KEY_NOTI);

	wd = evas_object_data_del(window, PRIVATE_DATA_KEY_DATA);
	ret_if(!wd);

	if (wd->rotation_event_handler) {
		ecore_event_handler_del(wd->rotation_event_handler);
	}
	free(wd);

	evas_object_del(window);
}

static void _rotate_cb(void *data, Evas_Object *obj, void *event)
{
	struct appdata *ad = data;
	Evas_Object *win = obj;
	struct Internal_Data *wd = NULL;
	Evas_Object *base = NULL;
	int angle = 0;

	ret_if(!ad);
	ret_if(!win);

	wd =  evas_object_data_get(win, PRIVATE_DATA_KEY_DATA);
	ret_if(!wd);

	base = evas_object_data_get(win, DATA_KEY_BASE_RECT);
	ret_if(!base);

	angle = elm_win_rotation_get(win);
	angle %= 360;
	if (angle < 0) {
		angle += 360;
	}
	_D("Ticker angle is %d degree", angle);

	switch (angle) {
	case 0:
	case 180:
		evas_object_resize(base, ad->win.port_w, wd->h);
		evas_object_size_hint_min_set(base, ad->win.port_w, wd->h);
		evas_object_resize(win, ad->win.port_w, wd->h);
		evas_object_move(win, 0, 0);
		break;
	case 90:
		evas_object_resize(base, ad->win.land_w, wd->h);
		evas_object_size_hint_min_set(base, ad->win.land_w, wd->h);
		evas_object_resize(win, ad->win.land_w, wd->h);
		evas_object_move(win, 0, 0);
		break;
	case 270:
		evas_object_resize(base, ad->win.land_w, wd->h);
		evas_object_size_hint_min_set(base, ad->win.land_w, wd->h);
		evas_object_resize(win, ad->win.land_w, wd->h);
		evas_object_move(win, ad->win.port_w - wd->h, 0);
		break;
	default:
		_E("cannot reach here");
	}

	wd->angle = angle;
}

static Evas_Object *_create_ticker_window(Evas_Object *parent, struct appdata *ad)
{
	Evas_Object *win = NULL;
	struct Internal_Data *wd = NULL;
	Evas *e = NULL;
	Ecore_Evas *ee = NULL;

	_D("A window is created for ticker notifications");

	win = elm_win_add(NULL, "noti_win", ELM_WIN_NOTIFICATION);
	retv_if(!win, NULL);

	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_title_set(win, "noti_win");
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);
	efl_util_set_notification_window_level(win, EFL_UTIL_NOTIFICATION_LEVEL_HIGH);
	elm_win_prop_focus_skip_set(win, EINA_TRUE);
	elm_win_role_set(win, "notification-normal");

	/* This is for rotation issue */
	e = evas_object_evas_get(win);
	goto_if(!e, error);

	ee = ecore_evas_ecore_evas_get(e);
	goto_if(!ee, error);

	ecore_evas_name_class_set(ee, "APP_POPUP", "APP_POPUP");

	/* you can use evas_object_resize() and evas_object_move() by using elm_win_aux_hint_add().
	   elm_win_aux_hint_add() makes it possible to set the size and location of the notification window freely.
	   if you do not use elm_win_aux_hint_add(), notification window is displayed full screen. */
	elm_win_aux_hint_add(win, "wm.policy.win.user.geometry", "1");
	evas_object_resize(win, ad->win.w, ad->win.h);
	evas_object_move(win, 0, 0);
	evas_object_hide(win);

	if (elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, rots, 4);
	}
	evas_object_smart_callback_add(win, "wm,rotation,changed", _rotate_cb, ad);

	wd = (struct Internal_Data *) calloc(1, sizeof(struct Internal_Data));
	goto_if(!wd, error);

	evas_object_data_set(win, PRIVATE_DATA_KEY_DATA, wd);
	evas_object_data_set(win, PRIVATE_DATA_KEY_NOTI, NULL);

	wd->angle = 0;
	wd->w = ad->win.w;
	wd->h = ad->win.h;

	evas_object_smart_callback_add(win, "sub-object-del", _sub_del, NULL);

	return win;

error:
	if (win)
		evas_object_del(win);

	return NULL;
}

static Evas_Object *_win_content_get(Evas_Object *obj)
{
	struct Internal_Data *wd;

	retv_if(!obj, NULL);

	wd = evas_object_data_get(obj, PRIVATE_DATA_KEY_DATA);
	retv_if(!wd, NULL);

	return wd->content;
}

static void _win_content_set(Evas_Object *obj, Evas_Object *content)
{
	struct Internal_Data *wd;

	ret_if (!obj);
	ret_if (!content);

	wd = evas_object_data_get(obj, PRIVATE_DATA_KEY_DATA);
	ret_if (!wd);

	if (wd->content) {
		evas_object_del(wd->content);
	}

	wd->content = content;
	if (wd->content) {
		evas_object_show(wd->content);
		evas_object_event_callback_add(wd->content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _content_changed_size_hints, obj);
	}
}

#define TICKER_EDJ EDJDIR"ticker.edj"
static void _create_ticker_noti(notification_h noti, struct appdata *ad, ticker_info_s *ticker_info)
{
	Eina_Bool ret = EINA_FALSE;
	Evas_Object *detail = NULL;
	Evas_Object *base = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *box = NULL;
	Evas_Object *textblock = NULL;
	const Evas_Object *tb;
	Evas_Object *edje_obj = NULL;
	char *line1 = NULL;
	char *line2 = NULL;
	int noti_height = 0;
	int *is_ticker_executed = NULL;
	int count = 0;

	notification_system_setting_h system_setting = NULL;
	notification_setting_h setting = NULL;
	bool do_not_disturb = false;
	bool do_not_disturb_exception = false;
	char *pkgname = NULL;

	ret_if(!ad);
	ret_if(!ticker_info);

	count = eina_list_count(ticker_info->ticker_list);
	if (count > 1) {
		_E("ticker notification exists");
		return;
	}

	_D("create ticker notification");

	/*
	 * @ Note
	 * Do Not Disturb
	 */
	ret = notification_system_setting_load_system_setting(&system_setting);
	if (ret != NOTIFICATION_ERROR_NONE || system_setting == NULL) {
		_E("Failed to load system_setting");
		return;
	}
	notification_system_setting_get_do_not_disturb(system_setting, &do_not_disturb);

	if (system_setting)
		notification_system_setting_free_system_setting(system_setting);

	ret = notification_get_pkgname(noti, &pkgname);
	if (ret != NOTIFICATION_ERROR_NONE) {
		_E("Failed to get package name");
	}

	ret = notification_setting_get_setting_by_package_name(pkgname, &setting);
	if (ret != NOTIFICATION_ERROR_NONE || setting == NULL) {
		_E("Failed to get setting by package name : %d", ret);
	} else {
		notification_setting_get_do_not_disturb_except(setting, &do_not_disturb_exception);

		if (setting)
			notification_setting_free_notification(setting);

		_D("do_not_disturb = %d, do_not_disturb_exception = %d", do_not_disturb, do_not_disturb_exception);
		if (do_not_disturb  == 1 && do_not_disturb_exception == 0)
			return;
	}

	/* create layout */
	detail = elm_layout_add(ad->ticker_win);
	goto_if(!detail, ERROR);

	ret = elm_layout_file_set(detail, util_get_res_file_path(TICKER_EDJ), "quickpanel/tickernoti/normal");

	goto_if(ret == EINA_FALSE, ERROR);

	elm_object_signal_callback_add(detail, "request,hide", "", _noti_hide_cb, ticker_info);
	evas_object_event_callback_add(detail, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, ticker_info);
	evas_object_size_hint_min_set(detail, 1, noti_height);
	_win_content_set(ad->ticker_win, detail);

	/* create base rectangle */
	base = evas_object_rectangle_add(evas_object_evas_get(detail));
	goto_if(!base, ERROR);
	evas_object_color_set(base, 0, 0, 0, 0);
	evas_object_resize(base, ad->win.w, ad->win.h);
	evas_object_size_hint_min_set(base, ad->win.w, ad->win.h);
	evas_object_show(base);
	elm_object_part_content_set(detail, "base", base);
	evas_object_data_set(ad->ticker_win, DATA_KEY_BASE_RECT, base);

	/* create icon */
	icon = _ticker_create_icon(detail, noti);
	if (icon) elm_object_part_content_set(detail, "icon", icon);
	evas_object_data_set(ad->ticker_win, PRIVATE_DATA_KEY_ICON, icon);

	/* create scroller */
	ticker_info->scroller = elm_scroller_add(detail);
	goto_if(!ticker_info->scroller, ERROR);

	elm_scroller_policy_set(ticker_info->scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_scroller_movement_block_set(ticker_info->scroller, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL|ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
	elm_object_part_content_set(detail, "text_rect", ticker_info->scroller);

	/* create box */
	box = elm_box_add(ticker_info->scroller);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	goto_if(!box, ERROR);

	elm_object_content_set(ticker_info->scroller, box);
	evas_object_show(box);
	evas_object_data_set(ad->ticker_win, PRIVATE_DATA_KEY_BOX, box);

	/* create textblock */
	textblock = elm_layout_add(box);
	goto_if(!textblock, ERROR);
	evas_object_size_hint_weight_set(textblock, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(textblock, EVAS_HINT_FILL, EVAS_HINT_FILL);

	ticker_info->current_page = 0;
	ticker_info->cursor_pos = 0;
	ticker_info->textblock = textblock;

	ret = elm_layout_file_set(textblock, util_get_res_file_path(TICKER_EDJ), "quickpanel/tickernoti/text");
	goto_if(ret == EINA_FALSE, ERROR);

	evas_object_show(textblock);

	elm_box_pack_end(box, textblock);

	edje_obj = elm_layout_edje_get(textblock);
	tb = edje_object_part_object_get(edje_obj, "elm.text");
	goto_if(!tb, ERROR);

	evas_object_event_callback_add((Evas_Object *)tb, EVAS_CALLBACK_RESIZE, _resize_textblock_cb, ticker_info);

	/* get noti text */
	_ticker_get_text(noti, 0, &line1, &line2);

	if (line1 == NULL) {
		if (line2 != NULL) {
			elm_object_part_text_set(textblock, "elm.text", line2);
			free(line2);
		}
	} else if (line2 == NULL) {
		elm_object_part_text_set(textblock, "elm.text", line1);
		free(line1);
	} else {
		Eina_Strbuf *buffer = eina_strbuf_new();

		eina_strbuf_append(buffer, line1);
		eina_strbuf_append(buffer, "<br/>");
		eina_strbuf_append(buffer, line2);

		elm_object_part_text_set(textblock, "elm.text", eina_strbuf_string_get(buffer));

		free(line1);
		free(line2);
		eina_strbuf_free(buffer);
	}
	evas_object_data_set(ad->ticker_win, DATA_KEY_TICKER_TEXT, textblock);

	evas_object_show(ad->ticker_win);

	is_ticker_executed = (int *)malloc(sizeof(int));
	if (is_ticker_executed != NULL) {
		*is_ticker_executed = 0;
		evas_object_data_set(detail, PRIVATE_DATA_KEY_TICKERNOTI_EXECUTED, is_ticker_executed);
	}

	/* When ticker noti is displayed, indicator window has to be hidden. */
	if (ad) util_signal_emit_by_win(&ad->win, "message.show.noeffect", "indicator.prog");

	ticker_info->timer = ecore_timer_add(TICKERNOTI_DURATION, _timeout_cb, ticker_info);

	evas_object_data_set(ad->win.win, PRIVATE_DATA_KEY_TICKER_INFO, ticker_info);
	evas_object_data_set(ticker_info->scroller, PRIVATE_DATA_KEY_APPDATA, ad);

	_rotate_cb(ad, ad->ticker_win, NULL);

	return;

ERROR:
	if (ad->ticker_win) _destroy_ticker_noti(ticker_info);
}

static void _destroy_ticker_noti(ticker_info_s *ticker_info)
{
	struct appdata *ad = NULL;
	Evas_Object *textblock = NULL;
	Evas_Object *box = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *detail = NULL;
	Evas_Object *base = NULL;
	int *is_ticker_executed = NULL;
	notification_h noti;

	ret_if(!ticker_info);

	_D("destroy ticker notification");

	ad = evas_object_data_del(ticker_info->scroller, PRIVATE_DATA_KEY_APPDATA);
	ret_if(!ad);
	ret_if(!ad->ticker_win);

	/* When ticker noti is hidden, indicator window has to be displayed. */
	if (ad)	util_signal_emit_by_win(&ad->win, "message.hide", "indicator.prog");

	if (ticker_info->timer) {
		ecore_timer_del(ticker_info->timer);
		ticker_info->timer = NULL;
	}

	textblock = evas_object_data_del(ad->ticker_win, DATA_KEY_TICKER_TEXT);
	if (textblock) evas_object_del(textblock);

	box = evas_object_data_del(ad->ticker_win, PRIVATE_DATA_KEY_BOX);
	if (box) evas_object_del(box);

	if (ticker_info->scroller) ticker_info->scroller = NULL;

	icon = evas_object_data_del(ad->ticker_win, PRIVATE_DATA_KEY_ICON);
	if (icon) evas_object_del(icon);

	base = evas_object_data_del(ad->ticker_win, DATA_KEY_BASE_RECT);
	if (base) evas_object_del(base);

	detail = _win_content_get(ad->ticker_win);
	if (detail) {
		is_ticker_executed = evas_object_data_del(detail, PRIVATE_DATA_KEY_TICKERNOTI_EXECUTED);
		if (is_ticker_executed != NULL) {
			free(is_ticker_executed);
		}
		_win_content_set(ad->ticker_win, NULL);
	}

	evas_object_hide(ad->ticker_win);

	noti = evas_object_data_del(ad->ticker_win, PRIVATE_DATA_KEY_NOTI);
	if (noti) {
		_request_to_delete_noti(noti);
		notification_free(noti);
	}

	if (ticker_info->ticker_list) {
		noti = _get_instant_latest_message_from_list(ticker_info);
		if (noti) {
			_create_ticker_noti(noti, ad, ticker_info);
		}
	}
}

static void _ticker_noti_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	struct appdata *ad = data;
	notification_h noti = NULL;
	notification_h noti_from_master = NULL;
	ticker_info_s *ticker_info = NULL;
	char *pkgname = NULL;
	int flags = 0;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;
	int op_type = 0;
	int priv_id = 0;
	int lock_state = SYSTEM_SETTINGS_LOCK_STATE_UNLOCK;
	int ret;

	ret_if(!ad);
	ret_if(!op_list);

	_D("");

	ret_if(num_op < 1);
	/* FIXME : num_op can be more than 1 */
	ret_if(num_op > 1);

	ret = system_settings_get_value_int(SYSTEM_SETTINGS_KEY_LOCK_STATE, &lock_state);
	if (ret != SYSTEM_SETTINGS_ERROR_NONE || lock_state == SYSTEM_SETTINGS_LOCK_STATE_LOCK)
		return;

	notification_op_get_data(op_list, NOTIFICATION_OP_DATA_TYPE, &op_type);
	notification_op_get_data(op_list, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
	notification_op_get_data(op_list, NOTIFICATION_OP_DATA_NOTI, &noti_from_master);

	//TODO: Below functions are depracated and should not be used
	ret = notification_op_get_data(op_list, NOTIFICATION_OP_DATA_TYPE, &op_type);
	ret_if(ret != NOTIFICATION_ERROR_NONE);
	ret = notification_op_get_data(op_list, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
	ret_if(ret != NOTIFICATION_ERROR_NONE);
	ret = notification_op_get_data(op_list, NOTIFICATION_OP_DATA_NOTI, &noti_from_master);
	ret_if(ret != NOTIFICATION_ERROR_NONE);

	if (op_type != NOTIFICATION_OP_INSERT && op_type != NOTIFICATION_OP_UPDATE) {
		return;
	}

	if (!noti_from_master) {
		_E("failed to get a notification from master");
		return;
	}

	if (notification_clone(noti_from_master, &noti) != NOTIFICATION_ERROR_NONE) {
		_E("failed to create a cloned notification");
		goto ERROR;
	}

	ret_if(!noti);

	notification_get_display_applist(noti, &applist);
	if (!((applist & NOTIFICATION_DISPLAY_APP_TICKER) || (applist & NOTIFICATION_DISPLAY_APP_INDICATOR))) {
		_D("displaying ticker option is off");
		goto ERROR;
	}

	if (_is_security_lockscreen_launched()) {
		_E("lockscreen launched, creating a ticker canceled");
		/* FIXME : In this case, we will remove this notification */
		_request_to_delete_noti(noti);
		goto ERROR;
	}

	notification_get_pkgname(noti, &pkgname);
	if (!pkgname) {
		_D("Disabled tickernoti");
		_request_to_delete_noti(noti);
		goto ERROR;
	}

//	ticker_info = evas_object_data_get(ad->win.win, PRIVATE_DATA_KEY_TICKER_INFO);
//	if (!ticker_info) {
		ticker_info = calloc(1, sizeof(ticker_info_s));
		if (!ticker_info) {
			_E("calloc error");
			_request_to_delete_noti(noti);
			goto ERROR;
		}
//	}

	notification_get_property(noti, &flags);
	if (flags & NOTIFICATION_PROP_DISABLE_TICKERNOTI) {
		_D("Disabled tickernoti");
		goto ERROR;
	}

	/* Moved cause of crash issue */
	evas_object_data_set(ad->ticker_win, PRIVATE_DATA_KEY_NOTI, noti);

	ticker_info->ticker_list = eina_list_append(ticker_info->ticker_list, noti);
	_create_ticker_noti(noti, ad, ticker_info);

	return;

ERROR:
	if (ticker_info) free(ticker_info);
	if (noti) notification_free(noti);
}

int ticker_init(void *data)
{
	struct appdata *ad = data;

	ad->ticker_win = _create_ticker_window(NULL, ad);
	retv_if(!ad->ticker_win, 0);

	notification_register_detailed_changed_cb(_ticker_noti_detailed_changed_cb, ad);

	return INDICATOR_ERROR_NONE;
}

int ticker_fini(void *data)
{
	struct appdata *ad = data;
	ticker_info_s *ticker_info = NULL;

	retv_if(!ad, 0);

	ticker_info = evas_object_data_del(ad->win.win, PRIVATE_DATA_KEY_TICKER_INFO);
	retv_if(!ticker_info, 0);

	_destroy_ticker_window(ad->ticker_win);
	ad->ticker_win = NULL;

	if (ticker_info->timer) {
		ecore_timer_del(ticker_info->timer);
		ticker_info->timer = NULL;
	}

	return INDICATOR_ERROR_NONE;
}
/* End of file */
