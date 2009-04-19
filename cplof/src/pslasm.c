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

#include <stdio.h>
#include <string.h>

#include "lex.h"
#include "plof.h"
#include "psl.h"

#define BUFSTEP 1024
#define TOKBUF 256

int main(int argc, char **argv)
{
    FILE *pslf;
    unsigned char *apsl;
    size_t len, rd, i;
    size_t slen, stype, stl;
    struct PlofObject *context;
    struct PlofReturn ret;
    unsigned char token[TOKBUF];

    GC_INIT();

    if (argc != 2) {
        fprintf(stderr, "Use: pslasm <file>\n");
        return 1;
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

    /* and tokenize */
    while (pslLex(&apsl, token, TOKBUF)) {
        printf("%s\n", token);
    }

    return 0;
}
