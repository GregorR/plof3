includeplofdir=$(includedir)/plof

lib_LIBRARIES=libplof.a libplof_noparser.a
includeplof_HEADERS=plof/bignum.h plof/buffer.h plof/helpers.h plof/memory.h \
plof/packrat.h plof/plof.h plof/prp.h plof/pslfile.h plof/psl.h
bin_PROGRAMS=cplof psli pslasm psldasm pslstrip
noinst_PROGRAMS=pslast

AM_CFLAGS=-DHAVE_CONFIG_H

libplof_a_SOURCES=bignum.c inlining.c intrinsics.c memory.c optimizations.c \
psl.c pslfile.c

libplof_noparser_a_SOURCES=bignum.c inlining.c intrinsics.c memory.c \
optimizations.c psl.c pslfile.c
libplof_noparser_a_CFLAGS=-DPLOF_NO_PARSER

cplof_SOURCES=ast.c main.c whereami.c packrat.c prp.c
cplof_LDADD=libplof.a @CNFI_LIBS@

psli_SOURCES=psli.c whereami.c
psli_CFLAGS=-DPLOF_NO_PARSER
psli_LDADD=libplof_noparser.a @CNFI_LIBS@

pslasm_SOURCES=bignum.c lex.c parse.c pslasm.c pslfile.c
psldasm_SOURCES=bignum.c psldasm.c pslfile.c

pslstrip_SOURCES=bignum.c pslstrip.c pslfile.c

pslast_SOURCES=ast.c pslast.c
pslast_CFLAGS=-DPLOF_NO_PARSER
pslast_LDADD=libplof_noparser.a @CNFI_LIBS@
