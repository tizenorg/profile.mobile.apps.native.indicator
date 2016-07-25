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


#ifndef __INDICATOR_ICON_UTIL_H__
#define __INDICATOR_ICON_UTIL_H__

#include <Elementary.h>

#include "indicator.h"
#include "main.h"

/**
 * @file icon.h
 */

#define MAX_NOTI_ICONS_PORT 7
#define MAX_NOTI_ICONS_LAND 7

#define SIGNAL_SIZE 32

enum {
	ICON_STATE_HIDDEN = 0,
	ICON_STATE_SHOWN
};

enum indicator_view_mode {
	INDICATOR_KEEP_VIEW = -1,
	INDICATOR_FULL_VIEW,
	INDICATOR_CLOCK_VIEW
};

/**
 * @brief Shows icon in related area.
 *
 * @param[in] obj icon to show
 */
extern void icon_show(icon_s *obj);

/**
 * @brief Hides icon in related area.
 *
 * @param[in] obj icon to hide
 */
extern void icon_hide(icon_s *obj);

/**
 * @brief Sets icon animation.
 *
 * @param[in] icon icon to show
 * @param[in] type animation type
 */
extern void icon_ani_set(icon_s *icon, enum indicator_icon_ani type);

/**
 * @brief Sets icon label.
 * @remarks will be removed
 */
extern char *icon_label_set(const char *buf, char *font_name,
							char *font_style, int font_size,
							void *data);

/**
 * @brief Creates icon object.
 *
 * @param[in] win win info
 * @param[in] icon icon to create object for
 *
 * @return EINA_TRUE in success, EINA_FALSE otherwise
 */
extern Eina_Bool icon_add(win_info *win, icon_s *icon);

/**
 * @brief Deletes icon object.
 *
 * @param[in] icon icon which object will be deleted
 *
 * @return EINA_TRUE in success, EINA_FALSE otherwise
 */
extern Eina_Bool icon_del(icon_s *icon);

/**
 * @brief Gets update flag.
 *
 * @return 1 if updated, 0 otherwise
 */
extern unsigned int icon_get_update_flag(void);

/**
 * @brief Sets update flag.
 *
 * @param[in] val flag value to set
 *
 */
extern void icon_set_update_flag(unsigned int val);

/**
 * @brief Resets lists of icons to show state.
 *
 */
extern void icon_reset_list(void);

/**
 * @brief Handles more noti icon.
 * @remarks The icon will be shown if more that PORT_NONFIXED_ICON_COUNT otherwise the icon will be hidden
 *
 * @param win win info
 *
 */
extern void icon_handle_more_notify_icon(win_info *win);

/**
 * @brief Makes util.
 *
 * @remarks Will be removed
 *
 */
extern void* icon_util_make(void *input);

/**
 * @brief Checks if non fixed icons count is greater than PORT_NONFIXED_ICON_COUNT.
 *
 * @return EINA_TRUE if icons count is greater then PORT_NONFIXED_ICON_COUNT, EINA_FALSE otherwise
 *
 */

Eina_Bool check_for_icons_overflow(void);

/**
 * @brief Checks to show more noti icon.
 *
 * @remarks Will be removed
 *
 * @param[in] win win info
 * @param[in] overflow indicates if there is to much icons pending to be shown
 *
 */
void check_to_show_more_noti(win_info *win, Eina_Bool overflow);

#endif /*__INDICATOR_ICON_UTIL_H__*/
