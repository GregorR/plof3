/*
 * An AST(ish) structure for PSL
 *
 * Copyright (C) 2010 Gregor Richards
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
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "plof/bignum.h"
#include "plof/memory.h"
#include "optimizations.h"
#include "plof/psl.h"

/* allocate a PSL AST node with these children */
struct PSLAstNode *allocPSLAstNode(unsigned short cmd,
                                   unsigned char *data, size_t datasz,
                                   size_t childct, struct PSLAstNode **children)
{
    struct PSLAstNode *ret = GC_MALLOC(sizeof(struct PSLAstNode) +
                                       (childct - 1) * sizeof(struct PSLAstNode *));

    ret->cmd = cmd;
    ret->temp0 = ret->temp1 = 0;
    ret->data = data;
    ret->datasz = datasz;
    ret->childct = childct;
    if (childct > 0) {
        memcpy(ret->children, children, childct * sizeof(struct PSLAstNode *));
    }

    return ret;
}

/* turn this PSL into a PSL AST */
struct PSLAstNode *pslToAst(unsigned char *psl, size_t psllen)
{
    size_t psli, si, curtemp;
    struct Buffer_PSLAstNode astout, aststack;
    struct PSLAstNode *cur;

    INIT_BUFFER(astout);
    INIT_BUFFER(aststack);
    curtemp = 1;

    /* make the arg */
    cur = allocPSLAstNode(pslast_arg, NULL, 0, 0, NULL);
    WRITE_BUFFER(aststack, &cur, 1);

    for (psli = 0; psli < psllen; psli++) {
        int arity = 0, pushes = 0;
        unsigned short cmd = psl[psli];
        unsigned char *data = NULL;
        size_t datasz = 0;

        /* maybe it has raw data */
        if (cmd >= psl_marker) {
            psli++;
            psli += pslBignumToInt(psl + psli, &datasz);

            /* make sure this doesn't go off the edge */
            if (psli + datasz <= psllen) {
                data = psl + psli;
                psli += datasz - 1;
            }
        }

        /* now get our arity info */
        switch (cmd) {
#define FOREACH(cmd)
#define LEAKY_PUSH(x)
#define ARITY(x) arity = x;
#define PUSHES(x) pushes = x;
#define LEAKA
#define LEAKB
#define LEAKC
#define LEAKP
#define LEAKALL
#include "psl-optim.c"
#undef FOREACH
#undef LEAKY_PUSH
#undef ARITY
#undef PUSHES
#undef LEAKA
#undef LEAKB
#undef LEAKC
#undef LEAKP
#undef LEAKALL

            default:
                arity = 0;
                pushes = 0;
        }

        /* if this has data, but the data is actually code, need to put that in place */
        if (cmd == psl_code || cmd == psl_immediate) {
            cur = pslToAst(data, datasz);
            data = NULL;
            datasz = 0;
            WRITE_BUFFER(aststack, &cur, 1);
            arity++;
        }

        /* if we need to flatten the stack (we're duplicating or removing stack elements), do so */
        if ((cmd >= psl_push0 && cmd <= psl_push7) || pushes != 1) {
            struct Buffer_PSLAstNode naststack;
            INIT_BUFFER(naststack);

            for (si = 0; si < aststack.bufused; si++) {
                if (aststack.buf[si]->cmd == pslast_gettemp) {
                    /* just copy it */
                    WRITE_BUFFER(naststack, aststack.buf + si, 1);

                } else {
                    cur = allocPSLAstNode(pslast_settemp, NULL, 0,
                            1, aststack.buf + si);
                    cur->temp0 = curtemp;
                    WRITE_BUFFER(astout, &cur, 1);

                    cur = allocPSLAstNode(pslast_gettemp, NULL, 0,
                            0, NULL);
                    cur->temp0 = curtemp;
                    WRITE_BUFFER(naststack, &cur, 1);

                    curtemp++;
                }
            }

            /* now replace aststack with the new one */
            aststack = naststack;
        }

        /* replace pushes by gettemps */
        if (cmd >= psl_push0 && cmd <= psl_push7) {
            int depth = cmd - psl_push0;
            if (aststack.bufused < depth + 1) {
                cur = allocPSLAstNode(psl_null, NULL, 0, 0, NULL);
            } else {
                cur = BUFFER_END(aststack)[-depth-1];
            }

            WRITE_BUFFER(aststack, &cur, 1);

        /* just perform pops */
        } else if (cmd == psl_pop) {
            if (aststack.bufused > 0)
                aststack.bufused--;

        /* resolve is weird */
        } else {
            /* figure out the mixed arity of an array */
            if (cmd == psl_array) {
                arity = 1;
                if (aststack.bufused > 0 && BUFFER_TOP(aststack)->cmd == psl_integer) {
                    /* OK, good, there's an integer. If it makes sense, get the value */
                    cur = BUFFER_TOP(aststack);
                    if (cur->children[0]->cmd == psl_raw) {
                        /* hooplah! Get the int val*/
                        struct PlofRawData *rd;
                        ptrdiff_t val;
                        cur = cur->children[0];
                        rd = newPlofRawDataNonAtomic(cur->datasz);
                        memcpy(rd->data, cur->data, cur->datasz);

                        val = parseRawInt(rd);

                        /* and extend the arity */
                        if (val > 0) arity += val;
                    }
                }
            }

            /* make sure we have all the children */
            while (aststack.bufused < arity) {
                /* need to put nulls at the start */
                while (BUFFER_SPACE(aststack) < 1) EXPAND_BUFFER(aststack);
                memmove(aststack.buf + 1, aststack.buf, aststack.bufused * sizeof(struct PSLAstNode *));
                aststack.buf[0] = allocPSLAstNode(psl_null, NULL, 0, 0, NULL);
                aststack.bufused++;
            }

            /* and create the current one */
            cur = allocPSLAstNode(cmd, data, datasz, arity, BUFFER_END(aststack) - arity);
            aststack.bufused -= arity;

            /* handle resolve's weird push */
            if (cmd == psl_resolve) {
                struct PSLAstNode *tmp;
                tmp = allocPSLAstNode(pslast_gettemp, NULL, 0, 0, NULL);
                tmp->temp0 = curtemp;
                WRITE_BUFFER(aststack, &tmp, 1);

                tmp = allocPSLAstNode(pslast_gettemp, NULL, 0, 0, NULL);
                tmp->temp0 = curtemp + 1;
                WRITE_BUFFER(aststack, &tmp, 1);

                cur->temp0 = curtemp;
                cur->temp1 = curtemp + 1;

                curtemp += 2;
            }

            /* put it either on the stack or in our instruction list */
            if (pushes == 1) {
                WRITE_BUFFER(aststack, &cur, 1);
            } else {
                WRITE_BUFFER(astout, &cur, 1);
            }

            /* FIXME: array, resolve special */
        }
    }

    WRITE_BUFFER(astout, aststack.buf, aststack.bufused);
    return allocPSLAstNode(pslast_seq, NULL, 0, astout.bufused, astout.buf);
}

/* dump an AST tree in ASCII */
void dumpPSLAst(FILE *to, struct PSLAstNode *tree, int spaces)
{
    int i;
    size_t si;

    for (i = 0; i < spaces; i++) fprintf(to, "  ");
    fprintf(to, "(");

    /* print the node name */
    switch (tree->cmd) {
#define FOREACH(pslcmd) \
        case psl_ ## pslcmd : \
            fprintf(to, "%s", #pslcmd); \
            break;
#include "psl_inst.h"
#undef FOREACH

        case pslast_seq:
            fprintf(to, "seq");
            break;

        case pslast_arg:
            fprintf(to, "arg");
            break;

        case pslast_settemp:
            fprintf(to, "settemp");
            break;

        case pslast_gettemp:
            fprintf(to, "gettemp");
            break;

        default:
            fprintf(to, "???");
    }

    fprintf(to, "\n");

    /* if it has temps, output those */
    if (tree->temp0) {
        for (i = 0; i < spaces; i++) fprintf(to, "  ");
        fprintf(to, "  %d", tree->temp0);
        if (tree->temp1) fprintf(to, " %d", tree->temp1);
        fprintf(to, "\n");
    }

    /* if it has data, output that */
    if (tree->datasz) {
        for (i = 0; i < spaces; i++) fprintf(to, "  ");
        if (tree->datasz == (size_t) -1) {
            fprintf(to, "  %d\n", (int) (size_t) tree->data);
        } else {
            fprintf(to, "  %.*s\n", (int) tree->datasz, (char *) tree->data);
        }
    }

    /* now print any children */
    for (si = 0; si < tree->childct; si++) {
        dumpPSLAst(to, tree->children[si], spaces + 1);
    }

    for (i = 0; i < spaces; i++) fprintf(to, "  ");
    fprintf(to, ")\n");
}

/* dump an AST tree in Dot */
void dumpPSLAstDot(FILE *to, struct PSLAstNode *tree)
{
    int i;
    size_t si;

    fprintf(to, "Ox%p [label=\"", tree);

    /* print the node name */
    switch (tree->cmd) {
#define FOREACH(pslcmd) \
        case psl_ ## pslcmd : \
            fprintf(to, "%s", #pslcmd); \
            break;
#include "psl_inst.h"
#undef FOREACH

        case pslast_seq:
            fprintf(to, "seq");
            break;

        case pslast_arg:
            fprintf(to, "arg");
            break;

        case pslast_settemp:
            fprintf(to, "settemp");
            break;

        case pslast_gettemp:
            fprintf(to, "gettemp");
            break;

        default:
            fprintf(to, "???");
    }

    /* if it has temps, output those */
    if (tree->temp0) {
        fprintf(to, "(%d", tree->temp0);
        if (tree->temp1) fprintf(to, ",%d", tree->temp1);
        fprintf(to, ")");
    }

    /* if it has data, output that */
    if (tree->datasz) {
        if (tree->datasz == (size_t) -1) {
            fprintf(to, "  %d", (int) (size_t) tree->data);
        } else {
            fprintf(to, "  \\\"%.*s\\\"", (int) tree->datasz, (char *) tree->data);
        }
    }

    fprintf(to, "\"];\n");

    /* now print any children */
    for (si = 0; si < tree->childct; si++) {
        fprintf(to, "Ox%p -> Ox%p;\n", tree, tree->children[si]);
        dumpPSLAstDot(to, tree->children[si]);
    }
}
