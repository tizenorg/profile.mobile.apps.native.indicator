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


#include <stdio.h>
#include <stdlib.h>
#include <notification.h>
#include <notification_internal.h>
#include <app_manager.h>
#include <app_preference.h>

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "box.h"
#include "icon.h"
#include "list.h"
#include "util.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"notification"

#define MSG_SERVER "/usr/bin/msg-server"
#define MSG_ICON "/usr/share/icons/default/small/org.tizen.message-lite.png"

static int noti_ready = 0;
static Eina_List *status_list;

static int register_noti_module(void *data);
static int unregister_noti_module(void);

icon_s noti = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.exist_in_view = EINA_FALSE,
	.init = register_noti_module,
	.fini = unregister_noti_module
};

struct noti_status {
	notification_h noti;
	int type;
	int cnt;
	icon_s *icon;
};


static void set_app_state(void *data)
{
	noti.ad = data;
}


static void show_icon_with_path(struct noti_status *data, char* icon_path)
{
	retm_if(data == NULL, "Invalid parameter!");
	_D("%s", icon_path);

	data->icon->img_obj.data = strdup(icon_path);
	icon_show(data->icon);
}


static void hide_image_icon(struct noti_status *data)
{
	retm_if(data == NULL, "Invalid parameter!");

	icon_hide(data->icon);
}


static void free_image_icon(struct noti_status *data)
{
	retm_if(data == NULL, "Invalid parameter!");

	list_remove_icon(data->icon);

	if (data->icon) {
		if (data->icon->img_obj.data) {
			free((char *)data->icon->img_obj.data);
			data->icon->img_obj.data = NULL;
		}

		if (data->icon->name) {
			free(data->icon->name);
			data->icon->name = NULL;
		}

		free(data->icon);
		data->icon = NULL;
	}


	if (data != NULL) {
		free(data);
		data = NULL;
	}
}


static void insert_icon_list(struct noti_status *data)
{
	retm_if(data == NULL, "Invalid parameter!");

	list_insert_icon(data->icon);
}


char *__indicator_ui_get_pkginfo_icon(const char *pkgid)
{
	int ret;
	char *icon_path = NULL;
	app_info_h app_info;

	retvm_if(pkgid == NULL, NULL, "invalid parameter");

	ret = app_info_create(pkgid, &app_info);

	retvm_if(ret != APP_MANAGER_ERROR_NONE, NULL, "app_info_create for %s failed %d", pkgid, ret);

	/* Icon path */
	ret = app_info_get_icon(app_info, &icon_path);
	if (ret != APP_MANAGER_ERROR_NONE) {
		app_info_destroy(app_info);
		_E("app_info_get_icon failed %d", ret);
		return NULL;
	}

	app_info_destroy(app_info);

	return icon_path;
}

static void show_image_icon(struct noti_status *data)
{
	retm_if(data == NULL, "Invalid parameter!");

	char *icon_path = NULL;

	notification_h noti = NULL;

	if (data->noti) {
		noti = data->noti;
		if (noti) {
			notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, &icon_path);

			if (icon_path == NULL || !ecore_file_exists(icon_path)) {
				if (icon_path != NULL && util_check_noti_ani(icon_path)) {
					show_icon_with_path(data, icon_path);
				} else {
					notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon_path);

					if (icon_path == NULL || !ecore_file_exists(icon_path)) {
						char *pkgname = NULL;
						char *icon_path_second = NULL;
						notification_get_pkgname(noti, &pkgname);
						icon_path_second = __indicator_ui_get_pkginfo_icon(pkgname);

						if (icon_path_second == NULL || !ecore_file_exists(icon_path_second))
							data->icon->img_obj.data = NULL;
						else
							show_icon_with_path(data, icon_path_second);

						if (icon_path_second != NULL)
							free(icon_path_second);
					} else
						show_icon_with_path(data, icon_path);
				}
			} else
				show_icon_with_path(data, icon_path);
		}
	}
}


static void show_image_icon_all(void)
{
	Eina_List *l = NULL;
	struct noti_status *data = NULL;

	EINA_LIST_REVERSE_FOREACH(status_list, l, data) {
		if (data) {
			show_image_icon(data);
		}
	}
}


static void _icon_add(struct noti_status *noti_data, const char *name, void *data)
{
	retm_if(noti_data == NULL || data == NULL, "Invalid parameter!");

	icon_s *obj = NULL;
	obj = calloc(1, sizeof(icon_s));

	if (obj) {
		memset(obj, 0, sizeof(icon_s));
		obj->type = INDICATOR_IMG_ICON;
		obj->name = strdup(name);
		obj->priority = ICON_PRIORITY;
		obj->always_top = EINA_FALSE;
		obj->ad = data;
		obj->area = INDICATOR_ICON_AREA_NOTI;
		obj->exist_in_view = EINA_FALSE;

		noti_data->icon = obj;
	}

	return;
}


static void _remove_all_noti(void)
{
	Eina_List *l = NULL;
	struct noti_status *n_data = NULL;

	/* Clear List and objects in list */
	EINA_LIST_FOREACH(status_list, l, n_data) {
		hide_image_icon(n_data);
		free_image_icon(n_data);
		status_list = eina_list_remove_list(status_list, l);
	}
	eina_list_free(status_list);
}


static int _is_exist_by_privid(const char *privid)
{
	Eina_List *l = NULL;
	struct noti_status *n_data = NULL;
	retvm_if(privid == NULL, 0, "Invalid parameter!");

	/* Clear List and objects in list */
	EINA_LIST_FOREACH(status_list, l, n_data) {
		if (!strcmp(n_data->icon->name, privid)) {
			return EINA_TRUE;
			break;
		}
	}
	return EINA_FALSE;
}


static int _indicator_noti_display_check(notification_h noti)
{
	int applist = 0;
	int noti_ret = 0;

	noti_ret = notification_get_display_applist(noti, &applist);

	retv_if(noti_ret != NOTIFICATION_ERROR_NONE, 0);

	retv_if(!(applist & NOTIFICATION_DISPLAY_APP_INDICATOR), 0);

	return 1;
}


static void _remove_noti_by_privid(int priv_id)
{
	Eina_List *l = NULL;
	struct noti_status *n_data = NULL;
	char priv_id_str[256] = {0,};

	snprintf(priv_id_str, sizeof(priv_id_str), "%d", priv_id);

	EINA_LIST_FOREACH(status_list, l, n_data) {

		if (strcmp(n_data->icon->name, priv_id_str) == 0) {
			_D("remove %s", priv_id_str);
			status_list = eina_list_remove(status_list, n_data);
			hide_image_icon(n_data);
			free_image_icon(n_data);
			break;
		}
	}
}


static void _insert_noti_by_privid(notification_h noti, void *data)
{
	int exist = 0;
	struct noti_status *status = NULL;
	int prev_id = -1;
	char *pkgname = NULL;
	char prev_id_str[256] = {0,};

	retm_if(noti == NULL, "Invalid parameter!");

	notification_get_pkgname(noti, &pkgname);
	notification_get_id(noti, NULL, &prev_id);

	ret_if(_indicator_noti_display_check(noti) == 0);

	snprintf(prev_id_str, sizeof(prev_id_str), "%d", prev_id);

	exist = _is_exist_by_privid(prev_id_str);

	if (exist != EINA_TRUE) {
		_D("Make New Event Icon : %s %s", pkgname, prev_id_str);
		status = calloc(1, sizeof(struct noti_status));
		status->type = 0;
		_icon_add(status, prev_id_str, data);
		status->noti = noti;
		insert_icon_list(status);
		status_list = eina_list_append(status_list, status);
		show_image_icon(status);
	}
}


static void _update_noti_by_privid(notification_h noti)
{
	Eina_List *l = NULL;
	struct noti_status *n_data = NULL;
	int priv_id = -1;
	char priv_id_str[256] = {0,};
	char *pkgname = NULL;

	retm_if(noti == NULL, "Invalid parameter!");

	notification_get_pkgname(noti, &pkgname);
	notification_get_id(noti, NULL, &priv_id);

	ret_if(_indicator_noti_display_check(noti) == 0);

	snprintf(priv_id_str, sizeof(priv_id_str), "%d", priv_id);

	char *indicator_path = NULL;
	char *icon_path = NULL;

	notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, &indicator_path);
	if (indicator_path == NULL || !ecore_file_exists(indicator_path)) {
		char *noti_path = NULL;

		notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &noti_path);

		if (noti_path == NULL || !ecore_file_exists(noti_path)) {
			char *pkgname = NULL;
			char *icon_path_second = NULL;
			notification_get_pkgname(noti, &pkgname);
			icon_path_second = __indicator_ui_get_pkginfo_icon(pkgname);
			if (icon_path_second != NULL)
				icon_path = strdup(icon_path_second);
			if (icon_path_second != NULL)
				free(icon_path_second);
		} else
			icon_path = strdup(noti_path);
	} else
		icon_path = strdup(indicator_path);

	EINA_LIST_FOREACH(status_list, l, n_data) {

		if (strcmp(n_data->icon->name, priv_id_str) == 0) {
			_D("Update Event Icon : %s %s, %s", priv_id_str, pkgname, icon_path);
			if (icon_path != NULL) {
				if (n_data->icon->img_obj.data != NULL) {
					if (strcmp(n_data->icon->img_obj.data, icon_path) == 0) {
						_D("same icon with exsting noti");
						if (icon_path != NULL) {
							free(icon_path);
							icon_path = NULL;
						}
					}
				}
			}
			n_data->noti = noti;
			show_image_icon(n_data);
		}
	}
	if (icon_path != NULL) {
		free(icon_path);
		icon_path = NULL;
	}
	return;
}


static void _change_icon_status(void *data, notification_list_h noti_list)
{
	int new_cnt = 0;
	Eina_List *l = NULL;
	notification_h noti = NULL;
	struct noti_status *n_data = NULL;
	int noti_count = 0, ongoing_count = 0;

	/* TODO: 2014/07/16 notification_get_count will be deprecated.
	   If this function is using, use another solution. */

	new_cnt = noti_count + ongoing_count;

	ret_if(!noti_list);

	/* Clear List and objects in list */
	EINA_LIST_FOREACH(status_list, l, n_data) {
		hide_image_icon(n_data);
		free_image_icon(n_data);
		status_list = eina_list_remove_list(status_list, l);
	}
	eina_list_free(status_list);

	while (noti_list) {
		char *pkgname = NULL;
		struct noti_status *status = NULL;
		Eina_Bool status_exist = EINA_FALSE;
		notification_error_e noti_ret = NOTIFICATION_ERROR_NONE;
		int applist;
		int prev_id = -1;
		char prev_id_str[256] = {0,};

		noti = notification_list_get_data(noti_list);
		noti_list = notification_list_get_next(noti_list);

		noti_ret = notification_get_display_applist(noti, &applist);
		if (noti_ret != NOTIFICATION_ERROR_NONE) continue;
		if (!(applist & NOTIFICATION_DISPLAY_APP_INDICATOR)) continue;

		noti_ret = notification_get_id(noti, NULL, &prev_id);

		snprintf(prev_id_str, sizeof(prev_id_str), "%d", prev_id);

		if (noti_ret != NOTIFICATION_ERROR_NONE) {
			noti_ret = notification_get_pkgname(noti, &pkgname);
			if (noti_ret != NOTIFICATION_ERROR_NONE) continue;
			_D("Cannot Get pkgname of notication! : %p %p", noti, pkgname);
		} else {
			status_exist = _is_exist_by_privid(prev_id_str);

			if (status_exist != EINA_TRUE) {
				status = calloc(1, sizeof(struct noti_status));
				ret_if(!status);

				status->type = 0;
				status->cnt = new_cnt;
				_icon_add(status, prev_id_str, data);
				status->noti = noti;
				insert_icon_list(status);
				status_list = eina_list_append(status_list, status);
			}
		}
	}

	show_image_icon_all();
}


void update_noti_module_new(void *data, notification_type_e type)
{
	notification_list_h list = NULL;
	notification_list_h noti_list_head = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	int get_event_count = box_get_max_count_in_non_fixed_list();

	retm_if(data == NULL, "Invalid parameter!");

	/* Get ongoing + noti count */
	noti_err = notification_get_list(NOTIFICATION_TYPE_NONE, get_event_count, &list);

	noti_list_head = list;

	if (noti_err != NOTIFICATION_ERROR_NONE || list == NULL) {
		_remove_all_noti();
		notification_free_list(noti_list_head);
		return;
	}

	_change_icon_status(data, list);

	notification_free_list(noti_list_head);
}

static void _indicator_noti_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	int i = 0;
	int op_type = 0;
	int priv_id = 0;

	notification_h noti_new = NULL;

	retm_if(num_op < 0, "invalid parameter %d", num_op);

	for (i = 0; i < num_op; i++) {

		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_TYPE, &op_type);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_NOTI, &noti_new);

		if (type != -1) {
			switch (op_type) {
				case NOTIFICATION_OP_INSERT:
					_insert_noti_by_privid(noti_new, data);
					break;
				case NOTIFICATION_OP_UPDATE:
					_update_noti_by_privid(noti_new);
					break;
				case NOTIFICATION_OP_DELETE:
					_remove_noti_by_privid(priv_id);
					break;
				default:
					break;
			}
		}

	}
}


static int register_noti_module(void *data)
{
	retvm_if(data == NULL, FAIL, "Invalid parameter!");
	static int bRegisterd = 0;

	set_app_state(data);

	if (bRegisterd == 0) {
		notification_register_detailed_changed_cb(_indicator_noti_detailed_changed_cb, data);
		bRegisterd = 1;
	}

	return OK;
}


static int unregister_noti_module(void)
{
	Eina_List *l = NULL;
	struct noti_status *data = NULL;

	notification_unregister_detailed_changed_cb(_indicator_noti_detailed_changed_cb, noti.ad);

	EINA_LIST_FOREACH(status_list, l, data) {
		if (data != NULL) {
			free_image_icon(data);
			status_list = eina_list_remove_list(status_list, l);
		}
	}

	eina_list_free(status_list);
	return OK;
}
