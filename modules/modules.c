/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
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


#include "modules.h"

#define INDICATOR_MODULE_NUMBERS 22

extern Indicator_Icon_Object home[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object rssi[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object usb[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object wifi[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object mobile_hotspot[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object conn[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object sos[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object call[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object call_divert[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object mmc[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object noti[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object useralarm[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object mp3_play[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object voice_recorder[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object silent[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object bluetooth[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object gps[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object nfc[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object wifi_direct[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object sysclock[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object battery[INDICATOR_WIN_MAX];
extern Indicator_Icon_Object earphone[INDICATOR_WIN_MAX];


static Indicator_Icon_Object *modules[INDICATOR_WIN_MAX][INDICATOR_MODULE_NUMBERS] = {
{
	&sysclock[0],
	&battery[0],
	&wifi[0],
	&rssi[0],
	&sos[0],
	&usb[0],
	&mobile_hotspot[0],
	&conn[0],
	&call[0],
	&call_divert[0],
	&mmc[0],
	&noti[0],
	&useralarm[0],
	&mp3_play[0],
	&voice_recorder[0],
	&silent[0],
	&bluetooth[0],
	&gps[0],
	&nfc[0],
	&wifi_direct[0],
	&earphone[0],
	NULL
},
{
	&sysclock[1],
	&battery[1],
	&wifi[1],
	&rssi[1],
	&sos[1],
	&usb[1],
	&mobile_hotspot[1],
	&conn[1],
	&call[1],
	&call_divert[1],
	&mmc[1],
	&noti[1],
	&useralarm[1],
	&mp3_play[1],
	&voice_recorder[1],
	&silent[1],
	&bluetooth[1],
	&gps[1],
	&nfc[1],
	&wifi_direct[1],
	&earphone[1],
	NULL
}

};

void indicator_init_modules(void *data)
{
	int i;
	int j = 0;

	for(j=0;j<INDICATOR_WIN_MAX;j++)
	{
		for (i = 0; modules[j][i]; i++) {
			indicator_icon_list_insert(modules[j][i]);
			modules[j][i]->ad = data;
			if (modules[j][i]->init)
				modules[j][i]->init(data);
		}
	}
}

void indicator_fini_modules(void *data)
{
	int i;
	int j = 0;

	for(j=0;j<INDICATOR_WIN_MAX;j++)
	{
		for (i = 0; modules[j][i]; i++) {
			if (modules[j][i]->fini)
				modules[j][i]->fini();
		}
	}

	indicator_icon_all_list_free();
}

void indicator_lang_changed_modules(void *data)
{
	int i;
	int j = 0;

	for(j=0;j<INDICATOR_WIN_MAX;j++)
	{
		for (i = 0; modules[j][i]; i++) {
			if (modules[j][i]->lang_changed)
				modules[j][i]->lang_changed(data);
		}
	}
}

void indicator_region_changed_modules(void *data)
{
	int i;
	int j = 0;

	for(j=0;j<INDICATOR_WIN_MAX;j++)
	{
		for (i = 0; modules[j][i]; i++) {
			if (modules[j][i]->region_changed)
				modules[j][i]->region_changed(data);
		}
	}
}

void indicator_minictrl_control_modules(int action, const char* name, void *data)
{
	int i;
	int j = 0;

	for(j=0;j<INDICATOR_WIN_MAX;j++)
	{
		for (i = 0; modules[j][i]; i++) {
			if (modules[j][i]->minictrl_control)
				modules[j][i]->minictrl_control(action, name, data);
		}
	}
}

void indicator_wake_up_modules(void *data)
{
	int i;
	int j = 0;

	for(j=0;j<INDICATOR_WIN_MAX;j++)
	{
		for (i = 0; modules[j][i]; i++) {
			if (modules[j][i]->wake_up)
				modules[j][i]->wake_up(data);
		}
	}
}

