PLOFC=../cplof/src/cplof -c
#PLOF_FLAGS=--debug
STD_PSL=../plof_include/std.psl

all: \
    test.psl \
    curry.psl \
    test_cnfi.psl \
    include.psl

../plof_include/included.psl: included.psl
	cp included.psl ../plof_include

include.psl: ../plof_include/included.psl

included.psl: included.plof
	$(PLOFC) included.plof -o included.psl

.SUFFIXES: .plof .psl

.plof.psl:
	$(PLOFC) $< -o $@

clean:
	rm -f *.psl
