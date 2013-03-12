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

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "indicator_icon_util.h"
#include "indicator_icon_list.h"
#include "indicator_ui.h"
#include "indicator_test_util.h"

int print_indicator_icon_object(Indicator_Icon_Object *obj)
{
#ifdef DEBUG_MODE
	retif(cond, ret, str, args...)(obj == NULL, FAIL, "Invalid parameter!");

	INFO(str, args...)(%s : priority(%d) obj(%x), obj->name, obj->priority,
			   (unsigned int)obj->obj);
#endif
	return OK;
}

int print_indicator_icon_list(Eina_List *list)
{
#ifdef DEBUG_MODE
	Eina_List *l;
	void *data;

	retif(list == NULL, FAIL, "Invalid parameter!");

	INFO("*******Indicator_Icon List(%x) *******", (unsigned int)list);
	EINA_LIST_FOREACH(list, l, data) {
		if (data) {
			print_indicator_icon_object(data);
		}
	}
#endif
	return OK;
}
