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
#include <app_preference.h>

#include "common.h"
#include "indicator.h"
#include "icon.h"
#include "modules.h"
#include "main.h"
#include "log.h"
#include "util.h"
#include "box.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_MIN
#define MODULE_NAME		"more_notify"

#define PART_NAME_MORE_NOTI "elm.swallow.more_noti"

static int register_more_notify_module(void *data);
static int unregister_more_notify_module(void);

icon_s more_notify = {
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_TRUE,
	.exist_in_view = EINA_FALSE,
	.img_obj = {0,},
	.img_obj.data = "Notify/B03_notify_more.png",
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_MORE_NOTI,
	.init = register_more_notify_module,
	.fini = unregister_more_notify_module
};

enum {
	MUSIC_PLAY,
	MUSIC_PAUSED,
};

static void set_app_state(void *data)
{
	more_notify.ad = data;
}

void indicator_more_notify_icon_change(Eina_Bool val)
{
	_D("indicator_more_notify_change. Val=%s", (val) ? "true" : "false");

	struct appdata *ad = more_notify.ad;
	retm_if(ad == NULL, "Invalid parameter!");

	if (val) {
		util_signal_emit(ad->win.data, "indicator.more_noti.show", "indicator.prog");
		evas_object_show(more_notify.img_obj.obj);
	} else {
		util_signal_emit(ad->win.data, "indicator.more_noti.hide", "indicator.prog");
		evas_object_hide(more_notify.img_obj.obj);
	}

	return;
}

Evas_Object *icon_create_and_swallow(icon_s *icon, const char *part_name)
{
	struct appdata *ad = (struct appdata *)icon->ad;
	retv_if(!ad, NULL);

	Evas_Object *obj = NULL;

	icon_add(&ad->win, icon);
	retv_if(!icon->img_obj.obj, NULL);

	obj = box_add(ad->win.layout);
	retv_if(!obj, NULL);

	elm_box_pack_end(obj, icon->img_obj.obj);

	evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_part_swallow(elm_layout_edje_get(ad->win.layout), part_name, obj);

	return obj;
}

static int register_more_notify_module(void *data)
{
	_D("register_more_notify_module");

	retvm_if(data == NULL, FAIL, "Invalid parameter!");

	struct appdata *ad = (struct appdata *)data;

	set_app_state(data);

	ad->win._more_noti_box = icon_create_and_swallow(&more_notify, PART_NAME_MORE_NOTI);

	return OK;
}


static int unregister_more_notify_module(void)
{
	struct appdata *ad = (struct appdata *)more_notify.ad;

	icon_del(&more_notify);
	edje_object_part_unswallow(ad->win.layout, ad->win._more_noti_box);

	return OK;
}
