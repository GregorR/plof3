/*
 * PSL interpreter
 *
 * Copyright (C) 2007, 2008, 2009 Gregor Richards
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

#if defined(DEBUG_TIMING) || defined(DEBUG_TIMING_PROCEDURE)
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
#include "impl.h"
#include "intrinsics.h"
#include "jump.h"
#include "leaky.h"
#include "memory.h"
#include "optimizations.h"
#include "plof.h"
#include "psl.h"
#include "pslfile.h"
#ifndef PLOF_NO_PARSER
#include "prp.h"
#endif



/* Include paths */
unsigned char **plofIncludePaths;

/* Use intrinsics? */
int plofLoadIntrinsics = 1;


/* The maximum number of version strings */
#define VERSION_MAX 100


/* Some versions of 'jump' require an enum */
#ifdef jumpenum
enum jumplabel {
    interp_psl_nop,
#define FOREACH(inst) interp_psl_ ## inst,
#include "psl_inst.h"
#undef FOREACH
    interp_psl_deletea,
    interp_psl_deleteb,
    interp_psl_deletec,
    interp_psl_deleted,
    interp_psl_deletee
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
    size_t stacktop;
    struct PSLStack stack;

    /* Slots for n-ary ops */
    struct PlofObject *a, *b, *c, *d, *e;

    struct PlofRawData *rd;
    struct PlofArrayData *ad;

    /* The PSL in various forms */
    size_t psllen;
    unsigned char *psl = NULL;
    void **cpsl = NULL;
    void **pc = NULL;
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
#ifdef DEBUG_TIMING_PROCEDURE
    struct timespec pstspec, petspec;
#endif

    a = b = c = d = e = NULL;

#ifdef DEBUG_TIMING_PROCEDURE
    clock_gettime(CLOCK_MONOTONIC, &pstspec);
#endif

    /* Get out the PSL */
    if (pslraw) {
        rd = (struct PlofRawData *) pslraw->data;
        psllen = rd->length;
        psl = rd->data;
        cpsl = rd->idata;
    } else {
        psllen = pslaltlen;
        psl = pslalt;
    }

    /* call the intrinsic if applicable */
    if (pslraw && rd->proc) {
        ret = rd->proc(context, arg);
#ifdef DEBUG_TIMING_PROCEDURE
        clock_gettime(CLOCK_MONOTONIC, &petspec);
#ifdef DEBUG_NAMES
        if (pslraw && pslraw->name) {
            printf("\"%s\", ", (char *) pslraw->name);
        } else
#endif
        printf("\"anonymous\", ");
        printf("%lld\n",
               (petspec.tv_sec - pstspec.tv_sec) * 1000000000LL +
               (petspec.tv_nsec - pstspec.tv_nsec));
#endif
        return ret;
    }

    /* Perhaps generate the context */
    if (generateContext) {
        a = newPlofObject();
        a->parent = context;
        context = a;
    }

    /* add +procedure */
    if (pslraw) {
        if (procedureHash == 0) {
            procedureHash = plofHash(10, (unsigned char *) "+procedure");
        }
        plofWrite(context, (unsigned char *) "+procedure", procedureHash, pslraw);
    }

    /* Start the stack */
    stack = newPSLStack();
    stacktop = 0;
    if (arg) {
        stack.data[0] = arg;
        stacktop = 1;
    }

    /* Make sure it's compiled */
    if (!cpsl) {
        int psli, cpsli;

        /* our stack of leakies for determining what can be freed immediately */
        struct Leaky *lstack;
        int lstacklen, lstackcur;

        /* start with 8 slots */
        int cpsllen = 8;

        /* allocate our stuff */
        lstacklen = 8;
        lstackcur = 0;
        lstack = GC_MALLOC_ATOMIC(lstacklen * sizeof(struct Leaky));
        cpsl = GC_MALLOC(cpsllen * sizeof(void*));

        /* now go through the PSL and translate it into compiled PSL */
        for (psli = 0, cpsli = 0;
             psli < psllen;
             psli++, cpsli += 2) {
            unsigned char cmd = psl[psli];
            struct PlofRawData *raw;

            /* make sure cpsl is big enough */
            while (cpsli >= cpsllen - 12) {
                cpsllen *= 2;
                cpsl = GC_REALLOC(cpsl, cpsllen * sizeof(void*));
            }

            /* maybe it has raw data */
            if (cmd >= psl_marker) {
                size_t len;

                psli++;
                psli += pslBignumToInt(psl + psli, (size_t *) &len);

                /* make sure this doesn't go off the edge */
                if (psli + len > psllen) {
                    fprintf(stderr, "Bad data in PSL!\n");
                    len = psllen - psli;
                }

                /* copy it in */
                raw = newPlofRawData(len);
                memcpy(raw->data, psl + psli, len);
                psli += len - 1;

                cpsl[cpsli + 1] = raw;
            } else {
                cpsl[cpsli + 1] = NULL;
            }

            /* either get only immediates, or everything else */
            cpsl[cpsli] = addressof(interp_psl_nop);
            if (immediate) {
                if (cmd == psl_immediate) {
                    cpsl[cpsli] = addressof(interp_psl_immediate);
                }

            } else {
                int arity = 0;
                int pushes = 0;
                int leaka, leakb, leakc, leakd, leake, leakp;
                int ari, pushi;
                leaka = leakb = leakc = leakd = leake = leakp = 0;

#define LEAKY_PUSH(depth) \
                { \
                    while (lstackcur >= lstacklen) { \
                        lstacklen *= 2; \
                        lstack = GC_REALLOC(lstack, lstacklen * sizeof(struct Leaky)); \
                    } \
                    if (lstackcur > depth) { \
                        lstack[lstackcur].dup = lstack + lstackcur - 1 - depth; \
                        lstackcur++; \
                    } else { \
                        lstack[lstackcur].dup = NULL; \
                        lstack[lstackcur].leaks = 1; \
                        lstackcur++; \
                    } \
                }

                switch (cmd) {
#include "psl-cpsl.c"

                    default:
                        fprintf(stderr, "Invalid operation: 0x%x\n", cmd);
                }

                /* pop the things it popped */
                if (arity) {
#define MARKLEAK(depth) \
                    { \
                        int _depth = (depth); \
                        if (depth > 0 && lstackcur >= _depth) { \
                            struct Leaky *curl = lstack + lstackcur - _depth; \
                            curl->leaks = 1; \
                        } \
                    }

                    /* first mark leaks */
                    if (leaka) { MARKLEAK(arity); }
                    if (leakb) { MARKLEAK(arity - 1); }
                    if (leakc) { MARKLEAK(arity - 2); }
                    if (leakd) { MARKLEAK(arity - 3); }
                    if (leake) { MARKLEAK(arity - 4); }
#undef MARKLEAK

                    /* now delete nonleaks */
                    for (ari = 0; ari < arity; ari++) {
                        int lsi = lstackcur - arity + ari;
                        struct Leaky *lcur;

                        if (lsi < 0) continue;

                        lcur = lstack + lsi;
                        if (!lcur->dup && !lcur->leaks) {
                            switch (ari) {
                                case 0: cpsl[cpsli += 2] = addressof(interp_psl_deletea); break;
                                case 1: cpsl[cpsli += 2] = addressof(interp_psl_deleteb); break;
                                case 2: cpsl[cpsli += 2] = addressof(interp_psl_deletec); break;
                                case 3: cpsl[cpsli += 2] = addressof(interp_psl_deleted); break;
                                case 4: cpsl[cpsli += 2] = addressof(interp_psl_deletee); break;
                            }
                            cpsl[cpsli+1] = NULL;
                        }
                    }

                    /* and update our stack */
                    lstackcur -= arity;
                    if (lstackcur < 0) lstackcur = 0;
                }

                /* now push whatever it pushes */
                if (pushes) {
                    while (lstackcur + pushes > lstacklen) {
                        lstacklen *= 2;
                        lstack = GC_REALLOC(lstack, lstacklen * sizeof(struct Leaky));
                    }
    
                    for (pushi = 0; pushi < pushes; pushi++) {
                        lstack[lstackcur].dup = NULL;
                        lstack[lstackcur].leaks = leakp;
                        lstackcur++;
                    }
                }
            }
        }

        /* now close off the end */
        cpsl[cpsli] = addressof(interp_psl_return);
        cpsl[cpsli+1] = NULL;

        GC_FREE(lstack);

        /* and save it */
        if (pslraw && !immediate) {
            ((struct PlofRawData *) pslraw->data)->idata = cpsl;
        }
    }

    /* ACTUAL INTERPRETER BEYOND HERE */
    pc = cpsl;
    prejump(*pc);

    jumphead;
#include "impl/nop.c"
#include "psl-impl.c"
#include "impl/delete.c"
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

/* Combine two objects */
struct PlofObject *plofCombine(struct PlofObject *a, struct PlofObject *b)
{
    struct PlofObject *newo;
    struct PlofRawData *rd;
    struct PlofArrayData *ad;

    /* start making the new object */
    newo = newPlofObject();
    newo->parent = b->parent;

    /* duplicate the left object */
    plofObjCopy(newo, a);

    /* then the right */
    plofObjCopy(newo, b);

    /* now get any data */
    if (ISRAW(a)) {
        if (ISRAW(b)) {
            struct PlofRawData *ra, *rb;
            ra = RAW(a);
            rb = RAW(b);

            rd = newPlofRawData(ra->length + rb->length);

            /* copy in both */
            memcpy(rd->data, ra->data, ra->length);
            memcpy(rd->data + ra->length, rb->data, rb->length);

            newo->data = (struct PlofData *) rd;

        } else {
            /* just the left */
            newo->data = a->data;

        }

    } else if (ISARRAY(a)) {
        if (ISRAW(b)) {
            /* just the right */
            newo->data = b->data;

        } else if (ISARRAY(b)) {
            /* combine the arrays */
            struct PlofArrayData *aa, *ab;
            aa = ARRAY(a);
            ab = ARRAY(b);

            ad = newPlofArrayData(aa->length + ab->length);

            /* copy in both */
            memcpy(ad->data, aa->data, aa->length * sizeof(struct PlofObject *));
            memcpy(ad->data + aa->length, ab->data, ab->length * sizeof(struct PlofObject *));

            newo->data = (struct PlofData *) ad;

        } else {
            /* duplicate the left array */
            struct PlofArrayData *aa = ARRAY(a);
            ad = newPlofArrayData(aa->length);
            memcpy(ad->data, aa->data, ad->length * sizeof(struct PlofObject *));
            newo->data = (struct PlofData *) ad;

        }

    } else {
        if (ISRAW(b)) {
            newo->data = b->data;

        } else if (ISARRAY(b)) {
            /* duplicate the right array */
            struct PlofArrayData *ab = ARRAY(b);
            ad = newPlofArrayData(ab->length);
            ad->type = PLOF_DATA_ARRAY;
            memcpy(ad->data, ab->data, ad->length * sizeof(struct PlofObject *));
            newo->data = (struct PlofData *) ad;

        }

    }

    return newo;
}

/* Copy the content of a PlofOHashTable into an object */
void plofObjCopyPrime(struct PlofObject *to, struct PlofOHashTable *from)
{
    if (from == NULL || from->name == NULL) return;

    plofWrite(to, from->name, from->hashedName, from->value);
    plofObjCopyPrime(to, from->next);
}

/* Copy the content of one object into another */
void plofObjCopy(struct PlofObject *to, struct PlofObject *from)
{
    int i;

    if (from->hashTable) {
        struct PlofOHashTables *ht = from->hashTable;
        for (i = 0; i < PLOF_HASHTABLE_SIZE; i++) {
            plofObjCopyPrime(to, &ht->elems[i]);
        }
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

    if (of == NULL || of->name == NULL) {
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
    rd = newPlofRawData(strlen((char *) of->name));
    memcpy(rd->data, of->name, rd->length);
    obj = newPlofObject();
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
    size_t len;

    /* get out the members */
    if (of->hashTable) {
        struct PlofOHashTables *ht = of->hashTable;
        for (i = 0; i < PLOF_HASHTABLE_SIZE; i++) {
            eachobjs[i] = plofMembersSub(&ht->elems[i]);
        }
    } else {
        memset(eachobjs, 0, PLOF_HASHTABLE_SIZE * sizeof(struct PlofObjects));
    }

    /* and combine them into the output */
    len = 0;
    for (i = 0; i < PLOF_HASHTABLE_SIZE; i++) len += eachobjs[i].length;
    ad = newPlofArrayData(len);

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
    struct PlofRawData *ret, *wr;
    struct Buffer_psl pslBuf;
    int i;

    /* preallocate some space */
    INIT_BUFFER(pslBuf);

    /* go through the commands in 'in' ... */
    for (i = 0; i < in->length; i++) {
        unsigned char cmd = in->data[i];
        struct PlofRawData *data = NULL;

        /* get out the data */
        if (cmd >= psl_marker) {
            size_t len;

            i++;
            i += pslBignumToInt(in->data + i, (size_t *) &len);
            if (i + len > in->length) {
                len = in->length - i;
            }

            data = newPlofRawData(len);
            memcpy(data->data, in->data + i, len);
            i += len - 1;
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

                /* copy it in */
                WRITE_BUFFER(pslBuf, wr->data, wr->length);
            }

        } else if (cmd == psl_code) {
            size_t bignumlen;

            /* recurse */
            struct PlofRawData *sub = pslReplace(data, with);

            /* figure out the bignum length */
            bignumlen = pslBignumLength(sub->length);

            /* then copy it all in */
            WRITE_BUFFER(pslBuf, &cmd, 1);
            while (BUFFER_SPACE(pslBuf) < bignumlen) EXPAND_BUFFER(pslBuf);
            pslIntToBignum(pslBuf.buf + pslBuf.bufused, sub->length, bignumlen);
            pslBuf.bufused += bignumlen;
            WRITE_BUFFER(pslBuf, sub->data, sub->length);

        } else if (cmd > psl_marker) {
            /* rewrite it */
            size_t bignumlen = pslBignumLength(data->length);
            WRITE_BUFFER(pslBuf, &cmd, 1);
            while (BUFFER_SPACE(pslBuf) < bignumlen) EXPAND_BUFFER(pslBuf);
            pslIntToBignum(pslBuf.buf + pslBuf.bufused, data->length, bignumlen);
            pslBuf.bufused += bignumlen;
            WRITE_BUFFER(pslBuf, data->data, data->length);

        } else {
            /* just write out the command */
            WRITE_BUFFER(pslBuf, &cmd, 1);

        }
    }

    ret = newPlofRawData(pslBuf.bufused);
    memcpy(ret->data, pslBuf.buf, pslBuf.bufused);
    return ret;
}

/* Function for getting a value from the hash table in an object */
struct PlofObject *plofRead(struct PlofObject *obj, unsigned char *name, size_t namehash)
{
    struct PlofObject *res = plofNull;
    struct PlofOHashTable *cur;
    if (!obj->hashTable) return plofNull;

    cur = &obj->hashTable->elems[namehash & PLOF_HASHTABLE_MASK];
    while (cur && cur->name) {
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
void plofWrite(struct PlofObject *obj, unsigned char *name, size_t namehash, struct PlofObject *value)
{
    struct PlofOHashTable *cur;
    struct PlofOHashTables *ht;
    size_t subhash = namehash & PLOF_HASHTABLE_MASK;

#ifdef DEBUG_NAMES
    /* note that these heuristics are not meant to be general, they're just
     * handy for Plof */
    if (name[0] != '+' && strcmp(name, "this") && strcmp(name, "__pul_fc") &&
        strcmp(name, "__pul_v") &&
        ISOBJ(value) && value->name == NULL) {
        value->name = name;
    }
#endif

    ht = obj->hashTable;
    if (ht == NULL) {
        obj->hashTable = ht = GC_NEW_Z(struct PlofOHashTables);
    }
    if (ht->elems[subhash].name == NULL) {
        plofHashtableNew(&ht->elems[subhash], name, namehash, value);
       
    } else {
        cur = &ht->elems[subhash];
        while (cur) {
            if (namehash == cur->hashedName) {
                cur->value = value; /* FIXME, collisions */
                cur = NULL;

            } else {
                if (cur->next) {
                    cur = cur->next;
                } else {
                    cur->next = plofHashtableNew(NULL, name, namehash, value);
                    cur = NULL;
                }
               
            }
        }
    }
}

/* Function for creating a new hashTable object */
struct PlofOHashTable *plofHashtableNew(struct PlofOHashTable *into, unsigned char *name, size_t namehash, struct PlofObject *value)
{
    struct PlofOHashTable *nht;
    if (into) {
        nht = into;
    } else {
        nht = GC_NEW_Z(struct PlofOHashTable);
    }
    nht->hashedName = namehash;

    nht->name = name; /* should always be safe maybe? */
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
