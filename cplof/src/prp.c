/*
 * Ratpack parser wrappers
 *
 * Copyright (C) 2009, 2010 Gregor Richards
 * Copyright (C) 2009 Josiah Worcester
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

#include <gc/gc.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define BUFFER_GC
#include "plof/bignum.h"
#include "plof/buffer.h"
#include "plof/helpers.h"
#include "plof/memory.h"
#include "plof/packrat.h"
#include "plof/plof.h"
#include "plof/psl.h"

#include "plof/prp.h"

BUFFER(target, unsigned char **);
BUFFER(psl_array, struct Buffer_psl);

struct UProduction {
    unsigned char *name;
    struct Buffer_target target;
    struct Buffer_psl_array psl;
    struct UProduction *right, *left;
};

struct PlofObject *parseHelper(unsigned char *code, struct ParseResult *pr);

static struct UProduction *getUProduction(unsigned char *name);

static struct UProduction *newUProduction(unsigned char *name);

static void gcommitRecurse(struct UProduction *curup);

static struct UProduction *new_grammar = NULL;

int prpDebug = 0;

void gadd(unsigned char *name, unsigned char **target, size_t psllen, unsigned char *psl)
{
    struct UProduction *curp = getUProduction(name);
    struct Buffer_psl pslbuf;
    pslbuf.buf = psl;
    pslbuf.bufsz = pslbuf.bufused = psllen;

#ifdef DEBUG
    fprintf(stderr, "PRP: gadd %s\n", name);
#endif

    if(BUFFER_SPACE(curp->target) < 1)
        EXPAND_BUFFER(curp->target);
    STEP_BUFFER(curp->target, 1);
    BUFFER_TOP(curp->target) = target;
    if(BUFFER_SPACE(curp->psl) < 1)
        EXPAND_BUFFER(curp->psl);
    STEP_BUFFER(curp->psl, 1);
    BUFFER_TOP(curp->psl) = pslbuf;
}

void grem(unsigned char *name)
{
    struct UProduction *curp = getUProduction(name);

    INIT_BUFFER(curp->target);
    INIT_BUFFER(curp->psl);
}

void gcommit()
{
    delAllProductions();
    gcommitRecurse(new_grammar);
}

struct PRPResult parseOne(unsigned char *code, unsigned char *top, unsigned char *file,
                          unsigned int line, unsigned int column)
{
    struct PRPResult ret;
    struct PlofObject *pobj;
    struct PlofRawData *rd;
    struct ParseContext *ctx = GC_NEW(struct ParseContext);
    struct Production *top_prod = getProduction(top);
    struct ParseResult *res = packratParse(ctx, top_prod, file, line,
                                           column, code);
    memset(&ret, 0, sizeof(struct PRPResult));

    /* pass out the context */
    ret.ctx = ctx;

    if (res == NULL) /* bail out */
        return ret;

    /* figure out how much we actually parsed */
    ret.remainder = code + res->consumedTo;
    ret.rline = res->eline;
    ret.rcol = res->ecol;

    /* get the resultant object */
    pobj = parseHelper(code, res);

    /* make sure it has raw data */
    if (pobj->data && pobj->data->type == PLOF_DATA_RAW) {
        rd = (struct PlofRawData *) pobj->data;
        ret.code.buf = rd->data;
        ret.code.bufused = ret.code.bufsz = rd->length;
    }

    return ret;
}

struct Buffer_psl parseAll(unsigned char *code, unsigned char *top, unsigned char *file)
{
    unsigned int line, column;
    struct Buffer_psl res;
    line = 0;
    column = 0;

    INIT_ATOMIC_BUFFER(res);

    while (*code) {
        struct PRPResult prpr = parseOne(code, top, file, line, column);
        if (prpr.code.buf == NULL) {
            fprintf(stderr, "Parse error in file %s ", file);

            /* get the error from the context */
            if (prpr.ctx && prpr.ctx->current) {
                fprintf(stderr, "line %d col %d, parsing %s, expected %s, found '%.10s'\n",
                        prpr.ctx->line + 1, prpr.ctx->col + 1,
                        prpr.ctx->current->name, prpr.ctx->expected->name,
                        code + prpr.ctx->loc);

            } else {
                fprintf(stderr, "near '%.10s'\n",
                        prpr.remainder);

            }
                        
            exit(1);
#ifdef DEBUG
        } else {
            fprintf(stderr, "Parsed %.*s\n", prpr.remainder - code, code);
#endif
        }

        code = prpr.remainder;
        line = prpr.rline;
        column = prpr.rcol;

        /* run immediates */
        interpretPSL(plofNull, plofNull, NULL, prpr.code.bufused, prpr.code.buf, 1, 1);

        /* and add it to our result */
        WRITE_BUFFER(res, prpr.code.buf, prpr.code.bufused);
    }

    return res;
}

struct PlofObject *parseHelper(unsigned char *code, struct ParseResult *pr)
{
    int i;
    struct PlofRawData *rd;
    struct PlofArrayData *ad;
    struct PlofObject *obj, *ret;
    size_t len;

    /* produce an array from the sub-results */
    for (len = 0; pr->subResults && pr->subResults[len]; len++);
    obj = newPlofObjectWithArray(len);
    obj->parent = plofNull;
    ad = (struct PlofArrayData *) obj->data;
    for (i = 0; i < ad->length; i++) {
        ad->data[i] = parseHelper(code, pr->subResults[i]);
    }

    /* run the code if applicable */
    if (pr->production->userarg) {
        struct PlofReturn pret;

        /* the code is stored as an array, needs to know which was chosen */
        struct Buffer_psl psl = ((struct Buffer_psl *) pr->production->userarg)[pr->choice];
        
        /* run it */
        pret = interpretPSL(plofNull, obj, NULL, psl.bufused, psl.buf, 1, 0);
        if (pret.isThrown) {
            /* uh oh ! */
            ret = plofNull;
            plofThrewUp(pret.ret);
        } else {
            ret = pret.ret;
        }

    } else {
        size_t parsedsz, bignumsz;

        /* just generate the code to return what was parsed */
        struct Buffer_psl psl;
        INIT_ATOMIC_BUFFER(psl);

        /* operation is raw */
        psl.buf[psl.bufused++] = psl_raw;

        /* now write the bignum of the size */
        parsedsz = pr->consumedTo - pr->consumedFrom;
        bignumsz = pslBignumLength(parsedsz);
        while (BUFFER_SPACE(psl) < bignumsz)
            EXPAND_BUFFER(psl);
        pslIntToBignum(BUFFER_END(psl), parsedsz, bignumsz);
        psl.bufused += bignumsz;

        /* and the data itself */
        WRITE_BUFFER(psl, code + pr->consumedFrom, pr->consumedTo - pr->consumedFrom);

        /* and put it in an object */
        rd = newPlofRawData(psl.bufused);
        memcpy(rd->data, psl.buf, psl.bufused);
        ret = newPlofObject();
        ret->parent = plofNull;
        ret->data = (struct PlofData *) rd;

    }

    /* perhaps add debugging info */
    if (prpDebug) {
        if (((size_t) ret & 0x1) == 0x0 &&
            ret->data &&
            ret->data->type == PLOF_DATA_RAW &&
            ((struct PlofRawData *) ret->data)->length > sizeof(size_t)) {
            struct PlofRawData *rd = (struct PlofRawData *) ret->data;
            struct Buffer_psl psl;
            size_t bignumsz, filenmsz;
            struct PlofObject *obj;

            /* now add the debug info */
            INIT_ATOMIC_BUFFER(psl);

            /* first the filename */
            while (BUFFER_SPACE(psl) < 1) EXPAND_BUFFER(psl);
            psl.buf[psl.bufused++] = psl_raw;
            filenmsz = strlen((char *) pr->file);
            bignumsz = pslBignumLength(filenmsz);
            while (BUFFER_SPACE(psl) < bignumsz) EXPAND_BUFFER(psl);
            pslIntToBignum(psl.buf + psl.bufused, filenmsz, bignumsz);
            psl.bufused += bignumsz;
            WRITE_BUFFER(psl, pr->file, filenmsz);
            while (BUFFER_SPACE(psl) < 1) EXPAND_BUFFER(psl);
            psl.buf[psl.bufused++] = psl_dsrcfile;

            /* then the line number */
            while (BUFFER_SPACE(psl) < 8) EXPAND_BUFFER(psl);
            psl.buf[psl.bufused++] = psl_raw;
            psl.buf[psl.bufused++] = 4;
            psl.buf[psl.bufused++] = (pr->sline) >> 24;
            psl.buf[psl.bufused++] = (pr->sline) >> 16;
            psl.buf[psl.bufused++] = (pr->sline) >> 8;
            psl.buf[psl.bufused++] = (pr->sline);
            psl.buf[psl.bufused++] = psl_integer;
            psl.buf[psl.bufused++] = psl_dsrcline;

            /* then the column */
            while (BUFFER_SPACE(psl) < 8) EXPAND_BUFFER(psl);
            psl.buf[psl.bufused++] = psl_raw;
            psl.buf[psl.bufused++] = 4;
            psl.buf[psl.bufused++] = (pr->scol) >> 24;
            psl.buf[psl.bufused++] = (pr->scol) >> 16;
            psl.buf[psl.bufused++] = (pr->scol) >> 8;
            psl.buf[psl.bufused++] = (pr->scol);
            psl.buf[psl.bufused++] = psl_integer;
            psl.buf[psl.bufused++] = psl_dsrccol;

            /* then the original source */
            WRITE_BUFFER(psl, rd->data, rd->length);

            /* now recreate the data */
            rd = newPlofRawData(psl.bufused);
            memcpy(rd->data, psl.buf, psl.bufused);
            obj = newPlofObject();
            obj->parent = ret->parent;
            obj->data = (struct PlofData *) rd;
            ret = obj;
        }
    }

    return ret;
}


static struct UProduction *getUProduction(unsigned char *name)
{
    struct UProduction *curp = new_grammar;

    /* first production */
    if (curp == NULL)
        return (new_grammar = newUProduction(name));

    while (1) {
        int cmp = strcmp((char *) name, (char *) curp->name);
        if (cmp < 0)
            if (curp->left)
                curp = curp->left;
            else
                return (curp->left = newUProduction(name));
        else if (cmp > 0)
            if (curp->right)
                curp = curp->right;
            else
                return (curp->right = newUProduction(name));
        else
            return curp;
    }
}    

static struct UProduction *newUProduction(unsigned char *name)
{
    struct UProduction *ret = NULL;
    SF(ret, GC_malloc, NULL,
       (sizeof(struct UProduction)));

    ret->name = (unsigned char *) GC_STRDUP((char *) name);
    INIT_BUFFER(ret->target);
    INIT_BUFFER(ret->psl);
    return ret;
}

static void gcommitRecurse(struct UProduction *curp)
{
    int i, j;

    for (i = 0; curp->target.buf[i]; i++) {
        for (j = 0; curp->target.buf[i][j]; j++) {
            if (curp->target.buf[i][j][0] == '/') {
               unsigned char *name = curp->target.buf[i][j];
               size_t regexlen = strlen((char *) name)-2;
               unsigned char *regex = (unsigned char *) GC_MALLOC_ATOMIC(regexlen+1);
               memcpy(regex, name+1, regexlen);
               regex[regexlen] = '\0';

               newPackratRegexTerminal(name, regex)->userarg = NULL;
            }
        }
    }

    newPackratNonterminal(curp->name, curp->target.buf)->userarg
        = curp->psl.buf;

    if (curp->left)
        gcommitRecurse(curp->left);
    if (curp->right)
        gcommitRecurse(curp->right);
}
