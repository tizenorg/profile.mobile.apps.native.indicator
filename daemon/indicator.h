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


#ifndef __INDICATOR_H__
#define __INDICATOR_H__

#include <Elementary.h>
#include <Ecore_X.h>
#include <Eina.h>

#define VCONF_INDICATOR_HOME_PRESSED "memory/private/"PACKAGE_NAME"/home_pressed"

#ifndef VCONFKEY_INDICATOR_STARTED
#define VCONFKEY_INDICATOR_STARTED "memory/private/"PACKAGE_NAME"/started"
#endif

#define _FIXED_COUNT	5


enum indicator_win_mode{
	INDICATOR_WIN_PORT = 0,
	INDICATOR_WIN_LAND,
	INDICATOR_WIN_MAX
};


enum {
	INDICATOR_PRIORITY_FIXED_MIN = 0,
	INDICATOR_PRIORITY_FIXED1 = INDICATOR_PRIORITY_FIXED_MIN,
	INDICATOR_PRIORITY_FIXED2,
	INDICATOR_PRIORITY_FIXED3,
	INDICATOR_PRIORITY_FIXED4,
	INDICATOR_PRIORITY_FIXED5,
	INDICATOR_PRIORITY_FIXED6,
	INDICATOR_PRIORITY_FIXED_MAX = INDICATOR_PRIORITY_FIXED6,
	INDICATOR_PRIORITY_SYSTEM_MIN,

	INDICATOR_PRIORITY_SYSTEM_6 = INDICATOR_PRIORITY_SYSTEM_MIN,
	INDICATOR_PRIORITY_SYSTEM_5,
	INDICATOR_PRIORITY_SYSTEM_4,
	INDICATOR_PRIORITY_SYSTEM_3,
	INDICATOR_PRIORITY_SYSTEM_2,
	INDICATOR_PRIORITY_SYSTEM_1,
	INDICATOR_PRIORITY_SYSTEM_MAX = INDICATOR_PRIORITY_SYSTEM_1,
	INDICATOR_PRIORITY_NOTI_MIN,
	INDICATOR_PRIORITY_NOTI_2 = INDICATOR_PRIORITY_NOTI_MIN,
	INDICATOR_PRIORITY_NOTI_1,
	INDICATOR_PRIORITY_NOTI_MAX = INDICATOR_PRIORITY_NOTI_2
};

enum indicator_icon_type {
	INDICATOR_IMG_ICON = 0,
	INDICATOR_TXT_ICON,
	INDICATOR_TXT_WITH_IMG_ICON
};

enum indicator_icon_area_type {
	INDICATOR_ICON_AREA_FIXED = 0,
	INDICATOR_ICON_AREA_SYSTEM,
	INDICATOR_ICON_AREA_NOTI
};

enum indicator_icon_ani {
	ICON_ANI_NONE = 0,
	ICON_ANI_BLINK,
	ICON_ANI_ROTATE,
	ICON_ANI_METRONOME,
	ICON_ANI_MAX
};

typedef struct _Text_Icon_Info {
	char *data;
	Evas_Object *obj;
	int width;
} Text_Icon_Info;

typedef struct _Img_Icon_Info {
	const char *data;
	Evas_Object *obj;
	int width;
	int height;
} Img_Icon_Info;

typedef struct _ind_win_info
{
	Evas *evas;
	Evas_Object* win_main;
	Evas_Object* layout_main;
	int w;
	int h;
	int angle;
	int type;
	Evas_Object *_fixed_box[_FIXED_COUNT];
	Evas_Object *_non_fixed_box;
	Evas_Object *_noti_box;

	struct {
	Eina_Bool trigger;
	int x;
	int y;
	} mouse_event;

#ifdef HOME_KEY_EMULATION
	Ecore_X_Atom atom_hwkey;
	Ecore_X_Window win_hwkey;
#endif /* HOME_KEY_EMULATION */
	void* data;
}win_info;


typedef struct Indicator_Icon {
	char *name;
	enum indicator_icon_type type;
	enum indicator_icon_area_type area;
	int priority;
	Eina_Bool always_top;


	int (*init) (void *);
	int (*fini) (void);
	int (*hib_enter) (void);
	int (*hib_leave) (void *);
	int (*lang_changed) (void *);
	int (*region_changed) (void *);
	int (*minictrl_control) (int, const char *, void *);

	char data[1024];
	void *ad;
	Eina_Bool obj_exist;
	Text_Icon_Info txt_obj;
	Img_Icon_Info img_obj;
	Eina_Bool wish_to_show;
	Eina_Bool exist_in_view;

	int win_type;
	enum indicator_icon_ani ani;
} Indicator_Icon_Object;

#endif /*__INDICATOR_H__*/
