/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
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


#include <Eina.h>
#include "common.h"
#include "indicator_box_util.h"
#include "indicator_icon_util.h"
#include "indicator_icon_list.h"
#include "indicator_ui.h"
#include "indicator_gui.h"
#include "indicator_util.h"

static unsigned int update_icon_flag = 1;

static void _animation_set(Indicator_Icon_Object *icon, int type)
{
	Evas_Object *img_edje, *txt_edje;

	retif(icon == NULL, , "Invalid parameter!");

	const char *BLINK_SIGNAL = "icon,state,blink";
	const char *ROATATE_SIGNAL = "icon,state,roate";
	const char *METRONOME_SIGNAL = "icon,state,metronome";
	const char *DEFAULT = "icon,state,default";

	const char *send_signal = DEFAULT;

	switch (type) {
	case ICON_ANI_BLINK:
		send_signal = BLINK_SIGNAL;
		break;
	case ICON_ANI_ROTATE:
		send_signal = ROATATE_SIGNAL;
		break;
	case ICON_ANI_METRONOME:
		send_signal = METRONOME_SIGNAL;
		break;
	default:
		break;
	}

	switch (icon->type) {
	case INDICATOR_IMG_ICON:
		img_edje = elm_layout_edje_get(icon->img_obj.obj);
		edje_object_signal_emit(img_edje, send_signal,
					"elm.swallow.icon");
		break;
	case INDICATOR_TXT_ICON:
		txt_edje = elm_layout_edje_get(icon->txt_obj.obj);
		edje_object_signal_emit(txt_edje, send_signal,
					"elm.swallow.icon");
		break;
	case INDICATOR_TXT_WITH_IMG_ICON:
		txt_edje = elm_layout_edje_get(icon->txt_obj.obj);
		img_edje = elm_layout_edje_get(icon->img_obj.obj);
		edje_object_signal_emit(txt_edje, send_signal,
					"elm.swallow.lefticon");
		edje_object_signal_emit(img_edje, send_signal,
					"elm.swallow.righticon");
		break;
	default:
		break;
	}
}

void indicator_util_icon_animation_set(Indicator_Icon_Object *icon,
				       enum indicator_icon_ani type)
{
	retif(icon == NULL, , "Invalid parameter!");

	icon->ani = type;
	if (icon->obj_exist)
	{
		_animation_set(icon, type);
	}
}

static Evas_Object *_img_icon_add(win_info *win, Indicator_Icon_Object *icon)
{
	struct appdata *ad = NULL;
	char path[PATH_MAX];
	Evas_Object *evas_icon;
	Evas_Object *ly;
	int area = 0;
	char *imgpath = NULL;
	int width = 0;
	int height = 0;

	retif(icon == NULL , NULL, "Invalid parameter!");
	retif(icon->ad == NULL || icon->img_obj.data == NULL,	NULL, "Invalid parameter!");
	retif(win == NULL || win->layout_main == NULL, NULL, "Invalid parameter!");

	ad = icon->ad;
	area = icon->area;
	imgpath = icon->img_obj.data;

	if (icon->img_obj.width <= 0)
		width = icon->img_obj.width = DEFAULT_ICON_WIDTH;
	if (icon->img_obj.height<= 0)
		height = icon->img_obj.height = DEFAULT_ICON_HEIGHT;

	memset(path, 0x00, sizeof(path));

	ly = elm_layout_add(win->layout_main);
	retif(ly == NULL, NULL, "Cannot create layout object!");

	if (area == INDICATOR_ICON_AREA_FIXED)
		elm_layout_file_set(ly, ICON_THEME_FILE,
				"elm/indicator/icon/base");
	else
		elm_layout_file_set(ly, ICON_NONFIXED_THEME_FILE,
				"elm/indicator/icon/base");

	evas_icon = elm_image_add(ly);
	retif(evas_icon == NULL, NULL, "Cannot create elm icon object!");

	if (strncmp(imgpath, "/", 1) != 0) {
		snprintf(path, sizeof(path), "%s/%s", get_icon_dir(), imgpath);
	} else {
		strncpy(path,imgpath,sizeof(path)-1);
	}

	if (!ecore_file_exists(path))
	{
		ERR("icon file does not exist!!: %s",path);
	}

	elm_image_file_set(evas_icon, path, NULL);

	evas_object_size_hint_min_set(evas_icon, width * elm_config_scale_get(),
				      height * elm_config_scale_get());

	elm_object_part_content_set(ly, "elm.swallow.icon", evas_icon);

	evas_object_data_set(ly, "imgicon", evas_icon);
	evas_object_size_hint_min_set(ly, width * elm_config_scale_get(),
				      height * elm_config_scale_get());
	evas_object_hide(ly);
	return ly;
}

static Evas_Object *_txt_icon_add(win_info *win, Indicator_Icon_Object *icon)
{
	struct appdata *ad = NULL;
	char *color_added_str = NULL;
	Evas_Object *evas_icon;
	Evas_Object *ly;
	char *txt = NULL;

	retif(icon == NULL , NULL, "Invalid parameter!");
	retif(icon->ad == NULL || icon->txt_obj.data == NULL,	NULL, "Invalid parameter!");
	retif(win == NULL || win->layout_main == NULL, NULL, "Invalid parameter!");

	ad = icon->ad;
	txt = icon->txt_obj.data;

	ly = elm_layout_add(win->layout_main);
	elm_layout_file_set(ly, ICON_THEME_FILE, "elm/indicator/icon/base");

	evas_icon = elm_label_add(ly);
	color_added_str = (char *)set_label_text_color(txt);

	elm_object_text_set(evas_icon, color_added_str);
	evas_object_size_hint_align_set(evas_icon, 0.5, 0.5);

	elm_object_part_content_set(ly, "elm.swallow.icon", evas_icon);

	evas_object_data_set(ly, "txticon", evas_icon);
	evas_object_hide(ly);

	return ly;
}

char *indicator_util_icon_label_set(const char *buf, char *font_name,
				    char *font_style, int font_size, void *data)
{
	Eina_Strbuf *temp_buf = NULL;
	char *ret_str = NULL;
	char *label_font = ICON_FONT_NAME;
	char *label_font_style = ICON_FONT_STYLE;
	int label_font_size = ICON_FONT_SIZE;
	Eina_Bool buf_result = EINA_FALSE;

	retif(data == NULL || buf == NULL, NULL, "Invalid parameter!");

	temp_buf = eina_strbuf_new();
	if (font_name != NULL)
		label_font = font_name;
	if (font_style != NULL)
		label_font_style = font_style;
	if (font_size > 0)
		label_font_size = font_size;

	buf_result = eina_strbuf_append_printf(temp_buf, CUSTOM_LABEL_STRING,
					       label_font, label_font_style,
					       label_font_size, buf);

	if (buf_result != EINA_FALSE)
		ret_str = eina_strbuf_string_steal(temp_buf);

	eina_strbuf_free(temp_buf);

	return ret_str;
}

Eina_Bool indicator_util_icon_add(win_info *win, Indicator_Icon_Object *icon)
{
	retif(icon == NULL, EINA_FALSE, "Invalid parameter!");

	switch (icon->type) {
	case INDICATOR_TXT_ICON:
		icon->txt_obj.obj = _txt_icon_add(win, icon);
		break;
	case INDICATOR_IMG_ICON:
		icon->img_obj.obj = _img_icon_add(win, icon);
		break;
	case INDICATOR_TXT_WITH_IMG_ICON:
		icon->txt_obj.obj = _txt_icon_add(win, icon);
		icon->img_obj.obj = _img_icon_add(win, icon);
		break;

	default:
		ERR("Icon type check error!");
		return EINA_FALSE;
	}
	_animation_set(icon, icon->ani);
	return EINA_TRUE;
}

Eina_Bool indicator_util_icon_del(Indicator_Icon_Object *icon)
{
	Evas_Object *icon_obj;
	retif(icon == NULL, EINA_FALSE, "Invalid parameter!");

	if (icon->obj_exist != EINA_FALSE) {
		if (icon->txt_obj.obj) {
			icon_obj =
			    evas_object_data_get(icon->txt_obj.obj, "txticon");
			evas_object_del(icon_obj);
			evas_object_del(icon->txt_obj.obj);
			icon->txt_obj.obj = NULL;
		}
		if (icon->img_obj.obj) {
			icon_obj =
			    evas_object_data_get(icon->img_obj.obj, "imgicon");
			evas_object_del(icon_obj);
			evas_object_del(icon->img_obj.obj);
			icon->img_obj.obj = NULL;
		}
	}
	return EINA_TRUE;
}


static int show_other_icon_in_same_priority(Indicator_Icon_Object *icon)
{
	Indicator_Icon_Object *wish_add_icon;
	int area = icon->area;
	retif(icon == NULL, FAIL, "Invalid parameter!");

	wish_add_icon = indicator_get_wish_to_show_icon(icon->win_type, icon->area,icon->priority);
	if (wish_add_icon == NULL)
	{
		DBG("wish_add_icon NULL!");
		return OK;
	}

	if (indicator_util_is_show_icon(wish_add_icon))
	{
		DBG("Wish Icon is alreay shown!");
		return OK;
	}

	if(wish_add_icon!=NULL)
	{
		DBG("%s icon is shown!", wish_add_icon->name);
	}

	if(area ==INDICATOR_ICON_AREA_NOTI)
	{
		icon_box_pack_append(wish_add_icon);
	}
	else
	{
		icon_box_pack(wish_add_icon);
	}

	return OK;
}

static int hide_other_icons_in_view_list(Indicator_Icon_Object *icon)
{
	Indicator_Icon_Object *wish_remove_icon = NULL;
	retif(icon == NULL, FAIL, "Invalid parameter!");

	if (INDICATOR_ICON_AREA_SYSTEM == icon->area || INDICATOR_ICON_AREA_NOTI == icon->area)
	{
		Icon_AddType ret;
		Eina_Error err;

		ret = indicator_is_enable_to_insert_in_non_fixed_list(icon);
		icon->wish_to_show = EINA_TRUE;
		err = indicator_icon_list_update(icon);

		switch (ret) {
		case CAN_ADD_WITH_DEL_NOTI:
			wish_remove_icon = indicator_get_wish_to_remove_icon(icon->win_type, INDICATOR_ICON_AREA_NOTI,0);

			icon_box_unpack(wish_remove_icon);

			retif(wish_remove_icon == NULL, FAIL, "Unexpected Error : CAN_ADD_WITH_DEL_NOTI");
			break;
		case CAN_ADD_WITH_DEL_SYSTEM:
			wish_remove_icon = indicator_get_wish_to_remove_icon(icon->win_type, INDICATOR_ICON_AREA_SYSTEM,0);

			icon_box_unpack(wish_remove_icon);
			retif(wish_remove_icon == NULL, FAIL, "Unexpected Error : CAN_ADD_WITH_DEL_SYSTEM");
			break;
		case CAN_ADD_WITHOUT_DEL:
			break;
		case CANNOT_ADD:
			DBG("[ICON UTIL SYSTEM] %s icon CANNOT_ADD!",icon->name);
			return FAIL;
			break;
		}

		return OK;
	}
	else if (INDICATOR_ICON_AREA_FIXED == icon->area)
	{
		wish_remove_icon = indicator_get_wish_to_remove_icon(icon->win_type, INDICATOR_ICON_AREA_FIXED,icon->priority);

		if (wish_remove_icon == NULL)
		{
			DBG("[ICON UTIL FIXED] NULL!");
			return OK;
		}

		if (wish_remove_icon == icon)
		{
			return FAIL;
		}

		icon->wish_to_show = EINA_TRUE;
		indicator_icon_list_update(icon);

		if (wish_remove_icon->always_top)
		{
			DBG("[ICON UTIL FIXED] %s!", wish_remove_icon->name);
			return FAIL;
		}

		DBG("[ICON UTIL FIXED] %s icon is hidden!",
					wish_remove_icon->name);
		icon_box_unpack(wish_remove_icon);
	}
	return OK;
}

int indicator_util_layout_del(win_info *win)
{
	return icon_box_fini(win);
}

int indicator_util_layout_add(win_info *win)
{
	retif(win == NULL
	      || win->layout_main == NULL, FAIL, "Invalid parameter!");
	indicator_util_layout_del(win);
	return icon_box_init(win);
}


int indicator_util_icon_width_set(Indicator_Icon_Object *icon)
{
	return 0;
}

static int _icon_update(Indicator_Icon_Object *icon)
{
	struct appdata *ad = NULL;
	Evas_Object *txt_eo, *img_eo;
	char buf[PATH_MAX];

	retif(icon == NULL || icon->ad == NULL, FAIL, "Invalid parameter!");
	ad = icon->ad;

	memset(buf, 0x00, sizeof(buf));

	if (icon->type == INDICATOR_IMG_ICON
	    || icon->type == INDICATOR_TXT_WITH_IMG_ICON) {
		img_eo = evas_object_data_get(icon->img_obj.obj, "imgicon");

		if (strncmp(icon->img_obj.data, "/", 1) != 0) {
			snprintf(buf, sizeof(buf), "%s/%s", get_icon_dir(),
				 icon->img_obj.data);
			elm_image_file_set(img_eo, buf, NULL);
		} else {
			retif(icon->img_obj.data[0] == '\0', FAIL,
			      "Invalid parameter!");
			elm_image_file_set(img_eo, icon->img_obj.data, NULL);
		}

		if (icon->img_obj.width >= 0 && icon->img_obj.height>=0) {
			evas_object_size_hint_min_set(img_eo,
				icon->img_obj.width * elm_config_scale_get(),
				icon->img_obj.height * elm_config_scale_get());
		} else {
			evas_object_size_hint_min_set(img_eo,
				DEFAULT_ICON_WIDTH * elm_config_scale_get(),
				DEFAULT_ICON_HEIGHT * elm_config_scale_get());
	}
	}

	if (icon->type == INDICATOR_TXT_ICON
	    || icon->type == INDICATOR_TXT_WITH_IMG_ICON) {
		char *color_added_str = NULL;
		txt_eo = evas_object_data_get(icon->txt_obj.obj, "txticon");
		color_added_str =
		    (char *)set_label_text_color(icon->txt_obj.data);
		elm_object_text_set(txt_eo, color_added_str);
		free(color_added_str);
	}
	return OK;
}

void indicator_util_icon_show(Indicator_Icon_Object *icon)
{
	struct appdata *ad = (struct appdata *)icon->ad;

	retif(icon == NULL, , "Invalid parameter!");

	if (icon->obj_exist != EINA_FALSE)
	{
		_icon_update(icon);
	}

	if (hide_other_icons_in_view_list(icon) == FAIL)
	{
		return;
	}

	icon->wish_to_show = EINA_TRUE;
	DBG("[ICON UTIL] %s %d icon is shown!", icon->name, icon->win_type);

	icon_box_pack(icon);

	indicator_util_update_display(&(ad->win[icon->win_type]));
}

void indicator_util_icon_hide(Indicator_Icon_Object *icon)
{
	int ret;

	retif(icon == NULL, , "Invalid parameter!");

	icon->wish_to_show = EINA_FALSE;

	DBG("%s icon is hidden!", icon->name);

	if (icon->exist_in_view == EINA_TRUE) {
		ret = icon_box_unpack(icon);

		if (ret == FAIL)
			ERR("Failed to unpack %s!", icon->name);

		show_other_icon_in_same_priority(icon);
	}

	struct appdata *ad = (struct appdata *)icon->ad;

	indicator_util_update_display(&(ad->win[icon->win_type]));
}

void indicator_util_event_count_set(int count, void *data)
{
	static int _cnt = -1;
	char buf[1024];

	retif(data == NULL, , "Cannot get layout!");

	if (_cnt != count) {
		memset(buf, 0x00, sizeof(buf));
		if (count) {
			snprintf(buf, sizeof(buf), "%d", count);
			indicator_signal_emit(data,"badge,show,1","elm.image.badge");
		} else {
			indicator_signal_emit(data,"badge,hide,1","elm.image.badge");
		}

		indicator_part_text_emit(data,"elm.text.badge", buf);
		_cnt = count;
	}
}

unsigned int indicator_util_max_visible_event_count(int type)
{
	return indicator_get_max_count_in_non_fixed_list(type);
}

unsigned int indicator_util_get_update_flag(void)
{
	return update_icon_flag;
}

void indicator_util_set_update_flag(unsigned int val)
{
	INFO("SET UPDATE FLAG %d",val);
	update_icon_flag = val;
}
