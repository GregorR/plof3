/*
 * Helpful macros for implementing PSL operations
 *
 * Copyright (C) 2009 Gregor Richards
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

#ifndef IMPL_H
#define IMPL_H

/* convenient macros for implementing PSL */
#ifdef HAVE_CONFIG_H
#include "../config.h"
#else
#include "basicconfig.h"
#endif

/* "Function" for pushing to the stack */
#define STACK_PUSH(val) \
{ \
    *stacktop = (val); \
    stacktop++; /* do NOT put this with previous line, as val could include stacktop */ \
}
#define STACK_POP(into) \
{ \
    into = *--stacktop; \
}

/* Standards for n-ary ops */
#define UNARY STACK_POP(a)
#define BINARY STACK_POP(b) STACK_POP(a)
#define TRINARY STACK_POP(c) STACK_POP(b) STACK_POP(a)
#define QUATERNARY STACK_POP(d) STACK_POP(c) STACK_POP(b) STACK_POP(a)
#define QUINARY STACK_POP(e) STACK_POP(d) STACK_POP(c) STACK_POP(b) STACK_POP(a)

/* Basic type-checks */
#define BADTYPE(cmd) \
{ \
    rd = newPlofRawData(sizeof(cmd)-1 + 15); \
    sprintf((char *) rd->data, "Type error in %s", cmd); \
    a = newPlofObject(); \
    a->parent = plofNull; \
    a->data = (struct PlofData *) rd; \
    \
    ret.ret = newPlofObject(); \
    ret.ret->parent = plofNull; \
    plofWrite(ret.ret, (unsigned char *) PSL_EXCEPTION_STACK, plofHash(sizeof(PSL_EXCEPTION_STACK)-1, (unsigned char *) PSL_EXCEPTION_STACK), a); \
    ret.isThrown = 1; \
    goto performThrow; \
}
#ifdef DEBUG
#define DBADTYPE BADTYPE
#else
#define DBADTYPE(cmd) ;
#endif

#if defined(PLOF_BOX_NUMBERS)
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
#define ISLOCALS(obj) (ISOBJ(obj) && \
                  (obj)->data && \
                  (obj)->data->type == PLOF_DATA_LOCALS)
#define RAW(obj) ((struct PlofRawData *) (obj)->data)
#define ARRAY(obj) ((struct PlofArrayData *) (obj)->data)
#define LOCALS(obj) ((struct PlofArrayData *) (obj)->data)
#define RAWSTRDUP(type, into, _rd) \
{ \
    unsigned char *_into = (unsigned char *) GC_MALLOC_ATOMIC((_rd)->length + 1); \
    memcpy(_into, (_rd)->data, (_rd)->length); \
    _into[(_rd)->length] = '\0'; \
    (into) = (type *) _into; \
}

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
#elif defined(PLOF_FREE_INTS)
#define ISINT(obj) ((size_t)(obj)&1)
#define ASINT(obj) ((ptrdiff_t)(obj)>>1)
#define SETINT(obj, val) (obj) = (void *) (((ptrdiff_t)(val)<<1) | 1)
#endif

/* Type coercions */
#define RDPTR(val) \
{ \
    ptrdiff_t _val = (ptrdiff_t) (val); \
    \
    rd = newPlofRawDataNonAtomic(sizeof(ptrdiff_t)); \
    *((ptrdiff_t *) rd->data) = _val; \
}
#define PUSHPTR(val) \
{ \
    struct PlofObject *newo; \
    RDPTR(val); \
    newo = newPlofObject(); \
    newo->parent = context; \
    newo->data = (struct PlofData *) rd; \
    STACK_PUSH(newo); \
}

#if defined(PLOF_BOX_NUMBERS)
#define RDINT(val) RDPTR(val)
#define PUSHINT(val) PUSHPTR(val)

#elif defined(PLOF_FREE_INTS)
#define RDINT(val) \
{ \
    rd = (struct PlofRawData *) (((ptrdiff_t)(val)<<1) | 1); \
}
#define PUSHINT(val) \
{ \
    STACK_PUSH((void *) (((ptrdiff_t)(val)<<1) | 1)); \
}

#endif

/* "Functions" for integer ops */
#define INTBINOP(op, opname) \
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
    } else { \
        BADTYPE(opname); \
    } \
    \
    PUSHINT(res); \
}
#define INTCMP(op) \
QUINARY; \
{ \
    if (ISINT(b) && ISINT(c) && ISRAW(d) && ISRAW(e)) { \
        ptrdiff_t ia, ib; \
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
            goto performThrow; \
        } \
        \
        STACK_PUSH(ret.ret); \
    } else { \
        BADTYPE("intcmp"); \
        STACK_PUSH(plofNull); \
    } \
}

#define UNIMPL(cmd) fprintf(stderr, "UNIMPLEMENTED: " cmd "\n"); STEP

#ifdef DEBUG_TIMING
#define DEBUG_CMD(cmd) tiName = cmd; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stspec)
#else
#ifdef DEBUG
#define DEBUG_CMD(cmd) fprintf(stderr, "DEBUG: " cmd "\n")
#else
#define DEBUG_CMD(cmd)
#endif
#endif

#ifdef DEBUG_TIMING
#define STEP \
{ \
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &etspec); \
    printf("\"%s\",%lld\n", \
           tiName, \
           (etspec.tv_sec - stspec.tv_sec) * 1000000000LL + \
           (etspec.tv_nsec - stspec.tv_nsec)); \
    pc += 2; \
    jump(*pc); \
}
#else
#define STEP pc += 2; jump(*pc)
#endif


/* inlining in compilePSL */
#define INLINE_PSL(op) \
{ \
if (cpsli >= 4 && cpsl[cpsli-4] == pslCompileLabels[label_psl_code] && cpsl[cpsli-2] == pslCompileLabels[label_psl_code]) { \
    struct PlofRawData *rda, *rdb; \
    void **cpsla, **cpslb; \
    size_t cpsllena, cpsllenb, ssa, ssb; \
    size_t cpslibase; \
 \
    rda = (struct PlofRawData *) cpslargs[cpslai-2]; \
    rdb = (struct PlofRawData *) cpslargs[cpslai-1]; \
 \
    cpsli -= 4; \
    cpslibase = cpsli; \
 \
    /* compile the first one */ \
    ret = compilePSL(rda->length, rda->data, 0, &cpslalen, &cpslai, cpslargs, &cpsllena, &cpslargs); \
    cpsla = ((struct CPSLArgsHeader *) cpslargs)->cpsl; \
    if (stacksize + ((struct CPSLArgsHeader *) cpslargs)->maxstacksize > maxstacksize) \
        maxstacksize = stacksize + ((struct CPSLArgsHeader *) cpslargs)->maxstacksize; \
    ssa = ((struct CPSLArgsHeader *) cpslargs)->endstacksize; \
    if (ret.isThrown) return ret; \
 \
    /* compile the second one */ \
    ret = compilePSL(rdb->length, rdb->data, 0, &cpslalen, &cpslai, cpslargs, &cpsllenb, &cpslargs); \
    cpslb = ((struct CPSLArgsHeader *) cpslargs)->cpsl; \
    if (stacksize + ((struct CPSLArgsHeader *) cpslargs)->maxstacksize > maxstacksize) \
        maxstacksize = stacksize + ((struct CPSLArgsHeader *) cpslargs)->maxstacksize; \
    ssb = ((struct CPSLArgsHeader *) cpslargs)->endstacksize; \
    if (ret.isThrown) return ret; \
 \
    /* put in the first jump instruction */ \
    cpsl[cpsli] = pslCompileLabels[label_psl_ ## op ]; \
    cpsl[cpsli+1] = (void *) (cpsllena + 6); /* + 6 for the intervening this' and jmps */ \
    cpsli += 2; \
 \
    /* now append the first bit */ \
    while (cpsllen < cpsli + cpsllena + 6) { \
        cpsllen *= 2; \
        cpsl = GC_REALLOC(cpsl, cpsllen * sizeof(void *)); \
    } \
    cpsl[cpsli] = pslCompileLabels[label_psl_pushthis]; \
    cpsli += 2; \
    memcpy(cpsl + cpsli, cpsla, cpsllena * sizeof(void *)); \
    cpsli += cpsllena - 2; \
 \
    /* replace the ending return with a stackfrunge */ \
    cpsl[cpsli] = pslCompileLabels[label_psl_stackfrunge]; \
    cpsl[cpsli+1] = (void *) ssa; \
    cpsli += 2; \
    cpsl[cpsli] = pslCompileLabels[label_psl_popthis]; \
    cpsli += 2; \
 \
    /* and add the jmp */ \
    cpsl[cpsli] = pslCompileLabels[label_psl_jmp]; \
    cpsl[cpsli+1] = (void *) (cpsllenb + 4); /* + 4 for the intervening this' */ \
    cpsli += 2; \
 \
    /* append the second bit */ \
    while (cpsllen < cpsli + cpsllenb + 4) { \
        cpsllen *= 2; \
        cpsl = GC_REALLOC(cpsl, cpsllen * sizeof(void *)); \
    } \
    cpsl[cpsli] = pslCompileLabels[label_psl_pushthis]; \
    cpsli += 2; \
    memcpy(cpsl + cpsli, cpslb, cpsllenb * sizeof(void *)); \
    cpsli += cpsllenb - 2; \
 \
    /* replace the ending return with a stackfrunge */ \
    cpsl[cpsli] = pslCompileLabels[label_psl_stackfrunge]; \
    cpsl[cpsli+1] = (void *) ssb; \
    cpsli += 2; \
    cpsl[cpsli] = pslCompileLabels[label_psl_popthis]; \
 \
    /* finally, we now leak everything */ \
    LEAKALL; \
} \
}
#undef INLINE_PSL
#define INLINE_PSL(op) {}


#endif
