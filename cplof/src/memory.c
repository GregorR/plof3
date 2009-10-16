/*
 * Memory abstraction
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

#include <stdlib.h>
#include <string.h>

#include "memory.h"

static struct PlofObject *plofObjectFreeList = NULL;

/* Allocate a PlofObject */
struct PlofObject *newPlofObject()
{
    struct PlofObject *ret;
    if (plofObjectFreeList) {
        ret = plofObjectFreeList;
        plofObjectFreeList = plofObjectFreeList->parent;
        ret->parent = NULL;
    } else {
        ret = GC_NEW_Z(struct PlofObject);
    }
    return ret;
}

/* Free a PlofObject (optional) */
void freePlofObject(struct PlofObject *tofree)
{
    memset(tofree, 0, sizeof(struct PlofObject));
    tofree->parent = plofObjectFreeList;
    plofObjectFreeList = tofree;
}

/* PSLStack freelist */
static struct PSLStack *pslStackFreeList = NULL;
static size_t pslStackFreeLen = 0;
static size_t pslStackFreeCur = 0;

/* Allocate a PSLStack */
struct PSLStack newPSLStack()
{
    struct PSLStack ret;

    /* do we have one free? */
    if (pslStackFreeCur > 0 &&
        pslStackFreeCur < pslStackFreeLen) {
        ret = pslStackFreeList[--pslStackFreeCur];

    } else {
        ret.length = 8;
        ret.data = GC_MALLOC(8 * sizeof(struct PlofObject *));
        memset(ret.data, 0, 8 * sizeof(struct PlofObject *));

    }

    return ret;
}

/* Resize a PSLStack */
struct PSLStack reallocPSLStack(struct PSLStack stack)
{
    /* just make a new one then free the old one */
    struct PSLStack ret;

    ret.length = stack.length * 2;
    ret.data = GC_MALLOC(ret.length * sizeof(struct PlofObject *));
    memcpy(ret.data, stack.data, stack.length * sizeof(struct PlofObject *));

    freePSLStack(stack);
    return ret;
}

/* Free a PSLStack */
void freePSLStack(struct PSLStack stack)
{
    if (pslStackFreeCur >= pslStackFreeLen) {
        /* our freelist needs more room */
        if (pslStackFreeLen == 0)
            pslStackFreeLen = 8;
        else
            pslStackFreeLen *= 2;
        pslStackFreeList = GC_REALLOC(pslStackFreeList, pslStackFreeLen * sizeof(struct PSLStack));
    }

    /* clear out the stack's memory */
    memset(stack.data, 0, stack.length * sizeof(struct PlofObject *));

    /* then add it to the freelist */
    pslStackFreeList[pslStackFreeCur++] = stack;
}

/* Allocate a PlofRawData */
struct PlofRawData *newPlofRawData(size_t length)
{
    struct PlofRawData *rd;
    rd = GC_NEW_Z(struct PlofRawData);
    rd->type = PLOF_DATA_RAW;
    rd->length = length;
    rd->data = GC_MALLOC_ATOMIC(length + 1);
    memset(rd->data, 0, length + 1);
    return rd;
}

/* Allocate a PlofRawData with non-atomic data */
struct PlofRawData *newPlofRawDataNonAtomic(size_t length)
{
    struct PlofRawData *rd;
    rd = GC_NEW_Z(struct PlofRawData);
    rd->type = PLOF_DATA_RAW;
    rd->length = length;
    rd->data = GC_MALLOC(length);
    memset(rd->data, 0, length);
    return rd;
}

/* Allocate a PlofArrayData */
struct PlofArrayData *newPlofArrayData(size_t length)
{
    struct PlofArrayData *ad;
    ad = (struct PlofArrayData *) GC_MALLOC(sizeof(struct PlofArrayData) +
                                            length * sizeof(struct PlofObject *));
    memset(ad, 0, sizeof(struct PlofArrayData) + length * sizeof(struct PlofObject *));
    ad->type = PLOF_DATA_ARRAY;
    ad->length = length;
    ad->data = (struct PlofObject **) (ad + 1);
    return ad;
}

/* Allocate an objects with raw data inline */
struct PlofObject *newPlofObjectWithRaw(size_t length)
{
    return NULL;
}

/* Allocate an objects with array data inline */
struct PlofObject *newPlofObjectWithArray(size_t length)
{
    struct PlofObject *obj;
    struct PlofArrayData *ad;
    size_t sz;

    /* allocate them */
    sz = sizeof(struct PlofObject) +
         sizeof(struct PlofArrayData) +
         length * sizeof(struct PlofObject *);
    obj = (struct PlofObject *) GC_MALLOC(sz);
    memset(obj, 0, sz);
    ad = (struct PlofArrayData *) (obj + 1);

    /* set up the object */
    obj->data = (struct PlofData *) ad;

    /* set up the array */
    ad->type = PLOF_DATA_ARRAY;
    ad->length = length;
    ad->data = (struct PlofObject **) (ad + 1);

    return obj;
}

/* Free a PlofData (either kind) */
void freePlofData(struct PlofData *obj) {}
