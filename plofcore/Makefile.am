plof_includedir = $(datadir)/plof_include
plof_include_DATA = std.psl cnfi.psl nonfi.psl
plof_debugdir = $(plof_includedir)/debug/
plof_debug_DATA = debug/std.psl debug/cnfi.psl debug/nonfi.psl debug/stddebug.psl

PUL_PSL_SOURCE=base.psl $(srcdir)/src/pul/pul_g.plof $(srcdir)/src/pul/pul.plof $(srcdir)/src/pul/object_g.plof

STD_PSL_SOURCE=$(srcdir)/src/pul/object.plof \
               $(srcdir)/src/pul/include_g.plof \
               $(srcdir)/src/pul/exceptions.plof \
               $(srcdir)/src/pul/boolean_g.plof $(srcdir)/src/pul/boolean.plof \
               $(srcdir)/src/pul/comparisons_g.plof $(srcdir)/src/pul/comparisons.plof \
               $(srcdir)/src/pul/conditionals.plof \
               $(srcdir)/src/pul/dynamicTypes_g.plof $(srcdir)/src/pul/dynamicTypes.plof \
               $(srcdir)/src/pul/number_g.plof $(srcdir)/src/pul/number.plof \
               $(srcdir)/src/pul/string_g.plof $(srcdir)/src/pul/string.plof \
               $(srcdir)/src/pul/exception_strings.plof \
               $(srcdir)/src/pul/modules_g.plof $(srcdir)/src/pul/modules.plof \
               $(srcdir)/src/pul/collection_g.plof $(srcdir)/src/pul/collection.plof \
               $(srcdir)/src/pul/eval_g.plof \
               $(srcdir)/src/pul/introspection.plof \
               $(srcdir)/src/pul/serialization.plof \
               $(srcdir)/src/pul/io.plof \
               $(srcdir)/src/pul/builtins.plof \
               $(srcdir)/src/pul/nfi.plof

CNFI_PSL_SOURCE=$(srcdir)/src/pul/cnfi.plof $(srcdir)/src/pul/cio.plof

NONFI_PSL_SOURCE=$(srcdir)/src/pul/nonfi.plof

MOSTLYCLEANFILES=pul.psl base.psl puldebug.psl std.psl cnfi.psl nonfi.psl debug/std.psl debug/cnfi.psl debug/nonfi.psl debug/stddebug.psl

pul.psl: $(PLOFC) $(PUL_PSL_SOURCE)
	$(PLOFC) -N -c $(PUL_PSL_SOURCE) -o $@

puldebug.psl: $(PLOFC) $(PUL_PSL_SOURCE)
	$(PLOFC) -N -c --debug $(PLOF_FLAGS) $(PUL_PSL_SOURCE) -o $@

std.psl: $(PLOFC) pul.psl $(STD_PSL_SOURCE)
	$(PLOFC) -N -c $(PLOF_FLAGS) pul.psl $(STD_PSL_SOURCE) -o $@

debug/std.psl: $(PLOFC) pul.psl $(STD_PSL_SOURCE)
	mkdir -p debug
	$(PLOFC) -N -c --debug $(PLOF_FLAGS) pul.psl $(STD_PSL_SOURCE) -o $@

debug/stddebug.psl: $(PLOFC) puldebug.psl $(STD_PSL_SOURCE)
	mkdir -p debug
	$(PLOFC) -N -c --debug $(PLOF_FLAGS) puldebug.psl $(STD_PSL_SOURCE) -o $@

cnfi.psl: $(PLOFC) $(CNFI_PSL_SOURCE) std.psl
	$(PLOFC) -c $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o $@

debug/cnfi.psl: $(PLOFC) $(CNFI_PSL_SOURCE) std.psl
	mkdir -p debug
	$(PLOFC) -c --debug $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o $@

nonfi.psl: $(PLOFC) $(NONFI_PSL_SOURCE) std.psl
	$(PLOFC) -c $(PLOF_FLAGS) $(NONFI_PSL_SOURCE) -o $@

debug/nonfi.psl: $(PLOFC) $(NONFI_PSL_SOURCE) std.psl $(PLOF_REQ)
	mkdir -p debug
	$(PLOFC) -c --debug $(PLOF_FLAGS) $(NONFI_PSL_SOURCE) -o $@

base.psl: $(PSLASM) src/base/base.apsl
	$(PSLASM) $(srcdir)/src/base/base.apsl $@
