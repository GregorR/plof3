/*
 * PSL assembler frontend (APSL->PSL transformer)
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

#include <stdio.h>
#include <string.h>

#include <gc/gc.h>

#include "plof/bignum.h"
#include "parse.h"
#include "plof/plof.h"
#include "plof/pslfile.h"

#define BUFSTEP 1024

int main(int argc, char **argv)
{
    FILE *pslf;
    unsigned char *apsl;
    char *ofname;
    size_t len, rd;
    struct UCharBuf parsed;

    GC_INIT();

    if (argc < 2) {
        fprintf(stderr, "Use: pslasm <file> [output file]\n");
        return 1;
    }

    /* check if an output filename was provided */
    if (argc >= 3) {
        ofname = argv[2];
    } else {
        size_t osl;
        
        ofname = GC_STRDUP(argv[1]);
        osl = strlen(ofname);

        if (osl < 5 || strcmp(ofname + osl - 5, ".apsl")) {
            /* couldn't guess an output name */
            fprintf(stderr, "Couldn't guess an output filename from the input name %s\n", argv[1]);
            return 1;
        }

        strcpy(ofname + osl - 4, "psl");
    }

    /* open the file */
    pslf = fopen(argv[1], "r");
    if (pslf == NULL) {
        perror(argv[1]);
        return 1;
    }

    /* preallocate the buffer */
    apsl = GC_MALLOC_ATOMIC(BUFSTEP);
    len = 0;

    /* now start reading */
    while ((rd = fread(apsl + len, 1, BUFSTEP, pslf))) {
        len += rd;
        if (rd < BUFSTEP) break;

        /* allocate more */
        apsl = GC_REALLOC(apsl, len + BUFSTEP);
    }
    fclose(pslf);

    /* parse */
    parsed = pslParse(&apsl);

    /* then write it out */
    pslf = fopen(ofname, "wb");
    if (pslf == NULL) {
        perror(ofname);
        return 1;
    }

    writePSLFile(pslf, parsed.len, parsed.ptr, 0);

    fclose(pslf);

    return 0;
}
