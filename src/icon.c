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


#include <Eina.h>
#include <vconf.h>
#include <app_preference.h>

#include "common.h"
#include "box.h"
#include "icon.h"
#include "list.h"
#include "main.h"
#include "indicator_gui.h"
#include "util.h"
#include "log.h"

#define PRIVATE_DATA_KEY_ICON_B_ANI "p_i_ba"

extern int current_angle;

#define ON_TIMER_ICON_ANIMATION_FRAME_TIME 0.3
#define UPLOAD_ICON_ANIMATION_SIGNAL 	"indicator.ani.uploading.%d"
#define DOWNLOAD_ICON_ANIMATION_SIGNAL "indicator.ani.downloading.%d"

static unsigned int update_icon_flag = 1;	// For battery problem



static void _reset_on_timer_icon_animation(icon_s *icon)
{
	ret_if(!icon);

	if (icon->p_animation_timer) {
		ecore_timer_del(icon->p_animation_timer);
		icon->p_animation_timer = NULL;
	}
	icon->animation_in_progress = EINA_FALSE;
	icon->last_animation_timestamp = ecore_time_unix_get();
	icon->signal_to_emit_prefix[0] = '\0';
	icon->animation_state = UD_ICON_ANI_STATE_0;
}



static Eina_Bool _animate_on_timer_cb(void *data)
{
	icon_s *icon = NULL;

	retv_if(!data, ECORE_CALLBACK_CANCEL);

	icon = (icon_s *)data;

	if (icon->animation_in_progress == EINA_FALSE) {
		icon->p_animation_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	if ((ecore_time_unix_get() - icon->last_animation_timestamp) < ON_TIMER_ICON_ANIMATION_FRAME_TIME) {
		return ECORE_CALLBACK_RENEW;
	}

	Evas_Object *img_edje = elm_layout_edje_get(icon->img_obj.obj);
	retv_if(!img_edje, ECORE_CALLBACK_CANCEL);

	char signal_to_emit[32] = {'\0',};
	sprintf(signal_to_emit,icon->signal_to_emit_prefix,icon->animation_state);

	edje_object_signal_emit(img_edje, signal_to_emit,"prog");

	if (icon->animation_state == UD_ICON_ANI_STATE_MAX) {
		icon->animation_state = UD_ICON_ANI_STATE_0;
	} else {
		icon->animation_state++;
	}
	icon->last_animation_timestamp = ecore_time_unix_get();

	return ECORE_CALLBACK_RENEW;
}



static const char *_icon_ani_type_set_send_signal(icon_s *icon, Icon_Ani_Type type)
{
	retv_if(!icon, NULL);

	const char *BLINK_SIGNAL = "icon,state,blink";
	const char *ROATATE_SIGNAL = "icon,state,rotate";
	const char *METRONOME_SIGNAL = "icon,state,metronome";
	const char *DEFAULT = "icon,state,default";
	const char *send_signal = DEFAULT;

	switch (type) {
	case ICON_ANI_BLINK:
		send_signal = BLINK_SIGNAL;
		break;
	case ICON_ANI_ROTATE:
		send_signal = ROATATE_SIGNAL;
		break;
	case ICON_ANI_METRONOME:
		send_signal = METRONOME_SIGNAL;
		break;
	case ICON_ANI_DOWNLOADING:
	case ICON_ANI_UPLOADING:
		/* If this icon is already animated during download/upload we don't have to set timer again */
		if (icon->animation_in_progress == EINA_FALSE) {
			_reset_on_timer_icon_animation(icon);
			send_signal = "dummy.signal";

			if (type == ICON_ANI_DOWNLOADING) {
				strncpy(icon->signal_to_emit_prefix, DOWNLOAD_ICON_ANIMATION_SIGNAL, sizeof(DOWNLOAD_ICON_ANIMATION_SIGNAL));
			}

			if (type == ICON_ANI_UPLOADING) {
				strncpy(icon->signal_to_emit_prefix, UPLOAD_ICON_ANIMATION_SIGNAL,sizeof(UPLOAD_ICON_ANIMATION_SIGNAL));
			}
			icon->animation_in_progress = EINA_TRUE;
			icon->p_animation_timer = ecore_timer_add(ON_TIMER_ICON_ANIMATION_FRAME_TIME,_animate_on_timer_cb, icon);
		}
		break;
	default:
		break;
	}

	return send_signal;
}



void icon_ani_set(icon_s *icon, Icon_Ani_Type type)
{
	Evas_Object *img_edje = NULL;
	const char *send_signal = NULL;

	ret_if(!icon);

	icon->ani = type;

	if (!icon->obj_exist) return;

	send_signal = _icon_ani_type_set_send_signal(icon, type);
	ret_if(!send_signal);

	switch (icon->type) {
	case INDICATOR_IMG_ICON:
		img_edje = elm_layout_edje_get(icon->img_obj.obj);
		edje_object_signal_emit(img_edje, send_signal,"elm.swallow.icon");
		break;
	case INDICATOR_TXT_ICON:
		break;
	case INDICATOR_TXT_WITH_IMG_ICON:
		break;
	case INDICATOR_DIGIT_ICON:
		img_edje = elm_layout_edje_get(icon->img_obj.obj);
		edje_object_signal_emit(img_edje, send_signal,"elm.swallow.icon");
		break;
	default:
		break;
	}
}



static void _fixed_icon_layout_file_set(icon_s *icon, Evas_Object *ly)
{
	ret_if(!icon);
	ret_if(!ly);

	if(icon->type == INDICATOR_DIGIT_ICON && icon->digit_area == DIGIT_DOZENS) {
		elm_layout_file_set(ly, ICON_THEME_FILE,"elm/indicator/icon/dozen_digit");
	} else {
		elm_layout_file_set(ly, ICON_THEME_FILE,"elm/indicator/icon/base");
	}
}



static void _noti_ani_icon_layout_file_set(int noti_is_ani, Evas_Object *ly)
{
	ret_if(!ly);

	if (noti_is_ani) {
		evas_object_data_set(ly, PRIVATE_DATA_KEY_ICON_B_ANI, (void *) 1);
		elm_layout_file_set(ly, ICON_NONFIXED_THEME_ANI_FILE, "elm/indicator/icon/base");
	} else {
		elm_layout_file_set(ly, ICON_NONFIXED_THEME_FILE, "elm/indicator/icon/base");
	}
}



static Evas_Object *_img_icon_add(win_info *win, icon_s *icon)
{
	char path[PATH_MAX];
	Evas_Object *evas_icon;
	Evas_Object *ly;
	char *imgpath = NULL;
	int noti_is_ani = 0;
	int b_ani = 0;

	retv_if(!win, NULL);
	retv_if(!icon, NULL);

	imgpath = (char *) icon->img_obj.data;

	_reset_on_timer_icon_animation(icon);

	if (icon->img_obj.width <= 0) {
		icon->img_obj.width = DEFAULT_ICON_WIDTH;
	}

	if (icon->img_obj.height <= 0) {
		icon->img_obj.height = DEFAULT_ICON_HEIGHT;
	}

	memset(path, 0x00, sizeof(path));

	ly = elm_layout_add(win->layout);
	retv_if(!ly, NULL);

	if (icon->area == INDICATOR_ICON_AREA_FIXED) {
		_fixed_icon_layout_file_set(icon, ly);
	} else {
		noti_is_ani = util_check_noti_ani(imgpath);
		_noti_ani_icon_layout_file_set(noti_is_ani, ly);
	}

	evas_icon = elm_image_add(ly);
	retv_if(!evas_icon, NULL);

	b_ani = (int) evas_object_data_get(ly, PRIVATE_DATA_KEY_ICON_B_ANI);
	if (!b_ani) {
		/* Absolute path? */
		if (strncmp(imgpath, "/", 1) != 0) {
			snprintf(path, sizeof(path), "%s/%s", util_get_icon_dir(), imgpath);
		} else {
			strncpy(path, imgpath, sizeof(path)-1);
		}

		if (!ecore_file_exists(path)) {
			_E("icon file does not exist : %s", path);
		}
		elm_image_file_set(evas_icon, path, NULL);
	}

	evas_object_size_hint_min_set(evas_icon, ELM_SCALE_SIZE(icon->img_obj.width), ELM_SCALE_SIZE(icon->img_obj.height));
	elm_object_part_content_set(ly, "elm.swallow.icon", evas_icon);

	evas_object_data_set(ly, DATA_KEY_IMG_ICON, evas_icon);
	evas_object_size_hint_min_set(ly, ELM_SCALE_SIZE(icon->img_obj.width), ELM_SCALE_SIZE(icon->img_obj.height));
	evas_object_hide(ly);

	return ly;
}



char *icon_label_set(const char *buf, char *font_name, char *font_style, int font_size, void *data)
{
	Eina_Strbuf *temp_buf = NULL;
	char *ret_str = NULL;
	char *label_font = ICON_FONT_NAME;
	char *label_font_style = ICON_FONT_STYLE;
	int label_font_size = ICON_FONT_SIZE;
	Eina_Bool buf_result = EINA_FALSE;

	retif(data == NULL || buf == NULL, NULL, "Invalid parameter!");

	temp_buf = eina_strbuf_new();
	if (font_name != NULL)
		label_font = font_name;
	if (font_style != NULL)
		label_font_style = font_style;
	if (font_size > 0)
		label_font_size = font_size;

	buf_result = eina_strbuf_append_printf(temp_buf, CUSTOM_LABEL_STRING,
						   label_font, label_font_style,
						   label_font_size, buf);

	if (buf_result != EINA_FALSE)
		ret_str = eina_strbuf_string_steal(temp_buf);

	eina_strbuf_free(temp_buf);

	return ret_str;
}



Eina_Bool icon_add(win_info *win, icon_s *icon)
{
	retv_if(!icon, EINA_FALSE);

	switch (icon->type) {
	case INDICATOR_TXT_ICON:
		break;
	case INDICATOR_IMG_ICON:
		icon->img_obj.obj = _img_icon_add(win, icon);
		break;
	case INDICATOR_TXT_WITH_IMG_ICON:
		break;
	case INDICATOR_DIGIT_ICON:
		icon->img_obj.obj = _img_icon_add(win, icon);
		break;
	default:
		_E("Icon type check error!");
		return EINA_FALSE;
	}
	icon->obj_exist = EINA_TRUE;

	return EINA_TRUE;
}



Eina_Bool icon_del(icon_s *icon)
{
	Evas_Object *icon_obj;
	retif(icon == NULL, EINA_FALSE, "Invalid parameter!");

	_reset_on_timer_icon_animation(icon);

	if (icon->obj_exist != EINA_FALSE) {
		if (icon->img_obj.obj) {
			icon_obj =
				evas_object_data_get(icon->img_obj.obj, DATA_KEY_IMG_ICON);
			evas_object_del(icon_obj);
			evas_object_del(icon->img_obj.obj);
			icon->img_obj.obj = NULL;
		}
	}
	return EINA_TRUE;
}



/******************************************************************************
 *
 * Static functions : util functions - check priority
 *
 *****************************************************************************/

static int _show_others_in_same_priority(icon_s *icon)
{
	icon_s *wish_add_icon;
	int area = icon->area;
	retif(icon == NULL, FAIL, "Invalid parameter!");

	wish_add_icon = list_try_to_find_icon_to_show(icon->area, icon->priority);
	if (wish_add_icon == NULL)
	{
		return OK;
	}

	if (box_exist_icon(wish_add_icon))
	{
		/* Already shown icon */
		return OK;
	}

	if(area ==INDICATOR_ICON_AREA_NOTI)
	{
		box_pack_append(wish_add_icon);
	}
	else
	{
		box_pack(wish_add_icon);
	}

	return OK;
}



static int _hide_others_in_view_list(icon_s *icon)
{
	icon_s *wish_remove_icon = NULL;
	retif(icon == NULL, FAIL, "Invalid parameter!");

	if (INDICATOR_ICON_AREA_SYSTEM == icon->area || INDICATOR_ICON_AREA_NOTI == icon->area || INDICATOR_ICON_AREA_MINICTRL == icon->area)
	{
		Icon_AddType ret;

		/* In Case of Nonfixed icon, remove same or
		 * lower priority icon. Check count of non-fixed view list
		 * to insert icon
		 */
		ret = box_is_enable_to_insert_in_non_fixed_list(icon);
		icon->wish_to_show = EINA_TRUE;
		list_update(icon);

		switch (ret) {
		case CAN_ADD_WITH_DEL_NOTI:
			wish_remove_icon = list_try_to_find_icon_to_remove(INDICATOR_ICON_AREA_NOTI,0);
			box_unpack(wish_remove_icon);

			retif(wish_remove_icon == NULL, FAIL, "Unexpected Error : CAN_ADD_WITH_DEL_NOTI");
			break;
		case CAN_ADD_WITH_DEL_SYSTEM:
			wish_remove_icon = list_try_to_find_icon_to_remove(INDICATOR_ICON_AREA_SYSTEM,0);

			box_unpack(wish_remove_icon);
			retif(wish_remove_icon == NULL, FAIL, "Unexpected Error : CAN_ADD_WITH_DEL_SYSTEM");
			break;
		case CAN_ADD_WITH_DEL_MINICTRL:
			wish_remove_icon = list_try_to_find_icon_to_remove(INDICATOR_ICON_AREA_MINICTRL,0);

			box_unpack(wish_remove_icon);
			retif(wish_remove_icon == NULL, FAIL, "Unexpected Error : CAN_ADD_WITH_DEL_MINICTRL");
			break;
		case CAN_ADD_WITHOUT_DEL:
			break;
		case CANNOT_ADD:
			return FAIL;
			break;
		}

		return OK;
	}
	else if (INDICATOR_ICON_AREA_FIXED == icon->area)
	{
		/* In Case of fixed icon, remove same priority icon */
		wish_remove_icon = list_try_to_find_icon_to_remove(INDICATOR_ICON_AREA_FIXED,icon->priority);

		/* First icon in the priority */
		if (wish_remove_icon == NULL)
		{
			return OK;
		}

		/* Already shown icon */
		if (wish_remove_icon == icon)
		{
			return FAIL;
		}

		icon->wish_to_show = EINA_TRUE;
		list_update(icon);

		/* Wish_remove_icon is always_top icon */
		if (wish_remove_icon->always_top)
		{
			return FAIL;
		}

		/* Other Icon of Same Priority should remove in view list */
		box_unpack(wish_remove_icon);
	}

	return OK;
}



/******************************************************************************
 *
 * Util Functions : external
 *
 *****************************************************************************/

#if 0
int icon_width_set(icon_s *icon)
{
	return 0;
}
#endif


static int _icon_update(icon_s *icon)
{
	struct appdata *ad = NULL;
	Evas_Object *img_eo;
	char buf[PATH_MAX];

	retif(icon == NULL || icon->ad == NULL, FAIL, "Invalid parameter!");
	ad = icon->ad;

	memset(buf, 0x00, sizeof(buf));

	if (icon->type == INDICATOR_IMG_ICON || icon->type == INDICATOR_TXT_WITH_IMG_ICON || icon->type == INDICATOR_DIGIT_ICON) {
		if (icon->area== INDICATOR_ICON_AREA_FIXED) {
			if(icon->type == INDICATOR_DIGIT_ICON && icon->digit_area == DIGIT_DOZENS) {
				elm_layout_file_set(icon->img_obj.obj, ICON_THEME_FILE,"elm/indicator/icon/dozen_digit");
			} else {
				elm_layout_file_set(icon->img_obj.obj, ICON_THEME_FILE,"elm/indicator/icon/base");
			}
		} else {
			if(util_check_noti_ani(icon->img_obj.data)) {
				elm_layout_file_set(icon->img_obj.obj, ICON_NONFIXED_THEME_ANI_FILE,"elm/indicator/icon/base");
			} else{
				elm_layout_file_set(icon->img_obj.obj, ICON_NONFIXED_THEME_FILE,"elm/indicator/icon/base");
			}
		}

		img_eo = evas_object_data_get(icon->img_obj.obj, DATA_KEY_IMG_ICON);

		util_start_noti_ani(icon);

		/* Check absolute path */
		retif(icon->img_obj.data == NULL, FAIL,"Invalid parameter!");

		if (strncmp(icon->img_obj.data, "/", 1) != 0) {
			snprintf(buf, sizeof(buf), "%s/%s", util_get_icon_dir(),icon->img_obj.data);
			elm_image_file_set(img_eo, buf, NULL);
		} else {
			retif(icon->img_obj.data[0] == '\0', FAIL,"Invalid parameter!");
			elm_image_file_set(img_eo, icon->img_obj.data, NULL);
		}

		if (icon->img_obj.width >= 0 && icon->img_obj.height>=0) {
			evas_object_size_hint_min_set(img_eo,
				ELM_SCALE_SIZE(icon->img_obj.width),
				ELM_SCALE_SIZE(icon->img_obj.height));
		} else {
			evas_object_size_hint_min_set(img_eo, ELM_SCALE_SIZE(DEFAULT_ICON_WIDTH), ELM_SCALE_SIZE(DEFAULT_ICON_HEIGHT));
		}
	}

	if (icon->area == INDICATOR_ICON_AREA_SYSTEM) {
		int bDisplay = 0;
		bDisplay = 1;
		if(ad->opacity_mode == INDICATOR_OPACITY_TRANSPARENT && bDisplay == 1) {
			util_send_status_message_start(ad,2.5);
		}
	}

	return OK;
}



void icon_show(icon_s *icon)
{
	struct appdata *ad = NULL;

	ret_if(!icon);
	ret_if(!(icon->ad));

	ad = (struct appdata *)icon->ad;

	if (icon->obj_exist != EINA_FALSE) {
		if (icon->priority == INDICATOR_PRIORITY_NOTI_2) {
			box_unpack(icon);
			box_pack(icon);
			box_update_display(&(ad->win));
		} else {
			_icon_update(icon);
		}
	}

	if (_hide_others_in_view_list(icon) == FAIL) {
		return;
	}

	box_pack(icon);

	box_update_display(&(ad->win));
}

void icon_hide(icon_s *icon)
{
	int ret;

	retif(icon == NULL, , "Invalid parameter!");
	struct appdata *ad = (struct appdata *)icon->ad;

	icon->wish_to_show = EINA_FALSE;

	if (icon->exist_in_view == EINA_TRUE) {
		ret = box_unpack(icon);

		if (ret == FAIL)
			SECURE_ERR("Failed to unpack %s!", icon->name);

		_show_others_in_same_priority(icon);

		box_update_display(&(ad->win));

	}
}


#if 0
void icon_event_count_set(int count, void *data)
{
	static int _cnt = -1;
	char buf[1024];

	retif(data == NULL, , "Cannot get layout!");

	if (_cnt != count) {
		memset(buf, 0x00, sizeof(buf));
		if (count) {
			snprintf(buf, sizeof(buf), "%d", count);
			util_signal_emit(data,"badge,show,1","elm.image.badge");
		} else {
			util_signal_emit(data,"badge,hide,1","elm.image.badge");
		}

		util_part_text_emit(data,"elm.text.badge", buf);
		_cnt = count;
	}
}
#endif


unsigned int icon_get_update_flag(void)
{
	return update_icon_flag;
}



void icon_set_update_flag(unsigned int val)
{
	DBG("SET UPDATE FLAG %d",val);
	update_icon_flag = val;
}



void icon_reset_list(void)
{
	int system_cnt = box_get_count(SYSTEM_LIST);

	if (system_cnt > box_get_enabled_system_count()) {
		while(system_cnt > box_get_enabled_system_count()) {
			icon_s *wish_remove_icon = NULL;
			wish_remove_icon = list_try_to_find_icon_to_remove(INDICATOR_ICON_AREA_SYSTEM, 0);

			if (wish_remove_icon == NULL) {
				break;
			}

			box_unpack(wish_remove_icon);
			system_cnt = box_get_count(SYSTEM_LIST);
			SECURE_DBG("system remove %s %d",wish_remove_icon->name,system_cnt);
		}
	} else {
		while (system_cnt < box_get_enabled_system_count()) {
			icon_s *wish_add_icon = NULL;
			wish_add_icon = list_try_to_find_icon_to_show(INDICATOR_ICON_AREA_SYSTEM, 0);
			if (wish_add_icon == NULL) {
				break;
			}

			if (box_exist_icon(wish_add_icon)) {
				break;
			}

			box_pack_append(wish_add_icon);
			system_cnt = box_get_count(SYSTEM_LIST);
			SECURE_DBG("system insert %s %d",wish_add_icon->name,system_cnt);
			if(system_cnt == box_get_enabled_system_count()) {
				SECURE_DBG("quit adding %d %d",system_cnt,box_get_enabled_system_count());
				break;
			}
		}
	}

	int minictrl_cnt = box_get_count(MINICTRL_LIST);

	if (minictrl_cnt > box_get_minictrl_list()) {
		DBG("11 minictrl_cnt : %d //  box_get_minictrl_list : %d", minictrl_cnt, box_get_minictrl_list());
		while (minictrl_cnt > box_get_minictrl_list()) {
			DBG("22 minictrl_cnt : %d //  box_get_minictrl_list : %d", minictrl_cnt, box_get_minictrl_list());
			icon_s *wish_remove_icon = NULL;
			wish_remove_icon = list_try_to_find_icon_to_remove(INDICATOR_ICON_AREA_MINICTRL,0);

			if (wish_remove_icon == NULL) {
				DBG("icon_reset_list NULL!");
				break;
			}

			box_unpack(wish_remove_icon);
			minictrl_cnt = box_get_count(MINICTRL_LIST);
			SECURE_DBG("minictrl remove %s %d",wish_remove_icon->name,minictrl_cnt);
		}
	} else {
		while (minictrl_cnt < box_get_minictrl_list()) {
			icon_s *wish_add_icon = NULL;
			wish_add_icon = list_try_to_find_icon_to_show(INDICATOR_ICON_AREA_MINICTRL, 0);
			if (wish_add_icon == NULL) {
				break;
			}

			if (box_exist_icon(wish_add_icon)) {
				break;
			}

			box_pack_append(wish_add_icon);
			minictrl_cnt = box_get_count(MINICTRL_LIST);
			SECURE_DBG("minictrl insert %s %d",wish_add_icon->name,minictrl_cnt);
			if(minictrl_cnt==box_get_minictrl_list()) {
				SECURE_DBG("quit adding %d %d", minictrl_cnt, box_get_minictrl_list());
				break;
			}
		}
	}

	int noti_cnt = box_get_count(NOTI_LIST);

	if (noti_cnt > box_get_enabled_noti_count()) {
		while (noti_cnt > box_get_enabled_noti_count()) {
			icon_s *wish_remove_icon = NULL;
			wish_remove_icon = list_try_to_find_icon_to_remove(INDICATOR_ICON_AREA_NOTI, 0);

			if (wish_remove_icon == NULL) {
				break;
			}

			box_unpack(wish_remove_icon);
			noti_cnt = box_get_count(NOTI_LIST);
			SECURE_DBG("remove %s %d",wish_remove_icon->name,noti_cnt);
		}
	} else {
		while (noti_cnt < box_get_enabled_noti_count()) {
			icon_s *wish_add_icon = NULL;
			wish_add_icon = list_try_to_find_icon_to_show(INDICATOR_ICON_AREA_NOTI, 0);
			if (wish_add_icon == NULL) {
				break;
			}

			if (box_exist_icon(wish_add_icon)) {
				break;
			}

			box_pack_append(wish_add_icon);
			noti_cnt = box_get_count(NOTI_LIST);
			SECURE_DBG("insert %s %d", wish_add_icon->name, noti_cnt);
			if(noti_cnt==box_get_enabled_noti_count()) {
				SECURE_DBG("quit adding %d %d", noti_cnt, box_get_enabled_noti_count());
				break;
			}
		}
	}
}



static void _show_hide_more_noti(win_info* win,int val)
{
	static int bShow = 0;

	if (bShow == val) {
		return;
	}

	bShow = val;

	if (val == 1) {
		preference_set_int(VCONFKEY_INDICATOR_SHOW_MORE_NOTI, 1);
	} else {
		preference_set_int(VCONFKEY_INDICATOR_SHOW_MORE_NOTI, 0);
	}
}



void icon_handle_more_notify_icon(win_info* win)
{
	retif(win == NULL, , "Invalid parameter!");
	DBG("icon_handle_more_notify_icon called !!");
/*	int system_cnt = box_get_count(SYSTEM_LIST);
	int minictrl_cnt = box_get_count(MINICTRL_LIST);
	int noti_cnt = list_get_noti_count();

	DBG("System count : %d, Minictrl count : %d, Notification count : %d", system_cnt, minictrl_cnt, noti_cnt);
	if(win->type == INDICATOR_WIN_PORT)
	{
		DBG("PORT :: %d", (system_cnt + minictrl_cnt + noti_cnt));
		if((system_cnt + minictrl_cnt + noti_cnt) > MAX_NOTI_ICONS_PORT)
		{
			_show_hide_more_noti(win,1);
			DBG("PORT :: handle_more_notify_show");
		}
		else
		{*/
			_show_hide_more_noti(win,0);
			DBG("PORT :: handle_more_notify_hide");
		/*}
	}*/
}



void* icon_util_make(void* input)
{
	icon_s *icon = (icon_s *)input;

	retif(input == NULL,NULL, "Invalid parameter!");

	icon_s *obj = NULL;
	obj = calloc(1, sizeof(icon_s));

	if (obj) {
		memset(obj, 0, sizeof(icon_s));
		memcpy(obj,input,sizeof(icon_s));
		obj->name = strdup(icon->name);
	}

	return obj;
}



/* End of file */
