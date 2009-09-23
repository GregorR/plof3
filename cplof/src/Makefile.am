lib_LIBRARIES=libplof.a libplof_noparser.a
bin_PROGRAMS=cplof psli pslasm psldasm

AM_CFLAGS=-DHAVE_CONFIG_H

libplof_a_SOURCES=bignum.c memory.c psl.c pslfile.c
if GCC
libplof_a_CFLAGS=-O3 -fno-crossjumping -fno-gcse $(AM_CFLAGS)
endif

libplof_noparser_a_SOURCES=bignum.c memory.c psl.c pslfile.c
libplof_noparser_a_CFLAGS=
if GCC
libplof_noparser_a_CFLAGS+=-O3 -fno-crossjumping -fno-gcse $(AM_CFLAGS)
endif
libplof_noparser_a_CFLAGS+=-DPLOF_NO_PARSER

cplof_SOURCES=main.c whereami.c packrat.c prp.c
cplof_LDADD=libplof.a @CNFI_LIBS@

psli_SOURCES=psli.c whereami.c
psli_CFLAGS=-DPLOF_NO_PARSER
psli_LDADD=libplof_noparser.a @CNFI_LIBS@

pslasm_SOURCES=bignum.c lex.c parse.c pslasm.c pslfile.c
psldasm_SOURCES=bignum.c psldasm.c pslfile.c
