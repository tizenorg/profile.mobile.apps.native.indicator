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

//#include <Ecore_X.h>
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

/* FIXME */
#if 0
win_info *_win;
#endif
static int icon_show_state = 0;
int previous_noti_count = 0;



static Evas_Object *_box_add(Evas_Object * parent)
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


#if 0
static void _update_window(win_info *win)
{
	int root_w, root_h;
	Ecore_X_Window xwin, root;

	retm_if(win == NULL, "Invalid parameter!");

	xwin = elm_win_xwindow_get(win->win);
	if (!xwin) return;
	root = ecore_x_window_root_get(xwin);
	if (!root) return;
	ecore_x_window_size_get(root, &root_w, &root_h);

	if (win->angle == 0 || win->angle == 180) win->w = root_w;
	else win->w = root_h;

	switch (win->angle) {
	case 0:
		ecore_x_window_shape_input_rectangle_set(xwin, root_w - win->w, 0, win->w, win->h);
		break;
	case 90:
		ecore_x_window_shape_input_rectangle_set(xwin, 0, 0, win->h, win->w);
		break;
	case 180:
		ecore_x_window_shape_input_rectangle_set(xwin, 0, 0, win->w, win->h);
		break;
	case 270:
		ecore_x_window_shape_input_rectangle_set(xwin, 0, root_h - win->w, win->h, win->w);
		break;
	default:
		break;
	}
}
#endif

#if 0
void box_delete_noti_icon_all(win_info *win)
{
	icon_s *icon;
	Eina_List *l;

	elm_box_unpack_all(win->_noti_box);

	EINA_LIST_FOREACH(_view_noti_list, l, icon) {
		if (icon->obj_exist == EINA_TRUE) {
			if (icon_del(icon) == EINA_TRUE) {
				icon->obj_exist = EINA_FALSE;
			}
		}
	}
}
#endif


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
			switch(icon->digit_area) {
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
	if (icon->area == INDICATOR_ICON_AREA_NOTI || icon->area == INDICATOR_ICON_AREA_ALARM) {
		util_start_noti_ani(icon);
	}
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

	EINA_LIST_FOREACH(list, l, icon) {
		if (icon->obj_exist == EINA_FALSE) {
			icon_add(win, icon);
		}

		if (icon->obj_exist == EINA_TRUE) {
			if (icon->area == INDICATOR_ICON_AREA_FIXED
			&& icon->priority <= INDICATOR_PRIORITY_FIXED_MAX) {
				_create_img_obj(icon);
				_fixed_box_pack_icon(win, icon);
			}
			if (icon->area == INDICATOR_ICON_AREA_SYSTEM
			&& icon->priority >= INDICATOR_PRIORITY_SYSTEM_MIN
			&& icon->priority <= INDICATOR_PRIORITY_SYSTEM_MAX) {
				_create_img_obj(icon);
				_box_pack_icon(icon, win->_non_fixed_box);
			}
			if (icon->area == INDICATOR_ICON_AREA_MINICTRL
			&& icon->priority >= INDICATOR_PRIORITY_MINICTRL_MIN
			&& icon->priority <= INDICATOR_PRIORITY_MINICTRL_MAX) {
				_create_img_obj(icon);
				_box_pack_icon(icon, win->_minictrl_box);
			}
			if (icon->area == INDICATOR_ICON_AREA_CONNECTION_SYSTEM
			&& icon->priority >= INDICATOR_PRIORITY_CONNECTION_SYSTEM_MIN
			&& icon->priority <= INDICATOR_PRIORITY_CONNECTION_SYSTEM_MAX) {
				_create_img_obj(icon);
				_box_pack_icon(icon, win->_connection_system_box);
			}
			if (icon->area == INDICATOR_ICON_AREA_NOTI) {
				_create_img_obj(icon);
				_box_pack_icon(icon, win->_noti_box);
			}
			if (icon->area == INDICATOR_ICON_AREA_ALARM) {
				_create_img_obj(icon);
				_box_pack_icon(icon, win->_alarm_box);
			}
		}
	}
}



extern unsigned int box_get_list_size(Box_List list)
{
	int count = 0;

	switch (list) {
	case FIXED_LIST:
		count = eina_list_count(_view_fixed_list);
		break;
	case SYSTEM_LIST:
		count = eina_list_count(_view_system_list);
		break;
	case MINICTRL_LIST:
		count = eina_list_count(_view_minictrl_list);
		break;
	case CONNECTION_SYSTEM_LIST:
		count = eina_list_count(_view_connection_system_list);
		break;
	case NOTI_LIST:
		count = eina_list_count(_view_noti_list);
		break;
	default:
		_D("List dose not exist");
		break;
	}

	return count;
}



static void _update_display(win_info *win)
{
	int i = 0;

	ret_if(!win);

	if (box_get_list_size(SYSTEM_LIST)) {
		util_signal_emit(win->data, "indicator.system.show", "indicator.prog");
	} else {
		util_signal_emit(win->data, "indicator.system.hide", "indicator.prog");
	}

	if (box_get_list_size(MINICTRL_LIST)) {
		util_signal_emit(win->data, "indicator.minictrl.show", "indicator.prog");
	} else {
		util_signal_emit(win->data, "indicator.minictrl.hide", "indicator.prog");
	}

	if (box_get_list_size(CONNECTION_SYSTEM_LIST)) {
		util_signal_emit(win->data, "indicator.connection/system.show", "indicator.prog");
	} else {
		util_signal_emit(win->data, "indicator.connection/system.hide", "indicator.prog");
	}

	if (box_get_list_size(NOTI_LIST)) {
		util_signal_emit(win->data, "indicator.noti.show", "indicator.prog");
	} else {
		util_signal_emit(win->data, "indicator.noti.hide", "indicator.prog");
	}

	for (i = 0; i < FIXED_COUNT; ++i) {
		elm_box_unpack_all(win->_fixed_box[i]);
	}

	elm_box_unpack_all(win->_non_fixed_box);
	elm_box_unpack_all(win->_minictrl_box);
	elm_box_unpack_all(win->_connection_system_box);
	elm_box_unpack_all(win->_noti_box);
	elm_box_unpack_all(win->_alarm_box);
	elm_box_unpack_all(win->_digit_box);

	_update_icon(win, _view_fixed_list);
	_update_icon(win, _view_connection_system_list);
	_update_icon(win, _view_system_list);
	_update_icon(win, _view_minictrl_list);
	_update_icon(win, _view_noti_list);
	_update_icon(win, _view_alarm_list);


#if 0
	if (win) _update_window(win);
#endif
}


extern void box_update_display(win_info *win)
{
	ret_if(!win);

#if 0
	_update_window(win);
#endif
	icon_reset_list();
	Eina_Bool overflow = check_for_icons_overflow();

	_update_display(win);

	check_to_show_more_noti(win, overflow);

}


extern int box_add_icon_to_list(icon_s *icon)
{
	struct appdata *ad = NULL;
	int noti_count = 0;

	retv_if(!icon, 0);

	ad = (struct appdata*)icon->ad;

	if (box_exist_icon(icon)) return OK;

	if (INDICATOR_ICON_AREA_FIXED == icon->area) {
		icon->exist_in_view = EINA_TRUE;
		_view_fixed_list = eina_list_append(_view_fixed_list, icon);
	} else if(INDICATOR_ICON_AREA_SYSTEM == icon->area) {
		icon_s *data;
		Eina_List *l = NULL;

		EINA_LIST_FOREACH(_view_system_list, l, data) {
			if (data->priority <= icon->priority) {
				icon->exist_in_view = EINA_TRUE;
				_view_system_list = eina_list_prepend_relative_list(_view_system_list, icon, l);
				_D("System (eina_list_append_relative_list) %s",icon->name);
				goto __CATCH;
			}
		}
		/* if finding condition is failed, append it at tail */
		icon->exist_in_view = EINA_TRUE;
		_view_system_list = eina_list_append(_view_system_list, icon);
		_D("System prepend (Priority low) : %s",icon->name);

	} else if(INDICATOR_ICON_AREA_MINICTRL == icon->area) {
		_D("Pack to MINICTRL list : %s", icon->name);
		icon_s *data;
		Eina_List *l;

		EINA_LIST_FOREACH(_view_minictrl_list, l, data) {
			if (data->priority <= icon->priority) {
				icon->exist_in_view = EINA_TRUE;
				_view_minictrl_list = eina_list_prepend_relative_list(_view_minictrl_list, icon, l);
				goto __CATCH;
			}
		}
		/* if finding condition is failed, append it at tail */
		icon->exist_in_view = EINA_TRUE;
		_view_minictrl_list = eina_list_append(_view_minictrl_list, icon);

	} else if(INDICATOR_ICON_AREA_NOTI == icon->area) {
		if(strncmp(icon->name, MORE_NOTI, strlen(MORE_NOTI))==0)
		{
			icon->exist_in_view = EINA_TRUE;
			_view_noti_list = eina_list_prepend(_view_noti_list, icon);
			goto __CATCH;
		}


		/* if finding condition is failed, append it at tail */
		icon->exist_in_view = EINA_TRUE;
		_view_noti_list = eina_list_append(_view_noti_list, icon);

	} else if(INDICATOR_ICON_AREA_CONNECTION_SYSTEM == icon->area) {
		_D("Pack to Connection/System list : %s", icon->name);
		icon_s *data;
		Eina_List *l;

		EINA_LIST_FOREACH(_view_connection_system_list, l, data) {
			if (data->priority <= icon->priority) {
				icon->exist_in_view = EINA_TRUE;
				_view_connection_system_list = eina_list_append_relative_list(_view_connection_system_list, icon, l);
				goto __CATCH;
			}
		}

		/* if finding condition is failed, append it at tail */
		icon->exist_in_view = EINA_TRUE;
		_view_connection_system_list = eina_list_append(_view_connection_system_list, icon);

	} else {
		icon->exist_in_view = EINA_TRUE;
		_view_alarm_list = eina_list_append(_view_alarm_list, icon);
	}

__CATCH:
	previous_noti_count = noti_count;
	if (icon->area == INDICATOR_ICON_AREA_NOTI
			|| icon->area == INDICATOR_ICON_AREA_SYSTEM
			|| icon->area == INDICATOR_ICON_AREA_MINICTRL
			|| icon->area == INDICATOR_ICON_AREA_CONNECTION_SYSTEM) {
		int bDisplay = 0;
		bDisplay = 1;

		if (ad->opacity_mode == INDICATOR_OPACITY_TRANSPARENT
				&& bDisplay == 1) {
			util_send_status_message_start(ad,2.5);
		}
	}

	return OK;
}



extern int box_append_icon_to_list(icon_s *icon)
{
	Eina_List *l;
	icon_s *data;

	retv_if(!icon, 0);

	if (box_exist_icon(icon)) return OK;

	switch (icon->area) {
	case INDICATOR_ICON_AREA_FIXED:
		icon->exist_in_view = EINA_TRUE;
		_view_fixed_list = eina_list_append(_view_fixed_list, icon);
		break;
	case INDICATOR_ICON_AREA_SYSTEM:
		icon->exist_in_view = EINA_TRUE;
		_view_system_list = eina_list_append(_view_system_list, icon);
		break;
	case INDICATOR_ICON_AREA_MINICTRL:
		icon->exist_in_view = EINA_TRUE;
		_view_minictrl_list = eina_list_append(_view_minictrl_list, icon);
		break;
	case INDICATOR_ICON_AREA_CONNECTION_SYSTEM:
		icon->exist_in_view = EINA_TRUE;
		_view_connection_system_list = eina_list_append(_view_connection_system_list, icon);
		break;
	case INDICATOR_ICON_AREA_NOTI:
		EINA_LIST_FOREACH(_view_noti_list, l, data) {
			if (strncmp(data->name, MORE_NOTI, strlen(MORE_NOTI)) == 0) {
				icon->exist_in_view = EINA_TRUE;
				_view_noti_list = eina_list_append_relative_list(_view_noti_list, icon, l);
				return OK;
			}
		}
		icon->exist_in_view = EINA_TRUE;
		_view_noti_list = eina_list_append(_view_noti_list, icon);
		break;
	default:
		_D("Icon area does not exists");
		break;
	}

	return OK;
}



int box_remove_icon_from_list(icon_s *icon)
{
	retv_if(!icon, 0);

	switch (icon->area) {
	case INDICATOR_ICON_AREA_FIXED:
		icon->exist_in_view = EINA_FALSE;
		_view_fixed_list = eina_list_remove(_view_fixed_list, icon);
		break;
	case INDICATOR_ICON_AREA_SYSTEM:
		icon->exist_in_view = EINA_FALSE;
		_view_system_list = eina_list_remove(_view_system_list, icon);
		break;
	case INDICATOR_ICON_AREA_MINICTRL:
		icon->exist_in_view = EINA_FALSE;
		_view_minictrl_list = eina_list_remove(_view_minictrl_list, icon);
		break;
	case INDICATOR_ICON_AREA_NOTI:
		icon->exist_in_view = EINA_FALSE;
		_view_noti_list = eina_list_remove(_view_noti_list, icon);
		break;
	case INDICATOR_ICON_AREA_CONNECTION_SYSTEM:
		icon->exist_in_view = EINA_FALSE;
		_view_connection_system_list = eina_list_remove(_view_connection_system_list, icon);
	break;
	case INDICATOR_ICON_AREA_ALARM:
		icon->exist_in_view = EINA_FALSE;
		_view_alarm_list = eina_list_remove(_view_alarm_list, icon);
		break;
	default:
		_D("icon area dose not exists");
		break;
	}

#if 0
	int noti_count = 0;

	noti_count = box_get_list_size(NOTI_LIST);
	if (noti_count > 0) {
		util_signal_emit(_win->data, "indicator.noti.show", "indicator.prog");
	} else {
		_D("Need to stop blink animation and hide icon");
		util_signal_emit_by_win(_win->data,"indicator.noti.hide", "indicator.prog");
	}
#endif
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
			win->_fixed_box[i] = _box_add(win->layout);
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
	win->_connection_system_box = _box_add(win->layout);
	ret_if(!(win->_connection_system_box));

	evas_object_size_hint_align_set(win->_connection_system_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), CONNECTION_SYSTEM_BOX_PART_NAME, win->_connection_system_box);

	/* Make Non Fixed Box(SYSTEM) Object */
	win->_non_fixed_box = _box_add(win->layout);
	ret_if(!(win->_non_fixed_box));

	evas_object_size_hint_align_set(win->_non_fixed_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), SYSTEM_BOX_PART_NAME, win->_non_fixed_box);

	/* Make Non Fixed Box(MINICTRL) Object */
	win->_minictrl_box = _box_add(win->layout);
	ret_if(!(win->_minictrl_box));

	evas_object_size_hint_align_set(win->_minictrl_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), MINICTRL_BOX_PART_NAME, win->_minictrl_box);

	/* Make Non Fixed Box(NOTI) Box Object */
	win->_noti_box = _box_add(win->layout);
	ret_if(!(win->_noti_box));

	evas_object_size_hint_align_set(win->_noti_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), NOTI_BOX_PART_NAME, win->_noti_box);

	win->_alarm_box = _box_add(win->layout);
	ret_if(!(win->_alarm_box));

	evas_object_size_hint_align_set(win->_alarm_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(win->layout), "indicator.alarm.icon", win->_alarm_box);

	win->_digit_box = _box_add(win->layout);
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

	if (win->_digit_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout), win->_digit_box);
		elm_box_unpack_all(win->_digit_box);
		evas_object_del(win->_digit_box);
		win->_digit_box = NULL;
	}

	if (win->_non_fixed_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout), win->_non_fixed_box);
		elm_box_unpack_all(win->_non_fixed_box);
		evas_object_del(win->_non_fixed_box);
		win->_non_fixed_box = NULL;
	}

	if (win->_connection_system_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout), win->_connection_system_box);
		elm_box_unpack_all(win->_connection_system_box);
		evas_object_del(win->_connection_system_box);
		win->_connection_system_box = NULL;
	}

	if (win->_minictrl_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout), win->_minictrl_box);
		elm_box_unpack_all(win->_minictrl_box);
		evas_object_del(win->_minictrl_box);
		win->_minictrl_box = NULL;
	}

	if (win->_noti_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout), win->_noti_box);
		elm_box_unpack_all(win->_noti_box);
		evas_object_del(win->_noti_box);
		win->_noti_box = NULL;
	}

	if (win->_alarm_box != NULL) {
		edje_object_part_unswallow(elm_layout_edje_get(win->layout), win->_alarm_box);
		elm_box_unpack_all(win->_alarm_box);
		evas_object_del(win->_alarm_box);
		win->_alarm_box = NULL;
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


int box_get_enabled_noti_count(void)
{
	int enabled_noti_cnt = 0;

	int system_cnt = box_get_list_size(SYSTEM_LIST);
	int minictrl_cnt = box_get_list_size(MINICTRL_LIST);
	_D("System Count : %d, Minictrl Count : %d", system_cnt, minictrl_cnt);

	enabled_noti_cnt = MAX_NOTI_ICONS_PORT - system_cnt - minictrl_cnt;
	if(enabled_noti_cnt <= 0) {
		enabled_noti_cnt = 1;    // Notification icon must show at least 1.
	}

	_D("Notification icon enabled_noti_cnt %d",enabled_noti_cnt);

	return enabled_noti_cnt;
}



int box_get_enabled_system_count(void)
{
	int system_cnt = 0;
	int noti_cnt = box_get_list_size(NOTI_LIST);
	int minictrl_cnt = box_get_list_size(MINICTRL_LIST);

	_D("Noti count : %d , MiniCtrl count : %d", noti_cnt, minictrl_cnt);

	system_cnt = PORT_SYSTEM_ICON_COUNT;  // MAX = 5(UI Guide).

	if(noti_cnt > 0) {
		system_cnt--;    // Notification icon must show at least 1.
	}

	if(minictrl_cnt > 0) {
		system_cnt--;    // Minictrl icon must show at least 1.
	}

	return system_cnt;
}


int box_get_enabled_connection_system_count(void)
{
	_D("box_get_enabled_connection_system_count");

	return PORT_CONNECTION_SYSTEM_ICON_COUNT; /* MAX = 2 */
}

int box_get_enabled_minictrl_count(void)
{
	int icon_count = 0;
	int noti_count = box_get_list_size(NOTI_LIST);
	int system_count = box_get_list_size(SYSTEM_LIST);

	icon_count = PORT_MINICTRL_ICON_COUNT; // = 2.    MIN (1) / MAX (6)

	if(noti_count) {	// noti_count >= 1
		if(system_count >= 3) {
			icon_count--;	// icon_count = 2 -> 1
		} else if(system_count <= 1) {
			icon_count++;	// icon_count = 2 -> 3
		}
	} else {	// noti_count == 0
		if(system_count >= 4) {
			icon_count--;	// icon_count = 2 -> 1
		} else if(system_count <= 2) {
			icon_count++;	// icon_count = 2 -> 3
		}
	}

	_D("Noti count : %d, System count : %d, Minictrl count : %d", noti_count, system_count, icon_count);

	return icon_count;
}



int box_get_max_count_in_non_fixed_list(void)
{
	return PORT_NONFIXED_ICON_COUNT;
}



Icon_AddType box_is_enable_to_insert_in_non_fixed_list(icon_s *obj)
{
	icon_s *icon;
	Eina_List *l;

	int higher_cnt = 0;
	int same_cnt = 0;
	int same_top_cnt = 0;
	int lower_cnt = 0;
	int noti_cnt = box_get_list_size(NOTI_LIST);
	Eina_List * tmp_list = NULL;

	retv_if(!obj, 0);

	switch (obj->area) {
	case INDICATOR_ICON_AREA_SYSTEM:
		tmp_list = _view_system_list;
		break;
	case INDICATOR_ICON_AREA_MINICTRL:
		tmp_list = _view_minictrl_list;
		break;
	case INDICATOR_ICON_AREA_NOTI:
		tmp_list = _view_noti_list;
		break;
	default:
		_D("obj area does not exists");
		break;
	}

	EINA_LIST_FOREACH(tmp_list, l, icon) {
		/* If same Icon exist in non-fixed view list, it need not to add icon */
		if (!strcmp(icon->name, obj->name)) {
			return CANNOT_ADD;
		}

		if (icon->priority > obj->priority) {
			++higher_cnt;
		} else if (icon->priority == obj->priority) {
			++same_cnt;
			if (icon->always_top == EINA_TRUE) {
				++same_top_cnt;
			}
		} else {
			lower_cnt++;
		}
	}

	if (obj->area == INDICATOR_ICON_AREA_SYSTEM) {
		if (higher_cnt + same_cnt + lower_cnt >= box_get_enabled_system_count()) {
			if (obj->always_top == EINA_TRUE) {
				if(same_top_cnt >= box_get_enabled_system_count()) {
					return CANNOT_ADD;
				} else {
					return CAN_ADD_WITH_DEL_SYSTEM;
				}
			} else {
				if (higher_cnt >= box_get_enabled_system_count()) {
					return CANNOT_ADD;
				} else {
					return CAN_ADD_WITH_DEL_SYSTEM;
				}
			}
		} else {
			return CAN_ADD_WITHOUT_DEL;
		}
	} else if (obj->area == INDICATOR_ICON_AREA_MINICTRL) {
		if (higher_cnt + same_cnt + lower_cnt >= box_get_enabled_minictrl_count()) {
			if (obj->always_top == EINA_TRUE) {
				if (same_top_cnt >= box_get_enabled_minictrl_count()) {
					return CANNOT_ADD;
				} else {
					return CAN_ADD_WITH_DEL_MINICTRL;
				}
			} else {
				if (higher_cnt >= box_get_enabled_minictrl_count()) {
					return CANNOT_ADD;
				} else {
					return CAN_ADD_WITH_DEL_MINICTRL;
				}
			}
		} else {
			return CAN_ADD_WITHOUT_DEL;
		}
	} else {
		if (noti_cnt > MAX_NOTI_ICONS_PORT) {
			return CAN_ADD_WITH_DEL_NOTI;
		} else {
			return CAN_ADD_WITHOUT_DEL;
		}
	}

	return CANNOT_ADD;
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



void box_icon_state_set(int bShow, char* file, int line)
{
	icon_show_state = bShow;
}



int box_icon_state_get(void)
{
	return icon_show_state;
}



extern Eina_Bool box_exist_icon(icon_s *obj)
{
	retv_if(!obj, ECORE_CALLBACK_CANCEL);

	switch (obj->area) {
	case INDICATOR_ICON_AREA_FIXED:
		if (eina_list_data_find(_view_fixed_list, obj)) {
			return EINA_TRUE;
		} else {
			return EINA_FALSE;
		}
	case INDICATOR_ICON_AREA_SYSTEM:
		if (eina_list_data_find(_view_system_list, obj)) {
			return EINA_TRUE;
		} else {
			return EINA_FALSE;
		}
	case INDICATOR_ICON_AREA_MINICTRL:
		if (eina_list_data_find(_view_minictrl_list, obj)) {
			return EINA_TRUE;
		} else {
			return EINA_FALSE;
		}
	case INDICATOR_ICON_AREA_CONNECTION_SYSTEM:
		if (eina_list_data_find(_view_connection_system_list, obj)) {
			return EINA_TRUE;
		} else {
			return EINA_FALSE;
		}
	case INDICATOR_ICON_AREA_NOTI:
		if (eina_list_data_find(_view_noti_list, obj)) {
			return EINA_TRUE;
		} else {
			return EINA_FALSE;
		}
	default:
		break;
	}

	return EINA_FALSE;
}



int box_handle_animated_gif(icon_s *icon)
{
	display_state_e state;
	Evas_Object *icon_eo = evas_object_data_get(icon->img_obj.obj, DATA_KEY_IMG_ICON);

	retvm_if(icon == NULL, FAIL, "Invalid parameter!");

	if (elm_image_animated_available_get(icon_eo) == EINA_FALSE) {
		return FAIL;
	}

	int ret = device_display_get_state(&state);
	if (ret != DEVICE_ERROR_NONE) {
		_E("device_display_get_state failed: %s", get_error_message(ret));
		return FAIL;
	}

	switch (state) {
	case DISPLAY_STATE_SCREEN_OFF:	//LCD OFF
		elm_image_animated_play_set(icon_eo, EINA_FALSE);
		break;
	case DISPLAY_STATE_NORMAL:	//LCD ON
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
			if (bStart == 1) {
				util_start_noti_ani(icon);
			} else {
				util_stop_noti_ani(icon);
			}
		}
	}
}

/* End of file */
