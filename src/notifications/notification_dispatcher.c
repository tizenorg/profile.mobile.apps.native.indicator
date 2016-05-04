/*
 *  Indicator
 *
 * Copyright (c) 2000 - 2016 Samsung Electronics Co., Ltd. All rights reserved.
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

#include <notification.h>
#include <notification_internal.h>


#include "main.h"
#include "log.h"
#include "instant_message.h"

struct noti_s {
	notification_h noti;
	int private_id;
};

static Eina_List *notifications_list;

static void list_notification_apps(int applist)
{
	_D("TICKER Display apps list:");
	if (applist & NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY) {
		_D("App set: NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY");
	}
	if (applist & NOTIFICATION_DISPLAY_APP_TICKER) {
		_D("App set: NOTIFICATION_DISPLAY_APP_TICKER");
	}
	if (applist & NOTIFICATION_DISPLAY_APP_LOCK) {
		_D("App set: NOTIFICATION_DISPLAY_APP_LOCK");
	}
	if (applist & NOTIFICATION_DISPLAY_APP_INDICATOR) {
		_D("App set: NOTIFICATION_DISPLAY_APP_INDICATOR");
	}
	if (applist & NOTIFICATION_DISPLAY_APP_ACTIVE) {
		_D("App set: NOTIFICATION_DISPLAY_APP_ACTIVE");
	}
	if (applist & NOTIFICATION_DISPLAY_APP_ALL) {
		_D("App set: NOTIFICATION_DISPLAY_APP_ALL");
	}
}



static void insert_noti(struct noti_s *notification, void *data)
{
	/*
	 * TODO
	 *
	 * Handle inserting notification dependably on applist
	 *
	 */
	int applist;
	int ret;

	/*
	 * INDICATOR type
	 */

	ret = notification_get_display_applist(notification->noti, &applist);
	if (applist & (NOTIFICATION_DISPLAY_APP_INDICATOR | NOTIFICATION_DISPLAY_APP_ALL)) {

		char *message = calloc(256, sizeof(char));
		notification_text_type_e type = NOTIFICATION_TEXT_TYPE_CONTENT;

		notification_get_text(notification->noti, type, &message);
//		new_message();

	}

	/*
	 * TICKER Type
	 *
	 */

	notifications_list = eina_list_append(notifications_list, notification);
}

static void update_noti(struct noti_s *notification)
{
	/*
	 * TODO
	 *
	 *  Update noti by private id
	 *
	 */
}

static void delete_noti_by_id(int private_id)
{
	/*
	 * TODO
	 *
	 *  Delete noti by private id
	 *
	 */
}

static void notification_dispatcher_detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	int i;
	for (i = 0 ; i < num_op ; i++) {
		_D("I:%d", i);
		notification_op_type_e op_type;
		int applist;

		struct noti_s *notification;

		notification = calloc(1, sizeof(struct noti_s));

		notification->noti = op_list[i].noti;
		op_type = op_list[i].type;
		notification->private_id = op_list[i].priv_id;

		_D("NOTIFICATION PRIVATE_ID:%d", notification->private_id);
		_D("NOTIFICATION TYPE:%d", type);

		notification_get_display_applist(notification->noti, &applist);
		list_notification_apps(applist);

		switch(op_type) {
		case NOTIFICATION_OP_INSERT:
			insert_noti(notification, data);
			break;
		case NOTIFICATION_OP_UPDATE:
			update_noti(notification);
			break;
		case NOTIFICATION_OP_DELETE:
			delete_noti_by_id(notification->private_id);
			free(notification);
			break;
		default:
			_E("Invalid operation type");
		}


	}
}

void notification_dispatcher_init(void *data)
{
	int ret;
	ret = notification_register_detailed_changed_cb(notification_dispatcher_detailed_changed_cb, data);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_register_detailed_changed_cb failed[%d]: %s",
			ret, get_error_message(ret));

	return;
}

void notification_dispatcher_deinit(void)
{
	int ret;
	ret = notification_unregister_detailed_changed_cb(notification_dispatcher_detailed_changed_cb, NULL);
	retm_if(ret != NOTIFICATION_ERROR_NONE, "notification_unregister_detailed_changed_cb failed[%d]: %s",
			ret, get_error_message(ret));

	return;
}
