/*
 * Memory abstraction
 *
 * Copyright (C) 2009, 2010 Gregor Richards
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

#ifndef PLOF_MEMORY_H
#define PLOF_MEMORY_H

#include "plof/plof.h"

/* Stack of objects, used only in interpPSL */
struct PSLStack {
    size_t length;
    struct PlofObject **data;
};

/* Allocate a PlofObject */
struct PlofObject *newPlofObject();

/* Free a PlofObject (optional) */
void freePlofObject(struct PlofObject *tofree);

/* Allocate a PSLStack */
struct PSLStack newPSLStack();

/* Resize a PSLStack */
struct PSLStack reallocPSLStack(struct PSLStack stack);

/* Free a PSLStack */
void freePSLStack(struct PSLStack stack);

/* Allocate a PlofRawData */
struct PlofRawData *newPlofRawData(size_t length);

/* Allocate a PlofRawData with non-atomic data */
struct PlofRawData *newPlofRawDataNonAtomic(size_t length);

/* Allocate a PlofArrayData */
struct PlofArrayData *newPlofArrayData(size_t length);

/* Allocate a PlofLocalsData */
struct PlofArrayData *newPlofLocalsData(size_t length);

/* Allocate objects with data inline */
struct PlofObject *newPlofObjectWithRaw(size_t length);
struct PlofObject *newPlofObjectWithArray(size_t length);

/* Free a PlofData (either kind) */
void freePlofData(struct PlofData *obj);

#endif
