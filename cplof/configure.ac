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

# Checks for library functions.
#AC_FUNC_MEMCMP

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
