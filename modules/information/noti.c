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


#include <stdio.h>
#include <stdlib.h>
#include <notification.h>
#include "common.h"
#include "indicator.h"
#include "indicator_icon_util.h"
#include "indicator_ui.h"
#include "indicator_icon_list.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"notification"

static int register_noti_module(void *data);
static int unregister_noti_module(void);
static int hib_enter_noti_module(void);
static int hib_leave_noti_module(void *data);

Indicator_Icon_Object noti[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.exist_in_view = EINA_FALSE,
	.init = register_noti_module,
	.fini = unregister_noti_module,
	.hib_enter = hib_enter_noti_module,
	.hib_leave = hib_leave_noti_module
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.exist_in_view = EINA_FALSE,
	.init = register_noti_module,
	.fini = unregister_noti_module,
	.hib_enter = hib_enter_noti_module,
	.hib_leave = hib_leave_noti_module
}
};

struct noti_status {
	notification_h noti;
	int type;
	int cnt;
	Indicator_Icon_Object *icon[INDICATOR_WIN_MAX];
};


static Eina_List *status_list;
static int noti_cnt;

static void set_app_state(void* data)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		noti[i].ad = data;
	}
}

static void show_icon_with_path(struct noti_status *data, char* icon_path)
{
	int i = 0;
	retif(data == NULL, , "Invalid parameter!");
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		data->icon[i]->img_obj.data = strdup(icon_path);
		indicator_util_icon_show(data->icon[i]);
	}
}

static void hide_image_icon(struct noti_status *data)
{
	int i = 0;
	retif(data == NULL, , "Invalid parameter!");
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(data->icon[i]);
	}
}

static void free_image_icon(struct noti_status *data)
{
	int i = 0;
	retif(data == NULL, , "Invalid parameter!");
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_icon_list_remove(data->icon[i]);

		if (data->icon[i])
		{
			if (data->icon[i]->img_obj.data)
				free((char *)data->icon[i]->img_obj.data);

			free(data->icon[i]);
		}
	}
}

static void insert_icon_list(struct noti_status *data)
{
	int i = 0;
	retif(data == NULL, , "Invalid parameter!");
	for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_icon_list_insert(data->icon[i]);
	}
}

static void show_image_icon(struct noti_status *data)
{
	retif(data == NULL, , "Invalid parameter!");

	char *icon_path;
	notification_h noti = NULL;

	if (data->noti) {
		noti = data->noti;
		if (noti) {
			notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, &icon_path);
			if(icon_path == NULL)
			{
				DBG("no indicator icon");
				notification_get_icon(noti, &icon_path);
			}
			DBG("Get Path of Notication %s : %s",data->icon[0]->name, icon_path);
			if (icon_path == NULL
				|| !ecore_file_exists(icon_path)) {
				int i = 0;
				for(i=0 ; i<INDICATOR_WIN_MAX ; i++)
				{
					data->icon[i]->img_obj.data = NULL;
				}
			} else {
				show_icon_with_path(data, icon_path);
			}
		}
	}
}

static void show_image_icon_all( void )
{
	Eina_List *l;
	struct noti_status *data;
	int total = 0, noti_count = 0;

	EINA_LIST_REVERSE_FOREACH(status_list, l, data) {
		if (data) {
			show_image_icon(data);
		}
	}
}


static void _icon_add(struct noti_status *noti_data, const char *name, void *data)
{
	int i = 0;
	DBG("noti_data %x",noti_data);
	retif(noti_data == NULL || data == NULL, NULL, "Invalid parameter!");

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		Indicator_Icon_Object *obj = NULL;
		obj = calloc(1, sizeof(Indicator_Icon_Object));

		if (obj) {
			memset(obj, 0, sizeof(Indicator_Icon_Object));
			obj->win_type = i;
			obj->type = INDICATOR_IMG_ICON;
			obj->name = strdup(name);
			obj->priority = ICON_PRIORITY;
			obj->always_top = EINA_FALSE;
			obj->ad = data;
			obj->area = INDICATOR_ICON_AREA_NOTI;
			obj->exist_in_view = EINA_FALSE;

			noti_data->icon[i] = obj;
		}
	}

	return;
}

static void _set_noti_cnt(int cnt)
{
	if (cnt >= 0) {
		noti_cnt = cnt;
		indicator_util_event_count_set(noti_cnt, noti[0].ad);
	}
}

static int _get_noti_cnt(void)
{
	return noti_cnt;
}

static void _update_all_status(void)
{
	Eina_List *l;
	struct noti_status *data;
	int total = 0, noti_count = 0;

	EINA_LIST_FOREACH(status_list, l, data) {
		if (data) {
			DBG("%s is updated! Cnt : %d",
				data->icon[0]->name, data->cnt);
			notification_get_count(NOTIFICATION_TYPE_NONE, NULL,
				NOTIFICATION_GROUP_ID_NONE,
				NOTIFICATION_PRIV_ID_NONE,
				&noti_count);
			data->cnt = noti_count;

			if (data->cnt == 0)
				hide_image_icon(data);
			else
				total += data->cnt;
		}
	}
}

static void _change_icon_status(void *data, notification_list_h noti_list)
{
	int cnt = 0, new_cnt = 0;
	Eina_List *l = NULL;
	notification_h noti = NULL;
	struct noti_status *n_data = NULL;
	int noti_count = 0, ongoing_count = 0;

	notification_get_count(NOTIFICATION_TYPE_NOTI, NULL,
		NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE,
		&noti_count);
	notification_get_count(NOTIFICATION_TYPE_ONGOING, NULL,
		NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE,
		&ongoing_count);

	new_cnt = noti_count + ongoing_count;

	retif(noti_list == NULL, , "Invalid parameter!");

	EINA_LIST_FOREACH(status_list, l, n_data) {
		DBG("Clear Status List : %s", n_data->icon[0]->name);
		hide_image_icon(n_data);
		free_image_icon(n_data);
		status_list = eina_list_remove_list(status_list, l);
	}
	eina_list_free(status_list);

	while (noti_list != NULL) {
		char *pkgname = NULL;
		Eina_List *l = NULL;
		struct noti_status *n_data = NULL;
		struct noti_status *status = NULL;
		Eina_Bool status_exist = EINA_FALSE;
		notification_error_e noti_ret = NOTIFICATION_ERROR_NONE;
		int applist;

		noti = notification_list_get_data(noti_list);
		noti_list = notification_list_get_next(noti_list);

		noti_ret = notification_get_display_applist(noti, &applist);
		if (noti_ret != NOTIFICATION_ERROR_NONE) {
			INFO("Cannot Get display app of notication! : %p ",
					noti);
			continue;
		}
		if (!(applist & NOTIFICATION_DISPLAY_APP_INDICATOR))
			continue;

		noti_ret = notification_get_pkgname(noti, &pkgname);

		if (noti_ret != NOTIFICATION_ERROR_NONE)
			DBG("Cannot Get pkgname of notication! : %p %p",
					noti, pkgname);
		else {
			EINA_LIST_FOREACH(status_list, l, n_data) {
				if (!strcmp(n_data->icon[0]->name, pkgname)) {
					DBG("%s is already existed", pkgname);
					status_exist = EINA_TRUE;
					break;
				}
			}

			if (status_exist != EINA_TRUE) {
				DBG("Make New Event Icon : %s", pkgname);
				status = calloc(1, sizeof(struct noti_status));
				status->type = 0;
				status->cnt = new_cnt;
				_icon_add(status,pkgname, data);
				status->noti = noti;
				insert_icon_list(status);
				status_list = eina_list_append(status_list, status);
				cnt = _get_noti_cnt() + status->cnt;
			}
		}
	}

	show_image_icon_all();
}

void update_noti_module_new(void *data, notification_type_e type)
{
	notification_list_h list = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	int get_event_count = indicator_util_max_visible_event_count(INDICATOR_WIN_LAND);

	retif(data == NULL, , "Invalid parameter!");

	noti_err = notification_get_grouping_list(NOTIFICATION_TYPE_NONE,
				get_event_count, &list);

	if (noti_err != NOTIFICATION_ERROR_NONE || list == NULL) {
		_update_all_status();
		return;
	}

	_change_icon_status(data, list);
}


static int _indicator_check_first_start(void)
{
	int status = 0;
	int ret = 0;

	ret = vconf_get_bool(VCONFKEY_INDICATOR_STARTED, &status);
	if (ret) {
		INFO("fail to get %s", VCONFKEY_INDICATOR_STARTED);
		ret = vconf_set_bool(VCONFKEY_INDICATOR_STARTED, 1);
		INFO("set : %s, result : %d", VCONFKEY_INDICATOR_STARTED, ret);
	}

	if (status)
		return 0;

	return 1;
}

static void _indicator_noti_delete_volatile_data(void)
{
	notification_list_h noti_list = NULL;
	notification_list_h noti_list_head = NULL;
	notification_h noti = NULL;
	int property = 0;

	notification_get_grouping_list(NOTIFICATION_TYPE_NONE, -1, &noti_list);

	noti_list_head = noti_list;

	while (noti_list != NULL) {
		noti = notification_list_get_data(noti_list);
		notification_get_property(noti, &property);

		if (property & NOTIFICATION_PROP_VOLATILE_DISPLAY) {
			notification_set_property(noti,
				property |
				NOTIFICATION_PROP_DISABLE_UPDATE_ON_DELETE);
			notification_delete(noti);
		}

		noti_list = notification_list_get_next(noti_list);
	}

	notification_free_list(noti_list_head);

	notification_update(NULL);
}


static int register_noti_module(void *data)
{

	retif(data == NULL, FAIL, "Invalid parameter!");
	notification_error_e ret = NOTIFICATION_ERROR_NONE;
	int is_first = 0;

	set_app_state(data);

	is_first = _indicator_check_first_start();
	{
		notifiation_clear(NOTIFICATION_TYPE_ONGOING);
		_indicator_noti_delete_volatile_data();
	}

	ret = notification_resister_changed_cb(update_noti_module_new, data);

	if (ret != NOTIFICATION_ERROR_NONE)
		DBG("Fail to Register notification_resister_changed_cb!");

	update_noti_module_new(data, NOTIFICATION_TYPE_NOTI);

	return OK;
}

static int unregister_noti_module(void)
{
	Eina_List *l;
	struct noti_status *data = NULL;
	notification_error_e ret = NOTIFICATION_ERROR_NONE;

	ret = notification_unresister_changed_cb(update_noti_module_new);

	if (ret != NOTIFICATION_ERROR_NONE)
		DBG("Fail to unregister notification_resister_changed_cb!");

	EINA_LIST_FOREACH(status_list, l, data) {
		free_image_icon(data);
		status_list = eina_list_remove_list(status_list, l);
	}

	eina_list_free(status_list);
	return OK;
}

static int hib_enter_noti_module(void)
{
	return OK;
}

static int hib_leave_noti_module(void *data)
{
	retif(data == NULL, FAIL, "Invalid parameter!");
	update_noti_module_new(data, NOTIFICATION_TYPE_NOTI);
	return OK;
}

