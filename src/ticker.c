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
#include <feedback.h>
#include <system_settings.h>
#include <player.h>
#include <sound_manager.h>
#include <metadata_extractor.h>
#include <notification.h>
#include <notification_text_domain.h>
#include <app_manager.h>

#include "common.h"
#include "main.h"
#include "noti_win.h"
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
#define PRIVATE_DATA_KEY_DETAIL "pdkd"
#define PRIVATE_DATA_KEY_TICKERNOTI_EXECUTED "pdkte"
#define PRIVATE_DATA_KEY_ANI_ICON_TYPE "pdkait"
#define PRIVATE_DATA_KEY_ICON "pdki"
#define PRIVATE_DATA_KEY_BOX "pdkb"
#define PRIVATE_DATA_KEY_TICKER_INFO "pdkti"
#define PRIVATE_DATA_KEY_NOTI "pdkn"

#define PATH_DOWNLOAD "reserved://quickpanel/ani/downloading"
#define PATH_UPLOAD "reserved://quickpanel/ani/uploading"
#define PATH_INSTALL "reserved://quickpanel/ani/install"

static void _create_tickernoti(notification_h noti, struct appdata *ad, ticker_info_s *ticker_info);
static void _destroy_tickernoti(ticker_info_s *ticker_info);

static inline int _is_text_exist(const char *text)
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

static int _check_is_noti_from_email(char *pkgname)
{
	retv_if(!pkgname, 0);

	if (strcmp(pkgname, TIZEN_EMAIL_PACKAGE) == 0 || strcmp(pkgname, "/usr/bin/eas-engine") == 0) {
		return 1;
	} else {
		return 0;
	}
}

static int _check_is_noti_from_message(char *pkgname)
{
	retv_if(!pkgname, 0);

	if (strcmp(pkgname, TIZEN_MESSAGE_PACKAGE) == 0 || strcmp(pkgname, "/usr/bin/msg-server") == 0) {
		return 1;
	} else {
		return 0;
	}
}

static int _check_is_noti_from_im(char *pkgname)
{
	retv_if(!pkgname, 0);

	if (strcmp(pkgname, "xnq5eh9vop.ChatON") == 0) {
		return 1;
	} else {
		return 0;
	}
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

static int _ticker_check_ticker_off(notification_h noti)
{
	char *pkgname = NULL;

	notification_get_pkgname(noti, &pkgname);

	if (pkgname == NULL) return 1;	/* Ticker is not displaying. */

	return 0;
}

static int _ticker_check_displaying_contents_off(notification_h noti)
{
	char *pkgname = NULL;
	int ret = 0;
	int boolval = 0;

	notification_get_pkgname(noti, &pkgname);

	if (pkgname == NULL) return 0;	/* Ticker is not displaying. */

	/* FIXME : we have to confirm architecture for communiating with message or email */
	if (_check_is_noti_from_message(pkgname) == 1) {
		ret = vconf_get_bool(VCONFKEY_TICKER_NOTI_DISPLAY_CONTENT_MESSASGES, &boolval);
		if (ret == 0 && boolval == 0) return 1;

	} else if (_check_is_noti_from_email(pkgname) == 1) {
		ret = vconf_get_bool(VCONFKEY_TICKER_NOTI_DISPLAY_CONTENT_EMAIL, &boolval);
		if (ret == 0 && boolval == 0) return 1;

	} else if (_check_is_noti_from_im(pkgname) == 1) {
		ret = vconf_get_bool(VCONFKEY_TICKER_NOTI_DISPLAY_CONTENT_IM, &boolval);
		if (ret == 0 && boolval == 0) return 1;

	}

	return 0;
}

static inline void __ticker_only_noti_del(notification_h noti)
{
	int applist = NOTIFICATION_DISPLAY_APP_ALL;

	ret_if(!noti);

	notification_get_display_applist(noti, &applist);
	if ((applist & NOTIFICATION_DISPLAY_APP_TICKER) || (applist & NOTIFICATION_DISPLAY_APP_INDICATOR)) {
		char *pkgname = NULL;
		int priv_id = 0;

		notification_get_pkgname(noti, &pkgname);
		notification_get_id(noti, NULL, &priv_id);
		notification_delete_by_priv_id(pkgname, NOTIFICATION_TYPE_NONE, priv_id);
	}
}

static Eina_Bool _timeout_cb(void *data)
{
	ticker_info_s *ticker_info = NULL;
	int h_page = 0;
	int v_page = 0;
	int h_last_page = 0;
	int v_last_page = 0;

	retv_if(!data, EINA_FALSE);

	_D("message is timeout");

	ticker_info = data;

	/* If count is 1, self*/
	if (ticker_info->ticker_list && eina_list_count(ticker_info->ticker_list) > 1) {
		if (ticker_info->timer) {
			ecore_timer_del(ticker_info->timer);
			ticker_info->timer = NULL;
		}
		_destroy_tickernoti(ticker_info);

		return ECORE_CALLBACK_CANCEL;
	}

	elm_scroller_last_page_get(ticker_info->scroller, &h_last_page, &v_last_page);
	elm_scroller_current_page_get(ticker_info->scroller, &h_page, &v_page);

	if (v_last_page > v_page) {
		elm_scroller_page_bring_in(ticker_info->scroller, h_page, v_page + 1);

		return ECORE_CALLBACK_RENEW;
	}

	if (ticker_info->timer) {
		ecore_timer_del(ticker_info->timer);
		ticker_info->timer = NULL;
	}
	_destroy_tickernoti(ticker_info);

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
	if (layout != NULL) {
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

	retvm_if(pkgid == NULL, NULL, "invalid parameter");

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

	retvm_if(noti == NULL || parent == NULL, NULL, "Invalid parameter!");

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

static inline char *_get_text(notification_h noti, notification_text_type_e text_type)
{
	char *text = NULL;

	notification_get_text(noti, text_type, &text);
	if (text) {
		return elm_entry_utf8_to_markup(text);
	}

	return NULL;
}

static inline void _strbuf_add(Eina_Strbuf *str_buf, char *text, const char *delimiter)
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
	retvm_if(address == NULL, 0, "address is NULL");

	int addr_len = 0;
	addr_len = strlen(address);

	if (addr_len == 0) {
		return 0;
	}

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

		if (*pszOneChar == '+') {
			++pszOneChar;
		}

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
	retm_if(dst == NULL, "invalid argument");
	retm_if(src == NULL, "invalid argument");

	int no_op = 0;
	int i = 0, j = 0, text_len = 0;

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

static inline void _check_and_add_to_buffer(Eina_Strbuf *str_buf, char *text, int is_check_phonenumber)
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

	retvm_if(noti == NULL, NULL, "Invalid parameter!");

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL) {
		bindtextdomain(domain, dir);
	}

	title_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE);
	if (_ticker_check_displaying_contents_off(noti) == 1) {
		content_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT_FOR_DISPLAY_OPTION_IS_OFF);
	} else {
		content_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT);
	}
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

//	if (event_count_utf8) {
//		free(event_count_utf8);
//	}

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

	retvm_if(noti == NULL, NULL, "Invalid parameter!");

	notification_get_text_domain(noti, &domain, &dir);
	if (domain != NULL && dir != NULL)
		bindtextdomain(domain, dir);

	title_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE);
	if (_ticker_check_displaying_contents_off(noti) == 1) {
		content_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT_FOR_DISPLAY_OPTION_IS_OFF);
	} else {
		content_utf8 = _get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT);
	}
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

	retvm_if(noti == NULL, NULL, "Invalid parameter!");

	notification_get_layout(noti, &layout);

	if (_ticker_check_displaying_contents_off(noti) == 1) {
		result = _ticker_get_label_layout_default(noti, is_screenreader, line1, line2);
	} else if (layout == NOTIFICATION_LY_NOTI_EVENT_SINGLE) {
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

	_destroy_tickernoti(ticker_info);
}

static void _mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_noti_hide_cb(data, NULL, NULL, NULL);
}

#define TICKER_EDJ EDJDIR"ticker.edj"
static void _create_tickernoti(notification_h noti, struct appdata *ad, ticker_info_s *ticker_info)
{
	Eina_Bool ret = EINA_FALSE;
	Evas_Object *detail = NULL;
	Evas_Object *base = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *box = NULL;
	Evas_Object *textblock = NULL;
	char *line1 = NULL;
	char *line2 = NULL;
	int noti_height = 0;
	int *is_ticker_executed = NULL;

	ret_if(!ad);
	ret_if(!ticker_info);

	if (ticker_info->win != NULL) {
		_E("ticker notification exists");
		return;
	}

	_D("create ticker notification");

	/* create window */
	ticker_info->win = noti_win_add(NULL, ad);
	ret_if(!ticker_info->win);
	evas_object_data_set(ticker_info->win, PRIVATE_DATA_KEY_APPDATA, ad);
	evas_object_data_set(ticker_info->win, PRIVATE_DATA_KEY_NOTI, noti);

	/* create layout */
	detail = elm_layout_add(ticker_info->win);
	goto_if(!detail, ERROR);

	//FIXME ticker.c: _create_tickernoti(837) > (ret == EINA_FALSE) -> goto
	ret = elm_layout_file_set(detail, util_get_res_file_path(TICKER_EDJ), "quickpanel/tickernoti/normal");
	goto_if(ret == EINA_FALSE, ERROR);

	elm_object_signal_callback_add(detail, "request,hide", "", _noti_hide_cb, ticker_info);
	evas_object_event_callback_add(detail, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, ticker_info);
	evas_object_size_hint_min_set(detail, 1, noti_height);
	noti_win_content_set(ticker_info->win, detail);
	evas_object_data_set(ticker_info->win, PRIVATE_DATA_KEY_DETAIL, detail);

	/* create base rectangle */
	base = evas_object_rectangle_add(evas_object_evas_get(detail));
	goto_if(!base, ERROR);
	/* FIXME */
	evas_object_color_set(base, 0, 0, 0, 0);
	evas_object_resize(base, ad->win.w, ad->win.h);
	evas_object_size_hint_min_set(base, ad->win.w, ad->win.h);
	evas_object_show(base);
	elm_object_part_content_set(detail, "base", base);
	evas_object_data_set(ticker_info->win, DATA_KEY_BASE_RECT, base);

	/* create icon */
	icon = _ticker_create_icon(detail, noti);
	if (icon) elm_object_part_content_set(detail, "icon", icon);
	evas_object_data_set(ticker_info->win, PRIVATE_DATA_KEY_ICON, icon);

	/* create scroller */
	ticker_info->scroller = elm_scroller_add(detail);
	goto_if(!ticker_info->scroller, ERROR);

	elm_scroller_policy_set(ticker_info->scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_scroller_page_size_set(ticker_info->scroller, 434, INDICATOR_HEIGHT - 5);
	elm_scroller_movement_block_set(ticker_info->scroller, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL|ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
	elm_object_part_content_set(detail, "text_rect", ticker_info->scroller);

	/* create box */
	box = elm_box_add(ticker_info->scroller);
	goto_if(!box, ERROR);

	elm_object_content_set(ticker_info->scroller, box);
	evas_object_show(box);
	evas_object_data_set(ticker_info->win, PRIVATE_DATA_KEY_BOX, box);

	/* create textblock */
	textblock = elm_layout_add(box);
	goto_if(!textblock, ERROR);

	ret = elm_layout_file_set(textblock, util_get_res_file_path(TICKER_EDJ), "quickpanel/tickernoti/text");
	goto_if(ret == EINA_FALSE, ERROR);

	evas_object_show(textblock);

	elm_box_pack_end(box, textblock);

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
	evas_object_data_set(ticker_info->win, DATA_KEY_TICKER_TEXT, textblock);

	is_ticker_executed = (int *)malloc(sizeof(int));
	if (is_ticker_executed != NULL) {
		*is_ticker_executed = 0;
		evas_object_data_set(detail, PRIVATE_DATA_KEY_TICKERNOTI_EXECUTED, is_ticker_executed);
	}

	/* When ticker noti is displayed, indicator window has to be hidden. */
	if (ad) util_signal_emit_by_win(&ad->win, "message.show.noeffect", "indicator.prog");

	ticker_info->timer = ecore_timer_add(TICKERNOTI_DURATION, _timeout_cb, ticker_info);

	evas_object_data_set(ad->win.win, PRIVATE_DATA_KEY_TICKER_INFO, ticker_info);

	return;

ERROR:
	if (ticker_info->win) _destroy_tickernoti(ticker_info);

	return;
}

static void _destroy_tickernoti(ticker_info_s *ticker_info)
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
	ret_if(!ticker_info->win);

	_D("destroy ticker notification");

	ad = evas_object_data_del(ticker_info->win, PRIVATE_DATA_KEY_APPDATA);
	/* When ticker noti is hidden, indicator window has to be displayed. */
	if (ad)	util_signal_emit_by_win(&ad->win, "message.hide", "indicator.prog");

	if (ticker_info->timer) {
		ecore_timer_del(ticker_info->timer);
		ticker_info->timer = NULL;
	}

	textblock = evas_object_data_del(ticker_info->win, DATA_KEY_TICKER_TEXT);
	if (textblock) evas_object_del(textblock);

	box = evas_object_data_del(ticker_info->win, PRIVATE_DATA_KEY_BOX);
	if (box) evas_object_del(box);

	if (ticker_info->scroller) ticker_info->scroller = NULL;

	icon = evas_object_data_del(ticker_info->win, PRIVATE_DATA_KEY_ICON);
	if (icon) evas_object_del(icon);

	base = evas_object_data_del(ticker_info->win, DATA_KEY_BASE_RECT);
	if (base) evas_object_del(base);

	detail = evas_object_data_del(ticker_info->win, PRIVATE_DATA_KEY_DETAIL);
	if (detail) {
		is_ticker_executed = evas_object_data_del(detail, PRIVATE_DATA_KEY_TICKERNOTI_EXECUTED);
		if (is_ticker_executed != NULL) {
			free(is_ticker_executed);
		}
		evas_object_del(detail);
	}

	evas_object_del(ticker_info->win);
	ticker_info->win = NULL;

	noti = evas_object_data_del(ticker_info->win, PRIVATE_DATA_KEY_NOTI);
	if (noti) {
		__ticker_only_noti_del(noti);
		notification_free(noti);
	}

	if (ticker_info->ticker_list) {
		noti = _get_instant_latest_message_from_list(ticker_info);
		if (noti) {
			_create_tickernoti(noti, ad, ticker_info);
		}
		else
			free(ticker_info);
	}
}

static void _ticker_noti_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	notification_h noti = NULL;
	notification_h noti_from_master = NULL;
	ticker_info_s *ticker_info = NULL;
	int flags = 0;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;
	int ret = 0;
	int op_type = 0;
	int priv_id = 0;

	ret_if(!op_list);

	_D("_ticker_noti_changed_cb");

	if (num_op == 1) {
		//TODO: Functions below are depracated, should not be used
		ret = notification_op_get_data(op_list, NOTIFICATION_OP_DATA_TYPE, &op_type);
		ret_if(ret != NOTIFICATION_ERROR_NONE);
		ret = notification_op_get_data(op_list, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
		ret_if(ret != NOTIFICATION_ERROR_NONE);
		ret = notification_op_get_data(op_list, NOTIFICATION_OP_DATA_NOTI, &noti_from_master);
		ret_if(ret != NOTIFICATION_ERROR_NONE);

		_D("op_type:%d", op_type);
		_D("op_priv_id:%d", priv_id);
		_D("noti:%p", noti_from_master);

		if (op_type != NOTIFICATION_OP_INSERT &&
				op_type != NOTIFICATION_OP_UPDATE) {
			return;
		}
		if (noti_from_master == NULL) {
			_E("failed to get a notification from master");
			return;
		}
		if (notification_clone(noti_from_master, &noti) != NOTIFICATION_ERROR_NONE) {
			_E("failed to create a cloned notification");
			return;
		}
#ifdef QP_EMERGENCY_MODE_ENABLE
		if (quickpanel_emergency_mode_is_on()) {
			if (quickpanel_emergency_mode_notification_filter(noti, 1)) {
				_D("notification filtered");
				notification_free(noti);
				return;
			}
		}
#endif
	}

	ret_if(!noti);

	notification_get_display_applist(noti, &applist);
	if (!((applist & NOTIFICATION_DISPLAY_APP_TICKER)
				|| (applist & NOTIFICATION_DISPLAY_APP_INDICATOR))) {
		_D("displaying ticker option is off");
		notification_free(noti);
		return;
	}

	/* Check setting's event notification */
	ret = _ticker_check_ticker_off(noti);
	if (ret == 1) {
		_D("Disabled tickernoti ret : %d", ret);
		/* delete temporary here only ticker noti display item */
		__ticker_only_noti_del(noti);
		notification_free(noti);

		return;
	}

	ticker_info = calloc(1, sizeof(ticker_info_s));
	ret_if(ticker_info == NULL);

	/* Skip if previous ticker is still shown */
/*
	if (ticker_info->win != NULL) {
		_D("delete ticker noti");
		_destroy_tickernoti();
		ticker_info->win = NULL;
	}
*/

	/* Check tickernoti flag */
	notification_get_property(noti, &flags);

	if (flags & NOTIFICATION_PROP_DISABLE_TICKERNOTI) {
		_D("NOTIFICATION_PROP_DISABLE_TICKERNOTI");
		__ticker_only_noti_del(noti);
		notification_free(noti);
	} else if ((applist & NOTIFICATION_DISPLAY_APP_TICKER)
			|| (applist & NOTIFICATION_DISPLAY_APP_INDICATOR)) {

		ticker_info->ticker_list = eina_list_append(ticker_info->ticker_list, noti);
		/* wait when win is not NULL */
		if (ticker_info->win == NULL) {
			_create_tickernoti(noti, data, ticker_info);
		}
		if (ticker_info->win == NULL) {
			_E("Fail to create tickernoti");
			__ticker_only_noti_del(noti);
			notification_free(noti);
			return;
		}
	}
}

static Eina_Bool _tickernoti_callback_register_idler_cb(void *data)
{
	retv_if(!data, EINA_FALSE);

	//TODO: Function below is not in public API
	notification_register_detailed_changed_cb(_ticker_noti_detailed_changed_cb, data);

	return EINA_FALSE;
}

int ticker_init(void *data)
{
	/* data is ad */
	/* Register notification changed cb */
	ecore_idler_add(_tickernoti_callback_register_idler_cb, data);

	return INDICATOR_ERROR_NONE;
}

int ticker_fini(void *data)
{
	struct appdata *ad = NULL;
	ticker_info_s *ticker_info = NULL;

	retv_if(!data, 0);

	ad = data;

	ticker_info = evas_object_data_del(ad->win.win, PRIVATE_DATA_KEY_TICKER_INFO);
	retv_if(!ticker_info, 0);

	if (ticker_info->timer) {
		ecore_timer_del(ticker_info->timer);
		ticker_info->timer = NULL;
	}

	return INDICATOR_ERROR_NONE;
}

/* End of file */
