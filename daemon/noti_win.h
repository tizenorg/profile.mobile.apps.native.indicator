/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
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


#ifndef __NOTI_WIN_H__
#define __NOTI_WIN_H__

#include <Evas.h>

typedef enum _indicator_animated_icon_type {
	INDICATOR_ANIMATED_ICON_NONE = -1,
	INDICATOR_ANIMATED_ICON_DOWNLOAD = 1,
	INDICATOR_ANIMATED_ICON_UPLOAD,
	INDICATOR_ANIMATED_ICON_INSTALL,
} indicator_animated_icon_type;

typedef struct _QP_Module {
	char *name;
	/* func */
	int (*init) (void *);
	void (*init_job_cb) (void *);
	int (*fini) (void *);
	int (*suspend) (void *);
	int (*resume) (void *);
	int (*hib_enter) (void *);
	int (*hib_leave) (void *);
	void (*lang_changed) (void *);
	void (*refresh) (void *);
	unsigned int (*get_height) (void *);
	void (*qp_opened) (void *);
	void (*qp_closed) (void *);
	void (*mw_enabled) (void *);
	void (*mw_disabled) (void *);

	/* do not modify this area */
	/* internal data */
	Eina_Bool state;
} QP_Module;

extern Evas_Object *noti_win_add(Evas_Object *parent, struct appdata *ad);
extern void noti_win_content_set(Evas_Object *obj, Evas_Object *content);

#endif
