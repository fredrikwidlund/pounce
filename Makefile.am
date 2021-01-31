SUBDIRS = lib/libdynamic lib/libreactor .
install installdirs: SUBDIRS =

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11

bin_PROGRAMS = \
bin/pounce

POUNCE_SOURCE_FILES = \
src/main.c \
src/pounce.c \
src/url.c \
src/worker.c \
src/stats.c \
src/connection.c \
src/net.c \
src/http_client.c

POUNCE_HEADER_FILES = \
src/pounce.h \
src/url.h \
src/worker.h \
src/stats.h \
src/connection.h \
src/net.h \
src/http_client.h

bin_pounce_SOURCES = $(POUNCE_SOURCE_FILES) $(POUNCE_HEADER_FILES)
bin_pounce_LDADD = lib/libreactor/libreactor.la lib/libdynamic/libdynamic.la -lm
bin_pounce_LDFLAGS = -pthread

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in

maintainer-clean-local:
	rm -rf autotools m4

indent:
	clang-format -i src/*.c

docker-build:
	docker build -t pounce-sdk .
	id=$$(docker create pounce-sdk); docker cp $$id:/pounce-1.0.0/bin/pounce pounce; docker rm -v $$id

.PHONY: indent
