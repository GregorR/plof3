#include <stdio.h>
#include <string.h>

#include "plof.h"
#include "psl.h"

/* Macro for getting the address of a label (currently only supports GCC) */
#define addressof(label) &&label

/* Internal functions for handling PSL bignums */
size_t pslBignumLength(size_t val);
void pslIntToBignum(unsigned char *buf, size_t val, size_t len);

/* The main PSL interpreter */
__attribute__((__noinline__))
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

    struct PlofRawData *rd;
    struct PlofArrayData *ad;

    /* The PSL in various forms */
    size_t psllen;
    unsigned char *psl;
    volatile void **cpsl;
    volatile void **pc;
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
        stacktop++; \
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

    /* Basic type-checks */
#define ISRAW(obj) ((obj)->data && \
                    (obj)->data->type == PLOF_DATA_RAW)
#define ISARRAY(obj) ((obj)->data && \
                      (obj)->data->type == PLOF_DATA_ARRAY)
#define RAW(obj) ((struct PlofRawData *) (obj)->data)
#define ARRAY(obj) ((struct PlofArrayData *) (obj)->data)

#define ISINT(obj) (ISRAW(obj) && RAW(obj)->length == sizeof(ptrdiff_t))
#define ASINT(obj) (*((ptrdiff_t *) RAW(obj)->data))

    /* Type coercions */
#define PUSHINT(val) \
    { \
        rd = GC_NEW_Z(struct PlofRawData); \
        rd->length = sizeof(ptrdiff_t); \
        rd->data = (unsigned char *) GC_NEW_Z(ptrdiff_t); \
        *((ptrdiff_t *) rd->data) = (val); \
        \
        a = GC_NEW_Z(struct PlofObject); \
        a->parent = context; \
        a->data = (struct PlofData *) rd; \
        STACK_PUSH(a); \
    }

    /* "Functions" for integer ops */
#define INTBINOP(op) \
    BINARY; \
    { \
        ptrdiff_t res = 0; \
        \
        if (ISINT(a) && ISINT(b)) { \
            ptrdiff_t ia, ib; \
            \
            /* get the values */ \
            ia = ASINT(a); \
            ib = ASINT(b); \
            res = ia op ib; \
        } \
        \
        PUSHINT(res); \
    }
#define INTCMP(op) \
    QUINARY; \
    { \
        if (ISINT(b) && ISINT(c) && ISRAW(d) && ISRAW(e)) { \
            ptrdiff_t ia, ib; \
            struct PlofReturn ret; \
            \
            /* get the values */ \
            ia = ASINT(b); \
            ib = ASINT(c); \
            \
            /* check them */ \
            if (ia op ib) { \
                ret = interpretPSL(d->parent, a, RAW(d), 0, NULL, 0); \
            } else { \
                ret = interpretPSL(e->parent, a, RAW(e), 0, NULL, 0); \
            } \
            \
            /* maybe rethrow */ \
            if (ret.isThrown) { \
                return ret; \
            } \
            \
            STACK_PUSH(ret.ret); \
        } else { \
            STACK_PUSH(plofNull); \
        } \
    }

    /* Get out the PSL */
    if (pslraw) {
        psllen = pslraw->length;
        psl = pslraw->data;
        cpsl = (volatile void **) pslraw->idata;
    } else {
        psllen = pslaltlen;
        psl = pslalt;
    }

    /* Make sure it's compiled */
    if (!cpsl) {
        volatile int psli, cpsli;

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
                raw->data = (unsigned char *) GC_MALLOC(raw->length);
                memcpy(raw->data, psl + psli, raw->length);
                psli += raw->length - 1;

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

#ifdef DEBUG
#define DEBUG_CMD(cmd) fprintf(stderr, "DEBUG: " cmd "\n")
#else
#define DEBUG_CMD(cmd)
#endif

interp_psl_nop:
    DEBUG_CMD("nop");
    /* do nothing */
    STEP;

    /* General "function" for PSL push* commands */
#define PSL_PUSH(n) \
    DEBUG_CMD("push"); \
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

interp_psl_pop:
    DEBUG_CMD("pop");
    UNARY;
    STEP;

interp_psl_this:
    DEBUG_CMD("this");
    STACK_PUSH(context);
    STEP;

interp_psl_null:
    DEBUG_CMD("null");
    STACK_PUSH(plofNull);
    STEP;

interp_psl_global:
    DEBUG_CMD("global");
    STACK_PUSH(plofGlobal);
    STEP;

interp_psl_new:
    DEBUG_CMD("new");
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    STACK_PUSH(a);
    STEP;

interp_psl_combine:
    DEBUG_CMD("combine");
    BINARY;

    /* start making the new object */
    c = GC_NEW_Z(struct PlofObject);
    c->parent = b->parent;

    /* duplicate the left object */
    plofObjCopy(c, a->hashTable);

    /* then the right */
    plofObjCopy(c, b->hashTable);

    /* now get any data */
    if (ISRAW(a)) {
        if (ISRAW(b)) {
            struct PlofRawData *ra, *rb;
            ra = RAW(a);
            rb = RAW(b);

            rd = GC_NEW_Z(struct PlofRawData);
            rd->type = PLOF_DATA_RAW;

            rd->length = ra->length + rb->length;
            rd->data = (unsigned char *) GC_MALLOC(rd->length);

            /* copy in both */
            memcpy(rd->data, ra->data, ra->length);
            memcpy(rd->data + ra->length, rb->data, rb->length);

            c->data = (struct PlofData *) rd;

        } else {
            /* just the left */
            c->data = a->data;

        }

    } else if (ISARRAY(a)) {
        if (ISRAW(b)) {
            /* just the right */
            c->data = b->data;

        } else if (ISARRAY(b)) {
            /* combine the arrays */
            struct PlofArrayData *aa, *ab;
            aa = ARRAY(a);
            ab = ARRAY(b);

            ad = GC_NEW_Z(struct PlofArrayData);
            ad->type = PLOF_DATA_ARRAY;

            ad->length = aa->length + ab->length;
            ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));

            /* copy in both */
            memcpy(ad->data, aa->data, aa->length * sizeof(struct PlofObject *));
            memcpy(ad->data + aa->length, ab->data, ab->length * sizeof(struct PlofObject *));

            c->data = (struct PlofData *) ad;

        } else {
            /* duplicate the left array */
            ad = GC_NEW_Z(struct PlofArrayData);
            memcpy(ad, ARRAY(a), sizeof(struct PlofArrayData));
            ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));
            memcpy(ad->data, ARRAY(a)->data, ad->length * sizeof(struct PlofObject *));

        }

    } else {
        if (ISRAW(b)) {
            c->data = b->data;

        } else if (ISARRAY(b)) {
            /* duplicate the right array */
            ad = GC_NEW_Z(struct PlofArrayData);
            memcpy(ad, ARRAY(b), sizeof(struct PlofArrayData));
            ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));
            memcpy(ad->data, ARRAY(b)->data, ad->length * sizeof(struct PlofObject *));

        }

    }

    STACK_PUSH(c);

    STEP;

interp_psl_member:
    DEBUG_CMD("member");
    BINARY;
    if (ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        rd = RAW(b);
        name = rd->data;
        namehash = plofHash(name);

        PLOF_READ(a, a, rd->length, name, namehash);
        STACK_PUSH(a);
    } else {
        STACK_PUSH(plofNull);
    }
    STEP;

interp_psl_memberset:
    DEBUG_CMD("memberset");
    TRINARY;
    if (ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        rd = RAW(b);
        name = rd->data;
        namehash = plofHash(name);

        PLOF_WRITE(a, rd->length, name, namehash, c);
    }
    STEP;

interp_psl_parent:
    DEBUG_CMD("parent");
    UNARY;
    STACK_PUSH(a->parent);
    STEP;

interp_psl_parentset:
    DEBUG_CMD("parentset");
    BINARY;
    a->parent = b;
    STEP;

interp_psl_call:
    DEBUG_CMD("call");
    BINARY;
    if (ISRAW(b)) {
        struct PlofReturn ret;

        /* make the context */
        c = GC_NEW_Z(struct PlofObject);
        c->parent = b->parent;

        ret = interpretPSL(c, a, RAW(b), 0, NULL, 0);

        /* check the return */
        if (ret.isThrown) {
            return ret;
        }

        STACK_PUSH(ret.ret);
    } else {
        /* quay? (ERROR) */
        STACK_PUSH(plofNull);
    }
    STEP;

interp_psl_return: UNIMPL("psl_return");

interp_psl_throw:
    DEBUG_CMD("throw");
    UNARY;
    return (struct PlofReturn) {a, 1};

interp_psl_catch:
    DEBUG_CMD("catch");
    TRINARY;
    if (ISRAW(b)) {
        struct PlofReturn ret = interpretPSL(b->parent, a, RAW(b), 0, NULL, 0);

        /* perhaps catch */
        if (ret.isThrown) {
            if (ISRAW(c)) {
                ret = interpretPSL(c->parent, ret.ret, RAW(c), 0, NULL, 0);
                if (ret.isThrown) {
                    return ret;
                }
            } else {
                ret.ret = plofNull;
            }
        }

        /* then push the result */
        STACK_PUSH(ret.ret);
    } else {
        STACK_PUSH(plofNull);
    }
    STEP;

interp_psl_cmp:
    DEBUG_CMD("cmp");
    QUINARY;
    if (b == c) {
        if (ISRAW(d)) {
            struct PlofReturn ret = interpretPSL(d->parent, a, RAW(d), 0, NULL, 0);

            /* rethrow */
            if (ret.isThrown) {
                return ret;
            }
            STACK_PUSH(ret.ret);
        } else {
            STACK_PUSH(plofNull);
        }
    } else {
        if (ISRAW(e)) {
            struct PlofReturn ret = interpretPSL(e->parent, a, RAW(e), 0, NULL, 0);

            /* rethrow */
            if (ret.isThrown) {
                return ret;
            }
            STACK_PUSH(ret.ret);
        } else {
            STACK_PUSH(plofNull);
        }
    }
    STEP;

interp_psl_concat:
    DEBUG_CMD("concat");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        struct PlofRawData *ra, *rb;

        ra = RAW(a);
        rb = RAW(b);

        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = ra->length + rb->length;
        rd->data = (unsigned char *) GC_MALLOC(rd->length);
        memcpy(rd->data, ra->data, ra->length);
        memcpy(rd->data + ra->length, rb->data, rb->length);

        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;

        STACK_PUSH(a);

    } else {
        STACK_PUSH(plofNull);

    }
    STEP;

interp_psl_wrap:
    DEBUG_CMD("wrap");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        size_t bignumsz;
        struct PlofRawData *ra, *rb;

        ra = RAW(a);
        rb = RAW(b);

        /* create the new rd */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;

        /* figure out how much space is needed */
        bignumsz = pslBignumLength(ra->length);
        rd->length = 1 + bignumsz + ra->length;
        rd->data = (unsigned char *) GC_MALLOC(rd->length);

        /* copy in the instruction */
        if (rb->length >= 1) {
            rd->data[0] = rb->data[0];
        }

        /* and the bignum */
        pslIntToBignum(rd->data + 1, ra->length, bignumsz);

        /* and the data */
        memcpy(rd->data + 1 + bignumsz, ra->data, ra->length);

        /* then push it */
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;
        STACK_PUSH(a);
    }
    STEP;

interp_psl_resolve:
    DEBUG_CMD("resolve");
    BINARY;
    {
        int i;
        size_t *hashes;

        /* get an array of names regardless */
        if (ISARRAY(b)) {
            ad = ARRAY(b);
        } else {
            ad = GC_NEW_Z(struct PlofArrayData);
            ad->type = PLOF_DATA_ARRAY;

            if (ISRAW(b)) {
                ad->length = 1;
                ad->data = (struct PlofObject **) GC_MALLOC(sizeof(struct PlofObject *));
                ad->data[0] = b;
            }
        }

        /* hash them all */
        hashes = (size_t *) GC_MALLOC(ad->length * sizeof(size_t));
        for (i = 0; i < ad->length; i++) {
            hashes[i] = plofHash(RAW(ad->data[i])->data);
        }

        /* now try to find a match */
        while (a && a != plofNull) {
            for (i = 0; i < ad->length; i++) {
                rd = RAW(ad->data[i]);
                PLOF_READ(b, a, rd->length, rd->data, hashes[i]);
                if (b != plofNull) {
                    /* done */
                    STACK_PUSH(a);
                    STACK_PUSH(ad->data[i]);
                    STEP;
                }
            }

            a = a->parent;
        }

        /* didn't find one */
        STACK_PUSH(plofNull);
        STACK_PUSH(plofNull);
    }
    STEP;

interp_psl_loop:
    DEBUG_CMD("loop");
    UNARY;

    /* fix the stack */
    stacktop = 1;
    stack[0] = a;

    /* then loop */
    LOOP;

interp_psl_replace: UNIMPL("psl_replace");

interp_psl_array:
    DEBUG_CMD("array");
    UNARY;
    {
        size_t length = 0;
        ptrdiff_t stacki, arri;
        length = stacki = arri = 0;

        if (ISINT(a)) {
            length = ASINT(a);
        }

        /* now make an array of the appropriate size */
        ad = GC_NEW_Z(struct PlofArrayData);
        ad->type = PLOF_DATA_ARRAY;
        ad->length = length;
        ad->data = GC_MALLOC(length * sizeof(struct PlofObject *));

        /* copy in the stack */
        if (length > 0) {
            for (stacki = stacktop - 1,
                 arri = length - 1;
                 stacki >= 0,
                 arri >= 0;
                 stacki--, arri--) {
                ad->data[arri] = stack[stacki];
            }
            stacktop = stacki + 1;
        }

        /* we may have exhausted the stack */
        for (; arri >= 0; arri--) {
            ad->data[arri] = plofNull;
        }

        /* then just put it in an object */
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) ad;
        STACK_PUSH(a);
    }
    STEP;

interp_psl_aconcat:
    DEBUG_CMD("aconcat");
    BINARY;
    {
        struct PlofArrayData *aa, *ba, *ra;
        size_t al, bl, rl, ai;
        aa = ba = ra = NULL;
        al = bl = rl = 0;

        /* get the arrays ... */
        if (ISARRAY(a)) {
            aa = ARRAY(a);
        }
        if (ISARRAY(b)) {
            ba = ARRAY(b);
        }

        /* get the lengths ... */
        if (aa) {
            al = aa->length;
        }
        if (ba) {
            bl = ba->length;
        }
        rl = al + bl;

        /* make the new array object */
        ra = GC_NEW_Z(struct PlofArrayData);
        ra->type = PLOF_DATA_ARRAY;
        ra->length = rl;
        ra->data = (struct PlofObject **) GC_MALLOC(rl * sizeof(struct PlofObject *));

        /* then copy */
        for (ai = 0; ai < al; ai++) {
            ra->data[ai] = aa->data[ai];
        }
        for (ai = 0; ai < bl; ai++) {
            ra->data[al+ai] = ba->data[ai];
        }

        /* now put it in an object */
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) ra;
        STACK_PUSH(a);
    }
    STEP;

interp_psl_length:
    DEBUG_CMD("length");
    UNARY;
    if (ISARRAY(a)) {
        PUSHINT(ARRAY(a)->length);
    } else {
        PUSHINT(0);
    }
    STEP;

interp_psl_lengthset: UNIMPL("psl_lengthset");

interp_psl_index:
    DEBUG_CMD("index");
    BINARY;
    if (ISARRAY(a) && ISINT(b)) {
        ptrdiff_t index = ASINT(b);
        ad = ARRAY(a);

        if (index < 0 || index >= ad->length) {
            STACK_PUSH(plofNull);
        } else {
            STACK_PUSH(ad->data[index]);
        }
    } else {
        STACK_PUSH(plofNull);
    }
    STEP;

interp_psl_indexset:
    DEBUG_CMD("indexset");
    TRINARY;
    if (ISARRAY(a) && ISINT(b)) {
        ptrdiff_t index = ASINT(b);
        ad = ARRAY(a);

        /* make sure it's long enough */
        if (index >= ad->length) {
            size_t i = ad->length;
            ad->length = index + 1;
            ad->data = GC_REALLOC(ad->data, ad->length * sizeof(struct PlofObject *));
            for (; i < ad->length; i++) {
                ad->data[i] = plofNull;
            }
        }

        /* then set it */
        if (index >= 0) {
            ad->data[index] = c;
        }
    }
    STEP;

interp_psl_members:
    DEBUG_CMD("members");
    UNARY;
    ad = plofMembers(a);
    b = GC_NEW_Z(struct PlofObject);
    b->parent = context;
    b->data = (struct PlofData *) ad;
    STACK_PUSH(b);
    STEP;

interp_psl_integer:
    DEBUG_CMD("integer");
    UNARY;
    {
        ptrdiff_t val = 0;

        /* get the value */
        if (ISRAW(a)) {
            rd = (struct PlofRawData *) a->data;

            switch (rd->length) {
                case 1:
                    val = rd->data[0];
                    break;

                case 2:
                    val = ((ptrdiff_t) rd->data[0] << 8) |
                          ((ptrdiff_t) rd->data[1]);
                    break;

                case 4:
                    val = ((ptrdiff_t) rd->data[0] << 24) |
                          ((ptrdiff_t) rd->data[1] << 16) |
                          ((ptrdiff_t) rd->data[2] << 8) |
                          ((ptrdiff_t) rd->data[3]);
                    break;

                case 8:
                    val = ((ptrdiff_t) rd->data[0] << 56) |
                          ((ptrdiff_t) rd->data[1] << 48) |
                          ((ptrdiff_t) rd->data[2] << 40) |
                          ((ptrdiff_t) rd->data[3] << 32) |
                          ((ptrdiff_t) rd->data[4] << 24) |
                          ((ptrdiff_t) rd->data[5] << 16) |
                          ((ptrdiff_t) rd->data[6] << 8) |
                          ((ptrdiff_t) rd->data[7]);
                    break;
            }
        }

        PUSHINT(val);
    }
    STEP;

interp_psl_intwidth: UNIMPL("psl_intwidth");

interp_psl_mul:
    DEBUG_CMD("mul");
    INTBINOP(*);
    STEP;

interp_psl_div:
    DEBUG_CMD("div");
    INTBINOP(/);
    STEP;

interp_psl_mod:
    DEBUG_CMD("mod");
    INTBINOP(%);
    STEP;

interp_psl_add:
    DEBUG_CMD("add");
    INTBINOP(+);
    STEP;

interp_psl_sub:
    DEBUG_CMD("sub");
    INTBINOP(-);
    STEP;

interp_psl_lt:
    DEBUG_CMD("lt");
    INTCMP(<);
    STEP;

interp_psl_lte:
    DEBUG_CMD("lte");
    INTCMP(<=);
    STEP;

interp_psl_eq:
    DEBUG_CMD("eq");
    INTCMP(==);
    STEP;

interp_psl_ne: UNIMPL("psl_ne");
interp_psl_gt: UNIMPL("psl_gt");
interp_psl_gte: UNIMPL("psl_gte");
interp_psl_sl: UNIMPL("psl_sl");
interp_psl_sr: UNIMPL("psl_sr");
interp_psl_or: UNIMPL("psl_or");

interp_psl_nor:
    DEBUG_CMD("nor");
    /* or it, then not it */
    INTBINOP(|);
    ASINT(stack[stacktop]) = ~ASINT(stack[stacktop]);
    STEP;

interp_psl_xor:
    DEBUG_CMD("xor");
    INTBINOP(^);
    STEP;

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

interp_psl_print:
    DEBUG_CMD("print");
    /* do our best to print this (debugging) */
    UNARY;
    if (ISRAW(a)) {
        printf("%.*s\n", RAW(a)->length, RAW(a)->data);

        if (RAW(a)->length == sizeof(ptrdiff_t)) {
            printf("Integer value: %d\n", *((ptrdiff_t *) RAW(a)->data));
        }
    } else {
        printf("%p\n", a);
    }
    STEP;

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

interp_psl_code:
interp_psl_raw:
    DEBUG_CMD("raw");
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    a->data = (struct PlofData *) pc[1];
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

/* Determine the number of bytes a bignum of a particular number will take */
size_t pslBignumLength(size_t val)
{
    if (val < ((size_t) 1<<7)) {
        return 1;
    } else if (val < ((size_t) 1<<14)) {
        return 2;
    } else if (val < ((size_t) 1<<21)) {
        return 3;
    } else if (val < ((size_t) 1<<28)) {
        return 4;
    } else if (val < ((size_t) 1<<35)) {
        return 5;
    } else if (val < ((size_t) 1<<42)) {
        return 6;
    } else if (val < ((size_t) 1<<49)) {
        return 7;
    } else if (val < ((size_t) 1<<56)) {
        return 8;
    } else if (val < ((size_t) 1<<63)) {
        return 9;
    } else {
        return 10;
    }
}

/* Write a bignum into a buffer */
void pslIntToBignum(unsigned char *buf, size_t val, size_t len)
{
    buf[--len] = val & 0x7F;
    val >>= 7;

    for (len--; len != (size_t) -1; len--) {
        buf[len] = (val & 0x7F) | 0x80;
        val >>= 7;
    }
}

/* Hash function */
size_t plofHash(unsigned char *str)
{
    /* this is the hash function used in sdbm */
    size_t hash = 0;
    int c;

    while (c = *str++)
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}

/* Copy the content of one object into another */
void plofObjCopy(struct PlofObject *to, struct PlofOHashTable *from)
{
    if (from == NULL) return;

    /*if ((ptrdiff_t) from->value & 1) {
        PLOF_WRITE(to, from->namelen, from->name, from->hashedName, from->itable[(ptrdiff_t) from->value>>1]);
    } else {*/
    /* FIXME */
        PLOF_WRITE(to, from->namelen, from->name, from->hashedName, from->value);
    /*}*/
    plofObjCopy(to, from->left);
    plofObjCopy(to, from->right);
}


struct PlofObjects {
    size_t length;
    struct PlofObject **data;
};

/* Internal function used by plofMembers */
struct PlofObjects plofMembersSub(struct PlofOHashTable *of)
{
    struct PlofObjects left, right, ret;
    struct PlofObject *obj;
    struct PlofRawData *rd;

    if (of == NULL) return (struct PlofObjects) { 0, NULL };

    /* get the left and right members */
    left = plofMembersSub(of->left);
    right = plofMembersSub(of->right);

    /* prepare ours */
    ret.length = left.length + right.length + 1;
    ret.data = (struct PlofObject **) GC_MALLOC(ret.length * sizeof(struct PlofObject *));

    /* and the object */
    rd = GC_NEW_Z(struct PlofRawData);
    rd->type = PLOF_DATA_RAW;
    rd->length = of->namelen;
    rd->data = of->name;
    obj = GC_NEW_Z(struct PlofObject);
    obj->parent = plofNull; /* FIXME */
    obj->data = (struct PlofData *) rd;

    /* then copy */
    memcpy(ret.data, left.data, left.length * sizeof(struct PlofObject *));
    ret.data[left.length] = obj;
    memcpy(ret.data + left.length + 1, right.data, right.length * sizeof(struct PlofObject *));

    return ret;
}

/* Make an array of the list of members of an object */
struct PlofArrayData *plofMembers(struct PlofObject *of)
{
    struct PlofArrayData *ad;

    /* get out the members */
    struct PlofObjects objs = plofMembersSub(of->hashTable);

    /* then make it into a PlofArrayData and an object */
    ad = GC_NEW_Z(struct PlofArrayData);
    ad->type = PLOF_DATA_ARRAY;
    ad->length = objs.length;
    ad->data = objs.data;

    return ad;
}

/* Null and global */
struct PlofObject *plofNull = NULL;
struct PlofObject *plofGlobal = NULL;
