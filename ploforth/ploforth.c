/*
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
#include <stdlib.h>
#include <string.h>

#include <gc/gc.h>

#include "ploforth.h"

#include "jump.h"
#include "token.h"

#ifdef jumpenum
enum jumplabel {
    interp_top,
#define FOREACH(func, name) \
    interp_ ## func,
#include "builtins.h"
#undef FOREACH
};
#endif

/* compiled code is an array of void *'s. Modulo 0 are code pointers, modulo 1
 * are arg pointers */

/* run compiled Ploforth code, or generate the initial dictionary (which needs
 * to be done here because of how labels work) */
PloforthState runPloforth(unsigned char *code, PloforthState *istate)
{
    PloforthState state;

    /* initialize the state */
    if (istate) {
        state = *istate;
        if (code != NULL)
            state.code = code;
    } else {
        Dict *dict, *ndict;
        void **toploop;

        state.compiling = 0;
        state.code = code;
        INIT_BUFFER(state.cstack);
        INIT_BUFFER(state.dstack);

        /* top-level is just an infinite loop of 'run' */
        toploop = (void **) GC_MALLOC_ATOMIC(4*sizeof(void*));
        toploop[0] = toploop[1] = toploop[3] = NULL;
        toploop[2] = addressof(interp_run);
        WRITE_BUFFER(state.cstack, &toploop, 1);

        /* generate the initial dictionary */
        dict = NULL;
#define FOREACH(func, name) \
        { \
            ndict = GC_NEW(Dict); \
            ndict->next = dict; \
            ndict->word = name; \
            ndict->code = addressof(interp_ ## func); \
            ndict->arg = NULL; \
            dict = ndict; \
        }
#include "builtins.h"
#undef FOREACH
        state.dict = dict;
    }

    /* if they gave us no code, they just want an initial state */
    if (code == NULL)
        return state;

    /* step after interpreting a command */
#define JUMP \
    { \
        jump(*BUFFER_TOP(state.cstack)); \
    }
#define STEP \
    { \
        BUFFER_TOP(state.cstack) += 2; /* immediate next is arg */ \
        JUMP; \
    }

    /* now the interpreter itself */
    STEP;
    prejump(*BUFFER_TOP(state.cstack));

    /* header for commands */
#ifdef DEBUG
#define DEBUG_CMD(cmd) \
    fprintf(stderr, "%s\n", #cmd);
#else
#define DEBUG_CMD(cmd)
#endif
#define CMD(cmd) \
label(cmd); \
    DEBUG_CMD(cmd);

    /* get the argument from the current command */
#define ARG (BUFFER_TOP(state.cstack)[1])

    jumphead;

CMD(interp_run);
    /* run/compile ASCII code */
    {
        char *token;
        Dict *cdict;

        /* this is an infinite loop */
        BUFFER_TOP(state.cstack) -= 2;

        /* get a token */
        token = consumeToken(&state.code);

        if (token == NULL) return state;

        /* find the matching dictionary entry */
        for (cdict = state.dict; cdict && strcmp(cdict->word, token); cdict = cdict->next);

        /* if nothing was found, bad */
        if (cdict == NULL) return state; /* FIXME: not a useful return */

        /* do this before coming back to interp_run again */
        memcpy(BUFFER_TOP(state.cstack), &cdict->code, 2 * sizeof(void*));

        JUMP;
    }

CMD(interp_nop);
    STEP;

CMD(interp_hello);
    printf("Hello, world!\n");
    STEP;

CMD(interp_define);
    /* go into compiling mode */
    state.compiling = 1;

    STEP;

CMD(interp_enddef);
    STEP;

CMD(interp_exit);
    /* pop off the call stack and continue */
    POP_BUFFER(state.cstack);

    /* make sure we're not exiting scope */
    if (state.cstack.bufused <= 0) {
        return state;
    }

    STEP;

CMD(interp_interp);
    {
        /* the argument is code to be run */
        void **arg = ((void **) ARG);

        /* push it on the buffer */
        WRITE_BUFFER(state.cstack, &arg, 1);

        /* and run it */
        JUMP;
    }

    jumptail;
}
