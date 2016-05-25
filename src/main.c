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
#include <app.h>
#include <unistd.h>
#include <app_manager.h>
#include <signal.h>
#include <feedback.h>
#include <notification.h>
#include <app_preference.h>
#include <wifi.h>
#include <device/display.h>
#include <device/callback.h>
#include <system_settings.h>
#include <runtime_info.h>
//FIXME
#if 0
#include <tzsh_indicator_service.h>
#endif

#include "common.h"
#include "box.h"
#include "icon.h"
#include "main.h"
#include "indicator_gui.h"
#include "modules.h"
#include "util.h"
#include "toast_popup.h"
#include "tts.h"
#include "log.h"
#include "indicator.h"
#include "ticker.h"

#define GRP_NAME "indicator"
#define WIN_TITLE "Illume Indicator"
#define VCONF_PHONE_STATUS "memory/startapps/sequence"

#define BUS_NAME       "org.tizen.system.deviced"
#define PATH_NAME    "/Org/Tizen/System/DeviceD/Display"
#define INTERFACE_NAME BUS_NAME".display"
#define MEMBER_NAME	"LCDOn"

#define MP_APP_ID "org.tizen.music-player-lite"
#define FMRADIO_APP_ID "org.tizen.fm-radio-lite"
#define VR_APP_ID "org.tizen.voicerecorder-lite"

#define STR_ATOM_MV_INDICATOR_GEOMETRY          "_E_MOVE_INDICATOR_GEOMETRY"

#define HIBERNATION_ENTER_NOTI	"HIBERNATION_ENTER"
#define HIBERNATION_LEAVE_NOTI	"HIBERNATION_LEAVE"

#define UNLOCK_ENABLED	0
#define TIMEOUT			5

#define ERROR_MESSAGE_LEN 256

#ifdef HOME_KEY_EMULATION
/* Predefine string */
#define PROP_HWKEY_EMULATION "_HWKEY_EMULATION"
#define KEY_MSG_PREFIX_PRESS "P:"
#define KEY_MSG_PREFIX_RELEASE "R:"
#define KEY_MSG_PREFIX_PRESS_C "PC"
#define KEY_MSG_PREFIX_RELEASE_C "RC"

#ifndef KEY_HOME
#define KEY_HOME "XF86Phone"
#endif /* KEY_HOME */
#endif /* HOME_KEY_EMULATION */

//static E_DBus_Connection *edbus_conn=NULL;
//static E_DBus_Signal_Handler *edbus_handler=NULL;
static Eina_Bool home_button_pressed = EINA_FALSE;
static Eina_Bool show_hide_pressed = EINA_FALSE;
Evas_Coord_Point indicator_press_coord = {0,0};
Ecore_Timer *clock_timer;
int is_transparent = 0;
int current_angle = 0;
int current_state = 0;

int isSimShowing;	// For Simcard Icon
int isRSSI1Showing;	// For RSSI1 Icon
int isRSSI2Showing;	// For RSSI2 Icon
int isSilentShowing;		// For Silent Icon
int isWifiDirectShowing;	// For WiFi Direct Icon

#if 0
static int bFirst_opacity = 1;
#endif

static struct _s_info {
	Ecore_Timer *listen_timer;
} s_info = {
	.listen_timer = NULL,
};


static indicator_error_e _start_indicator(void *data);
static indicator_error_e _terminate_indicator(void *data);

static void _indicator_low_bat_cb(app_event_info_h event_info, void *data);
static void _indicator_lang_changed_cb(app_event_info_h event_info, void *data);
static void _indicator_region_changed_cb(app_event_info_h event_info, void *data);
static void _indicator_window_delete_cb(void *data, Evas_Object * obj, void *event);
//static Eina_Bool _indicator_client_message_cb(void *data, int type, void *event);
static void _indicator_mouse_down_cb(void *data, Evas * e, Evas_Object * obj, void *event);
static void _indicator_mouse_move_cb(void *data, Evas * e, Evas_Object * obj, void *event);
static void _indicator_mouse_up_cb(void *data, Evas * e, Evas_Object * obj, void *event);


static void _indicator_low_bat_cb(app_event_info_h event_info, void *data)
{
}

static void _indicator_lang_changed_cb(app_event_info_h event_info, void *data)
{
	modules_lang_changed(data);
}

static void _indicator_region_changed_cb(app_event_info_h event_info, void *data)
{
	modules_region_changed(data);
}

static void _indicator_window_delete_cb(void *data, Evas_Object * obj, void *event)
{
	ret_if(!data);

	_terminate_indicator((struct appdata *)data);
}

#define SIGNAL_NAME_LEN 30
static void _indicator_notify_pm_state_cb(device_callback_e type, void *value, void *user_data)
{
	display_state_e state;

	ret_if(!user_data);

	if (type != DEVICE_CALLBACK_DISPLAY_STATE)
		return;

	state = (display_state_e)value;

	switch (state) {
	case DISPLAY_STATE_SCREEN_OFF:
		if (clock_timer != NULL) {
			ecore_timer_del(clock_timer);
			clock_timer = NULL;
		}
		icon_set_update_flag(0);
		box_noti_ani_handle(0);
		break;
	case DISPLAY_STATE_SCREEN_DIM:
		icon_set_update_flag(0);
		box_noti_ani_handle(0);
		break;
	case DISPLAY_STATE_NORMAL:
		if (!icon_get_update_flag()) {
			icon_set_update_flag(1);
			box_noti_ani_handle(1);
			modules_wake_up(user_data);
		}
		break;
	default:
		break;
	}
}

static void _indicator_lock_status_cb(system_settings_key_e key, void *data)
{
	static int lockstate = 0;
	extern int clock_mode;
	int val = -1;

	ret_if(!data);

	int err = system_settings_get_value_int(SYSTEM_SETTINGS_KEY_LOCK_STATE, &val);
	if (err != SYSTEM_SETTINGS_ERROR_NONE) {
		_E("system_settings_get_value_int failed: %s", get_error_message(err));
		return;
	}

	if (val == lockstate) return;
	lockstate = val;

	switch (val) {
	case SYSTEM_SETTINGS_LOCK_STATE_UNLOCK:
		if (!clock_mode) util_signal_emit(data,"clock.font.12","indicator.prog");
		else util_signal_emit(data,"clock.font.24","indicator.prog");
		break;
	case SYSTEM_SETTINGS_LOCK_STATE_LAUNCHING_LOCK:
	case SYSTEM_SETTINGS_LOCK_STATE_LOCK:
		util_signal_emit(data,"clock.invisible","indicator.prog");
		break;
	default:
		break;
	}
}

#if 0
static void _rotate_window(struct appdata *ad, int new_angle)
{
	ret_if(!ad);

	_D("Indicator angle is %d degree", new_angle);

	current_angle = new_angle;

	switch (new_angle) {
		case 0:
		case 180:
			evas_object_resize(ad->win.win, ad->win.port_w, ad->win.h);
			break;
		case 90:
		case 270:
			evas_object_resize(ad->win.win, ad->win.land_w, ad->win.h);
			break;
		default:
			break;
	}
}
#endif

#if 0
static void _change_opacity(void *data, enum indicator_opacity_mode mode)
{
	struct appdata *ad = NULL;
	const char *signal = NULL;
	retm_if(data == NULL, "Invalid parameter!");

	ad = data;

	if (bFirst_opacity==1) bFirst_opacity = 0;

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
		_E("unknown mode : %d", mode);
		signal = "bg.opaque";
		ad->opacity_mode = INDICATOR_OPACITY_OPAQUE;
		break;

	}
	util_signal_emit_by_win(&(ad->win),signal, "indicator.prog");
}
#endif

#if 0
static void _indicator_quickpanel_changed(void *data, int is_open)
{
	int val = 0;

	ret_if(!data);

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) return;
	if (val == VCONFKEY_IDLE_LOCK) return;
}
#endif

#if 0
static Eina_Bool _indicator_client_message_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message *) event;
	struct appdata *ad = NULL;
	ad = data;

	retv_if(data == NULL || event == NULL, ECORE_CALLBACK_RENEW);
	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) {
		if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON) {
			_indicator_quickpanel_changed(data, 1);
		} else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF) {
			_indicator_quickpanel_changed(data, 0);
		}
	}

	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE) {
		int new_angle = 0;
		if(ev->win != ad->active_indi_win) {
			return ECORE_CALLBACK_RENEW;
		}
		new_angle = ev->data.l[0];
		_rotate_window(ad, new_angle);
	}
	return EINA_TRUE;
}
#endif

/* this function will be reused */
#if 0
static Eina_Bool _active_indicator_handle(void* data,int type)
{
	int trans_mode = 0;
	int angle = 0;
//	Ecore_X_Illume_Indicator_Opacity_Mode illume_opacity = 0;

	retv_if(!data, EINA_FALSE);

	struct appdata *ad = (struct appdata *)data;
	switch (type) {
	/* Opacity */
	case 1:
		illume_opacity = ecore_x_e_illume_indicator_opacity_get(ad->active_indi_win);

		switch(illume_opacity) {
		case ECORE_X_ILLUME_INDICATOR_OPAQUE:
			trans_mode = INDICATOR_OPACITY_OPAQUE;
			break;
		case ECORE_X_ILLUME_INDICATOR_TRANSLUCENT:
			trans_mode = INDICATOR_OPACITY_TRANSLUCENT;
			break;
		case ECORE_X_ILLUME_INDICATOR_TRANSPARENT:
			trans_mode = INDICATOR_OPACITY_TRANSPARENT;
			break;
		default:
			trans_mode = INDICATOR_OPACITY_OPAQUE;
			break;
		}

		_change_opacity(ad, trans_mode);
		break;
	/* Rotate */
	case 2:
		util_win_prop_angle_get(ad->active_indi_win, &angle);
		_rotate_window(ad, angle);
		break;
	default :
		break;
	}

	return EINA_TRUE;
}
#endif

/* this function will be reused */
#if 0
static Eina_Bool _property_changed_cb(void *data, int type, void *event)
{
	struct appdata *ad = data;
	Ecore_X_Event_Window_Property *ev = event;

	retv_if(!ad, ECORE_CALLBACK_PASS_ON);
	retv_if(!ev, ECORE_CALLBACK_PASS_ON);

	if (ev->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE) {
		if (ev->win == ad->active_indi_win) {
			_active_indicator_handle(data, 2);
		}
	} else if (ev->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_OPACITY_MODE) {
		if (ev->win == ad->active_indi_win) {
			_active_indicator_handle(data, 1);
		}
	} else if (ev->atom == ad->atom_active) {
		int ret = 0;

		Ecore_X_Window active_win;

		ret = ecore_x_window_prop_window_get(ecore_x_window_root_first_get(), ad->atom_active, &(active_win), 1);
		if (ret <= -1) {
			_E("Count of fetched items : %d", ret);
			return ECORE_CALLBACK_PASS_ON;
		}

		if (active_win != ad->active_indi_win) {
			if (ad->active_indi_win != -1) {
				ecore_x_window_unsniff(ad->active_indi_win);
				_D("UNSNIFF API %x", ad->active_indi_win);
			}
			ad->active_indi_win = active_win;

			ecore_x_window_sniff(ad->active_indi_win);
		}

		_active_indicator_handle(data, 1);
		_active_indicator_handle(data, 2);
	}

	return ECORE_CALLBACK_PASS_ON;
}
#endif

#if 0
static void _mctrl_monitor_cb(minicontrol_action_e action, const char *name, unsigned int width, unsigned int height, minicontrol_priority_e priority, void *data)
{
	ret_if(!data);
	ret_if(!name);

	modules_minictrl_control(action,name,data);
}
#endif

static void _indicator_ecore_evas_msg_parent_handle(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
	ret_if(!data);

#ifdef _SUPPORT_SCREEN_READER
	if (msg_domain == MSG_DOMAIN_CONTROL_ACCESS) {
		struct appdata *ad = (struct appdata *)ecore_evas_data_get(ee,"indicator_app_data");

		ret_if(!ad);

		Elm_Access_Action_Info *action_info;
		Evas_Object* win = NULL;
		action_info = data;

		win = ad->win.win;

		if (msg_id == ELM_ACCESS_ACTION_ACTIVATE) {
			elm_access_action(win, action_info->action_type,action_info);
		} else if (msg_id == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT) {
			action_info->highlight_cycle = EINA_TRUE;
			elm_access_action(win,action_info->action_type,action_info);
		} else if (msg_id == ELM_ACCESS_ACTION_HIGHLIGHT_PREV) {
			action_info->highlight_cycle = EINA_TRUE;
			elm_access_action(win,action_info->action_type,action_info);
		} else if (msg_id == ELM_ACCESS_ACTION_UNHIGHLIGHT) {
			elm_access_action(win,action_info->action_type,action_info);
		} else if (msg_id == ELM_ACCESS_ACTION_READ) {
			elm_access_action(win,action_info->action_type,action_info);
		}
	}
#endif /* _SUPPORT_SCREEN_READER */
}

#if 0
static void on_changed_receive(void *data, DBusMessage *msg)
{
	int r;

	r = dbus_message_is_signal(msg, INTERFACE_NAME, MEMBER_NAME);
	ret_if(!r);

	_D("LCD On handling");

	if (!icon_get_update_flag()) {
		icon_set_update_flag(1);
		box_noti_ani_handle(1);
		modules_wake_up(data);
	}
}

static void edbus_cleaner(void)
{
	if (!edbus_conn) {
		_D("already unregistered");
		return;
	}

	if (edbus_handler) {
		e_dbus_signal_handler_del(edbus_conn, edbus_handler);
		edbus_handler = NULL;
	}

	if (edbus_conn) {
		e_dbus_connection_close(edbus_conn);
		edbus_conn = NULL;
	}

	e_dbus_shutdown();
}

static int edbus_listener(void* data)
{
	if (edbus_conn != NULL) {
		_D("alreay exist");
		return -1;
	}

	e_dbus_init();

	edbus_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (!edbus_conn) {
		_E("e_dbus_bus_get error");
		return -1;
	}

	edbus_handler = e_dbus_signal_handler_add(edbus_conn, NULL, PATH_NAME, INTERFACE_NAME, MEMBER_NAME, on_changed_receive, data);
	if (!edbus_handler) {
		_E("e_dbus_signal_handler_add error");
		return -1;
	}

	return 0;
}
#endif

static void _register_event_handler_both(win_info *win, void *data)
{
	Ecore_Evas *ee;

	ret_if(!win);

	ee = ecore_evas_ecore_evas_get(evas_object_evas_get(win->win));

	evas_object_smart_callback_add(win->win,"delete,request", _indicator_window_delete_cb, data);
	evas_object_event_callback_add(win->layout, EVAS_CALLBACK_MOUSE_DOWN, _indicator_mouse_down_cb, win);
	evas_object_event_callback_add(win->layout, EVAS_CALLBACK_MOUSE_MOVE, _indicator_mouse_move_cb, win);
	evas_object_event_callback_add(win->layout, EVAS_CALLBACK_MOUSE_UP,_indicator_mouse_up_cb, win);
	ecore_evas_callback_msg_parent_handle_set(ee, _indicator_ecore_evas_msg_parent_handle);
	ecore_evas_data_set(ee,"indicator_app_data",data);
}

/* FIXME */
#if 0
static void _indicator_service_cb(void *data, tzsh_indicator_service_h service, int angle, int opacity)
{
	_D("Indicator service callback");
}
#endif

static void register_event_handler(void *data)
{
	struct appdata *ad = data;
//	Ecore_Event_Handler *hdl = NULL;
	ret_if(!data);

	ad->active_indi_win = -1;
	//ad->atom_active = ecore_x_atom_get("_NET_ACTIVE_WINDOW");
	//ecore_x_window_sniff(ecore_x_window_root_first_get());

	_register_event_handler_both(&(ad->win),data);

	/* FIXME */
#if 0
	if (ad->indicator_service) {
		tzsh_indicator_service_property_change_cb_set(ad->indicator_service, _indicator_service_cb, NULL);
	}
#endif

#if 0
	hdl = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _indicator_client_message_cb, (void *)ad);
	ret_if(!hdl);
	ad->evt_handlers = eina_list_append(ad->evt_handlers, hdl);

	hdl = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _property_changed_cb, data);
	ret_if(!hdl);
	ad->evt_handlers = eina_list_append(ad->evt_handlers, hdl);
#endif
	int err = device_add_callback(DEVICE_CALLBACK_DISPLAY_STATE, _indicator_notify_pm_state_cb, ad);
	if (err != DEVICE_ERROR_NONE) {
		_E("device_add_callback failed: %s", get_error_message(err));
	}

	if (util_system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCK_STATE, _indicator_lock_status_cb, ad)) {
		_E("util_system_settings_set_changed_cb failed");
	}

//	edbus_listener(data);
}

static void _unregister_event_handler_both(win_info *win)
{
	ret_if(!win);

	evas_object_smart_callback_del(win->win, "delete-request", _indicator_window_delete_cb);
	evas_object_event_callback_del(win->layout, EVAS_CALLBACK_MOUSE_DOWN, _indicator_mouse_down_cb);
	evas_object_event_callback_del(win->layout, EVAS_CALLBACK_MOUSE_MOVE, _indicator_mouse_move_cb);
	evas_object_event_callback_del(win->layout, EVAS_CALLBACK_MOUSE_UP, _indicator_mouse_up_cb);
}

static int unregister_event_handler(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	retv_if(!data, 0);

	_unregister_event_handler_both(&(ad->win));

	device_remove_callback(DEVICE_CALLBACK_DISPLAY_STATE, _indicator_notify_pm_state_cb);
	util_system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCK_STATE, _indicator_lock_status_cb);

	Ecore_Event_Handler *hdl = NULL;
	EINA_LIST_FREE(ad->evt_handlers, hdl) {
		if (hdl) ecore_event_handler_del(hdl);
	}

//	edbus_cleaner();

	return OK;
}

static void _create_layout(struct appdata *ad, const char *file, const char *group)
{
	ad->win.layout = elm_layout_add(ad->win.win);
	ret_if(!ad->win.layout);

	if (EINA_FALSE == elm_layout_file_set(ad->win.layout, file, group)) {
		_E("Failed to set file of layout");
		evas_object_del(ad->win.layout);
		return;
	}

	evas_object_size_hint_min_set(ad->win.layout, ad->win.w, ad->win.h);
	/* FIXME */
	evas_object_size_hint_weight_set(ad->win.layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win.win, ad->win.layout);
	evas_object_move(ad->win.layout, 0, 0);
	evas_object_show(ad->win.layout);
}

static void _create_box(win_info *win)
{
	ret_if(!win);

	/* First, clear layout */
	box_fini(win);

	box_init(win);

	return;
}

//FIXME
#if 0
static indicator_error_e _tzsh_set(struct appdata* ad)
{
	tzsh_window tz_win;

	retv_if(!ad, INDICATOR_ERROR_INVALID_PARAMETER);
	retv_if(!ad->win.win, INDICATOR_ERROR_INVALID_PARAMETER);

	ad->tzsh = tzsh_create(TZSH_TOOLKIT_TYPE_EFL);
	retv_if(!ad->tzsh, INDICATOR_ERROR_FAIL);

	tz_win = elm_win_window_id_get(ad->win.win);
	if (!tz_win) {
		tzsh_destroy(ad->tzsh);
		_E("Failed to get Tizen window");
		/* FIXME */
		//return INDICATOR_ERROR_FAIL;
		return INDICATOR_ERROR_NONE;
	}

	ad->indicator_service = tzsh_indicator_service_create(ad->tzsh, tz_win);
	if (!ad->indicator_service) {
		tzsh_destroy(ad->tzsh);
		_E("Failed to create Tizen window indicator service");
		return INDICATOR_ERROR_FAIL;
	}

	return INDICATOR_ERROR_NONE;
}

static void _tzsh_unset(struct appdata *ad)
{
	ret_if(!ad);

	if (ad->indicator_service) {
		tzsh_indicator_service_destroy(ad->indicator_service);
		ad->indicator_service = NULL;
	}

	if (ad->tzsh) {
		tzsh_destroy(ad->tzsh);
		ad->tzsh = NULL;
	}
}
#endif

static Eina_Bool _indicator_listen_timer_cb(void* data)
{
	win_info *win = data;

	retv_if(!win, ECORE_CALLBACK_CANCEL);

	//win = (win_info*)data;

	if (!elm_win_socket_listen(win->win , INDICATOR_SERVICE_NAME, 0, EINA_FALSE)) {
		_E("failed to elm_win_socket_listen() %x", win->win);
		return ECORE_CALLBACK_RENEW;
	} else {
		_D("listen success");
		s_info.listen_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}
}

#define INDICATOR_HEIGHT_TM1 52
static void _create_window(struct appdata *ad)
{
	Evas_Object *dummy_win = NULL;

	_D("Create window");

	ad->win.win = elm_win_add(NULL, "indicator", ELM_WIN_SOCKET_IMAGE);
	ret_if(!(ad->win.win));

	elm_win_alpha_set(ad->win.win, EINA_TRUE);

	dummy_win = elm_win_add(NULL, "indicator_dummy", ELM_WIN_BASIC);
	if (dummy_win) {
		elm_win_screen_size_get(dummy_win, NULL, NULL, &ad->win.port_w, &ad->win.land_w);
		evas_object_del(dummy_win);
		_D("Dummy window w, h (%d, %d)", ad->win.port_w, ad->win.land_w);
	} else {
		_E("Critical error. Cannot create dummy window");
	}

	if (!elm_win_socket_listen(ad->win.win , INDICATOR_SERVICE_NAME, 0, EINA_FALSE)) {
		_E("Failed 1st to elm_win_socket_listen() %x", ad->win.win);

		if (s_info.listen_timer != NULL) {
			ecore_timer_del(s_info.listen_timer);
			s_info.listen_timer = NULL;
		}
		s_info.listen_timer = ecore_timer_add(3, _indicator_listen_timer_cb, &(ad->win));
	}

	elm_win_alpha_set(ad->win.win , EINA_TRUE);
	/* FIXME */
	elm_win_borderless_set(ad->win.win , EINA_TRUE);
	evas_object_size_hint_fill_set(ad->win.win , EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->win.win , 1.0, 0.5);

	evas_object_resize(ad->win.win, ad->win.port_w, INDICATOR_HEIGHT_TM1);
	_D("w,h(%d,%d)", ad->win.port_w, INDICATOR_HEIGHT_TM1);

	evas_object_show(ad->win.win);

}

static void _create_base_gui(void* data)
{
	struct appdata *ad = data;

	ret_if(!ad);

	_D("Start to create base gui");

	_create_window(ad);

	//FIXME
#if 0
	if (INDICATOR_ERROR_NONE != _tzsh_set(ad)) {
		_E("Failed to set tzsh");
	}
#endif

	/* FIXME */
	ad->win.h = INDICATOR_HEIGHT_TM1;
	ad->win.w = ad->win.port_w;
	ad->win.evas = evas_object_evas_get(ad->win.win);

	_D("win_size = Original(%d, %d), Scaled(%lf, %lf)", ad->win.port_w, ad->win.h, ELM_SCALE_SIZE(ad->win.port_w), ELM_SCALE_SIZE(ad->win.h));

	_create_layout(ad, util_get_res_file_path(EDJ_FILE), GRP_NAME);
	_create_box(&(ad->win));


#if 0 /* For test */
	Evas_Object *rect = evas_object_rectangle_add(ad->win.evas);
	ret_if(!rect);
	evas_object_resize(rect, 720, 52);
	evas_object_color_set(rect, 0, 0, 255, 255);
	evas_object_show(rect);
	evas_object_layer_set(rect, -256);
#endif
	ad->win.data = data;

	return;
}

static void _init_win_info(void * data)
{
	struct appdata *ad = data;

	ret_if(!ad);

	memset(&(ad->win),0x00,sizeof(win_info));
}

static void _init_tel_info(void * data)
{
	struct appdata *ad = data;

	ret_if(!ad);

	memset(&(ad->tel_info), 0x00, sizeof(telephony_info));
}

static indicator_error_e _start_indicator(void *data)
{
	retv_if(!data, INDICATOR_ERROR_INVALID_PARAMETER);

	_init_win_info(data);
	_init_tel_info(data);

	/* Create indicator window */
	_create_base_gui(data);

	return INDICATOR_ERROR_NONE;
}

static indicator_error_e _terminate_indicator(void *data)
{
	struct appdata *ad = data;

	retv_if(!ad, INDICATOR_ERROR_INVALID_PARAMETER);

	modules_fini(data);
	unregister_event_handler(ad);

	box_fini(&(ad->win));

	if (ad->win.evas)
		evas_image_cache_flush(ad->win.evas);

	if (ad->win.layout) {
		evas_object_del(ad->win.layout);
		ad->win.layout = NULL;
	}

	if (ad->win.win) {
		evas_object_del(ad->win.win);
		ad->win.win = NULL;
	}

	//FIXME
#if 0
	_tzsh_unset(ad);
#endif

	if (ad)
		free(ad);

	elm_exit();

	return INDICATOR_ERROR_NONE;
}

static void __indicator_set_showhide_press(int value, int line)
{
	show_hide_pressed = value;
}

static void _indicator_mouse_down_cb(void *data, Evas * e, Evas_Object * obj, void *event)
{
	win_info *win = (win_info*)data;
	Evas_Event_Mouse_Down *ev = NULL;

	retm_if(data == NULL || event == NULL, "Invalid parameter!");
	ev = event;

	win->mouse_event.x = ev->canvas.x;
	win->mouse_event.y = ev->canvas.y;

	if (ev->button != 1) {
		return;
	}

#ifdef HOME_KEY_EMULATION
	if (box_check_indicator_area(win, ev->canvas.x, ev->canvas.y)) {
		int lock_state = VCONFKEY_IDLE_UNLOCK;
		int ps_state = -1;
		int ret = -1;

	/*	if (indicator_message_disp_check(win->type) == 1) {
			return;
		}*/
		ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE,&lock_state);

		if (ret != 0 || lock_state == VCONFKEY_IDLE_LOCK) {
			return;
		}
		ret = vconf_get_int(VCONFKEY_SETAPPL_PSMODE,&ps_state);

		if (ret != 0 || ps_state == SETTING_PSMODE_EMERGENCY) {
			return;
		}

		if (box_check_home_icon_area(win, ev->canvas.x, ev->canvas.y)) {

			if (util_check_system_status() == FAIL) {
				_D("util_check_system_status failed");
				return;
			}
			home_button_pressed = EINA_TRUE;
		}
		indicator_press_coord.x = ev->canvas.x;
		indicator_press_coord.y = ev->canvas.y;
	}
#endif
}



static void _indicator_mouse_move_cb(void *data, Evas * e, Evas_Object * obj, void *event)
{
	Evas_Event_Mouse_Move *ev = NULL;
	win_info* win = (win_info*)data;

	retm_if(data == NULL || event == NULL, "Invalid parameter!");

	ev = event;

	if (home_button_pressed) {
		if (!box_check_home_icon_area(win,ev->cur.canvas.x,ev->cur.canvas.y)) {
			home_button_pressed = false;
		}

	}
	if (show_hide_pressed == 1) {
			if (!box_check_more_icon_area(win,ev->cur.canvas.x,ev->cur.canvas.y)) {
				__indicator_set_showhide_press(EINA_FALSE, __LINE__);
			}
	}
}

static void _indicator_mouse_up_cb(void *data, Evas * e, Evas_Object * obj, void *event)
{
	Evas_Event_Mouse_Up *ev = NULL;
	win_info *win = (win_info *)data;

	retm_if(data == NULL || event == NULL, "Invalid parameter!");

	ev = event;

#ifdef HOME_KEY_EMULATION
	if (box_check_indicator_area(win, ev->canvas.x, ev->canvas.y)) {

		if (box_check_home_icon_area(win, ev->canvas.x, ev->canvas.y)) {
			if (home_button_pressed == EINA_TRUE) {
				util_launch_search(win->data);
				feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);
			}

		} else if (box_check_more_icon_area(win, ev->canvas.x, ev->canvas.y)) {
			if(show_hide_pressed == EINA_TRUE) {
				_D("pressed area");
				feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);
			}
		}
	}

	home_button_pressed = EINA_FALSE;
	__indicator_set_showhide_press(EINA_FALSE, __LINE__);

#else /* HOME_REMOVE_LONGPRESS */
	int mouse_up_prio = -1;
	int mouse_down_prio = -1;
	int lock_state, lock_ret;

	if (home_button_pressed == EINA_TRUE) {
		home_button_pressed = EINA_FALSE;
	}

	mouse_down_prio =
		box_get_priority_in_move_area(win,win->mouse_event.x,
							win->mouse_event.y);
	mouse_up_prio = box_get_priority_in_move_area(win,ev->canvas.x,
							ev->canvas.y);

	if (mouse_down_prio > -1 && mouse_up_prio > -1
		&& mouse_down_prio == mouse_up_prio) {
		switch (mouse_down_prio) {
		case INDICATOR_PRIORITY_FIXED1:
			lock_ret = system_settings_get_value_int(SYSTEM_SETTINGS_KEY_LOCK_STATE,
					&lock_state);

			/* In Lock Screen, home button don't have to do */
			if (lock_ret == SYSTEM_SETTINGS_ERROR_NONE && lock_state == SYSTEM_SETTINGS_LOCK_STATE_LOCK)
				break;

			if (util_check_system_status() == FAIL)
				break;
		break;
		}
	}
#endif /* HOME_KEY_EMULATION */
	win->mouse_event.x = 0;
	win->mouse_event.y = 0;
}

#if 0
static void _app_terminate_cb(app_context_h app_context, app_context_status_e status, void *data)
{
	retm_if(data == NULL, "Invalid parameter!");
	_D("_app_terminate_cb");
	char *app_id = NULL;
	app_context_get_app_id(app_context, &app_id);
	if (app_id == NULL) {
		_E("app_id is null!!");
		return;
	} else {
		_D("_app_terminate_cb %s",app_id);
	}

	if (status == APP_CONTEXT_STATUS_TERMINATED) {
		if (strcmp(MP_APP_ID,app_id) == 0) {
			_D("hide music icon");
			hide_mp_icon();
		} else if(strcmp(FMRADIO_APP_ID,app_id) == 0) {
			_D("hide fm radio icon");
			hide_fm_radio_icon();
		} else if(strcmp(VR_APP_ID,app_id) == 0) {
			_D("hide voice recorder icon");
			hide_voice_recorder_icon();
		}
	}
	if (app_id!=NULL) {
		free(app_id);
		app_id = NULL;
	}
}

static void register_app_terminate_cb(void* data)
{
	retm_if(data == NULL, "Invalid parameter!");
	app_manager_set_app_context_status_cb(_app_terminate_cb, MP_APP_ID, data);
	app_manager_set_app_context_status_cb(_app_terminate_cb, FMRADIO_APP_ID, data);
	app_manager_set_app_context_status_cb(_app_terminate_cb, VR_APP_ID, data);
}
#endif

static void _signal_handler(int signum, siginfo_t *info, void *unused)
{
    ui_app_exit();
}

static bool app_create(void *data)
{
	struct appdata *ad = data;
	int ret;

	retv_if(!ad, false);

	elm_app_base_scale_set(2.6);

	/* Signal handler */
	struct sigaction act;
	memset(&act, 0x00, sizeof(struct sigaction));
	act.sa_sigaction = _signal_handler;
	act.sa_flags = SA_SIGINFO;

	ret = sigemptyset(&act.sa_mask);
	if (ret < 0) {
		char error_message[ERROR_MESSAGE_LEN] = {0,};
		strerror_r(errno, error_message, ERROR_MESSAGE_LEN);
		_E("Failed to sigemptyset[%s]", error_message);
	}
	ret = sigaddset(&act.sa_mask, SIGTERM);
	if (ret < 0) {
		char error_message[ERROR_MESSAGE_LEN] = {0,};
		strerror_r(errno, error_message, ERROR_MESSAGE_LEN);
		_E("Failed to sigaddset[%s]", error_message);
	}
	ret = sigaction(SIGTERM, &act, NULL);
	if (ret < 0) {
		char error_message[ERROR_MESSAGE_LEN] = {0,};
		strerror_r(errno, error_message, ERROR_MESSAGE_LEN);
		_E("Failed to sigaction[%s]", error_message);
	}

	ret = _start_indicator(ad);
	if (ret != INDICATOR_ERROR_NONE) {
		_D("Failed to create a new window!");
	}

	/* Set nonfixed-list size for display */
	modules_init_first(ad);

	if (ad->win.win) {
		elm_win_activate(ad->win.win);
	}
	evas_object_show(ad->win.layout);
	evas_object_show(ad->win.win);

	return true;
}

static void app_terminate(void *data)
{
	struct appdata *ad = data;
	modules_fini(data);
	ticker_fini(ad);
	indicator_toast_popup_fini();
#ifdef _SUPPORT_SCREEN_READER2
	indicator_service_tts_fini(data);
#endif

	unregister_event_handler(ad);

	feedback_deinitialize();

	box_fini(&(ad->win));
	evas_image_cache_flush(ad->win.evas);
	evas_object_del(ad->win.layout);
	evas_object_del(ad->win.win);

	_D("INDICATOR IS TERMINATED");
}

static void app_pause(void *data)
{
}

static void app_resume(void *data)
{
}

static void app_service(app_control_h service, void *data)
{
	struct appdata *ad = data;

	_D("INDICATOR IS STARTED");

	register_event_handler(ad);
	modules_init(data);
#ifdef _SUPPORT_SCREEN_READER
	modules_register_tts(data);
#endif
	feedback_initialize();
	indicator_toast_popup_init(data);
	if (INDICATOR_ERROR_NONE != ticker_init(ad)) {
		_E("Ticker cannot initialize");
	}
#ifdef _SUPPORT_SCREEN_READER2
	indicator_service_tts_init(data);
#endif
	_indicator_lock_status_cb(SYSTEM_SETTINGS_KEY_LOCK_STATE, data);
#if 0
	register_app_terminate_cb(data);
#endif
}

int main(int argc, char *argv[])
{
	struct appdata ad;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	int ret = 0;

	_D("Start indicator");

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_service;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, _indicator_low_bat_cb, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, _indicator_lang_changed_cb, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, _indicator_region_changed_cb, NULL);

	memset(&ad, 0x0, sizeof(struct appdata));

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		_E("app_main() is failed. err = %d", ret);
	}

	return ret;
}

/* End of file */
