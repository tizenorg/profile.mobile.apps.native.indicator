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


#ifndef __INDICATOR_ICON_LIST_H__
#define __INDICATOR_ICON_LIST_H__

#include "indicator.h"
extern void indicator_icon_object_free(Indicator_Icon_Object *icon);
extern int indicator_icon_all_list_free(void);
extern int indicator_icon_list_insert(Indicator_Icon_Object *obj);
extern int indicator_icon_list_remove(Indicator_Icon_Object *obj);
extern int indicator_icon_list_update(Indicator_Icon_Object *obj);
extern Indicator_Icon_Object
*indicator_get_wish_to_show_icon(int win_type, int area, int priority);
extern Indicator_Icon_Object
*indicator_get_wish_to_remove_icon(int win_type, int area, int priority);
extern Indicator_Icon_Object
*indicator_get_lowest_priority_icon_in_non_fixed_list(int area);
#endif /*__INDICATOR_ICON_LIST_H__*/
