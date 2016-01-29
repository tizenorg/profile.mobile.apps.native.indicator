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



#ifndef _INDICATOR_REMOVE_SEARCH
#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>
#include <app_preference.h>

#include "common.h"
#include "indicator.h"
#include "main.h"
#include "modules.h"
#include "icon.h"
#include "indicator_gui.h"
#include "util.h"
#include "log.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_FIXED10
#define MODULE_NAME		"search"

static int register_search_module(void *data);
static int unregister_search_module(void);
#ifdef _SUPPORT_SCREEN_READER
static int register_search_tts(void *data);
#endif
#define EXPORT_PUBLIC __attribute__ ((visibility ("default")))

icon_s search = {
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.img_obj = {0,0,FIXED4_ICON_WIDTH,FIXED4_ICON_HEIGHT},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_FIXED,
	.exist_in_view = EINA_FALSE,
	.init = register_search_module,
	.fini = unregister_search_module,
#ifdef _SUPPORT_SCREEN_READER
	.register_tts = register_search_tts,
#endif
};

static const char *icon_path[] = {
	"B03_search.png",
	"B03_search_press.png",
	NULL
};



static void set_app_state(void* data)
{
	search.ad = data;
}



static void show_image_icon(int index)
{
	search.img_obj.data = icon_path[index];
	icon_show(&search);
}



static void hide_image_icon(void)
{
	icon_hide(&search);
}



EXPORT_PUBLIC void hide_search_icon(void)
{
	hide_image_icon();
}



EXPORT_PUBLIC void show_search_icon(void)
{
	show_image_icon(0);
}

static void _handle_search_icon(void* data)
{
	int lock_status = -1;
	int ps_mode = -1;
	int bHide = 0;
	struct appdata *ad = (struct appdata *)data;

	retif(data == NULL, , "Invalid parameter!");

	vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock_status);
	vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &ps_mode);

	DBG("_indicator_lock_status_cb!!(%d)(%d)",lock_status,ps_mode);

	if(lock_status==VCONFKEY_IDLE_LOCK || ps_mode == SETTING_PSMODE_EMERGENCY)
	{
		bHide = 1;
	}
	else
	{
		bHide = 0;
	}

	if (bHide==0)
	{
		DBG("_lock_status_cb : show search!");
		show_image_icon(0);
		util_signal_emit_by_win(&(ad->win),"indicator.lock.off", "indicator.prog");
	}
	else
	{
		DBG("_lock_status_cb : hide search");
		util_signal_emit_by_win(&(ad->win), "indicator.lock.on", "indicator.prog");
		hide_image_icon();
	}
}



static void _ps_mode_cb(keynode_t *node, void *data)
{
	DBG("Ps mode change");
	_handle_search_icon(data);
}



static void _lock_status_cb(keynode_t *node, void *data)
{
	DBG("lock state change");
	_handle_search_icon(data);
}



static int register_search_module(void *data)
{
	retv_if(!data, 0);

	set_app_state(data);

	vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE,  _lock_status_cb, (void *)data);

	vconf_notify_key_changed(VCONFKEY_SETAPPL_PSMODE,  _ps_mode_cb, (void *)data);

	_handle_search_icon(data);
	return 0;
}



static int unregister_search_module(void)
{
	vconf_ignore_key_changed(VCONFKEY_IDLE_LOCK_STATE, _lock_status_cb);
	vconf_ignore_key_changed(VCONFKEY_SETAPPL_PSMODE, _ps_mode_cb);

	return 0;
}



#ifdef _SUPPORT_SCREEN_READER
static void _apptray_access_cb(void *data, Evas_Object *obj, Elm_Object_Item *item)
{
	util_launch_search(data);
}



static char *_access_info_cb(void *data, Evas_Object *obj)
{
	Evas_Object *item = data;
	char *tmp = NULL;
	char buf[256] = {0,};
	snprintf(buf, sizeof(buf), "%s, %s, %s", _("IDS_IDLE_SK_SMARTSEARCH_SEARCH"),_("IDS_COM_BODY_BUTTON_T_TTS"), _("IDS_IDLE_BODY_STATUS_BAR_ITEM"));

	tmp = strdup(buf);
	if (!tmp) return NULL;
	return tmp;
}



static int register_search_tts(void *data)
{
	int r = 0, ret = -1;

	retv_if(!data, 0);

	Evas_Object *to = NULL;
	Evas_Object *ao = NULL;
	struct appdata *ad = data;

	to = (Evas_Object *) edje_object_part_object_get(elm_layout_edje_get(ad->win.layout), "elm.swallow.fixed6.access");
	ao = util_access_object_register(to, ad->win.layout);
	util_access_object_info_cb_set(ao,ELM_ACCESS_INFO,_access_info_cb,data);
	util_access_object_activate_cb_set(ao,_apptray_access_cb,data);

	return 0;
}
#endif /* _SUPPORT_SCREEN_READER */
#endif /* _INDICATOR_REMOVE_SEARCH */
