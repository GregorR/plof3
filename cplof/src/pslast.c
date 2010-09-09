/*
 * pslast, an example for ast.c
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
#include <string.h>

#include "ast.h"
#include "plof/bignum.h"
#include "plof/memory.h"
#include "plof/plof.h"
#include "plof/prp.h"
#include "plof/psl.h"
#include "plof/pslfile.h"
#include "whereami.h"

#define BUFSTEP 1024
#define MAX_FILES 32

#define ARG(LONG, SHORT) if(!strcmp(argv[argn], "--" LONG) || !strcmp(argv[argn], "-" SHORT))
void usage();

int main(int argc, char **argv)
{
    FILE *fh;
    struct Buffer_psl file;
    char *filenm = NULL, *arg;
    struct Buffer_psl psl;
    int i, dot;
    struct PSLAstNode *ast;

    GC_INIT();

    dot = 0;
    for (i = 1; i < argc; i++) {
        arg = argv[i];
        if (arg[0] == '-') {
            if (!strcmp(arg, "-d")) {
                dot = 1;
            } else {
                usage();
                return 1;
            }
        } else {
            filenm = arg;
        }
    }

    if (filenm == NULL) {
        usage();
        return 1;
    }

    /* load in the file */
    INIT_BUFFER(file);

    /* find the file */
    fh = fopen(filenm, "rb");
    if (fh == NULL) {
        perror(filenm);
        return 1;
    }

    /* read it */
    READ_FILE_BUFFER(file, fh);
    WRITE_BUFFER(file, "\0", 1);
    file.bufused--;
    fclose(fh);

    /* FIXME: bounds checking */

    /* check what type of file it is */
    psl = readPSLFile(file.bufused, file.buf);

    ast = pslToAst(psl.buf, psl.bufused);
    if (!dot) {
        dumpPSLAst(stdout, ast, 0);
    } else {
        printf("digraph {\nnodesep=0;\nranksep=0;\n");
        dumpPSLAstDot(stdout, ast);
        printf("}\n");
    }
    return 0;
}

void usage()
{
    fprintf(stderr, "Use: pslast [-d] <psl file>\n");
}
