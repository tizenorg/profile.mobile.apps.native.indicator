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



#include <vconf.h>

#include "main.h"
#include "common.h"
#include "util.h"
#include "icon.h"
#include "box.h"
#include "log.h"


#define MSG_TIMEOUT 3.0

int plmn_disp_type = 0;



static void disp_handle(void *data,int status)
{
	struct appdata *ad = NULL;

	ret_if(!data);

	ad = (struct appdata *)data;

	if(status == 1)
	{
		if(util_dynamic_state_get()==1)
		{
			util_signal_emit(ad,"indicator.plmn2.show","indicator.prog");
			util_signal_emit(ad,"indicator.plmn.hide","indicator.prog");
		}
		else
		{
			util_signal_emit(ad,"indicator.plmn.show","indicator.prog");
			util_signal_emit(ad,"indicator.plmn2.hide","indicator.prog");
		}

		if(plmn_disp_type==1)
		{
			return;
		}
		plmn_disp_type = 1;

		box_update_display(&(ad->win));

	}
	else
	{
		if(plmn_disp_type==0)
		{
			return;
		}
		plmn_disp_type = 0;
		if(util_dynamic_state_get()==1)
			util_signal_emit(ad,"indicator.plmn2.hide","indicator.prog");
		else
			util_signal_emit(ad,"indicator.plmn.hide","indicator.prog");
		box_update_display(&(ad->win));

	}
}



static void disp_plmn(const char *plmn, void *data)
{
	if(!util_is_orf())
	{
		disp_handle(data,0);
		return;
	}

	int disp = 0;
	int plmn_val = 0;

	retif(plmn == NULL, , "invalid plmn!");
	retif(data == NULL, , "invalid plmn!");

	vconf_get_int(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION, &disp);

	if(disp == VCONFKEY_TELEPHONY_DISP_INVALID || disp == VCONFKEY_TELEPHONY_DISP_SPN)
	{
		disp_handle(data,0);
		return;
	}

	vconf_get_int(VCONFKEY_TELEPHONY_PLMN,&plmn_val);

	if(plmn_val!=20801 && plmn_val!=20802)
	{
		disp_handle(data,0);
		return;
	}

	if(strcmp(plmn,"OrangeF")!=0)
	{
		return;
	}

	disp_handle(data,1);
}



void indicator_plmn_display(void *data)
{
	char* text = NULL;
	text = vconf_get_str(VCONFKEY_TELEPHONY_NWNAME);
	if(text!=NULL)
	{
	}

	disp_plmn(text,data);
}



static void indicator_plmn_nwname_cb(keynode_t *node, void *data)
{
	char* text = NULL;
	text = vconf_get_str(VCONFKEY_TELEPHONY_NWNAME);
	if(text!=NULL)
	{
		SECURE_DBG("indicator_plmn_nwname_cb %s",text);
	}

	disp_plmn(text,data);
}



static void indicator_plmn_disp_cond_cb(keynode_t *node, void *data)
{
	int ret = 0;
	int status = 0;
	ret = vconf_get_int(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION,&status);
	if(ret == -1)
	{
		return;
	}

	SECURE_DBG("indicator_plmn_disp_cond_cb %d",status);

	indicator_plmn_display(data);
}



static void indicator_plmn_cb(keynode_t *node, void *data)
{
	int ret = 0;
	int status = 0;
	ret = vconf_get_int(VCONFKEY_TELEPHONY_PLMN,&status);
	if(ret == -1)
	{
		return;
	}

	SECURE_DBG("indicator_plmn_cb %d",status);

	indicator_plmn_display(data);
}



int indicator_plmn_init(void *data)
{
	int r = 0, ret = -1;

	retif(data == NULL, FAIL, "Invalid parameter!");

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_NWNAME,
				       indicator_plmn_nwname_cb, data);
	if (ret != OK) {
		r = ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_PLMN,
				       indicator_plmn_cb, data);
	if (ret != OK) {
		r = ret;
	}

	ret = vconf_notify_key_changed(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION,
				       indicator_plmn_disp_cond_cb, data);
	if (ret != OK) {
		r = r | ret;
	}

	indicator_plmn_display(data);

	return ret;
}



int indicator_plmn_fini(void)
{
	int ret;

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_NWNAME,
				       indicator_plmn_nwname_cb);

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_PLMN,
					       indicator_plmn_cb);

	ret = vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SPN_DISP_CONDITION,
				       indicator_plmn_disp_cond_cb);

	return ret;
}
