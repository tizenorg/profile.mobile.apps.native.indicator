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



extern int list_free_all(void)
{
	_list_free(fixed_icon_list);
	_list_free(system_icon_list);
	_list_free(noti_icon_list);
	_list_free(alarm_icon_list);

	return true;
}



static Eina_List *_insert_icon_to_list(Eina_List *list, icon_s *icon)
{
	Eina_List *l;
	icon_s *data;

	/* FIXME */
	//retv_if(!list, NULL);
	retv_if(!icon, NULL);

	/* Insert icon to list */
	EINA_LIST_REVERSE_FOREACH(list, l, data) {
		if (data->priority == icon->priority
			&& data->always_top == EINA_TRUE)
			continue;

		if (data->priority <= icon->priority) {
			list = eina_list_append_relative_list(list, icon, l);
			return list;
		}
	}

	/* If finding condition is failed, append it at tail */
	list = eina_list_prepend(list, icon);
	return list;
}



extern void list_update(icon_s *icon)
{
	ret_if(!icon);

	switch (icon->area) {
	case INDICATOR_ICON_AREA_FIXED:
		fixed_icon_list = eina_list_remove(fixed_icon_list, icon);
		fixed_icon_list = _insert_icon_to_list(fixed_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_SYSTEM:
		system_icon_list = eina_list_remove(system_icon_list, icon);
		system_icon_list = _insert_icon_to_list(system_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_NOTI:
		noti_icon_list = eina_list_remove(noti_icon_list, icon);
		noti_icon_list = _insert_icon_to_list(noti_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_ALARM:
		alarm_icon_list = eina_list_remove(alarm_icon_list, icon);
		alarm_icon_list = _insert_icon_to_list(alarm_icon_list, icon);
		break;
	default:
		break;
	}

	return;
}



static indicator_error_e _icon_exist_in_list(Eina_List *list, icon_s *icon)
{
	Eina_List *l;
	icon_s *data;

	/* Check name */
	EINA_LIST_REVERSE_FOREACH(fixed_icon_list, l, data) {
		if (data->name == icon->name) {
			_D("[%s] is already exist in the list", icon->name);
			return INDICATOR_ERROR_FAIL;
		}
	}
	return INDICATOR_ERROR_NONE;
}


extern void list_insert_icon(icon_s *icon)
{
	ret_if(!icon);
	ret_if(!icon->name);

	switch (icon->area) {
	case INDICATOR_ICON_AREA_FIXED:
		if (INDICATOR_ERROR_NONE != _icon_exist_in_list(fixed_icon_list, icon)) return;

		/* Set internal data */
		icon->wish_to_show = EINA_FALSE;
		fixed_icon_list = _insert_icon_to_list(fixed_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_SYSTEM:
		if (INDICATOR_ERROR_NONE != _icon_exist_in_list(system_icon_list, icon)) return;

		/* Set internal data */
		icon->wish_to_show = EINA_FALSE;
		system_icon_list = _insert_icon_to_list(system_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_NOTI:
		if (INDICATOR_ERROR_NONE != _icon_exist_in_list(noti_icon_list, icon)) return;

		/* Set internal data */
		icon->wish_to_show = EINA_FALSE;
		noti_icon_list = _insert_icon_to_list(noti_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_ALARM:
		if (INDICATOR_ERROR_NONE != _icon_exist_in_list(alarm_icon_list, icon)) return;

		/* Set internal data */
		icon->wish_to_show = EINA_FALSE;
		alarm_icon_list = eina_list_append(alarm_icon_list, icon);
		break;
	default:
		break;
	}

	return;
}


extern void list_remove_icon(icon_s *icon)
{
	ret_if(!icon);

	switch (icon->area) {
	case INDICATOR_ICON_AREA_FIXED:
		ret_if(!fixed_icon_list);
		fixed_icon_list = eina_list_remove(fixed_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_SYSTEM:
		ret_if(!system_icon_list);
		system_icon_list = eina_list_remove(system_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_NOTI:
		ret_if(!noti_icon_list);
		noti_icon_list = eina_list_remove(noti_icon_list, icon);
		break;
	case INDICATOR_ICON_AREA_ALARM:
		ret_if(!alarm_icon_list);
		alarm_icon_list = eina_list_remove(alarm_icon_list, icon);
		break;
	default:
		_E("default");
		break;
	}
}


extern icon_s *list_try_to_find_icon_to_show(int area, int priority)
{
	Eina_List *l;
	icon_s *data = NULL;
	icon_s *icon = NULL;

	switch (area) {
	case INDICATOR_ICON_AREA_FIXED:
		EINA_LIST_REVERSE_FOREACH(fixed_icon_list, l, data) {
			if (data->priority == priority
				&& data->wish_to_show == EINA_TRUE
				&& data->exist_in_view == EINA_FALSE) {
				icon = data;
				break;
			}
		}
		break;
	case INDICATOR_ICON_AREA_SYSTEM:
		EINA_LIST_REVERSE_FOREACH(system_icon_list, l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->exist_in_view == EINA_FALSE) {
				icon = data;
				break;
			}
		}
		break;
	case INDICATOR_ICON_AREA_NOTI:
		EINA_LIST_REVERSE_FOREACH(noti_icon_list, l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->exist_in_view == EINA_FALSE) {
				icon = data;
				break;
			}
		}
		break;
	case INDICATOR_ICON_AREA_ALARM:
		EINA_LIST_REVERSE_FOREACH(alarm_icon_list, l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->exist_in_view == EINA_FALSE) {
				icon = data;
				break;
			}
		}
		break;
	default:
		_E("default");
		break;
	}

	retv_if(!icon, NULL);

	return icon;
}



extern icon_s *list_try_to_find_icon_to_remove(int area, int priority)
{
	Eina_List *l;
	icon_s *data = NULL;
	icon_s *icon = NULL;


	switch (area) {
	case INDICATOR_ICON_AREA_FIXED:
		EINA_LIST_REVERSE_FOREACH(fixed_icon_list, l, data) {
			if (data->priority == priority
				&& data->wish_to_show == EINA_TRUE) {
				icon = data;
				break;
			}
		}
		break;
	case INDICATOR_ICON_AREA_SYSTEM:
		/* Find lowest priority of icon */
		EINA_LIST_FOREACH(system_icon_list, l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->always_top == EINA_FALSE
				&& data->exist_in_view == EINA_TRUE) {
				icon = data;
				break;
			}
		}
		break;
	case INDICATOR_ICON_AREA_NOTI:
		/* Find lowest priority of icon */
		EINA_LIST_REVERSE_FOREACH(noti_icon_list, l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->always_top == EINA_FALSE
				&& data->exist_in_view == EINA_TRUE) {
				icon = data;
				break;
			}
		}
		break;
	case INDICATOR_ICON_AREA_ALARM:
		/* Find lowest priority of icon */
		EINA_LIST_FOREACH(alarm_icon_list, l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->always_top == EINA_FALSE
				&& data->exist_in_view == EINA_TRUE) {
				icon = data;
				break;
			}
		}
		break;
	default:
		_E("default");
		break;
	}

	retv_if(!icon, NULL);

	return icon;
}



extern unsigned int list_get_noti_count(void)
{
	Eina_List *l;
	icon_s *data = NULL;
	int cnt = 0;

	EINA_LIST_FOREACH(noti_icon_list, l, data) {
		if (data->wish_to_show == EINA_TRUE
			|| data->exist_in_view == EINA_TRUE) {
			if (!strcmp(data->name, "more_notify"))	continue; // Except more noti
			cnt++;
		}
	}

	return cnt;
}

/* End of file */
