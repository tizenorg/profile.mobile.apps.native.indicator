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


#ifndef __INDICATOR_BOX_UTIL_H__
#define __INDICATOR_BOX_UTIL_H__

#include <Elementary.h>
#include "indicator.h"

typedef enum _Icon_AddType {
	CANNOT_ADD = -1,
	CAN_ADD_WITH_DEL_SYSTEM,
	CAN_ADD_WITH_DEL_NOTI,
	CAN_ADD_WITHOUT_DEL,
} Icon_AddType;

typedef enum _Icon_Display_Count {
	BATTERY_TEXT_ON_COUNT = 0,
	BATTERY_TEXT_OFF_COUNT = 0,
	PORT_NONFIXED_ICON_COUNT = 6,
	LAND_NONFIXED_ICON_COUNT = 14,
	PORT_NOTI_ICON_COUNT = 4,
	LAND_NOTI_ICON_COUNT = 9,
	PORT_SYSTEM_ICON_COUNT = 2,
	LAND_SYSTEM_ICON_COUNT = 5,
} Icon_Display_Count;

extern int icon_box_pack(Indicator_Icon_Object *icon);
extern int icon_box_pack_append(Indicator_Icon_Object *icon);
extern int icon_box_unpack(Indicator_Icon_Object *icon);
extern int icon_box_init(win_info *win);
extern int icon_box_fini(win_info *win);
extern void indicator_util_update_display(win_info *win);
extern unsigned int indicator_get_count_in_fixed_list(int type);
extern unsigned int indicator_get_count_in_system_list(int type);
extern unsigned int indicator_get_count_in_noti_list(int type);
extern void indicator_set_count_in_non_fixed_list(int angle, int status);
extern int indicator_get_max_count_in_non_fixed_list(int type);
extern Icon_AddType indicator_is_enable_to_insert_in_non_fixed_list(Indicator_Icon_Object *obj);
extern int indicator_util_get_priority_in_move_area(win_info *win, Evas_Coord, Evas_Coord);
int indicator_util_check_indicator_area(win_info *win,Evas_Coord curr_x, Evas_Coord curr_y);
int indicator_util_check_home_icon_area(win_info *win,Evas_Coord curr_x, Evas_Coord curr_y);
void indicator_util_show_hide_icons(void* data, int bShow, int bEffect);
extern Eina_Bool indicator_util_is_show_icon(Indicator_Icon_Object *obj);
int indicator_util_handle_animated_gif(Indicator_Icon_Object *icon);

#endif /*__INDICATOR_BOX_UTIL_H__*/
