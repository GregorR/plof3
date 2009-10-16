/*
 * A Plof-suitable packrat parser (memoized recursive descent parser)
 *
 * Copyright (C) 2009 Gregor Richards
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

#include <stdio.h>
#include <string.h>

#include <gc/gc.h>
#include <pcre.h>

#include "packrat.h"

#define BUFFER_DEFAULT_SIZE 8
#include "buffer.h"
BUFFER(ParseResult, struct ParseResult *);
BUFFER(Production, struct Production *);
BUFFER(Production_p, struct Production **);

struct Production *productions = NULL;

/* create a new, empty production with the given name */
static struct Production *newProduction(const unsigned char *name)
{
    struct Production *ret = GC_NEW(struct Production);
    memset(ret, 0, sizeof(struct Production));

    ret->name = (unsigned char *) GC_STRDUP((char *) name);

    return ret;
}

/* get a production matching a particular name. Always returns something, but
 * may have NULL functionality */
struct Production *getProduction(const unsigned char *name)
{
    struct Production *curp = productions;

    /* first production */
    if (curp == NULL) {
        return (productions = newProduction(name));
    }

    /* find a place to put it, or where it already is */
    while (1) {
        int cmp = strcmp((char *) name, (char *) curp->name);
        if (cmp < 0) {
            if (curp->left) {
                curp = curp->left;
            } else {
                return (curp->left = newProduction(name));
            }

        } else if (cmp > 0) {
            if (curp->right) {
                curp = curp->right;
            } else {
                return (curp->right = newProduction(name));
            }

        } else {
            return curp;

        }
    }
}

/* remove all productions with the given name */
void delProduction(const unsigned char *name)
{
    /* find the relevant production */
    struct Production *curp = getProduction(name),
                      *curp_left = curp->left,
                      *curp_right = curp->right;

    /* and blank it */
    memset(curp, 0, sizeof(struct Production));
    curp->name = (unsigned char *) GC_STRDUP((char *) name);
    curp->left = curp_left;
    curp->right = curp_right;
}

/* remove ALL productions */
void delAllProductions()
{
    productions = NULL;
}

/* parse using the specified production, not clearing out caches first (assumed
 * caches are good) */
static struct ParseResult **packratParsePrime(struct Production *production,
                                              unsigned char *file, int line, int col,
                                              unsigned char *input, size_t off)
{
    struct ParseResult **ret;

    /* make sure the cache is of the appropriate size */
    if (production->cachesz <= off) {
        production->cache = GC_REALLOC(production->cache,
                                       (off + 1) * sizeof(struct ParseResult **));
        memset(production->cache + production->cachesz, 0,
               (off + 1 - production->cachesz) * sizeof(struct ParseResult **));
        production->cachesz = off + 1;
    }

    /* then check if this is already cached */
    if (production->cache[off]) {
        return production->cache[off];
    }

#ifdef DEBUG
    fprintf(stderr, "Parsing %s at %d (%.5s)\n", production->name, off, input + off);
#endif

    /* go through each relevant production, collecting results */
    ret = NULL;
    if (production->parser) {
        /* run this production */
        ret = production->parser(production, file, line, col, input, off);
    } else {
        fprintf(stderr, "Production %s has no parser!\n", production->name);
    }

    if (ret == NULL) {
        ret = GC_MALLOC(sizeof(struct ParseResult *));
        ret[0] = NULL;
    }

    /* cache it */
    production->cache[off] = ret;

    /* done! */
    return ret;
}

/* clear out production caches */
static void clearCaches(struct Production *production)
{
    production->cachesz = 0;
    production->cache = NULL;

    if (production->left)
        clearCaches(production->left);

    if (production->right)
        clearCaches(production->right);
}

/* parse using the specified production */
struct ParseResult *packratParse(struct Production *production,
                                 unsigned char *file,
                                 int line, int col,
                                 unsigned char *input)
{
    struct ParseResult **pr, *longest;
    int i;

    /* first clear out all the caches */
    clearCaches(productions);

    /* then parse */
    pr = packratParsePrime(production, file, line, col, input, 0);

    /* choose the longest */
    longest = pr[0];
    for (i = 0; pr[i]; i++) {
        if (pr[i]->consumedTo > longest->consumedTo)
            longest = pr[i];
    }
    return longest;
}

/* parse a nonterminal (that is, parse some list of child nodes) */
struct ParseResult **packratNonterminal(struct Production *production,
                                        unsigned char *file, int line, int col,
                                        unsigned char *input, size_t off)
{
    struct Buffer_ParseResult result, lastResult, lastResultP, orResult, orResultP;
    struct Production ***subProductions = (struct Production ***) production->arg;
    struct Production **orProduction;
    int lrec, ors, thens, i, j;
    struct ParseResult *pr;

    struct ParseResult **subres;
    size_t srlen;

    INIT_BUFFER(result);


    /* set up an empty result */
    INIT_BUFFER(lastResult)
    pr = GC_NEW(struct ParseResult);
    memset(pr, 0, sizeof(struct ParseResult));
    pr->production = production;
    pr->file = file;
    pr->sline = pr->eline = line;
    pr->scol = pr->ecol = col;
    pr->choice = 0;
    pr->consumedFrom = pr->consumedTo = off;

    WRITE_BUFFER(lastResult, &pr, 1);

    /* loop over left recursions until we get to a fixed point */
    for (lrec = 0; lastResult.bufused; lrec = 1) {
        INIT_BUFFER(lastResultP);

        /* first loop over the ors */
        for (ors = 0; subProductions[ors]; ors++) {
            orProduction = subProductions[ors];

            /* make sure we only do left recursive when we're supposed to */
            if (orProduction[0] == production) {
                if (!lrec) continue;
            } else {
                if (lrec) continue;
            }

            INIT_BUFFER(orResult);

            /* start where we left off */
            WRITE_BUFFER(orResult, lastResult.buf, lastResult.bufused);

            /* then loop over the thens */
            for (thens = lrec; orProduction[thens]; thens++) {

                INIT_BUFFER(orResultP);

                /* loop over each of the current points */
                for (i = 0; i < orResult.bufused; i++) {
                    subres = packratParsePrime(orProduction[thens],
                                               file, orResult.buf[i]->eline, orResult.buf[i]->ecol,
                                               input, orResult.buf[i]->consumedTo);
                    for (srlen = 0; subres[srlen]; srlen++);

                    /* and extend them into orResultP */
                    for (j = 0; subres[j]; j++) {
                        pr = GC_NEW(struct ParseResult);
                        memcpy(pr, orResult.buf[i], sizeof(struct ParseResult));
                        pr->subResults = GC_MALLOC((thens + 2) * sizeof(struct ParseResult *));
                        memcpy(pr->subResults, orResult.buf[i]->subResults, thens * sizeof(struct ParseResult *));
                        pr->subResults[thens] = subres[j];
                        pr->subResults[thens+1] = NULL;
                        pr->eline = subres[j]->eline;
                        pr->ecol = subres[j]->ecol;
                        pr->choice = ors;
                        pr->consumedTo = subres[j]->consumedTo;
                        WRITE_BUFFER(orResultP, &pr, 1);
                    }
                }

                /* then replace orResult */
                orResult = orResultP;
            }

            /* now that one of the or's has succeeded, we can add it to the overall result */
            WRITE_BUFFER(result, orResult.buf, orResult.bufused);

            /* now package up this result for left recursion */
            for (i = 0; i < orResult.bufused; i++) {
                pr = GC_NEW(struct ParseResult);
                memcpy(pr, orResult.buf[i], sizeof(struct ParseResult));
                pr->subResults = GC_MALLOC(2 * sizeof(struct ParseResult *));
                pr->subResults[0] = orResult.buf[i];
                pr->subResults[1] = NULL;
                orResult.buf[i] = pr;
            }
            WRITE_BUFFER(lastResultP, orResult.buf, orResult.bufused);

        }

        lastResult = lastResultP;
    }

    pr = NULL;
    WRITE_BUFFER(result, &pr, 1);

    return result.buf;
}

/* parse a regex terminal */
#define OVECTOR_LEN 30
struct ParseResult **packratRegexTerminal(struct Production *production,
                                          unsigned char *file, int line, int col,
                                          unsigned char *input, size_t off)
{
    struct ParseResult **ret;
    int ovector[OVECTOR_LEN], result, i;

    /* even though we can only actually return one result, the standard is to
     * return an array */
    ret = GC_MALLOC(2 * sizeof(struct ParseResult *));
    ret[1] = NULL;
    ret[0] = GC_MALLOC(sizeof(struct ParseResult));
    memset(ret[0], 0, sizeof(struct ParseResult));
    ret[0]->production = production;
    ret[0]->file = file;
    ret[0]->sline = line;
    ret[0]->scol = col;
    ret[0]->consumedFrom = off;

    /* try to run the regex; FIXME: shouldn't need strlen here */
    result = pcre_exec((pcre *) production->arg, NULL,
                       (char *) input + off, strlen((char *) input) - off, 0,
                       PCRE_ANCHORED,
                       ovector, OVECTOR_LEN);

    /* if it failed, that's easy enough */
    if (result <= 0) {
        return NULL;
    }

    /* didn't fail, fill in consumedTo */
    if (result >= 2) {
        ret[0]->consumedTo = off + ovector[3];
    } else {
        ret[0]->consumedTo = off + ovector[1];
    }

    /* and the line/col numbers */
    ret[0]->eline = line;
    ret[0]->ecol = col;
    for (i = off; i < ret[0]->consumedTo; i++) {
        switch (input[i]) {
            case '\n':
                ret[0]->eline++;
                ret[0]->ecol = 0;
                break;

            default:
                ret[0]->ecol++;
        }
    }

    return ret;
}

/* create a nonterminal given only names */
struct Production *newPackratNonterminal(unsigned char *name, unsigned char ***sub)
{
    struct Production *ret = getProduction(name);
    struct Buffer_Production_p pors;
    struct Buffer_Production pthens;
    struct Production *pr;
    int ors, thens;

    ret->parser = packratNonterminal;

    /* now fill in the sub-productions */
    INIT_BUFFER(pors);
    for (ors = 0; sub[ors]; ors++) {
        INIT_BUFFER(pthens);
        for (thens = 0; sub[ors][thens]; thens++) {
            pr = getProduction(sub[ors][thens]);
            WRITE_BUFFER(pthens, &pr, 1);
        }
        pr = NULL;
        WRITE_BUFFER(pthens, &pr, 1);

        WRITE_BUFFER(pors, &pthens.buf, 1);
    }
    pr = NULL;
    WRITE_BUFFER(pors, (struct Production ***) &pr, 1);

    ret->arg = pors.buf;

    return ret;
}

/* create a regex nonterminal */
struct Production *newPackratRegexTerminal(unsigned char *name, unsigned char *regex)
{
    struct Production *ret = getProduction(name);
    const char *err;
    int erroffset;

    ret->parser = packratRegexTerminal;

    /* now fill in the arg */
    ret->arg = pcre_compile((char *) regex, PCRE_DOTALL, &err, &erroffset, NULL);

    /* and cry if it's bad */
    if (ret->arg == NULL) {
        fprintf(stderr, "Error compiling regex %s at (%d): %s\n", regex, erroffset, err);
        exit(1);
    }

    return ret;
}
