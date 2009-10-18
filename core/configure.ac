#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.6)
AC_INIT([plof-core], [0], [cdgregorr@sourceforge.net])
AC_CONFIG_AUX_DIR([scripts])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/base/base.apsl])

# Checks for programs.
AC_CHECK_PROGS([PSLASM], [pslasm])
# Check for a generic plofc or cplof in particular.
AC_CHECK_PROGS([PLOFC], [plofc])
AC_CHECK_PROGS([PLOFC], [cplof])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
