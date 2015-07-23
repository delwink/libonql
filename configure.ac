AC_PREREQ([2.60])
AC_INIT([libsqon],[1.2.0],[support@delwink.com])

AC_CONFIG_SRCDIR([sqon.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AM_SILENT_RULES([yes])
AM_PROG_AR
LT_INIT
AC_PROG_CC

PKG_CHECK_MODULES([jansson], [jansson])
PKG_CHECK_MODULES([libpq], [libpq])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
])
AC_OUTPUT
