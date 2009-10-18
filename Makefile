PREFIX=/usr
export CPLOF="$(PWD)/cplof/src/cplof"
export PSLASM="$(PWD)/cplof/src/pslasm"

all: CPLOF CORE

CPLOF:
	cd cplof ; ./configure --prefix="$(PREFIX)" ; $(MAKE)

CORE: CPLOF
	cd core ; ./configure --prefix="$(PREFIX)" ; $(MAKE)

install: all
	-cd cplof ; make install
	-cd core ; make install

clean:
	-cd cplof ; make distclean
	-cd core ; make distclean
