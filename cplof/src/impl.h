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
    if (stacktop == stack.length) { \
        stack = reallocPSLStack(stack); \
    } \
    stack.data[stacktop] = (val); \
    stacktop++; \
}
#define STACK_POP(into) \
{ \
    if (stacktop == 0) { \
        into = plofNull; \
    } else { \
        into = stack.data[--stacktop]; \
    } \
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
    plofWrite(ret.ret, (unsigned char *) PSL_EXCEPTION_STACK, a); \
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
#define RAW(obj) ((struct PlofRawData *) (obj)->data)
#define ARRAY(obj) ((struct PlofArrayData *) (obj)->data)
#define RAWSTRDUP(type, into, _rd) \
{ \
    unsigned char *_into = (unsigned char *) GC_MALLOC_ATOMIC((_rd)->length + 1); \
    memcpy(_into, (_rd)->data, (_rd)->length); \
    _into[(_rd)->length] = '\0'; \
    (into) = (type *) _into; \
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

#endif
