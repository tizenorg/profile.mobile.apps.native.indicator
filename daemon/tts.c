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



#ifdef _SUPPORT_SCREEN_READER2

#include <tts.h>
#include <tts_setting.h>
#include <vconf.h>
#include <vconf-keys.h>
#include "main.h"
#include "common.h"
#include "tts.h"

static void _tts_init(void);
static void _tts_fini(void);

static struct _s_info {
	tts_h tts_handler;
	Eina_List *list;
} s_info = {
	.tts_handler = NULL,
	.list = NULL,
};

typedef struct _QP_TTS {
	int id;
	int done;
	char *message;
} QP_TTS_T;



static QP_TTS_T * _tts_entry_new(int id, char *message)
{
	QP_TTS_T *entry = NULL;
	retif(message == NULL, NULL, "NULL message");

	entry = (QP_TTS_T *)calloc(1, sizeof(QP_TTS_T));
	retif(entry == NULL, NULL, "failed to memory allocation");

	entry->id = id;
	entry->message = strdup(message);

	return entry;
}



static void  _tts_entry_del(QP_TTS_T *entry)
{
	retif(entry == NULL, ,"invalid parameter");

	if (entry->message != NULL) {
		free(entry->message);
	}

	free(entry);
}



static QP_TTS_T *_tts_list_get_first(void)
{
	return eina_list_nth(s_info.list, 0);
}



static void _tts_list_add(QP_TTS_T *entry)
{
	retif(entry == NULL, ,"invalid parameter");

	s_info.list = eina_list_prepend(s_info.list, entry);
}



static void _tts_list_del(QP_TTS_T *entry)
{
	retif(entry == NULL, ,"invalid parameter");

	s_info.list = eina_list_remove(s_info.list, entry);
}



static void _tts_list_clean(void)
{
	QP_TTS_T *entry = NULL;

	while ((entry = _tts_list_get_first()) != NULL) {
		 _tts_list_del(entry);
		 _tts_entry_del(entry);
	}
}



static int _is_screenreader_on(void)
{
	int ret = -1, status = 0;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &status);
	retif(ret != 0, 0, "failed to read VCONFKEY_SETAPPL_ACCESSIBILITY_TTS %d", ret);

	return status;
}



static tts_state_e _tts_state_get(void)
{
	int ret = TTS_ERROR_NONE;
	tts_state_e state = TTS_STATE_READY;

	if (s_info.tts_handler != NULL) {
		ret = tts_get_state(s_info.tts_handler, &state);
		if (TTS_ERROR_NONE != ret){
			ERR("get state error(%d)", ret);
			return -1;
		}

		return state;
	}

	return -1;
}



static void _tts_play(const char *message)
{
	int utt = 0;
	int ret = TTS_ERROR_NONE;

	if (s_info.tts_handler == NULL) {
		ERR("critical, TTS handler isn't initialized");
		return;
	}

	DBG("adding %s", message);

	ret = tts_add_text(s_info.tts_handler, message, NULL, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &utt);
	if (TTS_ERROR_NONE != ret){
		ERR("add text error!");
		return;
	}

	ret = tts_play(s_info.tts_handler);
	if(ret != TTS_ERROR_NONE) {
		ERR("play error(%d) state(%d)", ret);
	}
}



static void _tts_stop(void)
{
	int ret = TTS_ERROR_NONE;

	if (s_info.tts_handler == NULL) {
		ERR("critical, TTS handler isn't initialized");
		return;
	}

	ret = tts_stop(s_info.tts_handler);
	if (TTS_ERROR_NONE != ret){
		ERR("failed to stop play:%d", ret);
		return;
	}
}



static void _tts_state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data)
{
	QP_TTS_T *entry = NULL;

	DBG("_tts_state_changed_cb(%d => %d)", previous, current);

	if(previous == TTS_STATE_CREATED && current == TTS_STATE_READY) {
		entry = _tts_list_get_first();
		if (entry != NULL) {
			 _tts_play(entry->message);
			 _tts_list_del(entry);
			 _tts_entry_del(entry);
		}
		_tts_list_clean();
	}
}



static void _tts_utt_started_cb(tts_h tts, int utt_id, void *user_data)
{
	DBG("_tts_utt_started_cb");
}



static void _tts_utt_completed_cb(tts_h tts, int utt_id, void *user_data)
{
	DBG("_tts_utt_completed_cb");
}



static void _tts_error_cb(tts_h tts, int utt_id, tts_error_e reason, void* user_data)
{
	DBG("_tts_error_cb");
}



static int _tts_callback_set(tts_h tts, void* data)
{
	int ret = 0;

	if (TTS_ERROR_NONE != (ret = tts_set_state_changed_cb(tts, _tts_state_changed_cb, tts))){
		ERR("set interrupted callback error !!:%d", ret);
		ret = -1;
	}

	if (TTS_ERROR_NONE != (ret = tts_set_utterance_started_cb(tts, _tts_utt_started_cb, data))) {
		ERR("set utterance started callback error !!:%d", ret);
		ret = -1;
	}

	if (TTS_ERROR_NONE != (ret = tts_set_utterance_completed_cb(tts, _tts_utt_completed_cb, data))) {
		ERR("set utterance completed callback error !!:%d", ret);
		ret = -1;
	}

	if (TTS_ERROR_NONE != (ret = tts_set_error_cb(tts, _tts_error_cb, data))) {
		ERR("set error callback error !!:%d", ret);
		ret = -1;
	}

	return ret;
}



static void _tts_init()
{
	tts_h tts = NULL;
	int ret = TTS_ERROR_NONE;

	if (s_info.tts_handler == NULL) {
		ret = tts_create(&tts);
		if(ret != TTS_ERROR_NONE) {
			ERR("tts_create() failed");
			return ;
		}

		ret = tts_set_mode(tts, TTS_MODE_NOTIFICATION);
		if(ret != TTS_ERROR_NONE) {
			ERR("tts_create() failed");
			tts_destroy(s_info.tts_handler);
			s_info.tts_handler = NULL;
			return ;
		}

		if(_tts_callback_set(tts, NULL) != 0) {
			ERR("_tts_callback_set() failed");
			tts_destroy(s_info.tts_handler);
			s_info.tts_handler = NULL;
			return ;
		}

		ret = tts_prepare(tts);
		if(ret != TTS_ERROR_NONE) {
			ERR("tts_create() failed");
			tts_destroy(s_info.tts_handler);
			s_info.tts_handler = NULL;
			return ;
		}

		s_info.tts_handler = tts;
	}
}



static void _tts_fini(void)
{
	int ret = TTS_ERROR_NONE;

	if (s_info.tts_handler != NULL) {
		ret = tts_destroy(s_info.tts_handler);
		if(ret != TTS_ERROR_NONE) {
			ERR("tts_destroy() failed");
		}
		s_info.tts_handler = NULL;
	}
}



static void _tts_vconf_cb(keynode_t *key, void *data){
	if(_is_screenreader_on() == 0) {
		DBG("TTS turned off");
		_tts_fini();
	}

	_tts_list_clean();
}



void indicator_service_tts_init(void *data) {
	int ret = 0;

    ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
		_tts_vconf_cb, data);
}



void indicator_service_tts_fini(void *data) {
	int ret = 0;

    ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
		_tts_vconf_cb, data);

    _tts_list_clean();
}



void indicator_service_tts_play(char *message) {
	tts_state_e state = 0;
	QP_TTS_T *entry = NULL;
	retif(message == NULL, ,"invalid parameter");

	if (_is_screenreader_on() == 1) {
		_tts_init();

		state = _tts_state_get();

		if (state == TTS_STATE_CREATED) {
			_tts_list_clean();
			entry = _tts_entry_new(-1, message);
			if (entry != NULL) {
				 _tts_list_add(entry);
			}
		} else if (state == TTS_STATE_PLAYING || state == TTS_STATE_PAUSED) {
			_tts_stop();
			_tts_play(message);
		} else if (state == TTS_STATE_READY) {
			_tts_play(message);
		} else {
			ERR("invalid status: %d", state);
		}
	}
}
#endif /* _SUPPORT_SCREEN_READER2 */
