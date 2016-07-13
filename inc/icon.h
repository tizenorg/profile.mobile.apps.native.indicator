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

extern void icon_show(icon_s *obj);
extern void icon_hide(icon_s *obj);
extern void icon_ani_set(icon_s *icon, enum indicator_icon_ani type);
extern char *icon_label_set(const char *buf, char *font_name,
							char *font_style, int font_size,
							void *data);
extern Eina_Bool icon_add(win_info *win, icon_s *icon);
extern Eina_Bool icon_del(icon_s *icon);
extern unsigned int icon_get_update_flag(void);
extern void icon_set_update_flag(unsigned int val);
extern void icon_reset_list(void);

Eina_Bool check_for_icons_overflow(void);
void check_to_show_more_noti(win_info *win, Eina_Bool overflow);

#endif /*__INDICATOR_ICON_UTIL_H__*/
