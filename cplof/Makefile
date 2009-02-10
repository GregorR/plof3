CC=gcc
CFLAGS=-O3 -g

LD=$(CC)
LDFLAGS=
LIBS=-lgc

AR=ar
ARFLAGS=rc

RANLIB=ranlib

EXEEXT=


LIBPLOF_A_OBJS=psl.o
PSLI_OBJS=psli.o libplof.a


all: libplof.a psli$(EXEEXT)

libplof.a: $(LIBPLOF_A_OBJS)
	$(AR) $(ARFLAGS) libplof.a $(LIBPLOF_A_OBJS)
	$(RANLIB) libplof.a

psli$(EXEEXT): $(PSLI_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(PSLI_OBJS) $(LIBS) -o psli$(EXEEXT)

.SUFFIXES: c o

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f libplof.a $(LIBPLOF_A_OBJS)
	rm -f psli$(EXEEXT) $(PSLI_OBJS)

deps:
	$(CC) $(CFLAGS) -MM *.c > deps

include deps
