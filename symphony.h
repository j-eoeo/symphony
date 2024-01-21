/* $Id: symphony.h 15 2024-01-16 15:10:10Z nishi $ */
#ifndef __SYMPHONY_SYMPHONY_H__
#define __SYMPHONY_SYMPHONY_H__

#include <stdbool.h>
#include <pthread.h>

#define SYMPHONY_VERSION "1.00"

#define __SYMPHONY_OP_HELLO		10
#define __SYMPHONY_OP_HEARTBEAT		1
#define __SYMPHONY_OP_IDENTIFY		2
#define __SYMPHONY_OP_PRESENCE_UPDATE	3
#define __SYMPHONY_OP_RESUME		6

enum ACTIVITIES {
	__SYMPHONY_ACTIVITY_GAME = 0,
	__SYMPHONY_ACTIVITY_STREAMING,
	__SYMPHONY_ACTIVITY_LISTENING,
	__SYMPHONY_ACTIVITY_WATCHING,
	__SYMPHONY_ACTIVITY_CUSTOM,
	__SYMPHONY_ACTIVITY_COMPETING
};

enum MSGS {
	__SYMPHONY_MSG_DEFAULT = 0,
	__SYMPHONY_MSG_RECIPIENT_ADD,
	__SYMPHONY_MSG_RECIPIENT_REMOVE,
	__SYMPHONY_MSG_CALL,
	__SYMPHONY_MSG_CHANNEL_NAME_CHANGE,
	__SYMPHONY_MSG_CHANNEL_ICON_CHANGE,
	__SYMPHONY_MSG_CHANNEL_PINNED_MESSAGE,
	__SYMPHONY_MSG_USER_JOIN,
	__SYMPHONY_MSG_GUILD_BOOST,
	__SYMPHONY_MSG_GUILD_BOOST_TIER_1,
	__SYMPHONY_MSG_GUILD_BOOST_TIER_2,
	__SYMPHONY_MSG_GUILD_BOOST_TIER_3,
	__SYMPHONY_MSG_CHANNEL_FOLLOW_ADD,
	__SYMPHONY_MSG_GUILD_DISCOVERY_DISQUALIFIED = 14,
	__SYMPHONY_MSG_GUILD_DISCOVERY_REQUALIFIED,
	__SYMPHONY_MSG_GUILD_DISCOVERY_GRACE_PERIOD_INITIAL_WARNING,
	__SYMPHONY_MSG_GUILD_DISCOVERY_GRACE_PERIOD_FINAL_WARNING,
	__SYMPHONY_MSG_THREAD_CREATED,
	__SYMPHONY_MSG_REPLY,
	__SYMPHONY_MSG_CHAT_INPUT_COMMAND,
	__SYMPHONY_MSG_THREAD_STARTER_MESSAGE,
	__SYMPHONY_MSG_GUILD_INVITE_REMINDER,
	__SYMPHONY_MSG_CONTEXT_MENU_COMMAND,
	__SYMPHONY_MSG_AUTO_MODERATION_ACTION,
	__SYMPHONY_MSG_ROLE_SUBSCRIPTION_PURCHASE,
	__SYMPHONY_MSG_INTERACTION_PREMIUM_UPSELL,
	__SYMPHONY_MSG_STAGE_START,
	__SYMPHONY_MSG_STAGE_END,
	__SYMPHONY_MSG_STAGE_SPEAKER,
	__SYMPHONY_MSG_STAGE_TOPIC = 31,
	__SYMPHONY_MSG_GUILD_APPLICATION_PREMIUM_SUBSCRIPTION
};

typedef unsigned long long snowflake_t;

struct symphony {
	void* ws;
	char* token;
	bool login;
	int repeat;
	int last;
	void** events;
	pthread_t heartbeet_thread;
	char* session_id;
	snowflake_t client_id;
};

struct symphony_author {
	char* username;
	snowflake_t id;
};

struct symphony_channel {
	struct symphony* s;
	char* name;
	snowflake_t id;
};

struct symphony_message {
	char* content;
	snowflake_t id;
	int type;
	struct symphony_author author;
	struct symphony_channel channel;
	struct symphony* symphony;
	struct symphony_message* replyto;
};

/*** Start internal stuff ***/
void __symphony_do_thing(struct symphony* s, char* payload, int payload_len);
void __symphony_dispatch(struct symphony* s, const char* eventname, void* arg);
void __symphony_message(struct symphony_message* msg, struct symphony* s, void* _data, bool loop);
/*** End internal stuff ***/

void symphony_library_init(void);
struct symphony* symphony_connect(const char* token);
void symphony_send(struct symphony_channel s, const char* message);
void symphony_set_status(struct symphony* s, const char* status, const char* activ0, const char* emoji, int activtype);
void symphony_on(struct symphony* s, const char* eventname, void(*func)(void*));
void symphony_loop(struct symphony* s);

#endif
