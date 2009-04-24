/*
 * Ratpack parser wrappers
 *
 * Copyright (c) 2009 Josiah Worcester
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

#include "packrat.h"
#include "buffer.h"
#include "helpers.h"

#include "prp.h"

BUFFER(target, unsigned char **);
BUFFER(psl, unsigned char *);
BUFFER(bytes, unsigned char);

struct UProduction {
    unsigned char *name;
    struct Buffer_target target;
    struct Buffer_psl psl;
    struct UProduction *right, *left;
};

static struct Buffer_bytes parse_helper(struct ParseResult *pr, struct Buffer_bytes psl);

static struct UProduction *getUProduction(unsigned char *name);

static struct UProduction *newUProduction(unsigned char *name);

static void gcommitRecurse(struct UProduction *curup);

static struct UProduction *new_grammer = NULL;

void gadd(unsigned char *name, unsigned char **target, unsigned char *psl)
{
    struct UProduction *curp = getUProduction(name);

    if(BUFFER_SPACE(curp->target) < 1)
	EXPAND_BUFFER(curp->target);
    STEP_BUFFER(curp->target, 1);
    BUFFER_TOP(curp->target) = target;
    if(BUFFER_SPACE(curp->psl) < 1)
	EXPAND_BUFFER(curp->psl);
    STEP_BUFFER(curp->psl, 1);
    BUFFER_TOP(curp->psl) = GC_STRDUP((char *)psl);
}

void grem(char *name)
{
    struct UProduction *curp = getUProduction(name);

    INIT_BUFFER(curp->target);
    INIT_BUFFER(curp->psl);
}

void gcommit()
{
    delAllProductions();
    gcommitRecurse(new_grammer);
}

unsigned char *parse(unsigned char *code, unsigned char *top, unsigned char *file,
	   unsigned int line, unsigned int column)
{
    struct Buffer_bytes psl;
    INIT_BUFFER(psl);

    while (code != NULL) {
	struct Production *top_prod = getProduction((unsigned char *)"top");
	struct ParseResult *res = packratParse(top_prod, file, line,
					       column, code);
	if (res == NULL) // Bail out
	    return psl.buf;

	psl = parse_helper(res, psl);
	line = res->eline;
	column = res->ecol;
	code += res->consumedTo;
    }
    return psl.buf;
}

struct PlofObject *parse_helper(struct ParseResult *pr, struct PlofObject *po, unsigned char *code)
{
    if (!pr->subResults) // We're done here.
	return po;

    for (int i = 0; pr->subResults[i]; i++)
	po = parse_helper(pr->subResults[i], po, code);

    if (pr->production->userarg) { // Non-terminal, has code
	size_t len = strlen((char *)((unsigned char **)pr->production->userarg)[pr->choice]);
	
	WRITE_BUFFER(psl, ((unsigned char **)pr->production->userarg)[pr->choice],
		     len);
	
	// printf("%i: %s", pr->choice, ((unsigned char **)pr->production->userarg)[pr->choice]);
    } else {
	// Create a raw Plof object, storing the code in it
	
    }

    return po;
}


static struct UProduction *getUProduction(unsigned char *name)
{
    struct UProduction *curp = new_grammer;

    // First production
    if (curp == NULL)
	return (new_grammer = newUProduction(name));

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

    ret->name = GC_STRDUP(name);
    INIT_BUFFER(ret->target);
    INIT_BUFFER(ret->psl);
    return ret;
}

static void gcommitRecurse(struct UProduction *curp)
{
    struct Buffer_bytes tmp;
    for (int i = 0; curp->target.buf[i]; i++)
	for (int j = 0; curp->target.buf[i][j]; j++)
		if (curp->target.buf[i][j][0] == '/') {
			// tmp will consist of target, without the /s at the beginning and end
			INIT_BUFFER(tmp);
			WRITE_BUFFER(tmp, curp->target.buf[i][j] + 1,
				     strlen((char *)curp->target.buf[i][j]) - 2);
			newPackratRegexTerminal(curp->target.buf[i][j],
						tmp.buf)->userarg = NULL;
		}
    newPackratNonterminal(curp->name, curp->target.buf)->userarg
	    = curp->psl.buf;

    if (curp->left)
	gcommitRecurse(curp->left);
    if (curp->right)
	gcommitRecurse(curp->right);
}
