/*
 * Copyright (c) 2007, 2008, 2009 Gregor Richards
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#else
#include "basicconfig.h"
#endif

/* For CNFI */
#ifdef WITH_CNFI
#include <dlfcn.h>
#include <ffi.h>
#endif

/* For predefs */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "jump.h"
#include "plof.h"
#include "psl.h"



/* The maximum number of version strings */
#define VERSION_MAX 100


/* Some versions of 'jump' require an enum */
#ifdef jumpenum
enum jumplabel {
    interp_psl_nop,
#define FOREACH(inst) interp_ ## inst,
#include "psl_inst.h"
#undef FOREACH
    interp_psl_done
};
#endif

/* Internal functions for handling PSL bignums */
size_t pslBignumLength(size_t val);
void pslIntToBignum(unsigned char *buf, size_t val, size_t len);

/* Implementation of 'replace' */
struct PlofRawData *pslReplace(struct PlofRawData *in, struct PlofArrayData *with);

/* The main PSL interpreter */
#ifdef __GNUC__
__attribute__((__noinline__))
#endif
struct PlofReturn interpretPSL(
        struct PlofObject *context,
        struct PlofObject *arg,
        struct PlofObject *pslraw,
        size_t pslaltlen,
        unsigned char *pslalt,
        int generateContext,
        int immediate)
{
    static size_t procedureHash = 0;

    /* Necessary jump variables */
    jumpvars

    /* The stack */
    size_t stacklen, stacktop;
    struct PlofObject **stack;

    /* Slots for n-ary ops */
    struct PlofObject *a, *b, *c, *d, *e;

    struct PlofRawData *rd;
    struct PlofArrayData *ad;

    /* The PSL in various forms */
    size_t psllen;
    unsigned char *psl = NULL;
    volatile void **cpsl = NULL;
    volatile void **pc = NULL;
    /* Compiled PSL is an array of pointers. Every pointer which is 0 mod 2 is
     * the op to run, the next pointer is an argument as a PlofRawData (if
     * applicable) */

    /* The eventual return */
    struct PlofReturn ret;

    /* Perhaps generate the context */
    if (generateContext) {
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        context = a;
    }

    /* Set +procedure */
    if (pslraw) {
        if (procedureHash == 0) {
            procedureHash = plofHash(10, (unsigned char *) "+procedure");
        }
        PLOF_WRITE(context, 10, (unsigned char *) "+procedure", procedureHash, pslraw);
    }

    /* Start the stack at size 8 */
    stack = GC_MALLOC(8 * sizeof(struct PlofObject *));
    stacklen = 8;
    if (arg) {
        stack[0] = arg;
        stacktop = 1;
    } else {
        stacktop = 0;
    }

    /* Get out the PSL */
    if (pslraw) {
        rd = (struct PlofRawData *) pslraw->data;
        psllen = rd->length;
        psl = rd->data;
        cpsl = (volatile void **) rd->idata;
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
            if (cmd >= psl_marker) {
                raw = GC_NEW_Z(struct PlofRawData);
                raw->type = PLOF_DATA_RAW;

                psli++;
                psli += pslBignumToInt(psl + psli, (size_t *) &raw->length);

                /* make sure this doesn't go off the edge */
                if (psli + raw->length > psllen) {
                    fprintf(stderr, "Bad data in PSL!\n");
                    raw->length = psllen - psli;
                }

                /* copy it in */
                raw->data = (unsigned char *) GC_MALLOC_ATOMIC(raw->length);
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
#define FOREACH(inst) \
                    case inst: \
                        cpsl[cpsli] = addressof(interp_ ## inst); \
                        break;
#include "psl_inst.h"
#undef FOREACH

                    default:
                        fprintf(stderr, "Invalid operation: 0x%x\n", cmd);
                }
            }
        }

        /* now close off the end */
        cpsl[cpsli] = addressof(interp_psl_done);

        /* and save it */
        if (pslraw && !immediate) {
            ((struct PlofRawData *) pslraw->data)->idata = cpsl;
        }
    }

    /* ACTUAL INTERPRETER BEYOND HERE */
    pc = cpsl;
    prejump(*pc);


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
#ifdef DEBUG
#define BADTYPE(cmd) fprintf(stderr, "Type error in " cmd "\n")
#else
#define BADTYPE(cmd)
#endif

#if defined(PLOF_BOX_NUMBERS) || defined(PLOF_NUMBERS_IN_OBJECTS)
#define ISOBJ(obj) 1
#elif defined(PLOF_FREE_INTS)
#define ISOBJ(obj) (((size_t) (obj) & 1) == 0)
#endif

#define ISRAW(obj) (ISOBJ(obj) && \
                    (obj)->data && \
                    (obj)->data->type == PLOF_DATA_RAW)
#define ISARRAY(obj) (ISOBJ(obj) && \
                      (obj)->data && \
                      (obj)->data->type == PLOF_DATA_ARRAY)
#define RAW(obj) ((struct PlofRawData *) (obj)->data)
#define ARRAY(obj) ((struct PlofArrayData *) (obj)->data)

#define HASHOF(into, rd) \
    if ((rd)->hash) { \
        (into) = (rd)->hash; \
    } else { \
        (into) = (rd)->hash = plofHash((rd)->length, (rd)->data); \
    }

#define ISPTR(obj) (ISRAW(obj) && RAW(obj)->length == sizeof(void*))
#define ASPTR(obj) (*((void **) RAW(obj)->data))
#define SETPTR(obj, val) ASPTR(obj) = (val)

#if defined(PLOF_BOX_NUMBERS)
#define ISINT(obj) (ISRAW(obj) && RAW(obj)->length == sizeof(ptrdiff_t))
#define ASINT(obj) (*((ptrdiff_t *) RAW(obj)->data))
#define SETINT(obj, val) ASINT(obj) = (val)
#elif defined(PLOF_NUMBERS_IN_OBJECTS)
#define ISINT(obj) 1
#define ASINT(obj) ((obj)->direct_data.int_data)
#define SETINT(obj, val) ASINT(obj) = (val)
#elif defined(PLOF_FREE_INTS)
#define ISINT(obj) ((size_t)(obj)&1)
#define ASINT(obj) ((ptrdiff_t)(obj)>>1)
#define SETINT(obj, val) (obj) = (void *) (((ptrdiff_t)(val)<<1) | 1)
#endif

    /* Type coercions */
#define PUSHPTR(val) \
    { \
        ptrdiff_t _val = (ptrdiff_t) (val); \
        \
        rd = GC_NEW_Z(struct PlofRawData); \
        rd->type = PLOF_DATA_RAW; \
        rd->length = sizeof(ptrdiff_t); \
        rd->data = (unsigned char *) GC_NEW(ptrdiff_t); \
        *((ptrdiff_t *) rd->data) = _val; \
        \
        a = GC_NEW_Z(struct PlofObject); \
        a->parent = context; \
        a->data = (struct PlofData *) rd; \
        STACK_PUSH(a); \
    }

#if defined(PLOF_BOX_NUMBERS)
#define PUSHINT(val) PUSHPTR(val)

#elif defined(PLOF_NUMBERS_IN_OBJECTS)
#define PUSHINT(val) \
    { \
        ptrdiff_t _val = (val); \
        \
        a = GC_NEW_Z(struct PlofObject); \
        a->parent = context; \
        a->direct_data.int_data = (ptrdiff_t) _val; \
        STACK_PUSH(a); \
    }
#elif defined(PLOF_FREE_INTS)
#define PUSHINT(val) \
    { \
        STACK_PUSH((void *) (((ptrdiff_t)(val)<<1) | 1)); \
    }
#endif

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
                ret = interpretPSL(d->parent, a, d, 0, NULL, 1, 0); \
            } else { \
                ret = interpretPSL(e->parent, a, e, 0, NULL, 1, 0); \
            } \
            \
            /* maybe rethrow */ \
            if (ret.isThrown) { \
                return ret; \
            } \
            \
            STACK_PUSH(ret.ret); \
        } else { \
            BADTYPE("intcmp"); \
            STACK_PUSH(plofNull); \
        } \
    }

#define STEP pc += 2; jump(*pc)
#define UNIMPL(cmd) fprintf(stderr, "UNIMPLEMENTED: " cmd "\n"); STEP

#ifdef DEBUG
#define DEBUG_CMD(cmd) fprintf(stderr, "DEBUG: " cmd "\n")
#else
#define DEBUG_CMD(cmd)
#endif


    jumphead;

label(interp_psl_nop);
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
label(interp_psl_push0); PSL_PUSH(0);
label(interp_psl_push1); PSL_PUSH(1);
label(interp_psl_push2); PSL_PUSH(2);
label(interp_psl_push3); PSL_PUSH(3);
label(interp_psl_push4); PSL_PUSH(4);
label(interp_psl_push5); PSL_PUSH(5);
label(interp_psl_push6); PSL_PUSH(6);
label(interp_psl_push7); PSL_PUSH(7);

label(interp_psl_pop);
    DEBUG_CMD("pop");
    UNARY;
    STEP;

label(interp_psl_this);
    DEBUG_CMD("this");
    STACK_PUSH(context);
    STEP;

label(interp_psl_null);
    DEBUG_CMD("null");
    STACK_PUSH(plofNull);
    STEP;

label(interp_psl_global);
    DEBUG_CMD("global");
    STACK_PUSH(plofGlobal);
    STEP;

label(interp_psl_new);
    DEBUG_CMD("new");
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    STACK_PUSH(a);
    STEP;

label(interp_psl_combine);
    DEBUG_CMD("combine");
    BINARY;

    if (!ISOBJ(a) || !ISOBJ(b)) {
        /* FIXME */
        BADTYPE("combine");
        STACK_PUSH(plofNull);
        STEP;
    }

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
            rd->data = (unsigned char *) GC_MALLOC_ATOMIC(rd->length);

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
            ad->type = PLOF_DATA_ARRAY;
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
            ad->type = PLOF_DATA_ARRAY;
            memcpy(ad, ARRAY(b), sizeof(struct PlofArrayData));
            ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));
            memcpy(ad->data, ARRAY(b)->data, ad->length * sizeof(struct PlofObject *));

        }

    }

    STACK_PUSH(c);

    STEP;

label(interp_psl_member);
    DEBUG_CMD("member");
    BINARY;
    if (ISOBJ(a) && ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        rd = RAW(b);
        name = rd->data;
        HASHOF(namehash, rd);

        PLOF_READ(a, a, rd->length, name, namehash);
        STACK_PUSH(a);
    } else {
        BADTYPE("member");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_memberset);
    DEBUG_CMD("memberset");
    TRINARY;
    if (ISOBJ(a) && ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        rd = RAW(b);
        name = rd->data;
        HASHOF(namehash, rd);

        PLOF_WRITE(a, rd->length, name, namehash, c);
    } else {
        BADTYPE("memberset");
    }
    STEP;

label(interp_psl_parent);
    DEBUG_CMD("parent");
    UNARY;
    if (ISOBJ(a)) {
        STACK_PUSH(a->parent);
    } else {
        BADTYPE("parent");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_parentset);
    DEBUG_CMD("parentset");
    BINARY;
    if (ISOBJ(a) && ISOBJ(b)) {
        a->parent = b;
    } else {
        BADTYPE("parentset");
    }
    STEP;

label(interp_psl_call);
    DEBUG_CMD("call");
    BINARY;
    if (ISRAW(b)) {
        struct PlofReturn ret = interpretPSL(b->parent, a, b, 0, NULL, 1, 0);

        /* check the return */
        if (ret.isThrown) {
            return ret;
        }

        STACK_PUSH(ret.ret);
    } else {
        /* quay? (ERROR) */
        BADTYPE("call");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_return);
    DEBUG_CMD("return");
    UNARY;
    ret.ret = a;
    ret.isThrown = 0;
    return ret;

label(interp_psl_throw);
    DEBUG_CMD("throw");
    UNARY;
    ret.ret = a;
    ret.isThrown = 1;
    return ret;

label(interp_psl_catch);
    DEBUG_CMD("catch");
    TRINARY;
    if (ISRAW(b)) {
        struct PlofReturn ret = interpretPSL(b->parent, a, b, 0, NULL, 1, 0);

        /* perhaps catch */
        if (ret.isThrown) {
            if (ISRAW(c)) {
                ret = interpretPSL(c->parent, ret.ret, c, 0, NULL, 1, 0);
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
        BADTYPE("catch");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_cmp);
    DEBUG_CMD("cmp");
    QUINARY;
    if (b == c) {
        if (ISRAW(d)) {
            struct PlofReturn ret = interpretPSL(d->parent, a, d, 0, NULL, 1, 0);

            /* rethrow */
            if (ret.isThrown) {
                return ret;
            }
            STACK_PUSH(ret.ret);
        } else {
            BADTYPE("cmp");
            STACK_PUSH(plofNull);
        }
    } else {
        if (ISRAW(e)) {
            struct PlofReturn ret = interpretPSL(e->parent, a, e, 0, NULL, 1, 0);

            /* rethrow */
            if (ret.isThrown) {
                return ret;
            }
            STACK_PUSH(ret.ret);
        } else {
            BADTYPE("cmp");
            STACK_PUSH(plofNull);
        }
    }
    STEP;

label(interp_psl_concat);
    DEBUG_CMD("concat");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        struct PlofRawData *ra, *rb;

        ra = RAW(a);
        rb = RAW(b);

        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = ra->length + rb->length;
        rd->data = (unsigned char *) GC_MALLOC_ATOMIC(rd->length);
        memcpy(rd->data, ra->data, ra->length);
        memcpy(rd->data + ra->length, rb->data, rb->length);

        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;

        STACK_PUSH(a);

    } else {
        BADTYPE("concat");
        STACK_PUSH(plofNull);

    }
    STEP;

label(interp_psl_wrap);
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
        rd->data = (unsigned char *) GC_MALLOC_ATOMIC(rd->length);

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

    } else {
        BADTYPE("raw");
        STACK_PUSH(plofNull);

    }
    STEP;

label(interp_psl_resolve);
    DEBUG_CMD("resolve");
    BINARY;
    {
        int i;
        size_t *hashes;

        if (!ISOBJ(a) || !ISOBJ(b)) {
            /* FIXME */
            BADTYPE("resolve");
            STACK_PUSH(plofNull);
            STACK_PUSH(plofNull);
            STEP;
        }

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
        hashes = (size_t *) GC_MALLOC_ATOMIC(ad->length * sizeof(size_t));
        for (i = 0; i < ad->length; i++) {
            rd = RAW(ad->data[i]);
            HASHOF(hashes[i], rd);
        }

        /* now try to find a match */
        b = plofNull;
        while (a && a != plofNull) {
            for (i = 0; i < ad->length; i++) {
                rd = RAW(ad->data[i]);
                PLOF_READ(b, a, rd->length, rd->data, hashes[i]);
                if (b != plofNull) {
                    /* done */
                    STACK_PUSH(a);
                    STACK_PUSH(ad->data[i]);
                    a = plofNull;
                    break;
                }
            }

            a = a->parent;
        }

        if (b == plofNull) {
            /* didn't find one */
            STACK_PUSH(plofNull);
            STACK_PUSH(plofNull);
        }
    }
    STEP;

label(interp_psl_calli);
    DEBUG_CMD("calli");
    UNARY;
    if (ISRAW(a)) {
        struct PlofReturn ret = interpretPSL(a->parent, plofNull, a, 0, NULL, 1, 1);

        /* check the return */
        if (ret.isThrown) {
            return ret;
        }

        STACK_PUSH(ret.ret);

    } else {
        /* quay? (ERROR) */
        BADTYPE("calli");
        STACK_PUSH(plofNull);

    }
    STEP;

label(interp_psl_while);
    DEBUG_CMD("while");
    TRINARY;

    if (ISRAW(b) && ISRAW(c)) {
        struct PlofReturn ret;

        /* now run the loop */
        while (1) {
            ret = interpretPSL(b->parent, plofNull, b, 0, NULL, 1, 0);
            
            if (ret.isThrown) {
                return ret;
            } else if (ret.ret == plofNull) {
                break;
            }

            /* condition succeeded, run the code */
            ret = interpretPSL(c->parent, a, c, 0, NULL, 1, 0);

            if (ret.isThrown) {
                return ret;
            }

            a = ret.ret;
        }

        STACK_PUSH(a);

    } else {
        BADTYPE("while");
        STACK_PUSH(plofNull);

    }

    STEP;

label(interp_psl_replace);
    DEBUG_CMD("replace");
    BINARY;
    if (ISRAW(a) && ISARRAY(b)) {
        size_t i;

        ad = ARRAY(b);

        /* verify that the array contains only raw data */
        for (i = 0; i < ad->length; i++) {
            if (!ISRAW(ad->data[i])) {
                /* problem! */
                goto psl_replace_error;
            }
        }

        /* now replace */
        rd = pslReplace(RAW(a), ad);

        /* and put it in an object */
        b = GC_NEW_Z(struct PlofObject);
        b->parent = a->parent;
        b->data = (struct PlofData *) rd;
        STACK_PUSH(b);

    } else {
        BADTYPE("replace");
psl_replace_error:
        STACK_PUSH(plofNull);

    }
    STEP;

label(interp_psl_array);
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
                 stacki >= 0 &&
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

label(interp_psl_aconcat);
    DEBUG_CMD("aconcat");
    BINARY;
    {
        struct PlofArrayData *aa, *ba, *ra;
        size_t al, bl, rl;
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
        if (al)
            memcpy(ra->data, aa->data, al * sizeof(struct PlofObject *));
        if (bl)
            memcpy(ra->data + al, ba->data, bl * sizeof(struct PlofObject *));

        /* now put it in an object */
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) ra;
        STACK_PUSH(a);
    }
    STEP;

label(interp_psl_length);
    DEBUG_CMD("length");
    UNARY;
    if (ISARRAY(a)) {
        PUSHINT(ARRAY(a)->length);
    } else {
        BADTYPE("length");
        PUSHINT(0);
    }
    STEP;

label(interp_psl_lengthset);
    DEBUG_CMD("lengthset");
    BINARY;
    if (ISARRAY(a) && ISINT(b)) {
        size_t oldlen, newlen, i;
        ad = ARRAY(a);
        oldlen = ad->length;
        newlen = ASINT(b);

        /* reallocate */
        ad->data = (struct PlofObject **) GC_REALLOC(ad->data, newlen * sizeof(struct PlofObject *));

        /* and assign nulls */
        for (i = oldlen; i < newlen; i++) {
            ad->data[i] = plofNull;
        }
    } else {
        BADTYPE("lengthset");
    }
    STEP;

label(interp_psl_index);
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
        BADTYPE("index");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_indexset);
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
    } else {
        BADTYPE("indexset");
    }
    STEP;

label(interp_psl_members);
    DEBUG_CMD("members");
    UNARY;
    if (ISOBJ(a)) {
        ad = plofMembers(a);
        b = GC_NEW_Z(struct PlofObject);
        b->parent = context;
        b->data = (struct PlofData *) ad;
        STACK_PUSH(b);
    } else {
        BADTYPE("members");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_rawlength);
    DEBUG_CMD("rawlength");
    UNARY;
    if (ISRAW(a)) {
        PUSHINT(RAW(a)->length);
    } else {
        BADTYPE("rawlength");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_slice);
    DEBUG_CMD("slice");
    TRINARY;
    if (ISRAW(a) && ISINT(b) && ISINT(c)) {
        size_t start = ASINT(b);
        size_t end = ASINT(c);
        struct PlofRawData *ra = RAW(a);

        /* make sure we're in bounds */
        if (start >= ra->length)
            start = ra->length - 1;
        if (end > ra->length)
            end = ra->length;
        if (end < start)
            end = start;

        /* start making a new rd */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = end - start;
        rd->data = ra->data + start;

        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;
        STACK_PUSH(a);
    } else {
        BADTYPE("slice");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_rawcmp);
    DEBUG_CMD("rawcmp");
    BINARY;
    {
        ptrdiff_t val = 0;

        /* make sure they're both raws */
        if (ISRAW(a) && ISRAW(b)) {
            size_t shorter;

            /* get the data */
            struct PlofRawData *ra, *rb;
            ra = RAW(a);
            rb = RAW(b);

            /* figure out the shorter one */
            shorter = ra->length;
            if (rb->length < shorter)
                shorter = rb->length;

            /* memcmp them */
            val = -memcmp(ra->data, rb->data, shorter);

            /* a zero might be false */
            if (val == 0) {
                if (ra->length < rb->length)
                    val = -1;
                else if (ra->length > rb->length)
                    val = 1;
            }
        } else {
            BADTYPE("rawcmp");
        }

        PUSHINT(val);
    }
    STEP;

label(interp_psl_integer);
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
#if SIZEOF_VOID_P < 8
                case 8:
#endif
                    val = ((ptrdiff_t) rd->data[0] << 24) |
                          ((ptrdiff_t) rd->data[1] << 16) |
                          ((ptrdiff_t) rd->data[2] << 8) |
                          ((ptrdiff_t) rd->data[3]);
                    break;

#if SIZEOF_VOID_P >= 8
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
#endif
            }
        }

        PUSHINT(val);
    }
    STEP;

label(interp_psl_intwidth);
    DEBUG_CMD("intwidth");
#if defined(PLOF_BOX_NUMBERS) || defined(PLOF_NUMBERS_IN_OBJECTS)
    PUSHINT(sizeof(ptrdiff_t)*8);
#elif defined(PLOF_FREE_INTS)
    PUSHINT(sizeof(ptrdiff_t)*8-1);
#endif
    STEP;

label(interp_psl_mul);
    DEBUG_CMD("mul");
    INTBINOP(*);
    STEP;

label(interp_psl_div);
    DEBUG_CMD("div");
    INTBINOP(/);
    STEP;

label(interp_psl_mod);
    DEBUG_CMD("mod");
    INTBINOP(%);
    STEP;

label(interp_psl_add);
    DEBUG_CMD("add");
    INTBINOP(+);
    STEP;

label(interp_psl_sub);
    DEBUG_CMD("sub");
    INTBINOP(-);
    STEP;

label(interp_psl_lt);
    DEBUG_CMD("lt");
    INTCMP(<);
    STEP;

label(interp_psl_lte);
    DEBUG_CMD("lte");
    INTCMP(<=);
    STEP;

label(interp_psl_eq);
    DEBUG_CMD("eq");
    INTCMP(==);
    STEP;

label(interp_psl_ne);
    DEBUG_CMD("ne");
    INTCMP(!=);
    STEP;

label(interp_psl_gt);
    DEBUG_CMD("gt");
    INTCMP(>);
    STEP;

label(interp_psl_gte);
    DEBUG_CMD("gte");
    INTCMP(>=);
    STEP;

label(interp_psl_sl);
    DEBUG_CMD("sl");
    INTBINOP(<<);
    STEP;

label(interp_psl_sr);
    DEBUG_CMD("sr");
    INTBINOP(>>);
    STEP;

label(interp_psl_or);
    DEBUG_CMD("or");
    INTBINOP(|);
    STEP;

label(interp_psl_nor);
    DEBUG_CMD("nor");
    /* or it, then not it */
    INTBINOP(|);
    /*ASINT(stack[stacktop]) = ~ASINT(stack[stacktop]);*/
    SETINT(stack[stacktop], ~ASINT(stack[stacktop]));
    STEP;

label(interp_psl_xor);
    DEBUG_CMD("xor");
    INTBINOP(^);
    STEP;

label(interp_psl_nxor);
    DEBUG_CMD("nxor");
    INTBINOP(^);
    /*ASINT(stack[stacktop]) = ~ASINT(stack[stacktop]);*/
    SETINT(stack[stacktop], ~ASINT(stack[stacktop]));
    STEP;

label(interp_psl_and);
    DEBUG_CMD("and");
    INTBINOP(&);
    STEP;

label(interp_psl_nand);
    DEBUG_CMD("nand");
    INTBINOP(&);
    /*ASINT(stack[stacktop]) = ~ASINT(stack[stacktop]);*/
    SETINT(stack[stacktop], ~ASINT(stack[stacktop]));
    STEP;

label(interp_psl_byte);
    DEBUG_CMD("byte");
    UNARY;
    if (ISINT(a)) {
        ptrdiff_t val = ASINT(a);

        /* prepare the new value */
        rd = (struct PlofRawData *) GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = 1;
        rd->data = (unsigned char *) GC_MALLOC_ATOMIC(1);
        rd->data[0] = (unsigned char) (val & 0xFF);

        /* and push it */
        a = (struct PlofObject *) GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;
        STACK_PUSH(a);
    } else {
        BADTYPE("byte");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_float); UNIMPL("psl_float");
label(interp_psl_fint); UNIMPL("psl_fint");
label(interp_psl_fmul); UNIMPL("psl_fmul");
label(interp_psl_fdiv); UNIMPL("psl_fdiv");
label(interp_psl_fmod); UNIMPL("psl_fmod");
label(interp_psl_fadd); UNIMPL("psl_fadd");
label(interp_psl_fsub); UNIMPL("psl_fsub");
label(interp_psl_flt); UNIMPL("psl_flt");
label(interp_psl_flte); UNIMPL("psl_flte");
label(interp_psl_feq); UNIMPL("psl_feq");
label(interp_psl_fne); UNIMPL("psl_fne");
label(interp_psl_fgt); UNIMPL("psl_fgt");
label(interp_psl_fgte); UNIMPL("psl_fgte");

label(interp_psl_version);
    DEBUG_CMD("version");
    {
        size_t i;
        ad = GC_NEW_Z(struct PlofArrayData);
        ad->type = PLOF_DATA_ARRAY;

#define CREATE_VERSION(str) \
        { \
            rd = GC_NEW_Z(struct PlofRawData); \
            rd->type = PLOF_DATA_RAW; \
            rd->length = strlen(str); \
            rd->data = (unsigned char *) str; \
            \
            a = GC_NEW_Z(struct PlofObject); \
            a->parent = context; \
            a->data = (struct PlofData *) rd; \
            \
            ad->data[i++] = a; \
        }

        ad->data = (struct PlofObject **) GC_MALLOC(VERSION_MAX * sizeof(struct PlofObject *));

        /* now go step by step */
        i = 0;
        CREATE_VERSION("cplof");

        /* CNFI, if applicable */
#ifdef WITH_CNFI
        CREATE_VERSION("CNFI");
#endif

        /* Standards */
#ifdef _POSIX_VERSION
        CREATE_VERSION("POSIX");
#endif

        /* Architectures */
#ifdef __arm__
        CREATE_VERSION("ARM");
#endif
#ifdef __mips__
        CREATE_VERSION("MIPS");
#endif
#ifdef __powerpc
        CREATE_VERSION("PowerPC");
#endif
#if defined(i386) || defined(__i386) || defined(_M_IX86) || \
    defined(__THW_INTEL__) || defined(__INTEL__) || defined(__amd64__)
        CREATE_VERSION("x86");
#endif
#ifdef __amd64__
        CREATE_VERSION("x86_64");
#endif

        /* Kernels */
#ifdef __APPLE__
        CREATE_VERSION("Darwin");
#endif
#ifdef MSDOS
        CREATE_VERSION("DOS");
#endif
#ifdef __FreeBSD__
        CREATE_VERSION("FreeBSD");
#endif
#ifdef __GNU__
        CREATE_VERSION("HURD");
#endif
#ifdef linux
        CREATE_VERSION("Linux");
#endif
#ifdef __NetBSD__
        CREATE_VERSION("NetBSD");
#endif
#ifdef __OpenBSD__
        CREATE_VERSION("OpenBSD");
#endif
#ifdef sun
        CREATE_VERSION("Solaris");
#endif

        /* Standard libraries */
#ifdef BSD
        CREATE_VERSION("BSD");
#endif
#ifdef __GLIBC__
        CREATE_VERSION("glibc");
#endif
#ifdef __APPLE__
        CREATE_VERSION("Mac OS X");
#endif
#ifdef _WIN32
        CREATE_VERSION("Windows");
#endif


        /* Finally, put it in an object */
        ad->length = i;
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) ad;
        STACK_PUSH(a);
    }
    STEP;


label(interp_psl_dsrcfile); UNIMPL("psl_dsrcfile");
label(interp_psl_dsrcline); UNIMPL("psl_dsrcline");
label(interp_psl_dsrccol); UNIMPL("psl_dsrccol");

label(interp_psl_print);
    DEBUG_CMD("print");
    /* do our best to print this (debugging) */
    UNARY;
    if (ISRAW(a)) {
        fwrite(RAW(a)->data, 1, RAW(a)->length, stdout);
        fputc('\n', stdout);

#if defined(PLOF_BOX_NUMBERS)
        if (RAW(a)->length == sizeof(ptrdiff_t)) {
            printf("Integer value: %d\n", (int) *((ptrdiff_t *) RAW(a)->data));
        }
#endif
    } else if (ISOBJ(a)) {
        printf("%d %p\n", (int) ASINT(a), (void *) a);
    } else if (ISINT(a)) {
        printf("%d\n", (int) ASINT(a));
    }
    STEP;

label(interp_psl_debug);
    DEBUG_CMD("debug");
    printf("STACK LENGTH: %d\n", (int) stacktop);
    STEP;

label(interp_psl_trap); UNIMPL("psl_trap");
label(interp_psl_include); UNIMPL("psl_include");
label(interp_psl_parse); UNIMPL("psl_parse");

label(interp_psl_gadd); DEBUG_CMD("gadd"); STEP;
label(interp_psl_grem); DEBUG_CMD("grem"); STEP;
label(interp_psl_gcommit); DEBUG_CMD("gcommit"); STEP;
label(interp_psl_marker); DEBUG_CMD("marker"); STEP;

label(interp_psl_immediate);
    DEBUG_CMD("immediate");
    if (immediate) {
        rd = (struct PlofRawData *) pc[1];
        interpretPSL(context, plofNull, NULL, rd->length, rd->data, 0, 0);
    }
    STEP;

label(interp_psl_code);
label(interp_psl_raw);
    DEBUG_CMD("raw");
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    a->data = (struct PlofData *) pc[1];
    STACK_PUSH(a);
    STEP;

#ifndef WITH_CNFI
label(interp_psl_dlopen); UNIMPL("psl_dlopen");
label(interp_psl_dlclose); UNIMPL("psl_dlclose");
label(interp_psl_dlsym); UNIMPL("psl_dlsym");
label(interp_psl_cmalloc); UNIMPL("psl_cmalloc");
label(interp_psl_cfree); UNIMPL("psl_cfree");
label(interp_psl_cget); UNIMPL("psl_cget");
label(interp_psl_cset); UNIMPL("psl_cset");
label(interp_psl_ctype); UNIMPL("psl_ctype");
label(interp_psl_cstruct); UNIMPL("psl_cstruct");
label(interp_psl_csizeof); UNIMPL("psl_csizeof");
label(interp_psl_csget); UNIMPL("psl_csget");
label(interp_psl_csset); UNIMPL("psl_csset");
label(interp_psl_prepcif); UNIMPL("psl_prepcif");
label(interp_psl_ccall); UNIMPL("psl_ccall");
#else

label(interp_psl_dlopen);
    DEBUG_CMD("dlopen");
    UNARY;

    /* the argument can be a string, or NULL */
    {
        void *hnd;
        char *fname = NULL;
        if (a != plofNull) {
            if (ISRAW(a)) {
                fname = (char *) RAW(a)->data;
            } else {
                BADTYPE("dlopen");
            }
        }

        /* OK, try to dlopen it */
        hnd = dlopen(fname, RTLD_LAZY|RTLD_GLOBAL);

        /* either turn that into a pointer in raw data, or push null */
        if (hnd == NULL) {
            STACK_PUSH(plofNull);

        } else {
            PUSHPTR(hnd);

        }
    }
    STEP;

label(interp_psl_dlclose);
    DEBUG_CMD("dlclose");
    UNARY;
    if (ISPTR(a)) {
        dlclose(ASPTR(a));
    } else {
        BADTYPE("dlclose");
    }
    STEP;

label(interp_psl_dlsym);
    DEBUG_CMD("dlsym");
    BINARY;

    {
        void *hnd = NULL;
        char *fname;
        void *fun;

        /* the handle may be null */
        if (a != plofNull) {
            if (ISPTR(a)) {
                hnd = ASPTR(a);

            } else {
                BADTYPE("dlsym");
                
            }
        }

        /* the function name can't */
        if (ISRAW(b)) {
            fname = (char *) RAW(b)->data;

            fun = dlsym(hnd, fname);

            if (fun == NULL) {
                STACK_PUSH(plofNull);

            } else {
                PUSHPTR(fun);

            }
        }

    }
    STEP;

label(interp_psl_cmalloc);
    DEBUG_CMD("cmalloc");
    UNARY;

    if (ISINT(a)) {
        void *ret = malloc(ASINT(a));
        if (ret == NULL) {
            STACK_PUSH(plofNull);
        } else {
            PUSHPTR(ret);
        }

    } else {
        BADTYPE("cmalloc");
        STACK_PUSH(plofNull);
    }

    STEP;

label(interp_psl_cfree);
    DEBUG_CMD("cfree");
    UNARY;

    if (ISPTR(a)) {
        free(ASPTR(a));
    } else {
        BADTYPE("cfree");
    }

    STEP;

label(interp_psl_cget);
    DEBUG_CMD("cget");
    BINARY;

    if (ISPTR(a) && ISINT(b)) {
        /* construct a raw data object */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = ASINT(b);
        rd->data = (unsigned char *) ASPTR(a);

        c = GC_NEW_Z(struct PlofObject);
        c->parent = context;
        c->data = (struct PlofData *) rd;

        STACK_PUSH(c);

    } else {
        BADTYPE("cget");
        STACK_PUSH(plofNull);

    }

    STEP;

label(interp_psl_cset);
    DEBUG_CMD("cset");
    BINARY;
    if (ISPTR(a) && ISRAW(b)) {
        memcpy(a, RAW(b)->data, RAW(b)->length);
    } else {
        BADTYPE("cset");
    }
    STEP;

label(interp_psl_ctype);
    DEBUG_CMD("ctype");
    UNARY;

    if (ISINT(a)) {
        int typenum = ASINT(a);
        ffi_type *type = NULL;

        switch (typenum) {
            case psl_ctype_void:
                type = &ffi_type_void;
                break;
            case psl_ctype_int:
                type = &ffi_type_sint;
                break;
            case psl_ctype_float:
                type = &ffi_type_float;
                break;
            case psl_ctype_double:
                type = &ffi_type_double;
                break;
            case psl_ctype_long_double:
                type = &ffi_type_longdouble;
                break;
            case psl_ctype_uint8:
                type = &ffi_type_uint8;
                break;
            case psl_ctype_int8:
                type = &ffi_type_sint8;
                break;
            case psl_ctype_uint16:
                type = &ffi_type_uint16;
                break;
            case psl_ctype_int16:
                type = &ffi_type_sint16;
                break;
            case psl_ctype_uint32:
                type = &ffi_type_uint32;
                break;
            case psl_ctype_int32:
                type = &ffi_type_sint32;
                break;
            case psl_ctype_uint64:
                type = &ffi_type_uint64;
                break;
            case psl_ctype_int64:
                type = &ffi_type_sint64;
                break;
            case psl_ctype_pointer:
                type = &ffi_type_pointer;
                break;
            case psl_ctype_uchar:
                type = &ffi_type_uchar;
                break;
            case psl_ctype_schar:
                type = &ffi_type_schar;
                break;
            case psl_ctype_ushort:
                type = &ffi_type_ushort;
                break;
            case psl_ctype_sshort:
                type = &ffi_type_sshort;
                break;
            case psl_ctype_uint:
                type = &ffi_type_uint;
                break;
            case psl_ctype_sint:
                type = &ffi_type_sint;
                break;
            case psl_ctype_ulong:
                type = &ffi_type_ulong;
                break;
            case psl_ctype_slong:
                type = &ffi_type_slong;
                break;
            case psl_ctype_ulonglong:
                type = &ffi_type_uint64;
                break;
            case psl_ctype_slonglong:
                type = &ffi_type_sint64;
                break;
            default:
                BADTYPE("ctype");
                type = &ffi_type_void;
        }

        PUSHPTR(type);

    } else {
        BADTYPE("ctype");
        STACK_PUSH(plofNull);

    }

    STEP;

label(interp_psl_cstruct); UNIMPL("psl_cstruct");

label(interp_psl_csizeof);
    DEBUG_CMD("csizeof");
    UNARY;
    if (ISPTR(a)) {
        /* this had better be an ffi_type pointer :) */
        ffi_type *type = (ffi_type *) ASPTR(a);
        PUSHINT(type->size);
    } else {
        BADTYPE("csizeof");
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_csget); UNIMPL("psl_csget");
label(interp_psl_csset); UNIMPL("psl_csset");

label(interp_psl_prepcif); UNIMPL("psl_prepcif");
    DEBUG_CMD("prepcif");
    TRINARY;

    if (ISPTR(a) && ISARRAY(b) && ISINT(c)) {
        ffi_cif *cif;
        ffi_type *rettype;
        int abi, i;
        ffi_type **atypes;
        ffi_status pcret;

        /* get the data for prepcif */
        rettype = (ffi_type *) ASPTR(a);
        ad = ARRAY(b);
        abi = ASINT(c);

        /* put the argument types in the proper type of array */
        atypes = GC_MALLOC_ATOMIC(ad->length * sizeof(ffi_type *));
        for (i = 0; i < ad->length; i++) {
            if (ISPTR(ad->data[i])) {
                atypes[i] = (ffi_type *) ASPTR(ad->data[i]);
            } else {
                atypes[i] = NULL;
            }
        }

        /* allocate space for the cif itself */
        cif = GC_MALLOC_ATOMIC(sizeof(ffi_cif));

        /* and call prepcif */
        pcret = ffi_prep_cif(cif, abi, ad->length, rettype, atypes);
        if (pcret != FFI_OK) {
            STACK_PUSH(plofNull);
        } else {
            PUSHPTR(cif);
        }

    } else {
        BADTYPE("prepcif");
        STACK_PUSH(plofNull);

    }

    STEP;

label(interp_psl_ccall); UNIMPL("psl_ccall");

#endif

label(interp_psl_done);
    UNARY;
    ret.ret = a;
    ret.isThrown = 0;
    return ret;

    jumptail;
}

/* Convert a PSL bignum to an int */
int pslBignumToInt(unsigned char *bignum, size_t *into)
{
    size_t ret = 0;
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
#if SIZEOF_VOID_P <= 2
    } else {
        return 3;
#else
    } else if (val < ((size_t) 1<<21)) {
        return 3;
    } else if (val < ((size_t) 1<<28)) {
        return 4;
#if SIZEOF_VOID_P <= 4
    } else {
        return 5;
#else
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
#endif /* 32 */
#endif /* 16 */
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
size_t plofHash(size_t slen, unsigned char *str)
{
    /* this is the hash function used in sdbm */
    size_t hash = 0;

    for (; slen > 0; slen--)
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;

    if (hash == 0) hash = 1; /* 0 is used as "hasn't been hashed" */

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

    if (of == NULL) {
        ret.length = 0;
        ret.data = NULL;
        return ret;
    }

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

/* Implementation of 'replace' */
struct PlofRawData *pslReplace(struct PlofRawData *in, struct PlofArrayData *with)
{
    size_t i, retalloc;
    struct PlofRawData *ret = GC_NEW_Z(struct PlofRawData);
    struct PlofRawData *wr;
    ret->type = PLOF_DATA_RAW;

    /* preallocate some space */
    retalloc = in->length;
    ret->length = 0;
    ret->data = (unsigned char *) GC_MALLOC_ATOMIC(retalloc);

    /* auto-reallocation */
#define SPACEFOR(n) \
    while (ret->length + (n) > retalloc) { \
        retalloc *= 2; \
        ret->data = (unsigned char *) GC_REALLOC(ret->data, retalloc); \
    }

    /* go through the commands in 'in' ... */
    for (i = 0; i < in->length; i++) {
        unsigned char cmd = in->data[i];
        struct PlofRawData *data = NULL;

        /* get out the data */
        if (cmd >= psl_marker) {
            data = GC_NEW_Z(struct PlofRawData);

            i++;
            i += pslBignumToInt(in->data + i, (size_t *) &data->length);
            if (i + data->length > in->length) {
                data->length = in->length - i;
            }
            data->data = in->data + i;
            i += data->length - 1;
        }

        /* do something with the command */
        if (cmd == psl_marker) {
            /* what's the marker #? */
            size_t mval = (size_t) -1;
            if (data->length == 1) {
                mval = *((unsigned char *) data);
            }

            /* maybe replace it */
            if (mval < with->length) {
                /* yup! */
                wr = RAW(with->data[mval]);

                /* do we have enough space? */
                SPACEFOR(wr->length);

                /* copy it in */
                memcpy(ret->data + ret->length, wr->data, wr->length);
                ret->length += wr->length;
            }

        } else if (cmd == psl_code) {
            size_t bignumlen;

            /* recurse */
            struct PlofRawData *sub = pslReplace(data, with);

            /* figure out the bignum length */
            bignumlen = pslBignumLength(sub->length);

            /* make sure we have enough room */
            SPACEFOR(1 + bignumlen + sub->length);

            /* then copy it all in */
            ret->data[ret->length] = cmd;
            pslIntToBignum(ret->data + ret->length + 1, sub->length, bignumlen);
            memcpy(ret->data + ret->length + 1 + bignumlen, sub->data, sub->length);
            ret->length += 1 + bignumlen + sub->length;

        } else if (cmd > psl_marker) {
            /* rewrite it */
            size_t bignumlen = pslBignumLength(data->length);
            SPACEFOR(1 + bignumlen + data->length);
            ret->data[ret->length] = cmd;
            pslIntToBignum(ret->data + ret->length + 1, data->length, bignumlen);
            memcpy(ret->data + ret->length + 1 + bignumlen, data->data, data->length);
            ret->length += 1 + bignumlen + data->length;

        } else {
            /* just write out the command */
            SPACEFOR(1);
            ret->data[ret->length++] = cmd;

        }
    }

    return ret;
}

/* GC on DJGPP is screwy */
#ifdef __DJGPP__
void vsnprintf() {}
#endif

/* Null and global */
struct PlofObject *plofNull = NULL;
struct PlofObject *plofGlobal = NULL;
