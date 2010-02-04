/*
 * PSL file exporter/importer
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

#include <string.h>

#include "plof/bignum.h"
#include "plof/psl.h"
#include "plof/pslfile.h"

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
    size_t hasprgdata = 0, hassprgdata = 0, hasstrtab = 0;
    size_t prgdatalen = 0, sprgdatalen = 0, strtablen = 0;

    memset(&psl, 0, sizeof(struct Buffer_psl));

    /* Go section-by-section */
    for (i = sizeof(PSL_FILE_MAGIC) - 1; i < sz;) {
        /* section length */
        i += pslBignumToInt(buf + i, (size_t *) &slen);

        /* section type */
        stl = pslBignumToInt(buf + i, (size_t *) &stype);

        /* if it's program data, we found it */
        if (stype == PSL_SECTION_PROGRAM_DATA) {
            hasprgdata = i + stl;
            prgdatalen = slen - stl;

        } else if (stype == PSL_SECTION_STRIPPED_PROGRAM_DATA) {
            hassprgdata = i + stl;
            sprgdatalen = slen - stl;

        } else if (stype == PSL_SECTION_RAW_DATA_TABLE) {
            hasstrtab = i + stl;
            strtablen = slen - stl;

        }
        i += slen;
    }

    /* get the real data */
    if (hasprgdata) {
        /* it's raw, easy */
        psl.buf = buf + hasprgdata;
        psl.bufused = psl.bufsz = prgdatalen;

    } else if (hassprgdata && hasstrtab) {
        struct Buffer_psl psls, strtab;
        psls.buf = buf + hassprgdata;
        psls.bufused = psls.bufsz = sprgdatalen;
        strtab.buf = buf + hasstrtab;
        strtab.bufused = strtab.bufsz = strtablen;
        psl = unstripPSL(psls, strtab);

    } else {
        fprintf(stderr, "No program data!\n");

    }

    return psl;
}


/* write a PSL section to a file (currently only works with types <128) */
static void writePSLSection(FILE *to, int type, struct Buffer_psl sect)
{
    size_t bnumsz;
    unsigned char *bnum;

    /* the section header */
    bnumsz = pslBignumLength(sect.bufused + 1);
    bnum = GC_MALLOC_ATOMIC(bnumsz);
    pslIntToBignum(bnum, sect.bufused + 1, bnumsz);
    fwrite(bnum, 1, bnumsz, to);
    fputc(type, to);

    /* and the content */
    fwrite(sect.buf, 1, sect.bufused, to);
}

/* write out PSL to a file */
void writePSLFile(FILE *to, size_t sz, unsigned char *buf)
{
    struct Buffer_psl psl, psls, strtab;

    /* strip it */
    psl.buf = buf;
    psl.bufused = psl.bufsz = sz;
    stripPSL(psl, &psls, &strtab);

    /* the header */
    fwrite(PSL_FILE_MAGIC, 1, sizeof(PSL_FILE_MAGIC)-1, to);

    /* the program data */
    writePSLSection(to, PSL_SECTION_STRIPPED_PROGRAM_DATA, psls);

    /* and the string table */
    writePSLSection(to, PSL_SECTION_RAW_DATA_TABLE, strtab);
}

/* unstrip PSL */
struct Buffer_psl unstripPSL(struct Buffer_psl psls, struct Buffer_psl strtab)
{
    struct Buffer_psl psl;
    size_t psli;

    INIT_BUFFER(psl);

    /* go command-by-command, unstripping as necessary */
    for (psli = 0; psli < psls.bufused; psli++) {
        unsigned char cmd = psls.buf[psli];
        
        /* write the command ... */
        WRITE_BUFFER(psl, &cmd, 1);

        /* if it's raw data, need to handle it specially */
        if (cmd == psl_raw) {
            size_t len, loc, blen;
            psli++;

            /* get the size ... */
            blen = pslBignumToInt(psls.buf + psli, &len);

            /* and write it */
            WRITE_BUFFER(psl, psls.buf + psli, blen);
            psli += blen;

            /* then write the string */
            blen = pslBignumToInt(psls.buf + psli, &loc);
            psli += blen - 1;
            if (loc > strtab.bufused || loc + len > strtab.bufused) {
                fprintf(stderr, "Error decoding raw data table!\n");
                for (; len > 0; len--) {
                    WRITE_BUFFER(psl, &cmd, 1);
                }
            } else {
                WRITE_BUFFER(psl, strtab.buf + loc, len);
            }
        }
    }

    return psl;
}

/* write the requested string into the string table, or use it if it's already there */
static size_t stripPSLStr(unsigned char *str, size_t len, struct Buffer_psl *strtab)
{
    size_t si;

    /* first check if it's there */
    if (strtab->bufused >= len) {
        for (si = 0; si < strtab->bufused - len; si++) {
            if (!memcmp(strtab->buf + si, str, len)) return si;
        }
    }

    /* not there, add it */
    si = strtab->bufused;
    WRITE_BUFFER(*strtab, str, len);

    return si;
}

/* strip PSL */
void stripPSL(struct Buffer_psl psl, struct Buffer_psl *psls, struct Buffer_psl *strtab)
{
    size_t psli;

    INIT_BUFFER(*psls);
    INIT_BUFFER(*strtab);

    /* go command-by-command, stripping as necessary */
    for (psli = 0; psli < psl.bufused; psli++) {
        unsigned char cmd = psl.buf[psli];
        
        /* write the command ... */
        WRITE_BUFFER(*psls, &cmd, 1);

        /* if it's raw data, need to handle it specially */
        if (cmd == psl_raw) {
            size_t len, loc, blen;
            psli++;

            /* get the size ... */
            blen = pslBignumToInt(psl.buf + psli, &len);

            /* and write it */
            WRITE_BUFFER(*psls, psl.buf + psli, blen);
            psli += blen;

            /* then get the string location */
            loc = stripPSLStr(psl.buf + psli, len, strtab);
            psli += len - 1;

            /* and write it */
            blen = pslBignumLength(loc);
            while (BUFFER_SPACE(*psls) < blen) EXPAND_BUFFER(*psls);
            pslIntToBignum(BUFFER_END(*psls), loc, blen);
            psls->bufused += blen;
        }
    }
}
