SRC = src/fetch.c
CC ?= cc
CFLAGS = -O2 -Wall -Wextra
DEBUGFLAGS = -g -Og -Wall -Wextra
PREFIX ?= /usr/local

all: swfetch

swfetch: ${SRC} src/config.h
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
