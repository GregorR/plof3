/* generated by gen.c */
case psl_push0:
    FOREACH(push0);
#include "optim/push0.c"
    break;
case psl_push1:
    FOREACH(push1);
#include "optim/push1.c"
    break;
case psl_push2:
    FOREACH(push2);
#include "optim/push2.c"
    break;
case psl_push3:
    FOREACH(push3);
#include "optim/push3.c"
    break;
case psl_push4:
    FOREACH(push4);
#include "optim/push4.c"
    break;
case psl_push5:
    FOREACH(push5);
#include "optim/push5.c"
    break;
case psl_push6:
    FOREACH(push6);
#include "optim/push6.c"
    break;
case psl_push7:
    FOREACH(push7);
#include "optim/push7.c"
    break;
case psl_pop:
    FOREACH(pop);
#include "optim/pop.c"
    break;
case psl_this:
    FOREACH(this);
#include "optim/this.c"
    break;
case psl_null:
    FOREACH(null);
#include "optim/null.c"
    break;
case psl_global:
    FOREACH(global);
#include "optim/global.c"
    break;
case psl_new:
    FOREACH(new);
#include "optim/new.c"
    break;
case psl_combine:
    FOREACH(combine);
#include "optim/combine.c"
    break;
case psl_member:
    FOREACH(member);
#include "optim/member.c"
    break;
case psl_memberset:
    FOREACH(memberset);
#include "optim/memberset.c"
    break;
case psl_parent:
    FOREACH(parent);
#include "optim/parent.c"
    break;
case psl_parentset:
    FOREACH(parentset);
#include "optim/parentset.c"
    break;
case psl_call:
    FOREACH(call);
#include "optim/call.c"
    break;
case psl_throw:
    FOREACH(throw);
#include "optim/throw.c"
    break;
case psl_catch:
    FOREACH(catch);
#include "optim/catch.c"
    break;
case psl_cmp:
    FOREACH(cmp);
#include "optim/cmp.c"
    break;
case psl_concat:
    FOREACH(concat);
#include "optim/concat.c"
    break;
case psl_wrap:
    FOREACH(wrap);
#include "optim/wrap.c"
    break;
case psl_resolve:
    FOREACH(resolve);
#include "optim/resolve.c"
    break;
case psl_while:
    FOREACH(while);
#include "optim/while.c"
    break;
case psl_calli:
    FOREACH(calli);
#include "optim/calli.c"
    break;
case psl_replace:
    FOREACH(replace);
#include "optim/replace.c"
    break;
case psl_array:
    FOREACH(array);
#include "optim/array.c"
    break;
case psl_aconcat:
    FOREACH(aconcat);
#include "optim/aconcat.c"
    break;
case psl_length:
    FOREACH(length);
#include "optim/length.c"
    break;
case psl_lengthset:
    FOREACH(lengthset);
#include "optim/lengthset.c"
    break;
case psl_index:
    FOREACH(index);
#include "optim/index.c"
    break;
case psl_indexset:
    FOREACH(indexset);
#include "optim/indexset.c"
    break;
case psl_members:
    FOREACH(members);
#include "optim/members.c"
    break;
case psl_locals:
    FOREACH(locals);
#include "optim/locals.c"
    break;
case psl_local:
    FOREACH(local);
#include "optim/local.c"
    break;
case psl_localset:
    FOREACH(localset);
#include "optim/localset.c"
    break;
case psl_nullarray:
    FOREACH(nullarray);
#include "optim/nullarray.c"
    break;
case psl_rawlength:
    FOREACH(rawlength);
#include "optim/rawlength.c"
    break;
case psl_slice:
    FOREACH(slice);
#include "optim/slice.c"
    break;
case psl_rawcmp:
    FOREACH(rawcmp);
#include "optim/rawcmp.c"
    break;
case psl_extractraw:
    FOREACH(extractraw);
#include "optim/extractraw.c"
    break;
case psl_integer:
    FOREACH(integer);
#include "optim/integer.c"
    break;
case psl_intwidth:
    FOREACH(intwidth);
#include "optim/intwidth.c"
    break;
case psl_mul:
    FOREACH(mul);
#include "optim/mul.c"
    break;
case psl_div:
    FOREACH(div);
#include "optim/div.c"
    break;
case psl_mod:
    FOREACH(mod);
#include "optim/mod.c"
    break;
case psl_add:
    FOREACH(add);
#include "optim/add.c"
    break;
case psl_sub:
    FOREACH(sub);
#include "optim/sub.c"
    break;
case psl_lt:
    FOREACH(lt);
#include "optim/lt.c"
    break;
case psl_lte:
    FOREACH(lte);
#include "optim/lte.c"
    break;
case psl_eq:
    FOREACH(eq);
#include "optim/eq.c"
    break;
case psl_ne:
    FOREACH(ne);
#include "optim/ne.c"
    break;
case psl_gt:
    FOREACH(gt);
#include "optim/gt.c"
    break;
case psl_gte:
    FOREACH(gte);
#include "optim/gte.c"
    break;
case psl_sl:
    FOREACH(sl);
#include "optim/sl.c"
    break;
case psl_sr:
    FOREACH(sr);
#include "optim/sr.c"
    break;
case psl_or:
    FOREACH(or);
#include "optim/or.c"
    break;
case psl_nor:
    FOREACH(nor);
#include "optim/nor.c"
    break;
case psl_xor:
    FOREACH(xor);
#include "optim/xor.c"
    break;
case psl_nxor:
    FOREACH(nxor);
#include "optim/nxor.c"
    break;
case psl_and:
    FOREACH(and);
#include "optim/and.c"
    break;
case psl_nand:
    FOREACH(nand);
#include "optim/nand.c"
    break;
case psl_byte:
    FOREACH(byte);
#include "optim/byte.c"
    break;
case psl_float:
    FOREACH(float);
#include "optim/float.c"
    break;
case psl_fint:
    FOREACH(fint);
#include "optim/fint.c"
    break;
case psl_fmul:
    FOREACH(fmul);
#include "optim/fmul.c"
    break;
case psl_fdiv:
    FOREACH(fdiv);
#include "optim/fdiv.c"
    break;
case psl_fmod:
    FOREACH(fmod);
#include "optim/fmod.c"
    break;
case psl_fadd:
    FOREACH(fadd);
#include "optim/fadd.c"
    break;
case psl_fsub:
    FOREACH(fsub);
#include "optim/fsub.c"
    break;
case psl_flt:
    FOREACH(flt);
#include "optim/flt.c"
    break;
case psl_flte:
    FOREACH(flte);
#include "optim/flte.c"
    break;
case psl_feq:
    FOREACH(feq);
#include "optim/feq.c"
    break;
case psl_fne:
    FOREACH(fne);
#include "optim/fne.c"
    break;
case psl_fgt:
    FOREACH(fgt);
#include "optim/fgt.c"
    break;
case psl_fgte:
    FOREACH(fgte);
#include "optim/fgte.c"
    break;
case psl_version:
    FOREACH(version);
#include "optim/version.c"
    break;
case psl_dsrcfile:
    FOREACH(dsrcfile);
#include "optim/dsrcfile.c"
    break;
case psl_dsrcline:
    FOREACH(dsrcline);
#include "optim/dsrcline.c"
    break;
case psl_dsrccol:
    FOREACH(dsrccol);
#include "optim/dsrccol.c"
    break;
case psl_dinstruction_sequence_start:
    FOREACH(dinstruction_sequence_start);
#include "optim/dinstruction_sequence_start.c"
    break;
case psl_dinstruction_sequence_end:
    FOREACH(dinstruction_sequence_end);
#include "optim/dinstruction_sequence_end.c"
    break;
case psl_print:
    FOREACH(print);
#include "optim/print.c"
    break;
case psl_debug:
    FOREACH(debug);
#include "optim/debug.c"
    break;
case psl_intrinsic:
    FOREACH(intrinsic);
#include "optim/intrinsic.c"
    break;
case psl_trap:
    FOREACH(trap);
#include "optim/trap.c"
    break;
case psl_include:
    FOREACH(include);
#include "optim/include.c"
    break;
case psl_parse:
    FOREACH(parse);
#include "optim/parse.c"
    break;
case psl_gadd:
    FOREACH(gadd);
#include "optim/gadd.c"
    break;
case psl_grem:
    FOREACH(grem);
#include "optim/grem.c"
    break;
case psl_gcommit:
    FOREACH(gcommit);
#include "optim/gcommit.c"
    break;
case psl_marker:
    FOREACH(marker);
#include "optim/marker.c"
    break;
case psl_immediate:
    FOREACH(immediate);
#include "optim/immediate.c"
    break;
case psl_code:
    FOREACH(code);
#include "optim/code.c"
    break;
case psl_raw:
    FOREACH(raw);
#include "optim/raw.c"
    break;
case psl_dlopen:
    FOREACH(dlopen);
#include "optim/dlopen.c"
    break;
case psl_dlclose:
    FOREACH(dlclose);
#include "optim/dlclose.c"
    break;
case psl_dlsym:
    FOREACH(dlsym);
#include "optim/dlsym.c"
    break;
case psl_cget:
    FOREACH(cget);
#include "optim/cget.c"
    break;
case psl_cset:
    FOREACH(cset);
#include "optim/cset.c"
    break;
case psl_cinteger:
    FOREACH(cinteger);
#include "optim/cinteger.c"
    break;
case psl_ctype:
    FOREACH(ctype);
#include "optim/ctype.c"
    break;
case psl_cstruct:
    FOREACH(cstruct);
#include "optim/cstruct.c"
    break;
case psl_csizeof:
    FOREACH(csizeof);
#include "optim/csizeof.c"
    break;
case psl_csget:
    FOREACH(csget);
#include "optim/csget.c"
    break;
case psl_csset:
    FOREACH(csset);
#include "optim/csset.c"
    break;
case psl_prepcif:
    FOREACH(prepcif);
#include "optim/prepcif.c"
    break;
case psl_ccall:
    FOREACH(ccall);
#include "optim/ccall.c"
    break;
