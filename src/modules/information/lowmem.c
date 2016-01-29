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
#include <vconf.h>
#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include <E_DBus.h>
#include <sys/statvfs.h>

#define BUS_NAME       "org.tizen.system.deviced"
#define PATH_NAME    "/Org/Tizen/System/DeviceD/Lowmem"
#define INTERFACE_NAME BUS_NAME".lowmem"
#define MEMBER_NAME	"ChangeState"

static E_DBus_Connection *edbus_conn=NULL;
static E_DBus_Signal_Handler *edbus_handler=NULL;

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"lowmem"
#define TIMER_INTERVAL	0.3

static int register_lowmem_module(void *data);
static int unregister_lowmem_module(void);
static int wake_up_cb(void *data);
void check_storage();
void get_internal_storage_status(double *total, double *avail);

icon_s lowmem = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_lowmem_module,
	.fini = unregister_lowmem_module,
	.wake_up = wake_up_cb
};

static const char *icon_path[] = {
	"Storage/B03_storage_memoryfull.png",
	NULL
};


static int updated_while_lcd_off = 0;
static int bShown = 0;



static void set_app_state(void* data)
{
	lowmem.ad = data;
}



static void show_image_icon(void)
{
	if(bShown == 1)
	{
		return;
	}

	lowmem.img_obj.data = icon_path[0];
	icon_show(&lowmem);

	bShown = 1;
}

static void hide_image_icon(void)
{
	icon_hide(&lowmem);

	bShown = 0;
}



static void indicator_lowmem_pm_state_change_cb(keynode_t *node, void *data)
{
}



static int wake_up_cb(void *data)
{
	if(updated_while_lcd_off==0 && lowmem.obj_exist == EINA_FALSE)
	{
		return OK;
	}

	return OK;
}



static void on_changed_receive(void *data, DBusMessage *msg)
{
	DBusError err;
	int response;
	int r;

	DBG("edbus signal Received");

	r = dbus_message_is_signal(msg, INTERFACE_NAME, MEMBER_NAME);
	if (!r) {
		ERR("dbus_message_is_signal error");
		return;
	}

	SECURE_ERR("%s - %s", INTERFACE_NAME, MEMBER_NAME);

	dbus_error_init(&err);
	r = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);
	if (!r) {
		ERR("dbus_message_get_args error");
		return;
	}

	SECURE_ERR("receive data : %d", response);

	if(response==1)
	{
		show_image_icon();
	}
	else
	{
		hide_image_icon();
	}
}



static void edbus_cleaner(void)
{
	if(edbus_conn==NULL)
	{
		DBG("already unregistered");
		return;
	}

	if (edbus_handler)
	{
		e_dbus_signal_handler_del(edbus_conn, edbus_handler);
		edbus_handler = NULL;
	}
	if (edbus_conn)
	{
		e_dbus_connection_close(edbus_conn);
		edbus_conn = NULL;
	}
	e_dbus_shutdown();
}



static int edbus_listener(void)
{
	if(edbus_conn!=NULL)
	{
		DBG("alreay exist");
		return -1;
	}
	// Init
	e_dbus_init();

	edbus_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (edbus_conn == NULL) {
		ERR("e_dbus_bus_get error");
		return -1;
	}
	edbus_handler = e_dbus_signal_handler_add(edbus_conn, NULL, PATH_NAME,
								INTERFACE_NAME, MEMBER_NAME,
								on_changed_receive, NULL);
	if (edbus_handler == NULL) {
		ERR("e_dbus_signal_handler_add error");
		return -1;
	}
	DBG("dbus listener run");
	return 0;

}



static int register_lowmem_module(void *data)
{
	int ret;

	retif(data == NULL, FAIL, "Invalid parameter!");

	set_app_state(data);

	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE,
						indicator_lowmem_pm_state_change_cb, data);

	check_storage();
	edbus_listener();

	return ret;
}



static int unregister_lowmem_module(void)
{
	int ret;


	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE,
						indicator_lowmem_pm_state_change_cb);

	edbus_cleaner();

	return ret;
}



void check_storage()
{
	double total = 0.0;
	double available = 0.0;
	double percentage = 0.0;
	get_internal_storage_status(&total, &available);
	percentage = (available/total) * 100.0;
	DBG("check_storage : Total : %lf, Available : %lf Percentage : %lf", total, available, percentage);
	if(percentage <= 5.0)
	{
		show_image_icon();
	}
}



void get_internal_storage_status(double *total, double *avail)
{
	int ret;
	double tmp_total;
	struct statvfs s;
	const double sz_32G = 32. * 1073741824;
	const double sz_16G = 16. * 1073741824;
	const double sz_8G = 8. * 1073741824;

	retif(total == NULL, , "Invalid parameter!");
	retif(avail == NULL, , "Invalid parameter!");

	ret = statvfs("/opt/usr", &s);
	if (0 == ret)
	{
		tmp_total = (double)s.f_frsize * s.f_blocks;
		*avail = (double)s.f_bsize * s.f_bavail;

		if (sz_16G < tmp_total)
			*total = sz_32G;
		else if (sz_8G < tmp_total)
			*total = sz_16G;
		else
			*total = sz_8G;
	}
}
