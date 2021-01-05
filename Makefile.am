AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -march=native -flto -I$(srcdir)/src

bin_PROGRAMS = \
bin/pounce

POUNCE_SOURCE_FILES = \
src/pounce.c

bin_pounce_SOURCES = $(POUNCE_SOURCE_FILES)
bin_pounce_LDADD = -lreactor -ldynamic

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in
maintainer-clean-local:; rm -rf autotools m4

indent:
	clang-format -i src/*.c

.PHONY: indent
