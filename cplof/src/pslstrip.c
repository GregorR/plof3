/*
 * PSL stripper
 *
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

#include <stdio.h>
#include <string.h>

#include <gc/gc.h>

#include "plof/bignum.h"
#include "plof/plof.h"
#include "plof/pslfile.h"

#define BUFSTEP 1024

int main(int argc, char **argv)
{
    char *ifile, *ofile;
    FILE *pslf;
    struct Buffer_psl pslin, psl;

    GC_INIT();

    if (argc < 2) {
        fprintf(stderr, "Use: pslstrip <file> [output file]\n");
        return 1;
    }

    ifile = argv[1];
    if (argc >= 3) {
        ofile = argv[2];
    } else {
        ofile = argv[1];
    }

    /* open the file */
    pslf = fopen(ifile, "rb");
    if (pslf == NULL) {
        perror(ifile);
        return 1;
    }

    /* preallocate the buffer */
    INIT_BUFFER(pslin);

    /* now read it */
    READ_FILE_BUFFER(pslin, pslf);
    fclose(pslf);

    /* read it as PSL */
    psl = readPSLFile(pslin.bufused, pslin.buf);

    /* then write it out */
    pslf = fopen(ofile, "wb");
    if (pslf == NULL) {
        perror(ofile);
        return 1;
    }

    writePSLFile(pslf, psl.bufused, psl.buf, 1);
    fclose(pslf);

    return 0;
}
