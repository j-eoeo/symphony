# $Id: GNUmakefile 13 2024-01-16 14:25:52Z nishi $
CC := cc
CFLAGS := -D_BSD_SOURCE $(DEBUG) -fcommon -std=c99 -fPIC
LDFLAGS := -fcommon -L libwsclient/.libs -Wl,-R libwsclient/.libs

ifeq ($(shell uname -s),NetBSD)
CFLAGS += -I /usr/pkg/include
LDFLAGS += -L /usr/pkg/lib -Wl,-R /usr/pkg/lib
endif

.PHONY: all clean

all: ./libsymphony.so ./testbot

./libsymphony.so: ./symphony.o ./websocket.o ./util.o
	$(CC) -shared $(LDFLAGS) -o $@ $^ -lwsclient -lcjson

./%.o: ./%.c ./%.h
	$(CC) -c $(CFLAGS) -I libwsclient -o $@ $<

./testbot: ./testbot.c ./libsymphony.so
	$(CC) -g -L . -I . -o $@ $< -lsymphony

clean:
	rm -f *.so *.o libwsclient
