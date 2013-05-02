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


#include <Ecore_X.h>
#include <Eina.h>
#include "common.h"
#include "indicator_box_util.h"
#include "indicator_icon_util.h"
#include "indicator_icon_list.h"
#include "indicator_ui.h"
#include "indicator_gui.h"
#include "indicator_util.h"
#include <vconf.h>

#define DEFAULT_SIZE	(CLOCK_WIDTH + (PADDING_WIDTH * 2))
#define _FIXED_BOX_PART_NAME		"elm.swallow.fixed"
#define _NON_FIXED_BOX_PART_NAME	"elm.swallow.nonfixed"
#define _NOTI_BOX_PART_NAME	"elm.swallow.noti"

#define _FIXED_COUNT	5
#define CORRECTION 10

Eina_List *_view_fixed_list[INDICATOR_WIN_MAX];
Eina_List *_view_system_list[INDICATOR_WIN_MAX];
Eina_List *_view_noti_list[INDICATOR_WIN_MAX];

extern int indicator_icon_show_state[INDICATOR_WIN_MAX];

static Evas_Object *indicator_box_add(Evas_Object * parent)
{
	Evas_Object *obj;

	retif(parent == NULL, NULL, "Invalid parameter!");

	obj = elm_box_add(parent);
	evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_box_horizontal_set(obj, EINA_TRUE);
	evas_object_show(obj);

	return obj;
}

static void _update_window(win_info *win)
{

	int root_w, root_h;
	Ecore_X_Window xwin, root;

	retif(win == NULL, , "Invalid parameter!");

	INFO("_update_window");

	xwin = elm_win_xwindow_get(win->win_main);
	if (!xwin)
		return;
	root = ecore_x_window_root_get(xwin);
	if (!root)
		return;
	ecore_x_window_size_get(root, &root_w, &root_h);

		if (win->angle == 0 || win->angle == 180)
			win->w = root_w;
		else
			win->w = root_h;

	switch (win->angle) {
	case 0:
		ecore_x_window_shape_input_rectangle_set(xwin, root_w - win->w,
							 0, win->w, win->h);
		break;
	case 90:
		ecore_x_window_shape_input_rectangle_set(xwin, 0, 0, win->h,
							 win->w);
		break;
	case 180:
		ecore_x_window_shape_input_rectangle_set(xwin, 0, 0, win->w,
							 win->h);
		break;
	case 270:
		ecore_x_window_shape_input_rectangle_set(xwin,
							 0, root_h - win->w,
							 win->h, win->w);
		break;
	default:
		break;
	}

}

static void _update_display(win_info *win)
{
	Indicator_Icon_Object *icon;
	Eina_List *l;
	int i = 0;

	retif(win == NULL, , "Invalid parameter!");

	for (i = 0; i < _FIXED_COUNT; ++i)
		elm_box_unpack_all(win->_fixed_box[i]);

	elm_box_unpack_all(win->_non_fixed_box);
	elm_box_unpack_all(win->_noti_box);

	DBG("win->type:%d",win->type);

	EINA_LIST_FOREACH(_view_fixed_list[win->type], l, icon) {
		if (icon->obj_exist == EINA_FALSE) {
			if (indicator_util_icon_add(win,icon) == EINA_TRUE)
				icon->obj_exist = EINA_TRUE;
		}

		if (icon->obj_exist == EINA_TRUE) {
			if (icon->area == INDICATOR_ICON_AREA_FIXED
			&& icon->priority < INDICATOR_PRIORITY_FIXED_MAX) {

				Evas_Coord x, y, h, w;

				Evas_Object *img_eo =
					evas_object_data_get(icon->img_obj.obj,
					"imgicon");
				evas_object_size_hint_min_set(img_eo,
					icon->img_obj.width * elm_config_scale_get(),
					icon->img_obj.height * elm_config_scale_get());

				indicator_util_handle_animated_gif(icon);

				switch (icon->type) {
				case INDICATOR_IMG_ICON:
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_start(win->_fixed_box
							   [icon->priority],
							   icon->img_obj.obj);
					break;
				case INDICATOR_TXT_ICON:
					evas_object_show(icon->txt_obj.obj);
					elm_box_pack_start(win->_fixed_box
							   [icon->priority],
							   icon->txt_obj.obj);
					break;
				case INDICATOR_TXT_WITH_IMG_ICON:
					evas_object_show(icon->txt_obj.obj);
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_start(win->_fixed_box
							   [icon->priority],
							   icon->img_obj.obj);
					elm_box_pack_start(win->_fixed_box
							   [icon->priority],
							   icon->txt_obj.obj);
					break;
				}

				evas_object_geometry_get(
						win->_fixed_box[icon->priority],
						&x, &y, &h, &w);

				INFO("Fixed Icon : %s %d %d %d %d",
						icon->name, x, y, h, w);
			}
		}
	}

	EINA_LIST_FOREACH(_view_system_list[win->type], l, icon) {
		if (icon->obj_exist == EINA_FALSE) {
			if (indicator_util_icon_add(win,icon) == EINA_TRUE)
				icon->obj_exist = EINA_TRUE;
		}
		if (icon->obj_exist == EINA_TRUE) {
			if (icon->area == INDICATOR_ICON_AREA_SYSTEM
			&& icon->priority >= INDICATOR_PRIORITY_SYSTEM_MIN
			&& icon->priority <= INDICATOR_PRIORITY_SYSTEM_MAX) {

				Evas_Coord x, y, h, w;

				Evas_Object *img_eo =
					evas_object_data_get(icon->img_obj.obj,
							"imgicon");
				evas_object_size_hint_min_set(img_eo,
					icon->img_obj.width * elm_config_scale_get(),
					icon->img_obj.height * elm_config_scale_get());

				indicator_util_handle_animated_gif(icon);

				switch (icon->type) {
				case INDICATOR_IMG_ICON:
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_end(win->_non_fixed_box,
							   icon->img_obj.obj);
					break;
				case INDICATOR_TXT_ICON:
					evas_object_show(icon->txt_obj.obj);
					elm_box_pack_end(win->_non_fixed_box,
							   icon->txt_obj.obj);
					break;
				case INDICATOR_TXT_WITH_IMG_ICON:
					evas_object_show(icon->txt_obj.obj);
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_end(win->_non_fixed_box,
							   icon->txt_obj.obj);
					elm_box_pack_end(win->_non_fixed_box,
							   icon->img_obj.obj);
					break;
				}
				evas_object_geometry_get(win->_non_fixed_box,
						&x, &y, &h, &w);
				INFO("Non-Fixed Icon : %s %d %d %d %d",
						icon->name, x, y, h, w);
			}


		}
	}

	EINA_LIST_FOREACH(_view_noti_list[win->type], l, icon) {
		if (icon->obj_exist == EINA_FALSE) {
			if (indicator_util_icon_add(win,icon) == EINA_TRUE)
				icon->obj_exist = EINA_TRUE;
		}
		if (icon->obj_exist == EINA_TRUE) {
			if (icon->area == INDICATOR_ICON_AREA_NOTI) {
				Evas_Coord x, y, h, w;

				Evas_Object *img_eo =
					evas_object_data_get(icon->img_obj.obj,
							"imgicon");
				evas_object_size_hint_min_set(img_eo,
					icon->img_obj.width * elm_config_scale_get(),
					icon->img_obj.height * elm_config_scale_get());

				indicator_util_handle_animated_gif(icon);

				switch (icon->type) {
				case INDICATOR_IMG_ICON:
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_end(win->_noti_box,
							   icon->img_obj.obj);
					break;
				case INDICATOR_TXT_ICON:
					evas_object_show(icon->txt_obj.obj);
					elm_box_pack_end(win->_noti_box,
							   icon->txt_obj.obj);
					break;
				case INDICATOR_TXT_WITH_IMG_ICON:
					evas_object_show(icon->txt_obj.obj);
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_end(win->_noti_box,
							   icon->txt_obj.obj);
					elm_box_pack_end(win->_noti_box,
							   icon->img_obj.obj);
					break;
				}
				evas_object_geometry_get(win->_noti_box,
						&x, &y, &h, &w);
				INFO("Non-Fixed Notification Icon : %s %d %d %d %d",
						icon->name, x, y, h, w);
			}


		}
	}

	if (win)
		_update_window(win);
}

void indicator_util_update_display(win_info *win)
{
	retif(win == NULL, , "Invalid parameter!");

	_update_window(win);

	_update_display(win);
}

int icon_box_pack(Indicator_Icon_Object *icon)
{
	retif(icon == NULL, FAIL, "Invalid parameter!");

	if (indicator_util_is_show_icon(icon))
		return OK;

	INFO("[icon_box_pack] %s %d!",icon->name, icon->win_type);

	if (INDICATOR_ICON_AREA_FIXED == icon->area)
	{
		icon->exist_in_view = EINA_TRUE;
		_view_fixed_list[icon->win_type] = eina_list_append(_view_fixed_list[icon->win_type], icon);
	}
	else if(INDICATOR_ICON_AREA_SYSTEM == icon->area)
	{
		Indicator_Icon_Object *data;
		Eina_List *l;

		EINA_LIST_FOREACH(_view_system_list[icon->win_type], l, data) {
			if (data->priority <= icon->priority) {
				icon->exist_in_view = EINA_TRUE;
				_view_system_list[icon->win_type] =
					eina_list_prepend_relative_list(
						_view_system_list[icon->win_type], icon, l);
				DBG("System prepend %s",icon->name);
				return OK;
			}
		}

		icon->exist_in_view = EINA_TRUE;
		_view_system_list[icon->win_type] =
			eina_list_append(_view_system_list[icon->win_type], icon);
		DBG("System append %s",icon->name);
	}
	else
	{
		Indicator_Icon_Object *data;
		Eina_List *l;

		EINA_LIST_FOREACH(_view_noti_list[icon->win_type], l, data) {
			if (data->priority <= icon->priority)
			{
				icon->exist_in_view = EINA_TRUE;
				_view_noti_list[icon->win_type] =
					eina_list_prepend_relative_list(
						_view_noti_list[icon->win_type], icon, l);
				DBG("Noti prepend %s",icon->name);
				return OK;
			}
		}

		icon->exist_in_view = EINA_TRUE;
		_view_noti_list[icon->win_type] =
			eina_list_append(_view_noti_list[icon->win_type], icon);
		DBG("Noti append %s",icon->name);
	}

	return OK;
}

int icon_box_pack_append(Indicator_Icon_Object *icon)
{
	retif(icon == NULL, FAIL, "Invalid parameter!");

	if (indicator_util_is_show_icon(icon))
		return OK;

	INFO("[icon_box_pack_append] %s!",icon->name);

	if (INDICATOR_ICON_AREA_FIXED == icon->area)
	{
		icon->exist_in_view = EINA_TRUE;
		_view_fixed_list[icon->win_type] = eina_list_append(_view_fixed_list[icon->win_type], icon);
	}
	else if(INDICATOR_ICON_AREA_SYSTEM == icon->area)
	{
		icon->exist_in_view = EINA_TRUE;
		_view_system_list[icon->win_type] =
			eina_list_append(_view_system_list[icon->win_type], icon);
	}
	else
	{
		icon->exist_in_view = EINA_TRUE;
		_view_noti_list[icon->win_type] =
			eina_list_append(_view_noti_list[icon->win_type], icon);
		DBG("Noti append %s",icon->name);
	}

	return OK;
}


int icon_box_unpack(Indicator_Icon_Object *icon)
{
	retif(icon == NULL, FAIL, "Invalid parameter!");

	INFO("[icon_box_unpack] %s!",icon->name);

	if (INDICATOR_ICON_AREA_FIXED == icon->area)
	{
		icon->exist_in_view = EINA_FALSE;
		_view_fixed_list[icon->win_type] = eina_list_remove(_view_fixed_list[icon->win_type], icon);
	}
	else if(INDICATOR_ICON_AREA_SYSTEM == icon->area)
	{
		icon->exist_in_view = EINA_FALSE;
		_view_system_list[icon->win_type] =
			eina_list_remove(_view_system_list[icon->win_type], icon);
	}
	else
	{
		icon->exist_in_view = EINA_FALSE;
		_view_noti_list[icon->win_type] =
			eina_list_remove(_view_noti_list[icon->win_type], icon);
	}

	if (icon->obj_exist == EINA_TRUE) {
		if (indicator_util_icon_del(icon) == EINA_TRUE) {
			icon->obj_exist = EINA_FALSE;
			INFO("%s icon object is freed!", icon->name);
		}
	}

	return OK;
}

int icon_box_init(win_info *win)
{
	char *str_text = NULL;
	int i = 0;
	retif(win == NULL, FAIL, "Invalid parameter!");

	for (i = 0; i < _FIXED_COUNT; ++i) {
		if (win->_fixed_box[i] == NULL) {
			Eina_Bool ret;

			win->_fixed_box[i] = indicator_box_add(win->layout_main);
			retif(win->_fixed_box[i] == NULL, FAIL,
				"Failed to add _fixed_box object!");

			Eina_Strbuf *temp_str = eina_strbuf_new();
			eina_strbuf_append_printf(temp_str, "%s%d",
				_FIXED_BOX_PART_NAME, i);
			str_text = eina_strbuf_string_steal(temp_str);

			ret = edje_object_part_swallow(elm_layout_edje_get
						 (win->layout_main), str_text,
						 win->_fixed_box[i]);
			INFO("[ICON INIT] : %d %s %d " , i, str_text, ret);
			eina_strbuf_free(temp_str);
			free(str_text);
		}
	}

	win->_non_fixed_box = indicator_box_add(win->layout_main);
	retif(win->_non_fixed_box == NULL, FAIL,
		"Failed to create _non_fixed_box object!");
	evas_object_size_hint_align_set(win->_non_fixed_box,
			EVAS_HINT_FILL, EVAS_HINT_FILL);

	edje_object_part_swallow(elm_layout_edje_get
				 (win->layout_main), _NON_FIXED_BOX_PART_NAME,
				 win->_non_fixed_box);

	win->_noti_box = indicator_box_add(win->layout_main);
	retif(win->_noti_box == NULL, FAIL,
		"Failed to create _non_fixed_box object!");
	evas_object_size_hint_align_set(win->_noti_box,
			EVAS_HINT_FILL, EVAS_HINT_FILL);

	edje_object_part_swallow(elm_layout_edje_get
				 (win->layout_main), _NOTI_BOX_PART_NAME,
				 win->_noti_box);

	indicator_util_update_display(win);

	return 0;
}

int icon_box_fini(win_info *win)
{
	int i = 0;

	retif(win == NULL || win->layout_main == NULL,
		FAIL, "Invalid parameter!");

	if (win->_non_fixed_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout_main),
					   win->_non_fixed_box);
		elm_box_unpack_all(win->_non_fixed_box);
		evas_object_del(win->_non_fixed_box);
		win->_non_fixed_box = NULL;
	}

	if (win->_noti_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout_main),
					   win->_noti_box);
		elm_box_unpack_all(win->_noti_box);
		evas_object_del(win->_noti_box);
		win->_noti_box = NULL;
	}

	for (i = 0; i < _FIXED_COUNT; ++i) {
		if (win->_fixed_box[i] != NULL) {
			edje_object_part_unswallow(elm_layout_edje_get
						   (win->layout_main),
						   win->_fixed_box[i]);
			elm_box_unpack_all(win->_fixed_box[i]);
			evas_object_del(win->_fixed_box[i]);
			win->_fixed_box[i] = NULL;
		}
	}
	return 0;
}

unsigned int indicator_get_count_in_fixed_list(int type)
{
	int r = eina_list_count(_view_fixed_list[type]);
	DBG("Fixed Count : %d",r);
	return r;
}
unsigned int indicator_get_count_in_system_list(int type)
{
	int r = eina_list_count(_view_system_list[type]);
	DBG("System Count : %d",r);
	return r;
}
unsigned int indicator_get_count_in_noti_list(int type)
{
	int r = eina_list_count(_view_noti_list[type]);
	DBG("Notification Count : %d",r);
	return r;
}

void indicator_set_count_in_non_fixed_list(int angle, int status)
{

}

int indicator_get_noti_list_index_count(int type)
{
	int added_count = 0;
	int ret = 0;
	int status = 0;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, &status);

	if (ret != OK)
	{
		ERR("Fail to get vconfkey : %s",
			VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL);
	}

	if (status == EINA_TRUE)
		added_count = BATTERY_TEXT_ON_COUNT;
	else
		added_count = BATTERY_TEXT_OFF_COUNT;

	if(type == INDICATOR_WIN_PORT)
	{
		ret = PORT_NOTI_ICON_COUNT + added_count;
	}
	else
	{
		ret = LAND_NOTI_ICON_COUNT + added_count;
	}

	return ret;
}

int indicator_get_system_list_index_count(int type)
{
	int ret = 0;

	if(type == INDICATOR_WIN_PORT)
	{
		ret = PORT_SYSTEM_ICON_COUNT;
	}
	else
	{
		ret = LAND_SYSTEM_ICON_COUNT;
	}

	return ret;
}

int indicator_get_max_count_in_non_fixed_list(int type)
{
	int added_count = 0;
	int ret = 0;
	int status = 0;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, &status);

	if (ret != OK)
	{
		ERR("Fail to get vconfkey : %s",
			VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL);
	}

	if (status == EINA_TRUE)
		added_count = BATTERY_TEXT_ON_COUNT;
	else
		added_count = BATTERY_TEXT_OFF_COUNT;

	if(type == INDICATOR_WIN_PORT)
	{
		ret = PORT_NONFIXED_ICON_COUNT + added_count;
	}
	else
	{
		ret = LAND_NONFIXED_ICON_COUNT + added_count;
	}

	return ret;
}

Icon_AddType indicator_is_enable_to_insert_in_non_fixed_list(Indicator_Icon_Object *obj)
{
	Indicator_Icon_Object *icon;
	Eina_List *l;

	int higher_cnt = 0;
	int same_cnt = 0;
	int same_top_cnt = 0;
	int lower_cnt = 0;
	int system_cnt = indicator_get_count_in_system_list(obj->win_type);
	int noti_cnt = indicator_get_count_in_noti_list(obj->win_type);
	int total_cnt = system_cnt + noti_cnt;
	Eina_List * tmpList = NULL;

	if(obj->area == INDICATOR_ICON_AREA_SYSTEM )
	{
		tmpList = _view_system_list[obj->win_type];
	}
	else if(obj->area == INDICATOR_ICON_AREA_NOTI)
	{
		tmpList = _view_noti_list[obj->win_type];
	}

	EINA_LIST_FOREACH(tmpList, l, icon) {
		if (!strcmp(icon->name, obj->name))
			return CANNOT_ADD;

		if (icon->priority > obj->priority)
			++higher_cnt;

		else if (icon->priority == obj->priority) {
			++same_cnt;
			if (icon->always_top == EINA_TRUE)
				++same_top_cnt;
		} else
			lower_cnt++;
	}

	INFO("[INSERT ENABLE] : %d %d %d %d %d %d",
			higher_cnt, lower_cnt, same_cnt, same_top_cnt,
			indicator_get_max_count_in_non_fixed_list(obj->win_type), system_cnt);
	INFO("[INSERT ENABLE2] : %d %d %d",
			obj->area, system_cnt, noti_cnt);

	if(obj->area == INDICATOR_ICON_AREA_SYSTEM )
	{
		if (higher_cnt + same_cnt + lower_cnt >= indicator_get_system_list_index_count(obj->win_type))
		{
			if (obj->always_top == EINA_TRUE)
			{
				if(same_top_cnt>=indicator_get_system_list_index_count(obj->win_type))
				{
					DBG("[CANNOT_ADD] %d",same_top_cnt);
					return CANNOT_ADD;
				}
				else
				{
					DBG("[CAN_ADD_WITH_DEL_SYSTEM]");
					return CAN_ADD_WITH_DEL_SYSTEM;
				}
			}
			else
			{
				if(higher_cnt >= indicator_get_system_list_index_count(obj->win_type))
				{
					DBG("[CANNOT_ADD]");
					return CANNOT_ADD;
				}
				else if(higher_cnt+same_cnt >= indicator_get_system_list_index_count(obj->win_type))
				{
					DBG("[CAN_ADD_WITH_DEL_SYSTEM]");
					return CAN_ADD_WITH_DEL_SYSTEM;
				}
				else
				{
					DBG("[CAN_ADD_WITH_DEL_SYSTEM]");
					return CAN_ADD_WITH_DEL_SYSTEM;
				}
			}
		}
		else
		{
			return CAN_ADD_WITHOUT_DEL;
		}
	}
	else
	{
		if(noti_cnt>=indicator_get_noti_list_index_count(obj->win_type))
		{

			DBG("[CAN_ADD_WITH_DEL_NOTI]");
			return CAN_ADD_WITH_DEL_NOTI;
		}
		else
		{
			INFO("[CAN_ADD_WITHOUT_DEL]");
			return CAN_ADD_WITHOUT_DEL;
		}
	}

	return CANNOT_ADD;
}

int indicator_util_get_priority_in_move_area(win_info *win, Evas_Coord curr_x,
					Evas_Coord curr_y)
{
	Evas_Coord x, y, h, w;

	evas_object_geometry_get(win->_fixed_box[INDICATOR_PRIORITY_FIXED1],
			&x, &y, &h, &w);
	INFO("[Current Location] %d %d %d %d %d %d",
			x, y, h, w, curr_x, curr_y);

	if (curr_x >= x - CORRECTION && curr_x <= x+h + CORRECTION) {
		if (curr_y == -1)
			return INDICATOR_PRIORITY_FIXED1;
		else if (curr_y >= y - CORRECTION && curr_y <= y+h + CORRECTION)
			return INDICATOR_PRIORITY_FIXED1;
	}


	return -1;
}

int indicator_util_check_indicator_area(win_info *win, Evas_Coord curr_x, Evas_Coord curr_y)
{
	Evas_Coord x, y, w, h;

	evas_object_geometry_get(win->layout_main,
			&x, &y, &w, &h);

	INFO("[indicator area] [%d, %d] [wxh][%dx%d], cur[%d, %d]",
			x, y, w, h, curr_x, curr_y);

	if (curr_x >= x && curr_x < x + w && curr_y >= y && curr_y < y + h)
		return 1;

	return 0;
}

int indicator_util_check_home_icon_area(win_info *win, Evas_Coord curr_x, Evas_Coord curr_y)
{
	Evas_Coord x, y, w, h;

	evas_object_geometry_get(win->_fixed_box[INDICATOR_PRIORITY_FIXED5],
			&x, &y, &w, &h);

	INFO("[Home icon area] [%d, %d] [wxh][%dx%d], cur[%d, %d]",
			x, y, w, h, curr_x, curr_y);

	if (curr_x >= x && curr_x < x + w && curr_y >= y && curr_y < y + h)
		return 1;

	return 0;
}

static void indicator_util_icon_state(int win_type, int bShow)
{
	DBG("win_type(%d) Show(%d)",win_type,bShow);
	indicator_icon_show_state[win_type] = bShow;
}

void indicator_util_show_hide_icons(void* data,int bShow, int bEffect)
{
	win_info *win = (win_info *)data;
	retif(data == NULL, , "Invalid parameter!");

	if(bShow)
	{
		indicator_util_icon_state(win->type,1);
	}
	else
	{
		indicator_util_icon_state(win->type,0);
	}

	if(bEffect)
	{
		if(bShow)
		{
			indicator_signal_emit_by_win(data,"indicator.clip.show", "indicator.prog");
			indicator_signal_emit_by_win(data,"indicator.noti.show", "indicator.prog");
		}
		else
		{
			indicator_signal_emit_by_win(data,"indicator.clip.hide", "indicator.prog");
			indicator_signal_emit_by_win(data,"indicator.noti.hide", "indicator.prog");
		}
	}
	else
	{
		if(bShow)
		{
			indicator_signal_emit_by_win(data,"indicator.clip.show.noeffect", "indicator.prog");
			indicator_signal_emit_by_win(data,"indicator.noti.show.noeffect", "indicator.prog");
		}
		else
		{
			indicator_signal_emit_by_win(data,"indicator.clip.hide.noeffect", "indicator.prog");
			indicator_signal_emit_by_win(data,"indicator.noti.hide.noeffect", "indicator.prog");
		}
	}
}

Eina_Bool indicator_util_is_show_icon(Indicator_Icon_Object *obj)
{
	retif(obj == NULL, FAIL, "Invalid parameter!");

	if (obj->area == INDICATOR_ICON_AREA_FIXED)
	{
		if (eina_list_data_find(_view_fixed_list[obj->win_type], obj))
			return 1;
		else
			return 0;
	}
	else if(obj->area == INDICATOR_ICON_AREA_SYSTEM)
	{
		if (eina_list_data_find(_view_system_list[obj->win_type], obj))
			return 1;
		else
			return 0;
	}

	else {
		if (eina_list_data_find(_view_noti_list[obj->win_type], obj))
			return 1;
		else
			return 0;
	}
}
int indicator_util_handle_animated_gif(Indicator_Icon_Object *icon)
{
	int bPlay = TRUE;
	int val = 0;

	retif(icon == NULL, FAIL, "Invalid parameter!");
	Evas_Object *icon_eo = evas_object_data_get(icon->img_obj.obj, "imgicon");

	if(elm_image_animated_available_get(icon_eo)== EINA_FALSE)
	{
		return FALSE;
	}

	if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		ERR("Cannot get VCONFKEY_PM_STATE");
		return FALSE;
	}

	switch(val)
	{
		case VCONFKEY_PM_STATE_LCDOFF :
			bPlay = FALSE;
			break;
		case VCONFKEY_PM_STATE_NORMAL :
			bPlay = TRUE;
			break;
		default:
			bPlay = TRUE;
			break;
	}

	if(bPlay == TRUE)
	{
		if(elm_image_animated_get(icon_eo)==EINA_FALSE)
		{
			elm_image_animated_set(icon_eo,EINA_TRUE);
		}

		if(elm_image_animated_play_get(icon_eo) == EINA_FALSE)
		{
			elm_image_animated_play_set(icon_eo, EINA_TRUE);
			INFO("PLAY ANIMATED GIF ICON(%s)",icon->name);
		}
	}
	else
	{
		if(elm_image_animated_play_get(icon_eo) == EINA_TRUE)
		{
			elm_image_animated_play_set(icon_eo, EINA_FALSE);
			INFO("STOP ANIMATED GIF ICON(%s)",icon->name);
		}
	}

	return TRUE;
}

