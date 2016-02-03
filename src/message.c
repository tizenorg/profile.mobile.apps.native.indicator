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



#include <notification.h>
#include <notification_status_internal.h>

#include "main.h"
#include "common.h"
#include "util.h"
#include "tts.h"
#include "box.h"


#define MSG_TIMEOUT 3
#define STR_BUF_SIZE 256
#define QUEUE_TIMEOUT 1
#define QUEUE_TIMEOUT2 5
#define QUEUE_SIZE 5

#define MESSAGE_LINE1 "message.text"
#define MESSAGE_LINE2 "message.text2"


typedef struct _str_buf {
	char *data;
	int index;
	double timer_val;
} MsgBuf;


static int msg_type = 0;
static Ecore_Timer *msg_timer = NULL;
static Ecore_Timer *ani_temp_timer = NULL;

extern int current_angle;
static int block_width = 0;
static int string_width = 0;
static char* message_buf = NULL;
static Ecore_Timer *retry_timer=NULL;
static int msg_retry = 0;
static struct appdata *app_data = NULL;
static Ecore_Timer *queue_timer=NULL;
static int current_buf_index = 0;
static int current_buf_cnt = 0;
static MsgBuf msg_queue[QUEUE_SIZE];



static Eina_Bool _ani_temp_timeout_cb(void *data)
{
	retif(data == NULL, ECORE_CALLBACK_CANCEL, "Invalid parameter!");

	if (ani_temp_timer)
	{
		ecore_timer_del(ani_temp_timer);
		ani_temp_timer = NULL;
	}

	return ECORE_CALLBACK_CANCEL;
}



void start_temp_ani_timer(void* data)
{
	retif(data == NULL, , "Invalid parameter!");
	win_info* win = (win_info*)data;

	if(ani_temp_timer != NULL)
	{
		ecore_timer_del(ani_temp_timer);
		ani_temp_timer = NULL;
	}
	ani_temp_timer = ecore_timer_add(0.3, (Ecore_Task_Cb)_ani_temp_timeout_cb, (void*)win);

}



static void _hide_message(void* data)
{
	retif(data == NULL, , "Invalid parameter!");
	win_info* win = NULL;
	win = (win_info*)data;

	box_update_display(win);

	start_temp_ani_timer(data);
	util_signal_emit_by_win(win,"message.hide", "indicator.prog");
}



static void _hide_message_all(void* data)
{
	retif(data == NULL, , "Invalid parameter!");

	util_signal_emit(data,"message.line2.hide.noeffect","indicator.prog");
}



static void _show_message(void* data)
{
	retif(data == NULL, , "Invalid parameter!");
	win_info* win = NULL;
	win = (win_info*)data;
	struct appdata* ad = (struct appdata*)win->data;

	start_temp_ani_timer(data);
	if(ad->opacity_mode==INDICATOR_OPACITY_TRANSPARENT)
	{
		DBG("Transparent");
		util_signal_emit_by_win(win,"message.show.noeffect", "indicator.prog");
		evas_object_show(win->win);
	}
	else
	{
		util_signal_emit_by_win(win,"message.show", "indicator.prog");
	}
}



static void _show_message_line2(void* data)
{
	retif(data == NULL, , "Invalid parameter!");
	win_info* win = NULL;
	win = (win_info*)data;

	if (win)
	{
		util_signal_emit_by_win(win,"message.line2.show", "indicator.prog");
	}
}



static Eina_Bool _msg_timeout_cb(void *data)
{
	retif(data == NULL, ECORE_CALLBACK_CANCEL, "Invalid parameter!");

	win_info* win = (win_info*)data;

	if(msg_type == 1)
	{
		msg_timer = NULL;
		_hide_message(win);
		return ECORE_CALLBACK_CANCEL;
	}
	else if(msg_type == 2)
	{

		msg_type = 0;
		if (msg_timer)
		{
			ecore_timer_del(msg_timer);
		}
		msg_timer = ecore_timer_add(3, (Ecore_Task_Cb)_msg_timeout_cb, (void*)win);
		_show_message_line2(win);
		return ECORE_CALLBACK_CANCEL;
	}
	else
	{
		msg_type = 0;
		msg_timer = NULL;
		_hide_message(win);
		return ECORE_CALLBACK_CANCEL;
	}
}



static Eina_Bool _retry_timeout_cb(void *data)
{
	retif(data == NULL, EINA_TRUE , "Invalid parameter!");

	if(message_buf!=NULL)
	{
		free(message_buf);
		message_buf = NULL;
	}

	if (retry_timer!=NULL)
	{
		ecore_timer_del(retry_timer);
		retry_timer = NULL;
	}
	return EINA_TRUE;

}



static int __get_block_width(void* data, const char* part)
{
	Evas_Object * eo = NULL;
	int geo_dx = 0;
	int geo_dy = 0;
	retif(data == NULL,-1, "Invalid parameter!");
	retif(part == NULL,-1, "Invalid parameter!");

	win_info* win = (win_info*)data;

	eo = (Evas_Object *) edje_object_part_object_get(elm_layout_edje_get(win->layout), part);

	evas_object_geometry_get(eo, NULL, NULL, &geo_dx, &geo_dy);

	return geo_dx;
}



static int __get_string_width(void* data, const char* part)
{
	Evas_Object * eo = NULL;
	int text_dx = 0;
	int text_dy = 0;
	retif(data == NULL,-1, "Invalid parameter!");
	retif(part == NULL,-1, "Invalid parameter!");

	win_info* win = (win_info*)data;

	eo = (Evas_Object *) edje_object_part_object_get(elm_layout_edje_get(win->layout), part);

	evas_object_textblock_size_formatted_get(eo, &text_dx, &text_dy);

	return text_dx;
}



static void __handle_2line(win_info* win,char* origin, char* part1, char* part2)
{
	retif(origin == NULL, , "Invalid parameter!");
	retif(part1 == NULL, , "Invalid parameter!");
	retif(part2 == NULL, , "Invalid parameter!");
	int index = 0;
	Eina_Unicode *uni_out = NULL;
	Eina_Unicode buf[STR_BUF_SIZE] = {0,};
	int len = 0;
	int len2 = 0;
	Eina_Unicode temp1[STR_BUF_SIZE] = {0,};
	Eina_Unicode temp2[STR_BUF_SIZE] = {0,};
	char* out1 = NULL;
	char* out2 = NULL;
	int char_len1 = 0;
	int char_len2 = 0;

	uni_out = eina_unicode_utf8_to_unicode(origin, &len);

	if(len >= STR_BUF_SIZE)
	{
		len2 = STR_BUF_SIZE-1;
	}
	else
	{
		len2 = len;
	}

	eina_unicode_strncpy(buf,uni_out,len2);

	int exceed_index = len2 * block_width / string_width;
	{
		int i = 0;
		for(i=0;i<100;i++)
		{
			Eina_Unicode temp1[STR_BUF_SIZE] = {0,};
			int char_len1 = 0;
			char* out1 = NULL;
			int width = 0;
			eina_unicode_strncpy(temp1,buf,exceed_index);
			out1 = eina_unicode_unicode_to_utf8(temp1,&char_len1);
			util_part_text_emit_by_win(win,"message.text.compare", out1);

			width = __get_string_width(win,"message.text.compare");

			if(width > block_width)
			{
				exceed_index = exceed_index -1;
				DBG("reduce exceed index(%d)",exceed_index,width);
			}
			else
			{
				if(out1!=NULL)
					free(out1);
				break;
			}

			if(out1!=NULL)
				free(out1);
		}
		for(i=0;i<100;i++)
		{
			Eina_Unicode temp1[STR_BUF_SIZE] = {0,};
			int char_len1 = 0;
			char* out1 = NULL;
			int width = 0;
			eina_unicode_strncpy(temp1,buf,exceed_index);
			out1 = eina_unicode_unicode_to_utf8(temp1,&char_len1);
			util_part_text_emit_by_win(win,"message.text.compare", out1);

			width = __get_string_width(win,"message.text.compare");

			if(width < block_width)
			{
				exceed_index = exceed_index +1;
				DBG("increase exceed index(%d)",exceed_index,width);
			}
			else
			{
				exceed_index = exceed_index -1;
				if(out1)
					free(out1);
				break;
			}

			if(out1)
				free(out1);
		}

	}

	if(exceed_index<0)
	{
		ERR("INDEX %d",exceed_index);
		goto __CATCH;
	}

	int i = exceed_index;

	while(i>0)
	{
		if(buf[i-1]==' ')
		{
			index = i-1;
			break;
		}
		i--;
	}

	if(index>0)
	{
		Eina_Unicode *temp3 = NULL;
		eina_unicode_strncpy(temp1,buf,index);
		temp3 = &(buf[index]);
		eina_unicode_strncpy(temp2,temp3,len2-index);
	}
	else
	{
		Eina_Unicode *temp3 = NULL;
		eina_unicode_strncpy(temp1,buf,exceed_index);
		temp3 = &(buf[exceed_index]);
		eina_unicode_strncpy(temp2,temp3,len2-exceed_index);
	}

	out1 = eina_unicode_unicode_to_utf8(temp1,&char_len1);
	out2 = eina_unicode_unicode_to_utf8(temp2,&char_len2);

	if(char_len1>=STR_BUF_SIZE)
		char_len1 = STR_BUF_SIZE-1;
	if(char_len2>=STR_BUF_SIZE)
		char_len2 = STR_BUF_SIZE-1;
	strncpy(part1,out1,char_len1);
	strncpy(part2,out2,char_len2);

__CATCH:
	if(uni_out!=NULL)
		free(uni_out);
	if(out1!=NULL)
		free(out1);
	if(out2!=NULL)
		free(out2);
}



static void _handle_message_by_win(char *message, void *data)
{
	win_info* win = NULL;
	char part1[256] = {0,};
	char part2[256] = {0,};
	char *text = NULL;
	double time_clk = 0;
	char* temp = NULL;
	retif(message == NULL, , "Invalid parameter!");
	retif(data == NULL, , "Invalid parameter!");

	win = data;

	if (msg_timer)
	{
		ecore_timer_del(msg_timer);
	}
	msg_type = 0;

	SECURE_DBG("message %s", message);

	temp = strdup(message);

	util_char_replace(temp,'\n',' ');

	text = evas_textblock_text_utf8_to_markup(NULL, temp);
	if (!text)
	{
		if(temp)
			free(temp);

		return;
	}
	block_width = __get_block_width(win,"message.text");
	util_part_text_emit_by_win(win,"message.text.compare", text);
	string_width = __get_string_width(win,"message.text.compare");

	if(block_width > string_width)
	{
		msg_type = 1;
	}
	else
	{
		msg_type = 2;
	}

	DBG("msg_type %d",  msg_type);

	_show_message(win);

	if(msg_type == 1)
	{
		time_clk = 3;
		util_part_text_emit_by_win(win,"message.text", text);
		util_send_status_message_start(win,2.5);
	}
	else
	{
		time_clk = 2.5;
		__handle_2line(win,text,part1,part2);
		util_part_text_emit_by_win(win,"message.text", part1);
		util_part_text_emit_by_win(win,"message.line2.text", part2);
		util_send_status_message_start(win,5);
	}

	if(text!=NULL)
		free(text);

	msg_timer = ecore_timer_add(time_clk, (Ecore_Task_Cb)_msg_timeout_cb, (void*)win);

	if(temp!=NULL)
		free(temp);
}



static void __message_callback(const char *message, void *data)
{
	struct appdata *ad = NULL;
	win_info* win = NULL;

	if (!data)
		return;

	ad = data;

	char buf[256] = {0,};
	strncpy(buf,message,256-1);
#ifdef _SUPPORT_SCREEN_READER2
	indicator_service_tts_play(buf);
#endif

	if(message_buf!=NULL)
	{
		free(message_buf);
		message_buf = NULL;
	}

	message_buf = strdup(message);
	msg_retry = 0;

	_hide_message_all(data);

	win = &(ad->win);
	_handle_message_by_win(message_buf,win);

	if(retry_timer!=NULL)
	{
		ecore_timer_del(retry_timer);
	}
	retry_timer = ecore_timer_add(0.5, (Ecore_Task_Cb)_retry_timeout_cb, (void*)win);

}



static void _buf_timeout_callback(void* data)
{

	if(current_buf_index<QUEUE_SIZE)
	if(msg_queue[current_buf_index].data!=NULL)
	{
		DBG("index %d,%s",current_buf_index,msg_queue[current_buf_index].data);
		__message_callback(msg_queue[current_buf_index].data,data);
		if(msg_queue[current_buf_index].data!=NULL)
		{
			free(msg_queue[current_buf_index].data);
			msg_queue[current_buf_index].data = NULL;
		}
		if(current_buf_index+1<QUEUE_SIZE)
		{
			if(msg_queue[current_buf_index+1].data!=NULL)
			{
				if(queue_timer!=NULL)
				{
					ecore_timer_del(queue_timer);
					queue_timer = NULL;
				}
				current_buf_index = current_buf_index+1;
				queue_timer = ecore_timer_add(msg_queue[current_buf_index].timer_val, (Ecore_Task_Cb)_buf_timeout_callback, data);
				return;
			}
		}
	}

	if(queue_timer!=NULL)
	{
		ecore_timer_del(queue_timer);
		queue_timer = NULL;
	}
	current_buf_cnt = 0;
	current_buf_index = 0;
	DBG("quit buffering..");
}



static void __buffer_msg_callback(const char *message, void *data)
{
	struct appdata *ad = NULL;
	retif(data == NULL, , "Invalid parameter!");
	ad = data;

	win_info *win = NULL;
	double timer_val;

	win = &(ad->win);

	block_width = __get_block_width(win,"message.text");
	util_part_text_emit_by_win(win,"message.text.compare", message);
	string_width = __get_string_width(win,"message.text.compare");

	if(block_width > string_width)
	{
		timer_val = QUEUE_TIMEOUT;
	}
	else
	{
		timer_val = QUEUE_TIMEOUT2;
	}

	if(queue_timer!=NULL)
	{
		if(current_buf_cnt>=QUEUE_SIZE)
		{
			ERR("QUEUE FULL");
			return;
		}
		SECURE_DBG("buffering... %d,%s",current_buf_cnt,message);
		if(msg_queue[current_buf_cnt].data!=NULL)
		{
			free(msg_queue[current_buf_cnt].data);
			msg_queue[current_buf_cnt].data = NULL;
		}
		msg_queue[current_buf_cnt].data = strdup(message);
		msg_queue[current_buf_cnt].index = current_buf_cnt;
		msg_queue[current_buf_cnt].timer_val = timer_val;
		current_buf_cnt++;
		return;
	}

	queue_timer = ecore_timer_add(timer_val, (Ecore_Task_Cb)_buf_timeout_callback, data);
	__message_callback(message,data);
}



int indicator_message_disp_check(void)
{
	if (msg_timer != NULL)
		return 1;
	else
		return 0;
}



int message_ani_playing_check(void)
{
	if(ani_temp_timer != NULL)
		return 1;
	else
		return 0;
}



int indicator_message_retry_check(void)
{
	if(retry_timer!=NULL)
		return 1;
	else
		return 0;
}



void indicator_message_display_trigger(void)
{
	win_info* win = NULL;

	if(msg_retry==1)
	{
		return;
	}

	DBG("retry message");

	msg_retry = 1;

	win = &(app_data->win);
	_handle_message_by_win(message_buf,win);

}



int indicator_message_init(void *data)
{
	int ret = 0;
	int i =0;
	for(i=0;i<QUEUE_SIZE;i++)
	{
		memset(&msg_queue[i],0x00,sizeof(MsgBuf));
	}
	ret = notification_status_monitor_message_cb_set(__buffer_msg_callback, data);
	app_data = data;


	return ret;
}



int indicator_message_fini(void)
{
	int ret = 0;

	ret = notification_status_monitor_message_cb_unset();

	return ret;
}
