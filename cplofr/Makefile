CC=gcc
CFLAGS=-O2 -g
LDFLAGS=
LIBS=-lgc

EXEEXT=

PSLI_OBJS=psl.o psli.o

all: psli$(EXEEXT)

psli$(EXEEXT): $(PSLI_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(PSLI_OBJS) $(LIBS) -o psli$(EXEEXT)

.SUFFIXES: c o

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f psli$(EXEEXT) $(PSLI_OBJS)
