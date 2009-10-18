plof_includedir = $(datadir)/plof_include
plof_include_DATA = plof_include/std.psl plof_include/cnfi.psl plof_include/nonfi.psl
plof_debugdir = $(plof_includedir)/debug/
plof_debug_DATA =  plof_include/debug/std.psl plof_include/debug/cnfi.psl plof_include/debug/nonfi.psl plof_include/debug/stddebug.psl

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

MOSTLYCLEANFILES=src/pul.psl src/base/base.psl plof_include/puldebug.psl plof_include/std.psl plof_include/cnfi.psl plof_include/nonfi.psl plof_include/debug/std.psl plof_include/debug/cnfi.psl plof_include/debug/nonfi.psl plof_include/debug/stddebug.psl

src/pul/pul.psl: $(PUL_PSL_SOURCE)
	$(PLOFC) -N $(PUL_PSL_SOURCE) -o $@

src/pul/puldebug.psl: $(PUL_PSL_SOURCE)
	$(PLOFC) -N --debug $(PLOF_FLAGS) $(PUL_PSL_SOURCE) -o $@

plof_include/std.psl: src/pul/pul.psl $(STD_PSL_SOURCE)
	$(PLOFC) -N $(PLOF_FLAGS) src/pul/pul.psl $(STD_PSL_SOURCE) -o $@

plof_include/debug/std.psl: src/pul/pul.psl $(STD_PSL_SOURCE)
	$(PLOFC) -N --debug $(PLOF_FLAGS) src/pul/pul.psl $(STD_PSL_SOURCE) -o $@

plof_include/debug/stddebug.psl: src/pul/puldebug.psl $(STD_PSL_SOURCE)
	$(PLOFC) -N --debug $(PLOF_FLAGS) src/pul/puldebug.psl $(STD_PSL_SOURCE) -o $@

plof_include/cnfi.psl: $(CNFI_PSL_SOURCE) plof_include/std.psl
	$(PLOFC) $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o $@

plof_include/debug/cnfi.psl: $(CNFI_PSL_SOURCE) plof_include/debug/std.psl
	$(PLOFC) --debug $(PLOF_FLAGS) $(CNFI_PSL_SOURCE) -o $@

plof_include/nonfi.psl: $(NONFI_PSL_SOURCE) plof_include/std.psl
	$(PLOFC) $(PLOF_FLAGS) $(NONFI_PSL_SOURCE) -o $@

plof_include/debug/nonfi.psl: $(NONFI_PSL_SOURCE) plof_include/std.psl $(PLOF_REQ)
	$(PLOFC) --debug $(PLOF_FLAGS) $(NONFI_PSL_SOURCE) -o $@

src/base/base.psl: src/base/base.apsl
	$(PSLASM) $< $@


