#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.6)
AC_INIT([plof], [0], [cdgregorr@sourceforge.net])
AC_CONFIG_AUX_DIR([scripts])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([cplof/src/psl.c])

AC_CONFIG_SUBDIRS([cplof])
PSLASM="`pwd`/cplof/src/pslasm"
export PSLASM
PLOFC="`pwd`/cplof/src/cplof"
export PLOFC
AC_CONFIG_SUBDIRS([plofcore])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
