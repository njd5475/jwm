#!/bin/sh
aclocal
automake -ac
autoreconf -fi
autoconf
touch config.rpath
