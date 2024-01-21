/* $Id: util.c 12 2024-01-16 07:51:54Z nishi $ */
#include "util.h"

#include "symphony.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

#include <openssl/ssl.h>

char* __symphony_strcat(const char* str1, const char* str2){
	char* str = malloc(strlen(str1) + strlen(str2) + 1);
	strcpy(str, str1);
	strcpy(str + strlen(str1), str2);
	str[strlen(str1) + strlen(str2)] = 0;
	return str;
}

char* __symphony_strcat3(const char* str1, const char* str2, const char* str3){
	char* tmp = __symphony_strcat(str1, str2);
	char* str = __symphony_strcat(tmp, str3);
	free(tmp);
	return str;
}

int __symphony_http(struct symphony* symph, const char* method, const char* url, const char* data){
#ifdef _DEBUG_
	fprintf(stderr, "DNS........ Resolving\n");
#endif
	struct addrinfo hints;
	struct addrinfo* result;
	struct addrinfo* rp;
	int s;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	s = getaddrinfo("discord.com", "443", &hints, &result);
	if(s != 0) return -1;
	int sock;
	for(rp = result; rp != NULL; rp = rp->ai_next){
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock == -1) continue;
		if(connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
		close(sock);
	}
	freeaddrinfo(result);
	if(rp == NULL){
#ifdef _DEBUG_
		fprintf(stderr, "DNS........ Failed to resolve\n");
#endif
		return -1;
	}
#ifdef _DEBUG_
	fprintf(stderr, "DNS........ Resolved\n");
#endif
	SSL_library_init();

	const SSL_METHOD* m = TLSv1_2_client_method();
	SSL_CTX* ctx = SSL_CTX_new(m);
	SSL* ssl = SSL_new(ctx);
	SSL_set_fd(ssl, sock);
	SSL_connect(ssl);

	SSL_write(ssl, method, strlen(method));
	SSL_write(ssl, " ", 1);
	SSL_write(ssl, url, strlen(url));
	SSL_write(ssl, " HTTP/1.1\r\n", 11);

#define DISCORD "Host: discord.com\r\n"
#define CTYPE "Content-Type: application/json\r\n"
#define CLEN "Content-Length: "
#define CONN "Connection: closed"
#define AUTH "Authorization: Bot "
	SSL_write(ssl, DISCORD, strlen(DISCORD));
	if(data != NULL){
		SSL_write(ssl, CTYPE, strlen(CTYPE));
		SSL_write(ssl, CLEN, strlen(CLEN));
		char* clen = malloc(512);
		memset(clen, 0, 512);
		sprintf(clen, "%d", strlen(data));
		SSL_write(ssl, clen, strlen(clen));
		free(clen);
		SSL_write(ssl, "\r\n", 2);
		SSL_write(ssl, AUTH, strlen(AUTH));
		SSL_write(ssl, symph->token, strlen(symph->token));
		SSL_write(ssl, "\r\n", 2);
		SSL_write(ssl, CONN, strlen(CONN));
		SSL_write(ssl, "\r\n", 2);
	}
	SSL_write(ssl, "\r\n", 2);
	if(data != NULL){
		SSL_write(ssl, data, strlen(data));
	}
	char databuf[512];
	while(true){
		int v = SSL_read(ssl, databuf, 512);
		if(v <= 0) break;
	}
	SSL_shutdown(ssl);
	close(sock);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

#ifdef _DEBUG_
	fprintf(stderr, "HTTP....... Disconnected\n");
#endif
}
