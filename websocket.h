/* $Id: websocket.h 9 2024-01-15 11:26:37Z nishi $ */
#ifndef __SYMPHONY_WEBSOCKET_H__
#define __SYMPHONY_WEBSOCKET_H__

#include "symphony.h"

void* __symphony_connect(const char* token, struct symphony* s);
void __symphony_block(void* client);
void __symphony_lock(void* client);
void __symphony_unlock(void* client);
void __symphony_write(void* client, const char* data);

#endif
