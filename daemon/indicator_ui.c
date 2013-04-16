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
#include <app.h>
#include <Ecore_X.h>
#include <vconf.h>
#include <unistd.h>
#include <privilege-control.h>
#include <app_manager.h>
#include <signal.h>
#include <minicontrol-monitor.h>
#include <feedback.h>

#include "common.h"
#include "indicator_box_util.h"
#include "indicator_icon_util.h"
#include "indicator_ui.h"
#include "indicator_gui.h"
#include "modules.h"
#include "indicator_util.h"

#define GRP_MAIN "indicator"

#define WIN_TITLE "Illume Indicator"

#define VCONF_PHONE_STATUS "memory/startapps/sequence"

#define UNLOCK_ENABLED	0
#define TIMEOUT			5

#ifdef HOME_KEY_EMULATION

#define PROP_HWKEY_EMULATION "_HWKEY_EMULATION"
#define KEY_MSG_PREFIX_PRESS "P:"
#define KEY_MSG_PREFIX_RELEASE "R:"
#define KEY_MSG_PREFIX_PRESS_C "PC"
#define KEY_MSG_PREFIX_RELEASE_C "RC"

#ifndef KEY_HOME
#define KEY_HOME "XF86Phone"
#endif
#endif

#define MENUSCREEN_PKG_NAME "com.samsung.menuscreen"
#define APP_TRAY_PKG_NAME "com.samsung.app-tray"

static Eina_Bool home_button_pressed = EINA_FALSE;
static Eina_Bool show_hide_pressed[INDICATOR_WIN_MAX] = {EINA_FALSE,EINA_FALSE};
static Ecore_Timer *hide_timer = NULL;

int indicator_icon_show_state[INDICATOR_WIN_MAX] = {0,};
int indicator_icon_backup_state[INDICATOR_WIN_MAX] = {0,};

static int is_quickpanel_opened = 0;
static int is_apptray_opened = 0;
static int current_angle = 0;
Evas_Coord_Point indicator_press_coord = {0,0};

#define STR_ATOM_MV_INDICATOR_GEOMETRY          "_E_MOVE_INDICATOR_GEOMETRY"

static void _change_home_padding(void *data, int angle);
static void _change_nonfixed_icon_padding(void *data, Eina_Bool status);
static void _change_max_nonfixed_icon_count(void *data,
					Eina_Bool status, int angle);
static Eina_Bool _change_view(Ecore_X_Window win, void *data);
static int check_system_status(void);

static int indicator_window_new(void *data);
static int indicator_window_del(void *data);

static void _indicator_check_battery_percent_on_cb(keynode_t *node, void *data);
static void _indicator_low_bat_cb(void *data);
static void _indicator_lang_changed_cb(void *data);
static void _indicator_region_changed_cb(void *data);
static void _indicator_window_delete_cb(void *data, Evas_Object * obj,
					void *event);
static Eina_Bool _indicator_client_message_cb(void *data, int type,
					      void *event);
static void _indicator_mouse_down_cb(void *data, Evas * e, Evas_Object * obj,
				     void *event);
static void _indicator_mouse_move_cb(void *data, Evas * e, Evas_Object * obj,
				     void *event);
static void _indicator_mouse_up_cb(void *data, Evas * e, Evas_Object * obj,
				   void *event);



static void _change_nonfixed_icon_padding(void *data, Eina_Bool status)
{
	retif(data == NULL, , "Invalid parameter!");

	if (status == EINA_TRUE)
		indicator_signal_emit(data, "change,padding,1", "elm.rect.*");
	else
		indicator_signal_emit(data, "change,padding,2", "elm.rect.*");
}


static void _change_max_nonfixed_icon_count(void *data,
					Eina_Bool status, int angle)
{
	struct appdata *ad = (struct appdata *)data;

	retif(data == NULL, , "Invalid parameter!");

	DBG("Current angle : %d", ad->angle);

	indicator_set_count_in_non_fixed_list(ad->angle, status);
}

static int check_system_status(void)
{
	int ret, value = -1;

	ret = vconf_get_int(VCONFKEY_PWLOCK_STATE, &value);
	if (ret == OK &&
	    (value == VCONFKEY_PWLOCK_BOOTING_LOCK ||
	     value == VCONFKEY_PWLOCK_RUNNING_LOCK))
		return FAIL;

	return OK;
}

static void _change_top_win(enum _win_type type, void *data)
{
	struct appdata *ad = data;
	int i = 0;
	retif(data == NULL, , "Invalid parameter!");

	DBG("Current Top Window : %d", type);
	ad->top_win = type;
	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		indicator_util_update_display(&(ad->win[i]));
	}
}

static char *_get_top_window_name(void *data)
{
	Ecore_X_Window topwin = ecore_x_window_root_first_get();
	Ecore_X_Window active;
	static Eina_Strbuf *temp_buf = NULL;

	char *pkgname = NULL;
	char *win_name = NULL;
	char *ret_name = NULL;

	int pid;

	retif(data == NULL, NULL, "Invalid parameter!");

	if (ecore_x_window_prop_xid_get(topwin, ECORE_X_ATOM_NET_ACTIVE_WINDOW,
					ECORE_X_ATOM_WINDOW, &active,
					1) == FAIL)
		return NULL;

	if (ecore_x_netwm_pid_get(active, &pid) == EINA_FALSE) {
		Ecore_X_Atom atom;
		unsigned char *in_pid;
		int num;

		atom = ecore_x_atom_get("X_CLIENT_PID");
		if (ecore_x_window_prop_property_get(topwin,
				atom, ECORE_X_ATOM_CARDINAL,
				sizeof(int), &in_pid, &num) == EINA_FALSE) {
			DBG("Failed to get PID from a window 0x%X", topwin);

			if(in_pid != NULL)
				free(in_pid);

			return NULL;
		}
		pid = *(int *)in_pid;
		free(in_pid);
	}

	DBG("Window (0x%X) PID is %d", topwin, pid);

	if (app_manager_get_package(pid,&pkgname) != APP_MANAGER_ERROR_NONE)
	{
		if (ecore_x_netwm_name_get(active, &win_name) == EINA_FALSE)
			return NULL;
		else
			return win_name;
	}

	DBG("Pkgname : %s", pkgname);

	temp_buf = eina_strbuf_new();
	eina_strbuf_append_printf(temp_buf, "%s", pkgname);
	ret_name = eina_strbuf_string_steal(temp_buf);
	eina_strbuf_free(temp_buf);

	if(pkgname != NULL)
		free(pkgname);

	return ret_name;
}

static Eina_Bool _change_view(Ecore_X_Window win, void *data)
{
	char *top_win_name = NULL;
	enum _win_type type;

	if (data == NULL)
		return EINA_FALSE;

	top_win_name = _get_top_window_name(data);

	if (top_win_name != NULL) {
		Eina_Bool ret = EINA_TRUE;

		INFO("TOP WINDOW NAME = %s", top_win_name);

		if (!strncmp(top_win_name, QUICKPANEL_NAME,
					strlen(top_win_name))) {
			type = TOP_WIN_QUICKPANEL;
			ret = EINA_FALSE;
		} else if (!strncmp(top_win_name, HOME_SCREEN_NAME,
						strlen(top_win_name)))
			type = TOP_WIN_HOME_SCREEN;
		else if (!strncmp(top_win_name, LOCK_SCREEN_NAME,
						strlen(top_win_name)))
			type = TOP_WIN_LOCK_SCREEN;
		else if (!strncmp(top_win_name, MENU_SCREEN_NAME,
						strlen(top_win_name)))
			type = TOP_WIN_MENU_SCREEN;
		else if (!strncmp(top_win_name, CALL_NAME,
						strlen(top_win_name)))
			type = TOP_WIN_CALL;
		else if (!strncmp(top_win_name, VTCALL_NAME,
						strlen(top_win_name)))
			type = TOP_WIN_CALL;
		else
			type = TOP_WIN_NORMAL;

		free(top_win_name);
		_change_top_win(type, data);

		return ret;
	} else {
		type = TOP_WIN_NORMAL;
	}
	_change_top_win(type, data);
	return EINA_TRUE;
}

static void _indicator_check_battery_percent_on_cb(keynode_t *node, void *data)
{
	struct appdata *ad = (struct appdata *)data;
	int ret = FAIL;
	int status = 0;
	retif(data == NULL, , "Invalid parameter!");

	ret = vconf_get_bool(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL, &status);

	if (ret == OK) {
		_change_nonfixed_icon_padding(data, status);
		_change_max_nonfixed_icon_count(data, status, ad->angle);
	} else
		ERR("Fail to get vconfkey : %s",
			VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL);
}

static void _indicator_low_bat_cb(void *data)
{
	INFO("LOW_BATTERY!");
}

static void _indicator_lang_changed_cb(void *data)
{
	INFO("CHANGE LANGUAGE!");
	indicator_lang_changed_modules(data);
}

static void _indicator_region_changed_cb(void *data)
{
	INFO("CHANGE REGION!");
	indicator_region_changed_modules(data);
}

static void _indicator_window_delete_cb(void *data, Evas_Object * obj,
					void *event)
{
	struct appdata *ad = (struct appdata *)data;
	retif(data == NULL, , "Invalid parameter!");

	indicator_window_del(ad);
}

static void _indicator_notify_pm_state_cb(keynode_t * node, void *data)
{

	int val = -1;

	if (data == NULL) {
		ERR("lockd is NULL");
		return;
	}

	if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		ERR("Cannot get VCONFKEY_PM_STATE");
		return;
	}

	DBG("PM state Notification!!(%d)",val);

	switch(val)
	{
		case VCONFKEY_PM_STATE_LCDOFF:
		case VCONFKEY_PM_STATE_SLEEP:
			indicator_util_set_update_flag(0);
			break;
		case VCONFKEY_PM_STATE_NORMAL:
			indicator_util_set_update_flag(1);
			indicator_wake_up_modules(data);
			break;
		case VCONFKEY_PM_STATE_LCDDIM:
		default:
			break;
	}

}

static void _indicator_notify_apptray_state_cb(keynode_t * node, void *data)
{

	struct appdata *ad = (struct appdata *)data;
	int val = -1;

	if (data == NULL) {
		ERR("lockd is NULL");
		return;
	}

	if (vconf_get_bool(VCONFKEY_APPTRAY_STATE, &val) < 0) {
		ERR("Cannot get VCONFKEY_APPTRAY_STATE");
		return;
	}

	DBG("_indicator_notify_apptray_state_cb!!(%d)",val);

	switch(val)
	{
		case 0:
			is_apptray_opened = 0;
			indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_LAND]),0,1);
			if (hide_timer != NULL) {
				ecore_timer_del(hide_timer);
				hide_timer = NULL;
			}
			break;
		case 1:
			is_apptray_opened = 1;
			indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_LAND]),1,0);
			if (hide_timer != NULL) {
				ecore_timer_del(hide_timer);
				hide_timer = NULL;
			}
			break;
		default:
			break;
	}

}

static void _indicator_power_off_status_cb(keynode_t * node, void *data)
{

	struct appdata *ad = (struct appdata *)data;
	int val = -1;

	if (data == NULL) {
		ERR("data is NULL");
		return;
	}

	if (vconf_get_int(VCONFKEY_SYSMAN_POWER_OFF_STATUS, &val) < 0) {
		ERR("Cannot get VCONFKEY_SYSMAN_POWER_OFF_STATUS");
		return;
	}

	DBG("_indicator_power_off_status_cb!!(%d)",val);

	switch(val)
	{
		case VCONFKEY_SYSMAN_POWER_OFF_DIRECT:
		case VCONFKEY_SYSMAN_POWER_OFF_RESTART:
			DBG("_indicator_power_off_status_cb : Terminated...");
			app_efl_exit();
			break;
		default:
			break;
	}

}

static void _rotate_window(void *data, int new_angle)
{
	retif(data == NULL, , "Invalid parameter!");

	struct appdata *ad = (struct appdata *)data;

	DBG("_rotate_window = %d",new_angle);

	current_angle = new_angle;

	switch (new_angle)
	{
		case 0:
		case 180:
			{
				if (hide_timer != NULL)
				{
					ecore_timer_del(hide_timer);
					hide_timer = NULL;
				}
				indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_LAND]),0,0);

				if(is_quickpanel_opened == 1)
				{
					indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_PORT]),1,0);
				}

				indicator_send_evas_ecore_message(&(ad->win[INDICATOR_WIN_PORT]),0,1);
			}
			break;
		case 90:
		case 270:
			if(is_quickpanel_opened == 0&&is_apptray_opened==0)
			{
				DBG("hide indicator = %d %d",is_quickpanel_opened,is_apptray_opened);
				indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_LAND]),0,0);
				indicator_send_evas_ecore_message(&(ad->win[INDICATOR_WIN_LAND]),1,2);
				if (hide_timer != NULL)
				{
					ecore_timer_del(hide_timer);
					hide_timer = NULL;
				}
			}
			else
			{
				indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_LAND]),1,0);
				indicator_send_evas_ecore_message(&(ad->win[INDICATOR_WIN_LAND]),0,1);
			}
			break;
		default:
			break;
	}
}

#ifdef INDICATOR_SUPPORT_OPACITY_MODE
static void _change_opacity(void *data, enum indicator_opacity_mode mode)
{
	struct appdata *ad = NULL;
	const char *signal = NULL;
	retif(data == NULL, , "Invalid parameter!");

	ad = data;

	switch (mode) {
	case INDICATOR_OPACITY_OPAQUE:
		signal = "bg.opaque";
		ad->opacity_mode = mode;
		break;
	case INDICATOR_OPACITY_TRANSLUCENT:
		signal = "bg.translucent";
		ad->opacity_mode = mode;
		break;
	case INDICATOR_OPACITY_TRANSPARENT:
		signal = "bg.transparent";
		ad->opacity_mode = mode;
		break;
	default:
		ERR("unknown mode : %d", mode);
		signal = "bg.opaque";
		ad->opacity_mode = INDICATOR_OPACITY_OPAQUE;
		break;

	}
	ad->opacity_mode = mode;

	indicator_signal_emit(data,signal, "indicator.prog");

	DBG("send signal [%s] to indicator layout", signal);
}

static void _indicator_quickpanel_changed(void *data, int is_open)
{
	struct appdata *ad = NULL;

	retif(data == NULL, , "Invalid parameter!");

	DBG("_indicator_quickpanel_changed %d %d\n", is_open, current_angle);

	ad = data;

	if (is_open)
	{
		if(is_quickpanel_opened==0)
		{
			is_quickpanel_opened = 1;

			if( current_angle==0 || current_angle == 180)
			{
				if(indicator_icon_backup_state[INDICATOR_WIN_PORT]==0)
				{
					indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_PORT]),1,1);
				}
				else
				{
					indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_PORT]),1,0);
				}
			}
			else
			{
				indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_LAND]),1,0);
			}

			vconf_set_int(VCONFKEY_BATTERY_DISP_STATE,2);

			if (hide_timer != NULL) {
				ecore_timer_del(hide_timer);
				hide_timer = NULL;
			}
		}
	}
	else
	{
		if(is_quickpanel_opened==1)
		{
			is_quickpanel_opened = 0;

			if( current_angle==0 || current_angle == 180)
			{
				indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_LAND]),0,0);

				if(indicator_icon_backup_state[INDICATOR_WIN_PORT]==0)
					indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_PORT]),0,1);
				else
					indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_PORT]),1,0);
			}
			else
			{
				indicator_util_show_hide_icons(&(ad->win[INDICATOR_WIN_LAND]),0,1);
				indicator_send_evas_ecore_message(&(ad->win[INDICATOR_WIN_LAND]),1,2);
			}
		}
	}
}
#endif

static Eina_Bool _indicator_client_message_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Client_Message *ev =
	    (Ecore_X_Event_Client_Message *) event;

	retif(data == NULL
	      || event == NULL, ECORE_CALLBACK_RENEW, "Invalid parameter!");


	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) {
		if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON)
			_indicator_quickpanel_changed(data, 1);
		else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF)
			_indicator_quickpanel_changed(data, 0);

	}

	return ECORE_CALLBACK_RENEW;
}


static Eina_Bool _property_changed_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Property *ev = event;

	if (ev == NULL || ev->atom != ECORE_X_ATOM_NET_ACTIVE_WINDOW)
		return EINA_FALSE;

	return EINA_TRUE;
}

static void _mctrl_monitor_cb(minicontrol_action_e action,
				const char *name, unsigned int width,
				unsigned int height,
				minicontrol_priority_e priority,
				void *data)
{
	retif(!data, , "data is NULL");
	retif(!name, , "name is NULL");

	indicator_minictrl_control_modules(action,name,data);
}

static void
_indicator_ecore_evas_msg_parent_handle(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
	DBG("Receive msg from clien msg_domain=%x msg_id=%x size=%d\n", msg_domain, msg_id, size);
	retif(!data, , "data is NULL");

	if (msg_domain == MSG_DOMAIN_CONTROL_INDICATOR)
	{
		struct appdata *ad = (struct appdata *)ecore_evas_data_get(ee,"indicator_app_data");

		if ((msg_id == MSG_ID_INDICATOR_ROTATION) && (size == sizeof(int)))
		{
			int *rot = data;
			DBG("Receive msg is portrait rotation . rot=%d \n", *rot);
			_rotate_window(ad,*rot);
		}
		if ((msg_id == MSG_ID_INDICATOR_OPACITY) && (size == sizeof(Elm_Win_Indicator_Opacity_Mode)))
		{
			Elm_Win_Indicator_Opacity_Mode *omod;
			int trans_mode;
			omod = data;
			DBG("Receive msg is opacity . opacity=%d \n", *omod);
			switch(*omod)
			{
				case ELM_WIN_INDICATOR_OPAQUE:
					trans_mode = INDICATOR_OPACITY_OPAQUE;
					break;
				case ELM_WIN_INDICATOR_TRANSLUCENT:
					trans_mode = INDICATOR_OPACITY_TRANSLUCENT;
					break;
				case ELM_WIN_INDICATOR_TRANSPARENT:
					trans_mode = INDICATOR_OPACITY_TRANSPARENT;
					break;
				default:
					trans_mode = INDICATOR_OPACITY_OPAQUE;
					break;
			}
		}
	}

}

static void
_indicator_port_ecore_evas_msg_parent_handle(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
	DBG("PORT : Receive msg from clien msg_domain=%x msg_id=%x size=%d\n", msg_domain, msg_id, size);
	retif(!data, , "data is NULL");

	_indicator_ecore_evas_msg_parent_handle(ee,msg_domain,msg_id,data,size);
}

static void
_indicator_land_ecore_evas_msg_parent_handle(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
	DBG("LAND : Receive msg from clien msg_domain=%x msg_id=%x size=%d\n", msg_domain, msg_id, size);
	retif(!data, , "data is NULL");

	_indicator_ecore_evas_msg_parent_handle(ee,msg_domain,msg_id,data,size);
}

static void _register_event_handler_both(win_info *win, void *data)
{
	Ecore_Evas *ee;

	retif(win == NULL, , "Invalid parameter!");

	ee = ecore_evas_ecore_evas_get(evas_object_evas_get(win->win_main));

	evas_object_smart_callback_add(win->win_main,
					       "delete,request",
					       _indicator_window_delete_cb, data);
	evas_object_event_callback_add(win->layout_main,
				       EVAS_CALLBACK_MOUSE_DOWN,
				       _indicator_mouse_down_cb, win);

	evas_object_event_callback_add(win->layout_main,
				       EVAS_CALLBACK_MOUSE_MOVE,
				       _indicator_mouse_move_cb, win);

	evas_object_event_callback_add(win->layout_main,
				       EVAS_CALLBACK_MOUSE_UP,
				       _indicator_mouse_up_cb, win);
	if(win->type == INDICATOR_WIN_PORT)
	{
		ecore_evas_callback_msg_parent_handle_set(ee, _indicator_port_ecore_evas_msg_parent_handle);
	}
	else
	{
		ecore_evas_callback_msg_parent_handle_set(ee, _indicator_land_ecore_evas_msg_parent_handle);
	}
	ecore_evas_data_set(ee,"indicator_app_data",data);

}

static void register_event_handler(void *data)
{
	int ret;
	int i = 0;
	struct appdata *ad = data;
	Ecore_Event_Handler *hdl = NULL;
	retif(data == NULL, , "Invalid parameter!");

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		_register_event_handler_both(&(ad->win[i]),data);
	}
	hdl = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
				      _indicator_client_message_cb, (void *)ad);
	retif(hdl == NULL, , "Failed to register ecore_event_handler!");
	ad->evt_handlers = eina_list_append(ad->evt_handlers, hdl);

	hdl = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
				      _property_changed_cb, data);
	retif(hdl == NULL, , "Failed to register ecore_event_handler!");
	ad->evt_handlers = eina_list_append(ad->evt_handlers, hdl);

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL,
		       _indicator_check_battery_percent_on_cb, (void *)ad);

	if (ret == -1) {
		ERR("noti start is failed\n");
		return;
	}

	if (vconf_notify_key_changed
	    (VCONFKEY_PM_STATE, _indicator_notify_pm_state_cb, (void *)ad) != 0) {
		ERR("Fail vconf_notify_key_changed : VCONFKEY_PM_STATE");
	}

	ret = vconf_notify_key_changed(VCONFKEY_APPTRAY_STATE,
		       _indicator_notify_apptray_state_cb, (void *)ad);

	if (ret == -1) {
		ERR("VCONFKEY_APPTRAY_STATE is failed\n");
		return;
	}

	ret = vconf_notify_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS,
		       _indicator_power_off_status_cb, (void *)ad);

	if (ret == -1) {
		ERR("VCONFKEY_SYSMAN_POWER_OFF_STATUS is failed\n");
		return;
	}

	ret = minicontrol_monitor_start(_mctrl_monitor_cb, data);
	if (ret != MINICONTROL_ERROR_NONE) {
		ERR("fail to minicontrol_monitor_start()- %d", ret);
		return;
	}

}

static void _unregister_event_handler_both(win_info *win)
{
	retif(win == NULL, , "Invalid parameter!");

	evas_object_smart_callback_del(win->win_main,
				       "delete-request",
				       _indicator_window_delete_cb);

	evas_object_event_callback_del(win->layout_main,
				       EVAS_CALLBACK_MOUSE_DOWN,
				       _indicator_mouse_down_cb);

	evas_object_event_callback_del(win->layout_main,
				       EVAS_CALLBACK_MOUSE_MOVE,
				       _indicator_mouse_move_cb);

	evas_object_event_callback_del(win->layout_main,
				       EVAS_CALLBACK_MOUSE_UP,
				       _indicator_mouse_up_cb);

}

static int unregister_event_handler(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	int i = 0;

	retif(data == NULL, FAIL, "Invalid parameter!");

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		_unregister_event_handler_both(&(ad->win[i]));
	}

	vconf_ignore_key_changed(VCONFKEY_SETAPPL_BATTERY_PERCENTAGE_BOOL,
				_indicator_check_battery_percent_on_cb);

	vconf_ignore_key_changed(VCONFKEY_PM_STATE,
				_indicator_notify_pm_state_cb);

	vconf_ignore_key_changed(VCONFKEY_APPTRAY_STATE,
				_indicator_notify_apptray_state_cb);

	vconf_ignore_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS,
				_indicator_power_off_status_cb);

	Ecore_Event_Handler *hdl = NULL;
	EINA_LIST_FREE(ad->evt_handlers, hdl) {
		if (hdl)
			ecore_event_handler_del(hdl);
	}

	minicontrol_monitor_stop();

	return OK;
}

static Evas_Object *load_edj(Evas_Object * parent, const char *file,
			     const char *group)
{
	Evas_Object *eo;
	int r;

	eo = elm_layout_add(parent);
	if (eo) {
		r = elm_layout_file_set(eo, file, group);
		if (!r) {
			evas_object_del(eo);
			return NULL;
		}

		evas_object_size_hint_weight_set(eo,
						 EVAS_HINT_EXPAND,
						 EVAS_HINT_EXPAND);
		elm_win_resize_object_add(parent, eo);
	}

	return eo;
}

static void create_win(void* data,int type)
{
	char *indi_name = NULL;

	struct appdata *ad = data;
	Ecore_X_Window xwin;
	Ecore_X_Window zone;
	Ecore_X_Window_State states[2];
	int root_w;
	int root_h;
	Ecore_X_Window root;

	root = ecore_x_window_root_first_get();
	ecore_x_window_size_get(root, &root_w, &root_h);
	INFO("xwin_size = %d %d", root_w, root_h);

	ad->scale = elm_config_scale_get();
	INFO("scale = %f", ad->scale);


	switch(type)
	{
	case INDICATOR_WIN_PORT:
		ad->win[type].win_main = elm_win_add(NULL, "portrait_indicator", ELM_WIN_SOCKET_IMAGE);
		indi_name = "elm_indicator_portrait";
		elm_win_title_set(ad->win[type].win_main, "win sock test:port");
		ad->win[type].w = root_w;
		break;
	case INDICATOR_WIN_LAND:
		ad->win[type].win_main = elm_win_add(NULL, "win_socket_test:land", ELM_WIN_SOCKET_IMAGE);
		indi_name = "elm_indicator_landscape";
		elm_win_title_set(ad->win[type].win_main, "win sock test:land");
		ad->win[type].w = root_h;
		break;
	default :
		break;
	}

	retif(ad->win[type].win_main == NULL, , "elm_win_add failed!");

	if (!elm_win_socket_listen(ad->win[type].win_main , indi_name, 0, EINA_FALSE))
	{
		printf("fail to elm_win_socket_listen():port \n");
		evas_object_del(ad->win[type].win_main);
		return;
	}
	elm_win_alpha_set(ad->win[type].win_main , EINA_TRUE);

	elm_win_borderless_set(ad->win[type].win_main , EINA_TRUE);

	evas_object_size_hint_fill_set(ad->win[type].win_main , EVAS_HINT_EXPAND,
				       EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->win[type].win_main , 1.0, 0.5);

	xwin = elm_win_xwindow_get(ad->win[type].win_main );
	ecore_x_icccm_hints_set(xwin, 0, 0, 0, 0, 0, 0, 0);
	states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
	states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
	ecore_x_netwm_window_state_set(xwin, states, 2);

	zone = ecore_x_e_illume_zone_get(xwin);
	ecore_x_event_mask_set(zone, ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);

	ad->win[type].evas = evas_object_evas_get(ad->win[type].win_main );

	switch(type)
	{
	case INDICATOR_WIN_PORT:
		ad->win[type].layout_main = load_edj(ad->win[type].win_main , EDJ_FILE0, GRP_MAIN);
		break;
	case INDICATOR_WIN_LAND:
		ad->win[type].layout_main = load_edj(ad->win[type].win_main , EDJ_FILE1, GRP_MAIN);
		break;
	default :
		break;
	}

	retif(ad->win[type].layout_main == NULL, , "Failed to get layout main!");

	ad->win[type].h = (int)(INDICATOR_HEIGHT * ad->scale);
	evas_object_resize(ad->win[type].win_main , ad->win[type].w, ad->win[type].h);
	evas_object_move(ad->win[type].win_main , 0, 0);
	INFO("win_size = %d, %d", ad->win[type].w, ad->win[type].h);

	ad->win[type].type = type;

#ifdef HOME_KEY_EMULATION
	int ret = 0;
	ad->win[type].atom_hwkey = ecore_x_atom_get(PROP_HWKEY_EMULATION);
	ret = ecore_x_window_prop_window_get(root, ad->win[type].atom_hwkey,
					&ad->win[type].win_hwkey, 1);
	if (ret <= 0)
		ERR("Failed to get window property ! (ret=%d)", ret);
#endif

	indicator_util_layout_add(&(ad->win[type]));

	ad->win[type].data = data;

	evas_object_show(ad->win[type].layout_main);
	evas_object_show(ad->win[type].win_main);
	return ;
}

static void create_overlay_win(void* data)
{

	struct appdata *ad = data;
	Evas_Object *conform = NULL;

	Evas_Object *eo;
	int w, h;
	int indi_h;
	Ecore_X_Window xwin;
	Ecore_X_Window zone;
	Ecore_X_Window_State states[2];
	Ecore_X_Atom ATOM_MV_INDICATOR_GEOMETRY = 0;

	indi_h = (int)(INDICATOR_HEIGHT * ad->scale);

	eo = elm_win_add(NULL, "INDICATOR", ELM_WIN_BASIC);
	elm_win_title_set(eo, "INDICATOR");
	elm_win_borderless_set(eo, EINA_TRUE);
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	evas_object_resize(eo, w, (int)(INDICATOR_HEIGHT * ad->scale));
	evas_object_move(eo , 0, 0);
	elm_win_alpha_set(eo, EINA_TRUE);

	elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_OPAQUE);

	conform = elm_conformant_add(eo);

	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND,EVAS_HINT_EXPAND);
	elm_win_resize_object_add(eo, conform);
	evas_object_show(conform);

	xwin = elm_win_xwindow_get(eo);
	ecore_x_icccm_hints_set(xwin, 0, 0, 0, 0, 0, 0, 0);
	states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
	states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
	ecore_x_netwm_window_state_set(xwin, states, 2);

	ecore_x_icccm_name_class_set(xwin, "INDICATOR", "INDICATOR");

	ecore_x_netwm_window_type_set(xwin, ECORE_X_WINDOW_TYPE_DOCK);

	unsigned int   ind_gio_val[16] = { 0, 0, w, indi_h,
                              0, 0, indi_h, h,
                              0, h-indi_h, w, indi_h,
                              w-indi_h, 0, indi_h, h };

	ATOM_MV_INDICATOR_GEOMETRY = ecore_x_atom_get(STR_ATOM_MV_INDICATOR_GEOMETRY);

	ecore_x_window_prop_card32_set(xwin,
					  ATOM_MV_INDICATOR_GEOMETRY,
					  ind_gio_val,
					  16);

	zone = ecore_x_e_illume_zone_get(xwin);
	ecore_x_event_mask_set(zone, ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
	evas_object_show(eo);

	ad->win_overlay = eo;

	return ;
}

static void _indicator_init_wininfo(void * data)
{
	int i = 0;
	struct appdata *ad = data;
	retif(data == NULL, , "Invalid parameter!");

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		memset(&(ad->win[i]),0x00,sizeof(win_info));
	}

	ad->win_overlay = NULL;
}

static int indicator_window_new(void *data)
{
	int i = 0;
	struct appdata *ad = data;

	retif(data == NULL, FAIL, "Invalid parameter!");

	_indicator_init_wininfo(data);

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		create_win(data,i);
		indicator_util_show_hide_icons(&(ad->win[i]),0,0);
	}

	create_overlay_win(data);
	register_event_handler(ad);

	return OK;
}

static int indicator_window_del(void *data)
{
	int i = 0;
	struct appdata *ad = (struct appdata *)data;

	retif(data == NULL, FAIL, "Invalid parameter!");

	indicator_fini_modules(data);
	unregister_event_handler(ad);

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		indicator_util_layout_del(&(ad->win[i]));
		evas_image_cache_flush(ad->win[i].evas);
		evas_object_del(ad->win[i].layout_main);
		ad->win[i].layout_main = NULL;
		evas_object_del(ad->win[i].win_main);
		ad->win[i].win_main = NULL;
	}

	evas_object_del(ad->win_overlay);
	ad->win_overlay = NULL;

	if (ad)
		free(ad);

	elm_exit();
	return OK;
}

static inline int _indicator_home_icon_action(void *data, int press)
{
	win_info *win = NULL;
	int ret = -1;
	const char *signal = NULL;

	retif(!data, ret, "data is NULL");
	win = (win_info*)data;
	retif(!win->layout_main, ret, "ad->layout_main is NULL");

	if (press)
		signal = "home.pressed";
	else
		signal = "home.released";

	if (!ret)
		elm_object_signal_emit(win->layout_main,
				signal, "indicator.prog");

	return ret;
}

#ifdef HOME_KEY_EMULATION
static Eina_Bool _indicator_hw_home_key_press(void *data)
{
	win_info *win = NULL;
	char message[20] = {'\0', };

	retif(!data, EINA_FALSE, "data is NULL");

	win = (win_info *)data;

	retif(!win->win_hwkey, EINA_FALSE, "window for hw emulation is NULL");

	snprintf(message, sizeof(message), "%s%s",
			KEY_MSG_PREFIX_PRESS, KEY_HOME);

	return ecore_x_client_message8_send(win->win_hwkey, win->atom_hwkey,
		message, strlen(message));
}

static Eina_Bool _indicator_hw_home_key_release(void *data)
{
	char message[20] = {'\0', };
	win_info *win = (win_info*)data;

	retif(!data, EINA_FALSE, "data is NULL");

	retif(!win->win_hwkey, EINA_FALSE, "window for hw emulation is NULL");


	snprintf(message, sizeof(message), "%s%s",
			KEY_MSG_PREFIX_RELEASE, KEY_HOME);

	return ecore_x_client_message8_send(win->win_hwkey, win->atom_hwkey,
		message, strlen(message));
}
static Eina_Bool _indicator_hw_home_key_press_cancel(void *data)
{
	win_info *win = (win_info*)data;
	char message[20] = {'\0', };

	retif(!data, EINA_FALSE, "data is NULL");

	retif(!win->win_hwkey, EINA_FALSE, "window for hw emulation is NULL");

	snprintf(message, sizeof(message), "%s%s",
			KEY_MSG_PREFIX_PRESS_C, KEY_HOME);

	return ecore_x_client_message8_send(win->win_hwkey, win->atom_hwkey,
		message, strlen(message));
}

static Eina_Bool _indicator_hw_home_key_release_cancel(void *data)
{
	win_info *win = (win_info*)data;
	char message[20] = {'\0', };

	retif(!data, EINA_FALSE, "data is NULL");

	retif(!win->win_hwkey, EINA_FALSE, "window for hw emulation is NULL");


	snprintf(message, sizeof(message), "%s%s",
			KEY_MSG_PREFIX_RELEASE_C, KEY_HOME);

	return ecore_x_client_message8_send(win->win_hwkey, win->atom_hwkey,
		message, strlen(message));
}

#endif

static void __indicator_launch_apptray(void* data)
{
	int lock_state = VCONFKEY_IDLE_UNLOCK;
	int lock_ret = -1;
	service_h service;
	int ret = SERVICE_ERROR_NONE;

	lock_ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE,
			&lock_state);
	DBG("Check Lock State : %d %d", lock_ret, lock_state);

	if (lock_ret == 0 && lock_state == VCONFKEY_IDLE_LOCK)
	{
		goto __CATCH;
	}

	INFO("[Home Button Released]");

	if (check_system_status() == FAIL)
	{
		INFO("check_system_status failed");
		goto __CATCH;
	}

	service_create(&service);

	service_set_operation(service, SERVICE_OPERATION_DEFAULT);

	service_set_app_id(service, APP_TRAY_PKG_NAME);

	service_add_extra_data(service, "LONG_PRESS", "0");

	ret = service_send_launch_request(service, NULL, NULL);

	if(ret != SERVICE_ERROR_NONE)
	{
		ERR("Cannot launch app");
	}

	service_destroy(service);

__CATCH:
	_indicator_home_icon_action(data, 0);
	home_button_pressed = EINA_FALSE;
}

static void __indicator_hide_icon_timer_cb(void* data)
{
	if (hide_timer != NULL) {
		ecore_timer_del(hide_timer);
		hide_timer = NULL;
	}

	indicator_util_show_hide_icons(data,0,1);
	indicator_send_evas_ecore_message(data,1,2);

}

static void _indicator_mouse_down_cb(void *data, Evas * e, Evas_Object * obj,
				     void *event)
{
	win_info *win = (win_info*)data;
	Evas_Event_Mouse_Down *ev = NULL;

	retif(data == NULL || event == NULL, , "Invalid parameter!");
	ev = event;

	win->mouse_event.x = ev->canvas.x;
	win->mouse_event.y = ev->canvas.y;

	DBG("_indicator_mouse_down_cb : %d %d", ev->canvas.x, ev->canvas.y);

#ifdef HOME_KEY_EMULATION
	if(indicator_util_check_indicator_area(win, ev->canvas.x, ev->canvas.y))
	{
		if( is_quickpanel_opened == 0 )
		{
			show_hide_pressed[win->type] = EINA_TRUE;
		}
		else
		{
			show_hide_pressed[win->type] = EINA_FALSE;
			DBG("quick panel is opened : %d", is_quickpanel_opened);
		}
		indicator_press_coord.x = ev->canvas.x;
		indicator_press_coord.y = ev->canvas.y;
	}

__CATCH :
	return;
#else
	int mouse_down_prio = -1;
	mouse_down_prio =
		indicator_util_get_priority_in_move_area(win, win->mouse_event.x,
							win->mouse_event.y);

	if (mouse_down_prio > -1) {
		switch (mouse_down_prio) {
		case INDICATOR_PRIORITY_FIXED1:
		{
			int lock_state = VCONFKEY_IDLE_UNLOCK;
			int lock_ret = -1;

			lock_ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE,
					&lock_state);
			DBG("Check Lock State : %d %d", lock_ret, lock_state);

			if (lock_ret == 0
				&& lock_state == VCONFKEY_IDLE_UNLOCK) {
				if (!_indicator_home_icon_action(win, 1))
					home_button_pressed = EINA_TRUE;
			}
		}
		break;
		}
	}
#endif
}

static void _indicator_mouse_move_cb(void *data, Evas * e, Evas_Object * obj,
				     void *event)
{
	Evas_Event_Mouse_Move *ev = NULL;
	win_info* win = (win_info*)data;

	retif(data == NULL || event == NULL, , "Invalid parameter!");

	ev = event;

	if(win->type == INDICATOR_WIN_PORT)
	{
		if (show_hide_pressed[win->type]) {
			if (!indicator_util_check_indicator_area(win,ev->cur.canvas.x,ev->cur.canvas.y)
			|| indicator_util_check_home_icon_area(win,ev->cur.canvas.x,ev->cur.canvas.y))
			{
				show_hide_pressed[win->type] = FALSE;
				DBG("cancel show/hide key");
			}
		}
	}
	else
	{
		if(ev->cur.canvas.y - indicator_press_coord.y >= INDICATOR_HIDE_TRIGER_H*elm_config_scale_get())
		{
			DBG("ev->cur.canvas.x(%d) ev->cur.canvas.y(%d)",ev->cur.canvas.x,ev->cur.canvas.y);
			DBG("indicator_press_coord.x(%d) indicator_press_coord.y(%d)",indicator_press_coord.x,indicator_press_coord.y);

			if(show_hide_pressed[win->type] == EINA_TRUE)
			{
				if(indicator_icon_show_state[win->type] == 0)
				{
					indicator_util_show_hide_icons(win,1,1);
					indicator_send_evas_ecore_message(win,0,1);
					if (hide_timer != NULL) {
						ecore_timer_del(hide_timer);
						hide_timer = NULL;
					}

					hide_timer =  ecore_timer_add(3, (void *)__indicator_hide_icon_timer_cb,data);

					show_hide_pressed[win->type] = EINA_FALSE;
				}
			}
		}
	}
}

static void _indicator_mouse_up_cb(void *data, Evas * e, Evas_Object * obj,
				   void *event)
{

	Evas_Event_Mouse_Up *ev = NULL;
	win_info *win = (win_info *)data;

	retif(data == NULL || event == NULL, , "Invalid parameter!");

	ev = event;

	DBG("_indicator_mouse_up_cb : %d %d", ev->canvas.x, ev->canvas.y);

#ifdef HOME_KEY_EMULATION
	if(indicator_util_check_indicator_area(win, ev->canvas.x, ev->canvas.y))
	{

		if(win->type == INDICATOR_WIN_PORT)
		{
			if(show_hide_pressed[win->type] == EINA_TRUE)
			{
				if(indicator_icon_show_state[win->type] == 0)
				{
					indicator_icon_backup_state[win->type] = 1;
					indicator_util_show_hide_icons(win,1,1);
				}
				else
				{
					indicator_icon_backup_state[win->type] = 0;
					indicator_util_show_hide_icons(win,0,1);
				}

				vconf_set_int(VCONFKEY_BATTERY_DISP_STATE,win->type);
				feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);
			}
		}

	}

	home_button_pressed = EINA_FALSE;
	show_hide_pressed[win->type] = EINA_FALSE;
#else
	int mouse_up_prio = -1;
	int mouse_down_prio = -1;

	if (home_button_pressed == EINA_TRUE) {
		_indicator_home_icon_action(data, 0);
		home_button_pressed = EINA_FALSE;
	}

	mouse_down_prio =
		indicator_util_get_priority_in_move_area(win,win->mouse_event.x,
							win->mouse_event.y);
	mouse_up_prio = indicator_util_get_priority_in_move_area(win,ev->canvas.x,
							ev->canvas.y);

	if (mouse_down_prio > -1 && mouse_up_prio > -1
		&& mouse_down_prio == mouse_up_prio) {
		switch (mouse_down_prio) {
		case INDICATOR_PRIORITY_FIXED1:
		{
			int lock_state = VCONFKEY_IDLE_UNLOCK;
			int lock_ret = -1;

			lock_ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE,
					&lock_state);
			DBG("Check Lock State : %d %d", lock_ret, lock_state);

			if (lock_ret == 0 && lock_state == VCONFKEY_IDLE_LOCK)
				break;

			char *package = NULL;
			char *top_win_name = NULL;

			INFO("[Home Button Released]");

			if (check_system_status() == FAIL)
				break;

			package = vconf_get_str("db/menuscreen/pkgname");
			if (package) {
				service_h service;
				int ret = SERVICE_ERROR_NONE;

				service_create(&service);

				service_set_operation(service, SERVICE_OPERATION_DEFAULT);

				service_set_package(service, package);

				top_win_name = _get_top_window_name(data);

				if (top_win_name != NULL
					&& !strncmp(top_win_name, package,
					strlen(package)))
				{

					DBG("service_send_launch_request : %s",
						top_win_name);

					ret = service_send_launch_request(service, NULL, NULL);

					if(ret != SERVICE_ERROR_NONE)
					{
						ERR("Cannot launch app");
					}

				}
				else
				{
					DBG("app_manager_resume_app : %s",
						top_win_name);

					ret = app_manager_resume_app(service);
					if(ret != APP_MANAGER_ERROR_NONE)
					{
						ERR("Cannot resume app");
					}
				}

				if (top_win_name)
					free(top_win_name);

				free(package);

				service_destroy(service);

			} else
				ERR("Cannot get vconf");
		}
		break;
		}
	}
#endif
	win->mouse_event.x = 0;
	win->mouse_event.y = 0;
}

static int register_indicator_modules(void *data)
{
	indicator_init_modules(data);
	return OK;
}

static void _signal_handler(int signum, siginfo_t *info, void *unused)
{
    DBG("_signal_handler : Terminated...");
    app_efl_exit();
}

static bool app_create(void *data)
{
	pid_t pid;
	int ret;

	struct sigaction act;
	memset(&act,0x00,sizeof(struct sigaction));
	act.sa_sigaction = _signal_handler;
	act.sa_flags = SA_SIGINFO;

	ret = sigemptyset(&act.sa_mask);
	if (ret < 0) {
		ERR("Failed to sigemptyset[%s]", strerror(errno));
	}
	ret = sigaddset(&act.sa_mask, SIGTERM);
	if (ret < 0) {
		ERR("Failed to sigaddset[%s]", strerror(errno));
	}
	ret = sigaction(SIGTERM, &act, NULL);
	if (ret < 0) {
		ERR("Failed to sigaction[%s]", strerror(errno));
	}

	pid = setsid();
	if (pid < 0)
		fprintf(stderr, "[INDICATOR] Failed to set session id!");

	ret = nice(2);
	if (ret == -1)
		ERR("Failed to set nice value!");
	return true;
}

static void app_terminate(void *data)
{
	int i = 0;
	struct appdata *ad = data;
	indicator_fini_modules(data);
	unregister_event_handler(ad);

	feedback_deinitialize();

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		indicator_util_layout_del(&(ad->win[i]));
		evas_image_cache_flush(ad->win[i].evas);
		evas_object_del(ad->win[i].layout_main);
		evas_object_del(ad->win[i].win_main);
	}

	INFO(" >>>>>>>>>>>>> INDICATOR IS TERMINATED!! <<<<<<<<<<<<<< ");

}

static void app_pause(void *data)
{

}

static void app_resume(void *data)
{

}

static void app_service(service_h service, void *data)
{
	struct appdata *ad = data;
	int ret;
	int i = 0;

	INFO("[INDICATOR IS STARTED]");
	ret = indicator_window_new(data);
	retif(ret != OK, , "Failed to create a new window!");

	_change_view(ecore_x_window_root_first_get(), data);

	_indicator_check_battery_percent_on_cb(NULL, data);
	register_indicator_modules(data);

	feedback_initialize();

	for(i=0;i<INDICATOR_WIN_MAX;i++)
	{
		if (ad->win[i].win_main)
			elm_win_activate(ad->win[i].win_main);
	}

}

int main(int argc, char *argv[])
{

	struct appdata ad;

	app_event_callback_s event_callback;

	int ret = 0;

	DBG("Start indicator");

	ret = control_privilege();
	if (ret != 0) {
		ERR("[INDICATOR] Failed to control privilege!");
	}

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.service = app_service;
	event_callback.low_memory = NULL;
	event_callback.low_battery = _indicator_low_bat_cb;
	event_callback.device_orientation = NULL;
	event_callback.language_changed = _indicator_lang_changed_cb;
	event_callback.region_format_changed = _indicator_region_changed_cb;

	memset(&ad, 0x0, sizeof(struct appdata));

	return app_efl_main(&argc, &argv, &event_callback, &ad);

}
