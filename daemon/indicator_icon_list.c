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


#include "common.h"
#include "indicator.h"
#include "indicator_icon_list.h"
#include "indicator_icon_util.h"

static Eina_List *fixed_icon_list[INDICATOR_WIN_MAX] = {NULL,};
static Eina_List *system_icon_list[INDICATOR_WIN_MAX] = {NULL,};
static Eina_List *noti_icon_list[INDICATOR_WIN_MAX] = {NULL,};


void indicator_icon_object_free(Indicator_Icon_Object *icon)
{
	if (icon) {
		DBG("%s!",icon->name);
		if (icon->obj_exist == EINA_TRUE) {
			if (indicator_util_icon_del(icon) == EINA_TRUE) {
				icon->obj_exist = EINA_FALSE;
				icon->txt_obj.obj = NULL;
				icon->img_obj.obj = NULL;
			}
		}
	}
}

static int indicator_icon_list_free(Eina_List *list)
{
	Eina_List *l;
	Eina_List *l_next;
	Indicator_Icon_Object *data;

	retif(list == NULL, OK, "Empty List!");

	EINA_LIST_FOREACH_SAFE(list, l, l_next, data) {
		indicator_icon_object_free(data);
		list = eina_list_remove_list(list, l);
		if (eina_error_get())
			return FAIL;
	}
	eina_list_free(list);
	list = NULL;
	return eina_error_get();
}

int indicator_icon_all_list_free(void)
{
	int i = 0;
	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		indicator_icon_list_free(fixed_icon_list[i]);
		indicator_icon_list_free(system_icon_list[i]);
		indicator_icon_list_free(noti_icon_list[i]);
	}

	return TRUE;
}

int indicator_icon_list_update(Indicator_Icon_Object *obj)
{
	Eina_List *l;

	Indicator_Icon_Object *data;

	retif(obj == NULL, FAIL, "Invalid parameter!");

	DBG("%s",obj->name);

	if (obj->area == INDICATOR_ICON_AREA_FIXED)
	{
		fixed_icon_list[obj->win_type] = eina_list_remove(fixed_icon_list[obj->win_type], obj);

		EINA_LIST_REVERSE_FOREACH(fixed_icon_list[obj->win_type], l, data) {
			if (data->priority == obj->priority
				&&data->always_top == EINA_TRUE)
				continue;

			if (data->priority <= obj->priority) {
				fixed_icon_list[obj->win_type] = eina_list_append_relative_list(
						fixed_icon_list[obj->win_type], obj, l);
				return eina_error_get();
			}
		}

		fixed_icon_list[obj->win_type] = eina_list_prepend(fixed_icon_list[obj->win_type], obj);
	}
	else if(obj->area == INDICATOR_ICON_AREA_SYSTEM)
	{
		system_icon_list[obj->win_type] = eina_list_remove(system_icon_list[obj->win_type], obj);

		EINA_LIST_REVERSE_FOREACH(system_icon_list[obj->win_type], l, data) {
			if (data->priority == obj->priority
				&&data->always_top == EINA_TRUE) {
				continue;
			}
			if (data->priority <= obj->priority) {
				system_icon_list[obj->win_type] =
					eina_list_append_relative_list(
						system_icon_list[obj->win_type], obj, l);
				return eina_error_get();
			}
		}

		system_icon_list[obj->win_type] = eina_list_prepend(system_icon_list[obj->win_type], obj);
	}
	else
	{
		noti_icon_list[obj->win_type] = eina_list_remove(noti_icon_list[obj->win_type], obj);

		EINA_LIST_REVERSE_FOREACH(noti_icon_list[obj->win_type], l, data) {
			if (data->priority == obj->priority
				&&data->always_top == EINA_TRUE) {
				continue;
			}
			if (data->priority >= obj->priority) {
				noti_icon_list[obj->win_type] =
					eina_list_append_relative_list(
						noti_icon_list[obj->win_type], obj, l);
				return eina_error_get();
			}
		}

		noti_icon_list[obj->win_type] = eina_list_prepend(noti_icon_list[obj->win_type], obj);
	}

	return eina_error_get();
}

int indicator_icon_list_insert(Indicator_Icon_Object *obj)
{
	Eina_List *l;
	Indicator_Icon_Object *data;

	retif(obj == NULL || obj->name == NULL, FAIL, "Invalid parameter!");

	DBG("%s!",obj->name);

	if (obj->area == INDICATOR_ICON_AREA_FIXED)
	{
		EINA_LIST_REVERSE_FOREACH(fixed_icon_list[obj->win_type], l, data) {
			retif(data->name == obj->name, FAIL,
			      "%s is already exist in the list!", obj->name);
		}

		obj->wish_to_show = EINA_FALSE;

		EINA_LIST_REVERSE_FOREACH(fixed_icon_list[obj->win_type], l, data) {
			if (data->priority == obj->priority
				&& data->always_top == EINA_TRUE)
				continue;

			if (data->priority <= obj->priority) {
				fixed_icon_list[obj->win_type] = eina_list_append_relative_list(
							fixed_icon_list[obj->win_type], obj, l);
					return eina_error_get();
			}
		}
		fixed_icon_list[obj->win_type] = eina_list_prepend(fixed_icon_list[obj->win_type], obj);
	}
	else if(obj->area == INDICATOR_ICON_AREA_SYSTEM)
	{
		EINA_LIST_REVERSE_FOREACH(system_icon_list[obj->win_type], l, data) {
			retif(data->name == obj->name, FAIL,
			      "%s is already exist in the list!", obj->name);
		}

		obj->wish_to_show = EINA_FALSE;

		EINA_LIST_REVERSE_FOREACH(system_icon_list[obj->win_type], l, data) {
			if (data->priority == obj->priority &&
				data->always_top == EINA_TRUE)
				continue;

			if (data->priority <= obj->priority) {
				system_icon_list[obj->win_type] =
					eina_list_append_relative_list(
						system_icon_list[obj->win_type], obj, l);
				return eina_error_get();
			}
		}

		system_icon_list[obj->win_type] = eina_list_prepend(system_icon_list[obj->win_type], obj);
	}
	else
	{
		EINA_LIST_REVERSE_FOREACH(noti_icon_list[obj->win_type], l, data) {
			retif(data->name == obj->name, FAIL,
			      "%s is already exist in the list!", obj->name);
		}

		obj->wish_to_show = EINA_FALSE;

		EINA_LIST_REVERSE_FOREACH(noti_icon_list[obj->win_type], l, data) {
			if (data->priority == obj->priority &&
				data->always_top == EINA_TRUE)
				continue;

			if (data->priority >= obj->priority) {
				noti_icon_list[obj->win_type] =
					eina_list_append_relative_list(
						noti_icon_list[obj->win_type], obj, l);
				DBG("Append");
				return eina_error_get();
			}
		}
		noti_icon_list[obj->win_type] = eina_list_prepend(noti_icon_list[obj->win_type], obj);
		DBG("Prepend");
	}

	return eina_error_get();
}

int indicator_icon_list_remove(Indicator_Icon_Object *obj)
{
	DBG("%s!",obj->name);

	if (obj->area == INDICATOR_ICON_AREA_FIXED)
	{
		retif(fixed_icon_list[obj->win_type] == NULL
			|| obj == NULL, FAIL, "Invalid parameter!");
		fixed_icon_list[obj->win_type] = eina_list_remove(fixed_icon_list[obj->win_type], obj);
	}
	else if(obj->area == INDICATOR_ICON_AREA_SYSTEM)
	{
		retif(system_icon_list[obj->win_type] == NULL
			|| obj == NULL, FAIL, "Invalid parameter!");
		system_icon_list[obj->win_type] = eina_list_remove(system_icon_list[obj->win_type], obj);
	}
	else
	{
		retif(noti_icon_list[obj->win_type] == NULL
			|| obj == NULL, FAIL, "Invalid parameter!");
		noti_icon_list[obj->win_type] = eina_list_remove(noti_icon_list[obj->win_type], obj);
	}

	return eina_error_get();
}

Indicator_Icon_Object
*indicator_get_wish_to_show_icon(int win_type, int area, int priority)
{
	Eina_List *l;
	Indicator_Icon_Object *data = NULL;
	Indicator_Icon_Object *ret = NULL;

	if (area == INDICATOR_ICON_AREA_FIXED)
	{
		EINA_LIST_REVERSE_FOREACH(fixed_icon_list[win_type], l, data) {
			if (data->priority == priority
				&& data->wish_to_show == EINA_TRUE
				&& data->exist_in_view == EINA_FALSE)
			{
				ret = data;
				break;
			}
		}

	}
	else if(area == INDICATOR_ICON_AREA_SYSTEM)
	{
		EINA_LIST_REVERSE_FOREACH(system_icon_list[win_type], l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->exist_in_view == EINA_FALSE)
			{
				ret = data;
				break;
			}
		}

	}
	else
	{
		EINA_LIST_REVERSE_FOREACH(noti_icon_list[win_type], l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->exist_in_view == EINA_FALSE)
			{
				ret = data;
				break;
			}
		}

	}

	if(ret != NULL)
		DBG("%d,%s",area, ret->name);
	else
		ret = NULL;

	return ret;
}

Indicator_Icon_Object
*indicator_get_wish_to_remove_icon(int win_type, int area, int priority)
{
	Eina_List *l;
	Indicator_Icon_Object *data = NULL;
	Indicator_Icon_Object *ret = NULL;

	if (area == INDICATOR_ICON_AREA_FIXED)
	{
		EINA_LIST_REVERSE_FOREACH(fixed_icon_list[win_type], l, data) {
			if (data->priority == priority
				&& data->wish_to_show == EINA_TRUE)
			{
				ret = data;
				break;
			}
		}
	}
	else if (area == INDICATOR_ICON_AREA_SYSTEM)
	{
		EINA_LIST_FOREACH(system_icon_list[win_type], l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->always_top == EINA_FALSE
				&& data->exist_in_view == EINA_TRUE)
			{
				ret = data;
				break;
			}
		}
	}
	else
	{
		EINA_LIST_FOREACH(noti_icon_list[win_type], l, data) {
			if (data->wish_to_show == EINA_TRUE
				&& data->always_top == EINA_FALSE
				&& data->exist_in_view == EINA_TRUE)
			{
				ret = data;
				break;
			}
		}
	}


	return ret;
}

