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

extern void icon_free(icon_s *icon);
extern int list_free_all(void);
unsigned int list_get_active_icons_cnt(enum indicator_icon_area_type area);
extern void list_update(icon_s *obj);
extern void list_insert_icon(icon_s *obj);
extern void list_remove_icon(icon_s *obj);
extern icon_s *list_try_to_find_icon_to_show(int area, int priority);
extern icon_s *list_try_to_find_icon_to_remove(int area, int priority);
extern unsigned int list_get_noti_count(void);

#endif /*__INDICATOR_ICON_LIST_H__*/
