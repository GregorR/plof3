/*
 * Ratpack parser wrappers
 *
 * Copyright (c) 2009 Gregor Richards, Josiah Worcester
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

#include "bignum.h"
#include "buffer.h"
#include "helpers.h"
#include "packrat.h"
#include "plof.h"
#include "psl.h"

#include "prp.h"

BUFFER(target, unsigned char **);
BUFFER(psl_array, struct Buffer_psl);

struct UProduction {
    unsigned char *name;
    struct Buffer_target target;
    struct Buffer_psl_array psl;
    struct UProduction *right, *left;
};

struct PlofObject *parse_helper(unsigned char *code, struct ParseResult *pr);

static struct UProduction *getUProduction(unsigned char *name);

static struct UProduction *newUProduction(unsigned char *name);

static void gcommitRecurse(struct UProduction *curup);

static struct UProduction *new_grammar = NULL;

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
    struct Production *top_prod = getProduction((unsigned char *)"top");
    struct ParseResult *res = packratParse(top_prod, file, line,
                                           column, code);
    memset(&ret, 0, sizeof(struct PRPResult));
    if (res == NULL) /* bail out */
        return ret;

    /* figure out how much we actually parsed */
    ret.remainder = code + res->consumedTo;
    ret.rline = res->eline;
    ret.rcol = res->ecol;

    /* get the resultant object */
    pobj = parse_helper(code, res);

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

    INIT_BUFFER(res);

    while (*code) {
        struct PRPResult prpr = parseOne(code, top, file, line, column);
        if (prpr.code.buf == NULL) {
            fprintf(stderr, "Parse error somewhere! (FIXME: better error message)\n");
#ifdef DEBUG
            fprintf(stderr, "%s\n", code);
#endif
            exit(1);
        }
        code = prpr.remainder;

        /* run immediates */
        interpretPSL(plofNull, plofNull, NULL, prpr.code.bufused, prpr.code.buf, 1, 1);

        /* and add it to our result */
        WRITE_BUFFER(res, prpr.code.buf, prpr.code.bufused);
    }

    return res;
}

struct PlofObject *parse_helper(unsigned char *code, struct ParseResult *pr)
{
    int i;
    struct PlofRawData *rd;
    struct PlofArrayData *ad;
    struct PlofObject *obj, *ret;

    /*if (!pr->subResults) // We're done here.
        return psl;*/

    /* produce an array from the sub-results */
    ad = GC_NEW_Z(struct PlofArrayData);
    ad->type = PLOF_DATA_ARRAY;
    for (ad->length = 0; pr->subResults[ad->length]; ad->length++);
    ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));
    for (i = 0; pr->subResults[i]; i++) {
        ad->data[i] = parse_helper(code, pr->subResults[i]);
    }

    /* put the array in an object */
    obj = GC_NEW_Z(struct PlofObject);
    obj->parent = plofNull;
    obj->data = (struct PlofData *) ad;

    /* run the code if applicable */
    if (pr->production->userarg) {
        struct PlofReturn pret;

        /* the code is stored as an array, needs to know which was chosen */
        struct Buffer_psl psl = ((struct Buffer_psl *) pr->production->userarg)[pr->choice];
        
        /* run it */
        pret = interpretPSL(plofNull, obj, NULL, psl.bufused, psl.buf, 1, 0);
        ret = pret.ret;

    } else {
        size_t parsedsz, bignumsz;

        /* just generate the code to return what was parsed */
        struct Buffer_psl psl;
        INIT_BUFFER(psl);

        /* operation is raw */
        psl.buf[psl.bufused++] = psl_raw;

        /* now write the bignum of the size */
        parsedsz = pr->consumedTo - pr->consumedFrom;
        bignumsz = pslBignumLength(parsedsz);
        while (BUFFER_SPACE(psl) < bignumsz)
            EXPAND_BUFFER(psl);
        pslIntToBignum(BUFFER_END(psl), parsedsz, bignumsz);
        psl.bufused += bignumsz;

        /* and put it in an object */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = psl.bufused;
        rd->data = psl.buf;
        ret = GC_NEW_Z(struct PlofObject);
        ret->parent = plofNull;
        ret->data = (struct PlofData *) rd;

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

    for (i = 0; curp->target.buf[i]; i++)
        for (j = 0; curp->target.buf[i][j]; j++)
                if (curp->target.buf[i][j][0] == '/') {
                        newPackratRegexTerminal(curp->target.buf[i][j],
                                                curp->target.buf[i][j])->userarg = NULL;
                }
    newPackratNonterminal(curp->name, curp->target.buf)->userarg
            = curp->psl.buf;

    if (curp->left)
        gcommitRecurse(curp->left);
    if (curp->right)
        gcommitRecurse(curp->right);
}
