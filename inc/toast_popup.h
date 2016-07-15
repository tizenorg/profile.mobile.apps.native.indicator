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


#ifndef __INDICATOR_TOAST_POPUP_H___
#define __INDICATOR_TOAST_POPUP_H___

/**
 * @file toast_popup.h
 */

/**
 * @brief Initializes toast popup module.
 *
 * @param[in] data the app data
 *
 * @return 0 on success, negative value on failure
 */
int indicator_toast_popup_init(void *data);

/**
 * @brief Deinitializes toast popup module.
 *
 * @return 0 on success, negative value on failure
 */
int indicator_toast_popup_fini(void);

#endif /* __INDICATOR_TOAST_POPUP_H___ */
