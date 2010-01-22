plof_includedir = $(datadir)/plof_include
plof_include_DATA = std.psl cnfi.psl nonfi.psl
plof_debugdir = $(plof_includedir)/debug/
plof_debug_DATA =  debug/std.psl debug/cnfi.psl debug/nonfi.psl debug/stddebug.psl

PUL_PSL_SOURCE=base.psl $(srcdir)/src/pul/pul_g.plof $(srcdir)/src/pul/pul.plof $(srcdir)/src/pul/object_g.plof

STD_PSL_SOURCE=$(srcdir)/src/pul/object.plof \
               $(srcdir)/src/pul/include_g.plof \
               $(srcdir)/src/pul/exceptions.plof \
               $(srcdir)/src/pul/boolean_g.plof $(srcdir)/src/pul/boolean.plof \
               $(srcdir)/src/pul/conditionals.plof \
               $(srcdir)/src/pul/dynamicTypes_g.plof $(srcdir)/src/pul/dynamicTypes.plof \
               $(srcdir)/src/pul/number_g.plof $(srcdir)/src/pul/number.plof \
               $(srcdir)/src/pul/string_g.plof $(srcdir)/src/pul/string.plof \
               $(srcdir)/src/pul/collection_g.plof $(srcdir)/src/pul/collection.plof \
	       $(srcdir)/src/pul/io.plof \
               $(srcdir)/src/pul/versions.plof \
               $(srcdir)/src/pul/nfi.plof

CNFI_PSL_SOURCE=$(srcdir)/src/pul/cnfi.plof $(srcdir)/src/pul/cio.plof

NONFI_PSL_SOURCE=$(srcdir)/src/pul/nonfi.plof

MOSTLYCLEANFILES=pul.psl base.psl puldebug.psl std.psl cnfi.psl nonfi.psl debug/std.psl debug/cnfi.psl debug/nonfi.psl debug/stddebug.psl

pul.psl: $(PUL_PSL_SOURCE)
	$(PLOFC) -N $(PUL_PSL_SOURCE) -o $@

puldebug.psl: $(PUL_PSL_SOURCE)
	$(PLOFC) -N --debug $(PLOF_FLAGS) $(PUL_PSL_SOURCE) -o $@

std.psl: pul.psl $(STD_PSL_SOURCE)
	$(PLOFC) -N $(PLOF_FLAGS) pul.psl $(STD_PSL_SOURCE) -o $@

debug/std.psl: debug pul.psl $(STD_PSL_SOURCE)
	mkdir -p debug
	$(PLOFC) -N --debug $(PLOF_FLAGS) pul.psl $(STD_PSL_SOURCE) -o $@

debug/stddebug.psl: debug puldebug.psl $(STD_PSL_SOURCE)
	mkdir -p debug
	$(PLOFC) -N --debug $(PLOF_FLAGS) puldebug.psl $(STD_PSL_SOURCE) -o $@

cnfi.psl: $(CNFI_PSL_SOURCE) std.psl
	$(PLOFC) $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o $@

debug/cnfi.psl: debug $(CNFI_PSL_SOURCE) std.psl
	mkdir -p debug
	$(PLOFC) --debug $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o $@

nonfi.psl: $(NONFI_PSL_SOURCE) std.psl
	$(PLOFC) $(PLOF_FLAGS) $(NONFI_PSL_SOURCE) -o $@

debug/nonfi.psl: debug $(NONFI_PSL_SOURCE) std.psl $(PLOF_REQ)
	mkdir -p debug
	$(PLOFC) --debug $(PLOF_FLAGS) $(NONFI_PSL_SOURCE) -o $@

base.psl: src/base/base.apsl
	$(PSLASM) $< $@
