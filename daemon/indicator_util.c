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


#include <Ecore.h>
#include <Ecore_X.h>
#include "common.h"
#include "indicator.h"
#include "indicator_ui.h"
#include "indicator_gui.h"
#include "indicator_util.h"
#include "indicator_icon_util.h"
#include "indicator_box_util.h"
#include <vconf.h>

#define DEFAULT_DIR	ICONDIR
#define DIR_PREFIX	"Theme_%02d_"
#define LABEL_STRING	"<color=#%02x%02x%02x%02x>%s</color>"

static char *_icondir;

char *set_label_text_color(const char *txt)
{
	Eina_Strbuf *temp_buf = NULL;
	Eina_Bool buf_result = EINA_FALSE;
	char *ret_str = NULL;

	retif(txt == NULL, NULL, "Invalid parameter!");

	temp_buf = eina_strbuf_new();
	buf_result = eina_strbuf_append_printf(temp_buf,
				LABEL_STRING, FONT_COLOR, txt);

	if (buf_result == EINA_FALSE)
		INFO("Failed to make label string!");
	else
		ret_str = eina_strbuf_string_steal(temp_buf);

	eina_strbuf_free(temp_buf);
	return ret_str;
}

const char *get_icon_dir(void)
{
	if (_icondir == NULL)
		_icondir = DEFAULT_DIR;

	return (const char *)_icondir;
}

static int _set_icon_dir(char *newdir)
{
	char *new_icon_dir = NULL;

	char dirname[PATH_MAX];
	int r;

	retif(!newdir, FAIL, "Invalid parameter!");
	memset(dirname, 0x00, sizeof(dirname));
	r = snprintf(dirname, sizeof(dirname), ICONDIR "/%s", newdir);
	if (r < 0) {
		ERR("Failed to set new dir name!");
		return FAIL;
	}

	new_icon_dir = strdup(dirname);
	_icondir = new_icon_dir;

	return 0;
}

void indicator_signal_emit(void* data, const char *emission, const char *source)
{
	struct appdata *ad = (struct appdata *)data;
	retif(data == NULL, , "Invalid parameter!");
	Evas_Object *edje;
	int i = 0;

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		retif(ad->win[i].layout_main == NULL, , "Invalid parameter!");
		edje = elm_layout_edje_get(ad->win[i].layout_main);
		edje_object_signal_emit(edje, emission, source);
	}
}

void indicator_part_text_emit(void* data, const char *part, const char *text)
{
	struct appdata *ad = (struct appdata *)data;
	retif(data == NULL, , "Invalid parameter!");
	Evas_Object *edje;
	int i = 0;

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		retif(ad->win[i].layout_main == NULL, , "Invalid parameter!");
		edje = elm_layout_edje_get(ad->win[i].layout_main);
		edje_object_part_text_set(edje, part, text);
	}
}

void indicator_signal_emit_by_win(void* data, const char *emission, const char *source)
{
	win_info *win = (win_info*)data;
	retif(data == NULL, , "Invalid parameter!");
	Evas_Object *edje;

	retif(win->layout_main == NULL, , "Invalid parameter!");
	edje = elm_layout_edje_get(win->layout_main);
	edje_object_signal_emit(edje, emission, source);
}

void indicator_part_text_emit_by_win(void* data, const char *part, const char *text)
{
	win_info *win = (win_info*)data;
	retif(data == NULL, , "Invalid parameter!");
	Evas_Object *edje;

	retif(win->layout_main == NULL, , "Invalid parameter!");
	edje = elm_layout_edje_get(win->layout_main);
	edje_object_part_text_set(edje, part, text);
}

void indicator_send_evas_ecore_message(void* data, int bRepeat, int bType)
{
	Ecore_Evas *ee_port;
	win_info* win = (win_info*)data;
	retif(data == NULL, , "Invalid parameter!");

	DBG("win(%d),bRepeat(%d),bType(%d)",win->type, bRepeat, bType);
	ee_port = ecore_evas_ecore_evas_get(evas_object_evas_get(win->win_main));
	ecore_evas_msg_send(ee_port, MSG_DOMAIN_CONTROL_INDICATOR, MSG_ID_INDICATOR_REPEAT_EVENT, &bRepeat, sizeof(int));
	ecore_evas_msg_send(ee_port, MSG_DOMAIN_CONTROL_INDICATOR, MSG_ID_INDICATOR_TYPE, &bType, sizeof(int));

}
