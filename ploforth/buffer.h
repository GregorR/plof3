/*
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

#ifndef BUFFER_H
#define BUFFER_H

#include <gc/gc.h>

#include "helpers.h"

#define BUFFER_DEFAULT_SIZE 1024

/* auto-expanding buffer */
#define BUFFER(name, type) \
typedef struct _Buffer_ ## name { \
    size_t bufsz, bufused; \
    type *buf; \
} Buffer_ ## name

BUFFER(char, char);
BUFFER(int, int);

/* initialize a buffer */
#define INIT_BUFFER(buffer) \
{ \
    (buffer).bufsz = BUFFER_DEFAULT_SIZE; \
    (buffer).bufused = 0; \
    SF((buffer).buf, GC_malloc, NULL, (BUFFER_DEFAULT_SIZE * sizeof(*(buffer).buf))); \
}

/* the address of the free space in the buffer */
#define BUFFER_END(buffer) ((buffer).buf + (buffer).bufused)

/* mark new bytes in a buffer */
#define STEP_BUFFER(buffer, by) ((buffer).bufused += by)

/* the amount of space left in the buffer */
#define BUFFER_SPACE(buffer) ((buffer).bufsz - (buffer).bufused)

/* get the top element of a buffer */
#define BUFFER_TOP(buffer) ((buffer).buf[(buffer).bufused - 1])

/* pop the top element off a buffer */
#define POP_BUFFER(buffer) ((buffer).bufused--)

/* expand a buffer */
#define EXPAND_BUFFER(buffer) \
{ \
    (buffer).bufsz *= 2; \
    SF((buffer).buf, GC_realloc, NULL, ((buffer).buf, (buffer).bufsz * sizeof(*(buffer).buf))); \
}

/* write a string to a buffer */
#define WRITE_BUFFER(buffer, string, len) \
{ \
    size_t _len = (len); \
    while (BUFFER_SPACE(buffer) < _len) { \
        EXPAND_BUFFER(buffer); \
    } \
    memcpy(BUFFER_END(buffer), string, _len * sizeof(*(buffer).buf)); \
    STEP_BUFFER(buffer, _len); \
}

#endif
