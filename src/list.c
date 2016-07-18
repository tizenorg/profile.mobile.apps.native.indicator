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



#include "indicator.h"
#include "log.h"
#include "util.h"
#include "list.h"
#include "icon.h"

static Eina_List *fixed_icon_list = NULL;
static Eina_List *system_icon_list = NULL;
static Eina_List *noti_icon_list = NULL;
static Eina_List *alarm_icon_list = NULL;
static Eina_List *minictrl_icon_list = NULL;
static Eina_List *connection_system_icon_list = NULL;



extern void icon_free(icon_s *icon)
{
	if (icon) {
		if (icon->obj_exist == EINA_TRUE) {
			if (icon_del(icon) == EINA_TRUE) {
				icon->obj_exist = EINA_FALSE;
				icon->img_obj.obj = NULL;
			}
		}
	}
}

static void _list_free(Eina_List *list)
{
	Eina_List *l;
	Eina_List *l_next;
	icon_s *data;

	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, l_next, data) {
		icon_free(data);
		list = eina_list_remove_list(list, l);
	}
	eina_list_free(list);
	list = NULL;

	return;
}

Eina_List *list_list_get(indicator_icon_area_type type)
{
	switch(type) {
	case INDICATOR_ICON_AREA_FIXED:
		return fixed_icon_list;
	case INDICATOR_ICON_AREA_SYSTEM:
		return system_icon_list;
	case INDICATOR_ICON_AREA_MINICTRL:
		return minictrl_icon_list;
	case INDICATOR_ICON_AREA_NOTI:
		return noti_icon_list;
	case INDICATOR_ICON_AREA_CONNECTION_SYSTEM:
		return connection_system_icon_list;
	case INDICATOR_ICON_AREA_ALARM:
		return alarm_icon_list;
	default:
		_E("Invalid area type");
		return NULL;
	}
}


static Eina_List **_list_list_ptr_get(indicator_icon_area_type type)
{
	switch(type) {
	case INDICATOR_ICON_AREA_FIXED:
		return &fixed_icon_list;
	case INDICATOR_ICON_AREA_SYSTEM:
		return &system_icon_list;
	case INDICATOR_ICON_AREA_MINICTRL:
		return &minictrl_icon_list;
	case INDICATOR_ICON_AREA_NOTI:
		return &noti_icon_list;
	case INDICATOR_ICON_AREA_CONNECTION_SYSTEM:
		return &connection_system_icon_list;
	case INDICATOR_ICON_AREA_ALARM:
		return &alarm_icon_list;
	default:
		_E("Invalid area type");
		return NULL;
	}
}

extern void list_free_all(void)
{
	_list_free(fixed_icon_list);
	_list_free(system_icon_list);
	_list_free(noti_icon_list);
	_list_free(alarm_icon_list);
	_list_free(connection_system_icon_list);
	_list_free(minictrl_icon_list);
}


unsigned int list_get_wish_to_show_icons_cnt(enum indicator_icon_area_type area)
{
	int count = 0;
	Eina_List *l;
	icon_s *icon;

	EINA_LIST_REVERSE_FOREACH(list_list_get(area), l, icon) {
				if (icon->wish_to_show)
					count++;
	}
	return count;
}

static Eina_List *_insert_icon_to_list(Eina_List *list, icon_s *icon)
{
	Eina_List *l;
	icon_s *data;

	retv_if(!icon, list);

	EINA_LIST_FOREACH(list, l, data) {
		if (data->priority >= icon->priority) {
			list = eina_list_prepend_relative_list(list, icon, l);
			return list;
		}
	}
	list = eina_list_append(list, icon);
	return list;
}


extern void list_update(icon_s *icon)
{
	ret_if(!icon);

	Eina_List *l;
	icon_s *data;

	if (icon->area == INDICATOR_ICON_AREA_NOTI) {
		EINA_LIST_FOREACH(noti_icon_list, l, data) {
			if (!strcmp(icon->name, data->name))
				noti_icon_list = eina_list_remove_list(noti_icon_list, l);
		}
		noti_icon_list = _insert_icon_to_list(noti_icon_list, icon);
	} else {
		Eina_List **list;
		list = _list_list_ptr_get(icon->area);

		*list = eina_list_remove(*list, icon);
		*list = _insert_icon_to_list(*list, icon);
	}
	return;
}


static indicator_error_e _icon_exist_in_list(Eina_List *list, icon_s *icon)
{
	Eina_List *l;
	icon_s *data;

	EINA_LIST_REVERSE_FOREACH(list, l, data) {
		if (!strcmp(data->name, icon->name)) {
			_D("[%s] already exists in the list", icon->name);
			return INDICATOR_ERROR_FAIL;
		}
	}
	return INDICATOR_ERROR_NONE;
}


extern void list_insert_icon(icon_s *icon)
{
	ret_if(!icon);
	ret_if(!icon->name);

	Eina_List **list;

	list = _list_list_ptr_get(icon->area);

	ret_if(!list);

	if (_icon_exist_in_list(*list, icon) != INDICATOR_ERROR_NONE)
		return;

	icon->wish_to_show = EINA_FALSE;
	*list = _insert_icon_to_list(*list, icon);

	return;
}


extern void list_remove_icon(icon_s *icon)
{
	ret_if(!icon);

	Eina_List **list;

	list = _list_list_ptr_get(icon->area);

	ret_if(!list);
	*list = eina_list_remove(*list, icon);
}


extern icon_s *list_try_to_find_icon_to_show(int area, int priority)
{
	Eina_List *l;
	icon_s *data = NULL;
	icon_s *icon = NULL;

	EINA_LIST_FOREACH(list_list_get(area), l, data) {
			if (data->wish_to_show == EINA_TRUE && data->exist_in_view == EINA_FALSE) {
			icon = data;
			break;
		}
	}
	return icon;
}


extern icon_s *list_try_to_find_icon_to_remove(int area, int priority)
{
	Eina_List *l;
	icon_s *data = NULL;
	icon_s *icon = NULL;

	EINA_LIST_REVERSE_FOREACH(list_list_get(area), l, data) {
			if (data->wish_to_show == EINA_TRUE && data->exist_in_view == EINA_TRUE) {
			icon = data;
			break;
		}
	}
	return icon;
}

/* End of file */
