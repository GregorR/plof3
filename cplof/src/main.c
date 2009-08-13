/*
 * cplof, the frontend to the PSL interpreter and PRP for Plof files
 *
 * Copyright (c) 2007, 2008, 2009 Gregor Richards
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

#include "bignum.h"
#include "plof.h"
#include "prp.h"
#include "psl.h"
#include "whereami.h"

#define BUFSTEP 1024
#define MAX_FILES 32

#define ARG(LONG, SHORT) if(!strcmp(argv[argn], "--" LONG) || !strcmp(argv[argn], "-" SHORT))
void usage();

int main(int argc, char **argv)
{
    FILE *fh;
    char *wdir, *wfil;
    struct Buffer_psl file;

    char *files[MAX_FILES+1];

    struct PlofObject *context;

    int i, fn, argn, compileOnly;

    struct Buffer_psl compileBuf;

    GC_INIT();

    /* get our search path */
    if (whereAmI(argv[0], &wdir, &wfil)) {
        plofIncludePaths = GC_MALLOC(3 * sizeof(unsigned char *));

        /* /../share/plof_include/ */
        plofIncludePaths[0] = GC_MALLOC_ATOMIC(strlen(wdir) + 24);
        sprintf((char *) plofIncludePaths[0], "%s/../share/plof_include/", wdir);

        /* /../../plof_include/ (for running from src/) */
        plofIncludePaths[1] = GC_MALLOC_ATOMIC(strlen(wdir) + 21);
        sprintf((char *) plofIncludePaths[1], "%s/../../plof_include/", wdir);

        plofIncludePaths[2] = NULL;

        /* FIXME: should support -I eventually */

    } else {
        fprintf(stderr, "Could not deterine include paths!\n");
        return 1;

    }

    /* load std.psl by default */
    files[0] = "std.psl";
    fn = 1;
    compileOnly = 0;

    /* handle args */
    for (argn = 1; argn < argc; argn++) {
        ARG("no-std", "N") {
            files[0] = NULL;

        } else ARG("compile", "c") {
            compileOnly = 1;

        } else ARG("help", "h") {
            usage();
            return 0;

        } else if (argv[argn][0] == '-') {
            fprintf(stderr, "Unrecognized option '%s'!\n\n", argv[argn]);
            usage();
            return 1;

        } else {
            files[fn++] = argv[++i];
            if (fn >= MAX_FILES) {
                fprintf(stderr, "Too many files!\n");
                return 1;
            }

        }
    }
    files[fn] = NULL;

    /* complain if there aren't any files */
    if (fn == 1) {
        usage();
        return 1;
    }
    if (compileOnly) {
        INIT_BUFFER(compileBuf);
    }
    
    /* Initialize null and global */
    plofNull = GC_NEW_Z(struct PlofObject);
    plofNull->parent = plofNull;
    plofGlobal = GC_NEW_Z(struct PlofObject);
    plofGlobal->parent = plofGlobal;

    /* And the context */
    context = GC_NEW_Z(struct PlofObject);
    context->parent = plofNull;

    /* load in the files */
    for (fn = 0; fn == 0 || files[fn]; fn++) {
        size_t slen, stype, stl;

        if (!files[fn]) continue;

        INIT_BUFFER(file);
        
        /* open the file */
        fh = fopen(files[fn], "rb");
        if (fh == NULL) {
            perror(files[fn]);
            return 1;
        }

        /* read it */
        READ_FILE_BUFFER(file, fh);
        fclose(fh);
    
        /* FIXME: bounds checking */

        /* check what type of file it is */
        if (!memcmp(file.buf, PSL_FILE_MAGIC, sizeof(PSL_FILE_MAGIC)-1)) {
            /* Go section-by-section */
            for (i = 8; i < file.bufused;) {
                /* section length */
                i += pslBignumToInt(file.buf + i, (size_t *) &slen);
        
                /* section type */
                stl = pslBignumToInt(file.buf + i, (size_t *) &stype);
        
                /* if it's program data, we found it */
                if (stype == 0) {
                    i += stl;
                    break;
        
                } else {
                    /* skip this section */
                    i += slen;
                }
            }
        
            /* make sure we found it */
            if (i >= file.bufused) {
                fprintf(stderr, "No program data!\n");
                return 1;
            }
    
            /* Now interp */
            interpretPSL(context, plofNull, NULL, slen - stl, file.buf + i, 0, 1);
            if (compileOnly) {
                if (fn > 0) {
                    WRITE_BUFFER(compileBuf, file.buf + i, slen - stl);
                }
            } else {
                interpretPSL(context, plofNull, NULL, slen - stl, file.buf + i, 0, 0);
            }

        } else {
            struct Buffer_psl psl = parseAll(file.buf, (unsigned char *) "top", (unsigned char *) files[fn]);
            fprintf(stderr, "%d\n", (int) psl.bufused);

        }
    }

    return 0;
}

void usage()
{
    fprintf(stderr,
            "Use: cplof [options] <files>\n"
            "Options:\n"
            "  --no-std|-N:\n"
            "\tDon't load std.psl\n");
}
