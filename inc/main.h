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


#ifndef __DEF_indicator_H_
#define __DEF_indicator_H_

#include <Elementary.h>
//#include <Ecore_X.h>
//FIXME
#if 0
#include <tzsh_indicator_service.h>
#endif
#include "indicator.h"


/**
 * @mainpage Indicator documentation
 *
 * @section overview Overview
 *
 * @image html indicator.png
 *
 * \n\n
 * Indiacator app main purpose is to give user quick preview for what is going on in the system -
 * connections and battery state, time, apps notifications etc.
 * \n
 *
 * @section h_s Helpful snippets
 *
 * @ref post_noti \n
 * @ref update_noti \n
 * @ref delete_noti \n\n
 *
 * @ref bg_change \n
 *
 * \n\n
 *
 *
 * @subsection post_noti Post notification icon
 *
 * @code
 * #define TAG_FOR_NOTI "example_unique_tag_to_set"
 *
 * void example_notification_prepare_and_post(void)
 * {
 *	notification_h noti;
 * 	notification_type_e noti_type = NOTIFICATION_TYPE_NOTI;
 *	notification_image_type_e img_type = NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR;
 *	const char *img_path = "example_path_to_notification_icon_in_shared res_folder";
 *	int applist = NOTIFICATION_DISPLAY_APP_INDICATOR;
 *
 *	noti = notification_create(noti_type);
 *	notification_set_image(noti, img_type, img_path);
 *	notification_set_display_applist(noti, applist);
 *	notification_set_tag(noti, TAG_FOR_NOTI);
 *
 *	notification_post(noti);
 *	notification_free(noti);
 * }
 * @endcode
 *
 * @subsection update_noti Update notification icon
 *
 * @code
 * void example_notification_update(void)
 * {
 *	const char *img_path_new = "example_path_to_new_notification_icon";
 *	notification_image_type_e img_type = NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR;
 *	notification_h noti = NULL;
 *
 *	noti = notification_load_by_tag(TAG_FOR_NOTI);
 *	notification_set_image(noti, img_type, img_path_new);
 *
 *	notification_update(noti);
 *	notification_free(noti);
 * }
 * @endcode
 *
 * @subsection delete_noti Delete notification icon
 *
 * @code
 * void example_notification_delete(void)
 * {
 *	notification_h noti = NULL;
 *	noti = notification_load_by_tag(TAG_FOR_NOTI);
 *
 *	notification_delete(noti);
 *	notification_free(noti);
}
 * @endcode
 *
 * @subsection bg_change Change indicator background color
 * Please note that default state is (0, 0, 0, 255)
 * @code
 * void example_indicator_bg_change(void)
 * {
 *	bundle *message;
 *	int ret = 0;
 *	int rgba_r = 255;
 *	int rgba_g = 0;
 *	int rgba_b = 100;
 *	int rgba_a = 100;
 *
 *
 *	char *remote_port_name = "indicator/bg/color"; //That name must not be changed
 *
 *	// Register message port
 *	port_id = message_port_register_trusted_local_port("port/name/defined/by/you", message_port_cb, NULL);
 *	// Create and set bundle message
 *	message = bundle_create();
 *
 *	ret = bundle_add_str(message, KEY_INDICATOR_BG, VALUE_INDICATOR_BG_RGB);
 *	ret = bundle_add_byte(message, KEY_R, (const void *)&rgba_r, sizeof(int));
 *	ret = bundle_add_byte(message, KEY_G, (const void *)&rgba_g, sizeof(int));
 *	ret = bundle_add_byte(message, KEY_B, (const void *)&rgba_b, sizeof(int));
 *	ret = bundle_add_byte(message, KEY_A, (const void *)&rgba_a, sizeof(int));
 *
 *	//Send message
 *	ret = message_port_send_trusted_message_with_local_port("org.tizen.indicator", remote_port_name, message, port_id);
 *
 *	bundle_free(message);
 * }
 * @endcode
 *
 */


#if !defined(PACKAGE)
#  define PACKAGE "indicator"
#endif

#if !defined(PACKAGEID)
#  define PACKAGEID "org.tizen.indicator"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR "locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR "resource/"
#endif

#if !defined(ICONDIR)
#  define ICONDIR EDJDIR"icons"
#endif

#define EDJ_FILE EDJDIR"/"PACKAGE"_port.edj"
#define ICON_THEME_FILE EDJDIR"/"PACKAGE"_icon_theme.edj"
#define ICON_NONFIXED_THEME_FILE EDJDIR"/"PACKAGE"_icon_nonfixed_theme.edj"
#define ICON_NONFIXED_THEME_ANI_FILE EDJDIR"/"PACKAGE"_icon_animation.edj"


#define _S(str)	dgettext("sys_string", str)

#define HOME_SCREEN_NAME		"org.tizen.live-magazine"
#define MENU_SCREEN_NAME		"org.tizen.menu-screen"
#define LOCK_SCREEN_NAME		"org.tizen.idle-lock"
#define QUICKPANEL_NAME			"E Popup"
#define CALL_NAME			"org.tizen.call-ui"
#define VTCALL_NAME			"org.tizen.vtmain"

#define MENUSCREEN_PKG_NAME "org.tizen.menuscreen"
#define APP_TRAY_PKG_NAME "org.tizen.app-tray"
#define SEARCH_PKG_NAME "org.tizen.sfinder"
#define TIZEN_EMAIL_PACKAGE "org.tizen.email"
#define TIZEN_MESSAGE_PACKAGE "org.tizen.message"

#define MSG_DOMAIN_CONTROL_INDICATOR 0x10001
#define MSG_ID_INDICATOR_REPEAT_EVENT 0x10002
#define MSG_ID_INDICATOR_ROTATION 0x10003
#define MSG_ID_INDICATOR_OPACITY 0X1004
#define MSG_ID_INDICATOR_TYPE 0X1005
#define MSG_ID_INDICATOR_OPACITY_OSP 0X10061
#define MSG_ID_INDICATOR_ANI_START 0x10006

#define MSG_DOMAIN_CONTROL_ACCESS (int)ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL

#define INDICATOR_SERVICE_NAME "elm_indicator"

enum _win_type {
	/* Clock view */
	TOP_WIN_NORMAL = 0,
	TOP_WIN_LOCK_SCREEN,
	/* Full line of indicator */
	/* CAUTION: Don't change order! */
	TOP_WIN_CALL,
	TOP_WIN_MENU_SCREEN,
	TOP_WIN_HOME_SCREEN,
	TOP_WIN_QUICKPANEL
};

enum indicator_opacity_mode{
	INDICATOR_OPACITY_OPAQUE = 0,
	INDICATOR_OPACITY_TRANSLUCENT,
	INDICATOR_OPACITY_TRANSPARENT,
};

typedef struct _telephony_info
{
	int network_service_type;
	int network_ps_type;
	int roaming_status;
	int signal_level;
	int prefered_data;
	int prefered_voice;
	int default_network;
	int sim_status;
	int call_forward;

	void* data;
}telephony_info;

struct appdata {

	win_info win;
	telephony_info tel_info;
	int prefered_data;	// Data prefered
	Evas_Object* win_overlay;
	Evas_Object *ticker_win;

	/* FIXME */
#if 0
	tzsh_h tzsh;
	tzsh_indicator_service_h indicator_service;
#endif

	double scale;
	int angle;

	Eina_List *evt_handlers;

	enum indicator_opacity_mode opacity_mode;

	Ecore_X_Atom atom_active;
	Ecore_X_Window active_indi_win;
	//Ea_Theme_Color_Table *color_table;
	Eina_List *font_table;

	void (*update_display) (int);
};

int isSimShowing;	// For Simcard Icon
int isRSSI1Showing;	// For RSSI1 Icon
int isRSSI2Showing;	// For RSSI2 Icon
int isSilentShowing;		// For Silent Icon
int isWifiDirectShowing;	// For WiFi Direct Icon

#endif				/* __DEF_indicator_H__ */
