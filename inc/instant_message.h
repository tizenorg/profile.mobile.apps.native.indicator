/*
 * Indicator
 *
 * Copyright (c) 2000 - 2016 Samsung Electronics Co., Ltd. All rights reserved.
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


#ifndef __INDICATOR_INSTANT_MESSAGE_H_DEF__
#define __INDICATOR_INSTANT_MESSAGE_H_DEF__


struct message_s {
	char *icon_path;
	char *message;
	int private_id;
};

typedef struct message_s *instant_message_h;

void new_message(instant_message_h message);

#endif /* __INDICATOR_INSTANT_MESSAGE_H_DEF__ */
