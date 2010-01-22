plof_includedir = $(datadir)/plof_include
plof_include_DATA = std.psl cnfi.psl nonfi.psl
plof_debugdir = $(plof_includedir)/debug/
plof_debug_DATA =  plof_include_debug/std.psl plof_include_debug/cnfi.psl plof_include_debug/nonfi.psl plof_include_debug/stddebug.psl

PUL_PSL_SOURCE=src/base/base.psl src/pul/pul_g.plof src/pul/pul.plof src/pul/object_g.plof

STD_PSL_SOURCE=src/pul/object.plof \
               src/pul/include_g.plof \
               src/pul/exceptions.plof \
               src/pul/boolean_g.plof src/pul/boolean.plof \
               src/pul/conditionals.plof \
               src/pul/dynamicTypes_g.plof src/pul/dynamicTypes.plof \
               src/pul/number_g.plof src/pul/number.plof \
               src/pul/string_g.plof src/pul/string.plof \
               src/pul/collection_g.plof src/pul/collection.plof \
	       src/pul/io.plof \
               src/pul/versions.plof \
               src/pul/nfi.plof

CNFI_PSL_SOURCE=src/pul/cnfi.plof src/pul/cio.plof

NONFI_PSL_SOURCE=src/pul/nonfi.plof

MOSTLYCLEANFILES=src/pul.psl src/base/base.psl puldebug.psl std.psl cnfi.psl nonfi.psl plof_include_debug/std.psl plof_include_debug/cnfi.psl plof_include_debug/nonfi.psl plof_include_debug/stddebug.psl

src/pul/pul.psl: $(PUL_PSL_SOURCE)
	$(PLOFC) -N $(PUL_PSL_SOURCE) -o $@

src/pul/puldebug.psl: $(PUL_PSL_SOURCE)
	$(PLOFC) -N --debug $(PLOF_FLAGS) $(PUL_PSL_SOURCE) -o $@

std.psl: src/pul/pul.psl $(STD_PSL_SOURCE)
	$(PLOFC) -N $(PLOF_FLAGS) src/pul/pul.psl $(STD_PSL_SOURCE) -o $@

plof_include_debug/std.psl: src/pul/pul.psl $(STD_PSL_SOURCE)
	$(PLOFC) -N --debug $(PLOF_FLAGS) src/pul/pul.psl $(STD_PSL_SOURCE) -o $@

plof_include_debug/stddebug.psl: src/pul/puldebug.psl $(STD_PSL_SOURCE)
	$(PLOFC) -N --debug $(PLOF_FLAGS) src/pul/puldebug.psl $(STD_PSL_SOURCE) -o $@

cnfi.psl: $(CNFI_PSL_SOURCE) std.psl
	$(PLOFC) $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o $@

plof_include_debug/cnfi.psl: $(CNFI_PSL_SOURCE) std.psl
	$(PLOFC) --debug $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o $@

nonfi.psl: $(NONFI_PSL_SOURCE) std.psl
	$(PLOFC) $(PLOF_FLAGS) $(NONFI_PSL_SOURCE) -o $@

plof_include_debug/nonfi.psl: $(NONFI_PSL_SOURCE) std.psl $(PLOF_REQ)
	$(PLOFC) --debug $(PLOF_FLAGS) $(NONFI_PSL_SOURCE) -o $@

src/base/base.psl: src/base/base.apsl
	$(PSLASM) $< $@


