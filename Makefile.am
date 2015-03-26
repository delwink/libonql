ACLOCAL_AMFLAGS = -I m4 --install
dist_doc_DATA = README.md
pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = sqon.pc

include_HEADERS = sqon.h
lib_LTLIBRARIES = libsqon.la
libsqon_la_SOURCES = sqon.c result.c

libsqon_la_LDFLAGS = -version-info 0:4:0 `mysql_config --libs`

libsqon_la_CFLAGS = -Wall -Wextra -Wunreachable-code -ftrapv -std=c11

libsqon_la_LIBADD = $(jansson_LIBS)

AM_CFLAGS = $(DEPS_CFLAGS)
AM_LIBS = $(DEPS_LIBS)
