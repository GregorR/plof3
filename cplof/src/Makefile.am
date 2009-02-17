lib_LIBRARIES=libplof.a
bin_PROGRAMS=psli

AM_CFLAGS=-DHAVE_CONFIG_H

libplof_a_SOURCES=psl.c
if GCC
libplof_a_CFLAGS=-O3 -fno-crossjumping -fno-gcse $(AM_CFLAGS)
endif

psli_SOURCES=psli.c
psli_LDADD=libplof.a
