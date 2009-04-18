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

#include <stdlib.h>

static inline int iswhite(unsigned char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

/* consume a token out of an input stream. Returns the token and modifies the
 * input stream pointer to point after the token. Returns NULL if the input
 * stream is out of tokens. */
unsigned char *consumeToken(unsigned char **streamp)
{
    unsigned char *stream = *streamp, *token;

    /* first consume any whitespace */
    for (; iswhite(*stream); stream++);

    /* if we're at the end of the stream, no more tokens */
    if (*stream == '\0') return NULL;

    /* otherwise, find the end of the token */
    token = stream;
    for (; !iswhite(*stream) && *stream != '\0'; stream++);

    /* null it to cap off the token */
    if (*stream != '\0')
        *(stream++) = '\0';

    /* save it back */
    *streamp = stream;

    /* and give the token */
    return token;
}
