PLOFC=../dplof/plofc
#PLOF_FLAGS=--debug
STD_PSL=../plof_include/std.psl

all: test.psl curry.psl cnfi.psl

.SUFFIXES: .plof .psl

.plof.psl:
	$(PLOFC) $(STD_PSL) $< -o $@

clean:
	rm -f *.psl
