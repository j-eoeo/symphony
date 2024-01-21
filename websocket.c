/* $Id: websocket.c 12 2024-01-16 07:51:54Z nishi $ */
#include "websocket.h"

#include "symphony.h"

#include <stdio.h>
#include <signal.h>

#include <pthread.h>

#include <wsclient.h>

extern struct symphony** __symphony_list;

int __symphony_onopen(wsclient* c){
#ifdef _DEBUG_
	fprintf(stderr, "WEBSOCKET.. Connected\n");
#endif
	return 0;
}

int __symphony_onerror(wsclient* c, wsclient_error* err){
#ifdef _DEBUG_
	fprintf(stderr, "WEBSOCKET.. Error %d\n", err->code);
#endif
	return 0;
}

int __symphony_onclose(wsclient* c){
#ifdef _DEBUG_
	fprintf(stderr, "WEBSOCKET.. Closed\n");
#endif
	int i;
	for(i = 0; __symphony_list[i] != NULL; i++){
		wsclient* listc = ((wsclient*)__symphony_list[i]->ws);
		if(listc->sockfd == c->sockfd){
			pthread_cancel(__symphony_list[i]->heartbeet_thread);
			__symphony_list[i]->ws = __symphony_connect(__symphony_list[i]->token, __symphony_list[i]);
		}
	}
	return 0;
}

extern void libwsclient_send(wsclient* c, const char* data);
void __symphony_write(void* client, const char* data){
	libwsclient_send(client, data);
}

int __symphony_onmessage(wsclient* c, wsclient_message* msg){
	int i;
	for(i = 0; __symphony_list[i] != NULL; i++){
		wsclient* listc = ((wsclient*)__symphony_list[i]->ws);
		if(listc->sockfd == c->sockfd){
			__symphony_do_thing(__symphony_list[i], msg->payload, msg->payload_len);
		}
	}
	return 0;
}

void* __symphony_connect(const char* token, struct symphony* s){
	wsclient* client = libwsclient_new("wss://gateway.discord.gg/?v=10&encoding=json");
	if(client == NULL){
#ifdef _DEBUG_
		fprintf(stderr, "WEBSOCKET.. Failed to create\n");
#endif
		return NULL;
	}else{
#ifdef _DEBUG_
		fprintf(stderr, "WEBSOCKET.. Created\n");
#endif
	}
	client->onopen = __symphony_onopen;
	client->onclose = __symphony_onclose;
	client->onmessage = __symphony_onmessage;
	client->onerror = __symphony_onerror;
	s->ws = client;
	return client;
}

void __symphony_lock(void* client){
	pthread_mutex_lock(&((wsclient*)client)->lock);
}

void __symphony_unlock(void* client){
	pthread_mutex_unlock(&((wsclient*)client)->lock);
}


void __symphony_block(void* client){
	libwsclient_run(client);
	libwsclient_finish(client);
}
