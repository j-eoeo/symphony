/* $Id: symphony.c 15 2024-01-16 15:10:10Z nishi $ */
#include "symphony.h"

#include "websocket.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include <pthread.h>

#include <cjson/cJSON.h>

struct symphony** __symphony_list;

void symphony_library_init(void){
#ifdef _DEBUG_
	fprintf(stderr, "........... This is Symphony, version %s\n", SYMPHONY_VERSION);
#endif
	__symphony_list = malloc(sizeof(*__symphony_list));
	__symphony_list[0] = NULL;
}

void* __symphony_heartbeat(void* _s){
	struct symphony* s = (struct symphony*)_s;
	cJSON* prop;
	while(true){
		__symphony_lock(s->ws);
		cJSON* alive = cJSON_CreateObject();
		prop = cJSON_CreateNumber(__SYMPHONY_OP_HEARTBEAT);
		cJSON_AddItemToObject(alive, "op", prop);
		prop = s->last == -1 ? cJSON_CreateNull() : cJSON_CreateNumber(s->last);
		cJSON_AddItemToObject(alive, "d", prop);
		__symphony_write(s->ws, cJSON_Print(alive));
		cJSON_Delete(alive);
		int i;
		__symphony_unlock(s->ws);
		sleep(s->repeat / 1000 - 1);
	}
	return NULL;
}

void __symphony_do_thing(struct symphony* s, char* payload, int payload_len){
#ifdef _DEBUG_i
	fprintf(stderr, "WEBSOCKET.. Got message, %d bytes\n", payload_len);
#endif
	cJSON* json = cJSON_Parse(payload);
	if(json != NULL){
		int op = (int)cJSON_GetObjectItemCaseSensitive(json, "op")->valuedouble;
		cJSON* seq = cJSON_GetObjectItemCaseSensitive(json, "s");
		if(seq != NULL && cJSON_IsNumber(seq)){
			s->last = seq->valuedouble;
		}
#ifdef _DEBUG_
		fprintf(stderr, "JSON....... Opcode is %d\n", op);
		if(op == 0){
			fprintf(stderr, "JSON....... Type is %s\n", cJSON_GetObjectItemCaseSensitive(json, "t")->valuestring);
		}
#endif
		if(op == 10){
			s->repeat = (int)cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(json, "d"), "heartbeat_interval")->valuedouble;
			if(s->login){
#ifdef _DEBUG_
				fprintf(stderr, "DISCORD.... Resuming\n");
#endif
				cJSON* login = cJSON_CreateObject();
				cJSON* prop = cJSON_CreateNumber(__SYMPHONY_OP_RESUME);
				cJSON_AddItemToObject(login, "op", prop);
				prop = cJSON_CreateObject();
				cJSON* prop2 = cJSON_CreateString(s->token);
				cJSON_AddItemToObject(prop, "token", prop2);
				prop2 = cJSON_CreateNumber(s->last);
				cJSON_AddItemToObject(prop, "seq", prop2);
				prop2 = cJSON_CreateString(s->session_id);
				cJSON_AddItemToObject(prop, "session_id", prop2);
				cJSON_AddItemToObject(login, "d", prop);
				__symphony_write(s->ws, cJSON_Print(login));
				cJSON_Delete(login);
			}else{
#ifdef _DEBUG_
				fprintf(stderr, "DISCORD.... Logging in\n");
#endif
				cJSON* login = cJSON_CreateObject();
				cJSON* prop = cJSON_CreateNumber(__SYMPHONY_OP_IDENTIFY);
				cJSON_AddItemToObject(login, "op", prop);
				prop = cJSON_CreateObject();
				cJSON* prop2 = cJSON_CreateString(s->token);
				cJSON_AddItemToObject(prop, "token", prop2);
				prop2 = cJSON_CreateObject();
				cJSON* prop3 = cJSON_CreateString("Symphony");
				cJSON_AddItemToObject(prop2, "browser", prop3);
				prop3 = cJSON_CreateString("Symphony");
				cJSON_AddItemToObject(prop2, "device", prop3);
				prop3 = cJSON_CreateString("Symphony");
				cJSON_AddItemToObject(prop2, "os", prop3);
				cJSON_AddItemToObject(prop, "properties", prop2);
				prop2 = cJSON_CreateNumber((1 << 15) | (1 << 9) | (1 << 8));
				cJSON_AddItemToObject(prop, "intents", prop2);
				cJSON_AddItemToObject(login, "d", prop);
				__symphony_write(s->ws, cJSON_Print(login));
				cJSON_Delete(login);
			}
		}else if(op == 0 && strcmp(cJSON_GetObjectItemCaseSensitive(json, "t")->valuestring, "READY") == 0){
#ifdef _DEBUG_
			fprintf(stderr, "DISCORD.... Ready\n");
#endif
			cJSON* data = cJSON_GetObjectItemCaseSensitive(json, "d");
			cJSON* user = cJSON_GetObjectItemCaseSensitive(data, "user");
			s->client_id = (snowflake_t)atoll(cJSON_GetObjectItemCaseSensitive(user, "id")->valuestring);
			s->session_id = strdup(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(json, "d"), "session_id")->valuestring);
			if(!s->login){
				__symphony_dispatch(s, "ready", s);
			}
			s->login = true;

			pthread_create(&s->heartbeet_thread, NULL, __symphony_heartbeat, s);
		}else if(op == 0 && strcmp(cJSON_GetObjectItemCaseSensitive(json, "t")->valuestring, "RESUMED") == 0){
#ifdef _DEBUG_
			fprintf(stderr, "DISCORD.... Resumed\n");
#endif

			pthread_create(&s->heartbeet_thread, NULL, __symphony_heartbeat, s);
		}else if(op == 0 && strcmp(cJSON_GetObjectItemCaseSensitive(json, "t")->valuestring, "MESSAGE_CREATE") == 0){
			struct symphony_message* msg = malloc(sizeof(*msg));
			msg->content = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(json, "d"), "content")->valuestring;
			msg->id = (snowflake_t)atoll(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(json, "d"), "id")->valuestring);

			cJSON* data = cJSON_GetObjectItemCaseSensitive(json, "d");
			__symphony_message(msg, s, (void*)data, true);
			if(msg->type == __SYMPHONY_MSG_REPLY){
				__symphony_dispatch(s, "reply", msg);
			}
			__symphony_dispatch(s, "message", msg);
			if(msg->replyto != NULL) free(msg->replyto);
			free(msg);
		}
	}
	cJSON_Delete(json);
}

void symphony_set_status(struct symphony* s, const char* status, const char* activ0, const char* emoji, int activtype){
	cJSON* json = cJSON_CreateObject();
	cJSON* prop;
	cJSON* prop2;

	prop = cJSON_CreateNumber(__SYMPHONY_OP_PRESENCE_UPDATE);
	cJSON_AddItemToObject(json, "op", prop);
	
	prop = cJSON_CreateObject();

	prop2 = cJSON_CreateNumber(0);
	cJSON_AddItemToObject(prop, "since", prop2);

	prop2 = cJSON_CreateArray();
	if(activ0 != NULL){
		cJSON* prop3 = cJSON_CreateObject();
		cJSON* prop4;

		prop4 = cJSON_CreateNumber(time(NULL) * 1000);
		cJSON_AddItemToObject(prop3, "created_at", prop4);

		prop4 = cJSON_CreateString(activ0);
		cJSON_AddItemToObject(prop3, "name", prop4);

		if(emoji == NULL){
			cJSON_AddItemToObject(prop3, "emoji", cJSON_CreateNull());
		}else{
			prop4 = cJSON_CreateObject();

			cJSON* prop5 = cJSON_CreateString("+1");
			cJSON_AddItemToObject(prop4, "name", prop5);

			cJSON_AddItemToObject(prop3, "emoji", prop4);
		}

		prop4 = cJSON_CreateNumber(activtype);
		cJSON_AddItemToObject(prop3, "type", prop4);

		cJSON_AddItemToArray(prop2, prop3);
	}
	cJSON_AddItemToObject(prop, "activities", prop2);

	prop2 = cJSON_CreateString(status);
	cJSON_AddItemToObject(prop, "status", prop2);

	prop2 = cJSON_CreateFalse();
	cJSON_AddItemToObject(prop, "afk", prop2);


	cJSON_AddItemToObject(json, "d", prop);

	__symphony_write(s->ws, cJSON_Print(json));

	cJSON_Delete(json);
}

void __symphony_message(struct symphony_message* msg, struct symphony* s, void* _data, bool loop){
	cJSON* data = (cJSON*)_data;
	cJSON* author = cJSON_GetObjectItemCaseSensitive(data, "author");
	msg->author.id = (snowflake_t)atoll(cJSON_GetObjectItemCaseSensitive(author, "id")->valuestring);
	msg->author.username = cJSON_GetObjectItemCaseSensitive(author, "username")->valuestring;
	msg->channel.s = s;
	msg->channel.id = (snowflake_t)atoll(cJSON_GetObjectItemCaseSensitive(data, "channel_id")->valuestring);
	msg->type = cJSON_GetObjectItemCaseSensitive(data, "type")->valuedouble;
	msg->replyto = NULL;
	msg->symphony = s;
	if(loop && msg->type == __SYMPHONY_MSG_REPLY){
		msg->replyto = malloc(sizeof(*msg->replyto));
		__symphony_message(msg->replyto, s, cJSON_GetObjectItemCaseSensitive(data, "referenced_message"), false);
	}
}

void symphony_send(struct symphony_channel c, const char* message){
	char* flake = malloc(512);
	memset(flake, 0, 512);
	sprintf(flake, "%llu", c.id);
	char* path = __symphony_strcat3("/api/v10/channels/", flake, "/messages");
	cJSON* json = cJSON_CreateObject();
	cJSON* prop;

	prop = cJSON_CreateString(message);
	cJSON_AddItemToObject(json, "content", prop);

	__symphony_http(c.s, "POST", path, cJSON_Print(json));
	free(path);
	cJSON_Delete(json);
}

void __symphony_dispatch(struct symphony* s, const char* eventname, void* arg){
	if(s->events == NULL) return;
	int i;
	for(i = 0; s->events[i] != NULL; i += 2){
		if(strcmp((const char*)s->events[i], eventname) == 0){
			void(*func)(void*) = (void(*)(void*))s->events[i + 1];
			func(arg);
			return;
		}
	}
}

void symphony_on(struct symphony* s, const char* eventname, void(*func)(void*)){
	int len = 0;
	if(s->events == NULL){
		s->events = malloc(sizeof(*s->events) * (len + 3));
	}else{
		void** old_events = s->events;
		for(len = 0; old_events[len] != NULL; len++);
		s->events = malloc(sizeof(*s->events) * (len + 3));
		for(len = 0; old_events[len] != NULL; len++){
			s->events[len] = old_events[len];
		}
		free(old_events);
	}
	s->events[len] = (void*)eventname;
	s->events[len + 1] = func;
	s->events[len + 2] = NULL;
}

struct symphony* symphony_connect(const char* token){
#ifdef _DEBUG_
	fprintf(stderr, "........... Connecting to Discord Gateway\n");
#endif
	struct symphony* s = malloc(sizeof(*s));
	s->ws = __symphony_connect(token, s);
	s->token = strdup(token);
	s->login = false;
	s->last = -1;
	s->events = NULL;
	s->heartbeet_thread = NULL;
	int i;
	struct symphony** __old_list = __symphony_list;
	for(i = 0; __symphony_list[i] != NULL; i++);
	__symphony_list = malloc(sizeof(*__symphony_list) * (i + 2));
	for(i = 0; __old_list[i] != NULL; i++){
		__symphony_list[i] = __old_list[i];
	}
	__symphony_list[i] = s;
	__symphony_list[i + 1] = NULL;
	free(__old_list);
	return s;
}

void symphony_loop(struct symphony* s){
	while(true){
		__symphony_block(s->ws);
	}
}
