#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([cplof], [0], [cdgregorr@sourceforge.net])
AC_CONFIG_AUX_DIR([scripts])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/psl.c])
AC_CONFIG_HEADER([config.h])

m4_include([scripts/ac_compile_check_sizeof.ac])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB
AM_CONDITIONAL([GCC], [test x$GCC = xyes])

# Checks for libraries.
AC_CHECK_LIB([gc], [GC_malloc])
# AC_CHECK_LIB([ffi], [ffi_raw_call])

# Checks for header files.
#AC_HEADER_STDC
#AC_CHECK_HEADERS([stddef.h string.h unistd.h])
AC_CHECK_HEADERS([unistd.h gc/gc.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_TYPE_SIZE_T
#AC_C_VOLATILE
#AC_CHECK_TYPES([ptrdiff_t])
AC_COMPILE_CHECK_SIZEOF(void *)

# Checks for library functions.
#AC_FUNC_MEMCMP

# Inclusions/exclusions
AC_ARG_WITH([boxed-numbers],
            [AC_HELP_STRING([--with-boxed-numbers], [Fully box all numbers])])
AC_ARG_WITH([numbers-in-objects],
            [AC_HELP_STRING([--with-numbers-in-objects], [Put numbers directly in objects, which is faster than full boxing but slower than free ints])])
AC_ARG_WITH([free-ints],
            [AC_HELP_STRING([--with-free-ints], [Use completely unboxed ints, the default])])

if test x$with_boxed_numbers = xyes; then
    AC_DEFINE([PLOF_BOX_NUMBERS], [], [Fully box all numbers])
elif test x$with_numbers_in_objects = xyes; then
    AC_DEFINE([PLOF_NUMBERS_IN_OBJECTS], [], [Put numbers directly in objects])
else
    AC_DEFINE([PLOF_FREE_INTS], [], [Use completely unboxed ints])
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT