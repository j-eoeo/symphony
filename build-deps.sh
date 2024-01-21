#!/bin/sh
# $Id: build-deps.sh 6 2024-01-15 03:17:53Z nishi $
if [ ! -e "libwsclient" ]; then
	git clone https://github.com/payden/libwsclient
else
	cd libwsclient
	git pull
	cd ..
fi
cd libwsclient
./autogen.sh
export CFLAGS="$CFLAGS -fcommon"
./configure --enable-static
MAKE=gmake
which $MAKE
if [ ! "$?" = "0" ]; then
	MAKE=make
fi
$MAKE
cd ..
