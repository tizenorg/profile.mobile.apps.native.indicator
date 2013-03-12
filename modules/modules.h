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


#ifndef __INDICATOR_MODULES_H__
#define __INDICATOR_MODULES_H__

#include <Ecore.h>
#include "indicator_ui.h"
#include "indicator_icon_list.h"

#define TIMER_STOP	ECORE_CALLBACK_CANCEL
#define TIMER_CONTINUE	ECORE_CALLBACK_RENEW

void indicator_init_modules(void *data);
void indicator_fini_modules(void *data);
void indicator_hib_enter_modules(void *data);
void indicator_hib_leave_modules(void *data);
void indicator_lang_changed_modules(void *data);
void indicator_region_changed_modules(void *data);

#endif
