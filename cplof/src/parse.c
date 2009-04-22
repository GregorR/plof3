/*
 * Parser for ASCII PSL
 *
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

#include <gc/gc.h>

#include "bignum.h"
#include "lex.h"
#include "parse.h"
#include "psl.h"

#define BUFSTEP 1024

/* parse a stream of APSL into PSL */
struct UCharBuf pslParse(unsigned char **apsl)
{
    size_t pslsz, psli;
    unsigned char *psl;
    struct UCharBuf ret;
    unsigned char token[BUFSTEP];

    /* initialize the buffer */
    pslsz = BUFSTEP;
    psli = 0;
    psl = GC_MALLOC_ATOMIC(BUFSTEP);

#define EXPAND_BUFFER(to) \
    while (psli + (to) > pslsz) { \
        pslsz += BUFSTEP; \
        psl = GC_REALLOC(psl, pslsz); \
    }

    /* and start receiving tokens */
    while (pslTok(apsl, (unsigned char *) token, BUFSTEP)) {
        /* make sure we have room for at least one more element */
        EXPAND_BUFFER(1);

        /* first all the normal psl ops */
#define FOREACH(op) \
        if (!strcmp((char *) token, #op)) { \
            psl[psli++] = psl_ ## op; \
        } else
#include "psl_inst.h"
#undef FOREACH

        /* then specials */
        if (token[0] >= '0' && token[0] <= '9') {
            long val = atol((char *) token);

            /* a number, push as a 32-bit raw and an 'integer' operation*/
            EXPAND_BUFFER(7);
            psl[psli++] = psl_raw;
            psl[psli++] = 4; /* length as (short) bignum */
            
            psl[psli+3] = val & 0xFF;
            val >>= 8;
            psl[psli+2] = val & 0xFF;
            val >>= 8;
            psl[psli+1] = val & 0xFF;
            val >>= 8;
            psl[psli] = val & 0xFF;
            psli += 4;

            psl[psli++] = psl_integer;

        } else if (!strcmp((char *) token, "{") || !strcmp((char *) token, "[")) {
            /* nesting structures, do a sub-parse */
            unsigned char op;
            size_t bnsiz;
            struct UCharBuf subp = pslParse(apsl);

            /* figure out how much space we need */
            if (token[0] == '{') {
                op = psl_code;
            } else {
                op = psl_immediate;
            }
            bnsiz = pslBignumLength(subp.len);
            EXPAND_BUFFER(1 + bnsiz + subp.len);

            /* then put it in place */
            psl[psli++] = op;
            pslIntToBignum(psl + psli, subp.len, bnsiz);
            psli += bnsiz;
            memcpy(psl + psli, subp.ptr, subp.len);
            psli += subp.len;

        } else if (!strcmp((char *) token, "}") || !strcmp((char *) token, "]")) {
            /* ending a block */
            break;

        } else if (token[0] == '"') {
            /* a raw string (FIXME: escapes) */
            size_t bnsiz, slen;
            slen = strlen((char *) token);
            slen--;
            if (slen >= 1) slen--;

            bnsiz = pslBignumLength(slen);
            EXPAND_BUFFER(1 + bnsiz + slen);

            psl[psli++] = psl_raw;
            pslIntToBignum(psl + psli, slen, bnsiz);
            psli += bnsiz;
            memcpy(psl + psli, token + 1, slen);
            psli += slen;

        } else {
            fprintf(stderr, "Unrecognized token %s\n", token);
        }
    }

    ret.len = psli;
    ret.ptr = psl;

    return ret;
}
