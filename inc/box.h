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


/**
 * @file box.h
 */

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

typedef enum Box_List {
	FIXED_LIST = 0,
	SYSTEM_LIST,
	MINICTRL_LIST,
	NOTI_LIST,
	CONNECTION_SYSTEM_LIST
} Box_List;

/***
 * @brief Adds @a icon to list of icons to show.
 *
 * @param[in] icon the icon to add
 *
 * @return 0 on success, -1 otherwise
 */
extern int box_add_icon_to_list(icon_s *icon);

/***
 * @brief Appends @a icon to list of icons to show.
 *
 * @param[in] icon the icon to append
 *
 * @return 0 on success, -1 otherwise
 */
extern int box_append_icon_to_list(icon_s *icon);

/**
 * @brief Removes @a icon from list of icons to show.
 *
 * @param[in] icon the icon to remove
 *
 * @return 0 on success, -1 otherwise
 */
extern int box_remove_icon_from_list(icon_s *icon);

/**
 * @brief Initializes boxes for every particular icons area.
 *
 * @remarks It initializes empty boxes. No icons are added for now
 *
 * @param[in] win internal structure with win informations
 *
 */
extern void box_init(win_info *win);

/**
 * @brief Deinitializes icons area boxes.
 *
 * @param[in] win internal structure with win informations
 *
 */
extern void box_fini(win_info *win);

/**
 * @brief Retrieves list containing only icons to show.
 *
 * @param[in] type list type
 *
 * @return The list
 */

Eina_List *box_get_list(indicator_icon_area_type type);

/**
 * @brief Retrieves size of a given list.
 *
 * @param[in] type list type of list to get size
 *
 * @return The size of the list
 */
extern unsigned int box_get_list_size(indicator_icon_area_type type);

/**
 * @brief Retrieves count of max allowed number of non fixed icons.
 *
 * @return count of max allowed number of non fixed icons
 */
extern int box_get_max_count_in_non_fixed_list(void);

/**
 * @brief Checks for possibility of inserting icon in non fixed area.
 *
 * @param[in] icon icon to check possibility to insert
 *
 * @return type of adding type
 * @retval CANNOT_ADD
 * @retval CAN_ADD_WITH_DEL_SYSTEM
 * @retval CAN_ADD_WITH_DEL_MINICTRL
 * @retval CAN_ADD_WITH_DEL_NOTI
 * @retval CAN_ADD_WITHOUT_DEL
 */

extern Icon_AddType box_is_enable_to_insert_in_non_fixed_list(icon_s *obj);

/**
 * @brief Gets priority in move area.
 *
 * @param[in] win win info
 * @param[in] x x coordinate of the area
 * @param[in] y y coordinate of the area
 *
 * @return priority
 */
extern int box_get_priority_in_move_area(win_info *win, Evas_Coord x, Evas_Coord y);

/**
 * @brief Checks indicator area.
 *
 * @param[in] win win info
 * @param[in] curr_x x coordinate of the point
 * @param[in] curr_y y coordinate of the point
 *
 * @return 1 if specified point belongs to area, 0 otherwise
 */
extern int box_check_indicator_area(win_info *win,Evas_Coord curr_x, Evas_Coord curr_y);

/**
 * @brief Checks home icon area.
 *
 * @param[in] win win info
 * @param[in] curr_x x coordinate of the point
 * @param[in] curr_y y coordinate of the point
 *
 * @return 1 if specified point belongs to area, 0 otherwise
 */
extern int box_check_home_icon_area(win_info *win,Evas_Coord curr_x, Evas_Coord curr_y);

/**
 * @brief Checks if icon exists in related area.
 *
 * @param[in] obj icon to check
 *
 * @return EINA_TRUE if belongs, EINA_FALSE otherwise
 */
extern Eina_Bool box_exist_icon(icon_s *obj);

/**
 * @brief Handles animation.
 *
 * @param[in] icon icon which animation need to be handled
 *
 * @return EINA_TRUE if belongs, EINA_FALSE otherwise
 */
extern int box_handle_animated_gif(icon_s *icon);

/**
 * @brief Handles notification animation.
 *
 * @param[in] icon icon which animation need to be handled
 *
 */
extern void box_noti_ani_handle(int bStart);

/**
 * @brief Sets icon state.
 *
 * @param[in] bShow state to set
 * @param file NOT USED
 * @param line NOT USED
 *
 */
extern void box_icon_state_set(int bShow,char* file,int line);

/**
 * @brief Gets icon state.
 *
 * @return state of the icon
 */
extern int box_icon_state_get(void);

/**
 * @brief Gets count in noti list except minictrl.
 *
 * @return count of icons
 */
extern unsigned int box_get_count_in_noti_list_except_minictrl(void);

/**
 * @brief Gets enabled notification area icons count.
 *
 * @return count of icons
 */
extern int box_get_enabled_noti_count(void);

/**
 * @brief Checks more noti icon area
 *
 * @param[in] win win info
 * @param[in] curr_x x coordinate of the point
 * @param[in] curr_y y coordinate of the point
 *
 * @return 1 if specified point belongs to area, 0 otherwise
 */
extern int box_check_more_icon_area(win_info *win, Evas_Coord curr_x, Evas_Coord curr_y);

/**
 * @brief Updates boxes.
 *
 * @param[in] win win info
 *
 */
extern void box_update_display(win_info *win);

/**
 * @brief Creates box.
 *
 * @param[in] parent parent fo box container
 *
 * @return box
 *
 */
Evas_Object *box_add(Evas_Object *parent);

#endif /*__INDICATOR_BOX_UTIL_H__*/
