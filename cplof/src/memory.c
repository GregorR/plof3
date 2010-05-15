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

#include "plof/memory.h"

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
        ret = GC_NEW(struct PlofObject);
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

/* Allocate a PlofRawData */
struct PlofRawData *newPlofRawData(size_t length)
{
    struct PlofRawData *rd;
    rd = GC_NEW(struct PlofRawData);
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
    rd = GC_NEW(struct PlofRawData);
    rd->type = PLOF_DATA_RAW;
    rd->length = length;
    rd->data = GC_MALLOC(length);
    return rd;
}

/* Allocate a PlofArrayData */
struct PlofArrayData *newPlofArrayData(size_t length)
{
    struct PlofArrayData *ad;
    ad = (struct PlofArrayData *) GC_MALLOC(sizeof(struct PlofArrayData) +
                                            length * sizeof(struct PlofObject *));
    ad->type = PLOF_DATA_ARRAY;
    ad->length = length;
    ad->data = (struct PlofObject **) (ad + 1);
    return ad;
}

/* Allocate a PlofLocalsData */
struct PlofArrayData *newPlofLocalsData(size_t length)
{
    size_t i;
    struct PlofArrayData *ad = newPlofArrayData(length);
    ad->type = PLOF_DATA_LOCALS;
    for (i = 0; i < length; i++) {
        ad->data[i] = plofNull;
    }
    return ad;
}

/* Allocate an objects with raw data inline */
struct PlofObject *newPlofObjectWithRaw(size_t length)
{
    struct PlofObject *obj;
    struct PlofRawData *rd;

    /* allocate them */
    obj = newPlofObject();
    rd = newPlofRawData(length);
    obj->data = (struct PlofData *) rd;

    return obj;
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
