#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.6)
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
AC_SEARCH_LIBS([GC_malloc], [gc])

# Need libdl and libffi for CNFI
have_dl=no
AC_CHECK_LIB([dl], [dlopen], [
    AC_CHECK_HEADER([dlfcn.h], [
        have_dl=yes
    ], [])
], [])

have_ffi=no
AC_CHECK_LIB([ffi], [ffi_call], [
    AC_CHECK_HEADER([ffi.h], [
        have_ffi=yes
    ], [])
], [])


# Need PCRE for parsing
AC_CHECK_LIB([pcre], [pcre_compile])


# Checks for header files.
#AC_HEADER_STDC
#AC_CHECK_HEADERS([stddef.h string.h unistd.h])
AC_CHECK_HEADERS([stdint.h unistd.h gc/gc.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_TYPE_SIZE_T
#AC_C_VOLATILE
#AC_CHECK_TYPES([ptrdiff_t])
AC_COMPILE_CHECK_SIZEOF([void *])
AC_C_BIGENDIAN()

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
else
    AC_DEFINE([PLOF_FREE_INTS], [], [Use completely unboxed ints])
fi

# CNFI
AC_ARG_WITH([cnfi],
            [AC_HELP_STRING([--without-cnfi], [Do not include CNFI support])])

cnfi_libs=
if test "x$with_cnfi" != "xno" -a "x$have_dl" = "xyes" -a "x$have_ffi" = "xyes"; then
    AC_DEFINE([WITH_CNFI], [1], [Enable support for CNFI])
    cnfi_libs="-lffi -ldl"
else
    AC_MSG_WARN([[CNFI support will not be included. Although CNFI is not strictly required, without it, Plof programs cannot interact with lower-level code, so it's very unlikely that this is the desired configuration.]])
fi
AC_SUBST([CNFI_LIBS], [$cnfi_libs])

# Timing debugging
DEBUG_TIMING=no
AC_ARG_WITH([debug-timing],
            [AC_HELP_STRING([--with-debug-timing], [Output timing debugger info])])
if test "x$with_debug_timing" = "xyes"; then
    AC_DEFINE([DEBUG_TIMING], [], [Enable timer debugging])
    AC_CHECK_LIB([rt], [clock_gettime])
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
