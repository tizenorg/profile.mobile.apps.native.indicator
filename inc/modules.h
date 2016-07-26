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

/**
 * @file modules.h
 */


/**
 * @defgroup modules Modules
 */

/**
 * @addtogroup modules
 * @{
 */

#define TIMER_STOP	ECORE_CALLBACK_CANCEL
#define TIMER_CONTINUE	ECORE_CALLBACK_RENEW

/**
 * @brief Initializes and registers all modules(icons) specified in the sources.
 *
 * @param[in] data the app data
 */
extern void modules_init(void *data);

/**
 * @brief Deinitializes and deregisters all modules(icons) that were initialized.
 *
 * @param[in] data the app data
 */
extern void modules_fini(void *data);

/**
 * @brief Propagates language change info to all initialized modules.
 *
 * @param[in] data the app data
 */
extern void modules_lang_changed(void *data);

/**
 * @brief Propagates region change info to all initialized modules.
 *
 * @param[in] data the app data
 */
extern void modules_region_changed(void *data);

/**
 * @brief Propagates minicontrol action request to all initialized modules.
 *
 * @param[in] action action
 * @param[in] name name
 * @param[in] data the app data
 */
extern void modules_minictrl_control(int action, const char* name, void *data);

/**
 * @brief Propagates wake up info request to all initialized modules.
 *
 * @remarks It is invoked when power button is pressed and LCD goes on
 *
 * @param[in] data the app data
 */
extern void modules_wake_up(void *data);

/**
 * @brief Initializes and registers only part of modules(icons) specified in the sources.
 *
 * @remarks only most important modules are initialized.
 *
 * @param[in] data the app data
 */
extern void modules_init_first(void *data);

#ifdef _SUPPORT_SCREEN_READER
extern void modules_register_tts(void *data);
#endif

/**
 * @}
 */

#endif /* __INDICATOR_MODULES_H__ */

