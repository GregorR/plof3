/*
 * Lexer for ASCII PSL
 *
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

#include <string.h>

static int iswhite(unsigned char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static int isalphanum(unsigned char c)
{
    return (
        (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c == '_')
    );
}

static int isspecial(unsigned char c)
{
    return (c == '/' || c == '"' || c == '{' || c == '[' || c == ']' || c == '}');
}

/* lex a single token out of an APSL string */
unsigned char *pslTok(unsigned char **apslp, unsigned char *intobuf, size_t intosz)
{
    unsigned char *apsl = *apslp;
    unsigned char *token;
    size_t tokensz;

    /* first skip any whitespace and comments */
    for (; iswhite(*apsl); apsl++);
    while (*apsl == '/') {
        if (apsl[1] == '*') {
            /* skip to the matching * and / */
            for (apsl += 2; *apsl && (apsl[0] != '*' || apsl[1] != '/'); apsl++);
        } else if (apsl[1] == '/') {
            /* skip to the end of the line */
            for (apsl += 2; *apsl && *apsl != '\n'; apsl++);
        } else {
            break;
        }
        for (; iswhite(*apsl); apsl++);
    }

    /* if there's no string left, there's no tokens left */
    if (*apsl == '\0') return NULL;
    token = apsl;

    /* now decide if it's alphanumeric or symbol */
    if (isalphanum(*apsl)) {
        /* find the end */
        for (apsl++; isalphanum(*apsl); apsl++);

    } else if (*apsl == '"') {
        /* a string */
        for (apsl++; *apsl &&
                     *apsl != '"';
                     apsl++) {
            if (*apsl == '\\') apsl++;
        }
        if (*apsl == '"') apsl++;

    } else if (isspecial(*apsl)) {
        apsl++;

    } else {
        /* symbolic, find the end */
        for (apsl++; *apsl &&
                     !iswhite(*apsl) &&
                     !isalphanum(*apsl) &&
                     *apsl != '/';
                     apsl++);

    }

    /* and copy it in */
    tokensz = apsl - token;
    if (tokensz >= intosz) tokensz = intosz - 1;
    strncpy(intobuf, token, tokensz);
    intobuf[tokensz] = '\0';
    *apslp = apsl;

    return intobuf;
}
