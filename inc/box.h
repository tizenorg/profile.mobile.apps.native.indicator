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


#ifndef __INDICATOR_BOX_UTIL_H__
#define __INDICATOR_BOX_UTIL_H__

#include <Elementary.h>

#include "indicator.h"

typedef enum _Icon_AddType {
	CANNOT_ADD = -1,
	CAN_ADD_WITH_DEL_SYSTEM,
	CAN_ADD_WITH_DEL_MINICTRL,
	CAN_ADD_WITH_DEL_NOTI,
	CAN_ADD_WITHOUT_DEL,
} Icon_AddType;

typedef enum _Icon_Display_Count {
	BATTERY_TEXT_ON_COUNT = 0,
	BATTERY_TEXT_OFF_COUNT = 0,
	PORT_NONFIXED_ICON_COUNT = 6,
	LAND_NONFIXED_ICON_COUNT = 6,
	PORT_SYSTEM_ICON_COUNT = 5,   // MIN : (1), MAX : (5)
	LAND_SYSTEM_ICON_COUNT = 5,
	PORT_MINICTRL_ICON_COUNT = 6, // MIN : (1), MAX : (6)
	LAND_MINICTRL_ICON_COUNT = 6,

	PORT_MINICTRL_ICON_MIN_COUNT = 1,
	PORT_NOTI_ICON_MIN_COUNT = 1,

	PORT_CONNECTION_SYSTEM_ICON_COUNT = 2, // MIN : (1), MAX : (2)
	LAND_CONNECTION_SYSTEM_ICON_COUNT = 2,
} Icon_Display_Count;

extern int box_add_icon_to_list(icon_s *icon);
extern int box_append_icon_to_list(icon_s *icon);
extern int box_remove_icon_from_list(icon_s *icon);
extern void box_init(win_info *win);
extern void box_fini(win_info *win);
extern unsigned int box_get_list_size(indicator_icon_area_type type);
extern int box_get_max_count_in_non_fixed_list(void);
extern Icon_AddType box_is_enable_to_insert_in_non_fixed_list(icon_s *obj);
extern int box_get_priority_in_move_area(win_info *win, Evas_Coord, Evas_Coord);
extern int box_check_indicator_area(win_info *win,Evas_Coord curr_x, Evas_Coord curr_y);
extern int box_check_home_icon_area(win_info *win,Evas_Coord curr_x, Evas_Coord curr_y);
extern Eina_Bool box_exist_icon(icon_s *obj);
extern int box_handle_animated_gif(icon_s *icon);
extern void box_noti_ani_handle(int bStart);
extern void box_icon_state_set(int bShow,char* file,int line);
extern int box_icon_state_get(void);
extern unsigned int box_get_count_in_noti_list_except_minictrl(void);
extern int box_check_more_icon_area(win_info *win, Evas_Coord curr_x, Evas_Coord curr_y);
extern void box_update_display(win_info *win);

#endif /*__INDICATOR_BOX_UTIL_H__*/
