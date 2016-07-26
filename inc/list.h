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



#ifndef __INDICATOR_ICON_LIST_H__
#define __INDICATOR_ICON_LIST_H__

/**
 * @file list.h
 */

/**
 * @defgroup list List
 */

/**
 * @addtogroup list
 * @{
 */

/**
 * @brief Deletes evas object related to specified icon and frees all related data.
 *
 * @param[in] icon icon related to object to delete
 *
 */
extern void icon_free(icon_s *icon);

/**
 * @brief Deletes evas objects related with icons that were added to icon lists and frees all related data.
 *
 * @remarks The lists are of all icons registered to indicator app, not only visible icons
 *
 * @see icon_free() allows to free only one specified list
 */
extern void list_free_all(void);

/**
 * @brief Retrieves count of icons that are set to be visible on indicator.
 *
 * @remarks Number of visible icons may be different that returned value.
 * This is due to limited space on indicator and GUI Guide.
 *
 * @param[in] area type of icons are that uniquely defines list of icons
 *
 * @return count of icons that are set to be visible
 */
unsigned int list_get_active_icons_cnt(enum indicator_icon_area_type area);

/**
 * @brief Updates list related to the given icon with the icon.
 *
 * @remarks The icon is inserted to the specific list based on priority of the icon
 *
 * @param[in] icon icon to insert
 */
extern void list_update(icon_s *icon);

/**
 * @brief Inserts icon to the list related to the given icon.
 *
 * @remarks The icon is inserted to the specific list based on priority of the icon
 *
 * @param[in] icon icon to insert
 */
extern void list_insert_icon(icon_s *icon);

/**
 * @brief Removes icon from specific list related to the given icon.
 *
 * @remarks The icon is not deleted, only removed from list
 *
 * @param[in] icon icon to remove
 */
extern void list_remove_icon(icon_s *icon);

/**
 * @brief Searches for icon that is pending to be shown, but is not shown yet.
 *
 * @param[in] area area of icon - indicator_icon_area_type
 * @param[in] priority priority of icon
 *
 * @return pointer to icon to show, NULL if icon were not found
 */
extern icon_s *list_try_to_find_icon_to_show(int area, int priority);

/**
 * @brief Searches for icon to be removed from indicator.
 *
 * @remarks It searches for visible icon. In fixed are the icon is specified by priority.
 * In other areas icon with lowest priority will be choosen.
 *
 * @param[in] area area in which the icon must be removed - indicator_icon_area_type
 * @param[in] priority priority of icon to remove (only for INDICATOR_ICON_AREA_FIXED area)
 *
 * @return pointer to icon to show, NULL if icon were not found
 */
extern icon_s *list_try_to_find_icon_to_remove(int area, int priority);

/**
 * @brief Retrives visible notification icons count.
 *
 * @return number of visible notification icons
 */
extern unsigned int list_get_noti_count(void);

/**
 * @}
 */

#endif /*__INDICATOR_ICON_LIST_H__*/
