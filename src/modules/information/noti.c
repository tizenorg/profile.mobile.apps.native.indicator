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


#define NOTI_MINICTRL_APP_LIST_SIZE 6

typedef struct {
	char *name;
	int priority;
} minictrl_prop;

static minictrl_prop noti_minictrl_app_list[NOTI_MINICTRL_APP_LIST_SIZE] = {
		{"call", 1},
		{"call_mute", 2},
		{"call_speaker", 3},
		{"music", 4},
		{"video", 5},
		{"voice_recorder", 6},
		/* {"3rd_party", 7} TODO This will be handled later*/
};

struct noti_status {
	notification_h noti;
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
	retm_if(!data, "Invalid parameter!");

	char *icon_path = NULL;

	if (data->noti) {
		notification_get_image(data->noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, &icon_path);

		if (icon_path) {
			_D("Icon path:%s", icon_path);

			char *real_path = util_get_real_path(icon_path);
			if (real_path) {
				show_icon_with_path(data, real_path);
				free(real_path);
				return;
			}

			if(ecore_file_exists(icon_path) || util_check_noti_ani(icon_path))
				show_icon_with_path(data, icon_path);
			else
				_E("The path is invalid:%s", icon_path);
		} else
			_E("The path is NULL");
	}
}


static void show_image_icon_all(void)
{
	Eina_List *l = NULL;
	struct noti_status *data = NULL;

	EINA_LIST_REVERSE_FOREACH(status_list, l, data) {
		if (data)
			show_image_icon(data);
	}
}


static int noti_icon_priority_get(const char *tag)
{
	int i;
	int minictrl_len = strlen("minicontrol_");
	for (i = 0; i < NOTI_MINICTRL_APP_LIST_SIZE; i++){
		if (!strcmp(tag + minictrl_len, noti_minictrl_app_list[i].name)) {
			_D("Sufix is is:%s", noti_minictrl_app_list[i].name);
			return noti_minictrl_app_list[i].priority;
		}
	}
	return 0;
}


static void _noti_minicontrol_set(struct noti_status *noti_data)
{
	const char *tag = NULL;
	int ret;

	ret = notification_get_tag(noti_data->noti, &tag);
	retm_if(ret != NOTIFICATION_ERROR_NONE,
			"notification_get_tag failed[%d]:%s", ret, get_error_message(ret));

	if (util_string_prefix_check("minicontrol", tag)){
		int p = noti_icon_priority_get(tag);
		if(p) {
			_D("The icon is for minicontrol area.");
			noti_data->icon->priority = p;
			noti_data->icon->area = INDICATOR_ICON_AREA_MINICTRL;
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

		_noti_minicontrol_set(noti_data);
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


static bool _indicator_noti_display_check(notification_h noti)
{
	int applist = 0;
	int noti_ret = 0;

	noti_ret = notification_get_display_applist(noti, &applist);

	retv_if(noti_ret != NOTIFICATION_ERROR_NONE, false);

	retv_if(!(applist & NOTIFICATION_DISPLAY_APP_INDICATOR), false);

	return true;
}


static void _remove_noti_by_privid(int priv_id)
{
	Eina_List *l = NULL;
	struct noti_status *n_data = NULL;
	char priv_id_str[256] = {0,};

	snprintf(priv_id_str, sizeof(priv_id_str), "%d", priv_id);

	EINA_LIST_FOREACH(status_list, l, n_data) {

		if (!strcmp(n_data->icon->name, priv_id_str)) {
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
	int ret;
	struct noti_status *status = NULL;
	int prev_id = -1;
	char *pkgname = NULL;
	char prev_id_str[256] = {0,};
	char *icon_path = NULL;

	retm_if(!noti, "Invalid parameter!");

	ret_if(!_indicator_noti_display_check(noti));

	ret = notification_get_pkgname(noti, &pkgname);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_get_pkgname failed[%d]:%s", ret , get_error_message(ret));

	ret = notification_get_id(noti, NULL, &prev_id);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_get_id failed[%d]:%s", ret , get_error_message(ret));

	ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, &icon_path);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_get_id failed[%d]:%s", ret , get_error_message(ret));

	if (!icon_path) {
		_E("The icon path is NULL");
		return;
	}

	/* Converting int into char* */
	snprintf(prev_id_str, sizeof(prev_id_str), "%d", prev_id);

	if (!_is_exist_by_privid(prev_id_str)) {
		status = calloc(1, sizeof(struct noti_status));
		notification_clone(noti, &status->noti);
		_icon_add(status, prev_id_str, data);

		insert_icon_list(status);
		status_list = eina_list_append(status_list, status);
		show_image_icon(status);
	} else
		_E("The notification is already registered to indicator!");
}


static void _update_noti_by_privid(notification_h noti)
{
	Eina_List *l = NULL;
	struct noti_status *n_data = NULL;
	int priv_id = -1;
	int ret;
	char priv_id_str[256] = {0,};
	char *pkgname = NULL;

	retm_if(noti == NULL, "Invalid parameter!");

	ret = notification_get_pkgname(noti, &pkgname);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_get_pkgname failed[%d]:%s", ret , get_error_message(ret));

	ret = notification_get_id(noti, NULL, &priv_id);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_get_id failed[%d]:%s", ret , get_error_message(ret));

	ret_if(!_indicator_noti_display_check(noti));

	snprintf(priv_id_str, sizeof(priv_id_str), "%d", priv_id);

	char *indicator_path = NULL;
	char *icon_path = NULL;

	ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, &indicator_path);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_get_image failed[%d]:%s", ret , get_error_message(ret));

	if (!indicator_path) {
		_E("The icon path is NULL");
		return;
	} else {
		if (ecore_file_exists(indicator_path) ||
				util_check_noti_ani(indicator_path) ||
				util_reserved_path_check(indicator_path)) {
			icon_path = strdup(indicator_path);

			EINA_LIST_FOREACH(status_list, l, n_data) {
				if (!strcmp(n_data->icon->name, priv_id_str)) {
					_D("Update Icon : %s %s, %s", priv_id_str, pkgname, icon_path);
							if (!strcmp(n_data->icon->img_obj.data, icon_path))
								_D("same icon with existing noti");
					if(n_data->noti) {
						free(n_data->noti);
						n_data->noti = NULL;
					}
					notification_clone(noti, &n_data->noti);
					retm_if(!n_data->noti, "Noti clone is NULL!");

					show_image_icon(n_data);
				}
			}
		} else
			_E("The path is invalid:%s", icon_path);

		if (icon_path) {
			free(icon_path);
			icon_path = NULL;
		}
	}
	return;
}


static void _change_icon_status(void *data, notification_list_h noti_list)
{
	Eina_List *l = NULL;
	notification_h noti = NULL;
	struct noti_status *n_data = NULL;
	int ret = NOTIFICATION_ERROR_NONE;
	ret_if(!noti_list);

	/* Clear List and objects in list */
	EINA_LIST_FOREACH(status_list, l, n_data) {
		hide_image_icon(n_data);
		free_image_icon(n_data);
		status_list = eina_list_remove_list(status_list, l);
	}
	eina_list_free(status_list);

	while (noti_list) {
		struct noti_status *status = NULL;
		int prev_id = -1;
		char prev_id_str[256] = {0,};
		char *icon_path = NULL;

		noti = notification_list_get_data(noti_list);
		ret = get_last_result();
		if (!noti || ret != NOTIFICATION_ERROR_NONE) {
			_E("notification_list_get_data failed[%d]:%s", ret, get_error_message(ret));
			break;
		}

		noti_list = notification_list_get_next(noti_list);

		if(!_indicator_noti_display_check(noti)) continue;

		ret = notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR, &icon_path);
		if (ret != NOTIFICATION_ERROR_NONE) continue;

		if (!icon_path) {
			_E("The file path is NULL");
			continue;
		}

		ret = notification_get_id(noti, NULL, &prev_id);
		if (ret != NOTIFICATION_ERROR_NONE) continue;

		snprintf(prev_id_str, sizeof(prev_id_str), "%d", prev_id);

		if (_is_exist_by_privid(prev_id_str) == EINA_FALSE) {
			status = calloc(1, sizeof(struct noti_status));
			ret_if(!status);

			_icon_add(status, prev_id_str, data);
			notification_clone(noti, &status->noti);
			insert_icon_list(status);
			status_list = eina_list_append(status_list, status);
		}
	}
	show_image_icon_all();
}


void update_noti_module_new(void *data, notification_type_e type)
{
	notification_list_h list = NULL;
	notification_error_e ret = NOTIFICATION_ERROR_NONE;

	retm_if(data == NULL, "Invalid parameter!");

	ret = notification_get_list(NOTIFICATION_TYPE_NONE, -1, &list);

	if (ret != NOTIFICATION_ERROR_NONE || !list)
		_remove_all_noti();
	else {
		_change_icon_status(data, list);
		notification_free_list(list);
	}
}

static void _indicator_noti_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	int i = 0;
	int op_type = 0;
	int priv_id = 0;

	notification_h noti_new = NULL;

	retm_if(num_op < 0, "invalid parameter %d", num_op);
	retm_if(!notification_is_service_ready(), "Notification service is not ready");


	for (i = 0; i < num_op; i++) {

		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_TYPE, &op_type);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_NOTI, &noti_new);

		if (type != NOTIFICATION_TYPE_NONE) {
			switch (op_type) {
				case NOTIFICATION_OP_SERVICE_READY:
					_D("");
					update_noti_module_new(data, type);
					break;
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
