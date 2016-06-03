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


#ifndef __INDICATOR_H__
#define __INDICATOR_H__

#include <Elementary.h>
//#include <Ecore_X.h>
#include <Ecore_File.h>
#include <Eina.h>
#include <stdbool.h>

#define VCONFKEY_APPTRAY_STATE "file/private/org.tizen.app-tray/is_top"

#define FIXED_COUNT	11

#define _INDICATOR_FEATURE_LITE
#define _INDICATOR_REMOVE_SEARCH

#define DATA_KEY_IMG_ICON "i_i"
#define DATA_KEY_BASE_RECT "dkbr"
#define DATA_KEY_TICKER_TEXT "dktt"

#define INDICATOR_MORE_NOTI "indicator_more_noti"

enum indicator_win_mode{
	INDICATOR_WIN_PORT = 0,
};


enum {
	/* Value is allocated from Left side in status bar */
	INDICATOR_PRIORITY_FIXED_MIN = 0,
	INDICATOR_PRIORITY_FIXED1 = INDICATOR_PRIORITY_FIXED_MIN,/* Always */
	INDICATOR_PRIORITY_FIXED2,		/* RSSI1 (elm.swallow.fixed1) */
	INDICATOR_PRIORITY_FIXED3,		/* RSSI2 (elm.swallow.fixed2) */
	INDICATOR_PRIORITY_FIXED4,		/* SimCard-Icon(DualSim) (elm.swallow.fixed3) */
	INDICATOR_PRIORITY_FIXED5,		/* Connection (elm.swallow.fixed4) */
	INDICATOR_PRIORITY_FIXED6,		/* Wifi (elm.swallow.fixed5) */
	INDICATOR_PRIORITY_FIXED7,		/* CONNECTION1 - BT (elm.swallow.fixed6) */
	INDICATOR_PRIORITY_FIXED8,		/* CONNECTION2 - WIFI DIRECT (elm.swallow.fixed7)*/
	INDICATOR_PRIORITY_FIXED9,		/* Battery (elm.swallow.fixed8)*/
	INDICATOR_PRIORITY_FIXED10,        /* Search*/
	INDICATOR_PRIORITY_FIXED11,        /* more */
	INDICATOR_PRIORITY_FIXED_MAX = INDICATOR_PRIORITY_FIXED11,

	/* Right Side */
	INDICATOR_PRIORITY_SYSTEM_MIN,
	INDICATOR_PRIORITY_SYSTEM_1 = INDICATOR_PRIORITY_SYSTEM_MIN, /* SYSTEM - Alarm */
	INDICATOR_PRIORITY_SYSTEM_2, /* SYSTEM - Call divert */
	INDICATOR_PRIORITY_SYSTEM_3, /* SYSTEM - MMC */
	INDICATOR_PRIORITY_SYSTEM_4,  /* SYSTEM - GPS */
	INDICATOR_PRIORITY_SYSTEM_5,  /* SYSTEM - Private mode */
	INDICATOR_PRIORITY_SYSTEM_6,  /* SYSTEM - Sound profile */
	INDICATOR_PRIORITY_SYSTEM_MAX = INDICATOR_PRIORITY_SYSTEM_6,

	INDICATOR_PRIORITY_MINICTRL_MIN,
	INDICATOR_PRIORITY_MINICTRL1 = INDICATOR_PRIORITY_MINICTRL_MIN, /* MINICTRL - Call-incomming/during call */
	INDICATOR_PRIORITY_MINICTRL2, /* MINICTRL - Call - Call Settings - Mute */
	INDICATOR_PRIORITY_MINICTRL3, /* MINICTRL - Call - Call Settings - Speaker on*/
	INDICATOR_PRIORITY_MINICTRL4, /* MINICTRL - 3rd Party app icon*/
	INDICATOR_PRIORITY_MINICTRL_MAX = INDICATOR_PRIORITY_MINICTRL4,

	INDICATOR_PRIORITY_NOTI_MIN,
	INDICATOR_PRIORITY_NOTI_1 = INDICATOR_PRIORITY_NOTI_MIN,
	INDICATOR_PRIORITY_NOTI_2, /* Ongoing/Normal notification */
	INDICATOR_PRIORITY_NOTI_MAX = INDICATOR_PRIORITY_NOTI_2
};

enum indicator_icon_type {
	INDICATOR_IMG_ICON = 0,
	INDICATOR_TXT_ICON,
	INDICATOR_TXT_WITH_IMG_ICON,
	INDICATOR_DIGIT_ICON,
};

enum indicator_icon_area_type {
	INDICATOR_ICON_AREA_FIXED = 0,
	INDICATOR_ICON_AREA_SYSTEM,
	INDICATOR_ICON_AREA_MINICTRL,
	INDICATOR_ICON_AREA_NOTI,
	INDICATOR_ICON_AREA_ALARM
};

typedef enum indicator_icon_ani {
	ICON_ANI_NONE = 0,
	ICON_ANI_BLINK,
	ICON_ANI_ROTATE,
	ICON_ANI_METRONOME,
	ICON_ANI_DOWNLOADING,
	ICON_ANI_UPLOADING,
	ICON_ANI_MAX
} Icon_Ani_Type;

enum indicator_digit_area {
	DIGIT_UNITY = 0,
	DIGIT_DOZENS,
	DIGIT_DOZENS_UNITY,
	DIGIT_HUNDREDS
};

/* Upload/download animation states 0-6 */
enum ud_indicator_icon_ani_state
{
	UD_ICON_ANI_STATE_0 = 0,
	UD_ICON_ANI_STATE_1,
	UD_ICON_ANI_STATE_2,
	UD_ICON_ANI_STATE_3,
	UD_ICON_ANI_STATE_4,
	UD_ICON_ANI_STATE_5,
	UD_ICON_ANI_STATE_6,
	UD_ICON_ANI_STATE_MAX
};

enum {
	TRANSFER_MIN = 0,
	TRANSFER_NONE = TRANSFER_MIN,
	TRANSFER_UP,
	TRANSFER_DOWN,
	TRANSFER_UPDOWN,
	TRANSFER_MAX
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
	Evas_Object* win;
	Evas_Object* layout;
	int w;
	int h;
	int port_w;
	int land_w;
	int angle;
	Evas_Object *_fixed_box[FIXED_COUNT];
	Evas_Object *_non_fixed_box;
	Evas_Object *_minictrl_box;
	Evas_Object *_noti_box;
	Evas_Object *_dynamic_box;
	Evas_Object *_dynamic_box_noti;
	Evas_Object *_alarm_box;
	Evas_Object *_digit_box;

	struct {
		int x;
		int y;
	} mouse_event;

	void* data;
}win_info;

typedef struct Indicator_Icon {
	char *name;
	enum indicator_icon_type type;
	enum indicator_icon_area_type area;
	int priority;
	Eina_Bool always_top;	/* Only for SOS messsage */

	enum ud_indicator_icon_ani_state animation_state;
	Eina_Bool animation_in_progress;
	Ecore_Timer *p_animation_timer;
	char signal_to_emit_prefix [32];
	double last_animation_timestamp;

	/* Func */
	int (*init) (void *);
	int (*fini) (void);
	int (*lang_changed) (void *);
	int (*region_changed) (void *);
	int (*minictrl_control) (int, const char *, void *);
	int (*wake_up) (void *);

	/* do not modify this area */
	/* internal data */
	void *ad;
	Eina_Bool obj_exist;
	Img_Icon_Info img_obj;
	Eina_Bool wish_to_show;
	Eina_Bool exist_in_view;
	enum indicator_icon_ani ani;

#ifdef _SUPPORT_SCREEN_READER
	int (*register_tts) (void*, int);
	char *(*access_cb)(void *, Evas_Object *);
	int tts_enable;
#endif
	int digit_area;
	Eina_Bool initialized; /* TRUE is module was correctly initialized */
} icon_s;

//int rssi_get_sim_number(TapiHandle *handle_obj);

#endif /*__INDICATOR_H__*/
