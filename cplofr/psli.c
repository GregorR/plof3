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

    GC_INIT();

    if (argc != 2) {
        fprintf(stderr, "Use: psli <file>\n");
        return 1;
    }

    /* open the file */
    pslf = fopen(argv[1], "r");
    if (pslf == NULL) {
        perror(argv[1]);
        return 1;
    }

    /* preallocate the buffer */
    psl = GC_MALLOC(BUFSTEP);
    len = 0;

    /* now start reading */
    while (rd = fread(psl + len, 1, BUFSTEP, pslf)) {
        len += rd;
        if (rd < BUFSTEP) break;

        /* allocate more */
        psl = GC_REALLOC(psl, len + BUFSTEP);
    }

    /* FIXME: bounds checking */

    /* Go section-by-section */
    for (i = 8; i < len;) {
        /* section length */
        i += pslBignumToInt(psl + i, &slen);

        /* section type */
        stl = pslBignumToInt(psl + i, &stype);

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
    interpretPSL(context, plofNull, NULL, slen - stl, psl + i, 0);
}
