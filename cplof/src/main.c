/*
 * cplof, the frontend to the PSL interpreter and PRP for Plof files
 *
 * Copyright (C) 2007, 2008, 2009, 2010 Gregor Richards
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
    char *wdir, *wfil;
    unsigned char **path;
    struct Buffer_psl file;

    char *files[MAX_FILES+1];

    struct PlofObject *context;

    int fn, argn, compileOnly, interactive;

    struct Buffer_psl compileBuf;
    char *compileFile;

    struct PlofReturn plofRet;

    int plofargc;
    char **plofargv;

    GC_INIT();

    /* load std.psl by default */
    files[0] = "std.psl";
    fn = 1;
    compileOnly = 0;
    compileFile = "a.psl";
    interactive = 0;
    plofargc = 0;
    plofargv = NULL;

    /* handle args */
    for (argn = 1; argn < argc; argn++) {
        ARG("no-std", "N") {
            files[0] = NULL;

        } else ARG("compile", "c") {
            compileOnly = 1;

        } else ARG("output", "o") {
            compileOnly = 1;
            compileFile = argv[++argn];

        } else ARG("debug", "g") {
            prpDebug = 1;

        } else ARG("interactive", "i") {
            interactive = 1;

        } else ARG("no-intrinsics", "\xFF") {
            plofLoadIntrinsics = 0;

        } else ARG("help", "h") {
            usage();
            return 0;

        } else if (argv[argn][0] == '-' && argv[argn][1]) {
            fprintf(stderr, "Unrecognized option '%s'!\n\n", argv[argn]);
            usage();
            return 1;

        } else {
            files[fn++] = argv[argn];
            if (fn >= MAX_FILES) {
                fprintf(stderr, "Too many files!\n");
                return 1;
            } else if (files[0] && !compileOnly && fn >= 2) {
                /* we have an input file, we're not compiling, so the rest is Plof args */
                argn++;
                plofargc = argc - argn;
                plofargv = argv + argn;
                break;
            }

        }
    }
    files[fn] = NULL;

    /* get our search path */
    if (whereAmI(argv[0], &wdir, &wfil)) {
        int onIncPath = 0;

        plofIncludePaths = GC_MALLOC(16 * sizeof(unsigned char *));

        plofIncludePaths[onIncPath] = GC_MALLOC_ATOMIC(3);
        sprintf((char *) plofIncludePaths[onIncPath++], "./");

        if (prpDebug) {
            plofIncludePaths[onIncPath] = GC_MALLOC_ATOMIC(strlen(wdir) + 30);
            sprintf((char *) plofIncludePaths[onIncPath++], "%s/../share/plof_include/debug/", wdir);

            plofIncludePaths[onIncPath] = GC_MALLOC_ATOMIC(strlen(wdir) + 23);
            sprintf((char *) plofIncludePaths[onIncPath++], "%s/../../plofcore/debug/", wdir);
        }

        /* /../share/plof_include/ */
        plofIncludePaths[onIncPath] = GC_MALLOC_ATOMIC(strlen(wdir) + 24);
        sprintf((char *) plofIncludePaths[onIncPath++], "%s/../share/plof_include/", wdir);

        /* /../../plof_include/ (for running from src/) */
        plofIncludePaths[onIncPath] = GC_MALLOC_ATOMIC(strlen(wdir) + 17);
        sprintf((char *) plofIncludePaths[onIncPath++], "%s/../../plofcore/", wdir);

        plofIncludePaths[onIncPath] = NULL;

        /* FIXME: should support -I eventually */

    } else {
        fprintf(stderr, "Could not deterine include paths!\n");
        return 1;

    }

    /* complain if there aren't any files */
    if (fn == 1 && !interactive) {
        usage();
        return 1;
    }
    /* FIXME: should be in if (compileOnly), but -Wall complains */
    INIT_ATOMIC_BUFFER(compileBuf);
    
    /* Initialize null and global */
    plofNull = newPlofObject();
    plofNull->parent = plofNull;
    plofGlobal = newPlofObject();
    plofGlobal->parent = plofGlobal;

    /* transfer args */
    plofSetArgs(plofGlobal, (unsigned char *) "args", plofargc, plofargv);

    /* And the context */
    context = newPlofObject();
    context->parent = plofNull;

    /* load in the files */
    for (fn = 0; fn == 0 || files[fn]; fn++) {
        struct Buffer_psl psl;

        if (!files[fn]) continue;

        /* if we're only compiling, list the files */
        if (compileOnly) {
            fprintf(stderr, "Compiling %s\n", files[fn]);
        }

        INIT_ATOMIC_BUFFER(file);
        
        /* find the file */
        if (!strcmp(files[fn], "-")) {
            /* read from stdin */
            fh = stdin;
        } else {
            fh = fopen(files[fn], "rb");
        }
        if (fh == NULL) {
            for (path = plofIncludePaths; !fh && *path; path++) {
                char *file = GC_MALLOC_ATOMIC(strlen((char *) *path) + strlen(files[fn]) + 1);
                sprintf(file, "%s%s", (char *) *path, files[fn]);
                fh = fopen(file, "rb");
            }
        }
        if (fh == NULL) {
            perror(files[fn]);
            return 1;
        }

        /* read it */
        READ_FILE_BUFFER(file, fh);
        WRITE_BUFFER(file, "\0", 1);
        file.bufused--;
        fclose(fh);

        /* check what type of file it is */
        if (isPSLFile(file.bufused, file.buf)) {
            psl = readPSLFile(file.bufused, file.buf);

            /* run immediates */
            interpretPSL(context, plofNull, NULL, psl.bufused, psl.buf, 0, 1);
   
        } else {
            /* parse it */
            psl = parseAll(file.buf, (unsigned char *) "top", (unsigned char *) files[fn]);

        }

        /* Now interp */
        if (compileOnly) {
            if (fn > 0) {
                WRITE_BUFFER(compileBuf, psl.buf, psl.bufused);
            }
        } else {
            plofRet = interpretPSL(context, plofNull, NULL, psl.bufused, psl.buf, 0, 0);
            if (plofRet.isThrown) {
                plofThrewUp(plofRet.ret);
                return 1;
            }
        }
    }

    /* perhaps go into interactive mode */
    if (interactive) {
        struct Buffer_char input;
        struct Buffer_psl psl;

        INIT_ATOMIC_BUFFER(input);

        /* read stuff */
        while (1) {
            if (input.bufused == 0) {
                printf("> ");
            } else {
                printf("# ");
            }
            fflush(stdout);

            /* get this line */
            while (BUFFER_SPACE(input) < 1024) EXPAND_BUFFER(input);
            if (fgets(input.buf + input.bufused, 1024, stdin)) {
                size_t newsz = strlen(input.buf);
                if (newsz == input.bufused + 1) {
                    input.bufused = 0;
                    input.buf[0] = '\0';
                    continue;
                }
                input.bufused = newsz;
            } else {
                break;
            }

            /* try to parse it */
            psl = parseOne((unsigned char *) input.buf, (unsigned char *) "top",
                           (unsigned char *) "<stdin>", 0, 0).code;
            if (psl.buf == NULL) {
                /* didn't parse */
                continue;
            }

            /* get rid ouf our input */
            input.bufused = 0;
            input.buf[0] = '\0';

            /* then run it */
            interpretPSL(context, plofNull, NULL, psl.bufused, psl.buf, 0, 1);
            if (compileOnly) {
                WRITE_BUFFER(compileBuf, psl.buf, psl.bufused);
            } else {
                plofRet = interpretPSL(context, plofNull, NULL, psl.bufused, psl.buf, 0, 0);
                if (plofRet.isThrown) {
                    plofThrewUp(plofRet.ret);
                }
            }
        }
    }


    if (compileOnly) {
        FILE *pslf;

        /* then write it out */
        pslf = fopen(compileFile, "wb");
        if (pslf == NULL) {
            perror(compileFile);
            return 1;
        }
    
        writePSLFile(pslf, compileBuf.bufused, compileBuf.buf, 0);
    
        fclose(pslf);
    }


    return 0;
}

void usage()
{
    fprintf(stderr,
            "Use: cplof [options] <files>\n"
            "Options:\n"
            "  --no-std|-N:\n"
            "\tDon't load std.psl\n"
            "  --compile|-c:\n"
            "\tCompile only, don't run.\n"
            "  --output|-o <file>:\n"
            "\tOutput filename (implies -c).\n"
            "  --debug|-g:\n"
            "\tCause the parser to produce debuggable output.\n"
            "  --interactive|-i:\n"
            "\tInteractive (read-execute-loop) mode.\n"
            "  --no-intrinsics:\n"
            "\tDo not load intrinsics (much slower execution).\n");
}
