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

#include <Eina.h>
#include <device/display.h>

#include "common.h"
#include "box.h"
#include "icon.h"
#include "list.h"
#include "main.h"
#include "indicator_gui.h"
#include "util.h"
#include "toast_popup.h"
#include "log.h"

#define FIXED_BOX_PART_NAME		"elm.swallow.fixed"
#define CONNECTION_SYSTEM_BOX_PART_NAME	"elm.swallow.connection/system"
#define SYSTEM_BOX_PART_NAME	"elm.swallow.system"
#define MINICTRL_BOX_PART_NAME	"elm.swallow.minictrl"
#define NOTI_BOX_PART_NAME	"elm.swallow.noti"
#define DIGIT_BOX_PART_NAME "percentage.digit.box"

#define CORRECTION 10
#define MORE_NOTI "more_notify"

Eina_List *_view_fixed_list;
Eina_List *_view_system_list;
Eina_List *_view_minictrl_list;
Eina_List *_view_noti_list;
Eina_List *_view_alarm_list;
Eina_List *_view_connection_system_list;


Evas_Object *box_add(Evas_Object *parent)
{
	Evas_Object *obj = NULL;

	retv_if(!parent, NULL);

	obj = elm_box_add(parent);
	retv_if(!obj, NULL);
	evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

	/* Align to left-top */
	elm_box_horizontal_set(obj, EINA_TRUE);
	evas_object_show(obj);

	return obj;
}


Eina_List *box_list_get(indicator_icon_area_type type)
{
	switch (type) {
	case INDICATOR_ICON_AREA_FIXED:
		return _view_fixed_list;
	case INDICATOR_ICON_AREA_SYSTEM:
		return _view_system_list;
	case INDICATOR_ICON_AREA_MINICTRL:
		return _view_minictrl_list;
	case INDICATOR_ICON_AREA_NOTI:
		return _view_noti_list;
	case INDICATOR_ICON_AREA_CONNECTION_SYSTEM:
		return _view_connection_system_list;
	case INDICATOR_ICON_AREA_ALARM:
		return _view_alarm_list;
	default:
		return NULL;
	}
}


Eina_List **box_list_ptr_get(indicator_icon_area_type type)
{
	switch (type) {
	case INDICATOR_ICON_AREA_FIXED:
		return &_view_fixed_list;
	case INDICATOR_ICON_AREA_SYSTEM:
		return &_view_system_list;
	case INDICATOR_ICON_AREA_MINICTRL:
		return &_view_minictrl_list;
	case INDICATOR_ICON_AREA_NOTI:
		return &_view_noti_list;
	case INDICATOR_ICON_AREA_CONNECTION_SYSTEM:
		return &_view_connection_system_list;
	case INDICATOR_ICON_AREA_ALARM:
		return &_view_alarm_list;
	default:
		return NULL;
	}
}

static void _fixed_box_pack_icon(win_info *win, icon_s *icon)
{
	ret_if(!win);
	ret_if(!icon);

	switch (icon->type) {
		case INDICATOR_IMG_ICON:
			evas_object_show(icon->img_obj.obj);
			elm_box_pack_start(win->_fixed_box[icon->priority], icon->img_obj.obj);
			break;
		case INDICATOR_TXT_ICON:
			break;
		case INDICATOR_TXT_WITH_IMG_ICON:
			break;
		case INDICATOR_DIGIT_ICON:
			switch (icon->digit_area) {
				case DIGIT_UNITY:
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_end(win->_digit_box, icon->img_obj.obj);
					break;
				case DIGIT_DOZENS:
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_end(win->_digit_box, icon->img_obj.obj);
					break;
				case DIGIT_DOZENS_UNITY:
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_end(win->_digit_box, icon->img_obj.obj);
					break;
				case DIGIT_HUNDREDS:
					evas_object_show(icon->img_obj.obj);
					elm_box_pack_end(win->_digit_box, icon->img_obj.obj);
					break;
				default:
					_E("default");
					break;
			}
			break;
		default:
			_E("default");
			break;
	}
#ifdef _SUPPORT_SCREEN_READER
	util_icon_access_register(icon);
#endif
}


static void _box_pack_icon(icon_s *icon, Evas_Object *box)
{
	ret_if(!icon);
	ret_if(!box);

	switch (icon->type) {
	case INDICATOR_IMG_ICON:
		evas_object_show(icon->img_obj.obj);
		elm_box_pack_end(box, icon->img_obj.obj);
		break;
	case INDICATOR_TXT_ICON:
		break;
	case INDICATOR_TXT_WITH_IMG_ICON:
		break;
	case INDICATOR_DIGIT_ICON:
		evas_object_show(icon->img_obj.obj);
		elm_box_pack_end(box, icon->img_obj.obj);
		break;
	default:
		_E("default");
		break;
	}
#ifdef _support_screen_reader
	util_icon_access_register(icon);
#endif

	if (icon->area == INDICATOR_ICON_AREA_NOTI || icon->area == INDICATOR_ICON_AREA_ALARM)
		util_start_noti_ani(icon);
}


static void _create_img_obj(icon_s *icon)
{
	Evas_Object *img_eo = NULL;

	ret_if(!icon);

	img_eo = evas_object_data_get(icon->img_obj.obj, DATA_KEY_IMG_ICON);
	evas_object_size_hint_min_set(img_eo, ELM_SCALE_SIZE(icon->img_obj.width), ELM_SCALE_SIZE(icon->img_obj.height));
	box_handle_animated_gif(icon);
}


static void _update_icon(win_info *win, Eina_List *list)
{
	icon_s *icon;
	Eina_List *l;

	ret_if(!win);
	ret_if(!list);

	EINA_LIST_REVERSE_FOREACH(list, l, icon) {
		if (icon->obj_exist == EINA_FALSE)
			icon_add(win, icon);

		if (icon->obj_exist == EINA_TRUE) {
			if (icon->area == INDICATOR_ICON_AREA_FIXED) {
				_create_img_obj(icon);
				_fixed_box_pack_icon(win, icon);
			} else {
				_create_img_obj(icon);
				_box_pack_icon(icon, indicator_box_object_get(win, icon->area));
			}

		}
	}
}


extern unsigned int box_get_list_size(indicator_icon_area_type type)
{
	int count = 0;

	count = eina_list_count(box_list_get(type));

	return count;
}


static void _update_display(win_info *win)
{
	int i = 0;

	ret_if(!win);

	if (box_get_list_size(INDICATOR_ICON_AREA_SYSTEM)) {
		util_signal_emit(win->data, "indicator.system.show", "indicator.prog");
	} else {
		util_signal_emit(win->data, "indicator.system.hide", "indicator.prog");
	}

	if (box_get_list_size(INDICATOR_ICON_AREA_MINICTRL)) {
		util_signal_emit(win->data, "indicator.minictrl.show", "indicator.prog");
	} else {
		util_signal_emit(win->data, "indicator.minictrl.hide", "indicator.prog");
	}

	if (box_get_list_size(INDICATOR_ICON_AREA_CONNECTION_SYSTEM)) {
		util_signal_emit(win->data, "indicator.connection/system.show", "indicator.prog");
	} else {
		util_signal_emit(win->data, "indicator.connection/system.hide", "indicator.prog");
	}

	if (box_get_list_size(INDICATOR_ICON_AREA_NOTI)) {
		util_signal_emit(win->data, "indicator.noti.show", "indicator.prog");
	} else {
		util_signal_emit(win->data, "indicator.noti.hide", "indicator.prog");
	}


	if (_view_fixed_list) {
		for (i = 0; i < FIXED_COUNT; ++i)
			elm_box_unpack_all(win->_fixed_box[i]);

		_update_icon(win, _view_fixed_list);
	}

	indicator_icon_area_type type;
	for (type = INDICATOR_ICON_AREA_SYSTEM; type < INDICATOR_ICON_AREA_CNT; type++) {
		if (box_list_get(type)) {
			elm_box_unpack_all(indicator_box_object_get(win, type));
			_update_icon(win, box_list_get(type));
		}
	}
}


extern void box_update_display(win_info *win)
{
	ret_if(!win);

	icon_reset_list();
	_update_display(win);
	check_to_show_more_noti(win);
}


extern int box_add_icon_to_list(icon_s *icon)
{
	struct appdata *ad = NULL;

	retv_if(!icon, 0);

	ad = (struct appdata *)icon->ad;

	if (box_exist_icon(icon)) return OK;

	icon->exist_in_view = EINA_TRUE;

	if (INDICATOR_ICON_AREA_FIXED == icon->area)
		_view_fixed_list = eina_list_append(_view_fixed_list, icon);

	else if (icon->area == INDICATOR_ICON_AREA_ALARM)
		_view_alarm_list = eina_list_append(_view_alarm_list, icon);

	else {
		icon_s *data;
		Eina_List *l = NULL;
		Eina_List **list;

		list = box_list_ptr_get(icon->area);

		EINA_LIST_FOREACH(*list, l, data) {
			if (data->priority >= icon->priority) {
				*list = eina_list_prepend_relative_list(*list, icon, l);
				goto __CATCH;
			}
		}
		*list = eina_list_append(*list, icon);
	}

__CATCH:
	if (icon->area == INDICATOR_ICON_AREA_NOTI
			|| icon->area == INDICATOR_ICON_AREA_SYSTEM
			|| icon->area == INDICATOR_ICON_AREA_MINICTRL
			|| icon->area == INDICATOR_ICON_AREA_CONNECTION_SYSTEM) {

		if (ad->opacity_mode == INDICATOR_OPACITY_TRANSPARENT)
			util_send_status_message_start(ad, 2.5);
	}

	return OK;
}


extern int box_append_icon_to_list(icon_s *icon)
{
	retv_if(!icon, 0);

	if (box_exist_icon(icon)) return OK;

	Eina_List **list;
	list = box_list_ptr_get(icon->area);

	icon->exist_in_view = EINA_TRUE;
	*list = eina_list_append(*list, icon);


	return OK;
}


int box_remove_icon_from_list(icon_s *icon)
{
	retv_if(!icon, 0);

	Eina_List **list;
	list = box_list_ptr_get(icon->area);

	icon->exist_in_view = EINA_FALSE;
	*list = eina_list_remove(*list, icon);

#ifdef _SUPPORT_SCREEN_READER
	util_icon_access_unregister(icon);
#endif

	if (icon->obj_exist == EINA_TRUE) {
		if (icon_del(icon) == EINA_TRUE) {
			icon->obj_exist = EINA_FALSE;
		}
	}

	return OK;
}


extern void box_init(win_info *win)
{
	char *str_text = NULL;
	int i = 0;

	ret_if(!win);

	/* Make Fixed Box Object */
	for (i = 0; i < FIXED_COUNT; ++i) {
		if (win->_fixed_box[i] == NULL) {
			win->_fixed_box[i] = box_add(win->layout);
			ret_if(!(win->_fixed_box[i]));

			Eina_Strbuf *temp_str = eina_strbuf_new();
			eina_strbuf_append_printf(temp_str, "%s%d", FIXED_BOX_PART_NAME, i);
			str_text = eina_strbuf_string_steal(temp_str);

			edje_object_part_swallow(elm_layout_edje_get(win->layout), str_text, win->_fixed_box[i]);
			eina_strbuf_free(temp_str);
			free(str_text);
		}
	}
	/* Make Non Fixed Box(CONNECTION/SYSTEM) Object */
	win->_connection_system_box = box_add(win->layout);
	ret_if(!(win->_connection_system_box));

	evas_object_size_hint_align_set(win->_connection_system_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), CONNECTION_SYSTEM_BOX_PART_NAME, win->_connection_system_box);

	/* Make Non Fixed Box(SYSTEM) Object */
	win->_system_box = box_add(win->layout);
	ret_if(!(win->_system_box));

	evas_object_size_hint_align_set(win->_system_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), SYSTEM_BOX_PART_NAME, win->_system_box);

	/* Make Non Fixed Box(MINICTRL) Object */
	win->_minictrl_box = box_add(win->layout);
	ret_if(!(win->_minictrl_box));

	evas_object_size_hint_align_set(win->_minictrl_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), MINICTRL_BOX_PART_NAME, win->_minictrl_box);

	/* Make Non Fixed Box(NOTI) Box Object */
	win->_noti_box = box_add(win->layout);
	ret_if(!(win->_noti_box));

	evas_object_size_hint_align_set(win->_noti_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), NOTI_BOX_PART_NAME, win->_noti_box);

	win->_alarm_box = box_add(win->layout);
	ret_if(!(win->_alarm_box));

	evas_object_size_hint_align_set(win->_alarm_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), "indicator.alarm.icon", win->_alarm_box);

	win->_digit_box = box_add(win->layout);
	ret_if(!(win->_digit_box));

	evas_object_size_hint_align_set(win->_digit_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), DIGIT_BOX_PART_NAME, win->_digit_box);

	return;
}


extern void box_fini(win_info *win)
{
	int i = 0;

	ret_if(!win);
	ret_if(!(win->layout));

	indicator_icon_area_type type;
	for (type = INDICATOR_ICON_AREA_SYSTEM; type < INDICATOR_ICON_AREA_CNT; type++) {
		Evas_Object **obj = indicator_box_object_ptr_get(win, type);
		if (obj && *obj) {
			edje_object_part_unswallow(elm_layout_edje_get(win->layout), *obj);
			elm_box_unpack_all(*obj);
			evas_object_del(*obj);
			*obj = NULL;
		}
	}

	if (win->_digit_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout), win->_digit_box);
		elm_box_unpack_all(win->_digit_box);
		evas_object_del(win->_digit_box);
		win->_digit_box = NULL;
	}

	for (i = 0; i < FIXED_COUNT; ++i) {
		if (win->_fixed_box[i] != NULL) {
			edje_object_part_unswallow(elm_layout_edje_get(win->layout), win->_fixed_box[i]);
			elm_box_unpack_all(win->_fixed_box[i]);
			evas_object_del(win->_fixed_box[i]);
			win->_fixed_box[i] = NULL;
		}
	}

	return;
}

int box_get_max_count_in_non_fixed_list(void)
{
	return PORT_NONFIXED_ICON_COUNT;
}


int box_get_priority_in_move_area(win_info *win, Evas_Coord curr_x, Evas_Coord curr_y)
{
	Evas_Coord x, y, h, w;

	/* Home Area Check for launching home */
	evas_object_geometry_get(win->_fixed_box[INDICATOR_PRIORITY_FIXED1], &x, &y, &h, &w);

	if (curr_x >= x - CORRECTION && curr_x <= x+h + CORRECTION) {
		if (curr_y == -1) {
			return INDICATOR_PRIORITY_FIXED1;
		} else if (curr_y >= y - CORRECTION && curr_y <= y+h + CORRECTION) {
			return INDICATOR_PRIORITY_FIXED1;
		}
	}

	/* Non Fixed Area check for show/hide quickpanel */
	return -1;
}


int box_check_indicator_area(win_info *win, Evas_Coord curr_x, Evas_Coord curr_y)
{
	Evas_Coord x, y, w, h;

	/* Home Area Check for launching home */
	evas_object_geometry_get(win->layout, &x, &y, &w, &h);

	if (curr_x >= x && curr_x < x + w && curr_y >= y && curr_y < y + h) {
		return 1;
	}

	return 0;
}


int box_check_home_icon_area(win_info *win, Evas_Coord curr_x, Evas_Coord curr_y)
{
	Evas_Coord x, y, w, h;

	/* Home Area Check for launching home */
	evas_object_geometry_get(win->_fixed_box[INDICATOR_PRIORITY_FIXED7], &x, &y, &w, &h);

	if (curr_x >= x && curr_x < x + w && curr_y >= y && curr_y < y + h) {
		return 1;
	}

	return 0;
}


int box_check_more_icon_area(win_info *win, Evas_Coord curr_x, Evas_Coord curr_y)
{
	Evas_Coord x, y, w, h;

	/* Home Area Check for launching home */
	evas_object_geometry_get(win->_fixed_box[INDICATOR_PRIORITY_FIXED11], &x, &y, &w, &h);

	if (curr_x >= x && curr_x < x + w && curr_y >= y && curr_y < y + h) {
		return 1;
	}

	return 0;
}


extern Eina_Bool box_exist_icon(icon_s *obj)
{
	retv_if(!obj, ECORE_CALLBACK_CANCEL);

	if (eina_list_data_find(box_list_get(obj->area), obj))
		return EINA_TRUE;
	else
		return EINA_FALSE;
}


int box_handle_animated_gif(icon_s *icon)
{
	display_state_e state;
	Evas_Object *icon_eo = evas_object_data_get(icon->img_obj.obj, DATA_KEY_IMG_ICON);

	retvm_if(icon == NULL, FAIL, "Invalid parameter!");

	if (elm_image_animated_available_get(icon_eo) == EINA_FALSE)
		return FAIL;

	int ret = device_display_get_state(&state);
	if (ret != DEVICE_ERROR_NONE) {
		_E("device_display_get_state failed: %s", get_error_message(ret));
		return FAIL;
	}

	switch (state) {
	case DISPLAY_STATE_SCREEN_OFF:
		elm_image_animated_play_set(icon_eo, EINA_FALSE);
		break;
	case DISPLAY_STATE_NORMAL:
	default:
		elm_image_animated_set(icon_eo, EINA_TRUE);
		if (!elm_image_animated_play_get(icon_eo))
			elm_image_animated_play_set(icon_eo, EINA_TRUE);
		break;
	}

	return OK;
}


void box_noti_ani_handle(int bStart)
{
	icon_s *icon;
	Eina_List *l;

	EINA_LIST_FOREACH(_view_noti_list, l, icon) {
		if (icon->obj_exist == EINA_TRUE) {
			if (bStart == 1)
				util_start_noti_ani(icon);
			else
				util_stop_noti_ani(icon);
		}
	}
}

/* End of file */
