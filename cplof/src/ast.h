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

#ifndef AST_H
#define AST_H

#include <stdio.h>

#define BUFFER_GC
#include "plof/buffer.h"
#include "plof/plof.h"

/* A PSL AST node is fairly generic */
struct PSLAstNode {
    unsigned short cmd;
    unsigned short temp0, temp1;
    unsigned char *data;
    size_t datasz;
    size_t childct;
    struct PSLAstNode *children[1];
};

BUFFER(PSLAstNode, struct PSLAstNode *);

/* commands unique to PSL AST nodes */
#define pslast_seq      0x100
#define pslast_settemp  0x101
#define pslast_gettemp  0x102

/* allocate a PSL AST node with these children */
struct PSLAstNode *allocPSLAstNode(unsigned short cmd,
                                   unsigned char *data, size_t datasz,
                                   size_t childct, struct PSLAstNode **children);

/* turn this PSL into a PSL AST */
struct PSLAstNode *pslToAst(unsigned char *psl, size_t psllen);

/* dump an AST tree in ASCII */
void dumpPSLAst(FILE *to, struct PSLAstNode *tree, int spaces);

#endif
