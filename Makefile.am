AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -I$(srcdir)/src -I$(srcdir)/lib/libdynamic/src -I$(srcdir)/lib/libreactor/src

EXTRA_DIST = lib/libdynamic lib/libreactor

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
#bin_pounce_LDFLAGS = -L$(srcdir)/lib/libdynamic -L$(srcdir)/lib/libreactor -pthread

lib/libreactor/libreactor.la:
	$(MAKE) -C lib/libreactor CFLAGS="$(CFLAGS) -I../libdynamic/src"

lib/libdynamic/libdynamic.la:
	$(MAKE) -C lib/libdynamic

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in

dist-hook:
	rm -f lib/libdynamic/Makefile lib/libdynamic/libdynamic.pc
	rm -f lib/libreactor/Makefile lib/libreactor/libreactor.pc
	rm -rf `find $(distdir) -name ".deps" -o -name ".dirstamp"`

clean-local:
	$(MAKE) -C lib/libreactor clean
	$(MAKE) -C lib/libdynamic clean

maintainer-clean-local:
	rm -rf autotools m4

indent:
	clang-format -i src/*.c

.PHONY: indent
