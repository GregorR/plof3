/*
 * Copyright (c) 2007, 2008, 2009 gregor richards
 * 
 * permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "software"), to deal
 * in the software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the software, and to permit persons to whom the software is
 * furnished to do so, subject to the following conditions:
 * 
 * the above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the software.
 * 
 * the software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and noninfringement. in no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising from,
 * out of or in connection with the software or the use or other dealings in
 * the software.
 */

#include <stdio.h>
#include <string.h>

#include "plof.h"
#include "psl.h"

#define BUFSTEP 1024

int main(int argc, char **argv)
{
    FILE *pslf;
    unsigned char *psl;
    size_t len, rd, i;
    size_t slen, stype, stl;
    struct PlofObject *context;
    struct PlofReturn ret;

    GC_INIT();

    if (argc != 2) {
        fprintf(stderr, "Use: psli <file>\n");
        return 1;
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
    plofNull = GC_NEW_Z(struct PlofObject);
    plofNull->parent = plofNull;
    plofGlobal = GC_NEW_Z(struct PlofObject);
    plofGlobal->parent = plofGlobal;

    /* And the context */
    context = GC_NEW_Z(struct PlofObject);
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
