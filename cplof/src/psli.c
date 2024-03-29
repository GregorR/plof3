/*
 * psli, the frontend to the PSL interpreter for simple PSL (non-Plof) files
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
#include <string.h>

#include "plof/bignum.h"
#include "plof/memory.h"
#include "plof/plof.h"
#include "plof/psl.h"
#include "whereami.h"

#define BUFSTEP 1024

int main(int argc, char **argv)
{
    FILE *pslf;
    unsigned char *psl;
    size_t len, rd, i;
    size_t slen, stype, stl;
    struct PlofObject *context;
    struct PlofReturn ret;
    char *wdir, *wfil;

    GC_INIT();

    if (argc != 2) {
        fprintf(stderr, "Use: psli <file>\n");
        return 1;
    }

    /* get our search path */
    if (whereAmI(argv[0], &wdir, &wfil)) {
        plofIncludePaths = GC_MALLOC(3 * sizeof(unsigned char *));

        /* /../share/plof_include/ */
        plofIncludePaths[0] = GC_MALLOC_ATOMIC(strlen(wdir) + 24);
        sprintf((char *) plofIncludePaths[0], "%s/../share/plof_include/", wdir);

        /* /../../plof_include/ (for running from src/) */
        plofIncludePaths[1] = GC_MALLOC_ATOMIC(strlen(wdir) + 21);
        sprintf((char *) plofIncludePaths[1], "%s/../../plof_include/", wdir);

        plofIncludePaths[2] = NULL;

        /* FIXME: should support -I eventually */

    } else {
        plofIncludePaths[0] = NULL;

    }

    /* open the file */
    pslf = fopen(argv[1], "rb");
    if (pslf == NULL) {
        perror(argv[1]);
        return 1;
    }

    /* preallocate the buffer */
    psl = GC_MALLOC_ATOMIC(BUFSTEP);
    len = 0;

    /* now start reading */
    while ((rd = fread(psl + len, 1, BUFSTEP, pslf))) {
        len += rd;
        if (rd < BUFSTEP) break;

        /* allocate more */
        psl = GC_REALLOC(psl, len + BUFSTEP);
    }
    fclose(pslf);

    /* FIXME: bounds checking */

    /* Go section-by-section */
    for (i = 8; i < len;) {
        /* section length */
        i += pslBignumToInt(psl + i, (size_t *) &slen);

        /* section type */
        stl = pslBignumToInt(psl + i, (size_t *) &stype);

        /* if it's program data, we found it */
        if (stype == 0) {
            i += stl;
            break;

        } else {
            /* skip this section */
            i += slen;
        }
    }

    /* make sure we found it */
    if (i >= len) {
        fprintf(stderr, "No program data!\n");
        return 1;
    }

    /* Initialize null and global */
    plofNull = newPlofObject();
    plofNull->parent = plofNull;
    plofGlobal = newPlofObject();
    plofGlobal->parent = plofGlobal;

    /* And the context */
    context = newPlofObject();
    context->parent = plofNull;

    /* Now interp */
    interpretPSL(context, plofNull, NULL, slen - stl, psl + i, 0, 1);
    ret = interpretPSL(context, plofNull, NULL, slen - stl, psl + i, 0, 0);

    if (ret.isThrown) {
        fprintf(stderr, "PSL threw up!\n");
        return 1;
    }

    return 0;
}
