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


#ifndef __DEF_indicator_H_
#define __DEF_indicator_H_

#include <Elementary.h>
#include <Ecore_X.h>
#include "indicator.h"

#if !defined(PACKAGE)
#  define PACKAGE "indicator"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR "/usr/apps/com.samsung.indicator/res/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR "/usr/apps/com.samsung.indicator/res/edje"
#endif

#if !defined(ICONDIR)
#  define ICONDIR "/usr/apps/com.samsung.indicator/res/icons"
#endif

#define EDJ_FILE EDJDIR"/"PACKAGE".edj"
#define ICON_THEME_FILE EDJDIR"/"PACKAGE"_icon_theme.edj"
#define ICON_NONFIXED_THEME_FILE EDJDIR"/"PACKAGE"_icon_nonfixed_theme.edj"

#define _S(str)	dgettext("sys_string", str)
#define _(str) gettext(str)

#define HOME_SCREEN_NAME		"com.samsung.live-magazine"
#define MENU_SCREEN_NAME		"com.samsung.menu-screen"
#define LOCK_SCREEN_NAME		"com.samsung.idle-lock"
#define QUICKPANEL_NAME			"E Popup"
#define CALL_NAME			"com.samsung.call"
#define VTCALL_NAME			"com.samsung.vtmain"

enum _win_type {
	TOP_WIN_NORMAL = 0,
	TOP_WIN_LOCK_SCREEN,
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

struct appdata {

	win_info win[INDICATOR_WIN_MAX];

	double xscale;
	double yscale;
	double scale;
	int angle;

	Eina_Bool lock;
	Eina_Bool menu;
	Eina_Bool quickpanel;

	int notifd;
	Eina_List *evt_handlers;

	enum indicator_opacity_mode opacity_mode;

	enum _win_type top_win;

	void (*update_display) (int);
};

#endif
