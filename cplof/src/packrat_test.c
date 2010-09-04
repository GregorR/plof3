/*
 * A simple test case for packrat parsing
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

#include <stdio.h>

#define BUFFER_GC
#include "plof/buffer.h"
#include "plof/packrat.h"
#include "plof/prp.h"

void printParseResult(struct ParseResult *pr, unsigned char *input, int spcs)
{
    int i;

    for (i = 0; i < spcs; i++) printf(" ");

    printf("%s: %.*s\n", pr->production->name,
                         (int) (pr->consumedTo - pr->consumedFrom),
                         input + pr->consumedFrom);

    if (pr->subResults)
        for (i = 0; pr->subResults[i]; i++)
            printParseResult(pr->subResults[i], input, spcs+1);
}

int main(int argc, char **argv)
{
    struct ParseResult *result;

    char ***mul, ***add, ***term, *inp;
    char **mul_a, **mul_b, **add_a, **add_b, **term_a, **top_a;

    mul_a = GC_MALLOC(4 * sizeof(unsigned char *));
    mul_a[0] = "mul"; mul_a[1] = "/\\*/"; mul_a[2] = "add"; mul_a[3] = NULL;
    mul_b = GC_MALLOC(2 * sizeof(unsigned char *));
    mul_b[0] = "add"; mul_b[1] = NULL;
    add_a = GC_MALLOC(4 * sizeof(unsigned char *));
    add_a[0] = "add"; add_a[1] = "/\\+/"; add_a[2] = "term"; add_a[3] = NULL;
    add_b = GC_MALLOC(2 * sizeof(unsigned char *));
    add_b[0] = "term"; add_b[1] = NULL;
    term_a = GC_MALLOC(2 * sizeof(unsigned char *));
    term_a[0] = "/[0-9]+/"; term_a[1] = NULL;
    top_a = GC_MALLOC(2 * sizeof(unsigned char *));
    top_a[0] = "mul"; top_a[1] = NULL;

    /*
    mul = GC_MALLOC(3 * sizeof(unsigned char **));
    mul[0] = mul_a; mul[1] = mul_b; mul[2] = NULL;
    add = GC_MALLOC(3 * sizeof(unsigned char **));
    add[0] = add_a; add[1] = add_b; add[2] = NULL;
    term = GC_MALLOC(2 * sizeof(unsigned char **));
    term[0] = term_a; term[1] = NULL;

    newPackratNonterminal((unsigned char *) "mul", (unsigned char ***) mul);
    newPackratNonterminal((unsigned char *) "add", (unsigned char ***) add);
    newPackratNonterminal((unsigned char *) "term", (unsigned char ***) term);
    */ // newPackratRegexTerminal((unsigned char *) "/\\*/", (unsigned char *) "\\*");
    /*
    newPackratRegexTerminal((unsigned char *) "/\\+/", (unsigned char *) "\\+");
    newPackratRegexTerminal((unsigned char *) "/[0-9]+/", (unsigned char *) "[0-9]+");
    */

    inp = "1+2*3+4";
    gadd((unsigned char *)"mul", (unsigned char **)mul_a, (unsigned char *)"mul = mul /\\*/ add -> mul * add\n");
    gadd((unsigned char *)"mul", (unsigned char **)mul_b, (unsigned char *)"mul = add -> add\n");
    gadd((unsigned char *)"add", (unsigned char **)add_a, (unsigned char *)"add = add /\\+/ term -> add + term\n");
    gadd((unsigned char *)"add", (unsigned char **)add_b, (unsigned char *)"add = term -> term\n");
    gadd((unsigned char *)"term", (unsigned char **)term_a, (unsigned char *)"term = /[0-9]+/ -> [0-9]+\n");
    gadd((unsigned char *)"top", (unsigned char **)top_a, (unsigned char *)"top = mul -> mul\n");
    gcommit();

    result = packratParse(getProduction((unsigned char *) "top"), (unsigned char *) "INPUT", 0, 0, (unsigned char *) inp);
    printParseResult(result, (unsigned char *) inp, 0);

    printf((char *)parse((unsigned char *)inp, (unsigned char *)"top", (unsigned char *)"packrat_test.c", 0, 0));

    return 0;
}
