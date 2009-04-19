CC=gcc
ECLFAGS=
CFLAGS=-O3 -fno-crossjumping -fno-gcse -g $(ECFLAGS)

LD=$(CC)
LDFLAGS=
LIBS=-lgc -pthread

AR=ar
ARFLAGS=rc

RANLIB=ranlib

EXEEXT=


PLOFORTH_OBJS=ploforth.o repl.o token.o


all: ploforth$(EXEEXT)

ploforth$(EXEEXT): $(PLOFORTH_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(PLOFORTH_OBJS) $(LIBS) -o ploforth$(EXEEXT)

.SUFFIXES: c o

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f ploforth$(EXEEXT) $(PLOFORTH_OBJS)

deps:
	$(CC) $(CFLAGS) -MM *.c > deps

include deps
