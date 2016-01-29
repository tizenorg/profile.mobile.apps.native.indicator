/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <Elementary.h>
//#include <Ecore_X.h>
//#include <utilX.h>
#include <unicode/uloc.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>
#include <runtime_info.h>
#include <system_settings.h>
#include <efl_util.h>

#include "log.h"
#include "indicator.h"
#include "common.h"
#include "main.h"
#include "noti_win.h"

/* Using this macro to emphasize that some portion like stacking and
rotation handling are implemented for X based platform */
#ifndef __UNUSED__
#define __UNUSED__ __attribute__((unused))
#endif

struct Internal_Data {
	Evas_Object *content;
	Ecore_Event_Handler *rotation_event_handler;
	Evas_Coord w;
	Evas_Coord h;
	int angle;
};

#define PRIVATE_DATA_KEY_DATA "pdk_data"

static void _content_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	Evas_Coord h;
	struct Internal_Data *wd = evas_object_data_get(data, PRIVATE_DATA_KEY_DATA);

	ret_if(!wd);

	evas_object_size_hint_min_get(obj, NULL, &h);
	if ((h > 0)) {
		wd->h = h;
		evas_object_size_hint_min_set(obj, wd->w, wd->h);
		evas_object_size_hint_min_set(data, wd->w, wd->h);
	}
}

static void _sub_del(void *data __UNUSED__, Evas_Object *obj, void *event_info)
{
	struct Internal_Data *wd = evas_object_data_get(obj, PRIVATE_DATA_KEY_DATA);
	Evas_Object *sub = event_info;

	ret_if(!wd);

	if (sub == wd->content) {
		evas_object_event_callback_del(wd->content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _content_changed_size_hints);
		wd->content = NULL;
	}
}

static void _noti_win_destroy(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
	Evas_Object *win = NULL;
	struct Internal_Data *wd = NULL;

	ret_if(!obj);

	win = obj;

	wd = evas_object_data_del(win, PRIVATE_DATA_KEY_DATA);
	ret_if(!wd);

	if (wd->rotation_event_handler) {
		ecore_event_handler_del(wd->rotation_event_handler);
	}
	free(wd);
#if 0
	quickpanel_dbus_ticker_visibility_send(0);
#endif
}

static void _rotate_cb(void *data, Evas_Object *obj, void *event)
{
	struct appdata *ad = data;
	struct Internal_Data *wd = NULL;
	Evas_Object *base = NULL;
	int angle = 0;

	ret_if(!obj);

	wd =  evas_object_data_get(obj, PRIVATE_DATA_KEY_DATA);
	ret_if(!wd);

	base = evas_object_data_get(obj, DATA_KEY_BASE_RECT);
	ret_if(!base);

	angle = elm_win_rotation_get(obj);
	angle %= 360;
	if (angle < 0) {
		angle += 360;
	}
	_D("Ticker angle is %d degree", angle);

	switch (angle) {
	case 0:
	case 180:
		evas_object_resize(base, ad->win.port_w, wd->h);
		evas_object_size_hint_min_set(base, ad->win.port_w, wd->h);
		evas_object_resize(obj, ad->win.port_w, wd->h);
		evas_object_move(obj, 0, 0);
		break;
	case 90:
		evas_object_resize(base, ad->win.land_w, wd->h);
		evas_object_size_hint_min_set(base, ad->win.land_w, wd->h);
		evas_object_resize(obj, ad->win.land_w, wd->h);
		evas_object_move(obj, 0, 0);
		break;
	case 270:
		evas_object_resize(base, ad->win.land_w, wd->h);
		evas_object_size_hint_min_set(base, ad->win.land_w, wd->h);
		evas_object_resize(obj, ad->win.land_w, wd->h);
		evas_object_move(obj, ad->win.port_w - wd->h, 0);
		break;
	default:
		_E("cannot reach here");
	}

	wd->angle = angle;
}

Evas_Object *noti_win_add(Evas_Object *parent, struct appdata *ad)
{
	Evas_Object *win = NULL;
	struct Internal_Data *wd = NULL;

	_D("A window is created for ticker notifications");
	win = elm_win_add (NULL, "noti_win", ELM_WIN_NOTIFICATION);
	retv_if(!win, NULL);

	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_title_set(win, "noti_win");
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);
	//efl_util_set_notification_window_level(win, EFL_UTIL_NOTIFICATION_LEVEL_HIGH);
	elm_win_prop_focus_skip_set(win, EINA_TRUE);
	/* you can use evas_object_resize() and evas_object_move() by using elm_win_aux_hint_add().
	   elm_win_aux_hint_add() makes it possible to set the size and location of the notification window freely.
	   if you do not use elm_win_aux_hint_add(), notification window is displayed full screen. */
//	elm_win_aux_hint_add(win, "wm.policy.win.user.geometry", "1");
	evas_object_resize(win, ad->win.w, ad->win.h);
	evas_object_move(win, 0, 0);
	evas_object_show(win);

	if (elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, rots, 4);
	}
	evas_object_smart_callback_add(win, "wm,rotation,changed", _rotate_cb, ad);

	wd = (struct Internal_Data *) calloc(1, sizeof(struct Internal_Data));
	if (!wd) {
		if (win) evas_object_del(win);
		return NULL;
	}
	evas_object_data_set(win, PRIVATE_DATA_KEY_DATA, wd);

	wd->angle = 0;
	wd->w = ad->win.w;
	wd->h = ad->win.h;

	evas_object_smart_callback_add(win, "sub-object-del", _sub_del, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _noti_win_destroy, NULL);

	return win;
}

void noti_win_content_set(Evas_Object *obj, Evas_Object *content)
{
	struct Internal_Data *wd;

	ret_if (!obj);

	wd = evas_object_data_get(obj, PRIVATE_DATA_KEY_DATA);
	ret_if (!wd);

	if (wd->content && content != NULL) {
		evas_object_del(content);
		content = NULL;
	}

	wd->content = content;
	if (content) {
		evas_object_show(wd->content);
		evas_object_event_callback_add(wd->content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _content_changed_size_hints, obj);
	}
}
/*End of file */
