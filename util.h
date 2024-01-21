/* $Id: util.h 12 2024-01-16 07:51:54Z nishi $ */
#ifndef __SYMPHONY_UTIL_H__
#define __SYMPHONY_UTIL_H__

#include "symphony.h"

char* __symphony_strcat(const char* str1, const char* str2);
char* __symphony_strcat3(const char* str1, const char* str2, const char* str3);
int __symphony_http(struct symphony* symph, const char* method, const char* url, const char* data);

#endif
