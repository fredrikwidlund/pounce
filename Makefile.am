SUBDIRS = lib/libdynamic lib/libreactor .
AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -march=native -flto -I$(srcdir)/src -I$(srcdir)/lib/libdynamic/src -I$(srcdir)/lib/libreactor/src

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
bin_pounce_LDADD = -lreactor -ldynamic -lm
bin_pounce_LDFLAGS = -L$(srcdir)/lib/libdynamic -L$(srcdir)/lib/libreactor -pthread

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in
maintainer-clean-local:; rm -rf autotools m4

indent:
	clang-format -i src/*.c

.PHONY: indent
