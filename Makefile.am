ACLOCAL_AMFLAGS = -I m4 --install
dist_doc_DATA = README.md
pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = onql.pc

include_HEADERS = onql.h
lib_LTLIBRARIES = libonql.la
libonql_la_LDFLAGS = -version-info 0:0:0
libonql_la_SOURCES = onql.c safemem.c
libonql_la_CFLAGS = -Wall -Wextra -std=c11 `mysql_config --cflags --libs` `pkg-config --cflags --libs jansson`

AM_CFLAGS = $(DEPS_CFLAGS)
AM_LIBS = $(DEPS_LIBS)
