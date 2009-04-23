/*
 * Copyright (c) 2009 Gregor Richards
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

#ifndef PLOFORTH_H
#define PLOFORTH_h

#include "buffer.h"
#include "dict.h"

BUFFER(voidp, void *);
BUFFER(voidpp, void **);

/* running state of a Ploforth program */
typedef struct _PloforthState PloforthState;
struct _PloforthState {
    /* are we compiling or interpreting? */
    unsigned char compiling;

    /* top-level code */
    unsigned char *code;

    /* call stack */
    Buffer_voidpp cstack;

    /* data stack */
    Buffer_voidp dstack;

    /* word currently being compiled (used by ":", will eventually be on the stack) */
    unsigned char *curword;

    /* target of compilation */
    Buffer_voidpp compbuf;

    /* dictionary */
    Dict *dict;
};

/* run Ploforth code */
PloforthState runPloforth(unsigned char *code, PloforthState *istate);

#endif
