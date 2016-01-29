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
#include <efl_assist.h>
#include <tzsh_indicator_service.h>
#include "indicator.h"

#if !defined(PACKAGE)
#  define PACKAGE "indicator"
#endif

#if !defined(PACKAGEID)
#  define PACKAGEID "org.tizen.indicator"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR "/usr/apps/org.tizen.indicator/res/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR "/usr/apps/org.tizen.indicator/res/edje"
#endif

#if !defined(ICONDIR)
#  define ICONDIR "/usr/apps/org.tizen.indicator/res/icons"
#endif

#if !defined(CHANGEABLEDIR)
#  define CHANGEABLEDIR "/usr/apps/org.tizen.indicator/shared/res/tables"
#endif

#define EDJ_FILE EDJDIR"/"PACKAGE"_port.edj"
#define ICON_THEME_FILE EDJDIR"/"PACKAGE"_icon_theme.edj"
#define ICON_NONFIXED_THEME_FILE EDJDIR"/"PACKAGE"_icon_nonfixed_theme.edj"
#define ICON_NONFIXED_THEME_ANI_FILE EDJDIR"/"PACKAGE"_icon_animation.edj"
#define ICON_NONFIXED_THEME_SYNC_FILE EDJDIR"/"PACKAGE"_icon_activesync.edj"

#define CHANGEABLE_COLOR_FILE CHANGEABLEDIR"/"PACKAGEID"_ChangeableColorInfo.xml"
#define CHANGEABLE_FONT_FILE CHANGEABLEDIR"/"PACKAGEID"_ChangeableFontInfo.xml"


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

	/* FIXME */
	tzsh_h tzsh;
	tzsh_indicator_service_h indicator_service;

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
