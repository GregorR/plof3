#include <stdio.h>
#include <string.h>

#include "plof.h"
#include "psl.h"

/* Macro for getting the address of a label (currently only supports GCC) */
#define addressof(label) &&label

/* Internal function for handling PSL bignums */
int pslBignumToInt(unsigned char *bignum, ptrdiff_t *into);

/* The main PSL interpreter */
struct PlofReturn interpretPSL(
        struct PlofObject *context,
        struct PlofObject *arg,
        struct PlofRawData *pslraw,
        size_t pslaltlen,
        unsigned char *pslalt,
        int immediate)
{
    /* The stack */
    size_t stacklen, stacktop;
    struct PlofObject **stack;

    /* Slots for n-ary ops */
    struct PlofObject *a, *b, *c, *d, *e;

    /* The PSL in various forms */
    size_t psllen;
    unsigned char *psl;
    void **cpsl;
    void **pc;
    /* Compiled PSL is an array of pointers. Every pointer which is 0 mod 2 is
     * the op to run, the next pointer is an argument as a PlofRawData (if
     * applicable) */

    /* Start the stack at size 8 */
    stack = GC_MALLOC(8 * sizeof(struct PlofObject *));
    stacklen = 8;
    stack[0] = arg;
    stacktop = 1;

    /* "Function" for pushing to the stack */
#define STACK_PUSH(val) \
    { \
        if (stacktop == stacklen) { \
            stacklen *= 2; \
            stack = GC_REALLOC(stack, stacklen * sizeof(struct PlofObject *)); \
        } \
        stack[stacktop] = (val); \
    }
#define STACK_POP(into) \
    { \
        if (stacktop == 0) { \
            into = plofNull; \
        } else { \
            into = stack[--stacktop]; \
        } \
    }

    /* Standards for n-ary ops */
#define UNARY STACK_POP(a)
#define BINARY STACK_POP(b) STACK_POP(a)
#define TRINARY STACK_POP(c) STACK_POP(b) STACK_POP(a)
#define QUATERNARY STACK_POP(d) STACK_POP(c) STACK_POP(b) STACK_POP(a)
#define QUINARY STACK_POP(e) STACK_POP(d) STACK_POP(c) STACK_POP(b) STACK_POP(a)

    /* Get out the PSL */
    if (pslraw) {
        psllen = pslraw->length;
        psl = pslraw->data;
        cpsl = (void **) pslraw->idata;
    } else {
        psllen = pslaltlen;
        psl = pslalt;
    }

    /* Make sure it's compiled */
    if (!cpsl) {
        int psli, cpsli;

        /* start with 8 slots */
        size_t cpsllen = 8;
        cpsl = GC_MALLOC(cpsllen * sizeof(void*));

        /* now go through the PSL and translate it into compiled PSL */
        for (psli = 0, cpsli = 0;
             psli < psllen;
             psli++, cpsli += 2) {
            unsigned char cmd = psl[psli];
            struct PlofRawData *raw;

            /* make sure cpsl is big enough */
            if (cpsli > cpsllen - 2) {
                cpsllen *= 2;
                cpsl = GC_REALLOC(cpsl, cpsllen * sizeof(void*));
            }

            /* maybe it has raw data */
            if (cmd >= psl_immediate) {
                raw = GC_NEW_Z(struct PlofRawData);
                raw->type = PLOF_DATA_RAW;

                psli++;
                psli += pslBignumToInt(psl + psli, &raw->length);
                raw->data = psl + psli;
                psli += raw->length;

                cpsl[cpsli + 1] = raw;
            } else {
                cpsl[cpsli + 1] = NULL;
            }

            /* either get only immediates, or not */
            cpsl[cpsli] = addressof(interp_psl_nop);
            if (immediate) {
                if (cmd == psl_immediate) {
                    cpsl[cpsli] = addressof(interp_psl_immediate);
                }

            } else {
                switch (cmd) {
                    case psl_push0:
                        cpsl[cpsli] = addressof(interp_psl_push0);
                        break;
                    case psl_push1:
                        cpsl[cpsli] = addressof(interp_psl_push1);
                        break;
                    case psl_push2:
                        cpsl[cpsli] = addressof(interp_psl_push2);
                        break;
                    case psl_push3:
                        cpsl[cpsli] = addressof(interp_psl_push3);
                        break;
                    case psl_push4:
                        cpsl[cpsli] = addressof(interp_psl_push4);
                        break;
                    case psl_push5:
                        cpsl[cpsli] = addressof(interp_psl_push5);
                        break;
                    case psl_push6:
                        cpsl[cpsli] = addressof(interp_psl_push6);
                        break;
                    case psl_push7:
                        cpsl[cpsli] = addressof(interp_psl_push7);
                        break;
                    case psl_pop:
                        cpsl[cpsli] = addressof(interp_psl_pop);
                        break;
                    case psl_this:
                        cpsl[cpsli] = addressof(interp_psl_this);
                        break;
                    case psl_null:
                        cpsl[cpsli] = addressof(interp_psl_null);
                        break;
                    case psl_global:
                        cpsl[cpsli] = addressof(interp_psl_global);
                        break;
                    case psl_new:
                        cpsl[cpsli] = addressof(interp_psl_new);
                        break;
                    case psl_combine:
                        cpsl[cpsli] = addressof(interp_psl_combine);
                        break;
                    case psl_member:
                        cpsl[cpsli] = addressof(interp_psl_member);
                        break;
                    case psl_memberset:
                        cpsl[cpsli] = addressof(interp_psl_memberset);
                        break;
                    case psl_parent:
                        cpsl[cpsli] = addressof(interp_psl_parent);
                        break;
                    case psl_parentset:
                        cpsl[cpsli] = addressof(interp_psl_parentset);
                        break;
                    case psl_call:
                        cpsl[cpsli] = addressof(interp_psl_call);
                        break;
                    case psl_return:
                        cpsl[cpsli] = addressof(interp_psl_return);
                        break;
                    case psl_throw:
                        cpsl[cpsli] = addressof(interp_psl_throw);
                        break;
                    case psl_catch:
                        cpsl[cpsli] = addressof(interp_psl_catch);
                        break;
                    case psl_cmp:
                        cpsl[cpsli] = addressof(interp_psl_cmp);
                        break;
                    case psl_concat:
                        cpsl[cpsli] = addressof(interp_psl_concat);
                        break;
                    case psl_wrap:
                        cpsl[cpsli] = addressof(interp_psl_wrap);
                        break;
                    case psl_resolve:
                        cpsl[cpsli] = addressof(interp_psl_resolve);
                        break;
                    case psl_loop:
                        cpsl[cpsli] = addressof(interp_psl_loop);
                        break;
                    case psl_replace:
                        cpsl[cpsli] = addressof(interp_psl_replace);
                        break;
                    case psl_array:
                        cpsl[cpsli] = addressof(interp_psl_array);
                        break;
                    case psl_aconcat:
                        cpsl[cpsli] = addressof(interp_psl_aconcat);
                        break;
                    case psl_length:
                        cpsl[cpsli] = addressof(interp_psl_length);
                        break;
                    case psl_lengthset:
                        cpsl[cpsli] = addressof(interp_psl_lengthset);
                        break;
                    case psl_index:
                        cpsl[cpsli] = addressof(interp_psl_index);
                        break;
                    case psl_indexset:
                        cpsl[cpsli] = addressof(interp_psl_indexset);
                        break;
                    case psl_members:
                        cpsl[cpsli] = addressof(interp_psl_members);
                        break;
                    case psl_integer:
                        cpsl[cpsli] = addressof(interp_psl_integer);
                        break;
                    case psl_intwidth:
                        cpsl[cpsli] = addressof(interp_psl_intwidth);
                        break;
                    case psl_mul:
                        cpsl[cpsli] = addressof(interp_psl_mul);
                        break;
                    case psl_div:
                        cpsl[cpsli] = addressof(interp_psl_div);
                        break;
                    case psl_mod:
                        cpsl[cpsli] = addressof(interp_psl_mod);
                        break;
                    case psl_add:
                        cpsl[cpsli] = addressof(interp_psl_add);
                        break;
                    case psl_sub:
                        cpsl[cpsli] = addressof(interp_psl_sub);
                        break;
                    case psl_lt:
                        cpsl[cpsli] = addressof(interp_psl_lt);
                        break;
                    case psl_lte:
                        cpsl[cpsli] = addressof(interp_psl_lte);
                        break;
                    case psl_eq:
                        cpsl[cpsli] = addressof(interp_psl_eq);
                        break;
                    case psl_ne:
                        cpsl[cpsli] = addressof(interp_psl_ne);
                        break;
                    case psl_gt:
                        cpsl[cpsli] = addressof(interp_psl_gt);
                        break;
                    case psl_gte:
                        cpsl[cpsli] = addressof(interp_psl_gte);
                        break;
                    case psl_sl:
                        cpsl[cpsli] = addressof(interp_psl_sl);
                        break;
                    case psl_sr:
                        cpsl[cpsli] = addressof(interp_psl_sr);
                        break;
                    case psl_or:
                        cpsl[cpsli] = addressof(interp_psl_or);
                        break;
                    case psl_nor:
                        cpsl[cpsli] = addressof(interp_psl_nor);
                        break;
                    case psl_xor:
                        cpsl[cpsli] = addressof(interp_psl_xor);
                        break;
                    case psl_nxor:
                        cpsl[cpsli] = addressof(interp_psl_nxor);
                        break;
                    case psl_and:
                        cpsl[cpsli] = addressof(interp_psl_and);
                        break;
                    case psl_nand:
                        cpsl[cpsli] = addressof(interp_psl_nand);
                        break;
                    case psl_byte:
                        cpsl[cpsli] = addressof(interp_psl_byte);
                        break;
                    case psl_float:
                        cpsl[cpsli] = addressof(interp_psl_float);
                        break;
                    case psl_fint:
                        cpsl[cpsli] = addressof(interp_psl_fint);
                        break;
                    case psl_fmul:
                        cpsl[cpsli] = addressof(interp_psl_fmul);
                        break;
                    case psl_fdiv:
                        cpsl[cpsli] = addressof(interp_psl_fdiv);
                        break;
                    case psl_fmod:
                        cpsl[cpsli] = addressof(interp_psl_fmod);
                        break;
                    case psl_fadd:
                        cpsl[cpsli] = addressof(interp_psl_fadd);
                        break;
                    case psl_fsub:
                        cpsl[cpsli] = addressof(interp_psl_fsub);
                        break;
                    case psl_flt:
                        cpsl[cpsli] = addressof(interp_psl_flt);
                        break;
                    case psl_flte:
                        cpsl[cpsli] = addressof(interp_psl_flte);
                        break;
                    case psl_feq:
                        cpsl[cpsli] = addressof(interp_psl_feq);
                        break;
                    case psl_fne:
                        cpsl[cpsli] = addressof(interp_psl_fne);
                        break;
                    case psl_fgt:
                        cpsl[cpsli] = addressof(interp_psl_fgt);
                        break;
                    case psl_fgte:
                        cpsl[cpsli] = addressof(interp_psl_fgte);
                        break;
                    case psl_version:
                        cpsl[cpsli] = addressof(interp_psl_version);
                        break;
                    case psl_dsrcfile:
                        cpsl[cpsli] = addressof(interp_psl_dsrcfile);
                        break;
                    case psl_dsrcline:
                        cpsl[cpsli] = addressof(interp_psl_dsrcline);
                        break;
                    case psl_dsrccol:
                        cpsl[cpsli] = addressof(interp_psl_dsrccol);
                        break;
                    case psl_print:
                        cpsl[cpsli] = addressof(interp_psl_print);
                        break;
                    case psl_debug:
                        cpsl[cpsli] = addressof(interp_psl_debug);
                        break;
                    case psl_include:
                        cpsl[cpsli] = addressof(interp_psl_include);
                        break;
                    case psl_parse:
                        cpsl[cpsli] = addressof(interp_psl_parse);
                        break;
                    case psl_gadd:
                        cpsl[cpsli] = addressof(interp_psl_gadd);
                        break;
                    case psl_grem:
                        cpsl[cpsli] = addressof(interp_psl_grem);
                        break;
                    case psl_gaddstop:
                        cpsl[cpsli] = addressof(interp_psl_gaddstop);
                        break;
                    case psl_gremstop:
                        cpsl[cpsli] = addressof(interp_psl_gremstop);
                        break;
                    case psl_gaddgroup:
                        cpsl[cpsli] = addressof(interp_psl_gaddgroup);
                        break;
                    case psl_gremgroup:
                        cpsl[cpsli] = addressof(interp_psl_gremgroup);
                        break;
                    case psl_gcommit:
                        cpsl[cpsli] = addressof(interp_psl_gcommit);
                        break;
                    case psl_marker:
                        cpsl[cpsli] = addressof(interp_psl_marker);
                        break;
                    case psl_code:
                        cpsl[cpsli] = addressof(interp_psl_code);
                        break;
                    case psl_raw:
                        cpsl[cpsli] = addressof(interp_psl_raw);
                        break;
                    case psl_dlopen:
                        cpsl[cpsli] = addressof(interp_psl_dlopen);
                        break;
                    case psl_dlclose:
                        cpsl[cpsli] = addressof(interp_psl_dlclose);
                        break;
                    case psl_dlsym:
                        cpsl[cpsli] = addressof(interp_psl_dlsym);
                        break;
                    case psl_cmalloc:
                        cpsl[cpsli] = addressof(interp_psl_cmalloc);
                        break;
                    case psl_cfree:
                        cpsl[cpsli] = addressof(interp_psl_cfree);
                        break;
                    case psl_cget:
                        cpsl[cpsli] = addressof(interp_psl_cget);
                        break;
                    case psl_cset:
                        cpsl[cpsli] = addressof(interp_psl_cset);
                        break;
                    case psl_ctype:
                        cpsl[cpsli] = addressof(interp_psl_ctype);
                        break;
                    case psl_cstruct:
                        cpsl[cpsli] = addressof(interp_psl_cstruct);
                        break;
                    case psl_csizeof:
                        cpsl[cpsli] = addressof(interp_psl_csizeof);
                        break;
                    case psl_csget:
                        cpsl[cpsli] = addressof(interp_psl_csget);
                        break;
                    case psl_csset:
                        cpsl[cpsli] = addressof(interp_psl_csset);
                        break;
                    case psl_prepcif:
                        cpsl[cpsli] = addressof(interp_psl_prepcif);
                        break;
                    case psl_ccall:
                        cpsl[cpsli] = addressof(interp_psl_ccall);
                        break;
                }
            }
        }

        /* now close off the end */
        cpsl[cpsli] = addressof(interp_psl_done);
    }

    /* ACTUAL INTERPRETER BEYOND HERE */
    pc = cpsl;
    goto **pc;

    /* These will need to change for non-GCC */
#define STEP pc += 2; goto **pc
#define LOOP pc = cpsl; goto **pc
#define UNIMPL(cmd) fprintf(stderr, "UNIMPLEMENTED: " cmd "\n"); STEP

interp_psl_nop:
    /* do nothing */
    STEP;

    /* General "function" for PSL push* commands */
#define PSL_PUSH(n) \
    if (stacktop <= n) { \
        STACK_PUSH(plofNull); \
    } else { \
        STACK_PUSH(stack[stacktop - n - 1]); \
    } \
    STEP
interp_psl_push0: PSL_PUSH(0);
interp_psl_push1: PSL_PUSH(1);
interp_psl_push2: PSL_PUSH(2);
interp_psl_push3: PSL_PUSH(3);
interp_psl_push4: PSL_PUSH(4);
interp_psl_push5: PSL_PUSH(5);
interp_psl_push6: PSL_PUSH(6);
interp_psl_push7: PSL_PUSH(7);

interp_psl_pop: UNIMPL("psl_pop");
interp_psl_this: UNIMPL("psl_this");
interp_psl_null: UNIMPL("psl_null");
interp_psl_global: UNIMPL("psl_global");
interp_psl_new: UNIMPL("psl_new");
interp_psl_combine: UNIMPL("psl_combine");
interp_psl_member: UNIMPL("psl_member");
interp_psl_memberset: UNIMPL("psl_memberset");
interp_psl_parent: UNIMPL("psl_parent");
interp_psl_parentset: UNIMPL("psl_parentset");
interp_psl_call: UNIMPL("psl_call");
interp_psl_return: UNIMPL("psl_return");
interp_psl_throw: UNIMPL("psl_throw");
interp_psl_catch: UNIMPL("psl_catch");
interp_psl_cmp: UNIMPL("psl_cmp");
interp_psl_concat: UNIMPL("psl_concat");
interp_psl_wrap: UNIMPL("psl_wrap");
interp_psl_resolve: UNIMPL("psl_resolve");
interp_psl_loop: UNIMPL("psl_loop");
interp_psl_replace: UNIMPL("psl_replace");
interp_psl_array: UNIMPL("psl_array");
interp_psl_aconcat: UNIMPL("psl_aconcat");
interp_psl_length: UNIMPL("psl_length");
interp_psl_lengthset: UNIMPL("psl_lengthset");
interp_psl_index: UNIMPL("psl_index");
interp_psl_indexset: UNIMPL("psl_indexset");
interp_psl_members: UNIMPL("psl_members");
interp_psl_integer: UNIMPL("psl_integer");
interp_psl_intwidth: UNIMPL("psl_intwidth");
interp_psl_mul: UNIMPL("psl_mul");
interp_psl_div: UNIMPL("psl_div");
interp_psl_mod: UNIMPL("psl_mod");
interp_psl_add: UNIMPL("psl_add");
interp_psl_sub: UNIMPL("psl_sub");
interp_psl_lt: UNIMPL("psl_lt");
interp_psl_lte: UNIMPL("psl_lte");
interp_psl_eq: UNIMPL("psl_eq");
interp_psl_ne: UNIMPL("psl_ne");
interp_psl_gt: UNIMPL("psl_gt");
interp_psl_gte: UNIMPL("psl_gte");
interp_psl_sl: UNIMPL("psl_sl");
interp_psl_sr: UNIMPL("psl_sr");
interp_psl_or: UNIMPL("psl_or");
interp_psl_nor: UNIMPL("psl_nor");
interp_psl_xor: UNIMPL("psl_xor");
interp_psl_nxor: UNIMPL("psl_nxor");
interp_psl_and: UNIMPL("psl_and");
interp_psl_nand: UNIMPL("psl_nand");
interp_psl_byte: UNIMPL("psl_byte");
interp_psl_float: UNIMPL("psl_float");
interp_psl_fint: UNIMPL("psl_fint");
interp_psl_fmul: UNIMPL("psl_fmul");
interp_psl_fdiv: UNIMPL("psl_fdiv");
interp_psl_fmod: UNIMPL("psl_fmod");
interp_psl_fadd: UNIMPL("psl_fadd");
interp_psl_fsub: UNIMPL("psl_fsub");
interp_psl_flt: UNIMPL("psl_flt");
interp_psl_flte: UNIMPL("psl_flte");
interp_psl_feq: UNIMPL("psl_feq");
interp_psl_fne: UNIMPL("psl_fne");
interp_psl_fgt: UNIMPL("psl_fgt");
interp_psl_fgte: UNIMPL("psl_fgte");
interp_psl_version: UNIMPL("psl_version");
interp_psl_dsrcfile: UNIMPL("psl_dsrcfile");
interp_psl_dsrcline: UNIMPL("psl_dsrcline");
interp_psl_dsrccol: UNIMPL("psl_dsrccol");
interp_psl_print: UNIMPL("psl_print");
interp_psl_debug: UNIMPL("psl_debug");
interp_psl_include: UNIMPL("psl_include");
interp_psl_parse: UNIMPL("psl_parse");
interp_psl_gadd: UNIMPL("psl_gadd");
interp_psl_grem: UNIMPL("psl_grem");
interp_psl_gaddstop: UNIMPL("psl_gaddstop");
interp_psl_gremstop: UNIMPL("psl_gremstop");
interp_psl_gaddgroup: UNIMPL("psl_gaddgroup");
interp_psl_gremgroup: UNIMPL("psl_gremgroup");
interp_psl_gcommit: UNIMPL("psl_gcommit");
interp_psl_marker: UNIMPL("psl_marker");
interp_psl_immediate: UNIMPL("psl_immediate");
interp_psl_code: UNIMPL("psl_code");

interp_psl_raw:
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    a->data = pc[1];
    STACK_PUSH(a);
    STEP;

interp_psl_dlopen: UNIMPL("psl_dlopen");
interp_psl_dlclose: UNIMPL("psl_dlclose");
interp_psl_dlsym: UNIMPL("psl_dlsym");
interp_psl_cmalloc: UNIMPL("psl_cmalloc");
interp_psl_cfree: UNIMPL("psl_cfree");
interp_psl_cget: UNIMPL("psl_cget");
interp_psl_cset: UNIMPL("psl_cset");
interp_psl_ctype: UNIMPL("psl_ctype");
interp_psl_cstruct: UNIMPL("psl_cstruct");
interp_psl_csizeof: UNIMPL("psl_csizeof");
interp_psl_csget: UNIMPL("psl_csget");
interp_psl_csset: UNIMPL("psl_csset");
interp_psl_prepcif: UNIMPL("psl_prepcif");
interp_psl_ccall: UNIMPL("psl_ccall");

interp_psl_done:
    UNARY;
    return (struct PlofReturn) {a, 0};
}

/* Convert a PSL bignum to an int */
int pslBignumToInt(unsigned char *bignum, ptrdiff_t *into)
{
    ptrdiff_t ret = 0;
    unsigned c = 0;

    for (;; bignum++) {
        c++;
        ret <<= 7;
        ret |= ((*bignum) & 0x7F);
        if ((*bignum) < 128) break;
    }

    *into = ret;

    return c;
}

/* Null and global */
struct PlofObject *plofNull = NULL;
struct PlofObject *plofGlobal = NULL;
