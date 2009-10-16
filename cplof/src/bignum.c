/*
 * Support for reading/writing PSL bignums
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

#include "bignum.h"

/* Convert a PSL bignum to an int */
int pslBignumToInt(unsigned char *bignum, size_t *into)
{
    size_t ret = 0;
    unsigned c = 0;

    for (;; bignum++) {
        c++;
        ret <<= 7;
        ret |= ((*bignum) & 0x7F);
        if ((*bignum) < 128) break;
    }

    *into = ret;

    return c;
}

/* Determine the number of bytes a bignum of a particular number will take */
size_t pslBignumLength(size_t val)
{
    if (val < ((size_t) 1<<7)) {
        return 1;
    } else if (val < ((size_t) 1<<14)) {
        return 2;
#if SIZEOF_VOID_P <= 2
    } else {
        return 3;
#else
    } else if (val < ((size_t) 1<<21)) {
        return 3;
    } else if (val < ((size_t) 1<<28)) {
        return 4;
#if SIZEOF_VOID_P <= 4
    } else {
        return 5;
#else
    } else if (val < ((size_t) 1<<35)) {
        return 5;
    } else if (val < ((size_t) 1<<42)) {
        return 6;
    } else if (val < ((size_t) 1<<49)) {
        return 7;
    } else if (val < ((size_t) 1<<56)) {
        return 8;
    } else if (val < ((size_t) 1<<63)) {
        return 9;
    } else {
        return 10;
#endif /* 32 */
#endif /* 16 */
    }
}

/* Write a bignum into a buffer */
void pslIntToBignum(unsigned char *buf, size_t val, size_t len)
{
    buf[--len] = val & 0x7F;
    val >>= 7;

    for (len--; len != (size_t) -1; len--) {
        buf[len] = (val & 0x7F) | 0x80;
        val >>= 7;
    }
}
