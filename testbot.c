/* $Id: testbot.c 16 2024-01-16 23:15:37Z nishi $ */
#include <symphony.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void message(void* data){
	struct symphony_message* msg = (struct symphony_message*)data;
	printf("ID      : %llu\nUsername: %s\n[%s]\n", msg->id, msg->author.username, msg->content);
	if(msg->type == __SYMPHONY_MSG_REPLY && msg->replyto->author.id == msg->symphony->client_id){
		symphony_send(msg->channel, "You replied to me!");
	}else if(strcmp(msg->content, "Open sesame") == 0){
		symphony_send(msg->channel, "Hello");
	}
}

void ready(void* data){
	struct symphony* s = (struct symphony*)data;
	printf("Ready\n");
	symphony_set_status(s, "online", "Shanghai Teahouse", "+1", __SYMPHONY_ACTIVITY_LISTENING);
}

int main(){
	symphony_library_init();
	struct symphony* s = symphony_connect(getenv("TOKEN"));
	symphony_on(s, "ready", ready);
	symphony_on(s, "message", message);
	symphony_loop(s);
}
