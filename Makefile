SRC = src/fetch.c
CC ?= cc
CFLAGS = -O2 -Wall -Wextra
PREFIX ?= /usr/local

all: swfetch

swfetch: ${SRC} src/config.h
	${CC} ${CFLAGS} ${SRC} -o swfetch

clean:
	rm -rf swfetch swfetch.dSYM

install: swfetch
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp swfetch ${DESTDIR}${PREFIX}/bin
	chmod 711 ${DESTDIR}${PREFIX}/bin/swfetch

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/swfetch

.PHONY: all clean debug install uninstall
