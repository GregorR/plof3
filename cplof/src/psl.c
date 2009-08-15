/*
 * PSL interpreter
 *
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

#ifdef DEBUG_TIMING
#include <time.h>
#endif

/* For CNFI */
#ifdef WITH_CNFI
#include <dlfcn.h>
#include <ffi.h>

/* a cif with some extra info */
typedef struct _ffi_cif_plus {
    ffi_cif cif;

    /* since cifs seem to lose the types sometimes */
    ffi_type *rtype;
    ffi_type **atypes;
} ffi_cif_plus;
#endif

/* For predefs */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "bignum.h"
#include "jump.h"
#include "plof.h"
#include "psl.h"
#include "pslfile.h"
#ifndef PLOF_NO_PARSER
#include "prp.h"
#endif



/* Include paths */
unsigned char **plofIncludePaths;


/* The maximum number of version strings */
#define VERSION_MAX 100


/* Some versions of 'jump' require an enum */
#ifdef jumpenum
enum jumplabel {
    interp_psl_nop,
#define FOREACH(inst) interp_psl_ ## inst,
#include "psl_inst.h"
#undef FOREACH
    interp_psl_done
};
#endif

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

    /* The current file/line/col (for debugging) */
    unsigned char *dfile = NULL;
    ptrdiff_t dline = -1, dcol = -1;

    /* timing info */
#ifdef DEBUG_TIMING
    char *tiName;
    struct timespec stspec, etspec;
#endif

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
        plofWrite(context, 10, (unsigned char *) "+procedure", procedureHash, pslraw);
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
                raw->data = (unsigned char *) GC_MALLOC_ATOMIC(raw->length + 1);
                memcpy(raw->data, psl + psli, raw->length);
                raw->data[raw->length] = '\0';
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
                    case psl_ ## inst: \
                        cpsl[cpsli] = addressof(interp_psl_ ## inst); \
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
#define BADTYPE(cmd) \
    { \
        fprintf(stderr, "Type error in " cmd); \
        if (dfile) { \
            fprintf(stderr, " at %s, line %d col %d", dfile, (int) dline+1, (int) dcol+1); \
        } \
        fprintf(stderr, "\n"); \
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

#elif defined(PLOF_FREE_INTS)
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
        printf("\"%s\",%d\n", \
               tiName, \
               (etspec.tv_sec - stspec.tv_sec) * 1000000000 + \
               (etspec.tv_nsec - stspec.tv_nsec)); \
        pc += 2; \
        jump(*pc); \
    }
#else
#define STEP pc += 2; jump(*pc)
#endif


    jumphead;
#include "impl/nop.c"
#include "psl-impl.c"
#include "impl/done.c"
    jumptail;
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

/* Copy the content of a PlofOHashTable into an object */
void plofObjCopyPrime(struct PlofObject *to, struct PlofOHashTable *from)
{
    if (from == NULL) return;

    plofWrite(to, from->namelen, from->name, from->hashedName, from->value);
    plofObjCopyPrime(to, from->next);
}

/* Copy the content of one object into another */
void plofObjCopy(struct PlofObject *to, struct PlofObject *from)
{
    int i;

    for (i = 0; i < PLOF_HASHTABLE_SIZE; i++) {
        plofObjCopyPrime(to, from->hashTable[i]);
    }
}


struct PlofObjects {
    size_t length;
    struct PlofObject **data;
};

/* Internal function used by plofMembers */
struct PlofObjects plofMembersSub(struct PlofOHashTable *of)
{
    struct PlofObjects next, ret;
    struct PlofObject *obj;
    struct PlofRawData *rd;

    if (of == NULL) {
        ret.length = 0;
        ret.data = NULL;
        return ret;
    }

    /* get the left and right members */
    next = plofMembersSub(of->next);

    /* prepare ours */
    ret.length = next.length + 1;
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
    ret.data[0] = obj;
    memcpy(ret.data + 1, next.data, next.length * sizeof(struct PlofObject *));

    /* FIXME: this is all horrendously inefficient for a list, but then again these lists are meant to be quite short */

    return ret;
}

/* Make an array of the list of members of an object */
struct PlofArrayData *plofMembers(struct PlofObject *of)
{
    struct PlofArrayData *ad;
    int i, off;
    struct PlofObjects eachobjs[PLOF_HASHTABLE_SIZE];

    /* get out the members */
    for (i = 0; i < PLOF_HASHTABLE_SIZE; i++) {
        eachobjs[i] = plofMembersSub(of->hashTable[i]);
    }

    /* and combine them into the output */
    ad = GC_NEW_Z(struct PlofArrayData);
    ad->type = PLOF_DATA_ARRAY;
    ad->length = 0;
    for (i = 0; i < PLOF_HASHTABLE_SIZE; i++) ad->length += eachobjs[i].length;
    ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));

    off = 0;
    for (i = 0; i < PLOF_HASHTABLE_SIZE; i++) {
        if (eachobjs[i].length) {
            memcpy(ad->data + off, eachobjs[i].data, eachobjs[i].length * sizeof(struct PlofObject *));
            off += eachobjs[i].length;
        }
    }

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
                mval = *data->data;
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

/* Function for getting a value from the hash table in an object */
struct PlofObject *plofRead(struct PlofObject *obj, size_t namelen, unsigned char *name, size_t namehash)
{
    struct PlofObject *res = plofNull;
    struct PlofOHashTable *cur = obj->hashTable[namehash & PLOF_HASHTABLE_MASK];
    while (cur) {
        if (namehash == cur->hashedName) {
            /* FIXME: collisions, name check */
            res = cur->value;
            cur = NULL;
        } else {
            cur = cur->next;
        }
    }
    return res;
}

/* Function for writing a value into an object */
void plofWrite(struct PlofObject *obj, size_t namelen, unsigned char *name, size_t namehash, struct PlofObject *value)
{
    struct PlofOHashTable *cur;
    size_t subhash = namehash & PLOF_HASHTABLE_MASK;
    if (obj->hashTable[subhash] == NULL) {
        obj->hashTable[subhash] = plofHashtableNew(namelen, name, namehash, value);
       
    } else {
        cur = obj->hashTable[subhash];
        while (cur) {
            if (namehash == cur->hashedName) {
                cur->value = value; /* FIXME, collisions */
                cur = NULL;

            } else {
                if (cur->next) {
                    cur = cur->next;
                } else {
                    cur->next = plofHashtableNew(namelen, name, namehash, value);
                    cur = NULL;
                }
               
            }
        }
    }
}

/* Function for creating a new hashTable object */
struct PlofOHashTable *plofHashtableNew(size_t namelen, unsigned char *name, size_t namehash, struct PlofObject *value)
{
    unsigned char *namedup;
    struct PlofOHashTable *nht = GC_NEW_Z(struct PlofOHashTable);
    nht->hashedName = namehash;
    nht->namelen = namelen;

    namedup = GC_MALLOC_ATOMIC(namelen + 1);
    memcpy(namedup, name, namelen);
    namedup[namelen] = '\0';

    nht->name = namedup;
    nht->value = value;
    return nht;
}


/* GC on DJGPP is screwy */
#ifdef __DJGPP__
void vsnprintf() {}
#endif

/* Null and global */
struct PlofObject *plofNull = NULL;
struct PlofObject *plofGlobal = NULL;
