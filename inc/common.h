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


#ifndef __DEF_common_H_
#define __DEF_common_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define OK		(0)
#define FAIL	(-1)

#ifdef _DLOG_USED
//#define LOG_TAG "INDICATOR"
#include <dlog.h>

#define ERR(str, args...)	LOGE("%s[%d]\t " #str "\n", \
					__func__, __LINE__, ##args)
#define DBG(str, args...)	LOGD("%s[%d]\t " #str "\n", \
					__func__, __LINE__, ##args)
#define INFO(str, args...)	LOGI(#str"\n", ##args)

#define SECURE_ERR(str, args...)	SECURE_LOGE("%s[%d]\t " #str "\n", \
					__func__, __LINE__, ##args)
#define SECURE_DBG(str, args...)	SECURE_LOGD("%s[%d]\t " #str "\n", \
					__func__, __LINE__, ##args)
#define SECURE_INFO(str, args...)	SECURE_LOGI(#str"\n", ##args)

#elif FILE_DEBUG /*_DLOG_USED*/
#include "indicator_debug_util.h"
#define ERR(str, args...)	debug_printf("%s[%d]\t " #str "\n", \
					__func__, __LINE__, ##args)
#define DBG(str, args...)	debug_printf("%s[%d]\t " #str "\n", \
					__func__, __LINE__, ##args)
#define INFO(str, args...)	debug_printf(#str"\n", ##args)
#else /*_DLOG_USED*/
#define ERR(str, args...)	fprintf(stderr, "%s[%d]\t " #str "\n",\
					__func__, __LINE__, ##args)
#define DBG(str, args...)	fprintf(stderr, "%s[%d]\t " #str "\n",\
					__func__, __LINE__, ##args)
#define INFO(str, args...)	fprintf(stderr, #str"\n", ##args)
#endif /*_DLOG_USED*/

#define retif(cond, ret, str, args...) do {\
	if (cond) { \
		ERR(str, ##args);\
		return ret;\
	} \
} while (0);
#define ECORE_FILE_MONITOR_DELIF(p) ({if (p) {ecore_file_monitor_del(p); p = NULL; }})

#define gotoif(cond, target, str, args...) do {\
	if (cond) { \
		DBG(str, ##args);\
		goto target;\
	} \
} while (0);

#endif
