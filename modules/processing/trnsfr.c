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


#include <stdio.h>
#include <stdlib.h>
#include <vconf.h>
#include <Ecore_X.h>
#include "common.h"
#include "indicator.h"
#include "indicator_ui.h"
#include "modules.h"
#include "indicator_icon_util.h"

#define ICON_PRIORITY	INDICATOR_PRIORITY_NOTI_2
#define MODULE_NAME		"transfer"
#define TIMER_INTERVAL	0.03

static int register_trnsfr_module(void *data);
static int unregister_trnsfr_module(void);

Indicator_Icon_Object trnsfr[INDICATOR_WIN_MAX] = {
{
	.win_type = INDICATOR_WIN_PORT,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_trnsfr_module,
	.fini = unregister_trnsfr_module
},
{
	.win_type = INDICATOR_WIN_LAND,
	.type = INDICATOR_IMG_ICON,
	.name = MODULE_NAME,
	.priority = ICON_PRIORITY,
	.always_top = EINA_FALSE,
	.exist_in_view = EINA_FALSE,
	.txt_obj = {0,},
	.img_obj = {0,},
	.obj_exist = EINA_FALSE,
	.area = INDICATOR_ICON_AREA_NOTI,
	.init = register_trnsfr_module,
	.fini = unregister_trnsfr_module
}
};

static const char *icon_path[] = {
	"Processing/B03_Processing_01.png",
	"Processing/B03_Processing_02.png",
	"Processing/B03_Processing_03.png",
	"Processing/B03_Processing_04.png",
	"Processing/B03_Processing_05.png",
	"Processing/B03_Processing_06.png",
	"Processing/B03_Processing_07.png",
	"Processing/B03_Processing_08.png",
	"Processing/B03_Processing_09.png",
	"Processing/B03_Processing_10.png",
	"Processing/B03_Processing_11.png",
	"Processing/B03_Processing_12.png",
	"Processing/B03_Processing_13.png",
	"Processing/B03_Processing_14.png",
	"Processing/B03_Processing_15.png",
	"Processing/B03_Processing_16.png",
	"Processing/B03_Processing_17.png",
	"Processing/B03_Processing_18.png",
	"Processing/B03_Processing_19.png",
	"Processing/B03_Processing_20.png",
	"Processing/B03_Processing_21.png",
	"Processing/B03_Processing_22.png",
	"Processing/B03_Processing_23.png",
	"Processing/B03_Processing_24.png",
	"Processing/B03_Processing_25.png",
	"Processing/B03_Processing_26.png",
	"Processing/B03_Processing_27.png",
	"Processing/B03_Processing_28.png",
	"Processing/B03_Processing_29.png",
	"Processing/B03_Processing_30.png",
	NULL
};

static Ecore_Timer *timer = NULL;
static unsigned int process_req_cnt = 0;

static void delete_timer(void)
{
	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
	}
}

static void show_image_icon(void *data, int index)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		trnsfr[i].img_obj.data = icon_path[index];
		indicator_util_icon_show(&trnsfr[i]);
	}
}

static void hide_image_icon(void)
{
	int i = 0;

	for (i=0 ; i<INDICATOR_WIN_MAX ; i++)
	{
		indicator_util_icon_hide(&trnsfr[i]);
	}
}

static Eina_Bool indicator_trnsfr_transfer_cb(void *data)
{
	static int i = 0;

	retif(data == NULL, ECORE_CALLBACK_CANCEL, "Invalid parameter!");

	show_image_icon(data, i);
	i = (++i % 30) ? i : 0;

	return ECORE_CALLBACK_RENEW;
}

void show_trnsfr_icon(void *data)
{

	process_req_cnt++;
}

void hide_trnsfr_icon(void)
{
	process_req_cnt--;

	if (process_req_cnt <= 0) {
		process_req_cnt = 0;
		delete_timer();
	}
}

static int register_trnsfr_module(void *data)
{
	return OK;
}

static int unregister_trnsfr_module(void)
{
	delete_timer();
	process_req_cnt = 0;

	return OK;
}
