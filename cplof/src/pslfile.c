/*
 * PSL file exporter/importer
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

#include <string.h>

#include "bignum.h"
#include "pslfile.h"

/* return true if this buffer points to a PSL file */
int isPSLFile(size_t sz, unsigned char *buf)
{
    if (sz >= sizeof(PSL_FILE_MAGIC) - 1 &&
        !memcmp(PSL_FILE_MAGIC, (char *) buf, sizeof(PSL_FILE_MAGIC) - 1)) {
        return 1;
    }
    return 0;
}

/* read in the PSL from a PSL file */
struct Buffer_psl readPSLFile(size_t sz, unsigned char *buf)
{
    struct Buffer_psl psl;
    size_t i, slen, stype, stl;

    memset(&psl, 0, sizeof(struct Buffer_psl));

    /* Go section-by-section */
    for (i = sizeof(PSL_FILE_MAGIC) - 1; i < sz;) {
        /* section length */
        i += pslBignumToInt(buf + i, (size_t *) &slen);

        /* section type */
        stl = pslBignumToInt(buf + i, (size_t *) &stype);

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
    if (i >= sz) {
        fprintf(stderr, "No program data!\n");
        return psl;
    }

    psl.buf = buf + i;
    psl.bufused = slen - stl;
    psl.bufsz = psl.bufused;

    return psl;
}


/* write out PSL to a file */
void writePSLFile(FILE *to, size_t sz, unsigned char *buf)
{
    size_t bnumsz;
    unsigned char *bnum;

    /* the header */
    fwrite(PSL_FILE_MAGIC, 1, sizeof(PSL_FILE_MAGIC)-1, to);

    /* the section header */
    bnumsz = pslBignumLength(sz + 1);
    bnum = GC_MALLOC_ATOMIC(bnumsz);
    pslIntToBignum(bnum, sz + 1, bnumsz);
    fwrite(bnum, 1, bnumsz, to);
    fwrite("\x00", 1, 1, to);

    /* and the content */
    fwrite(buf, 1, sz, to);
}
