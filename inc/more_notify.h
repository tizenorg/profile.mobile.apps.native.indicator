/*
 *  Indicator
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved.
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


#ifndef MORE_NOTIFY_H_
#define MORE_NOTIFY_H_

/**
 * @file more_notify.h
 */

/**
 * @brief Shows or Hides more notify icon.
 *
 * @remarks The icon should be shown only if non fixed icon count exceeds PORT_NONFIXED_ICON_COUNT
 *
 * @param[in] val If EINA_TRUE, the icon will be shown, If EINA_FALSE, the icon will be hidden
 */
void indicator_more_notify_icon_change(Eina_Bool val);

#endif /* MORE_NOTIFY_H_ */
