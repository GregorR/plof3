PREFIX=/usr

PSLASM=./cplof/src/pslasm
PLOF=./cplof/src/cplof
PLOF_REQ=cplof/src/cplof
#PLOF_FLAGS=--debug

# base.psl contains the base grammar
BASE_PSL_SOURCE=core/base/base.apsl

# pul.psl is the minimum required for a PUL system
PUL_PSL_SOURCE=base.psl core/pul/pul_g.plof core/pul/pul.plof core/pul/object_g.plof

# std.psl is the PUL standard library
STD_PSL_SOURCE=core/pul/object.plof \
               core/pul/include_g.plof \
               core/pul/exceptions.plof \
               core/pul/boolean_g.plof core/pul/boolean.plof \
               core/pul/conditionals.plof \
               core/pul/dynamicTypes_g.plof core/pul/dynamicTypes.plof \
               core/pul/number_g.plof core/pul/number.plof \
               core/pul/string_g.plof core/pul/string.plof \
               core/pul/collection_g.plof core/pul/collection.plof \
               core/pul/versions.plof \
               core/pul/nfi.plof

# cnfi.psl is the C NFI, loaded by nfi.plof if CNFI is set
CNFI_PSL_SOURCE=core/pul/cnfi.plof

all: cplof/src/cplof \
     plof_include/std.psl plof_include/debug/std.psl plof_include/debug/stddebug.psl \
     plof_include/cnfi.psl plof_include/debug/cnfi.psl
#all: plof_include/std.psl plof_include/cnfi.psl

base.psl: $(BASE_PSL_SOURCE) $(PSLASM)
	$(PSLASM) $(BASE_PSL_SOURCE) base.psl


pul.psl: $(PUL_PSL_SOURCE) $(PLOF_REQ)
	$(PLOF) -N $(PLOF_FLAGS) $(PUL_PSL_SOURCE) -o pul.psl

puldebug.psl: $(PUL_PSL_SOURCE) $(PLOF_REQ)
	$(PLOF) -N --debug $(PLOF_FLAGS) $(PUL_PSL_SOURCE) -o puldebug.psl


plof_include/std.psl: pul.psl $(STD_PSL_SOURCE) $(PLOF_REQ)
	$(PLOF) -N $(PLOF_FLAGS) pul.psl $(STD_PSL_SOURCE) -o plof_include/std.psl

plof_include/debug/std.psl: pul.psl $(STD_PSL_SOURCE) $(PLOF_REQ)
	$(PLOF) -N --debug $(PLOF_FLAGS) pul.psl $(STD_PSL_SOURCE) -o plof_include/debug/std.psl

plof_include/debug/stddebug.psl: puldebug.psl $(STD_PSL_SOURCE) $(PLOF_REQ)
	$(PLOF) -N --debug $(PLOF_FLAGS) puldebug.psl $(STD_PSL_SOURCE) -o plof_include/debug/stddebug.psl


plof_include/cnfi.psl: $(CNFI_PSL_SOURCE) plof_include/std.psl $(PLOF_REQ)
	$(PLOF) $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o plof_include/cnfi.psl

plof_include/debug/cnfi.psl: $(CNFI_PSL_SOURCE) plof_include/debug/std.psl $(PLOF_REQ)
	$(PLOF) --debug $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o plof_include/debug/cnfi.psl


cplof/src/pslasm: cplof/src/cplof
	true

cplof/src/cplof:
	cd cplof ; ./configure --prefix="$(PREFIX)" ; $(MAKE)



install: all
	-cd cplof ; make install
	mkdir -p $(PREFIX)/share/plof_include
	cp -dRf plof_include/* $(PREFIX)/share/plof_include



clean:
	-cd cplof ; make distclean
	rm -f base.psl pul.psl puldebug.psl \
	    plof_include/std.psl plof_include/debug/std.psl \
	    plof_include/debug/stddebug.psl \
	    plof_include/cnfi.psl plof_include/debug/cnfi.psl