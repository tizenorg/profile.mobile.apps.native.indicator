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
//#include <Ecore_X.h>
#include <vconf.h>
#include <unistd.h>
#include <privilege-control.h>
#include <app_manager.h>
#include <signal.h>
#include <minicontrol-monitor.h>
#include <feedback.h>
#include <notification.h>
//#include <notification_internal.h>
#include <app_preference.h>
#include <wifi.h>
#if 0
#include <app_manager_product.h>
#endif

#include "common.h"
#include "box.h"
#include "icon.h"
#include "main.h"
#include "indicator_gui.h"
#include "modules.h"
#include "util.h"
#include "plmn.h"
#include "message.h"
#include "tts.h"
#include "log.h"
#include "indicator.h"
#include "ticker.h"

#define GRP_MAIN "indicator"
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
static int bFirst_opacity = 1;

static int _window_new(void *data);
static int _window_del(void *data);
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

	_window_del((struct appdata *)data);
}

static void _indicator_notify_pm_state_cb(keynode_t * node, void *data)
{
	static int nMove = 0;
	static int nIndex = 1;
	int val = -1;

	ret_if(!data);

	if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) return;

	switch (val) {
	case VCONFKEY_PM_STATE_LCDOFF:
		if (clock_timer != NULL) {
			ecore_timer_del(clock_timer);
			clock_timer = NULL;
		}
	case VCONFKEY_PM_STATE_SLEEP: // lcd off 2
		/* FIXME */
		nMove = nMove+nIndex;
		if(nMove>=4)
			nIndex = -1;
		else if(nMove<=0)
			nIndex = 1;
		{
			char temp[30] = {0,};
			sprintf(temp,"indicator.padding.resize.%d",nMove);
			util_signal_emit(data,temp,"indicator.prog");
		}
		icon_set_update_flag(0);
		box_noti_ani_handle(0);
		break;
	case VCONFKEY_PM_STATE_NORMAL:
		if (!icon_get_update_flag()) {
			icon_set_update_flag(1);
			box_noti_ani_handle(1);
			modules_wake_up(data);
		}
		break;
	case VCONFKEY_PM_STATE_LCDDIM:
	default:
		break;
	}
}

static void _indicator_power_off_status_cb(keynode_t * node, void *data)
{
	int val = -1;

	ret_if(!data);

	if (vconf_get_int(VCONFKEY_SYSMAN_POWER_OFF_STATUS, &val) < 0) return;

	switch (val) {
	case VCONFKEY_SYSMAN_POWER_OFF_DIRECT:
	case VCONFKEY_SYSMAN_POWER_OFF_RESTART:
		ui_app_exit();
		break;
	default:
		break;
	}

}

static void _indicator_lock_status_cb(keynode_t * node, void *data)
{
	static int lockstate = 0;
	extern int clock_mode;
	int val = -1;

	ret_if(!data);

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) return;
	if (val == lockstate) return;

	lockstate = val;

	switch (val) {
	case VCONFKEY_IDLE_UNLOCK:
		if (!clock_mode) util_signal_emit(data,"clock.font.12","indicator.prog");
		else util_signal_emit(data,"clock.font.24","indicator.prog");
		break;
	case VCONFKEY_IDLE_LOCK:
	/*case VCONFKEY_IDLE_LAUNCHING_LOCK:
		util_signal_emit(data,"clock.invisible","indicator.prog");
		break;*/
	default:
		break;
	}

}

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

#ifdef INDICATOR_SUPPORT_OPACITY_MODE
static void _change_opacity(void *data, enum indicator_opacity_mode mode)
{
	struct appdata *ad = NULL;
	const char *signal = NULL;
	retif(data == NULL, , "Invalid parameter!");

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
#if 0
static void _indicator_quickpanel_changed(void *data, int is_open)
{
	int val = 0;

	ret_if(!data);

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) return;
	if (val == VCONFKEY_IDLE_LOCK) return;
}
#endif
#endif /* INDICATOR_SUPPORT_OPACITY_MODE */

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
#if 0
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
#endif
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

static Eina_Bool _property_changed_cb(void *data, int type, void *event)
{
#if 0
//	Ecore_X_Event_Window_Property *ev = event;
	struct appdata *ad = NULL;

	ad = data;
	retv_if(!data, EINA_FALSE);
//	retv_if(!ev, EINA_FALSE);
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

		ret = ecore_x_window_prop_window_get(elm_win_xwindow_get(ad->win_overlay), ad->atom_active, &(active_win), 1);
		if (ret == -1) return EINA_FALSE;

		if (active_win != ad->active_indi_win) {
			if (ad->active_indi_win != -1) {
				ecore_x_window_unsniff(ad->active_indi_win);
				_D("UNSNIFF API %x", ad->active_indi_win);
			}
			ad->active_indi_win = active_win;

			ecore_x_window_sniff(ad->active_indi_win);
			if (indicator_message_retry_check()) {
				indicator_message_display_trigger();
			}
		}

		_active_indicator_handle(data, 1);
		_active_indicator_handle(data, 2);
	}
#endif
	return EINA_TRUE;
}

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

static void register_event_handler(void *data)
{
	struct appdata *ad = data;
//	Ecore_Event_Handler *hdl = NULL;
	ret_if(!data);

	_register_event_handler_both(&(ad->win),data);

#if 0
	hdl = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _indicator_client_message_cb, (void *)ad);
	ret_if(!hdl);
	ad->evt_handlers = eina_list_append(ad->evt_handlers, hdl);

	hdl = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _property_changed_cb, data);
	ret_if(!hdl);
	ad->evt_handlers = eina_list_append(ad->evt_handlers, hdl);
#endif
	if (vconf_notify_key_changed(VCONFKEY_PM_STATE, _indicator_notify_pm_state_cb, (void *)ad) != 0) {
		_E("Fail to set callback for VCONFKEY_PM_STATE");
	}

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS, _indicator_power_off_status_cb, (void *)ad) < 0) {
		_E("Failed to set callback for VCONFKEY_SYSMAN_POWER_OFF_STATUS");
	}

	if (vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE, _indicator_lock_status_cb, (void *)ad) < 0) {
		_E("Failed to set callback for VCONFKEY_IDLE_LOCK_STATE");
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

	vconf_ignore_key_changed(VCONFKEY_PM_STATE, _indicator_notify_pm_state_cb);
	vconf_ignore_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS, _indicator_power_off_status_cb);
	vconf_ignore_key_changed(VCONFKEY_IDLE_LOCK_STATE, _indicator_lock_status_cb);

	Ecore_Event_Handler *hdl = NULL;
	EINA_LIST_FREE(ad->evt_handlers, hdl) {
		if (hdl) ecore_event_handler_del(hdl);
	}

//	edbus_cleaner();

	return OK;
}

static Evas_Object *_create_layout(Evas_Object * parent, const char *file, const char *group)
{
	Evas_Object *layout = NULL;
	int ret;

	layout = elm_layout_add(parent);
	if (layout) {
		ret = elm_layout_file_set(layout, file, group);
		if (!ret) {
			evas_object_del(layout);
			return NULL;
		}
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_win_resize_object_add(parent, layout);
	}

	return layout;
}

static Eina_Bool _indicator_listen_timer_cb(void* data)
{
	win_info *win = NULL;

	ret_if(!data);

	win = (win_info*)data;

	if (!elm_win_socket_listen(win->win , INDICATOR_SERVICE_NAME, 0, EINA_FALSE)) {
		_E("faile to elm_win_socket_listen() %x", win->win);
		return ECORE_CALLBACK_RENEW;
	} else {
		_D("listen success");
		return ECORE_CALLBACK_CANCEL;
	}
}

static void _create_box(win_info *win)
{
	ret_if(!win);

	/* First, clear layout */
	box_fini(win);

	box_init(win);

	return;
}

#define qHD_RESOLUTION_WIDTH 540
#define INDICATOR_HEIGHT_HD 48
#define INDICATOR_HEIGHT_qHD 38
#define INDICATOR_HEIGHT_WVGA 36
static void _create_win(void* data)
{
	struct appdata *ad = NULL;
	Ecore_X_Window xwin;
//	Ecore_X_Window zone;
//	Ecore_X_Window_State states[2];
	int root_w;
	int root_h;
//	Ecore_X_Window root;

	ret_if(!data);

	_D("Window created");

	ad = data;

//	root = ecore_x_window_root_first_get();
//	ecore_x_window_size_get(root, &root_w, &root_h);
#if 0
	if (root_w > qHD_RESOLUTION_WIDTH) { // HD
		_D("Window w, h (%d,%d)", root_w, root_h);
		ad->win.port_w = root_w;
		ad->win.land_w = root_h;
		ad->win.h = INDICATOR_HEIGHT_HD;
	} else if (root_w < qHD_RESOLUTION_WIDTH) { // WVGA
		_D("Window w, h (%d,%d)", root_w, root_h);
		ad->win.port_w = root_w;
		ad->win.land_w = root_h;
		ad->win.h = INDICATOR_HEIGHT_WVGA;
	} else { // qHD
		_D("Window w, h (%d,%d)", root_w, root_h);
		ad->win.port_w = root_w;
		ad->win.land_w = root_h;
		ad->win.h = INDICATOR_HEIGHT_qHD;
	}
#endif

	/* Create socket window */
	ad->win.win = elm_win_add(NULL, "indicator", ELM_WIN_SOCKET_IMAGE);
	ret_if(!(ad->win.win));

	/* FIXME : get indicator width and height withour ecore_x API */
	elm_win_screen_size_get(ad->win.win, NULL, NULL, &root_w, &root_h);
	_D("Window w, h (%d, %d)", root_w, root_h);

//	if (root_w > qHD_RESOLUTION_WIDTH) { // HD
		ad->win.port_w = 1440;
		ad->win.land_w = 2560;
		ad->win.h = 96;
		/* FIXME */
		root_w = 1440;
#if 0
	} else if (root_w < qHD_RESOLUTION_WIDTH) { // WVGA
		ad->win.port_w = 480;
		ad->win.land_w = 800;
		ad->win.h = 36;
		/* FIXME */
		root_w = 480;
	} else { // qHD
		ad->win.port_w = 540;
		ad->win.land_w = 960;
		ad->win.h = 38;
		/* FIXME */
		root_w = 540;
	}
#endif
	ad->win.w = root_w;
	_D("=============================== Window w, h (%d, %d)", ad->win.port_w, ad->win.h);

	if (!elm_win_socket_listen(ad->win.win , INDICATOR_SERVICE_NAME, 0, EINA_FALSE)) {
		_E("failed 1st to elm_win_socket_listen() %x", ad->win.win);
		/* Start timer */
		if (ecore_timer_add(3, _indicator_listen_timer_cb, &(ad->win))) {
			_E("Failed to add timer object");
		}
	}
#if 0
	elm_win_alpha_set(ad->win.win , EINA_TRUE);
	elm_win_borderless_set(ad->win.win , EINA_TRUE);
	evas_object_size_hint_fill_set(ad->win.win , EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->win.win , 1.0, 0.5);
#endif
	ad->win.evas = evas_object_evas_get(ad->win.win);
#if 0
	Evas_Object *rect = evas_object_rectangle_add(ad->win.evas);
	evas_object_resize(rect, 1440, 96);
	evas_object_color_set(rect, 0, 0, 255, 255);
	evas_object_show(rect);
	evas_object_layer_set(rect, -256);
#endif
	ad->win.layout = _create_layout(ad->win.win, EDJ_FILE0, GRP_MAIN);
	ret_if(!(ad->win.layout));

	_D("win_size = Original(%d, %d), Scaled(%lf, %lf)", ad->win.port_w, ad->win.h, ELM_SCALE_SIZE(ad->win.port_w), ELM_SCALE_SIZE(ad->win.h));

	evas_object_resize(ad->win.win, ad->win.port_w, ad->win.h);
	evas_object_move(ad->win.win, 0, 0);

	_create_box(&(ad->win));

	ad->win.data = data;

	evas_object_show(ad->win.layout);
	evas_object_show(ad->win.win);

	return;
}



static void create_overlay_win(void* data)
{
	struct appdata *ad = data;

	Evas_Object *eo;
	int w, h;
	int indi_h;
//	int id = -1;
	Ecore_X_Window xwin;
//	Ecore_X_Window zone;
//	Ecore_X_Window_State states[2];
//	Ecore_X_Atom ATOM_MV_INDICATOR_GEOMETRY = 0;

	indi_h = (int)ELM_SCALE_SIZE(INDICATOR_HEIGHT);

	ad->active_indi_win = -1;

	eo = elm_win_add(NULL, "INDICATOR", ELM_WIN_BASIC);
	/*id = elm_win_aux_hint_add(eo, "wm.policy.win.user.geometry", "1");
	if(id == -1) {
		_E("Cannot add user.geometry");
		return;
	}*/
	elm_win_title_set(eo, "INDICATOR");
	elm_win_borderless_set(eo, EINA_TRUE);
	//ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);

	_D("win_size = Original(%d, %d), Scaled(%lf, %lf)", 2,2, ELM_SCALE_SIZE(2), ELM_SCALE_SIZE(2));
	evas_object_resize(eo, ELM_SCALE_SIZE(2), ELM_SCALE_SIZE(2));

	evas_object_move(eo , 0, 0);
	elm_win_alpha_set(eo, EINA_TRUE);

	xwin = elm_win_xwindow_get(eo);
//	ecore_x_icccm_hints_set(xwin, 0, 0, 0, 0, 0, 0, 0);
//	states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
//	states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
//	ecore_x_netwm_window_state_set(xwin, states, 2);

//	ecore_x_icccm_name_class_set(xwin, "INDICATOR", "INDICATOR");

//	ecore_x_netwm_window_type_set(xwin, ECORE_X_WINDOW_TYPE_DOCK);

	unsigned int ind_gio_val[16] = { 0, 0, w, indi_h,    /* angle 0 (x,y,w,h) */
				  						0, 0, indi_h, h,   /* angle 90 (x,y,w,h) */
				  						0, h-indi_h, w, indi_h, /* angle 180 (x,y,w,h) */
				  						w-indi_h, 0, indi_h, h /* angle 270 (x,y,w,h) */  };

//	ATOM_MV_INDICATOR_GEOMETRY = ecore_x_atom_get(STR_ATOM_MV_INDICATOR_GEOMETRY);

//	ecore_x_window_prop_card32_set(xwin, ATOM_MV_INDICATOR_GEOMETRY, ind_gio_val, 16);

//	zone = ecore_x_e_illume_zone_get(xwin);
//	ecore_x_event_mask_set(zone, ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
	evas_object_show(eo);

	ad->win_overlay = eo;
//	ad->atom_active = ecore_x_atom_get("_E_ACTIVE_INDICATOR_WIN");

	return ;
}



static void _init_win_info(void * data)
{
	struct appdata *ad = data;
	retif(data == NULL, , "Invalid parameter!");

	memset(&(ad->win),0x00,sizeof(win_info));

	ad->win_overlay = NULL;
}



static void _init_tel_info(void * data)
{
	struct appdata *ad = data;

	ret_if(!ad);

	memset(&(ad->tel_info), 0x00, sizeof(telephony_info));
}



static int _window_new(void *data)
{
	retif(data == NULL, FAIL, "Invalid parameter!");

	_init_win_info(data);
	_init_tel_info(data);

	/* Create indicator window */
	_create_win(data);

	return INDICATOR_ERROR_NONE;
}



static int _window_del(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	retif(data == NULL, FAIL, "Invalid parameter!");

	modules_fini(data);
	unregister_event_handler(ad);

	box_fini(&(ad->win));
	evas_image_cache_flush(ad->win.evas);
	evas_object_del(ad->win.layout);
	ad->win.layout = NULL;

	evas_object_del(ad->win.win);
	ad->win.win = NULL;

	evas_object_del(ad->win_overlay);
	ad->win_overlay = NULL;

	if (ad) free(ad);

	elm_exit();
	return OK;
}

static void __indicator_set_showhide_press(int value, int line)
{
	show_hide_pressed = value;
}

static void _indicator_mouse_down_cb(void *data, Evas * e, Evas_Object * obj, void *event)
{
	win_info *win = (win_info*)data;
	Evas_Event_Mouse_Down *ev = NULL;

	retif(data == NULL || event == NULL, , "Invalid parameter!");
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

	retif(data == NULL || event == NULL, , "Invalid parameter!");

	ev = event;

	if (home_button_pressed) {
		if (!box_check_home_icon_area(win,ev->cur.canvas.x,ev->cur.canvas.y)) {
			home_button_pressed = FALSE;
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

	retif(data == NULL || event == NULL, , "Invalid parameter!");

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
				DBG("pressed area");
				feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);
			}
		}
	}

	home_button_pressed = EINA_FALSE;
	__indicator_set_showhide_press(EINA_FALSE, __LINE__);

#else /* HOME_REMOVE_LONGPRESS */
	int mouse_up_prio = -1;
	int mouse_down_prio = -1;

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
			int lock_state = VCONFKEY_IDLE_UNLOCK;
			int lock_ret = -1;

			lock_ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE,
					&lock_state);

			/* In Lock Screen, home button don't have to do */
			if (lock_ret == 0 && lock_state == VCONFKEY_IDLE_LOCK)
				break;

			char *top_win_name = NULL;

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
	retif(data == NULL, , "Invalid parameter!");
	DBG("_app_terminate_cb");
	char *app_id = NULL;
	app_context_get_app_id(app_context, &app_id);
	if (app_id == NULL) {
		ERR("app_id is null!!");
		return;
	} else {
		DBG("_app_terminate_cb %s",app_id);
	}

	if (status == APP_CONTEXT_STATUS_TERMINATED) {
		if (strcmp(MP_APP_ID,app_id) == 0) {
			DBG("hide music icon");
			hide_mp_icon();
		} else if(strcmp(FMRADIO_APP_ID,app_id) == 0) {
			DBG("hide fm radio icon");
			hide_fm_radio_icon();
		} else if(strcmp(VR_APP_ID,app_id) == 0) {
			DBG("hide voice recorder icon");
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
	retif(data == NULL, , "Invalid parameter!");
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
	struct appdata *ad = NULL;
	int ret;

	ad = data;
	elm_app_base_scale_set(1.7);

	/* Signal handler */
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

	ret = _window_new(ad);
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
	indicator_message_fini();
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

	create_overlay_win(data);
	register_event_handler(ad);
	modules_init(data);
#ifdef _SUPPORT_SCREEN_READER
	modules_register_tts(data);
#endif
	feedback_initialize();
	indicator_message_init(data);
	if (INDICATOR_ERROR_NONE != ticker_init(ad)) {
		_E("Ticker cannot initialize");
	}
#ifdef _SUPPORT_SCREEN_READER2
	indicator_service_tts_init(data);
#endif
	_indicator_lock_status_cb(NULL,data);
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

	ret = perm_app_set_privilege("org.tizen.", NULL, NULL);
	if (ret != PC_OPERATION_SUCCESS) {
		_E("[INDICATOR] Failed to set privilege (%d)", ret);
	}

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
