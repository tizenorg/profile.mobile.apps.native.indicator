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


#ifndef __INDICATOR_MODULES_H__
#define __INDICATOR_MODULES_H__

#include <Ecore.h>

#include "main.h"
#include "list.h"

#define TIMER_STOP	ECORE_CALLBACK_CANCEL
#define TIMER_CONTINUE	ECORE_CALLBACK_RENEW

extern void modules_init(void *data);
extern void modules_fini(void *data);
extern void modules_lang_changed(void *data);
extern void modules_region_changed(void *data);
extern void modules_minictrl_control(int action, const char* name, void *data);
extern void modules_wake_up(void *data);
extern void modules_init_first(void *data);

#ifdef _SUPPORT_SCREEN_READER
extern void modules_register_tts(void *data);
#endif
#endif
