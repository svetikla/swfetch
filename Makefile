SRC = src/fetch.c
CC ?= cc
CFLAGS = -O2 -std=c99 -Wall -Wextra
DEBUGFLAGS = -g -Og -std=c99 -Wall -Wextra
PREFIX ?= /usr/local

all: swfetch

swfetch: ${SRC} src/config.h src/wm.h
	${CC} ${CFLAGS} ${SRC} -o swfetch

debug:
	${CC}  ${DEBUGFLAGS} ${SRC} -o swfetch-debug

clean:
	rm -rf swfetch swfetch.dSYM swfetch-debug swfetch-debug.dSYM

install: swfetch
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp swfetch ${DESTDIR}${PREFIX}/bin
	chmod 711 ${DESTDIR}${PREFIX}/bin/swfetch

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/swfetch

.PHONY: all clean debug install uninstall
