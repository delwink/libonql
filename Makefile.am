ACLOCAL_AMFLAGS = -I m4 --install
dist_doc_DATA = README.md
pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = sqon.pc

include_HEADERS = sqon.h
lib_LTLIBRARIES = libsqon.la
libsqon_la_LDFLAGS = -version-info 0:0:0
libsqon_la_SOURCES = sqon.c safemem.c internal.c
libsqon_la_CFLAGS = -Wall -Wextra -std=c11 `mysql_config --cflags --libs` `pkg-config --cflags --libs jansson`

AM_CFLAGS = $(DEPS_CFLAGS)
AM_LIBS = $(DEPS_LIBS)
