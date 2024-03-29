/*
 * Ratpack parser wrappers
 *
 * Copyright (C) 2009 Josiah Worcester
 * Copyright (C) 2010 Gregor Richards
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

#ifndef PLOF_PRP_H
#define PLOF_PRP_H

#define BUFFER_GC
#include "plof/buffer.h"
#include "plof/plof.h"

/* Parsing returns Buffer_psl (the resultant code), and a pointer to the remainder of the code */
struct PRPResult {
    struct ParseContext *ctx;
    struct Buffer_psl code;
    unsigned char *remainder;
    unsigned int rline, rcol;
};

/* These correspond directly to underlying PSL instructions */
void gadd(unsigned char *name, unsigned char **target,
          size_t prepsllen, unsigned char *prepsl,
          size_t postpsllen, unsigned char *postpsl);
void grem(unsigned char *name);
void gcommit(void);

/* Parse some part of PSL code */
struct PRPResult parseOne(unsigned char *code, unsigned char *top, unsigned char *file,
                          unsigned int line, unsigned int column);

/* Parse the entirety of PSL code. Note that this will interpret immediates, whereas parseOne will not. */
struct Buffer_psl parseAll(unsigned char *code, unsigned char *top, unsigned char *file);

/* Should PRP produce debugging data? */
extern int prpDebug;

#endif
