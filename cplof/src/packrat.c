/*
 * A Plof-suitable packrat parser (memoized recursive descent parser)
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
#include <pcre.h>

#include "packrat.h"

struct Production *productions = NULL;

/* create a new, empty production with the given name */
static struct Production *newProduction(const char *name)
{
    struct Production *ret = GC_NEW(struct Production);
    memset(ret, 0, sizeof(struct Production));

    ret->name = GC_STRDUP(name);

    return ret;
}

/* get a production matching a particular name. Always returns something, but
 * may have NULL functionality */
struct Production *getProduction(const char *name)
{
    struct Production *curp = productions;

    /* first production */
    if (curp == NULL) {
        return (productions = newProduction(name));
    }

    /* find a place to put it, or where it already is */
    while (1) {
        int cmp = strcmp(name, curp->name);
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

/* add a production */
void addProduction(struct Production *production)
{
    /* find where to put it */
    struct Production *curp = getProduction(production->name);

    /* and either overwrite or add it */
    if (curp->parser) {
        for (; curp->next; curp = curp->next);
        curp->next = production;

    } else {
        memcpy(curp, production, sizeof(struct Production));

    }
}

/* remove all productions with the given name */
void delProductions(const char *name)
{
    /* find the relevant production */
    struct Production *curp = getProduction(name);

    /* and blank it */
    memset(curp, 0, sizeof(struct Production));
    curp->name = GC_STRDUP(name);
}

/* remove ALL productions */
void delAllProductions()
{
    productions = NULL;
}

/* parse using the specified production, not clearing out caches first (assumed
 * caches are good) */
static struct ParseResult **packratParsePrime(struct Production *production,
                                              char *file, int line, int col,
                                              unsigned char *input, size_t off)
{
    struct ParseResult **ret, **subret;
    struct Production *curp;
    size_t retlen, srlen;

    retlen = 0;
    ret = GC_MALLOC(sizeof(struct ParseResult *));
    ret[0] = NULL;

    /* make sure the cache is of the appropriate size */
    if (production->cachesz <= off) {
        production->cache = GC_REALLOC(production->cache, off + 1);
        memset(production->cache + production->cachesz, 0, off + 1 - production->cachesz);
        production->cachesz = off + 1;
    }

    /* then check if this is already cached */
    if (production->cache[off])
        return production->cache[off];

    /* go through each relevant production, collecting results */
    for (curp = production; curp; curp = curp->next) {
        /* run this production */
        subret = curp->parser(curp, file, line, col, input, off);

        /* and add it to our list */
        if (subret) {
            for (srlen = 0; subret[srlen]; srlen++);

            ret = GC_REALLOC(ret, retlen + srlen + 1);
            memcpy(ret + retlen, subret, (srlen + 1) * sizeof(struct ParseResult *));
            retlen += srlen;
        }
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
                                 char *file,
                                 int line, int col,
                                 unsigned char *input)
{
    struct ParseResult **pr;

    /* first clear out all the caches */
    clearCaches(productions);

    /* then parse */
    pr = packratParsePrime(production, file, line, col, input, 0);

    /* packratParsePrime always returns a NULL-terminated array, so we can just
     * accept the first result, which may be NULL */
    return pr[0];
}

/* parse a nonterminal (that is, parse some list of child nodes) */
struct ParseResult **packratNonterminal(struct Production *production,
                                        char *file, int line, int col,
                                        unsigned char *input, size_t off)
{
    struct ParseResult **curret, **subret, **newret, *genres;
    int i, j;
    size_t srlen, nrlen;

    struct Production *subpr;
    int subpri;

    /* initialize our current solitary result */
    curret = GC_MALLOC(2 * sizeof(struct ParseResult *));
    curret[1] = NULL;
    curret[0] = GC_MALLOC(sizeof(struct ParseResult));
    memset(curret[0], 0, sizeof(struct ParseResult));
    curret[0]->production = production;
    curret[0]->file = file;
    curret[0]->sline = line;
    curret[0]->scol = col;
    curret[0]->consumedFrom = off;
    curret[0]->consumedTo = off;

    /* then go through each sub-production, expanding our results */
    for (subpri = 0; production->sub[subpri]; subpri++) {
        subpr = production->sub[subpri];
        newret = NULL;
        nrlen = 0;

        /* for each of our current states, */
        for (i = 0; curret[i]; i++) {

            /* parse this production at the end of that state, */
            subret = packratParsePrime(subpr, file, line, col, input, curret[i]->consumedTo);
            for (srlen = 0; subret[srlen]; srlen++);

            /* allocate space */
            if (srlen)
                newret = GC_REALLOC(newret, (nrlen + srlen + 1) * sizeof(struct ParseResult *));

            /* then add a new state for each output state */
            for (j = 0; subret[j]; j++) {
                genres = GC_MALLOC(sizeof(struct ParseResult));
                memcpy(genres, curret[i], sizeof(struct ParseResult));

                /* copy in the new subret */
                genres->subResults = GC_MALLOC((subpri + 2) * sizeof(struct ParseResult *));
                memcpy(genres->subResults, curret[i]->subResults, subpri * sizeof(struct ParseResults *));
                genres->subResults[subpri] = subret[j];
                genres->subResults[subpri+1] = NULL;

                /* and set up the consumed properly */
                genres->consumedTo = subret[j]->consumedTo;

                newret[nrlen + j] = genres;
            }

            nrlen += srlen;
            newret[nrlen] = NULL;
        }

        /* if we didn't generate any newret, we've failed! */
        if (newret == NULL) return NULL;
    }

    /* the final newret is our result */
    return newret;
}

/* parse a regex terminal */
#define OVECTOR_LEN 30
struct ParseResult **packratRegexTerminal(struct Production *production,
                                          char *file, int line, int col,
                                          unsigned char *input, size_t off)
{
    struct ParseResult **ret;
    int ovector[OVECTOR_LEN], result;

    /* even though we can only actually return one result, the standard is to
     * return an array */
    ret = GC_MALLOC(2 * sizeof(struct ParseResult *));
    ret[1] = NULL;
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
    if (result <= 0) return NULL;

    /* didn't fail, fill in consumedTo */
    if (result >= 2) {
        ret[0]->consumedTo = ovector[3];
    } else {
        ret[0]->consumedTo = ovector[1];
    }

    return ret;
}

/* create a nonterminal given only names */
struct Production *newPackratNonterminal(char *name, char **sub)
{
    struct Production *ret = getProduction(name);
    size_t spcount, i;

    ret->parser = packratNonterminal;

    /* now fill in the sub-productions */
    for (spcount = 0; sub[spcount]; spcount++);
    ret->sub = GC_MALLOC((spcount + 1) * sizeof(struct Production *));
    for (i = 0; sub[spcount]; i++) {
        ret->sub[i] = getProduction(sub[i]);
    }
    ret->sub[i] = NULL;

    return ret;
}

/* create a regex nonterminal */
struct Production *newPackratRegexTerminal(char *name, char *regex)
{
    struct Production *ret = getProduction(name);
    const char *err;
    int erroffset;

    ret->parser = packratNonterminal;

    /* now fill in the arg */
    ret->arg = pcre_compile(regex, PCRE_DOTALL, &err, &erroffset, NULL);

    /* and cry if it's bad */
    if (ret->arg == NULL) {
        fprintf(stderr, "Error compiling regex %s at (%d): %s\n", regex, erroffset, err);
        exit(1);
    }

    return ret;
}
