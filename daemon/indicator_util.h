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

#ifndef __INDICATOR_UTIL_H__
#define __INDICATOR_UTIL_H__

extern char *set_label_text_color(const char *txt);
extern const char *get_icon_dir(void);
void indicator_signal_emit(void* data, const char *emission, const char *source);
void indicator_part_text_emit(void* data, const char *part, const char *text);

#endif
