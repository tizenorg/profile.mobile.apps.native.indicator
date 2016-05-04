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
 *
 *  Created on: Apr 30, 2016
 *      Author: r.czerski
 */

#include <Elementary.h>
#include <Ecore_File.h>
#include <Eina.h>

#include "instant_message.h"

static Eina_List *messages_list;

static Ecore_Timer *message_timer = NULL;

static void split_message_text_into_lines(instant_message_h message)
{
	/*
	 * - check the width of the screen
	 * -
	 */
}

static void prepare_icon()
{
	/*
	 * prepare icon and set it to proper part content
	 */
}

static void prepare_line()
{
	/*
	 * prepare line and set it to proper part content
	 */
}

static void display_message(instant_message_h message)
{

	/*
	 * Check how many messages are waiting to be displayed and decide what to do according to Guide
	 * -
	 * -  <= 2 - display next message immediately(no matter how many lines left).
	 * -   > 2 - Display only newest message and skip remains.
	 *
	 *
	 */
	split_message_text_into_lines(message);

	prepare_icon();
	prepare_line();

	//emit signal to display first line(linear transition from top to position of indicator)

	//set message_timer if necessary

}

static void add_message_to_queue(instant_message_h message)
{
	/*
	 * add message to messages_list
	 */
}

static void remove_message_from_queue(instant_message_h message)
{
	/*
	 * remove message from messages_list
	 */
}

static Eina_Bool is_messages_list_empty(void)
{
	/*
	 *  Check if messages_list is empty
	 */
	return EINA_FALSE;
}

void new_message(instant_message_h message)
{
	/*
	 * - Add message to queue(messages_list)
	 * - Check if any other message is displayed at the moment(if there are any other messages in the queue)
	 * - - if not, display current message
	 * - - if so, check if message_timer is alive( != NULL)
	 * - - - if not, display current message
	 * - - - if so, return
	 *
	 */
}



